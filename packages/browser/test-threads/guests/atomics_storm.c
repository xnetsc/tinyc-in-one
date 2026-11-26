/* Atomics (design §11): an atomic_fetch_add storm across
 * threads sums exactly, and an atomic_flag used as a guest
 * spinlock keeps a plain counter exact. Runs in both tiers;
 * the standalone form is covered by the backend atomics
 * stage. */
#include "common.h"
#include <stdatomic.h>

#define NTHREADS 4
#define PER 5000
static _Atomic int storm;
static atomic_flag flag = ATOMIC_FLAG_INIT;
static int guarded;

static void *worker(void *arg) {
  (void)arg;
  for (int i = 0; i < PER; i++) {
    atomic_fetch_add(&storm, 1);
    while (atomic_flag_test_and_set(&flag)) { }
    guarded++;
    atomic_flag_clear(&flag);
  }
  return 0;
}

int main(void) {
  pthread_t ids[NTHREADS];
  for (int i = 0; i < NTHREADS; i++)
    pthread_create(&ids[i], 0, worker, 0);
  for (int i = 0; i < NTHREADS; i++) pthread_join(ids[i], 0);
  int total = atomic_load(&storm);
  report("atomics", total, guarded, NTHREADS * PER, 0);
  return (total == NTHREADS * PER && guarded == NTHREADS * PER) ? 0 : 2;
}
