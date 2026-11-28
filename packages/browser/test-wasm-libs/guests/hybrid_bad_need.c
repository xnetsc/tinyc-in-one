/* Main for the duplicate-definition case: it declares the
 * bad hybrid unit (whose JS part also claims the wasm
 * export wasm_double) as a needed dependency. The load must
 * fail before main runs, naming the duplicated export; the
 * suite asserts the rejection text. */
#include <stdio.h>

int wasm_double(int x);

int main(void) {
  printf("unreachable: %d\n", wasm_double(1));
  return 0;
}
