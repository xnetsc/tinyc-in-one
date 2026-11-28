/* Guest test helpers: ok/FAIL lines and a failure count. */
#ifndef GUEST_CHECK_H
#define GUEST_CHECK_H
#include <stdio.h>
static int guest_failures = 0;
#define CK(name, cond) do { \
    if (cond) { printf("ok %s\n", name); } \
    else { printf("FAIL %s\n", name); guest_failures++; } \
  } while (0)
#define DONE() do { \
    printf("done: %d failures\n", guest_failures); \
    return guest_failures; \
  } while (0)
#endif
