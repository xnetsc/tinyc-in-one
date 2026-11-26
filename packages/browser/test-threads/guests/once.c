/* pthread_once (design §11): concurrent first calls from all
 * threads run the init routine exactly once per object. */
#include "common.h"

static pthread_once_t onces[3] = {
  PTHREAD_ONCE_INIT, PTHREAD_ONCE_INIT, PTHREAD_ONCE_INIT,
};
static int initCount[3];
static int which;

static void init0(void) { __atomic_add_fetch(&initCount[0], 1, __ATOMIC_SEQ_CST); }
static void init1(void) { __atomic_add_fetch(&initCount[1], 1, __ATOMIC_SEQ_CST); }
static void init2(void) { __atomic_add_fetch(&initCount[2], 1, __ATOMIC_SEQ_CST); }
static void (*const inits[3])(void) = { init0, init1, init2 };

static void *worker(void *arg) {
  (void)arg;
  for (int round = 0; round < 5; round++)
    for (int k = 0; k < 3; k++) pthread_once(&onces[k], inits[k]);
  return 0;
}

int main(void) {
  pthread_t ids[4];
  for (int i = 0; i < 4; i++) pthread_create(&ids[i], 0, worker, 0);
  for (int i = 0; i < 4; i++) pthread_join(ids[i], 0);
  report("once", initCount[0], initCount[1], initCount[2], 0);
  return (initCount[0] == 1 && initCount[1] == 1 && initCount[2] == 1)
    ? 0 : 2;
}
