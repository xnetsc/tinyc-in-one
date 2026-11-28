# Configuration for the bundled default libraries (native targets)

The native packages bundle three upstream C libraries as static
archives in each target's sysroot, built by the target's own TCC
from the vendored sources (`vendor-curl/`, `vendor-pcre2/`,
`vendor-mbedtls/`):

- libcurl 8.11.1 — HTTP/HTTPS/WebSocket only, TLS via mbedTLS
- PCRE2 10.45 — 8-bit code units, no JIT
- mbedTLS 3.6.2 — TLS client and server, X.509, full crypto

This directory carries the build configuration for them, one
subdirectory per platform family (`linux-gnu/`, `android/`,
`darwin/`, `windows/`). The configurations are hand-written per
family; they are not the output of the libraries' configure
scripts, which probe the build host rather than the target.

Each family directory holds:

- `curl_config.h` — libcurl's configuration header (found by the
  library sources through the include path; not shipped).
- `pcre2/config.h` — PCRE2's configuration header (build-internal;
  not shipped).
- `pcre2/pcre2.h` — the public PCRE2 header, generated from
  `vendor-pcre2/src/pcre2.h.in` by `tools/gen_pcre2_headers.sh`
  (the only substitutions are the version fields; the Windows
  family's copy additionally predefines `PCRE2_STATIC`, because
  the bundled library is static-only and the header would
  otherwise declare the API `__declspec(dllimport)`).
- `mbedtls/mbedtls_config.h` — the mbedTLS configuration. It is
  staged into the sysroot in place of the upstream default, so
  programs are compiled against the same configuration the
  library was built with. Relative to the upstream default it
  drops the assembly/intrinsics paths TCC cannot build
  (`MBEDTLS_HAVE_ASM`, `MBEDTLS_AESNI_C`, `MBEDTLS_AESCE_C`,
  `MBEDTLS_PADLOCK_C`) in favour of the portable C code, and
  enables the threading support (`MBEDTLS_THREADING_C` with the
  pthread implementation) on all four families; on Windows the
  pthread implementation is the win32 sysroot's bundled
  winpthreads.

`pcre2_chartables.c` (shared by all families) is the release
tarball's pre-generated character tables
(`src/pcre2_chartables.c.dist`), copied unchanged: the tables
depend only on the Unicode and newline settings, which are the
same for every family here.

Source-level patches, when a library needs one to build with
the bundled compiler, live in `patches/`; the builder applies
them to scratch copies, so the vendored trees stay pristine.
Current patches:

- `mbedtls-entropy-poll-syscall.patch`: guard the optional
  `<sys/syscall.h>` getrandom probe in entropy_poll.c with
  `__has_include`; the bundled header sets do not ship that
  header, and the code falls back to /dev/urandom without it.
- `curl-setup-targetconditionals.patch`: guard the Apple SDK
  <TargetConditionals.h> include with `__has_include`; the
  bundled darwin header set does not ship it.
- `curl-system-h-tinyc-win32.patch`: give curl/system.h's
  __TINYC__ branch a _WIN32 case (winsock length type int,
  no <sys/socket.h> pull).

The builder is `tools/build_default_libs.sh`; `build.sh` calls it
for every native dist it produces.
