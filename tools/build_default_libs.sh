#!/bin/bash
# Build the bundled default libraries (libcurl, PCRE2, mbedTLS)
# for one native target and stage them into its dist sysroot.
#
# Usage: build_default_libs.sh --tcc <target-tcc> --triple <triple>
#          --family <linux-gnu|android|darwin|windows> --dist <dist>
#
# <target-tcc> is a TCC binary that runs on this machine and
# compiles for the target (in build.sh, the per-target cross
# compiler from the target's work tree - the same compiler the
# win32 pthread merge uses). The libraries are compiled with the
# target's own staged headers, so the objects see exactly the
# macro environment a program compiled by the shipped compiler
# sees, plus the library-build defines (HAVE_CONFIG_H and the
# like) and the family configuration in libs-config/<family>/.
#
# Staged results, per family layout:
#   headers  curl/, pcre2.h, pcre2posix.h, mbedtls/, psa/
#            -> the dist's libc header directory (the win32
#               include directory for the windows family)
#   archives libcurl.a libpcre2.a libpcre2-posix.a
#            libmbedtls.a libmbedx509.a libmbedcrypto.a
#            -> the dist's library search directory (libc/ for
#               linux-gnu and android, lib/ for darwin, win32/lib
#               for windows), next to the crt objects and stubs
#   license files next to the archives.
#
# Work files land in build/work-libs/<triple>/; the per-library
# compile logs stay there for diagnosis.
set -e
cd "$(dirname "$0")/.."
ROOT=$PWD

TCC="" TRIPLE="" FAMILY="" DIST=""
while [ $# -gt 0 ]; do
    case $1 in
        --tcc) TCC=$2; shift 2 ;;
        --triple) TRIPLE=$2; shift 2 ;;
        --family) FAMILY=$2; shift 2 ;;
        --dist) DIST=$2; shift 2 ;;
        *) echo "unknown argument: $1" >&2; exit 1 ;;
    esac
done
[ -n "$TCC" ] && [ -n "$TRIPLE" ] && [ -n "$FAMILY" ] && [ -n "$DIST" ] || {
    echo "usage: build_default_libs.sh --tcc X --triple T --family F --dist D" >&2
    exit 1
}
CFG=$ROOT/libs-config/$FAMILY
WORK=$ROOT/build/work-libs/$TRIPLE
# CPU count, portable: macOS ships no nproc(1); sysctl is the
# native way there, and Linux keeps its own tool.
if [ "$(uname -s)" = Darwin ]; then
    JOBS=$(sysctl -n hw.ncpu)
else
    JOBS=$(nproc)
fi

# File size in bytes, portable: GNU stat -c%s does not exist on
# macOS (BSD stat takes -f%z); wc -c works on every runner.
fsize() { wc -c < "$1" | tr -d '[:space:]'; }

case $FAMILY in
    windows)
        INCDIR=$DIST/lib/tcc/win32/include
        LIBDIR=$DIST/lib/tcc/win32/lib
        SYSINC=("$INCDIR/winapi" "$INCDIR")
        ;;
    darwin)
        INCDIR=$DIST/lib/tcc/include-libc
        LIBDIR=$DIST/lib/tcc/lib
        SYSINC=("$DIST/lib/tcc/include-libc" "$DIST/lib/tcc/include")
        ;;
    linux-gnu)
        INCDIR=$DIST/lib/tcc/include-libc
        LIBDIR=$DIST/lib/tcc/libc
        SYSINC=("$DIST/lib/tcc/include-libc" "$DIST/lib/tcc/include")
        ;;
    android)
        INCDIR=$DIST/lib/tcc/include-libc
        LIBDIR=$DIST/lib/tcc/libc
        SYSINC=("$DIST/lib/tcc/include" "$DIST/lib/tcc/include-libc" \
                "$DIST/lib/tcc/include-libc/$TRIPLE")
        ;;
    *) echo "unknown family: $FAMILY" >&2; exit 1 ;;
esac
SYSINCFLAGS=()
for d in "${SYSINC[@]}"; do SYSINCFLAGS+=(-I"$d"); done

rm -rf "$WORK"
mkdir -p "$WORK" "$INCDIR" "$LIBDIR"

# compile_list <logfile> <objdir> <src...> -- <flags...>
# Compiles each source with $TCC, up to $JOBS in parallel; on
# any failure, names the file in the log and fails. Objects land
# in <objdir> by basename.
#
# The parallelism is a plain shell job pool, not xargs: BSD
# xargs (macOS) will not assemble a -I command line once the
# argument carrying the replacement string - here the inline
# sh -c script with the build tree's absolute paths inlined -
# passes its 255-byte limit, and fails with "command line
# cannot be assembled, too long" before compiling anything.
compile_list() {
    local log=$1 objdir=$2
    shift 2
    local srcs=()
    while [ "$1" != -- ]; do srcs+=("$1"); shift; done
    shift
    mkdir -p "$objdir"
    : > "$log"
    local src obj pid failed=0
    local running=()
    for src in "${srcs[@]}"; do
        obj="$objdir/$(basename "${src%.*}").o"
        (
            if ! "$TCC" -c "$src" -o "$obj" "$@" >>"$log" 2>&1; then
                echo "COMPILE FAILED: $src" >>"$log"
                exit 1
            fi
        ) &
        running+=($!)
        if [ "${#running[@]}" -ge "$JOBS" ]; then
            if ! wait "${running[0]}"; then failed=1; fi
            running=("${running[@]:1}")
        fi
    done
    for pid in "${running[@]}"; do
        if ! wait "$pid"; then failed=1; fi
    done
    if [ "$failed" -ne 0 ]; then
        tail -40 "$log"
        echo "library build failed (see $log)" >&2
        exit 1
    fi
}

archive() { # archive <out.a> <objdir>
    local out=$1 objdir=$2
    rm -f "$out"
    "$TCC" -ar rcs "$out" "$objdir"/*.o
}

# Prepare a scratch copy of the libcurl tree with the
# bundled-build patches applied; both the staged headers and
# the library build use it, so consumers see the same headers
# the library was built with.
prepare_curl_src() {
    CURLSRC=$WORK/curl-src
    rm -rf "$CURLSRC"
    mkdir -p "$CURLSRC"
    cp -r "$ROOT/vendor-curl/lib" "$ROOT/vendor-curl/include" "$CURLSRC/"
    local p
    for p in "$ROOT"/libs-config/patches/curl-*.patch; do
        [ -f "$p" ] || continue
        # No `patch` binary: it is not on PATH on the Windows
        # runner (MSYS2 ships no patch package). The Python
        # applier needs only python3, which this script already
        # requires, and applies byte-identically to `patch`.
        (cd "$CURLSRC" && python3 "$ROOT/tools/apply_patch.py" "$p" .) || {
            echo "failed to apply $p" >&2; exit 1; }
    done
}

# ---------------- header staging ----------------
stage_headers() {
    mkdir -p "$INCDIR/curl" "$INCDIR/mbedtls" "$INCDIR/psa"
    cp "$CURLSRC"/include/curl/*.h "$INCDIR/curl/"
    if [ "$FAMILY" = windows ]; then
        # The bundled libcurl is static-only; declare its API
        # without the DLL import decoration (see gen script for
        # the PCRE2 twin of this edit).
        python3 - "$INCDIR/curl/curl.h" <<'PYEOF'
import sys
p = sys.argv[1]
t = open(p).read()
anchor = "#include \"curlver.h\""
add = ("/* The bundled libcurl is static-only, so declare its API\n"
       "   without the DLL import decoration. */\n"
       "#define CURL_STATICLIB 1\n")
assert anchor in t
t = t.replace(anchor, add + anchor, 1)
open(p, "w").write(t)
PYEOF
    fi
    cp "$CFG/pcre2/pcre2.h" "$INCDIR/pcre2.h"
    cp "$ROOT/vendor-pcre2/src/pcre2posix.h" "$INCDIR/pcre2posix.h"
    cp "$ROOT"/vendor-mbedtls/include/mbedtls/*.h "$INCDIR/mbedtls/"
    cp "$ROOT"/vendor-mbedtls/include/psa/*.h "$INCDIR/psa/"
    cp "$CFG/mbedtls/mbedtls_config.h" "$INCDIR/mbedtls/mbedtls_config.h"
    cp "$ROOT/vendor-curl/COPYING" "$LIBDIR/LICENSE-curl"
    cp "$ROOT/vendor-pcre2/LICENCE.md" "$LIBDIR/LICENSE-pcre2"
    cp "$ROOT/vendor-mbedtls/LICENSE" "$LIBDIR/LICENSE-mbedtls"
    if [ "$FAMILY" = windows ]; then
        # The bundled win32 headers predate the CNG entropy API
        # mbedTLS uses on Windows; stage the minimal compat
        # headers and the bcrypt import definition (see
        # libs-config/windows/compat/).
        cp "$CFG/compat/bcrypt.h" "$CFG/compat/intsafe.h" "$INCDIR/"
        cp "$CFG/compat/bcrypt.def" "$LIBDIR/"
    fi
}

# ---------------- PCRE2 ----------------
build_pcre2() {
    local V=$ROOT/vendor-pcre2/src
    local objdir=$WORK/pcre2/obj
    local srcs=()
    local f
    for f in pcre2_auto_possess pcre2_chkdint pcre2_compile \
             pcre2_compile_class pcre2_config pcre2_context \
             pcre2_convert pcre2_dfa_match pcre2_error pcre2_extuni \
             pcre2_find_bracket pcre2_jit_compile pcre2_maketables \
             pcre2_match pcre2_match_data pcre2_newline \
             pcre2_ord2utf pcre2_pattern_info pcre2_script_run \
             pcre2_serialize pcre2_string_utils pcre2_study \
             pcre2_substitute pcre2_substring pcre2_tables \
             pcre2_ucd pcre2_valid_utf pcre2_xclass; do
        srcs+=("$V/$f.c")
    done
    srcs+=("$ROOT/libs-config/pcre2_chartables.c")
    compile_list "$WORK/pcre2.log" "$objdir" "${srcs[@]}" -- \
        -DHAVE_CONFIG_H -DPCRE2_CODE_UNIT_WIDTH=8 \
        -I"$CFG/pcre2" -I"$V" "${SYSINCFLAGS[@]}"
    archive "$LIBDIR/libpcre2.a" "$objdir"

    local pobj=$WORK/pcre2/posix
    mkdir -p "$pobj"
    if ! "$TCC" -c "$V/pcre2posix.c" -o "$pobj/pcre2posix.o" \
        -DHAVE_CONFIG_H -DPCRE2_CODE_UNIT_WIDTH=8 \
        -I"$CFG/pcre2" -I"$V" "${SYSINCFLAGS[@]}" \
        >>"$WORK/pcre2.log" 2>&1; then
        tail -20 "$WORK/pcre2.log"
        echo "pcre2posix build failed" >&2
        exit 1
    fi
    archive "$LIBDIR/libpcre2-posix.a" "$pobj"
    echo "pcre2: $(fsize "$LIBDIR/libpcre2.a") + $(fsize "$LIBDIR/libpcre2-posix.a") bytes"
}

# ---------------- mbedTLS ----------------
# File groups follow upstream library/CMakeLists.txt (src_crypto,
# src_x509, src_tls). The family configuration shadows the
# upstream mbedtls_config.h through the include path.
build_mbedtls() {
    # Compile from a scratch copy so the bundled-build patches in
    # libs-config/patches/ can be applied without dirtying the
    # vendored tree.
    local V=$WORK/mbedtls-src
    rm -rf "$V"
    mkdir -p "$V"
    cp -r "$ROOT/vendor-mbedtls/library" "$ROOT/vendor-mbedtls/include" "$V/"
    local p
    for p in "$ROOT"/libs-config/patches/mbedtls-*.patch; do
        [ -f "$p" ] || continue
        # Same `patch`-free rule as prepare_curl_src above.
        (cd "$V" && python3 "$ROOT/tools/apply_patch.py" "$p" .) || {
            echo "failed to apply $p" >&2; exit 1; }
    done
    local crypto="aes aesni aesce aria asn1parse asn1write base64
        bignum bignum_core bignum_mod bignum_mod_raw block_cipher
        camellia ccm chacha20 chachapoly cipher cipher_wrap
        constant_time cmac ctr_drbg des dhm ecdh ecdsa ecjpake ecp
        ecp_curves ecp_curves_new entropy entropy_poll error gcm
        hkdf hmac_drbg lmots lms md md5 memory_buffer_alloc nist_kw
        oid padlock pem pk pk_ecc pk_wrap pkcs12 pkcs5 pkparse
        pkwrite platform platform_util poly1305 psa_crypto psa_crypto_aead
        psa_crypto_cipher psa_crypto_client
        psa_crypto_driver_wrappers_no_static psa_crypto_ecp
        psa_crypto_ffdh psa_crypto_hash psa_crypto_mac
        psa_crypto_pake psa_crypto_rsa psa_crypto_se
        psa_crypto_slot_management psa_crypto_storage psa_its_file
        psa_util ripemd160 rsa rsa_alt_helpers sha1 sha256 sha512
        sha3 threading timing version version_features"
    local x509="pkcs7 x509 x509_create x509_crl x509_crt x509_csr
        x509write x509write_crt x509write_csr"
    local tls="debug mps_reader mps_trace net_sockets ssl_cache
        ssl_ciphersuites ssl_client ssl_cookie
        ssl_debug_helpers_generated ssl_msg ssl_ticket ssl_tls
        ssl_tls12_client ssl_tls12_server ssl_tls13_keys
        ssl_tls13_server ssl_tls13_client ssl_tls13_generic"
    local group name srcs f
    for group in crypto x509 tls; do
        srcs=()
        for f in ${!group}; do srcs+=("$V/library/$f.c"); done
        compile_list "$WORK/mbedtls-$group.log" "$WORK/mbedtls/$group" \
            "${srcs[@]}" -- \
            -I"$CFG" -I"$V/include" -I"$V/library" "${SYSINCFLAGS[@]}"
    done
    archive "$LIBDIR/libmbedcrypto.a" "$WORK/mbedtls/crypto"
    archive "$LIBDIR/libmbedx509.a" "$WORK/mbedtls/x509"
    archive "$LIBDIR/libmbedtls.a" "$WORK/mbedtls/tls"
    echo "mbedtls: $(fsize "$LIBDIR/libmbedtls.a") + $(fsize "$LIBDIR/libmbedx509.a") + $(fsize "$LIBDIR/libmbedcrypto.a") bytes"
}

# ---------------- libcurl ----------------
# The file list is upstream's CSOURCES set (lib/Makefile.inc):
# every protocol and backend file is compiled, and the disabled
# ones are emptied by the CURL_DISABLE_* / USE_* configuration
# (the HTTP/3 and SSH backends' third-party includes sit behind
# their feature guards, so their files compile empty too).
build_curl() {
    local V=$CURLSRC
    local files
    files=$(python3 - "$V/lib/Makefile.inc" <<'PYEOF'
import sys
want = ('LIB_CFILES', 'LIB_VAUTH_CFILES', 'LIB_VTLS_CFILES',
        'LIB_VQUIC_CFILES', 'LIB_VSSH_CFILES')
out, seen = [], set()
lines = open(sys.argv[1]).read().split('\n')
i = 0
while i < len(lines):
    line = lines[i]
    if any(line.startswith(v + ' =') or line.startswith(v + '=') for v in want):
        block = line
        while block.rstrip().endswith('\\') and i + 1 < len(lines):
            i += 1
            block += '\n' + lines[i]
        for tok in block.replace('\\', ' ').split():
            if tok.endswith('.c') and tok not in seen:
                seen.add(tok)
                out.append(tok)
    i += 1
print('\n'.join(out))
PYEOF
)
    local srcs=() f
    for f in $files; do srcs+=("$V/lib/$f"); done
    # curl's configure adds -D_GNU_SOURCE to CPPFLAGS on Linux
    # (configure.ac); the glibc and bionic families take the same
    # define so the target headers expose the GNU surface the
    # configuration claims (memrchr and friends).
    local gnudef=()
    case $FAMILY in linux-gnu|android) gnudef=(-D_GNU_SOURCE) ;; esac
    compile_list "$WORK/curl.log" "$WORK/curl/obj" "${srcs[@]}" -- \
        -DBUILDING_LIBCURL -DCURL_STATICLIB -DHAVE_CONFIG_H \
        "${gnudef[@]}" \
        -I"$CFG" -I"$V/lib" -I"$V/include" "${SYSINCFLAGS[@]}"
    archive "$LIBDIR/libcurl.a" "$WORK/curl/obj"
    echo "curl: $(fsize "$LIBDIR/libcurl.a") bytes"
}

prepare_curl_src
stage_headers
build_pcre2
build_mbedtls
build_curl
echo "default libs staged for $TRIPLE ($FAMILY) in $DIST"
