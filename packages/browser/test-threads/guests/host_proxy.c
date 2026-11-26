/* Host functions (design §5.7): in the real tier they exist
 * only in the coordinator's context and a worker thread's
 * call crosses through the proxy; in the simulated tier the
 * same calls bind directly. Scalar arguments and results of
 * every word type, a pointer argument the host dereferences
 * in the shared memory, and a host function that answers
 * asynchronously — called from the main thread and from a
 * created thread alike. The asynchronous answer is part of
 * the proxy's contract; the simulated tier's direct binding
 * does not await it, so the harness asserts that line only
 * in the real tier. */
#include "common.h"

int host_add(int a, int b);
long long host_add64(long long a, long long b);
float host_halve(float x);
double host_scale(double x, double f);
int host_bump(int *counter);
int host_async_add(int a, int b);

static int counter = 41;
static int seen[8];

static void *worker(void *arg) {
  (void)arg;
  seen[0] = host_add(19, 23);                        /* 42 */
  long long v = host_add64(4000000000LL, 2000000005LL);
  seen[1] = (int)(v >> 32);                          /* 1 */
  seen[2] = (int)v;                                  /* 1705032709 */
  seen[3] = (int)(host_halve(9.0f) * 10);            /* 45 */
  seen[4] = (int)(host_scale(1.5, 4.0) * 10);        /* 60 */
  seen[5] = host_bump(&counter);                     /* 41; counter -> 48 */
  seen[6] = host_async_add(30, 12);                  /* 42 */
  seen[7] = counter;                                 /* 48 */
  return 0;
}

int main(void) {
  /* The main thread reaches the host through the same path
   * (in the real tier it runs in the main worker). */
  int mine = host_add(1, 2);                         /* 3 */
  pthread_t t;
  if (pthread_create(&t, 0, worker, 0) != 0) {
    report("host_proxy", -1, 0, 0, 0);
    return 1;
  }
  void *ret = 0;
  int rc = pthread_join(t, &ret);
  report("host_proxy", seen[0], seen[1], seen[3], seen[4]);
  report("host_proxy2", seen[5], seen[7], mine, rc);
  report("host_proxy3", seen[2], (int)ret, 0, 0);
  report("host_async", seen[6], 0, 0, 0);
  return (seen[0] == 42 && seen[1] == 1 && seen[2] == 1705032709 &&
          seen[3] == 45 && seen[4] == 60 && seen[5] == 41 &&
          seen[7] == 48 && mine == 3 && rc == 0)
    ? 0 : 2;
}
