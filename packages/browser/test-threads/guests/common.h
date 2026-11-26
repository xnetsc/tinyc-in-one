/* Shared declarations for the thread test guests. The guests
 * are freestanding dylink modules: libc surface comes from the
 * process's shims, declared here rather than through hosted
 * headers. */
#ifndef TINYC_THREAD_TEST_COMMON_H
#define TINYC_THREAD_TEST_COMMON_H

#include <pthread.h>
#include <stdatomic.h>   /* for the __ATOMIC_* order constants */

int printf(const char *fmt, ...);
void exit(int code);
long clock(void);
long time(long *tloc);

/* Spin-wait for an atomic flag, yielding so the simulated
 * tier's rotation can run the thread that sets it. In the real
 * tier sched_yield is a no-op and the spin is a plain poll. */
static void wait_flag(int *flag, int want) {
  while (__atomic_load_n(flag, __ATOMIC_ACQUIRE) != want) {
    sched_yield();
  }
}

static void set_flag(int *flag, int value) {
  __atomic_store_n(flag, value, __ATOMIC_RELEASE);
}

/* Print one "name: a b c" result line; the harness compares
 * the numbers against the case's expectations. */
static void report(const char *name, int a, int b, int c, int d) {
  printf("%s: %d %d %d %d\n", name, a, b, c, d);
}

#endif
