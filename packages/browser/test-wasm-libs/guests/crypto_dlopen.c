/* mbedTLS crypto subset, dlopen form: no needed entry — the
 * guest loads libmbedtls by name and binds the functions
 * through dlsym. */
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include "guest_check.h"

typedef int (*sha256_fn)(const unsigned char *, size_t, unsigned char *, int);
typedef void (*aes_init_fn)(void *);
typedef int (*aes_setkey_fn)(void *, const unsigned char *, unsigned int);
typedef int (*aes_ecb_fn)(void *, int, const unsigned char *, unsigned char *);

static void hexof(const unsigned char *in, int n, char *out) {
  static const char *H = "0123456789abcdef";
  for (int i = 0; i < n; i++) { out[2*i] = H[in[i] >> 4]; out[2*i+1] = H[in[i] & 15]; }
  out[2*n] = 0;
}

int main(void) {
  void *h = dlopen("mbedtls", RTLD_NOW);
  CK("dlopen_mbedtls", h != NULL);
  if (!h) { printf("dlerror: %s\n", dlerror()); DONE(); }
  sha256_fn sha256 = (sha256_fn)dlsym(h, "mbedtls_sha256");
  CK("dlsym_sha256", sha256 != NULL);
  unsigned char out[32];
  char hex[65];
  CK("sha256_rc", sha256((const unsigned char *)"abc", 3, out, 0) == 0);
  hexof(out, 32, hex);
  CK("sha256_abc", strcmp(hex,
    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0);

  aes_init_fn aes_init = (aes_init_fn)dlsym(h, "mbedtls_aes_init");
  aes_setkey_fn setkey = (aes_setkey_fn)dlsym(h, "mbedtls_aes_setkey_enc");
  aes_ecb_fn ecb = (aes_ecb_fn)dlsym(h, "mbedtls_aes_crypt_ecb");
  CK("dlsym_aes", aes_init && setkey && ecb);
  unsigned char ctxbuf[512];
  unsigned char key[16], pt[16], ct[16];
  for (int i = 0; i < 16; i++) { key[i] = (unsigned char)i; pt[i] = (unsigned char)(i * 0x11); }
  aes_init(ctxbuf);
  CK("aes_setkey", setkey(ctxbuf, key, 128) == 0);
  CK("aes_ecb", ecb(ctxbuf, 1, pt, ct) == 0);
  hexof(ct, 16, hex);
  CK("aes128_ecb_nist", strcmp(hex, "69c4e0d86a7b0430d8cdb78070b4c55a") == 0);
  CK("dlclose", dlclose(h) == 0);
  DONE();
}
