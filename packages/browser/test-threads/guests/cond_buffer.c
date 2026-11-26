/* Condition variables (design §11): a bounded buffer passes
 * an exact item count between producer and consumer; a
 * timedwait nobody signals returns ETIMEDOUT inside its
 * window and still holding the mutex; a broadcast wakes every
 * waiter exactly once. */
#include "common.h"

#define CAP 4
#define ITEMS 20
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t notFull = PTHREAD_COND_INITIALIZER;
static pthread_cond_t notEmpty = PTHREAD_COND_INITIALIZER;
static int buf[CAP], head, tail, count, consumedSum, consumed;

static void *producer(void *arg) {
  (void)arg;
  for (int i = 1; i <= ITEMS; i++) {
    pthread_mutex_lock(&mtx);
    while (count == CAP) pthread_cond_wait(&notFull, &mtx);
    buf[tail] = i;
    tail = (tail + 1) % CAP;
    count++;
    pthread_cond_signal(&notEmpty);
    pthread_mutex_unlock(&mtx);
  }
  return 0;
}

static void *consumer(void *arg) {
  (void)arg;
  for (int i = 0; i < ITEMS; i++) {
    pthread_mutex_lock(&mtx);
    while (count == 0) pthread_cond_wait(&notEmpty, &mtx);
    consumedSum += buf[head];
    head = (head + 1) % CAP;
    count--;
    consumed++;
    pthread_cond_signal(&notFull);
    pthread_mutex_unlock(&mtx);
  }
  return 0;
}

/* broadcast part */
static pthread_cond_t bcond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t bmtx = PTHREAD_MUTEX_INITIALIZER;
static int ready, wakes[3];

static void *waiter(void *arg) {
  int me = (int)arg;
  pthread_mutex_lock(&bmtx);
  __atomic_add_fetch(&ready, 1, __ATOMIC_SEQ_CST);
  pthread_cond_wait(&bcond, &bmtx);
  wakes[me]++;
  pthread_mutex_unlock(&bmtx);
  return 0;
}

int main(void) {
  int fails = 0;
  pthread_t p, c;
  pthread_create(&p, 0, producer, 0);
  pthread_create(&c, 0, consumer, 0);
  pthread_join(p, 0);
  pthread_join(c, 0);
  if (consumed != ITEMS || consumedSum != ITEMS * (ITEMS + 1) / 2) fails++;

  /* timedwait: one second out, nobody signals */
  pthread_mutex_t tm = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t tc = PTHREAD_COND_INITIALIZER;
  struct timespec ts;
  /* time() truncates to whole seconds, so +2 puts the actual
   * wait in [1000, 2000] ms. */
  ts.tv_sec = (int)time(0) + 2;
  ts.tv_nsec = 0;
  pthread_mutex_lock(&tm);
  long t0 = clock();
  int tw = pthread_cond_timedwait(&tc, &tm, &ts);
  long elapsedMs = (clock() - t0) / 1000;
  /* still holding the mutex: an unlock must succeed */
  int un = pthread_mutex_unlock(&tm);
  if (tw != ETIMEDOUT || un != 0) fails++;
  if (elapsedMs < 800 || elapsedMs > 8000) fails++;

  /* broadcast wakes each waiter exactly once */
  pthread_t w[3];
  for (int i = 0; i < 3; i++) pthread_create(&w[i], 0, waiter, (void *)i);
  wait_flag(&ready, 3);
  /* give the waiters a turn to actually block on the cond */
  for (int i = 0; i < 50; i++) sched_yield();
  pthread_mutex_lock(&bmtx);
  pthread_cond_broadcast(&bcond);
  pthread_mutex_unlock(&bmtx);
  for (int i = 0; i < 3; i++) pthread_join(w[i], 0);
  int wakeSum = wakes[0] + wakes[1] + wakes[2];
  if (wakeSum != 3) fails++;

  report("cond_buffer", consumed, consumedSum, tw, (int)elapsedMs);
  report("cond_bcast", wakeSum, wakes[0], wakes[1], wakes[2]);
  return fails ? 2 : 0;
}
