/* threads.c - the win32 target's C11 threads surface,
 * implemented over the bundled pthread (win32/lib/pthread.c).
 * build.sh compiles this file with the win32 cross compiler
 * and merges the object into libtcc1.a, which every program
 * links, so the surface is available with no extra link flag.
 *
 * Mapping: a thrd_t is a pthread_t (the int return value of a
 * C11 start routine travels as the pthread void* result);
 * mutexes and condition variables are the pthread objects of
 * the same shape; a tss_t is a pthread key; call_once is
 * pthread_once.  thrd_sleep is a Sleep() loop (Sleep is not
 * interruptible by anything this layer delivers). */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <threads.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#ifndef ETIMEDOUT
#define ETIMEDOUT 138
#endif
#ifndef EBUSY
#define EBUSY 16
#endif
#ifndef ENOMEM
#define ENOMEM 12
#endif

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
	/* Sleep() is not interruptible by anything this layer
	 * delivers, so the sleep always completes; the loop only
	 * splits durations past the 32-bit millisecond range. */
	long long left_ms = (long long)duration->tv_sec * 1000 +
		duration->tv_nsec / 1000000;
	if (duration->tv_nsec % 1000000)
		left_ms += 1;
	while (left_ms > 0) {
		DWORD chunk = left_ms > 0x7FFFFFFFLL
			? (DWORD)0x7FFFFFFF : (DWORD)left_ms;
		Sleep(chunk);
		left_ms -= chunk;
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
	return tinyc_thrd_rc(pthread_mutex_timedlock(mutex, time_point));
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
