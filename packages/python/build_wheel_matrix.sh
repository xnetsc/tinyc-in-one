#!/usr/bin/env bash
# Build one platform wheel per host triple. A wheel's platform tag is
# the host dimension: the wheel carries the TCC binaries that run on
# that host, with every staged target variant inside. The two macOS
# hosts are the exception: they share one universal2 wheel
# (macosx_11_0_universal2), built by build_universal2 below - the
# universal2 floor is 11.0, so Intel macOS 10.9-10.15 lose
# installability (the old x86_64 wheel was macosx_10_9).
#
# A wheel is built only when staging for that host succeeds, i.e. real
# binaries for the host exist in ../../build/dist. Hosts without a
# staged build are reported PENDING and skipped; no wheel is produced
# for them. Afterwards the staging for the current host is restored.
set -u
cd "$(dirname "$0")"
OUT=dist-wheels
mkdir -p "$OUT"

build_one() {
  local host=$1 plat=$2
  if ! python3 bundle_native.py "$host" >/tmp/tinyc-wheel-stage.log 2>&1; then
    echo "PENDING: host $host ($plat) - no staged native build for this host"
    return
  fi
  # A wheel without its native (host == target) combination is broken;
  # refuse to build it (staging can miss a combo while build/dist is
  # being rebuilt).
  if ! python3 -c "
import json, sys
m = json.load(open('tinyc/packaging-manifest.json'))
ok = any(c['host'] == '$host' and c['target'] == '$host'
         for c in m['combinations'])
sys.exit(0 if ok else 1)"; then
    echo "FAILED:  host $host ($plat) - staging lacks the native (host == target) combination"
    return
  fi
  rm -rf build tinyc.egg-info
  if TINYC_WHEEL_PLAT=$plat python3 -m pip wheel . -w "$OUT" --no-deps \
       --no-build-isolation >/tmp/tinyc-wheel-build.log 2>&1; then
    echo "BUILT:   host $host -> $OUT (tag $plat)"
  else
    echo "FAILED:  host $host ($plat) - see /tmp/tinyc-wheel-build.log"
  fi
  rm -rf build tinyc.egg-info
}

build_universal2() {
  # One wheel for both macOS hosts: bundle_native.py --merge-darwin
  # stages both darwin hosts into one tinyc/ tree (sysroot once per
  # target); the wheel is tagged macosx_11_0_universal2. Both darwin
  # dists must be reachable (TINYC_DIST_DIRS / --dist passthrough in
  # bundle_native.py); the release aggregation job collects them
  # from the two mac jobs.
  if python3 bundle_native.py --merge-darwin >/tmp/tinyc-wheel-stage.log 2>&1; then
    # A wheel without both native (host == target) combinations is
    # broken; refuse to build it.
    if ! python3 -c "
import json, sys
m = json.load(open('tinyc/packaging-manifest.json'))
hosts = ('arm64-apple-darwin', 'x86_64-apple-darwin')
ok = all(any(c['host'] == h and c['target'] == h
             for c in m['combinations']) for h in hosts)
sys.exit(0 if ok else 1)"; then
      echo "FAILED:  hosts arm64/x86_64-apple-darwin (macosx_11_0_universal2) - staging lacks a native (host == target) combination"
      return
    fi
    rm -rf build tinyc.egg-info
    if TINYC_WHEEL_PLAT=macosx_11_0_universal2 python3 -m pip wheel . -w "$OUT" --no-deps \
         --no-build-isolation >/tmp/tinyc-wheel-build.log 2>&1; then
      echo "BUILT:   hosts arm64/x86_64-apple-darwin -> $OUT (tag macosx_11_0_universal2)"
    else
      echo "FAILED:  hosts arm64/x86_64-apple-darwin (macosx_11_0_universal2) - see /tmp/tinyc-wheel-build.log"
    fi
    rm -rf build tinyc.egg-info
    return
  fi
  # Merge staging failed: only one darwin dist is reachable, which
  # is the normal case on a single-arch mac CI job (its TINYC_DIST_DIR
  # holds just its own host). Build this host's single-arch wheel
  # instead, with its original tag, so the job's consumer test still
  # has a wheel; the merged universal2 wheel is built by the release
  # aggregation job, where both darwin dists are collected.
  case "$(uname -m)" in
    arm64)  build_one arm64-apple-darwin macosx_11_0_arm64 ;;
    x86_64) build_one x86_64-apple-darwin macosx_10_9_x86_64 ;;
    *) echo "FAILED:  hosts arm64/x86_64-apple-darwin (macosx_11_0_universal2) - merge staging failed and this host ($(uname -m)) is not a darwin host" ;;
  esac
}

build_one x86_64-linux-gnu    manylinux_2_17_x86_64
build_one aarch64-linux-gnu   manylinux_2_17_aarch64
build_one riscv64-linux-gnu   manylinux_2_17_riscv64
build_one x86_64-w64-mingw32  win_amd64
build_universal2

# restore staging for the host this script runs on
python3 bundle_native.py >/dev/null 2>&1 \
  && echo "staging restored for current host" \
  || echo "WARNING: could not restore staging for current host"
ls -1 "$OUT" 2>/dev/null
