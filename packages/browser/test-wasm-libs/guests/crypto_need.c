/* mbedTLS crypto subset, static-dependency form: the main
 * module declares libmbedtls in its needed list and calls the
 * API directly. Known-answer vectors throughout. */
#include <stdio.h>
#include <string.h>
#include "guest_check.h"
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>
#include <mbedtls/md5.h>
#include <mbedtls/md.h>
#include <mbedtls/aes.h>
#include <mbedtls/base64.h>

static void hexof(const unsigned char *in, int n, char *out) {
  static const char *H = "0123456789abcdef";
  for (int i = 0; i < n; i++) { out[2*i] = H[in[i] >> 4]; out[2*i+1] = H[in[i] & 15]; }
  out[2*n] = 0;
}

int main(void) {
  unsigned char out[64];
  char hex[129];

  CK("sha256_rc", mbedtls_sha256((const unsigned char *)"abc", 3, out, 0) == 0);
  hexof(out, 32, hex);
  CK("sha256_abc", strcmp(hex,
    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0);

  CK("sha512_rc", mbedtls_sha512((const unsigned char *)"abc", 3, out, 0) == 0);
  hexof(out, 64, hex);
  CK("sha512_abc", strcmp(hex,
    "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
    "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f") == 0);

  CK("md5_rc", mbedtls_md5((const unsigned char *)"abc", 3, out) == 0);
  hexof(out, 16, hex);
  CK("md5_abc", strcmp(hex, "900150983cd24fb0d6963f7d28e17f72") == 0);

  /* Streaming SHA-256 over 1000 patterned bytes, odd chunks. */
  {
    unsigned char data[1000];
    for (int i = 0; i < 1000; i++) data[i] = (unsigned char)(i * 7);
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    CK("sha256_starts", mbedtls_sha256_starts(&ctx, 0) == 0);
    mbedtls_sha256_update(&ctx, data, 1);
    mbedtls_sha256_update(&ctx, data + 1, 63);
    mbedtls_sha256_update(&ctx, data + 64, 500);
    mbedtls_sha256_update(&ctx, data + 564, 436);
    CK("sha256_finish", mbedtls_sha256_finish(&ctx, out) == 0);
    mbedtls_sha256_free(&ctx);
    /* The expected value is verified against the one-shot of
     * the same bytes through the generic MD layer below being
     * equal; the absolute vector is the "abc" case above. */
    unsigned char out2[32];
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    CK("md_info", info != NULL);
    CK("md_oneshot", mbedtls_md(info, data, 1000, out2) == 0);
    CK("sha256_stream_eq_oneshot", memcmp(out, out2, 32) == 0);
    CK("md_get_size", mbedtls_md_get_size(info) == 32);
  }

  /* HMAC-SHA256, RFC 4231 test case 1. */
  {
    unsigned char key[20];
    memset(key, 0x0b, sizeof(key));
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    CK("hmac_rc", mbedtls_md_hmac(info, key, 20,
      (const unsigned char *)"Hi There", 8, out) == 0);
    hexof(out, 32, hex);
    CK("hmac_sha256_rfc4231_1", strcmp(hex,
      "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7") == 0);
  }

  /* AES-128 ECB, NIST vector. */
  {
    unsigned char key[16], pt[16], ct[16], back[16];
    for (int i = 0; i < 16; i++) { key[i] = (unsigned char)i; pt[i] = (unsigned char)(i * 0x11); }
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    CK("aes_setkey_enc", mbedtls_aes_setkey_enc(&ctx, key, 128) == 0);
    CK("aes_ecb_enc", mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, pt, ct) == 0);
    hexof(ct, 16, hex);
    CK("aes128_ecb_nist", strcmp(hex, "69c4e0d86a7b0430d8cdb78070b4c55a") == 0);
    CK("aes_setkey_dec", mbedtls_aes_setkey_dec(&ctx, key, 128) == 0);
    CK("aes_ecb_dec", mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, ct, back) == 0);
    CK("aes128_ecb_roundtrip", memcmp(back, pt, 16) == 0);
    mbedtls_aes_free(&ctx);
  }

  /* AES-256 CBC, NIST SP 800-38A F.2.5 first block. */
  {
    static const unsigned char key[32] = {
      0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
      0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4 };
    static const unsigned char pt[16] = {
      0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a };
    unsigned char iv[16], ct[16], back[16];
    for (int i = 0; i < 16; i++) iv[i] = (unsigned char)i;
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx, key, 256);
    unsigned char iv1[16]; memcpy(iv1, iv, 16);
    CK("aes_cbc_enc", mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, 16, iv1, pt, ct) == 0);
    hexof(ct, 16, hex);
    CK("aes256_cbc_nist", strcmp(hex, "f58c4c04d6e5f1ba779eabfb5f7bfbd6") == 0);
    mbedtls_aes_setkey_dec(&ctx, key, 256);
    unsigned char iv2[16]; memcpy(iv2, iv, 16);
    CK("aes_cbc_dec", mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, 16, iv2, ct, back) == 0);
    CK("aes256_cbc_roundtrip", memcmp(back, pt, 16) == 0);
    mbedtls_aes_free(&ctx);
  }

  /* Base64 roundtrip against the known encoding. */
  {
    unsigned char enc[64], dec[64];
    size_t olen = 0;
    const char *msg = "hello, base64 world!";
    CK("b64_enc_rc", mbedtls_base64_encode(enc, sizeof(enc), &olen,
      (const unsigned char *)msg, strlen(msg)) == 0);
    CK("b64_enc", strcmp((char *)enc, "aGVsbG8sIGJhc2U2NCB3b3JsZCE=") == 0);
    size_t dlen = 0;
    CK("b64_dec_rc", mbedtls_base64_decode(dec, sizeof(dec), &dlen, enc, olen) == 0);
    CK("b64_roundtrip", dlen == strlen(msg) && memcmp(dec, msg, dlen) == 0);
  }

  DONE();
}
