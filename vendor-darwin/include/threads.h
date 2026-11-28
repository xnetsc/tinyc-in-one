/* threads.h - C11 threads, Darwin.
 *
 * Written for this package from the C11 <threads.h> requirements.
 * libSystem provides no C11 threads surface, so the types alias
 * the Darwin pthread forms and the functions are implemented
 * over pthread in lib/threads.c, which ships inside libtcc1.a:
 * a program that includes this header links with no extra flag.
 * One implementation note: Darwin has no pthread_mutex_timedlock,
 * so mtx_timedlock polls with trylock against a clock_gettime
 * deadline; the semantics are the standard ones, the wake
 * latency is coarser than a native timed lock.
 */

#ifndef _TINYC_DARWIN_THREADS_H
#define _TINYC_DARWIN_THREADS_H

#include <pthread.h>
#include <time.h>

#define thread_local	_Thread_local

typedef pthread_t	thrd_t;
typedef pthread_mutex_t	mtx_t;
typedef pthread_cond_t	cnd_t;
typedef pthread_key_t	tss_t;
typedef pthread_once_t	once_flag;
typedef int (*thrd_start_t)(void *);

#define ONCE_FLAG_INIT		PTHREAD_ONCE_INIT
#define TSS_DTOR_ITERATIONS	PTHREAD_DESTRUCTOR_ITERATIONS

enum {
	thrd_success	= 0,
	thrd_nomem	= 1,
	thrd_timedout	= 2,
	thrd_busy	= 3,
	thrd_error	= 4
};

enum {
	mtx_plain	= 0,
	mtx_timed	= 1,
	mtx_recursive	= 2
};

/* Threads */
int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
int thrd_join(thrd_t thr, int *res);
int thrd_detach(thrd_t thr);
_Noreturn void thrd_exit(int res);
thrd_t thrd_current(void);
int thrd_equal(thrd_t lhs, thrd_t rhs);
void thrd_yield(void);
int thrd_sleep(const struct timespec *duration,
	       struct timespec *remaining);

/* Mutexes */
int mtx_init(mtx_t *mutex, int type);
void mtx_destroy(mtx_t *mutex);
int mtx_lock(mtx_t *mutex);
int mtx_trylock(mtx_t *mutex);
int mtx_timedlock(mtx_t *mutex, const struct timespec *time_point);
int mtx_unlock(mtx_t *mutex);

/* Condition variables */
int cnd_init(cnd_t *cond);
void cnd_destroy(cnd_t *cond);
int cnd_signal(cnd_t *cond);
int cnd_broadcast(cnd_t *cond);
int cnd_wait(cnd_t *cond, mtx_t *mutex);
int cnd_timedwait(cnd_t *cond, mtx_t *mutex,
		  const struct timespec *time_point);

/* Thread-specific storage */
int tss_create(tss_t *key, void (*dtor)(void *));
void tss_delete(tss_t key);
int tss_set(tss_t key, void *val);
void *tss_get(tss_t key);

/* Once */
void call_once(once_flag *flag, void (*func)(void));

#endif
