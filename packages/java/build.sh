#!/usr/bin/env bash
# 构建 TinyC 的 Java 服务器包：dist/tinyc.jar（一 jar 全量）
# Jar 内是统一布局：natives/<host>/toolchain/<target>/tinyc +
# sysroot/<target>/ + packaging-manifest.json，运行时按宿主 triple
# 解析组合、解压到缓存。需要 JDK 21+（Panama FFM；21 为预览，22+ 正式）。
set -euo pipefail
cd "$(dirname "$0")"

ROOT=$(cd ../.. && pwd)
BUILD=build
DIST=dist
JARROOT=$BUILD/jarroot

# ---- JDK 定位：JAVA_HOME > ~/workspace/envs/jdk-25 > 系统 JVM > PATH ----
if [ -z "${JAVA_HOME:-}" ]; then
  for cand in "$HOME/workspace/envs/jdk-25" /usr/lib/jvm/java-25-openjdk-amd64 /usr/lib/jvm/java-21-openjdk-amd64; do
    if [ -x "$cand/bin/javac" ]; then JAVA_HOME=$cand; break; fi
  done
fi
if [ -n "${JAVA_HOME:-}" ]; then
  JAVAC="$JAVA_HOME/bin/javac"; JAR="$JAVA_HOME/bin/jar"
else
  JAVAC=javac; JAR=jar
fi

# ---- 构建宿主 triple（决定 staging 哪些二进制进 jar；仅 64 位） ----
OS=$(uname -s)
ARCH=$(uname -m)
case "$OS/$ARCH" in
  Linux/x86_64)  HOST_TRIPLE=x86_64-linux-gnu ;;
  Linux/aarch64) HOST_TRIPLE=aarch64-linux-gnu ;;
  Linux/riscv64) HOST_TRIPLE=riscv64-linux-gnu ;;
  Darwin/arm64)  HOST_TRIPLE=arm64-apple-darwin ;;
  Darwin/x86_64) HOST_TRIPLE=x86_64-apple-darwin ;;
  */i386|*/i486|*/i586|*/i686)
    echo "unsupported build arch: $ARCH (32-bit hosts are not supported)" >&2; exit 1 ;;
  *) echo "unsupported build host: $OS/$ARCH" >&2; exit 1 ;;
esac
HOST_TRIPLE="${TINYC_HOST_TRIPLE:-${TINYC_TRIPLE:-$HOST_TRIPLE}}"

VERSION="$(tr -d '[:space:]' < "$ROOT/vendor-tinycc/VERSION")-java1"

rm -rf "$BUILD"
mkdir -p "$JARROOT" "$DIST"

# ---- 统一布局 staging：natives/ + sysroot/ + packaging-manifest.json ----
python3 "$ROOT/packaging/stage_layout.py" --root "$JARROOT" --host "$HOST_TRIPLE"

NATIVE_NAME=tinyc
STAGED_BIN="$JARROOT/natives/$HOST_TRIPLE/toolchain/$HOST_TRIPLE/$NATIVE_NAME"
[ -f "$STAGED_BIN" ] || { echo "native (host == target) build not staged: $STAGED_BIN" >&2; exit 1; }
echo "$NATIVE_NAME" > "$BUILD/native-name.txt"

# 导出表核对：tcc_new 必须在；main/tinyc_main 缺失只警告（runCli 不可用）
nm -D "$STAGED_BIN" | grep -q ' T tcc_new' \
  || { echo "ERROR: tcc_new not exported by $STAGED_BIN" >&2; exit 1; }
if nm -D "$STAGED_BIN" | grep -qE ' T (tinyc_main|main)$'; then
  echo "native: main entry exported"
else
  echo "WARNING: no main/tinyc_main exported, runCli will be unavailable" >&2
fi

cat > "$JARROOT/tinyc-native.properties" <<EOF
version=$VERSION
EOF

# ---- 编译 Java 并打包 ----
mkdir -p "$BUILD/classes"
find src/main/java -name '*.java' | sort > "$BUILD/sources.txt"
"$JAVAC" -d "$BUILD/classes" @"$BUILD/sources.txt"
cp -r "$JARROOT/." "$BUILD/classes/"

cat > "$BUILD/manifest.mf" <<EOF
Implementation-Title: tinyc-java
Implementation-Version: $VERSION
EOF
"$JAR" --create --file "$DIST/tinyc.jar" --manifest "$BUILD/manifest.mf" -C "$BUILD/classes" .

# ---- 测试类与开发模式 staging（原生文件 + 运行时同目录） ----
mkdir -p "$BUILD/test-classes" "$BUILD/test-runtime"
"$JAVAC" -cp "$DIST/tinyc.jar" -d "$BUILD/test-classes" $(find src/test/java -name '*.java' | sort)
cp "$STAGED_BIN" "$BUILD/test-runtime/$NATIVE_NAME"
# 整个 lib/tcc 目录平铺进 test-runtime：除 libtcc1.a 与 include 外，
# -run 需要的 runmain.o 等辅助目标文件也在其中
cp -r "$JARROOT/sysroot/$HOST_TRIPLE/lib/tcc/." "$BUILD/test-runtime/"

echo "built: $PWD/$DIST/tinyc.jar (version $VERSION, host $HOST_TRIPLE)"
