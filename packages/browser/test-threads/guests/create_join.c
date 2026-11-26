/* create/join (design §11): N threads bump disjoint counters
 * and return distinct values; join collects them all; a second
 * join and a join of a detached thread return EINVAL, a
 * self-join EDEADLK, an unknown id ESRCH. */
#include "common.h"

#define NTHREADS 4
static int counters[NTHREADS];
static pthread_t ids[NTHREADS];

static void *worker(void *arg) {
  int me = (int)arg;
  for (int i = 0; i < 250; i++) counters[me]++;
  return (void *)(me * 7 + 1);
}

static void *noop(void *arg) { (void)arg; return 0; }

int main(void) {
  for (int i = 0; i < NTHREADS; i++) {
    if (pthread_create(&ids[i], 0, worker, (void *)i) != 0) {
      report("create_join", -1, i, 0, 0);
      return 1;
    }
  }
  int sum = 0, rets = 0;
  for (int i = 0; i < NTHREADS; i++) {
    void *ret = 0;
    int rc = pthread_join(ids[i], &ret);
    if (rc != 0) { report("create_join", -2, i, rc, 0); return 1; }
    sum += counters[i];
    rets += (int)ret;
  }
  int second = pthread_join(ids[0], 0);
  pthread_t detached;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, 1);
  pthread_create(&detached, &attr, noop, 0);
  int joinDetached = pthread_join(detached, 0);
  int selfJoin = pthread_join(pthread_self(), 0);
  int unknown = pthread_join(9999, 0);
  /* counters: 4*250 = 1000; rets: 1+8+15+22 = 46 */
  report("create_join", sum, rets, second, joinDetached);
  report("create_join2", selfJoin, unknown, 0, 0);
  return (sum == 1000 && rets == 46 && second == EINVAL &&
          joinDetached == EINVAL && selfJoin == EDEADLK &&
          unknown == ESRCH) ? 0 : 2;
}
