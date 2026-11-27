/* pthread.c - the win32 target's pthread, scheduling and
 * semaphore surface, implemented over the Win32 threading
 * primitives.  build.sh compiles this file with the win32
 * cross compiler and merges the object into libtcc1.a, which
 * every program links, so the surface is available with no
 * extra link flag and the resulting program depends only on
 * the Windows system DLLs.
 *
 * Mapping: a thread is a _beginthreadex thread; a mutex is a
 * CRITICAL_SECTION plus owner bookkeeping (the section itself
 * is always recursive, so the POSIX mutex types are enforced
 * in this layer); a condition variable is a
 * CONDITION_VARIABLE used with the mutex's section; a rwlock
 * is an SRWLOCK; a key is a Win32 TLS slot with a registered
 * destructor; an unnamed semaphore is a Win32 semaphore.
 * Cancellation is deferred (see pthread.h). */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <semaphore.h>

#ifndef ETIMEDOUT
#define ETIMEDOUT 138
#endif
#ifndef EOVERFLOW
#define EOVERFLOW 132
#endif
#ifndef ENOTSUP
#define ENOTSUP 129
#endif

/* Primitives the bundled winapi headers predate; the exports
 * themselves are in kernel32 (see win32/lib/kernel32.def). */
typedef struct { void *Ptr; } tinyc_CONDITION_VARIABLE;
typedef struct { void *Ptr; } tinyc_SRWLOCK;
__declspec(dllimport) BOOL __stdcall SleepConditionVariableCS(
    tinyc_CONDITION_VARIABLE *cv, CRITICAL_SECTION *cs,
    DWORD milliseconds);
__declspec(dllimport) void __stdcall WakeConditionVariable(
    tinyc_CONDITION_VARIABLE *cv);
__declspec(dllimport) void __stdcall WakeAllConditionVariable(
    tinyc_CONDITION_VARIABLE *cv);
__declspec(dllimport) void __stdcall AcquireSRWLockExclusive(
    tinyc_SRWLOCK *lock);
__declspec(dllimport) void __stdcall AcquireSRWLockShared(
    tinyc_SRWLOCK *lock);
__declspec(dllimport) BOOL __stdcall TryAcquireSRWLockExclusive(
    tinyc_SRWLOCK *lock);
__declspec(dllimport) BOOL __stdcall TryAcquireSRWLockShared(
    tinyc_SRWLOCK *lock);
__declspec(dllimport) void __stdcall ReleaseSRWLockExclusive(
    tinyc_SRWLOCK *lock);
__declspec(dllimport) void __stdcall ReleaseSRWLockShared(
    tinyc_SRWLOCK *lock);
__declspec(dllimport) ULONGLONG __stdcall GetTickCount64(void);

/* ---------------------------------------------------------------- */
/* Thread control block                                             */

struct tinyc_cb {
    HANDLE handle;              /* real thread handle */
    unsigned int id;            /* Win32 thread id */
    void *retval;
    volatile long state;        /* 0 running, 1 start routine returned */
    volatile long detached;
    volatile long claimed;      /* finalizer CAS guard */
    volatile long foreign;      /* not created by pthread_create */
    volatile long cancel_pending;
    volatile long cancel_state;
    volatile long cancel_type;
    struct _tinyc_cleanup *cleanups;
};

static DWORD g_cb_fls = FLS_OUT_OF_INDEXES;

static struct tinyc_cb *cb_of(pthread_t t)
{
    return (struct tinyc_cb *)t._cb;
}

static void run_key_destructors(void);
static void finalize_cb(struct tinyc_cb *cb);

static void __stdcall cb_fls_cleanup(void *data)
{
    struct tinyc_cb *cb = (struct tinyc_cb *)data;
    if (!cb)
        return;
    run_key_destructors();
    if (cb->detached)
        finalize_cb(cb);
}

static void ensure_fls(void)
{
    if (g_cb_fls == FLS_OUT_OF_INDEXES) {
        DWORD v = FlsAlloc(cb_fls_cleanup);
        if (InterlockedCompareExchange((volatile LONG *)&g_cb_fls,
                                       (LONG)v, (LONG)FLS_OUT_OF_INDEXES)
                != (LONG)FLS_OUT_OF_INDEXES && v != FLS_OUT_OF_INDEXES)
            FlsFree(v);
    }
}

static struct tinyc_cb *self_cb(void)
{
    struct tinyc_cb *cb;
    ensure_fls();
    if (g_cb_fls == FLS_OUT_OF_INDEXES)
        return NULL;
    cb = (struct tinyc_cb *)FlsGetValue(g_cb_fls);
    if (!cb) {
        cb = (struct tinyc_cb *)calloc(1, sizeof *cb);
        if (!cb)
            return NULL;
        cb->foreign = 1;
        cb->detached = 1;
        cb->cancel_state = PTHREAD_CANCEL_ENABLE;
        cb->cancel_type = PTHREAD_CANCEL_DEFERRED;
        cb->id = GetCurrentThreadId();
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
                        GetCurrentProcess(), &cb->handle, 0, FALSE,
                        DUPLICATE_SAME_ACCESS);
        FlsSetValue(g_cb_fls, cb);
    }
    return cb;
}

static void finalize_cb(struct tinyc_cb *cb)
{
    if (InterlockedCompareExchange(&cb->claimed, 1, 0) != 0)
        return;
    if (cb->handle)
        CloseHandle(cb->handle);
    free(cb);
}

/* Run the cleanup stack and the key destructors, then end the
 * calling thread with cb's return value set by the caller. */
static void thread_end(struct tinyc_cb *cb)
{
    while (cb->cleanups) {
        struct _tinyc_cleanup *cl = cb->cleanups;
        cb->cleanups = cl->_next;
        if (cl->_routine)
            cl->_routine(cl->_arg);
    }
    cb->state = 1;
    if (cb->foreign)
        ExitThread(0);
    _endthreadex(0);
}

static void deliver_cancel(struct tinyc_cb *cb)
{
    cb->retval = PTHREAD_CANCELED;
    thread_end(cb);
}

/* A deferred cancellation point: deliver a pending cancel. */
static void cancel_point(struct tinyc_cb *cb)
{
    if (cb && cb->cancel_pending &&
        cb->cancel_state == PTHREAD_CANCEL_ENABLE)
        deliver_cancel(cb);
}

struct tinyc_start {
    void *(*fn)(void *);
    void *arg;
    struct tinyc_cb *cb;
};

static unsigned __stdcall thread_main(void *data)
{
    struct tinyc_start *st = (struct tinyc_start *)data;
    struct tinyc_cb *cb = st->cb;
    ensure_fls();
    if (g_cb_fls != FLS_OUT_OF_INDEXES)
        FlsSetValue(g_cb_fls, cb);
    cb->retval = st->fn(st->arg);
    free(st);
    thread_end(cb);
    return 0;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start)(void *), void *arg)
{
    struct tinyc_cb *cb;
    struct tinyc_start *st;
    unsigned int id = 0;
    size_t stacksize = 0;

    if (!thread || !start)
        return EINVAL;
    cb = (struct tinyc_cb *)calloc(1, sizeof *cb);
    st = (struct tinyc_start *)malloc(sizeof *st);
    if (!cb || !st) {
        free(cb);
        free(st);
        return EAGAIN;
    }
    cb->cancel_state = PTHREAD_CANCEL_ENABLE;
    cb->cancel_type = PTHREAD_CANCEL_DEFERRED;
    if (attr) {
        stacksize = attr->stacksize;
        if (attr->detachstate == PTHREAD_CREATE_DETACHED)
            cb->detached = 1;
    }
    st->fn = start;
    st->arg = arg;
    st->cb = cb;
    cb->handle = (HANDLE)_beginthreadex(NULL, (unsigned)stacksize,
                                         thread_main, st,
                                         CREATE_SUSPENDED, &id);
    if (!cb->handle) {
        free(cb);
        free(st);
        return EAGAIN;
    }
    cb->id = id;
    thread->_handle = cb->handle;
    thread->_cb = cb;
    ResumeThread(cb->handle);
    return 0;
}

void pthread_exit(void *retval)
{
    struct tinyc_cb *cb = self_cb();
    if (!cb)
        ExitThread(0);
    cb->retval = retval;
    thread_end(cb);
}

int pthread_join(pthread_t thread, void **retval)
{
    struct tinyc_cb *cb = cb_of(thread);
    struct tinyc_cb *me = self_cb();
    if (!cb)
        return ESRCH;
    if (cb == me)
        return EDEADLK;
    if (cb->detached)
        return EINVAL;
    for (;;) {
        DWORD r = WaitForSingleObject(cb->handle, 50);
        if (r == WAIT_OBJECT_0)
            break;
        if (r != WAIT_TIMEOUT)
            return ESRCH;
        cancel_point(me);
    }
    if (retval)
        *retval = cb->retval;
    finalize_cb(cb);
    return 0;
}

int pthread_detach(pthread_t thread)
{
    struct tinyc_cb *cb = cb_of(thread);
    if (!cb)
        return ESRCH;
    if (cb->detached)
        return EINVAL;
    InterlockedExchange(&cb->detached, 1);
    if (cb->state)
        finalize_cb(cb);
    return 0;
}

pthread_t pthread_self(void)
{
    pthread_t t;
    struct tinyc_cb *cb = self_cb();
    t._handle = cb ? cb->handle : NULL;
    t._cb = cb;
    return t;
}

int pthread_equal(pthread_t a, pthread_t b)
{
    return a._cb == b._cb;
}

int pthread_yield(void)
{
    SwitchToThread();
    return 0;
}

int pthread_cancel(pthread_t thread)
{
    struct tinyc_cb *cb = cb_of(thread);
    if (!cb)
        return ESRCH;
    InterlockedExchange(&cb->cancel_pending, 1);
    return 0;
}

int pthread_setcancelstate(int state, int *oldstate)
{
    struct tinyc_cb *cb = self_cb();
    if (!cb)
        return ESRCH;
    if (state != PTHREAD_CANCEL_ENABLE &&
        state != PTHREAD_CANCEL_DISABLE)
        return EINVAL;
    if (oldstate)
        *oldstate = (int)cb->cancel_state;
    InterlockedExchange(&cb->cancel_state, state);
    if (state == PTHREAD_CANCEL_ENABLE)
        cancel_point(cb);
    return 0;
}

int pthread_setcanceltype(int type, int *oldtype)
{
    struct tinyc_cb *cb = self_cb();
    if (!cb)
        return ESRCH;
    if (type != PTHREAD_CANCEL_DEFERRED &&
        type != PTHREAD_CANCEL_ASYNCHRONOUS)
        return EINVAL;
    if (oldtype)
        *oldtype = (int)cb->cancel_type;
    InterlockedExchange(&cb->cancel_type, type);
    return 0;
}

void pthread_testcancel(void)
{
    cancel_point(self_cb());
}

static void (*g_atfork_prepare)(void);
static void (*g_atfork_parent)(void);
static void (*g_atfork_child)(void);

int pthread_atfork(void (*prepare)(void), void (*parent)(void),
                   void (*child)(void))
{
    g_atfork_prepare = prepare;
    g_atfork_parent = parent;
    g_atfork_child = child;
    return 0;
}

int pthread_getconcurrency(void)
{
    return 0;
}

int pthread_setconcurrency(int level)
{
    (void)level;
    return 0;
}

void _tinyc_cleanup_push(struct _tinyc_cleanup *cl)
{
    struct tinyc_cb *cb = self_cb();
    if (!cb)
        return;
    cl->_next = cb->cleanups;
    cb->cleanups = cl;
}

void _tinyc_cleanup_pop(struct _tinyc_cleanup *cl, int execute)
{
    struct tinyc_cb *cb = self_cb();
    struct _tinyc_cleanup **pp;
    if (!cb)
        return;
    pp = &cb->cleanups;
    while (*pp && *pp != cl)
        pp = &(*pp)->_next;
    if (*pp == cl)
        *pp = cl->_next;
    if (execute && cl->_routine)
        cl->_routine(cl->_arg);
}

/* ---------------------------------------------------------------- */
/* Once                                                             */

int pthread_once(pthread_once_t *once, void (*init)(void))
{
    if (!once || !init)
        return EINVAL;
    for (;;) {
        long s = InterlockedCompareExchange(
            (volatile LONG *)&once->_state, 2, 0);
        if (s == 1)
            return 0;
        if (s == 0) {
            init();
            InterlockedExchange((volatile LONG *)&once->_state, 1);
            return 0;
        }
        Sleep(1);
    }
}

/* ---------------------------------------------------------------- */
/* Keys (Win32 TLS slots with registered destructors)               */

#define TINYC_KEYS_MAX 128
static struct {
    DWORD tls;
    void (*destructor)(void *);
    volatile long used;
} g_keys[TINYC_KEYS_MAX];
static volatile long g_keys_lock;

static void spin_lock(volatile long *l)
{
    while (InterlockedExchange(l, 1) != 0)
        Sleep(0);
}

static void spin_unlock(volatile long *l)
{
    InterlockedExchange(l, 0);
}

static void run_key_destructors(void)
{
    int iter, i, progress;
    for (iter = 0; iter < PTHREAD_DESTRUCTOR_ITERATIONS; iter++) {
        progress = 0;
        for (i = 0; i < TINYC_KEYS_MAX; i++) {
            void *v;
            if (!g_keys[i].used || !g_keys[i].destructor)
                continue;
            v = TlsGetValue(g_keys[i].tls);
            if (v) {
                TlsSetValue(g_keys[i].tls, NULL);
                g_keys[i].destructor(v);
                progress = 1;
            }
        }
        if (!progress)
            break;
    }
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    int i;
    if (!key)
        return EINVAL;
    spin_lock(&g_keys_lock);
    for (i = 0; i < TINYC_KEYS_MAX; i++) {
        if (!g_keys[i].used) {
            DWORD tls = TlsAlloc();
            if (tls == TLS_OUT_OF_INDEXES) {
                spin_unlock(&g_keys_lock);
                return EAGAIN;
            }
            g_keys[i].tls = tls;
            g_keys[i].destructor = destructor;
            g_keys[i].used = 1;
            spin_unlock(&g_keys_lock);
            *key = (pthread_key_t)i;
            return 0;
        }
    }
    spin_unlock(&g_keys_lock);
    return EAGAIN;
}

int pthread_key_delete(pthread_key_t key)
{
    if (key >= TINYC_KEYS_MAX || !g_keys[key].used)
        return EINVAL;
    spin_lock(&g_keys_lock);
    TlsFree(g_keys[key].tls);
    g_keys[key].used = 0;
    g_keys[key].destructor = NULL;
    spin_unlock(&g_keys_lock);
    return 0;
}

void *pthread_getspecific(pthread_key_t key)
{
    if (key >= TINYC_KEYS_MAX || !g_keys[key].used)
        return NULL;
    return TlsGetValue(g_keys[key].tls);
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
    if (key >= TINYC_KEYS_MAX || !g_keys[key].used)
        return EINVAL;
    return TlsSetValue(g_keys[key].tls, (void *)value) ? 0 : EINVAL;
}

/* ---------------------------------------------------------------- */
/* Mutexes                                                          */

static CRITICAL_SECTION *mutex_cs(pthread_mutex_t *m)
{
    return (CRITICAL_SECTION *)m->_cs;
}

static int mutex_lazy_init(pthread_mutex_t *m)
{
    for (;;) {
        unsigned int r = (unsigned int)InterlockedCompareExchange(
            (volatile LONG *)&m->_ready, 2, 0);
        if (r == 1)
            return 0;
        if (r == 0) {
            InitializeCriticalSection(mutex_cs(m));
            InterlockedExchange((volatile LONG *)&m->_ready, 1);
            return 0;
        }
        Sleep(0);
    }
}

int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr)
{
    if (!mutex)
        return EINVAL;
    memset(mutex, 0, sizeof *mutex);
    mutex->_type = attr ? attr->_type : PTHREAD_MUTEX_NORMAL;
    mutex->_ceil = attr ? attr->_ceil : 0;
    mutex_lazy_init(mutex);
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    if (mutex->_owner != 0)
        return EBUSY;
    if (mutex->_ready == 1) {
        DeleteCriticalSection(mutex_cs(mutex));
        mutex->_ready = 0;
    }
    return 0;
}

static int mutex_lock_common(pthread_mutex_t *mutex, int try_only)
{
    unsigned int me;
    if (!mutex)
        return EINVAL;
    mutex_lazy_init(mutex);
    me = GetCurrentThreadId();
    if (mutex->_owner == me) {
        if (mutex->_type == PTHREAD_MUTEX_ERRORCHECK)
            return try_only ? EBUSY : EDEADLK;
        if (mutex->_type == PTHREAD_MUTEX_NORMAL) {
            if (try_only)
                return EBUSY;
            /* POSIX leaves a NORMAL relock deadlocked. */
            for (;;)
                Sleep(INFINITE);
        }
        /* RECURSIVE falls through to the section's own recursion. */
    }
    if (try_only) {
        if (!TryEnterCriticalSection(mutex_cs(mutex)))
            return EBUSY;
    } else {
        EnterCriticalSection(mutex_cs(mutex));
    }
    if (mutex->_owner == 0) {
        mutex->_owner = me;
        mutex->_count = 1;
    } else {
        mutex->_count++;
    }
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    return mutex_lock_common(mutex, 0);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    return mutex_lock_common(mutex, 1);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (!mutex)
        return EINVAL;
    if (mutex->_owner != GetCurrentThreadId())
        return EPERM;
    if (--mutex->_count == 0)
        mutex->_owner = 0;
    LeaveCriticalSection(mutex_cs(mutex));
    return 0;
}

static long long abstime_ms(const struct timespec *ts)
{
    return (long long)ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
}

static long long now_ms(int clock_id)
{
    if (clock_id == CLOCK_MONOTONIC)
        return (long long)GetTickCount64();
    {
        FILETIME ft;
        unsigned long long t;
        GetSystemTimeAsFileTime(&ft);
        t = ((unsigned long long)ft.dwHighDateTime << 32) |
            ft.dwLowDateTime;
        return (long long)(t / 10000) - 11644473600000LL;
    }
}

int pthread_mutex_timedlock(pthread_mutex_t *mutex,
                            const struct timespec *abstime)
{
    long long deadline;
    if (!mutex || !abstime)
        return EINVAL;
    deadline = abstime_ms(abstime);
    for (;;) {
        int rc = mutex_lock_common(mutex, 1);
        if (rc != EBUSY)
            return rc;
        if (now_ms(CLOCK_REALTIME) >= deadline)
            return ETIMEDOUT;
        Sleep(1);
    }
}

int pthread_mutex_getprioceiling(const pthread_mutex_t *mutex,
                                 int *prioceiling)
{
    if (!mutex || !prioceiling)
        return EINVAL;
    *prioceiling = mutex->_ceil;
    return 0;
}

int pthread_mutex_setprioceiling(pthread_mutex_t *mutex,
                                 int prioceiling, int *old_ceiling)
{
    if (!mutex)
        return EINVAL;
    if (old_ceiling)
        *old_ceiling = mutex->_ceil;
    mutex->_ceil = prioceiling;
    return 0;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    if (!attr)
        return EINVAL;
    attr->_type = PTHREAD_MUTEX_NORMAL;
    attr->_pshared = PTHREAD_PROCESS_PRIVATE;
    attr->_protocol = PTHREAD_PRIO_NONE;
    attr->_ceil = 0;
    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    (void)attr;
    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
    if (!attr || type < 0 || type > 2)
        return EINVAL;
    attr->_type = type;
    return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
    if (!attr || !type)
        return EINVAL;
    *type = attr->_type;
    return 0;
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared)
{
    if (!attr)
        return EINVAL;
    attr->_pshared = pshared;
    return 0;
}

int pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr,
                                 int *pshared)
{
    if (!attr || !pshared)
        return EINVAL;
    *pshared = attr->_pshared;
    return 0;
}

int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr,
                                  int protocol)
{
    if (!attr)
        return EINVAL;
    attr->_protocol = protocol;
    return 0;
}

int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr,
                                  int *protocol)
{
    if (!attr || !protocol)
        return EINVAL;
    *protocol = attr->_protocol;
    return 0;
}

int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr,
                                     int prioceiling)
{
    if (!attr)
        return EINVAL;
    attr->_ceil = prioceiling;
    return 0;
}

int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *attr,
                                     int *prioceiling)
{
    if (!attr || !prioceiling)
        return EINVAL;
    *prioceiling = attr->_ceil;
    return 0;
}

/* ---------------------------------------------------------------- */
/* Condition variables                                              */

/* The condattr clock has no room in the bundled
 * pthread_cond_t, so it is kept in a small side table keyed
 * by the cond address.  The default is CLOCK_REALTIME. */
static struct {
    pthread_cond_t *cond;
    int clock_id;
} g_cond_clocks[64];
static volatile long g_cond_clocks_lock;

static int cond_clock(pthread_cond_t *cond)
{
    int i, result = CLOCK_REALTIME;
    spin_lock(&g_cond_clocks_lock);
    for (i = 0; i < 64; i++)
        if (g_cond_clocks[i].cond == cond) {
            result = g_cond_clocks[i].clock_id;
            break;
        }
    spin_unlock(&g_cond_clocks_lock);
    return result;
}

static void cond_set_clock(pthread_cond_t *cond, int clock_id)
{
    int i, free_slot = -1;
    spin_lock(&g_cond_clocks_lock);
    for (i = 0; i < 64; i++) {
        if (g_cond_clocks[i].cond == cond) {
            if (clock_id < 0)
                g_cond_clocks[i].cond = NULL; /* forget the entry */
            else
                g_cond_clocks[i].clock_id = clock_id;
            spin_unlock(&g_cond_clocks_lock);
            return;
        }
        if (!g_cond_clocks[i].cond && free_slot < 0)
            free_slot = i;
    }
    if (free_slot >= 0 && clock_id >= 0) {
        g_cond_clocks[free_slot].cond = cond;
        g_cond_clocks[free_slot].clock_id = clock_id;
    }
    spin_unlock(&g_cond_clocks_lock);
}

int pthread_cond_init(pthread_cond_t *cond,
                      const pthread_condattr_t *attr)
{
    if (!cond)
        return EINVAL;
    cond->_cv = NULL;
    cond_set_clock(cond, attr ? attr->_clock : CLOCK_REALTIME);
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    if (cond)
        cond_set_clock(cond, -1);
    return 0;
}

static int cond_wait_common(pthread_cond_t *cond,
                            pthread_mutex_t *mutex,
                            const struct timespec *abstime)
{
    struct tinyc_cb *cb;
    int saved_count;
    long long deadline = 0;
    int clock_id;
    if (!cond || !mutex)
        return EINVAL;
    cb = self_cb();
    clock_id = cond_clock(cond);
    if (abstime) {
        deadline = abstime_ms(abstime);
        if (now_ms(clock_id) >= deadline)
            return ETIMEDOUT; /* mutex untouched, still held */
    }
    saved_count = mutex->_count;
    mutex->_count = 0;
    mutex->_owner = 0;
    for (;;) {
        DWORD slice = 50;
        BOOL ok;
        if (abstime) {
            long long rem = deadline - now_ms(clock_id);
            if (rem <= 0) {
                mutex->_owner = GetCurrentThreadId();
                mutex->_count = saved_count;
                return ETIMEDOUT;
            }
            if (rem < 50)
                slice = (DWORD)rem;
        }
        /* SleepConditionVariableCS releases the section while
         * sleeping and reacquires it before every return -
         * wake, timeout and failure alike - so the bookkeeping
         * is simply restored on the way out. */
        ok = SleepConditionVariableCS(
            (tinyc_CONDITION_VARIABLE *)cond, mutex_cs(mutex), slice);
        if (ok || GetLastError() != ERROR_TIMEOUT) {
            mutex->_owner = GetCurrentThreadId();
            mutex->_count = saved_count;
            return 0;
        }
        /* A slice expired: the wait continues, but a pending
         * cancellation is delivered, with the mutex held as
         * POSIX requires at cancellation. */
        if (cb && cb->cancel_pending &&
            cb->cancel_state == PTHREAD_CANCEL_ENABLE) {
            mutex->_owner = GetCurrentThreadId();
            mutex->_count = saved_count;
            deliver_cancel(cb);
        }
    }
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    return cond_wait_common(cond, mutex, NULL);
}

int pthread_cond_timedwait(pthread_cond_t *cond,
                           pthread_mutex_t *mutex,
                           const struct timespec *abstime)
{
    if (!abstime)
        return EINVAL;
    return cond_wait_common(cond, mutex, abstime);
}

int pthread_cond_signal(pthread_cond_t *cond)
{
    if (!cond)
        return EINVAL;
    WakeConditionVariable((tinyc_CONDITION_VARIABLE *)cond);
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    if (!cond)
        return EINVAL;
    WakeAllConditionVariable((tinyc_CONDITION_VARIABLE *)cond);
    return 0;
}

int pthread_condattr_init(pthread_condattr_t *attr)
{
    if (!attr)
        return EINVAL;
    attr->_pshared = PTHREAD_PROCESS_PRIVATE;
    attr->_clock = CLOCK_REALTIME;
    return 0;
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
    (void)attr;
    return 0;
}

int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared)
{
    if (!attr)
        return EINVAL;
    attr->_pshared = pshared;
    return 0;
}

int pthread_condattr_getpshared(const pthread_condattr_t *attr,
                                int *pshared)
{
    if (!attr || !pshared)
        return EINVAL;
    *pshared = attr->_pshared;
    return 0;
}

int pthread_condattr_setclock(pthread_condattr_t *attr, int clock_id)
{
    if (!attr)
        return EINVAL;
    if (clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC)
        return EINVAL;
    attr->_clock = clock_id;
    return 0;
}

int pthread_condattr_getclock(const pthread_condattr_t *attr,
                              int *clock_id)
{
    if (!attr || !clock_id)
        return EINVAL;
    *clock_id = attr->_clock;
    return 0;
}

/* ---------------------------------------------------------------- */
/* Read-write locks (SRWLOCK)                                       */

int pthread_rwlock_init(pthread_rwlock_t *rwlock,
                        const pthread_rwlockattr_t *attr)
{
    (void)attr;
    if (!rwlock)
        return EINVAL;
    rwlock->_srw = NULL;
    rwlock->_owner = 0;
    rwlock->_mode = 0;
    return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
    (void)rwlock;
    return 0;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock)
        return EINVAL;
    AcquireSRWLockShared((tinyc_SRWLOCK *)rwlock);
    return 0;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock)
        return EINVAL;
    return TryAcquireSRWLockShared((tinyc_SRWLOCK *)rwlock)
               ? 0
               : EBUSY;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock)
        return EINVAL;
    AcquireSRWLockExclusive((tinyc_SRWLOCK *)rwlock);
    rwlock->_owner = GetCurrentThreadId();
    rwlock->_mode = 2;
    return 0;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock)
        return EINVAL;
    if (!TryAcquireSRWLockExclusive((tinyc_SRWLOCK *)rwlock))
        return EBUSY;
    rwlock->_owner = GetCurrentThreadId();
    rwlock->_mode = 2;
    return 0;
}

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
    if (!rwlock)
        return EINVAL;
    if (rwlock->_mode == 2 &&
        rwlock->_owner == GetCurrentThreadId()) {
        rwlock->_mode = 0;
        rwlock->_owner = 0;
        ReleaseSRWLockExclusive((tinyc_SRWLOCK *)rwlock);
    } else {
        ReleaseSRWLockShared((tinyc_SRWLOCK *)rwlock);
    }
    return 0;
}

static int rwlock_timed(pthread_rwlock_t *rwlock, int write,
                        const struct timespec *abstime)
{
    long long deadline;
    if (!rwlock || !abstime)
        return EINVAL;
    deadline = abstime_ms(abstime);
    for (;;) {
        int rc = write ? pthread_rwlock_trywrlock(rwlock)
                       : pthread_rwlock_tryrdlock(rwlock);
        if (rc != EBUSY)
            return rc;
        if (now_ms(CLOCK_REALTIME) >= deadline)
            return ETIMEDOUT;
        Sleep(1);
    }
}

int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock,
                               const struct timespec *abstime)
{
    return rwlock_timed(rwlock, 0, abstime);
}

int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock,
                               const struct timespec *abstime)
{
    return rwlock_timed(rwlock, 1, abstime);
}

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr)
{
    if (!attr)
        return EINVAL;
    attr->_pshared = PTHREAD_PROCESS_PRIVATE;
    return 0;
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr)
{
    (void)attr;
    return 0;
}

int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr,
                                  int pshared)
{
    if (!attr)
        return EINVAL;
    attr->_pshared = pshared;
    return 0;
}

int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *attr,
                                  int *pshared)
{
    if (!attr || !pshared)
        return EINVAL;
    *pshared = attr->_pshared;
    return 0;
}

/* ---------------------------------------------------------------- */
/* Spin locks                                                       */

int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    (void)pshared;
    if (!lock)
        return EINVAL;
    lock->_lock = 0;
    return 0;
}

int pthread_spin_destroy(pthread_spinlock_t *lock)
{
    (void)lock;
    return 0;
}

int pthread_spin_lock(pthread_spinlock_t *lock)
{
    if (!lock)
        return EINVAL;
    while (InterlockedExchange(&lock->_lock, 1) != 0)
        Sleep(0);
    return 0;
}

int pthread_spin_trylock(pthread_spinlock_t *lock)
{
    if (!lock)
        return EINVAL;
    return InterlockedExchange(&lock->_lock, 1) == 0 ? 0 : EBUSY;
}

int pthread_spin_unlock(pthread_spinlock_t *lock)
{
    if (!lock)
        return EINVAL;
    InterlockedExchange(&lock->_lock, 0);
    return 0;
}

/* ---------------------------------------------------------------- */
/* Barriers                                                         */

int pthread_barrier_init(pthread_barrier_t *barrier,
                         const pthread_barrierattr_t *attr,
                         unsigned int count)
{
    (void)attr;
    if (!barrier || count == 0)
        return EINVAL;
    pthread_mutex_init(&barrier->_mtx, NULL);
    pthread_cond_init(&barrier->_cond, NULL);
    barrier->_total = count;
    barrier->_count = 0;
    barrier->_phase = 0;
    return 0;
}

int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
    if (!barrier)
        return EINVAL;
    pthread_mutex_destroy(&barrier->_mtx);
    return 0;
}

int pthread_barrier_wait(pthread_barrier_t *barrier)
{
    unsigned int phase;
    int serial = 0;
    if (!barrier)
        return EINVAL;
    pthread_mutex_lock(&barrier->_mtx);
    phase = barrier->_phase;
    if (++barrier->_count == barrier->_total) {
        barrier->_count = 0;
        barrier->_phase++;
        serial = 1;
        pthread_cond_broadcast(&barrier->_cond);
    } else {
        while (barrier->_phase == phase)
            pthread_cond_wait(&barrier->_cond, &barrier->_mtx);
    }
    pthread_mutex_unlock(&barrier->_mtx);
    return serial ? PTHREAD_BARRIER_SERIAL_THREAD : 0;
}

int pthread_barrierattr_init(pthread_barrierattr_t *attr)
{
    if (!attr)
        return EINVAL;
    attr->_pshared = PTHREAD_PROCESS_PRIVATE;
    return 0;
}

int pthread_barrierattr_destroy(pthread_barrierattr_t *attr)
{
    (void)attr;
    return 0;
}

int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr,
                                   int pshared)
{
    if (!attr)
        return EINVAL;
    attr->_pshared = pshared;
    return 0;
}

int pthread_barrierattr_getpshared(const pthread_barrierattr_t *attr,
                                   int *pshared)
{
    if (!attr || !pshared)
        return EINVAL;
    *pshared = attr->_pshared;
    return 0;
}

/* ---------------------------------------------------------------- */
/* Thread attributes                                                */

int pthread_attr_init(pthread_attr_t *attr)
{
    if (!attr)
        return EINVAL;
    memset(attr, 0, sizeof *attr);
    attr->detachstate = PTHREAD_CREATE_JOINABLE;
    attr->inheritsched = PTHREAD_INHERIT_SCHED;
    attr->schedpolicy = SCHED_OTHER;
    attr->scope = PTHREAD_SCOPE_SYSTEM;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    (void)attr;
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int state)
{
    if (!attr || (state != PTHREAD_CREATE_JOINABLE &&
                  state != PTHREAD_CREATE_DETACHED))
        return EINVAL;
    attr->detachstate = state;
    return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *state)
{
    if (!attr || !state)
        return EINVAL;
    *state = attr->detachstate;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t size)
{
    if (!attr || size < (size_t)PTHREAD_STACK_MIN)
        return EINVAL;
    attr->stacksize = size;
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *size)
{
    if (!attr || !size)
        return EINVAL;
    *size = attr->stacksize;
    return 0;
}

int pthread_attr_setstackaddr(pthread_attr_t *attr, void *addr)
{
    if (!attr)
        return EINVAL;
    attr->stackaddr = addr;
    return 0;
}

int pthread_attr_getstackaddr(const pthread_attr_t *attr, void **addr)
{
    if (!attr || !addr)
        return EINVAL;
    *addr = attr->stackaddr;
    return 0;
}

int pthread_attr_setstack(pthread_attr_t *attr, void *addr, size_t size)
{
    int rc = pthread_attr_setstacksize(attr, size);
    if (rc)
        return rc;
    attr->stackaddr = addr;
    return 0;
}

int pthread_attr_getstack(const pthread_attr_t *attr, void **addr,
                          size_t *size)
{
    if (!attr || !addr || !size)
        return EINVAL;
    *addr = attr->stackaddr;
    *size = attr->stacksize;
    return 0;
}

int pthread_attr_setguardsize(pthread_attr_t *attr, size_t size)
{
    if (!attr)
        return EINVAL;
    attr->guardsize = size;
    return 0;
}

int pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *size)
{
    if (!attr || !size)
        return EINVAL;
    *size = attr->guardsize;
    return 0;
}

int pthread_attr_setinheritsched(pthread_attr_t *attr, int inherit)
{
    if (!attr)
        return EINVAL;
    attr->inheritsched = inherit;
    return 0;
}

int pthread_attr_getinheritsched(const pthread_attr_t *attr,
                                 int *inherit)
{
    if (!attr || !inherit)
        return EINVAL;
    *inherit = attr->inheritsched;
    return 0;
}

int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy)
{
    if (!attr)
        return EINVAL;
    attr->schedpolicy = policy;
    return 0;
}

int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy)
{
    if (!attr || !policy)
        return EINVAL;
    *policy = attr->schedpolicy;
    return 0;
}

int pthread_attr_setschedparam(pthread_attr_t *attr,
                               const struct sched_param *param)
{
    if (!attr || !param)
        return EINVAL;
    attr->param = *param;
    return 0;
}

int pthread_attr_getschedparam(const pthread_attr_t *attr,
                               struct sched_param *param)
{
    if (!attr || !param)
        return EINVAL;
    *param = attr->param;
    return 0;
}

int pthread_attr_setscope(pthread_attr_t *attr, int scope)
{
    if (!attr)
        return EINVAL;
    if (scope == PTHREAD_SCOPE_PROCESS)
        return ENOTSUP;
    attr->scope = scope;
    return 0;
}

int pthread_attr_getscope(const pthread_attr_t *attr, int *scope)
{
    if (!attr || !scope)
        return EINVAL;
    *scope = attr->scope;
    return 0;
}

/* ---------------------------------------------------------------- */
/* Scheduling                                                       */

int sched_yield(void)
{
    SwitchToThread();
    return 0;
}

int sched_get_priority_max(int policy)
{
    (void)policy;
    return 15;
}

int sched_get_priority_min(int policy)
{
    (void)policy;
    return -15;
}

int sched_rr_get_interval(pid_t pid, struct timespec *interval)
{
    (void)pid;
    if (!interval)
        return EINVAL;
    interval->tv_sec = 0;
    interval->tv_nsec = 0;
    return 0;
}

/* ---------------------------------------------------------------- */
/* Semaphores (unnamed, over Win32 semaphore objects)               */

struct tinyc_sem {
    HANDLE handle;
    volatile long value;
};

int sem_init(sem_t *sem, int pshared, unsigned int value)
{
    struct tinyc_sem *s;
    (void)pshared;
    if (!sem)
        return errno = EINVAL, -1;
    s = (struct tinyc_sem *)malloc(sizeof *s);
    if (!s)
        return errno = ENOMEM, -1;
    s->handle = CreateSemaphoreW(NULL, (LONG)value, 0x7fffffffL, NULL);
    if (!s->handle) {
        free(s);
        return errno = ENOMEM, -1;
    }
    s->value = (long)value;
    sem->_impl = s;
    return 0;
}

int sem_destroy(sem_t *sem)
{
    struct tinyc_sem *s;
    if (!sem || !sem->_impl)
        return errno = EINVAL, -1;
    s = (struct tinyc_sem *)sem->_impl;
    CloseHandle(s->handle);
    free(s);
    sem->_impl = NULL;
    return 0;
}

sem_t *sem_open(const char *name, int oflag, ...)
{
    (void)name;
    (void)oflag;
    errno = ENOSYS;
    return SEM_FAILED;
}

int sem_close(sem_t *sem)
{
    (void)sem;
    return errno = ENOSYS, -1;
}

int sem_unlink(const char *name)
{
    (void)name;
    return errno = ENOSYS, -1;
}

int sem_wait(sem_t *sem)
{
    struct tinyc_sem *s;
    struct tinyc_cb *cb;
    if (!sem || !sem->_impl)
        return errno = EINVAL, -1;
    s = (struct tinyc_sem *)sem->_impl;
    cb = self_cb();
    for (;;) {
        DWORD r = WaitForSingleObject(s->handle, 50);
        if (r == WAIT_OBJECT_0) {
            InterlockedDecrement(&s->value);
            return 0;
        }
        if (r != WAIT_TIMEOUT)
            return errno = EINVAL, -1;
        if (cb && cb->cancel_pending &&
            cb->cancel_state == PTHREAD_CANCEL_ENABLE)
            deliver_cancel(cb);
    }
}

int sem_trywait(sem_t *sem)
{
    struct tinyc_sem *s;
    DWORD r;
    if (!sem || !sem->_impl)
        return errno = EINVAL, -1;
    s = (struct tinyc_sem *)sem->_impl;
    r = WaitForSingleObject(s->handle, 0);
    if (r == WAIT_OBJECT_0) {
        InterlockedDecrement(&s->value);
        return 0;
    }
    return errno = EAGAIN, -1;
}

int sem_timedwait(sem_t *sem, const struct timespec *abstime)
{
    struct tinyc_sem *s;
    long long deadline;
    if (!sem || !sem->_impl || !abstime)
        return errno = EINVAL, -1;
    s = (struct tinyc_sem *)sem->_impl;
    deadline = abstime_ms(abstime);
    for (;;) {
        long long rem = deadline - now_ms(CLOCK_REALTIME);
        DWORD r;
        if (rem <= 0)
            return errno = ETIMEDOUT, -1;
        r = WaitForSingleObject(s->handle,
                                rem < 50 ? (DWORD)rem : 50);
        if (r == WAIT_OBJECT_0) {
            InterlockedDecrement(&s->value);
            return 0;
        }
        if (r != WAIT_TIMEOUT)
            return errno = EINVAL, -1;
    }
}

int sem_post(sem_t *sem)
{
    struct tinyc_sem *s;
    if (!sem || !sem->_impl)
        return errno = EINVAL, -1;
    s = (struct tinyc_sem *)sem->_impl;
    if (!ReleaseSemaphore(s->handle, 1, NULL))
        return errno = EOVERFLOW, -1;
    InterlockedIncrement(&s->value);
    return 0;
}

int sem_getvalue(sem_t *sem, int *sval)
{
    struct tinyc_sem *s;
    if (!sem || !sem->_impl || !sval)
        return errno = EINVAL, -1;
    s = (struct tinyc_sem *)sem->_impl;
    *sval = (int)s->value;
    return 0;
}
