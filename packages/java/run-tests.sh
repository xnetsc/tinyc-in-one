#!/usr/bin/env bash
# 运行绑定层测试，三遍：
#   A 开发模式：系统属性直接指定本地原生文件与运行时目录
#   B 分发模式：无属性，从 Jar resources 按组合解压到 ~/.cache/tinyc-java/<version>/
#   C 宿主模拟：TINYC_HOST_TRIPLE 覆盖探测。期望由本包 manifest 驱动
#     （与 Python 包的宿主模拟测试同一形态）：模拟的组合在本包内则
#     解析并跑全套，不在则断言 missing-combination 错误指名该宿主
#     并列出包内全部组合；32 位宿主单独断言拒绝。
set -euo pipefail
cd "$(dirname "$0")"

NAME=$(cat build/native-name.txt)
CP="dist/tinyc.jar:build/test-classes"
# JDK 定位与 build.sh 同一套规则：按版本逐个核验候选，取第一个主版本号
# >= 22 的，保证运行时 java 与 build.sh 编译用的 javac 出自同一个 JDK。
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
JAVA="$JDK_BIN/java --enable-native-access=ALL-UNNAMED"

echo "=== mode A: tinyc.native.path (dev) ==="
$JAVA -Dtinyc.native.path="$PWD/build/test-runtime/$NAME" \
      -Dtinyc.lib.path="$PWD/build/test-runtime" \
      -cp "$CP" tinyc.TinyCTest

echo "=== mode B: jar extraction (distribution) ==="
$JAVA -cp "$CP" tinyc.TinyCTest

# 本包 manifest 的组合清单，每行 "host target"（build.sh 的 staging
# 产物，与 jar 内的 packaging-manifest.json 同一份）。
manifest_combos() {
  python3 - "$PWD/build/jarroot/packaging-manifest.json" <<'PYEOF'
import json, sys
with open(sys.argv[1]) as f:
    mf = json.load(f)
for c in mf["combinations"]:
    print(c["host"], c["target"])
PYEOF
}

echo "=== mode C: host simulation via TINYC_HOST_TRIPLE ==="
echo "--- C1: x86_64-linux-gnu host simulation"
if manifest_combos | grep -qx "x86_64-linux-gnu x86_64-linux-gnu"; then
  echo "    (combination present in this package: resolves and runs the full suite)"
  TINYC_HOST_TRIPLE=x86_64-linux-gnu $JAVA -cp "$CP" tinyc.TinyCTest >/dev/null
else
  echo "    (combination absent from this package: the error must name it)"
  set +e
  OUT=$(TINYC_HOST_TRIPLE=x86_64-linux-gnu $JAVA -cp "$CP" tinyc.HostProbe 2>&1)
  RC=$?
  set -e
  [ $RC -ne 0 ] || { echo "C1 FAILED: probe unexpectedly succeeded"; echo "$OUT"; exit 1; }
  grep -q "no native build for host x86_64-linux-gnu" <<<"$OUT" \
    || { echo "C1 FAILED: host not named"; echo "$OUT"; exit 1; }
  grep -q "available combinations:" <<<"$OUT" \
    || { echo "C1 FAILED: available combinations not listed"; echo "$OUT"; exit 1; }
fi
echo "C1 OK"

echo "--- C2: combination absent from the package names host and available combinations"
# 缺席宿主取包内不存在者：默认 x86_64-w64-mingw32；若本包宿主正是它
# （Windows 上构建的 jar），改用 aarch64-linux-gnu。
ABSENT_HOST=x86_64-w64-mingw32
if manifest_combos | grep -q "^$ABSENT_HOST "; then
  ABSENT_HOST=aarch64-linux-gnu
fi
set +e
OUT=$(TINYC_HOST_TRIPLE=$ABSENT_HOST $JAVA -cp "$CP" tinyc.HostProbe 2>&1)
RC=$?
set -e
[ $RC -ne 0 ] || { echo "C2 FAILED: probe unexpectedly succeeded"; echo "$OUT"; exit 1; }
grep -q "no native build for host $ABSENT_HOST" <<<"$OUT" \
  || { echo "C2 FAILED: host not named"; echo "$OUT"; exit 1; }
# 错误信息里的 available 列表必须逐个列出本包 manifest 的全部组合。
while read -r combo_host combo_target; do
  grep -q "$combo_host x $combo_target" <<<"$OUT" \
    || { echo "C2 FAILED: available list missing $combo_host x $combo_target"; echo "$OUT"; exit 1; }
done < <(manifest_combos)
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
