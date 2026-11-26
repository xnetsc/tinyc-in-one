/* No-JSPI environment (design §11, §4.2): without
 * WebAssembly.promising, a direct module's pthread_create
 * fails at the call site with the named error; there is no
 * approximate fallback. The harness runs this guest with
 * promising temporarily removed and expects the run to fail
 * with that error. */
#include "common.h"

static void *worker(void *arg) { (void)arg; return 0; }

int main(void) {
  pthread_t t;
  int rc = pthread_create(&t, 0, worker, 0);
  report("nojspi", rc, 0, 0, 0);
  return 0;
}
