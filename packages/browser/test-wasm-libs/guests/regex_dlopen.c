/* PCRE2, dlopen form. */
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>
#include "guest_check.h"

typedef void *(*compile_fn)(const char *, size_t, uint32_t, int *, size_t *, void *);
typedef void *(*mdcreate_fn)(void *, void *);
typedef int (*match_fn)(void *, const char *, size_t, size_t, uint32_t, void *, void *);
typedef size_t *(*ovector_fn)(void *);

int main(void) {
  void *h = dlopen("pcre2", RTLD_NOW);
  CK("dlopen_pcre2", h != NULL);
  if (!h) { printf("dlerror: %s\n", dlerror()); DONE(); }
  compile_fn compile = (compile_fn)dlsym(h, "pcre2_compile_8");
  mdcreate_fn mdcreate = (mdcreate_fn)dlsym(h, "pcre2_match_data_create_from_pattern_8");
  match_fn match = (match_fn)dlsym(h, "pcre2_match_8");
  ovector_fn ovector = (ovector_fn)dlsym(h, "pcre2_get_ovector_pointer_8");
  CK("dlsym_all", compile && mdcreate && match && ovector);
  int errcode = 0;
  size_t erroffset = 0;
  void *code = compile("(\\w+)@(\\w+)", ~(size_t)0, 0, &errcode, &erroffset, NULL);
  CK("compile", code != NULL);
  void *md = mdcreate(code, NULL);
  int rc = match(code, "user@host", 9, 0, 0, md, NULL);
  CK("match_rc", rc == 3);
  size_t *ov = ovector(md);
  CK("ov_group2", ov[4] == 5 && ov[5] == 9);
  CK("dlclose", dlclose(h) == 0);
  DONE();
}
