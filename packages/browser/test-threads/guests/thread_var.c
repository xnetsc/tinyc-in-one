/* __thread variables (design §11): two threads write and
 * read back distinct values of the same variable, in its
 * initialized (.tdata) and zero (.tbss) forms, and an extern
 * __thread defined by a side module behaves the same through
 * GOT.tls. The main thread's values are its own throughout. */
#include "common.h"

__thread int tv = 41;
__thread char tbss[8];
extern __thread int ext_tv;
int get_ext(void);
void set_ext(int v);

static int seenTv[2], seenBss[2], seenExt[2];

static void *worker(void *arg) {
  int me = (int)arg;
  tv = 100 + me;
  tbss[0] = (char)(50 + me);
  set_ext(200 + me);
  seenTv[me] = tv;
  seenBss[me] = tbss[0];
  seenExt[me] = ext_tv;   /* direct view must match get_ext() */
  if (seenExt[me] != get_ext()) seenExt[me] = -1;
  return 0;
}

int main(void) {
  int before = tv;
  pthread_t ids[2];
  for (int i = 0; i < 2; i++) pthread_create(&ids[i], 0, worker, (void *)i);
  for (int i = 0; i < 2; i++) pthread_join(ids[i], 0);
  tv = 55;
  report("thread_var", before, tv, get_ext(), 0);
  report("thread_seen", seenTv[0], seenTv[1], seenBss[0], seenBss[1]);
  report("thread_ext", seenExt[0], seenExt[1], 0, 0);
  return (before == 41 && tv == 55 && get_ext() == 7 &&
          seenTv[0] == 100 && seenTv[1] == 101 &&
          seenBss[0] == 50 && seenBss[1] == 51 &&
          seenExt[0] == 200 && seenExt[1] == 201) ? 0 : 2;
}
