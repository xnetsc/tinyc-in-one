/* probe_threads.c - ordinary use of <threads.h>: create/join
 * with a return value, a mutex-guarded counter, a condition
 * hand-off, thread-specific storage, call_once, thrd_sleep,
 * a recursive mutex, and the two timed entry points. */
#include <threads.h>
#include <stdio.h>
#include <time.h>

static mtx_t counter_mtx;
static int counter;
static cnd_t go_cnd;
static mtx_t go_mtx;
static int go_flag;
static tss_t tss_key;
static once_flag once = ONCE_FLAG_INIT;
static int once_count;

static void run_once(void)
{
	once_count++;
}

static int worker(void *arg)
{
	int i;
	tss_set(tss_key, arg);
	for (i = 0; i < 1000; i++) {
		mtx_lock(&counter_mtx);
		counter++;
		mtx_unlock(&counter_mtx);
	}
	call_once(&once, run_once);
	if (tss_get(tss_key) != arg)
		return -1;
	return 42;
}

static int waiter(void *arg)
{
	(void)arg;
	mtx_lock(&go_mtx);
	while (!go_flag)
		cnd_wait(&go_cnd, &go_mtx);
	mtx_unlock(&go_mtx);
	return 7;
}

static void abstime_plus_2s(struct timespec *ts)
{
	ts->tv_sec = time(NULL) + 2;
	ts->tv_nsec = 0;
}

int main(void)
{
	thrd_t t1, t2, w;
	int r1 = 0, r2 = 0, rw = 0, fails = 0;
	struct timespec ts;
	mtx_t rec, timed;

	if (mtx_init(&counter_mtx, mtx_plain) != thrd_success) fails++;
	if (cnd_init(&go_cnd) != thrd_success) fails++;
	if (mtx_init(&go_mtx, mtx_plain) != thrd_success) fails++;
	if (tss_create(&tss_key, NULL) != thrd_success) fails++;

	if (thrd_create(&t1, worker, (void *)1) != thrd_success) fails++;
	if (thrd_create(&t2, worker, (void *)2) != thrd_success) fails++;
	if (thrd_create(&w, waiter, NULL) != thrd_success) fails++;

	/* thrd_sleep for a short interval completes with 0. */
	ts.tv_sec = 0;
	ts.tv_nsec = 1000000;
	if (thrd_sleep(&ts, NULL) != 0) fails++;

	mtx_lock(&go_mtx);
	go_flag = 1;
	cnd_signal(&go_cnd);
	mtx_unlock(&go_mtx);

	if (thrd_join(t1, &r1) != thrd_success || r1 != 42) fails++;
	if (thrd_join(t2, &r2) != thrd_success || r2 != 42) fails++;
	if (thrd_join(w, &rw) != thrd_success || rw != 7) fails++;
	if (counter != 2000) fails++;
	if (once_count != 1) fails++;
	if (!thrd_equal(thrd_current(), thrd_current())) fails++;

	/* A recursive mutex locks twice from the same thread. */
	if (mtx_init(&rec, mtx_recursive) != thrd_success) fails++;
	if (mtx_lock(&rec) != thrd_success) fails++;
	if (mtx_lock(&rec) != thrd_success) fails++;
	if (mtx_trylock(&rec) != thrd_success) fails++;
	mtx_unlock(&rec);
	mtx_unlock(&rec);
	mtx_unlock(&rec);
	mtx_destroy(&rec);

	/* mtx_timedlock on a free mutex succeeds; cnd_timedwait
	 * on an unsignalled condition reports the timeout. */
	if (mtx_init(&timed, mtx_timed) != thrd_success) fails++;
	abstime_plus_2s(&ts);
	if (mtx_timedlock(&timed, &ts) != thrd_success) fails++;
	mtx_unlock(&timed);
	mtx_destroy(&timed);
	mtx_lock(&go_mtx);
	go_flag = 0;
	abstime_plus_2s(&ts);
	if (cnd_timedwait(&go_cnd, &go_mtx, &ts) != thrd_timedout) fails++;
	mtx_unlock(&go_mtx);

	tss_delete(tss_key);
	cnd_destroy(&go_cnd);
	mtx_destroy(&go_mtx);
	mtx_destroy(&counter_mtx);

	if (fails) {
		printf("threads probe: %d failures\n", fails);
		return 1;
	}
	printf("threads probe: OK\n");
	return 0;
}
