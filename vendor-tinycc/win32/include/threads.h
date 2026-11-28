/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the TinyCC package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

/*
 * ISO C Standard:  7.26  Thread support  <threads.h>
 *
 * msvcrt provides no C11 threads surface, so the types alias
 * the bundled pthread forms (win32/include/pthread.h) and the
 * functions are implemented over that pthread in
 * win32/lib/threads.c, which ships inside libtcc1.a: a program
 * that includes this header links with no extra flag.
 */

#ifndef _THREADS_H_
#define _THREADS_H_

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

#endif /* _THREADS_H_ */
