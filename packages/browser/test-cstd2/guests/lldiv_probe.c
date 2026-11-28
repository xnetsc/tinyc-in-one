/* libtcc1 helpers in ordinary code: long long division and
 * remainder, variable-count 64-bit shifts, and the float
 * conversions — the calls the compiler emits __divdi3 and
 * family for. No declarations by hand: the point is that plain
 * code resolves them. */
#include <stdio.h>
#include <stdlib.h>
#include "guest_check.h"

static long long dvd(long long a, long long b) { return a / b; }
static long long mod(long long a, long long b) { return a % b; }
static unsigned long long udvd(unsigned long long a, unsigned long long b) { return a / b; }
static unsigned long long umod(unsigned long long a, unsigned long long b) { return a % b; }
static long long shl(long long a, int n) { return a << n; }
static long long shr(long long a, int n) { return a >> n; }
static unsigned long long ushr(unsigned long long a, int n) { return a >> n; }

int main(void) {
  CK("div_exact", dvd(97000000679LL, 97) == 1000000007LL);
  CK("mod_exact", mod(97000000679LL, 97) == 0);
  CK("div_neg", dvd(-100000000000LL, 7) == -14285714285LL);
  CK("mod_neg", mod(-100000000000LL, 7) == -5);
  CK("div_neg_divisor", dvd(100000000000LL, -7) == -14285714285LL);
  CK("mod_neg_divisor", mod(100000000000LL, -7) == 5);
  CK("udiv_pow2", udvd(1ULL << 60, 1ULL << 20) == (1ULL << 40));
  CK("umod_pow2", umod(1ULL << 60, 1000) == 976);
  CK("udiv_big", udvd(12345678901234567890ULL, 1000000ULL) == 12345678901234ULL);
  CK("umod_big", umod(12345678901234567890ULL, 1000000ULL) == 567890);
  CK("divmod_identity",
     dvd(-98765432198765LL, 12345) * 12345 + mod(-98765432198765LL, 12345)
       == -98765432198765LL);
  CK("shl_var", shl(3, 40) == 3298534883328LL);
  CK("shr_var_neg", shr(-(1LL << 50), 20) == -1073741824LL);
  CK("ushr_var", ushr(1ULL << 60, 44) == 65536);
  CK("shl_one", shl(1, 63) == (-9223372036854775807LL - 1));
  /* The unit's float conversion helpers, reached through casts
   * of runtime values. */
  CK("fixdfdi", (long long)strtod("3700000000.5", NULL) == 3700000000LL);
  CK("floatdidf", (double)9007199254740993LL == 9007199254740992.0);
  DONE();
}
