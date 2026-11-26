/* Region reclamation (design §3.1): hundreds of create+join
 * and create+detach rounds must run in bounded memory. The
 * guest reads the allocator's heap-top header word (§5.6)
 * after a warmup and again at the end, and reports the
 * growth; the harness bounds it to a small slack. Without
 * reclamation every round's stack, key array and record
 * would advance the frontier for good — hundreds of times
 * one round's regions. */
#include "common.h"

#define WARMUP 25
#define ROUNDS 175

static int detached_ran;

static void *joiner(void *arg) { return arg; }

static void *detached_fn(void *arg) {
  (void)arg;
  __atomic_add_fetch(&detached_ran, 1, __ATOMIC_SEQ_CST);
  return 0;
}

static int heap_top(void) { return *(volatile int *)512; }

static int one_join(int v) {
  pthread_t t;
  void *ret = 0;
  if (pthread_create(&t, 0, joiner, (void *)(long)v) != 0) return -1;
  if (pthread_join(t, &ret) != 0) return -2;
  return (int)ret == v ? 0 : -3;
}

static int one_detach(void) {
  pthread_t t;
  if (pthread_create(&t, 0, detached_fn, 0) != 0) return -1;
  return pthread_detach(t) != 0 ? -2 : 0;
}

int main(void) {
  const int total = WARMUP + ROUNDS;
  const int detaches = (total + 1) / 2;
  int h0 = 0;
  for (int i = 0; i < total; i++) {
    int rc = one_join(i + 1);
    if (rc != 0) { report("churn", rc, i, 0, 0); return 1; }
    if ((i & 1) == 0 && one_detach() != 0) {
      report("churn", -4, i, 0, 0);
      return 1;
    }
    if (i == WARMUP - 1) {
      /* Settle the warmup's detached threads, then take the
       * baseline: every size class now exists and holds
       * recycled regions. */
      while (__atomic_load_n(&detached_ran, __ATOMIC_ACQUIRE) <
             (WARMUP + 1) / 2)
        sched_yield();
      h0 = heap_top();
    }
  }
  while (__atomic_load_n(&detached_ran, __ATOMIC_ACQUIRE) < detaches)
    sched_yield();
  /* A few trailing joins: they synchronize with worker
   * exits, so every earlier thread's exit path — frees
   * included — has completed before the final read. */
  for (int i = 0; i < 10; i++) {
    if (one_join(1000 + i) != 0) { report("churn", -5, i, 0, 0); return 1; }
  }
  int h1 = heap_top();
  report("churn", total, detaches, h1 - h0, 0);
  return 0;
}
