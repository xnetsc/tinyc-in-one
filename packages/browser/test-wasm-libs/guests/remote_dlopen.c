/* Remote js-kind load, dlopen form: this process's local
 * module source does not serve libpcre2, so the guest's
 * dlopen retrieves the unit from the base URL — a js-kind
 * unit fetched remotely loads and runs like a local one. */
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>
#include "guest_check.h"

typedef void *(*compile_fn)(const char *, size_t, uint32_t, int *, size_t *, void *);
typedef void *(*mdcreate_fn)(void *, void *);
typedef int (*match_fn)(void *, const char *, size_t, size_t, uint32_t, void *, void *);

int main(void) {
  void *h = dlopen("pcre2", RTLD_NOW);
  CK("dlopen_remote_pcre2", h != NULL);
  if (!h) { printf("dlerror: %s\n", dlerror()); DONE(); }
  compile_fn compile = (compile_fn)dlsym(h, "pcre2_compile_8");
  mdcreate_fn mdcreate = (mdcreate_fn)dlsym(h, "pcre2_match_data_create_from_pattern_8");
  match_fn match = (match_fn)dlsym(h, "pcre2_match_8");
  CK("dlsym_all", compile && mdcreate && match);
  int errcode = 0;
  size_t erroffset = 0;
  void *code = compile("remote-(\\d+)", ~(size_t)0, 0, &errcode, &erroffset, NULL);
  CK("compile", code != NULL);
  void *md = mdcreate(code, NULL);
  CK("match", match(code, "a remote-42 z", 13, 0, 0, md, NULL) == 2);
  CK("dlclose", dlclose(h) == 0);
  DONE();
}
