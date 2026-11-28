/* time.h declares what the layer supplies: time, clock,
 * difftime. Declared only through the header — with the old
 * types-only header this file needed hand declarations, and
 * difftime's double return was silently taken as int. */
#include <stdio.h>
#include <time.h>
#include "guest_check.h"

int main(void) {
  time_t t0 = time(NULL);
  CK("time_positive", t0 > 1700000000);
  time_t t1 = 0;
  time_t r = time(&t1);
  CK("time_store", r == t1 && t1 >= t0);
  clock_t c0 = clock();
  CK("clock_nonneg", c0 >= 0);
  volatile long s = 0;
  for (long i = 0; i < 200000; i++) s += i;
  clock_t c1 = clock();
  CK("clock_monotonic", c1 >= c0);
  CK("difftime_exact", difftime((time_t)100000, (time_t)40000) == 60000.0);
  CK("difftime_negative", difftime((time_t)2, (time_t)7) == -5.0);
  DONE();
}
