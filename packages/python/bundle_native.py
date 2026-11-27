#!/usr/bin/env python3
"""Stage the unified (host x target) layout into tinyc/ so the package
is self-contained:

    tinyc/packaging-manifest.json
    tinyc/natives/<host>/toolchain/<target>/tinyc
    tinyc/sysroot/<target>/...

Usage: bundle_native.py [host-triple]   (default: the current host)
       bundle_native.py --merge-darwin  (stage both macOS hosts into
                                         one tinyc/ tree, for the
                                         universal2 wheel)
Run after ../../build.sh. Staging is done by ../../packaging/stage_layout.py.

The two macOS hosts share one wheel (pip platform tag universal2):
--merge-darwin stages both darwin hosts, each with its native
combination, sysroot stored once per target. The merged tree can
only be assembled where both darwin dists are present, so dists
come from repeatable --dist, TINYC_DIST_DIRS (space separated) or
TINYC_DIST_DIR; with none given, stage_layout falls back to its own
default (TINYC_DIST_DIR or build/dist).
"""
import json
import os
import shutil
import subprocess
import sys

here = os.path.dirname(os.path.abspath(__file__))
pkg = os.path.join(here, "tinyc")
stage = os.path.normpath(os.path.join(here, "..", "..", "packaging",
                                      "stage_layout.py"))

MERGE_DARWIN_HOSTS = ("arm64-apple-darwin", "x86_64-apple-darwin")


def detect_host():
    import platform
    machine = platform.machine().lower()
    if sys.platform.startswith("linux"):
        if machine in ("x86_64", "amd64"):
            return "x86_64-linux-gnu"
        if machine in ("aarch64", "arm64"):
            return "aarch64-linux-gnu"
        if machine == "riscv64":
            return "riscv64-linux-gnu"
    if sys.platform == "win32" and machine in ("amd64", "x86_64"):
        return "x86_64-w64-mingw32"
    if sys.platform == "darwin":
        if machine == "arm64":
            return "arm64-apple-darwin"
        if machine in ("x86_64", "amd64"):
            return "x86_64-apple-darwin"
    sys.exit(f"cannot detect a 64-bit host triple for "
             f"{sys.platform}/{machine}")


args = sys.argv[1:]
merge_darwin = "--merge-darwin" in args
dist_args = []
i = 0
while i < len(args):
    if args[i] == "--dist":
        if i + 1 >= len(args):
            sys.exit("--dist needs a directory")
        dist_args.append(args[i + 1])
        i += 2
    else:
        i += 1
if os.environ.get("TINYC_DIST_DIRS"):
    dist_args.extend(os.environ["TINYC_DIST_DIRS"].split())
if os.environ.get("TINYC_DIST_DIR"):
    dist_args.append(os.environ["TINYC_DIST_DIR"])
positional = [a for j, a in enumerate(args)
              if a != "--merge-darwin" and a != "--dist"
              and not (j > 0 and args[j - 1] == "--dist")]
if len(positional) > 1:
    sys.exit("bundle_native.py takes at most one host triple")
if merge_darwin and positional:
    sys.exit("--merge-darwin takes no host triple")
hosts = MERGE_DARWIN_HOSTS if merge_darwin else \
    (positional[0],) if positional else (detect_host(),)

for name in ("natives", "sysroot", "packaging-manifest.json", "native"):
    p = os.path.join(pkg, name)
    if os.path.isdir(p):
        shutil.rmtree(p)
    elif os.path.exists(p):
        os.remove(p)
cmd = [sys.executable, stage, "--root", pkg]
for h in hosts:
    cmd += ["--host", h]
for d in dict.fromkeys(dist_args):  # dedupe, keep order
    cmd += ["--dist", d]
subprocess.run(cmd, check=True)

if merge_darwin:
    # The merged manifest must carry both darwin hosts, each with a
    # host == target native combination; otherwise the wheel would
    # silently serve only one arch. Fail naming the missing hosts
    # (TINYC_MERGE_ALLOW_PARTIAL=1 relaxes it for local dry runs).
    with open(os.path.join(pkg, "packaging-manifest.json")) as f:
        mf = json.load(f)
    staged = mf.get("hosts", [])
    combos = mf.get("combinations", [])
    missing = [h for h in MERGE_DARWIN_HOSTS if h not in staged]
    no_native = [h for h in MERGE_DARWIN_HOSTS
                 if not any(c["host"] == h and c["target"] == h
                            for c in combos)]
    print(f"merged staging: hosts {sorted(staged)} "
          f"({len(combos)} combinations)")
    if missing or no_native:
        print(f"ERROR: merged darwin staging is missing hosts {missing} / "
              f"hosts without a native combination {no_native}",
              file=sys.stderr)
        if os.environ.get("TINYC_MERGE_ALLOW_PARTIAL") != "1":
            sys.exit(1)
        print("WARNING: partial merged staging allowed by "
              "TINYC_MERGE_ALLOW_PARTIAL=1", file=sys.stderr)
else:
    print(f"staged host {hosts[0]} into {pkg}")
