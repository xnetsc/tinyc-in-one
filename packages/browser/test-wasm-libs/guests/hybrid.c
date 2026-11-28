/* The wasm side of the hybrid test unit (libhybrid.tinyc.js).
 * Compiled as a dylink side module; the unit's JS part
 * provides js_add, which this side imports and calls. The
 * shared counter lives in this side's static memory; the JS
 * part reaches it through counter_addr. */
extern int js_add(int x);

static int counter = 0;

int *counter_addr(void) { return &counter; }
int wasm_double(int x) { return x * 2; }
int wasm_calls_js(int x) { return js_add(x) + 1; }
int wasm_bump(void) { return ++counter; }
int wasm_read_counter(void) { return counter; }
