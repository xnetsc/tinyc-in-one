/* errno as a working lvalue: set by a failed call, read,
 * stored through, cleared, set again — and reported by perror
 * (the runner checks the message text). pthread.h comes first:
 * the two headers publish some of the same numbers and must
 * coexist in this order too. */
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include "guest_check.h"

int main(void) {
  errno = 0;
  FILE *f = fopen("/no-such-dir/nope.txt", "r");
  CK("fopen_fails", f == NULL);
  CK("errno_enoent", errno == ENOENT);
  int *p = &errno;
  *p = EINVAL;
  CK("errno_store_load", errno == EINVAL);
  errno = 0;
  CK("errno_cleared", errno == 0);
  FILE *g = fopen("/also-missing.txt", "r");
  CK("errno_set_again", g == NULL && errno == ENOENT);
  perror("cstd2-perror");
  DONE();
}
