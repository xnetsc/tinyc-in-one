/* dlopen the same library twice from the main thread
 * (design §5.4): the first call loads it through the
 * coordinator, the second must find the already-loaded
 * replica by name in the worker and return the same
 * handle — no second load, no failed lookup. */
#include "common.h"
#include <dlfcn.h>

int main(void) {
  void *h1 = dlopen("libplain", RTLD_NOW);
  if (!h1) { report("dlopen_twice", -1, 0, 0, 0); return 1; }
  void *h2 = dlopen("libplain", RTLD_NOW);
  if (!h2) { report("dlopen_twice", -2, 0, 0, 0); return 1; }
  int (*answer)(void) = (int (*)(void))dlsym(h1, "plain_answer");
  int v = answer ? answer() : -3;
  report("dlopen_twice", v, h1 == h2 ? 1 : 0, 0, 0);
  return (v == 42 && h1 == h2) ? 0 : 2;
}
