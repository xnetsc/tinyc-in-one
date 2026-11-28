/* libcurl WebSocket face, static-dependency form: connect
 * with perform, echo a text frame and a binary frame through
 * the suite's echo server. */
#include <stdio.h>
#include <string.h>
#include "guest_check.h"
#include <curl/curl.h>

int main(void) {
  CURL *h = curl_easy_init();
  CK("easy_init", h != NULL);
  char url[512];
  snprintf(url, sizeof(url), "%s/ws", TEST_WS_BASE);
  curl_easy_setopt(h, CURLOPT_URL, url);
  curl_easy_setopt(h, CURLOPT_CONNECT_ONLY, 1L);
  CURLcode rc = curl_easy_perform(h);
  CK("ws_perform", rc == CURLE_OK);

  /* Text frame echo. */
  {
    const char *msg = "hello ws";
    size_t sent = 0;
    rc = curl_ws_send(h, msg, strlen(msg), &sent, 0, CURLWS_TEXT);
    CK("ws_send_text", rc == CURLE_OK && sent == strlen(msg));
    char buf[128];
    size_t recvd = 0;
    const struct curl_ws_frame *meta = NULL;
    rc = curl_ws_recv(h, buf, sizeof(buf), &recvd, &meta);
    CK("ws_recv_text", rc == CURLE_OK && recvd == strlen(msg)
       && memcmp(buf, msg, recvd) == 0);
    CK("ws_meta_text", meta != NULL && (meta->flags & CURLWS_TEXT));
    CK("ws_meta_len", meta != NULL && meta->len == recvd && meta->bytesleft == 0);
  }

  /* Binary frame echo. */
  {
    static const unsigned char bin[5] = { 0x00, 0x01, 0x02, 0xff, 0xfe };
    size_t sent = 0;
    rc = curl_ws_send(h, bin, sizeof(bin), &sent, 0, CURLWS_BINARY);
    CK("ws_send_binary", rc == CURLE_OK && sent == sizeof(bin));
    unsigned char buf[128];
    size_t recvd = 0;
    const struct curl_ws_frame *meta = NULL;
    rc = curl_ws_recv(h, buf, sizeof(buf), &recvd, &meta);
    CK("ws_recv_binary", rc == CURLE_OK && recvd == sizeof(bin)
       && memcmp(buf, bin, sizeof(bin)) == 0);
    CK("ws_meta_binary", meta != NULL && (meta->flags & CURLWS_BINARY));
    const struct curl_ws_frame *m2 = curl_ws_meta(h);
    CK("ws_meta_fn", m2 != NULL && m2->len == sizeof(bin));
  }

  curl_easy_cleanup(h);
  DONE();
}
