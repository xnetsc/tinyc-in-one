/* pthread.h - POSIX threads, Darwin.
 *
 * Written for this package from the POSIX <pthread.h>
 * requirements with the Darwin type forms: pthread_t is a
 * pointer to an opaque structure; the attribute, mutex,
 * condition, rwlock and once types are a signature word followed
 * by a fixed-size opaque area (the sizes are part of the Darwin
 * ABI: 56/8/40/8/192/16/8 bytes of opaque storage respectively),
 * and the static initializers store the platform signature
 * constants.  The macOS CI probe compares every sizeof here
 * against the platform headers.
 *
 * Not declared, deliberately: the cancellation API and the spin
 * lock API (their constant sets could not be stated with the
 * same confidence), and barriers, which Darwin does not have.
 */

#ifndef _TINYC_DARWIN_PTHREAD_H
#define _TINYC_DARWIN_PTHREAD_H

#include <stddef.h>
#include <sys/types.h>
#include <time.h>
#include <sched.h>

typedef struct _opaque_pthread_t *pthread_t;
typedef unsigned long pthread_key_t;

typedef struct {
	long __sig;
	char __opaque[56];
} pthread_attr_t;

typedef struct {
	long __sig;
	char __opaque[56];
} pthread_mutex_t;

typedef struct {
	long __sig;
	char __opaque[8];
} pthread_mutexattr_t;

typedef struct {
	long __sig;
	char __opaque[40];
} pthread_cond_t;

typedef struct {
	long __sig;
	char __opaque[8];
} pthread_condattr_t;

typedef struct {
	long __sig;
	char __opaque[192];
} pthread_rwlock_t;

typedef struct {
	long __sig;
	char __opaque[16];
} pthread_rwlockattr_t;

typedef struct {
	long __sig;
	char __opaque[8];
} pthread_once_t;

#define _PTHREAD_MUTEX_SIG_init		0x32AAABA7L
#define _PTHREAD_COND_SIG_init		0x3CB0B1BBL
#define _PTHREAD_RWLOCK_SIG_init	0x2DA8B3B4L
#define _PTHREAD_ONCE_SIG_init		0x30B1BCBAL

#define PTHREAD_MUTEX_INITIALIZER	{ _PTHREAD_MUTEX_SIG_init, { 0 } }
#define PTHREAD_COND_INITIALIZER	{ _PTHREAD_COND_SIG_init, { 0 } }
#define PTHREAD_RWLOCK_INITIALIZER	{ _PTHREAD_RWLOCK_SIG_init, { 0 } }
#define PTHREAD_ONCE_INIT		{ _PTHREAD_ONCE_SIG_init, { 0 } }

#define PTHREAD_CREATE_JOINABLE		1
#define PTHREAD_CREATE_DETACHED		2

#define PTHREAD_MUTEX_NORMAL		0
#define PTHREAD_MUTEX_ERRORCHECK	1
#define PTHREAD_MUTEX_RECURSIVE		2
#define PTHREAD_MUTEX_FIRSTFIT		3
#define PTHREAD_MUTEX_DEFAULT		PTHREAD_MUTEX_NORMAL

#define PTHREAD_PROCESS_SHARED		1
#define PTHREAD_PROCESS_PRIVATE		2

#define PTHREAD_INHERIT_SCHED		1
#define PTHREAD_EXPLICIT_SCHED		2

#define PTHREAD_SCOPE_SYSTEM		1
#define PTHREAD_SCOPE_PROCESS		2

#define PTHREAD_DESTRUCTOR_ITERATIONS	4
#define PTHREAD_KEYS_MAX		512
#define PTHREAD_STACK_MIN		8192

/* Thread creation and lifetime */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
		   void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **value_ptr);
int pthread_detach(pthread_t thread);
void pthread_exit(void *value_ptr);
pthread_t pthread_self(void);
int pthread_equal(pthread_t t1, pthread_t t2);
int pthread_atfork(void (*prepare)(void), void (*parent)(void),
		   void (*child)(void));
int pthread_kill(pthread_t thread, int sig);
int pthread_main_np(void);
int pthread_threadid_np(pthread_t thread, uint64_t *thread_id);
pthread_t pthread_from_mach_thread_np(uint32_t mach_tid);
uint32_t pthread_mach_thread_np(pthread_t thread);

/* Attributes */
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);
int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr,
			  size_t stacksize);
int pthread_attr_getstack(const pthread_attr_t *attr, void **stackaddr,
			  size_t *stacksize);
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);
int pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *guardsize);
int pthread_attr_setschedparam(pthread_attr_t *attr,
			       const struct sched_param *param);
int pthread_attr_getschedparam(const pthread_attr_t *attr,
			       struct sched_param *param);
int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);
int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy);
int pthread_attr_setinheritsched(pthread_attr_t *attr, int inheritsched);
int pthread_attr_getinheritsched(const pthread_attr_t *attr, int *inheritsched);
int pthread_attr_setscope(pthread_attr_t *attr, int scope);
int pthread_attr_getscope(const pthread_attr_t *attr, int *scope);
int pthread_getschedparam(pthread_t thread, int *policy,
			  struct sched_param *param);
int pthread_setschedparam(pthread_t thread, int policy,
			  const struct sched_param *param);
int pthread_setschedprio(pthread_t thread, int prio);

/* Once */
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

/* Thread-specific data */
int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

/* Mutexes */
int pthread_mutex_init(pthread_mutex_t *mutex,
		       const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);
int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared);
int pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr, int *pshared);

/* Conditions */
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
			   const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);
int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared);
int pthread_condattr_getpshared(const pthread_condattr_t *attr, int *pshared);

/* Read/write locks */
int pthread_rwlock_init(pthread_rwlock_t *rwlock,
			const pthread_rwlockattr_t *attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared);
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *attr, int *pshared);

#endif
