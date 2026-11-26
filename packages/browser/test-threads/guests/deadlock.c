/* Deadlock (design §11, simulated tier only): two threads
 * take two mutexes in opposite orders with a forced
 * interleave; the run must fail with the §4.6 error naming
 * both threads, both mutexes and their owners. The real tier
 * makes no deadlock promise and does not run this case. */
#include "common.h"

static pthread_mutex_t mA = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mB = PTHREAD_MUTEX_INITIALIZER;
static int flagA, flagB;

static void *t1(void *arg) {
  (void)arg;
  pthread_mutex_lock(&mA);
  set_flag(&flagA, 1);
  wait_flag(&flagB, 1);
  pthread_mutex_lock(&mB);   /* blocks forever */
  pthread_mutex_unlock(&mB);
  pthread_mutex_unlock(&mA);
  return 0;
}

static void *t2(void *arg) {
  (void)arg;
  pthread_mutex_lock(&mB);
  set_flag(&flagB, 1);
  wait_flag(&flagA, 1);
  pthread_mutex_lock(&mA);   /* blocks forever */
  pthread_mutex_unlock(&mA);
  pthread_mutex_unlock(&mB);
  return 0;
}

int main(void) {
  pthread_t a, b;
  pthread_create(&a, 0, t1, 0);
  pthread_create(&b, 0, t2, 0);
  pthread_join(a, 0);
  pthread_join(b, 0);
  report("deadlock", -1, 0, 0, 0);  /* never reached */
  return 1;
}
