/* vector integer division has no NEON instruction and must be
   rejected by name */
#include <arm_neon.h>
int32x4_t f(int32x4_t a, int32x4_t b) { return a / b; }
