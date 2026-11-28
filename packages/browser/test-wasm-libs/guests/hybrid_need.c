/* Hybrid unit coverage, static-dependency form. The unit
 * (libhybrid.tinyc.js) carries a wasm body and a JS part in
 * one module record; this guest exercises both kinds, both
 * call directions, function pointers across the kinds, a JS
 * call back into a guest callback, and data shared through
 * the module's memory. */
#include <stdio.h>
#include "guest_check.h"

int wasm_double(int x);
int wasm_calls_js(int x);
int wasm_bump(void);
int wasm_read_counter(void);
int js_add(int x);
int js_calls_wasm(int x);
int js_read_counter(void);
int js_invoke_cb(int (*cb)(int), int x);

static int triple(int x) { return x * 3; }

int main(void) {
  CK("wasm_export", wasm_double(21) == 42);
  CK("js_export", js_add(1) == 101);              /* counter: 1 */
  CK("wasm_calls_js", wasm_calls_js(1) == 102);   /* counter: 2 */
  CK("js_calls_wasm", js_calls_wasm(21) == 43);

  /* The address of a JS implementation, called through the
   * process table like any function pointer. */
  int (*fp)(int) = js_add;
  CK("js_via_fptr", fp(2) == 102);                /* counter: 3 */

  /* A JS implementation invoking a guest callback pointer. */
  CK("js_invoke_cb", js_invoke_cb(triple, 5) == 15);

  /* Data across kinds: the JS part's bumps are visible to
   * the wasm side, and the wasm side's bump to the JS part. */
  CK("counter_js_to_wasm", wasm_read_counter() == 3);
  wasm_bump();                                    /* counter: 4 */
  CK("counter_wasm_to_js", js_read_counter() == 4);

  DONE();
}
