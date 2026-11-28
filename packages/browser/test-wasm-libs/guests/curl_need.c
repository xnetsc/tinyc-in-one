/* libcurl easy interface, static-dependency form: GET and
 * POST against the suite's local echo server. The server's
 * base URL arrives as TEST_HTTP_BASE at compile time. */
#include <stdio.h>
#include <string.h>
#include "guest_check.h"
#include <curl/curl.h>

static char body[8192];
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
  CK("global_init", curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
  printf("version: %s\n", curl_version());

  /* GET */
  {
    body_len = 0; body[0] = 0;
    CURL *h = curl_easy_init();
    CK("easy_init", h != NULL);
    char url[512];
    snprintf(url, sizeof(url), "%s/get", TEST_HTTP_BASE);
    curl_easy_setopt(h, CURLOPT_URL, url);
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, on_data);
    curl_easy_setopt(h, CURLOPT_USERAGENT, "tinyc-test/1.0");
    CURLcode rc = curl_easy_perform(h);
    CK("get_perform", rc == CURLE_OK);
    long code = 0;
    CK("get_info", curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &code) == CURLE_OK);
    CK("get_code_200", code == 200);
    CK("get_body_method", strstr(body, "\"method\":\"GET\"") != NULL);
    /* The echo's ua field carries the User-Agent the platform
     * actually sent: the value set with CURLOPT_USERAGENT
     * where the transport may set that header (fetch forbids
     * it in browsers, where the browser's own UA goes out). */
    CK("get_body_ua", strstr(body, "\"ua\":\"") != NULL);
    char *ct = NULL;
    curl_easy_getinfo(h, CURLINFO_CONTENT_TYPE, &ct);
    CK("get_content_type", ct != NULL && strstr(ct, "application/json") != NULL);
    curl_easy_cleanup(h);
  }

  /* POST with a custom header list */
  {
    body_len = 0; body[0] = 0;
    CURL *h = curl_easy_init();
    char url[512];
    snprintf(url, sizeof(url), "%s/echo", TEST_HTTP_BASE);
    curl_easy_setopt(h, CURLOPT_URL, url);
    curl_easy_setopt(h, CURLOPT_POST, 1L);
    curl_easy_setopt(h, CURLOPT_POSTFIELDS, "a=1&b=2");
    struct curl_slist *list = NULL;
    list = curl_slist_append(list, "X-Custom: hello");
    list = curl_slist_append(list, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(h, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, on_data);
    CURLcode rc = curl_easy_perform(h);
    CK("post_perform", rc == CURLE_OK);
    CK("post_body", strstr(body, "\"body\":\"a=1&b=2\"") != NULL);
    CK("post_method", strstr(body, "\"method\":\"POST\"") != NULL);
    curl_slist_free_all(list);
    curl_easy_cleanup(h);
  }

  /* Redirect: 302 without follow, 200 with */
  {
    body_len = 0; body[0] = 0;
    CURL *h = curl_easy_init();
    char url[512];
    snprintf(url, sizeof(url), "%s/redir", TEST_HTTP_BASE);
    curl_easy_setopt(h, CURLOPT_URL, url);
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, on_data);
    CK("redir_perform", curl_easy_perform(h) == CURLE_OK);
    long code = 0;
    curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &code);
    /* Not followed: the redirect response itself is the
     * result — status 302 where the transport exposes it.
     * A browser's fetch answers an unfollowed redirect with
     * an opaque response (status 0, no headers, no body);
     * either way the target's body must not appear. */
    CK("redir_not_followed", code == 302 || code == 0);
    CK("redir_no_body", body_len == 0);
    curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
    CK("follow_perform", curl_easy_perform(h) == CURLE_OK);
    curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &code);
    CK("follow_200", code == 200);
    curl_easy_cleanup(h);
  }

  /* Unknown option is refused by name of its code. */
  {
    CURL *h = curl_easy_init();
    CK("unknown_option", curl_easy_setopt(h, (CURLoption)99999, 1L) == CURLE_UNKNOWN_OPTION);
    CK("strerror", strcmp(curl_easy_strerror(CURLE_OK), "No error") == 0);
    curl_easy_cleanup(h);
  }

  curl_global_cleanup();
  DONE();
}
