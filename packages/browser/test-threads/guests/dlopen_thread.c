/* dlopen from a thread (design §11): a worker thread opens a
 * side module that is absent locally — the simulated tier
 * suspends the thread while the closure is fetched, the real
 * tier coordinates replicas — and calls into it. The main
 * thread then opens a second module, and the earlier thread
 * calls into that one too: the thread existed before the
 * module, so its per-(thread, module) state was backfilled. */
#include "common.h"
#include <dlfcn.h>

static int flagSecondLoaded, flagWorkerDone;
static int workerFirst, workerSecond;

static void *worker(void *arg) {
  (void)arg;
  void *h = dlopen("libfetch", RTLD_NOW | RTLD_GLOBAL);
  if (!h) { workerFirst = -1; set_flag(&flagWorkerDone, 1); return 0; }
  int (*answer)(void) = (int (*)(void))dlsym(h, "fetch_answer");
  workerFirst = answer ? answer() : -2;
  wait_flag(&flagSecondLoaded, 1);
  void *h2 = dlopen("libsecond", RTLD_NOW | RTLD_GLOBAL);
  int (*second)(void) = h2
    ? (int (*)(void))dlsym(h2, "second_answer") : 0;
  workerSecond = second ? second() : -3;
  set_flag(&flagWorkerDone, 1);
  return 0;
}

int main(void) {
  pthread_t t;
  pthread_create(&t, 0, worker, 0);
  void *h = dlopen("libsecond", RTLD_NOW | RTLD_GLOBAL);
  if (!h) { report("dlopen_thread", -9, 0, 0, 0); return 1; }
  set_flag(&flagSecondLoaded, 1);
  pthread_join(t, 0);
  report("dlopen_thread", workerFirst, workerSecond, 0, 0);
  return (workerFirst == 42 && workerSecond == 43) ? 0 : 2;
}
