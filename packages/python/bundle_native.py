#!/usr/bin/env python3
"""Stage the unified (host x target) layout into tinyc/ so the package
is self-contained:

    tinyc/packaging-manifest.json
    tinyc/natives/<host>/toolchain/<target>/tinyc
    tinyc/sysroot/<target>/...

Usage: bundle_native.py [host-triple]   (default: the current host)
Run after ../../build.sh. Staging is done by ../../packaging/stage_layout.py.
"""
import os
import shutil
import subprocess
import sys

here = os.path.dirname(os.path.abspath(__file__))
pkg = os.path.join(here, "tinyc")
stage = os.path.normpath(os.path.join(here, "..", "..", "packaging",
                                      "stage_layout.py"))


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


host = sys.argv[1] if len(sys.argv) > 1 else detect_host()
for name in ("natives", "sysroot", "packaging-manifest.json", "native"):
    p = os.path.join(pkg, name)
    if os.path.isdir(p):
        shutil.rmtree(p)
    elif os.path.exists(p):
        os.remove(p)
subprocess.run([sys.executable, stage, "--root", pkg, "--host", host],
               check=True)
print(f"staged host {host} into {pkg}")
