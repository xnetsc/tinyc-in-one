/* A host function that throws (design §5.7): the failure
 * crosses back to the calling thread naming the function
 * and carrying the host's error text, and the run fails
 * with it. The harness checks the rejection's message. */
#include "common.h"

int host_add(int a, int b);
int host_fail(int x);

static void *worker(void *arg) {
  (void)arg;
  return (void *)(long)host_add(40, 2);   /* 42 */
}

int main(void) {
  pthread_t t;
  if (pthread_create(&t, 0, worker, 0) != 0) return 1;
  void *ret = 0;
  if (pthread_join(t, &ret) != 0 || (int)ret != 42) return 2;
  /* The main thread's call is the one that fails. */
  host_fail(3);
  report("host_fail", -1, 0, 0, 0);   /* never reached */
  return 0;
}
