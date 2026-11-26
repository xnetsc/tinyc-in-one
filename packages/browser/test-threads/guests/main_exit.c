/* Exit semantics (design §11, §2.3 rule 3): the main thread
 * pthread_exits while a worker is still computing; when the
 * worker finishes, the run ends with {exitCode: 0}. The
 * worker's result stays readable in the exported variable
 * after the run. */
#include "common.h"

int worker_result;
static int flagGo;

static void *worker(void *arg) {
  (void)arg;
  int acc = 0;
  for (int i = 1; i <= 100; i++) {
    acc += i;
    if ((i & 7) == 0) sched_yield();
  }
  worker_result = acc;
  return 0;
}

int main(void) {
  pthread_t t;
  pthread_create(&t, 0, worker, 0);
  pthread_exit(0);
  return 1;   /* not reached */
}
