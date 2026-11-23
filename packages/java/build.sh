#!/usr/bin/env bash
# 构建 TinyC 的 Java 服务器包：dist/tinyc.jar（一 jar 全量）
# Jar 内是统一布局：natives/<host>/toolchain/<target>/tinyc +
# sysroot/<target>/ + packaging-manifest.json，运行时按宿主 triple
# 解析组合、解压到缓存。需要 JDK 22+（Panama FFM）。
set -euo pipefail
cd "$(dirname "$0")"

ROOT=$(cd ../.. && pwd)
BUILD=build
DIST=dist
JARROOT=$BUILD/jarroot

# ---- JDK 定位：按版本逐个核验候选，取第一个主版本号 >= 22 的 ----
# 候选顺序：JAVA_HOME、JAVA_HOME_25_X64、JAVA_HOME_25_ARM64、JAVA_HOME_25、
# ~/workspace/envs/jdk-25、/usr/lib/jvm 系统 JVM、PATH 中的 javac。
# 多 JDK 环境里 JAVA_HOME 可能指向低版本，只认它会误用旧版编译。
javac_major() {
  local out ver major
  out=$("$1" -version 2>&1) || return 1
  ver=$(printf '%s\n' "$out" | grep -oE '[0-9]+(\.[0-9]+)*' | head -n 1) || return 1
  [ -n "$ver" ] || return 1
  major=${ver%%.*}
  if [ "$major" = "1" ]; then major=$(printf '%s' "$ver" | cut -d. -f2); fi
  printf '%s' "$major"
}

resolve_jdk_bin() {
  local cand major javac
  for cand in "${JAVA_HOME:-}" "${JAVA_HOME_25_X64:-}" "${JAVA_HOME_25_ARM64:-}" \
              "${JAVA_HOME_25:-}" "$HOME/workspace/envs/jdk-25" \
              /usr/lib/jvm/java-25-openjdk-amd64 /usr/lib/jvm/java-21-openjdk-amd64; do
    [ -n "$cand" ] || continue
    [ -x "$cand/bin/javac" ] || continue
    major=$(javac_major "$cand/bin/javac") || continue
    if [ "$major" -ge 22 ]; then printf '%s' "$cand/bin"; return 0; fi
  done
  if javac=$(command -v javac 2>/dev/null); then
    major=$(javac_major "$javac") || major=0
    if [ "$major" -ge 22 ]; then dirname "$javac"; return 0; fi
  fi
  return 1
}

JDK_BIN=$(resolve_jdk_bin) || {
  echo "error: JDK 22 or newer is required (Panama FFM); no candidate JDK qualifies" >&2
  exit 1
}
JAVAC="$JDK_BIN/javac"; JAR="$JDK_BIN/jar"

# ---- 构建宿主 triple（决定 staging 哪些二进制进 jar；仅 64 位） ----
# Windows 宿主上构建走 Git Bash/MSYS2/Cygwin 的 shell，其 uname -s 是
# MINGW64_NT*/MSYS_NT*/CYGWIN_NT*，宿主本体是原生 Windows（MinGW PE）。
OS=$(uname -s)
ARCH=$(uname -m)
case "$OS/$ARCH" in
  Linux/x86_64)  HOST_TRIPLE=x86_64-linux-gnu ;;
  Linux/aarch64) HOST_TRIPLE=aarch64-linux-gnu ;;
  Linux/riscv64) HOST_TRIPLE=riscv64-linux-gnu ;;
  Darwin/arm64)  HOST_TRIPLE=arm64-apple-darwin ;;
  Darwin/x86_64) HOST_TRIPLE=x86_64-apple-darwin ;;
  MINGW64_NT*/x86_64|MSYS_NT*/x86_64|CYGWIN_NT*/x86_64)
    HOST_TRIPLE=x86_64-w64-mingw32 ;;
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

# staged 的二进制名与 stage_layout 一致（取 dist 清单 dual_file 的基名）：
# PE 宿主是 tinyc.exe，其余宿主是 tinyc。
NATIVE_NAME=tinyc
case "$HOST_TRIPLE" in
  *-w64-mingw32) NATIVE_NAME=tinyc.exe ;;
esac
STAGED_BIN="$JARROOT/natives/$HOST_TRIPLE/toolchain/$HOST_TRIPLE/$NATIVE_NAME"
[ -f "$STAGED_BIN" ] || { echo "native (host == target) build not staged: $STAGED_BIN" >&2; exit 1; }
echo "$NATIVE_NAME" > "$BUILD/native-name.txt"

# 导出表核对：tcc_new 必须在；main/tinyc_main 缺失只警告（runCli 不可用）。
# ELF 宿主上导出就在 staged 文件的动态符号表里（nm -D）。PE 宿主的
# 导出在导出表里，nm -D 读不到（报 no symbols），用 objdump -p 的
# [Ordinal/Name Pointer] 表核对——那正是运行时按名解析所见的集合。
# Darwin 的双形态文件是 Mach-O 可执行文件，没有动态符号表，导出在它
# 内嵌的 __TINYC,__dylib 段（构建时以 -sectcreate 嵌入的那份 dylib）
# 里：先把该段按其 Mach-O 段头解出，再对解出的 dylib 做同一条核对。
if [ "$OS" = "Darwin" ]; then
  CHECK_BIN="$BUILD/staged-libtinyc.dylib"
  python3 - "$STAGED_BIN" "$CHECK_BIN" <<'PYEOF'
import struct, sys
src, dst = sys.argv[1], sys.argv[2]
data = open(src, "rb").read()
if data[:4] == b"\xcf\xfa\xed\xfe":    # MH_MAGIC_64, little-endian
    fmt = "<"
elif data[:4] == b"\xfe\xed\xfa\xcf":  # MH_CIGAM_64, big-endian
    fmt = ">"
else:
    sys.exit(f"not a 64-bit Mach-O file: {src}")
ncmds = struct.unpack_from(fmt + "I", data, 16)[0]
off = 32  # sizeof(mach_header_64)
for _ in range(ncmds):
    cmd, cmdsize = struct.unpack_from(fmt + "II", data, off)
    if cmd == 0x19:  # LC_SEGMENT_64
        segname = data[off + 8:off + 24].split(b"\0")[0]
        nsects = struct.unpack_from(fmt + "I", data, off + 64)[0]
        soff = off + 72  # sizeof(segment_command_64)
        for _ in range(nsects):
            sectname = data[soff:soff + 16].split(b"\0")[0]
            size = struct.unpack_from(fmt + "Q", data, soff + 40)[0]
            offset = struct.unpack_from(fmt + "I", data, soff + 48)[0]
            if segname == b"__TINYC" and sectname == b"__dylib":
                open(dst, "wb").write(data[offset:offset + size])
                sys.exit(0)
            soff += 80  # sizeof(section_64)
    off += cmdsize
sys.exit(f"no __TINYC,__dylib section in {src}")
PYEOF
  nm -gU "$CHECK_BIN" | grep -qE ' [Tt] _?tcc_new$' \
    || { echo "ERROR: tcc_new not exported by the dylib embedded in $STAGED_BIN" >&2; exit 1; }
  if nm -gU "$CHECK_BIN" | grep -qE ' [Tt] _?(tinyc_main|main)$'; then
    echo "native: main entry exported"
  else
    echo "WARNING: no main/tinyc_main exported, runCli will be unavailable" >&2
  fi
elif [ "$HOST_TRIPLE" = "x86_64-w64-mingw32" ]; then
  PE_EXPORTS=$(objdump -p "$STAGED_BIN" | sed -n '/\[Ordinal\/Name Pointer\] Table/,$p')
  grep -qE '\[[ 0-9]+\] tcc_new$' <<<"$PE_EXPORTS" \
    || { echo "ERROR: tcc_new not exported by $STAGED_BIN" >&2; exit 1; }
  if grep -qE '\[[ 0-9]+\] (tinyc_main|main)$' <<<"$PE_EXPORTS"; then
    echo "native: main entry exported"
  else
    echo "WARNING: no main/tinyc_main exported, runCli will be unavailable" >&2
  fi
else
  nm -D "$STAGED_BIN" | grep -q ' T tcc_new' \
    || { echo "ERROR: tcc_new not exported by $STAGED_BIN" >&2; exit 1; }
  if nm -D "$STAGED_BIN" | grep -qE ' T (tinyc_main|main)$'; then
    echo "native: main entry exported"
  else
    echo "WARNING: no main/tinyc_main exported, runCli will be unavailable" >&2
  fi
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
