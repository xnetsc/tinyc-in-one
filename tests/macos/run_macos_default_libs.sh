#!/bin/bash
# Default libraries battery on macOS.
#
# Builds the battery programs (tests/default_libs.c,
# tests/default_libs_net.c, tests/default_libs_tlsserve.c,
# tests/default_libs_httpsget.c) with the bundle dist compiler
# named by TINYC_MAC_DIST (mac-test-bundle/dist/<this runner's
# triple>) against the staged default libraries (libcurl,
# PCRE2, mbedTLS) and runs them natively: PCRE2 matching and
# mbedTLS known-answer vectors, the local HTTP/HTTPS/WebSocket
# round trips through libcurl, and two interop legs - the
# system curl against the guest mbedTLS server, and the guest
# libcurl against a python3/OpenSSL TLS server.
#
# Output is PASS:/FAIL: lines; exit status is the FAIL count.
set -u
cd "$(dirname "$0")/../.."
ROOT=$PWD
DIST=${TINYC_MAC_DIST:?TINYC_MAC_DIST must name the bundle dist}
TC=$DIST/bin/tinyc
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FAIL=0
pass() { echo "PASS: $1"; }
fail() { echo "FAIL: $1"; FAIL=$((FAIL+1)); }

"$TC" "$ROOT/tests/default_libs.c" -lpcre2 -lmbedcrypto \
    -o "$TMP/battery1" || { fail "battery1 build"; exit 1; }
# Independent check with the system nm: whether the ws handler
# symbols are in the staged archive at all. This runs BEFORE
# the battery2 link on purpose: it only needs libcurl.a, so it
# still reports the archive side even when the link itself
# fails below (previously the whole diagnostics block sat after
# the builds and never ran once the link failed, so a link
# failure also erased the archive evidence). A handler present
# in the archive but missing from the binary points at the
# link; absent from the archive points at the library build.
# Diagnostic only (|| true): never fails the step itself.
if command -v nm >/dev/null 2>&1; then
    echo "--- ws symbols (nm) ---"
    # One archive per triple dist (lib/tcc/<family>/libcurl.a).
    CURLA_LIST=$(find "$DIST" -name libcurl.a -path '*lib/tcc/*' 2>/dev/null)
    if [ -z "$CURLA_LIST" ]; then
        echo "nm: no libcurl.a found under $DIST"
    fi
    for CURLA in $CURLA_LIST; do
        echo "libcurl.a: $CURLA"
        nm "$CURLA" 2>/dev/null | grep -E 'Curl_handler_ws|_protocols' \
            || echo "nm: no Curl_handler_ws/_protocols in libcurl.a"
    done
fi

# libcurl is linked whole-archive, and the link must resolve
# -lcurl to the staged libcurl.a: the linker's form-major
# search used to let the macOS SDK's libcurl.tbd shadow the
# staged archive, so earlier rounds silently linked the system
# libcurl (whose "ws not supported" was misread as the archive
# pull dropping ws.o -- the staged archive was never in the
# binary at all). The search is directory-major now, so the
# dist's lib directory wins over the SDK path; -vv below lists
# every archive member the link loads, which is the decisive
# check that the staged archive is the one being linked.
# Echo the link command: whether -Wl,-whole-archive actually
# reaches the link is itself evidence. The -no-whole-archive
# after -lcurl keeps mbedTLS on the normal pull.
echo "link battery2: $TC $ROOT/tests/default_libs_net.c -Wl,-whole-archive -lcurl -Wl,-no-whole-archive -lmbedtls -lmbedx509 -lmbedcrypto -o $TMP/battery2"
"$TC" -vv "$ROOT/tests/default_libs_net.c" \
    -Wl,-whole-archive -lcurl -Wl,-no-whole-archive \
    -lmbedtls -lmbedx509 -lmbedcrypto \
    -o "$TMP/battery2" || fail "battery2 build"
"$TC" "$ROOT/tests/default_libs_tlsserve.c" \
    -lmbedtls -lmbedx509 -lmbedcrypto \
    -o "$TMP/tlsserve" || fail "tlsserve build"
"$TC" "$ROOT/tests/default_libs_httpsget.c" \
    -lcurl -lmbedtls -lmbedx509 -lmbedcrypto \
    -o "$TMP/httpsget" || fail "httpsget build"
# A failed build above is recorded, not fatal: the remaining
# diagnostics and interop legs still run (they need tlsserve /
# httpsget, not battery2), and the final exit status is the
# FAIL count. Exiting at the first failed build used to erase
# all of the downstream evidence in the same round.
[ "$FAIL" -eq 0 ] && pass "battery programs build"

# The ws handler lives in a perfect-hash table emitted as
# chained fixups; dump it so a "ws not supported" failure
# shows whether the table itself lost the handler. Needs the
# linked binary; skip when the link failed.
if [ -x "$TMP/battery2" ] && command -v python3 >/dev/null 2>&1; then
    python3 "$ROOT/tests/macos/check_protocols_table.py" "$TMP/battery2" || true
fi

# Same for the linked binary's own symbols (the archive half of
# this check already ran above, before the link).
if [ -x "$TMP/battery2" ] && command -v nm >/dev/null 2>&1; then
    nm "$TMP/battery2" 2>/dev/null | grep -E 'Curl_handler_ws|_protocols' \
        || echo "nm: no Curl_handler_ws/_protocols in battery2"
fi

"$TMP/battery1" && pass "battery1 run" || fail "battery1 run"
if [ -x "$TMP/battery2" ]; then
    "$TMP/battery2" && pass "battery2 run" || fail "battery2 run"
else
    echo "battery2 run skipped: no binary (link failed)"
fi

CERT=$TMP/cert
mkdir -p "$CERT"
if command -v openssl >/dev/null 2>&1; then
    openssl req -x509 -newkey rsa:2048 -keyout "$CERT/ca.key" \
        -out "$CERT/ca.crt" -days 3650 -nodes \
        -subj "/CN=tinyc-test-ca" >/dev/null 2>&1
    openssl req -newkey rsa:2048 -keyout "$CERT/srv.key" \
        -out "$CERT/srv.csr" -nodes -subj "/CN=localhost" >/dev/null 2>&1
    printf "subjectAltName=IP:127.0.0.1,DNS:localhost\n" > "$CERT/srv.ext"
    openssl x509 -req -in "$CERT/srv.csr" -CA "$CERT/ca.crt" \
        -CAkey "$CERT/ca.key" -CAcreateserial -out "$CERT/srv.crt" \
        -days 3650 -extfile "$CERT/srv.ext" >/dev/null 2>&1
fi
if [ -f "$CERT/ca.crt" ]; then
    ("$TMP/tlsserve" 18241 "$CERT/srv.crt" "$CERT/srv.key" \
        > "$TMP/tlsserve.log" 2>&1 &)
    sleep 1
    if curl -s --cacert "$CERT/ca.crt" https://127.0.0.1:18241/ \
            | grep -q tinyc-tls-ok; then
        pass "interop system-curl -> guest mbedTLS"
    else
        fail "interop system-curl -> guest mbedTLS"
        echo "--- tlsserve.log ---"
        cat "$TMP/tlsserve.log" 2>/dev/null || echo "(no tlsserve.log)"
        echo "--- curl -v ---"
        curl -v --cacert "$CERT/ca.crt" https://127.0.0.1:18241/ 2>&1 | head -40 || true
    fi
    cat > "$TMP/pytls.py" <<'EOF'
import socket, ssl, sys
ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
ctx.load_cert_chain(sys.argv[2], sys.argv[3])
s = socket.socket(); s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('127.0.0.1', int(sys.argv[1]))); s.listen(1)
conn, _ = s.accept()
tls = ctx.wrap_socket(conn, server_side=True)
tls.recv(4096)
body = b'tinyc-tls-ok\n'
tls.sendall(b'HTTP/1.1 200 OK\r\nContent-Length: ' +
            str(len(body)).encode() + b'\r\n\r\n' + body)
tls.close()
EOF
    if command -v python3 >/dev/null 2>&1; then
        (python3 "$TMP/pytls.py" 18242 "$CERT/srv.crt" "$CERT/srv.key" \
            > "$TMP/pytls.log" 2>&1 &)
        sleep 1
        if "$TMP/httpsget" https://127.0.0.1:18242/ "$CERT/ca.crt" \
                | grep -q tinyc-tls-ok; then
            pass "interop guest libcurl -> system OpenSSL"
        else
            fail "interop guest libcurl -> system OpenSSL"
        fi
    fi
fi
exit $FAIL
