/* Mutex kinds (design §11): recursive counting, error-checking
 * relock EDEADLK, non-owner unlock EPERM, trylock EBUSY,
 * destroying a locked mutex EBUSY. */
#include "common.h"

static pthread_mutex_t held = PTHREAD_MUTEX_INITIALIZER;
static int flagLocked, flagRelease, unlockByOther;

static void *holder(void *arg) {
  (void)arg;
  pthread_mutex_lock(&held);
  set_flag(&flagLocked, 1);
  wait_flag(&flagRelease, 1);
  pthread_mutex_unlock(&held);
  return 0;
}

int main(void) {
  int fails = 0;

  /* recursive: relock by the owner counts, trylock too */
  pthread_mutexattr_t ma;
  pthread_mutexattr_init(&ma);
  pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_t rec;
  pthread_mutex_init(&rec, &ma);
  int r = 0;
  r += pthread_mutex_lock(&rec);
  r += pthread_mutex_lock(&rec);
  r += pthread_mutex_lock(&rec);
  r += pthread_mutex_trylock(&rec);
  r += pthread_mutex_unlock(&rec);
  r += pthread_mutex_unlock(&rec);
  r += pthread_mutex_unlock(&rec);
  r += pthread_mutex_unlock(&rec);
  if (r != 0) fails++;
  pthread_mutex_destroy(&rec);

  /* error-checking: relock is EDEADLK */
  pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_t ec;
  pthread_mutex_init(&ec, &ma);
  pthread_mutex_lock(&ec);
  int relock = pthread_mutex_lock(&ec);
  if (relock != EDEADLK) fails++;
  pthread_mutex_unlock(&ec);
  pthread_mutex_destroy(&ec);

  /* cross-thread checks against a mutex another thread holds */
  pthread_t t;
  pthread_create(&t, 0, holder, 0);
  wait_flag(&flagLocked, 1);
  int tryHeld = pthread_mutex_trylock(&held);
  unlockByOther = pthread_mutex_unlock(&held);
  int destroyHeld = pthread_mutex_destroy(&held);
  if (tryHeld != EBUSY) fails++;
  if (unlockByOther != EPERM) fails++;
  if (destroyHeld != EBUSY) fails++;
  set_flag(&flagRelease, 1);
  pthread_join(t, 0);
  pthread_mutex_destroy(&held);

  report("mutex_kinds", relock, tryHeld, unlockByOther, destroyHeld);
  return fails ? 2 : 0;
}
