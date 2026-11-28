/**
 * \file mbedtls_config.h
 *
 * \brief Configuration options for the mbedTLS headers shipped in
 *        the wasm32 sysroot.
 *
 * This is the configuration the sysroot's mbedTLS header subset is
 * used with. It enables exactly the modules the runtime supplies
 * an implementation for — the message digests (MD5, SHA-256,
 * SHA-512 and the generic MD layer with HMAC), AES (ECB and CBC
 * modes) and Base64 — and nothing else. In particular no TLS,
 * X.509, PK or random-generator module is enabled: the shipped
 * header set carries no declarations for them.
 *
 * The file follows upstream Mbed TLS 3.6 conventions: it is the
 * file mbedtls/build_info.h includes as the configuration, and
 * every consuming build is expected to provide its own.
 */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* The supplied modules. */
#define MBEDTLS_MD5_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA512_C
#define MBEDTLS_MD_C
#define MBEDTLS_AES_C
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_BASE64_C

/* SHA-256/SHA-512 in this configuration use the small, fully
 * portable C forms upstream selects when no accelerator is
 * configured. */

#endif /* MBEDTLS_CONFIG_H */
