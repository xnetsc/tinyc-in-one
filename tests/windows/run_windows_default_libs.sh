#!/bin/bash
# Default libraries battery on Windows.
#
# Builds the battery programs (tests/default_libs.c,
# tests/default_libs_net.c, tests/default_libs_tlsserve.c,
# tests/default_libs_httpsget.c) with the Windows host dist
# compiler (build/dist-host/x86_64-w64-mingw32/bin/tinyc.exe)
# against the staged default libraries (libcurl, PCRE2,
# mbedTLS) and runs them natively: PCRE2 matching and mbedTLS
# known-answer vectors, the local HTTP/HTTPS/WebSocket round
# trips through libcurl, and two interop legs - the system
# curl against the guest mbedTLS server, and the guest
# libcurl against a python3/OpenSSL TLS server.
set -u
cd "$(dirname "$0")/../.."
ROOT=$PWD
TC=$ROOT/build/dist-host/x86_64-w64-mingw32/bin/tinyc.exe
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FAIL=0
note() { echo "$1"; }
fail() { note "FAIL: $1"; FAIL=1; }
WINLIBS="-lws2_32 -lbcrypt"

"$TC" "$ROOT/tests/default_libs.c" -lpcre2 -lmbedcrypto \
    -o "$TMP/battery1.exe" || { fail "battery1 build"; exit 1; }
"$TC" "$ROOT/tests/default_libs_net.c" \
    -lcurl -lmbedtls -lmbedx509 -lmbedcrypto $WINLIBS \
    -o "$TMP/battery2.exe" || { fail "battery2 build"; exit 1; }
"$TC" "$ROOT/tests/default_libs_tlsserve.c" \
    -lmbedtls -lmbedx509 -lmbedcrypto $WINLIBS \
    -o "$TMP/tlsserve.exe" || { fail "tlsserve build"; exit 1; }
"$TC" "$ROOT/tests/default_libs_httpsget.c" \
    -lcurl -lmbedtls -lmbedx509 -lmbedcrypto $WINLIBS \
    -o "$TMP/httpsget.exe" || { fail "httpsget build"; exit 1; }
note "windows: battery programs build"

(cd "$TMP" && ./battery1.exe) && note "windows: battery1 run" \
    || fail "windows: battery1 run"
(cd "$TMP" && ./battery2.exe) && note "windows: battery2 run" \
    || fail "windows: battery2 run"

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
    ("$TMP/tlsserve.exe" 18241 "$CERT/srv.crt" "$CERT/srv.key" \
        > "$TMP/tlsserve.log" 2>&1 &)
    sleep 1
    if curl -s --cacert "$CERT/ca.crt" https://127.0.0.1:18241/ \
            | grep -q tinyc-tls-ok; then
        note "windows: interop system-curl -> guest mbedTLS ok"
    else
        fail "windows: interop system-curl -> guest mbedTLS"
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
        if "$TMP/httpsget.exe" https://127.0.0.1:18242/ "$CERT/ca.crt" \
                | grep -q tinyc-tls-ok; then
            note "windows: interop guest libcurl -> system OpenSSL ok"
        else
            fail "windows: interop guest libcurl -> system OpenSSL"
        fi
    fi
fi
[ $FAIL = 0 ] && note "windows default libs battery: all legs ok" \
              || note "windows default libs battery: FAILURES"
exit $FAIL
