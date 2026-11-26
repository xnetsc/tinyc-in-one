/* Exit semantics (design §11, §2.3 rule 1): a thread calling
 * exit(7) while another thread is blocked ends the run with
 * code 7; the blocked thread stops where it is. */
#include "common.h"

static pthread_mutex_t mainHeld = PTHREAD_MUTEX_INITIALIZER;
static int flagReady;

static void *blocker(void *arg) {
  (void)arg;
  set_flag(&flagReady, 1);
  pthread_mutex_lock(&mainHeld);   /* main holds it: blocks */
  return 0;
}

static void *exiter(void *arg) {
  (void)arg;
  wait_flag(&flagReady, 1);
  exit(7);
  return 0;
}

int main(void) {
  pthread_mutex_lock(&mainHeld);
  pthread_t a, b;
  pthread_create(&a, 0, blocker, 0);
  pthread_create(&b, 0, exiter, 0);
  pthread_join(a, 0);   /* never returns: exit(7) ends the run */
  pthread_join(b, 0);
  report("exit7", -1, 0, 0, 0);
  return 1;
}
