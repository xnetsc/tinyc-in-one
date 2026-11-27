/* pthread.h - POSIX threads for the win32 target.
 *
 * The implementation lives in win32/lib/pthread.c, over the
 * Win32 threading primitives (threads, CRITICAL_SECTION,
 * CONDITION_VARIABLE, SRWLOCK, FLS, semaphores), and ships
 * inside libtcc1.a, which every program links: a pthread
 * program needs no extra link flag and no runtime library
 * beyond the Windows system DLLs.  The types below are laid
 * out for that implementation; the storage fields are not
 * program-visible state.
 *
 * Cancellation is deferred: it takes effect at
 * pthread_testcancel and inside this implementation's own
 * blocking waits (condition wait, join, semaphore wait).
 * pthread_atfork records its handlers; Windows has no fork,
 * so they never run. */
#ifndef _PTHREAD_H
#define _PTHREAD_H

#include <stddef.h>
#include <time.h>
#include <sched.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

/* Thread handle plus the implementation control block. */
typedef struct {
    void *_handle;
    void *_cb;
} pthread_t;

typedef struct {
    size_t stacksize;
    void *stackaddr;
    int detachstate;
    int inheritsched;
    int schedpolicy;
    struct sched_param param;
    int scope;
    size_t guardsize;
} pthread_attr_t;

/* _cs carries a CRITICAL_SECTION (40 bytes on this target). */
typedef struct {
    unsigned long long _cs[5];
    unsigned int _owner;
    int _count;
    int _type;
    unsigned int _ready;
    int _ceil;
    int _pad;
} pthread_mutex_t;

typedef struct {
    int _type;
    int _pshared;
    int _protocol;
    int _ceil;
} pthread_mutexattr_t;

/* _cv carries a CONDITION_VARIABLE (one pointer, zero is the
 * initialized state). */
typedef struct {
    void *_cv;
} pthread_cond_t;

typedef struct {
    int _pshared;
    int _clock;
} pthread_condattr_t;

/* _srw carries an SRWLOCK (one pointer, zero initialized). */
typedef struct {
    void *_srw;
    unsigned int _owner;
    int _mode;
    int _pad;
} pthread_rwlock_t;

typedef struct {
    int _pshared;
} pthread_rwlockattr_t;

typedef struct {
    volatile long _lock;
} pthread_spinlock_t;

typedef struct {
    pthread_mutex_t _mtx;
    pthread_cond_t _cond;
    unsigned int _total;
    unsigned int _count;
    unsigned int _phase;
    unsigned int _pad;
} pthread_barrier_t;

typedef struct {
    int _pshared;
} pthread_barrierattr_t;

typedef unsigned int pthread_key_t;

typedef struct {
    long _state;
} pthread_once_t;

#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1
#define PTHREAD_INHERIT_SCHED   0
#define PTHREAD_EXPLICIT_SCHED  1
#define PTHREAD_SCOPE_SYSTEM    0
#define PTHREAD_SCOPE_PROCESS   1
#define PTHREAD_PROCESS_PRIVATE 0
#define PTHREAD_PROCESS_SHARED  1

#define PTHREAD_MUTEX_NORMAL    0
#define PTHREAD_MUTEX_ERRORCHECK 1
#define PTHREAD_MUTEX_RECURSIVE 2
#define PTHREAD_MUTEX_DEFAULT   PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_TIMED_NP      PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_RECURSIVE_NP  PTHREAD_MUTEX_RECURSIVE
#define PTHREAD_MUTEX_ERRORCHECK_NP PTHREAD_MUTEX_ERRORCHECK

#define PTHREAD_PRIO_NONE    0
#define PTHREAD_PRIO_INHERIT 1
#define PTHREAD_PRIO_PROTECT 2

#define PTHREAD_CANCEL_ENABLE       0
#define PTHREAD_CANCEL_DISABLE      1
#define PTHREAD_CANCEL_DEFERRED     0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1
#define PTHREAD_CANCELED ((void *)-1)

#define PTHREAD_ONCE_INIT { 0 }
#define PTHREAD_MUTEX_INITIALIZER \
    { { 0, 0, 0, 0, 0 }, 0, 0, PTHREAD_MUTEX_NORMAL, 0, 0, 0 }
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP \
    { { 0, 0, 0, 0, 0 }, 0, 0, PTHREAD_MUTEX_RECURSIVE, 0, 0, 0 }
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP \
    { { 0, 0, 0, 0, 0 }, 0, 0, PTHREAD_MUTEX_ERRORCHECK, 0, 0, 0 }
#define PTHREAD_COND_INITIALIZER { 0 }
#define PTHREAD_RWLOCK_INITIALIZER { 0, 0, 0, 0 }

#define PTHREAD_DESTRUCTOR_ITERATIONS 4
#define PTHREAD_KEYS_MAX 128
#define PTHREAD_STACK_MIN 16384
#define PTHREAD_BARRIER_SERIAL_THREAD (-1)

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start)(void *), void *arg);
void pthread_exit(void *retval);
int pthread_join(pthread_t thread, void **retval);
int pthread_detach(pthread_t thread);
pthread_t pthread_self(void);
int pthread_equal(pthread_t a, pthread_t b);
int pthread_yield(void);

int pthread_once(pthread_once_t *once, void (*init)(void));

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

int pthread_cancel(pthread_t thread);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);
void pthread_testcancel(void);

int pthread_atfork(void (*prepare)(void), void (*parent)(void),
                   void (*child)(void));
int pthread_getconcurrency(void);
int pthread_setconcurrency(int level);

struct _tinyc_cleanup {
    void (*_routine)(void *);
    void *_arg;
    struct _tinyc_cleanup *_next;
};
void _tinyc_cleanup_push(struct _tinyc_cleanup *cl);
void _tinyc_cleanup_pop(struct _tinyc_cleanup *cl, int execute);
#define pthread_cleanup_push(routine, arg) \
    do { \
        struct _tinyc_cleanup _tinyc_cl; \
        _tinyc_cl._routine = (routine); \
        _tinyc_cl._arg = (arg); \
        _tinyc_cleanup_push(&_tinyc_cl);
#define pthread_cleanup_pop(execute) \
        _tinyc_cleanup_pop(&_tinyc_cl, (execute)); \
    } while (0)

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int state);
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *state);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t size);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *size);
int pthread_attr_setstackaddr(pthread_attr_t *attr, void *addr);
int pthread_attr_getstackaddr(const pthread_attr_t *attr, void **addr);
int pthread_attr_setstack(pthread_attr_t *attr, void *addr, size_t size);
int pthread_attr_getstack(const pthread_attr_t *attr, void **addr,
                          size_t *size);
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t size);
int pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *size);
int pthread_attr_setinheritsched(pthread_attr_t *attr, int inherit);
int pthread_attr_getinheritsched(const pthread_attr_t *attr, int *inherit);
int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);
int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy);
int pthread_attr_setschedparam(pthread_attr_t *attr,
                               const struct sched_param *param);
int pthread_attr_getschedparam(const pthread_attr_t *attr,
                               struct sched_param *param);
int pthread_attr_setscope(pthread_attr_t *attr, int scope);
int pthread_attr_getscope(const pthread_attr_t *attr, int *scope);

int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t *mutex,
                            const struct timespec *abstime);
int pthread_mutex_getprioceiling(const pthread_mutex_t *mutex,
                                 int *prioceiling);
int pthread_mutex_setprioceiling(pthread_mutex_t *mutex, int prioceiling,
                                 int *old_ceiling);

int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);
int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared);
int pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr,
                                 int *pshared);
int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol);
int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr,
                                  int *protocol);
int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr,
                                     int prioceiling);
int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *attr,
                                     int *prioceiling);

int pthread_cond_init(pthread_cond_t *cond,
                      const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);

int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);
int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared);
int pthread_condattr_getpshared(const pthread_condattr_t *attr,
                                int *pshared);
int pthread_condattr_setclock(pthread_condattr_t *attr, int clock_id);
int pthread_condattr_getclock(const pthread_condattr_t *attr,
                              int *clock_id);

int pthread_rwlock_init(pthread_rwlock_t *rwlock,
                        const pthread_rwlockattr_t *attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock,
                               const struct timespec *abstime);
int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock,
                               const struct timespec *abstime);

int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared);
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *attr,
                                  int *pshared);

int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
int pthread_spin_destroy(pthread_spinlock_t *lock);
int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_trylock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);

int pthread_barrier_init(pthread_barrier_t *barrier,
                         const pthread_barrierattr_t *attr,
                         unsigned int count);
int pthread_barrier_destroy(pthread_barrier_t *barrier);
int pthread_barrier_wait(pthread_barrier_t *barrier);

int pthread_barrierattr_init(pthread_barrierattr_t *attr);
int pthread_barrierattr_destroy(pthread_barrierattr_t *attr);
int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr,
                                   int pshared);
int pthread_barrierattr_getpshared(const pthread_barrierattr_t *attr,
                                   int *pshared);

#ifdef __cplusplus
}
#endif

#endif /* _PTHREAD_H */
