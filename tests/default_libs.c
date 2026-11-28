/* Default libraries battery, part 1: PCRE2 matching and
   mbedTLS known-answer vectors. Built with the packaged
   compiler against the staged libraries (-lpcre2
   -lmbedcrypto) and run on the target. Prints one line per
   check and exits nonzero on the first failure. */
#define PCRE2_CODE_UNIT_WIDTH 8
#include <stdio.h>
#include <string.h>
#include <pcre2.h>
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/aes.h"
#include "mbedtls/gcm.h"
#include "mbedtls/md.h"

static int check_pcre2(void) {
    int err;
    PCRE2_SIZE erroff;
    pcre2_code *re = pcre2_compile(
        (PCRE2_SPTR)"^(\\w+)@(\\w+)\\.com$", PCRE2_ZERO_TERMINATED,
        0, &err, &erroff, NULL);
    if (!re) { printf("pcre2: compile failed (%d)\n", err); return 1; }
    pcre2_match_data *md = pcre2_match_data_create_from_pattern(re, NULL);
    const char *subj = "user@example.com";
    int rc = pcre2_match(re, (PCRE2_SPTR)subj, PCRE2_ZERO_TERMINATED,
                         0, 0, md, NULL);
    if (rc != 3) { printf("pcre2: match rc=%d\n", rc); return 1; }
    PCRE2_SIZE *ov = pcre2_get_ovector_pointer(md);
    char g1[32], g2[32];
    memcpy(g1, subj + ov[2], ov[3] - ov[2]); g1[ov[3] - ov[2]] = 0;
    memcpy(g2, subj + ov[4], ov[5] - ov[4]); g2[ov[5] - ov[4]] = 0;
    if (strcmp(g1, "user") || strcmp(g2, "example")) {
        printf("pcre2: captures were %s / %s\n", g1, g2);
        return 1;
    }
    rc = pcre2_match(re, (PCRE2_SPTR)"not-an-address",
                     PCRE2_ZERO_TERMINATED, 0, 0, md, NULL);
    if (rc != PCRE2_ERROR_NOMATCH) {
        printf("pcre2: negative match rc=%d\n", rc);
        return 1;
    }
    /* Named capture groups, fetched by name. */
    {
        pcre2_code *re2 = pcre2_compile(
            (PCRE2_SPTR)"^(?<user>\\w+)@(?<host>\\w+)\\.com$",
            PCRE2_ZERO_TERMINATED, 0, &err, &erroff, NULL);
        if (!re2) { printf("pcre2: named compile failed\n"); return 1; }
        pcre2_match_data *md2 = pcre2_match_data_create_from_pattern(re2, NULL);
        rc = pcre2_match(re2, (PCRE2_SPTR)subj, PCRE2_ZERO_TERMINATED,
                         0, 0, md2, NULL);
        if (rc != 3) { printf("pcre2: named match rc=%d\n", rc); return 1; }
        PCRE2_UCHAR *np = NULL;
        PCRE2_SIZE nlen = 0;
        if (pcre2_substring_get_byname(md2, (PCRE2_SPTR)"host", &np, &nlen) ||
            nlen != 7 || memcmp(np, "example", 7)) {
            printf("pcre2: named group host mismatch\n");
            return 1;
        }
        printf("pcre2: match, captures, named groups and non-match ok\n");
    }
    return 0;
}

static void hex32(const unsigned char *in, char *out) {
    for (int i = 0; i < 32; i++) sprintf(out + 2 * i, "%02x", in[i]);
}

static int check_mbedtls(void) {
    unsigned char out[64];
    char hex[129];
    /* SHA-256("abc"), FIPS 180-4 example. */
    mbedtls_sha256((const unsigned char *)"abc", 3, out, 0);
    hex32(out, hex);
    if (strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a"
                    "9cb410ff61f20015ad")) {
        printf("mbedtls: sha256 mismatch: %s\n", hex);
        return 1;
    }
    /* SHA-384("abc"): the native configuration keeps the
       SHA-224/384 family enabled. */
    mbedtls_sha512((const unsigned char *)"abc", 3, out, 1);
    {
        static const unsigned char want384[48] = {
            0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b, 0xb5, 0xa0,
            0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07, 0x27, 0x2c, 0x32, 0xab,
            0x0e, 0xde, 0xd1, 0x63, 0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff,
            0x5b, 0xed, 0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23,
            0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7 };
        if (memcmp(out, want384, 48)) {
            printf("mbedtls: sha384 mismatch\n");
            return 1;
        }
    }
    /* AES-128 ECB, NIST SP 800-38A F.1.1 first block. */
    {
        unsigned char key[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae,
            0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
        unsigned char pt[16] = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40,
            0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a };
        static const unsigned char want[16] = { 0x3a, 0xd7, 0x7b, 0xb4,
            0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66,
            0xef, 0x97 };
        unsigned char ct[16];
        mbedtls_aes_context aes;
        mbedtls_aes_init(&aes);
        mbedtls_aes_setkey_enc(&aes, key, 128);
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, pt, ct);
        mbedtls_aes_free(&aes);
        if (memcmp(ct, want, 16)) {
            printf("mbedtls: aes-128 mismatch\n");
            return 1;
        }
    }
    /* AES-128 GCM, zero key/IV/plaintext test vector. */
    {
        unsigned char key[16] = { 0 }, iv[12] = { 0 }, pt[16] = { 0 };
        unsigned char ct[16], tag[16];
        static const unsigned char wantct[16] = { 0x03, 0x88, 0xda,
            0xce, 0x60, 0xb6, 0xa3, 0x92, 0xf3, 0x28, 0xc2, 0xb9, 0x71,
            0xb2, 0xfe, 0x78 };
        static const unsigned char wanttag[16] = { 0xab, 0x6e, 0x47,
            0xd4, 0x2c, 0xec, 0x13, 0xbd, 0xf5, 0x3a, 0x67, 0xb2, 0x12,
            0x57, 0xbd, 0xdf };
        mbedtls_gcm_context gcm;
        mbedtls_gcm_init(&gcm);
        int rc = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 128);
        if (!rc)
            rc = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT,
                                           16, iv, 12, NULL, 0, pt, ct,
                                           16, tag);
        mbedtls_gcm_free(&gcm);
        if (rc || memcmp(ct, wantct, 16) || memcmp(tag, wanttag, 16)) {
            printf("mbedtls: aes-gcm mismatch\n");
            return 1;
        }
    }
    printf("mbedtls: sha256, sha384, aes-ecb, aes-gcm vectors ok\n");
    return 0;
}

int main(void) {
    if (check_pcre2()) return 1;
    if (check_mbedtls()) return 1;
    printf("default libs battery 1 ok\n");
    return 0;
}
