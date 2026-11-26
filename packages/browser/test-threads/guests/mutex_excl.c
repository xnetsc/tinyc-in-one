/* Mutex exclusion (design §11): N x M increments of one
 * shared counter under one mutex give the exact total. In the
 * real tier this also proves the atomic emission and the
 * futex-style wait path; in the simulated tier it proves the
 * rotation interleaves at blocking points. */
#include "common.h"

#define NTHREADS 4
#define PER 5000
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int counter;

static void *worker(void *arg) {
  (void)arg;
  for (int i = 0; i < PER; i++) {
    pthread_mutex_lock(&lock);
    counter++;
    pthread_mutex_unlock(&lock);
  }
  return 0;
}

int main(void) {
  pthread_t ids[NTHREADS];
  for (int i = 0; i < NTHREADS; i++)
    pthread_create(&ids[i], 0, worker, 0);
  for (int i = 0; i < NTHREADS; i++) pthread_join(ids[i], 0);
  report("mutex_excl", counter, NTHREADS * PER, 0, 0);
  return counter == NTHREADS * PER ? 0 : 2;
}
