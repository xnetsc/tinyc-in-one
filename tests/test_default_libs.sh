#!/bin/bash
# Default libraries battery driver.
#
# For every triple present in build/dist, build the battery
# programs (tests/default_libs.c, tests/default_libs_net.c,
# tests/default_libs_tlsserve.c, tests/default_libs_httpsget.c)
# with that triple's packaged compiler against the staged
# default libraries. The linux-gnu triples also run them:
# natively for the host triple, under qemu-user for the other
# two (run legs SKIP when the qemu binary is absent, the
# test_matrix.sh rule; the build legs still run). Two interop
# legs run when the system tools exist: the system curl
# against the guest mbedTLS server, and the guest libcurl
# against a python3/OpenSSL TLS server.
#
# The android, darwin and windows triples get compile + link
# closure here; their execution belongs to the platform CI
# legs.
set -u
cd "$(dirname "$0")/.."
ROOT=$PWD
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FAIL=0
note() { echo "$1"; }
fail() { note "FAIL: $1"; FAIL=1; }

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

# battery <triple> <run|build> [runner...]
battery() {
    local triple=$1 mode=$2; shift 2
    local dist=$ROOT/build/dist/$triple
    local tc=$dist/bin/tinyc
    [ -x "$tc" ] || { note "SKIP $triple (no dist)"; return; }
    local out=$TMP/$triple; mkdir -p "$out"
    # The windows legs additionally link the winsock import
    # library and the staged bcrypt import definition (the
    # entropy source's BCryptGenRandom).
    local winlibs=""
    [ "$triple" = x86_64-w64-mingw32 ] && winlibs="-lws2_32 -lbcrypt"
    "$tc" "$ROOT/tests/default_libs.c" -lpcre2 -lmbedcrypto \
        -o "$out/battery1" || { fail "$triple: battery1 build"; return; }
    "$tc" "$ROOT/tests/default_libs_net.c" \
        -lcurl -lmbedtls -lmbedx509 -lmbedcrypto $winlibs \
        -o "$out/battery2" || { fail "$triple: battery2 build"; return; }
    "$tc" "$ROOT/tests/default_libs_tlsserve.c" \
        -lmbedtls -lmbedx509 -lmbedcrypto $winlibs \
        -o "$out/tlsserve" || { fail "$triple: tlsserve build"; return; }
    "$tc" "$ROOT/tests/default_libs_httpsget.c" \
        -lcurl -lmbedtls -lmbedx509 -lmbedcrypto $winlibs \
        -o "$out/httpsget" || { fail "$triple: httpsget build"; return; }
    if [ "$mode" = build ]; then
        note "$triple: built (compile + link closure)"
        return
    fi
    "$@" "$out/battery1" || { fail "$triple: battery1 run"; return; }
    "$@" "$out/battery2" || { fail "$triple: battery2 run"; return; }
    note "$triple: battery ok"
    if [ -f "$CERT/ca.crt" ]; then
        if command -v curl >/dev/null 2>&1; then
            ("$@" "$out/tlsserve" 18241 "$CERT/srv.crt" "$CERT/srv.key" \
                > "$out/tlsserve.log" 2>&1 &)
            sleep 1
            if curl -s --cacert "$CERT/ca.crt" https://127.0.0.1:18241/ \
                    | grep -q tinyc-tls-ok; then
                note "$triple: interop system-curl -> guest mbedTLS ok"
            else
                fail "$triple: interop system-curl -> guest mbedTLS"
            fi
        fi
        if command -v python3 >/dev/null 2>&1; then
            (python3 "$TMP/pytls.py" 18242 "$CERT/srv.crt" "$CERT/srv.key" \
                > "$out/pytls.log" 2>&1 &)
            sleep 1
            if "$@" "$out/httpsget" https://127.0.0.1:18242/ "$CERT/ca.crt" \
                    | grep -q tinyc-tls-ok; then
                note "$triple: interop guest libcurl -> system OpenSSL ok"
            else
                fail "$triple: interop guest libcurl -> system OpenSSL"
            fi
        fi
    fi
}

# Run legs for the linux-gnu targets: the host's own triple runs
# natively, the others run under qemu-user when it is installed
# (both CI linux jobs install it), else they stay build legs.
HOST_ARCH=$(uname -m)
for T in x86_64-linux-gnu aarch64-linux-gnu riscv64-linux-gnu; do
    A=${T%%-*}
    if [ "$A" = "$HOST_ARCH" ]; then
        battery "$T" run
    elif command -v "qemu-$A-static" >/dev/null 2>&1; then
        battery "$T" run "qemu-$A-static" -L "/usr/$T"
    elif [ -d "$ROOT/build/dist/$T" ]; then
        note "SKIP $T run (no qemu-$A-static)"
        battery "$T" build
    fi
done

for t in aarch64-linux-android x86_64-linux-android \
         arm64-apple-darwin x86_64-apple-darwin x86_64-w64-mingw32; do
    battery "$t" build
done

[ $FAIL = 0 ] && note "default libs battery: all legs ok" \
              || note "default libs battery: FAILURES"
exit $FAIL
