/* pthread keys (design §11): two threads hold different
 * values under one key at the same time; at exit the
 * destructor runs once per value, and a value the destructor
 * itself sets under a second key is destructed on a later
 * pass (the POSIX multi-pass rule, §7.1). */
#include "common.h"

static pthread_key_t keyA, keyB;
static int aCount, bCount;

static void dtorB(void *v) {
  (void)v;
  __atomic_add_fetch(&bCount, 1, __ATOMIC_SEQ_CST);
}

static void dtorA(void *v) {
  (void)v;
  __atomic_add_fetch(&aCount, 1, __ATOMIC_SEQ_CST);
  /* Setting another key from a destructor: its destructor
   * runs on a subsequent pass. */
  pthread_setspecific(keyB, (void *)1);
}

static int seen[2];

static void *worker(void *arg) {
  int me = (int)arg;
  pthread_setspecific(keyA, (void *)(100 + me));
  seen[me] = (int)pthread_getspecific(keyA);
  return 0;
}

int main(void) {
  pthread_key_create(&keyB, dtorB);
  pthread_key_create(&keyA, dtorA);
  pthread_t ids[2];
  for (int i = 0; i < 2; i++) pthread_create(&ids[i], 0, worker, (void *)i);
  for (int i = 0; i < 2; i++) pthread_join(ids[i], 0);
  /* main's own value under the same key is independent, and
   * the main thread's destructors do not run at join time. */
  pthread_setspecific(keyA, (void *)9);
  int mine = (int)pthread_getspecific(keyA);
  report("key_tls", seen[0], seen[1], mine, 0);
  report("key_dtor", aCount, bCount, 0, 0);
  return (seen[0] == 100 && seen[1] == 101 && mine == 9 &&
          aCount == 2 && bCount == 2) ? 0 : 2;
}
