/* Default libraries battery: one-shot HTTPS GET for the
   interoperability legs. Fetches the URL with certificate
   verification on, against the given CA file, and prints the
   body. Usage: default_libs_httpsget <url> <ca-file> */
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

static char buf[4096];
static size_t blen;
static size_t wr(void *p, size_t sz, size_t nm, void *u) {
    size_t n = sz * nm;
    memcpy(buf + blen, p, n);
    blen += n;
    buf[blen] = 0;
    return n;
}

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    CURL *c = curl_easy_init();
    if (!c) return 1;
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, wr);
    curl_easy_setopt(c, CURLOPT_URL, argv[1]);
    curl_easy_setopt(c, CURLOPT_CAINFO, argv[2]);
    CURLcode rc = curl_easy_perform(c);
    if (rc != CURLE_OK) {
        printf("httpsget: %s\n", curl_easy_strerror(rc));
        return 1;
    }
    printf("%s", buf);
    return 0;
}
