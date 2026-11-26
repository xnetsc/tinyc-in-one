#if defined(__wasm__) || defined(__wasm32__)

#ifndef _PTHREAD_H
#define _PTHREAD_H

/* Threads.  In the tinyc-web process model these functions are
   provided by the host process layer as env imports; the object
   layouts and constant values below are the process ABI's own
   (they are not any host platform's).  A process runs in one of
   two tiers — real (one Web Worker per thread over a shared
   memory) or simulated (cooperative virtual threads) — and the
   guest-visible surface is identical in both.  Errors are
   returned as values, never through errno.
 *
 * A process that is not threaded supplies the same imports with
   pthread_create failing EAGAIN: no thread can ever exist there,
   and the remaining functions operate on the one main thread. */

/* Error numbers, for the returns of the functions below.  The
   guards keep a later errno.h from redefining them. */
#ifndef EPERM
#define EPERM		1
#endif
#ifndef ESRCH
#define ESRCH		3
#endif
#ifndef EAGAIN
#define EAGAIN		11
#endif
#ifndef EBUSY
#define EBUSY		16
#endif
#ifndef EINVAL
#define EINVAL		22
#endif
#ifndef EDEADLK
#define EDEADLK		35
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT	110
#endif

#ifndef _TINYC_TIMESPEC_DEFINED
#define _TINYC_TIMESPEC_DEFINED
struct timespec {
	long tv_sec;
	long tv_nsec;
};
#endif

/* Clock ids, as stored in a condition variable's clock field. */
#define CLOCK_REALTIME	0
#define CLOCK_MONOTONIC	1

/* A thread id: assigned monotonically from 1 (the main thread),
   never reused within a process.  0 is never a thread. */
typedef int pthread_t;

/* Attributes are plain host-side storage in the caller's memory.
   Of the thread attributes only the detach state takes effect;
   the rest are accepted, stored and read back without effect. */
typedef struct {
	int detachstate;
	int reserved[3];
} pthread_attr_t;

#define PTHREAD_CREATE_JOINABLE	0
#define PTHREAD_CREATE_DETACHED	1

/* state: 0 unlocked, 1 locked, 2 locked with waiters.
   owner: the holding thread's id (for the error-checking and
   recursive kinds, and for unlock ownership checks).
   count: the recursive hold count.
   kind: 0 normal, 1 error-checking, 2 recursive; -1 after
   pthread_mutex_destroy. */
typedef struct {
	int state;
	int owner;
	int count;
	int kind;
} pthread_mutex_t;

#define PTHREAD_MUTEX_NORMAL		0
#define PTHREAD_MUTEX_ERRORCHECK	1
#define PTHREAD_MUTEX_RECURSIVE		2
#define PTHREAD_MUTEX_DEFAULT		PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_INITIALIZER	{ 0, 0, 0, 0 }

typedef struct {
	int type;
} pthread_mutexattr_t;

/* seq: bumped by every signal and broadcast; a waiter sleeps
   until it changes.  waiters: the number of threads inside
   pthread_cond_wait / pthread_cond_timedwait.
   clock: 0 realtime, 1 monotonic — the clock pthread_cond_
   timedwait's abstime is expressed in. */
typedef struct {
	int seq;
	int waiters;
	int clock;
	int reserved;
} pthread_cond_t;

#define PTHREAD_COND_INITIALIZER	{ 0, 0, 0, 0 }

typedef struct {
	int clock;
} pthread_condattr_t;

typedef int pthread_key_t;
#define PTHREAD_KEYS_MAX		128
#define PTHREAD_DESTRUCTOR_ITERATIONS	4

typedef struct {
	int state;	/* 0 never ran, 1 running, 2 done */
} pthread_once_t;
#define PTHREAD_ONCE_INIT		{ 0 }

typedef int pthread_spinlock_t;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
		   void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **value_ptr);
int pthread_detach(pthread_t thread);
pthread_t pthread_self(void);
int pthread_equal(pthread_t t1, pthread_t t2);
void pthread_exit(void *value_ptr);

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_attr_getdetachstate(const pthread_attr_t *attr,
				int *detachstate);

int pthread_mutex_init(pthread_mutex_t *mutex,
		       const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t *mutex,
			    const struct timespec *abstime);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr,
			      int *type);

int pthread_cond_init(pthread_cond_t *cond,
		      const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond,
			   pthread_mutex_t *mutex,
			   const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);

int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);
int pthread_condattr_setclock(pthread_condattr_t *attr, int clock_id);
int pthread_condattr_getclock(const pthread_condattr_t *attr,
			      int *clock_id);

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
int pthread_spin_destroy(pthread_spinlock_t *lock);
int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_trylock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);
void *pthread_getspecific(pthread_key_t key);

int sched_yield(void);

#endif /* _PTHREAD_H */

#else

/* This header ships in the wasm target's include set.  For any
   other target the system's own pthread.h is the right one. */
#include_next <pthread.h>

#endif
