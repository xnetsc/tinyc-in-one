#!/usr/bin/env bash
# 运行绑定层测试，三遍：
#   A 开发模式：系统属性直接指定本地原生文件与运行时目录
#   B 分发模式：无属性，从 Jar resources 按组合解压到 ~/.cache/tinyc-java/<version>/
#   C 宿主模拟：TINYC_HOST_TRIPLE 覆盖探测——本机宿主正常解析并跑全套；
#     包内不存在的组合与 32 位宿主分别断言指名错误
set -euo pipefail
cd "$(dirname "$0")"

NAME=$(cat build/native-name.txt)
CP="dist/tinyc.jar:build/test-classes"
# JDK 定位与 build.sh 一致：JAVA_HOME > ~/workspace/envs/jdk-25 > PATH
if [ -n "${JAVA_HOME:-}" ]; then
  JAVA="$JAVA_HOME/bin/java --enable-native-access=ALL-UNNAMED"
elif [ -x "$HOME/workspace/envs/jdk-25/bin/java" ]; then
  JAVA="$HOME/workspace/envs/jdk-25/bin/java --enable-native-access=ALL-UNNAMED"
else
  JAVA="java --enable-native-access=ALL-UNNAMED"
fi

echo "=== mode A: tinyc.native.path (dev) ==="
$JAVA -Dtinyc.native.path="$PWD/build/test-runtime/$NAME" \
      -Dtinyc.lib.path="$PWD/build/test-runtime" \
      -cp "$CP" tinyc.TinyCTest

echo "=== mode B: jar extraction (distribution) ==="
$JAVA -cp "$CP" tinyc.TinyCTest

echo "=== mode C: host simulation via TINYC_HOST_TRIPLE ==="
echo "--- C1: x86_64-linux-gnu resolves and runs the full suite"
TINYC_HOST_TRIPLE=x86_64-linux-gnu $JAVA -cp "$CP" tinyc.TinyCTest >/dev/null
echo "C1 OK"

echo "--- C2: combination absent from the package names host and available combinations"
set +e
OUT=$(TINYC_HOST_TRIPLE=x86_64-w64-mingw32 $JAVA -cp "$CP" tinyc.HostProbe 2>&1)
RC=$?
set -e
[ $RC -ne 0 ] || { echo "C2 FAILED: probe unexpectedly succeeded"; echo "$OUT"; exit 1; }
grep -q "no native build for host x86_64-w64-mingw32" <<<"$OUT" \
  || { echo "C2 FAILED: host not named"; echo "$OUT"; exit 1; }
grep -q "x86_64-linux-gnu x x86_64-linux-gnu" <<<"$OUT" \
  || { echo "C2 FAILED: available combinations not listed"; echo "$OUT"; exit 1; }
echo "C2 OK"

echo "--- C3: 32-bit host is rejected"
set +e
OUT=$(TINYC_HOST_TRIPLE=i386-linux-gnu $JAVA -cp "$CP" tinyc.HostProbe 2>&1)
RC=$?
set -e
[ $RC -ne 0 ] || { echo "C3 FAILED: probe unexpectedly succeeded"; echo "$OUT"; exit 1; }
grep -q "32-bit hosts are not supported" <<<"$OUT" \
  || { echo "C3 FAILED: 32-bit rejection missing"; echo "$OUT"; exit 1; }
grep -q "i386-linux-gnu" <<<"$OUT" \
  || { echo "C3 FAILED: host triple not named"; echo "$OUT"; exit 1; }
echo "C3 OK"

echo "=== run-tests: OK ==="
