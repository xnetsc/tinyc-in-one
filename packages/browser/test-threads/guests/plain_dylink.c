/* Regression shape (design §11): an ordinary dylink program
 * with no pthread use, run in an unthreaded process and in a
 * threaded one; the observable behavior must be identical. */
#include "common.h"
#include <dlfcn.h>

int main(void) {
  void *h = dlopen("libplain", RTLD_NOW);
  if (!h) { report("plain", -1, 0, 0, 0); return 1; }
  int (*answer)(void) = (int (*)(void))dlsym(h, "plain_answer");
  int v = answer ? answer() : -2;
  report("plain", v, 0, 0, 0);
  return v == 42 ? 0 : 2;
}
