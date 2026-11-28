/* threads.c - the darwin target's C11 threads surface,
 * implemented over the pthread primitives in libSystem.
 * build.sh compiles this file with the darwin cross compiler
 * and merges the object into libtcc1.a, which every program
 * links, so the surface is available with no extra link flag.
 *
 * Mapping: a thrd_t is a pthread_t (the int return value of a
 * C11 start routine travels as the pthread void* result, which
 * is wide enough for an int on this target); mutexes and
 * condition variables are the pthread objects of the same
 * shape; a tss_t is a pthread key; call_once is pthread_once.
 * Darwin has no pthread_mutex_timedlock, so mtx_timedlock
 * polls trylock against the deadline with short sleeps. */

#include <threads.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

static int tinyc_thrd_rc(int rc)
{
	switch (rc) {
	case 0:		return thrd_success;
	case ENOMEM:	return thrd_nomem;
	case ETIMEDOUT:	return thrd_timedout;
	case EBUSY:	return thrd_busy;
	default:	return thrd_error;
	}
}

struct tinyc_thrd_boot {
	thrd_start_t start;
	void *arg;
};

static void *tinyc_thrd_trampoline(void *p)
{
	struct tinyc_thrd_boot *boot = p;
	thrd_start_t start = boot->start;
	void *arg = boot->arg;
	free(boot);
	return (void *)(intptr_t)start(arg);
}

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
	struct tinyc_thrd_boot *boot = malloc(sizeof *boot);
	int rc;
	if (!boot)
		return thrd_nomem;
	boot->start = func;
	boot->arg = arg;
	rc = pthread_create(thr, NULL, tinyc_thrd_trampoline, boot);
	if (rc)
		free(boot);
	return tinyc_thrd_rc(rc);
}

int thrd_join(thrd_t thr, int *res)
{
	void *val = NULL;
	int rc = pthread_join(thr, &val);
	if (rc == 0 && res)
		*res = (int)(intptr_t)val;
	return tinyc_thrd_rc(rc);
}

int thrd_detach(thrd_t thr)
{
	return tinyc_thrd_rc(pthread_detach(thr));
}

_Noreturn void thrd_exit(int res)
{
	pthread_exit((void *)(intptr_t)res);
	for (;;) { }	/* pthread_exit does not return */
}

thrd_t thrd_current(void)
{
	return pthread_self();
}

int thrd_equal(thrd_t lhs, thrd_t rhs)
{
	return pthread_equal(lhs, rhs) != 0;
}

void thrd_yield(void)
{
	sched_yield();
}

int thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
	struct timespec rem = *duration;
	int rc;
	while ((rc = nanosleep(&rem, &rem)) != 0) {
		if (errno != EINTR) {
			if (remaining)
				*remaining = rem;
			return -1;
		}
	}
	if (remaining) {
		remaining->tv_sec = 0;
		remaining->tv_nsec = 0;
	}
	return 0;
}

int mtx_init(mtx_t *mutex, int type)
{
	int rc;
	if (type & mtx_recursive) {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		rc = pthread_mutex_init(mutex, &attr);
		pthread_mutexattr_destroy(&attr);
	} else {
		rc = pthread_mutex_init(mutex, NULL);
	}
	return tinyc_thrd_rc(rc);
}

void mtx_destroy(mtx_t *mutex)
{
	(void)pthread_mutex_destroy(mutex);
}

int mtx_lock(mtx_t *mutex)
{
	return tinyc_thrd_rc(pthread_mutex_lock(mutex));
}

int mtx_trylock(mtx_t *mutex)
{
	return tinyc_thrd_rc(pthread_mutex_trylock(mutex));
}

int mtx_timedlock(mtx_t *mutex, const struct timespec *time_point)
{
	/* No pthread_mutex_timedlock on this platform: poll. */
	for (;;) {
		int rc = pthread_mutex_trylock(mutex);
		struct timespec now, nap;
		if (rc == 0)
			return thrd_success;
		if (rc != EBUSY)
			return tinyc_thrd_rc(rc);
		clock_gettime(CLOCK_REALTIME, &now);
		if (now.tv_sec > time_point->tv_sec ||
		    (now.tv_sec == time_point->tv_sec &&
		     now.tv_nsec >= time_point->tv_nsec))
			return thrd_timedout;
		nap.tv_sec = 0;
		nap.tv_nsec = 1000000L;	/* 1 ms */
		nanosleep(&nap, NULL);
	}
}

int mtx_unlock(mtx_t *mutex)
{
	return tinyc_thrd_rc(pthread_mutex_unlock(mutex));
}

int cnd_init(cnd_t *cond)
{
	return tinyc_thrd_rc(pthread_cond_init(cond, NULL));
}

void cnd_destroy(cnd_t *cond)
{
	(void)pthread_cond_destroy(cond);
}

int cnd_signal(cnd_t *cond)
{
	return tinyc_thrd_rc(pthread_cond_signal(cond));
}

int cnd_broadcast(cnd_t *cond)
{
	return tinyc_thrd_rc(pthread_cond_broadcast(cond));
}

int cnd_wait(cnd_t *cond, mtx_t *mutex)
{
	return tinyc_thrd_rc(pthread_cond_wait(cond, mutex));
}

int cnd_timedwait(cnd_t *cond, mtx_t *mutex,
		  const struct timespec *time_point)
{
	return tinyc_thrd_rc(pthread_cond_timedwait(cond, mutex, time_point));
}

int tss_create(tss_t *key, void (*dtor)(void *))
{
	return tinyc_thrd_rc(pthread_key_create(key, dtor));
}

void tss_delete(tss_t key)
{
	(void)pthread_key_delete(key);
}

int tss_set(tss_t key, void *val)
{
	return tinyc_thrd_rc(pthread_setspecific(key, val));
}

void *tss_get(tss_t key)
{
	return pthread_getspecific(key);
}

void call_once(once_flag *flag, void (*func)(void))
{
	(void)pthread_once(flag, func);
}
