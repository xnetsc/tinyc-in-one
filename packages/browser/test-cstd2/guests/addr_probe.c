/* Taking the address of an imported function: as a value in
 * code, in a static initializer, and in a table mixing imported
 * and local functions. Compiled both standalone and as a dylink
 * main module; the standalone form is the one that used to fail
 * at compile time with "cannot resolve address of 'abs'". */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "guest_check.h"

static int (*abs_fp)(int) = abs;
static size_t (*len_fp)(const char *) = strlen;

static int local_double(int x) { return 2 * x; }
static int (*table[2])(int) = { abs, local_double };

int main(void) {
  int (*p)(int) = abs;
  CK("addr_call_local_var", p(-7) == 7);
  CK("addr_static_init", abs_fp(-8) == 8);
  CK("addr_strlen_init", len_fp("hello") == 5);
  CK("addr_table_import", table[0](-9) == 9);
  CK("addr_table_local", table[1](21) == 42);
  int (*q)(int) = &abs;
  CK("addr_explicit_amp", q(-3) == 3 && q == p);
  DONE();
}
