/* Directory block boundary (design §6.6): the harness
 * presets the process's thread-id counter just below 8192,
 * so the few threads created here straddle the first block
 * boundary and enter the second block. Every one must
 * register, join with its exit value, and detach exactly as
 * below the boundary. Real tier only — the simulated tier
 * keeps no directory. */
#include "common.h"

static int go;

static void *worker(void *arg) { return arg; }

static void *gated(void *arg) {
  wait_flag(&go, 1);
  return arg;
}

int main(void) {
  pthread_t a, b, c, d;
  void *ra = 0, *rb = 0, *rc = 0;
  if (pthread_create(&a, 0, worker, (void *)11) != 0) return 1;
  if (pthread_create(&b, 0, worker, (void *)22) != 0) return 1;
  if (pthread_create(&c, 0, worker, (void *)33) != 0) return 1;
  int ja = pthread_join(a, &ra);
  int jb = pthread_join(b, &rb);
  int jc = pthread_join(c, &rc);
  /* The detached thread stays alive on the gate while main
   * detaches and joins it, so both calls see its record. */
  if (pthread_create(&d, 0, gated, (void *)44) != 0) return 1;
  int det = pthread_detach(d);
  int jd = pthread_join(d, 0);   /* EINVAL: detached */
  set_flag(&go, 1);
  report("dir_boundary", (int)a, (int)b, (int)c, (int)d);
  report("dir_join", ja + jb + jc, (int)ra + (int)rb + (int)rc, det, jd);
  return (ja == 0 && jb == 0 && jc == 0 && (int)ra == 11 &&
          (int)rb == 22 && (int)rc == 33 && det == 0 &&
          jd == EINVAL) ? 0 : 2;
}
