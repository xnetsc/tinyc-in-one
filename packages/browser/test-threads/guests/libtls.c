/* Side module for thread_var: defines a __thread variable
 * the main module also names directly (the GOT.tls path), so
 * both views of it must agree per thread. */
__thread int ext_tv = 7;

int get_ext(void) { return ext_tv; }
void set_ext(int v) { ext_tv = v; }
