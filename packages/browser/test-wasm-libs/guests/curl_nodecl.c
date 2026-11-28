/* No needed entry, no dlopen: the curl symbols have no
 * provider in this process, so calling one must fail, naming
 * the symbol. The suite asserts the run rejects and the error
 * names curl_easy_init. */
#include <stdio.h>
#include <curl/curl.h>

int main(void) {
  printf("before the call\n");
  CURL *h = curl_easy_init();
  printf("unreachable: %p\n", (void *)h);
  return 0;
}
