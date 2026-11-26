/* Spinlocks (design §11): a counter under a spinlock reaches
 * the exact total. Completion in the simulated tier also
 * shows the forced yield keeps a spinning thread from
 * starving the lock holder. */
#include "common.h"

#define NTHREADS 4
#define PER 5000
static pthread_spinlock_t spin;
static int counter;

static void *worker(void *arg) {
  (void)arg;
  for (int i = 0; i < PER; i++) {
    pthread_spin_lock(&spin);
    counter++;
    pthread_spin_unlock(&spin);
  }
  return 0;
}

int main(void) {
  pthread_spin_init(&spin, 0);
  pthread_t ids[NTHREADS];
  for (int i = 0; i < NTHREADS; i++)
    pthread_create(&ids[i], 0, worker, 0);
  for (int i = 0; i < NTHREADS; i++) pthread_join(ids[i], 0);
  report("spin", counter, NTHREADS * PER, 0, 0);
  return counter == NTHREADS * PER ? 0 : 2;
}
