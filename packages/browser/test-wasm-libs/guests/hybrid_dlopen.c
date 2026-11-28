/* Hybrid unit coverage, dlopen form. */
#include <stdio.h>
#include <dlfcn.h>
#include "guest_check.h"

typedef int (*i2i)(int);
typedef int (*i2v)(void);

int main(void) {
  void *h = dlopen("hybrid", RTLD_NOW);
  CK("dlopen_hybrid", h != NULL);
  if (!h) { printf("dlerror: %s\n", dlerror()); DONE(); }
  i2i wasm_double = (i2i)dlsym(h, "wasm_double");
  i2i js_add = (i2i)dlsym(h, "js_add");
  i2i wasm_calls_js = (i2i)dlsym(h, "wasm_calls_js");
  i2i js_calls_wasm = (i2i)dlsym(h, "js_calls_wasm");
  i2v wasm_read_counter = (i2v)dlsym(h, "wasm_read_counter");
  CK("dlsym_all", wasm_double && js_add && wasm_calls_js && js_calls_wasm
     && wasm_read_counter);
  CK("wasm_export", wasm_double(21) == 42);
  CK("js_export", js_add(1) == 101);
  CK("wasm_calls_js", wasm_calls_js(1) == 102);
  CK("js_calls_wasm", js_calls_wasm(21) == 43);
  CK("counter_js_to_wasm", wasm_read_counter() == 2);
  CK("dlclose", dlclose(h) == 0);
  DONE();
}
