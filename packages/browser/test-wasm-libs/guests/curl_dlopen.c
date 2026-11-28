/* libcurl, dlopen form: a GET through functions bound with
 * dlsym after dlopen("curl"). */
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include "guest_check.h"

typedef void *(*init_fn)(void);
typedef int (*setopt_fn)(void *, int, ...);
typedef int (*perform_fn)(void *);
typedef int (*getinfo_fn)(void *, int, ...);
typedef void (*cleanup_fn)(void *);

static char body[4096];
static size_t body_len = 0;
static size_t on_data(char *ptr, size_t size, size_t nmemb, void *userdata) {
  size_t n = size * nmemb;
  if (body_len + n < sizeof(body)) {
    memcpy(body + body_len, ptr, n);
    body_len += n;
    body[body_len] = 0;
  }
  return n;
}

int main(void) {
  void *h = dlopen("curl", RTLD_NOW);
  CK("dlopen_curl", h != NULL);
  if (!h) { printf("dlerror: %s\n", dlerror()); DONE(); }
  init_fn easy_init = (init_fn)dlsym(h, "curl_easy_init");
  setopt_fn setopt = (setopt_fn)dlsym(h, "curl_easy_setopt");
  perform_fn perform = (perform_fn)dlsym(h, "curl_easy_perform");
  getinfo_fn getinfo = (getinfo_fn)dlsym(h, "curl_easy_getinfo");
  cleanup_fn cleanup = (cleanup_fn)dlsym(h, "curl_easy_cleanup");
  CK("dlsym_all", easy_init && setopt && perform && getinfo && cleanup);

  void *easy = easy_init();
  CK("easy_init", easy != NULL);
  char url[512];
  snprintf(url, sizeof(url), "%s/get", TEST_HTTP_BASE);
  /* CURLOPT_URL=10002, WRITEFUNCTION=20011 — passed as plain
   * values because this guest binds the variadic prototypes
   * itself. */
  setopt(easy, 10002, url);
  setopt(easy, 20011, on_data);
  CK("perform", perform(easy) == 0);
  long code = 0;
  getinfo(easy, 0x200002 /* CURLINFO_RESPONSE_CODE */, &code);
  CK("code_200", code == 200);
  CK("body_method", strstr(body, "\"method\":\"GET\"") != NULL);
  cleanup(easy);
  CK("dlclose", dlclose(h) == 0);
  DONE();
}
