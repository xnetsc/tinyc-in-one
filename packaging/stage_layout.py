#!/usr/bin/env python3
"""Stage the unified (host x target) distribution layout into a package root.

Layout written under <root>:

    packaging-manifest.json
    natives/<host-triple>/toolchain/<target-triple>/tinyc[.exe]
    sysroot/<target-triple>/include/         (embedder headers: libtcc.h, ...)
    sysroot/<target-triple>/lib/tcc/         (libtcc1.a, TCC's own include/, ...)
    sysroot/<target-triple>/manifest.json    (per-target build manifest)

Sources are the kernel builds in build/dist/<target-triple>/. A dist
binary is staged under a host only when its file format/machine matches
that host: an x86-64 ELF is never staged as an aarch64 host build.
Sysroots are target data, host-independent, staged once per target.

Only 64-bit hosts and targets exist in this layout.
"""
import argparse
import json
import os
import shutil
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_DIST = os.path.normpath(os.path.join(HERE, "..", "build", "dist"))

# 64-bit hosts this distribution is planned for (host == target combos).
PLANNED_HOSTS = [
    "x86_64-linux-gnu",
    "aarch64-linux-gnu",
    "x86_64-w64-mingw32",
    "arm64-apple-darwin",
    "x86_64-apple-darwin",
]

ELF_MACHINE = {"x86_64": 62, "aarch64": 183, "riscv64": 243}


def binary_machine(path):
    """Return ('elf', e_machine) or ('pe', None) or ('macho', None)."""
    with open(path, "rb") as f:
        head = f.read(20)
    if head[:2] == b"MZ":
        return ("pe", None)
    if head[:4] == b"\x7fELF":
        e_machine = struct.unpack("<H", head[18:20])[0]
        return ("elf", e_machine)
    if head[:4] in (b"\xfe\xed\xfa\xcf", b"\xcf\xfa\xed\xfe"):
        return ("macho", None)
    return ("unknown", None)


def host_accepts(host, fmt, machine):
    arch = host.split("-")[0]
    if fmt == "elf":
        return "linux" in host and ELF_MACHINE.get(arch) == machine
    if fmt == "pe":
        return "windows" in host or "mingw" in host
    if fmt == "macho":
        return "apple" in host or "darwin" in host
    return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True, help="package root to stage into")
    ap.add_argument("--host", required=True, help="host triple the binaries run on")
    ap.add_argument("--dist", default=os.environ.get("TINYC_DIST_DIR", DEFAULT_DIST),
                    help="build/dist directory (or $TINYC_DIST_DIR)")
    args = ap.parse_args()

    root = os.path.abspath(args.root)
    host = args.host
    combos = []
    kernel_version = None

    if not os.path.isdir(args.dist):
        sys.exit(f"dist directory not found: {args.dist}")

    for target in sorted(os.listdir(args.dist)):
        tdir = os.path.join(args.dist, target)
        manifest_path = os.path.join(tdir, "manifest.json")
        if not os.path.isdir(tdir) or not os.path.exists(manifest_path):
            if os.path.isdir(tdir):
                print(f"WARNING: dist target {target} has no manifest.json "
                      f"(build incomplete?) - not staged", file=sys.stderr)
            continue
        with open(manifest_path) as f:
            tman = json.load(f)
        dual = tman.get("dual_file", "bin/tinyc")
        src_bin = os.path.join(tdir, dual)
        if not os.path.exists(src_bin):
            print(f"WARNING: dist target {target} has no {dual} "
                  f"(build incomplete?) - not staged", file=sys.stderr)
            continue
        fmt, machine = binary_machine(src_bin)
        if not host_accepts(host, fmt, machine):
            print(f"skip {target}: binary format {fmt}/{machine} "
                  f"does not run on host {host}")
            continue
        kernel_version = kernel_version or tman.get("version")

        bin_name = os.path.basename(dual)
        dst_bin = os.path.join(root, "natives", host, "toolchain", target, bin_name)
        os.makedirs(os.path.dirname(dst_bin), exist_ok=True)
        shutil.copy2(src_bin, dst_bin)
        os.chmod(dst_bin, 0o755)

        sysroot_rel = os.path.join("sysroot", target)
        sysroot_abs = os.path.join(root, sysroot_rel)
        if not os.path.exists(os.path.join(sysroot_abs, "manifest.json")):
            os.makedirs(sysroot_abs, exist_ok=True)
            for sub in ("include", "lib"):
                src_sub = os.path.join(tdir, sub)
                if os.path.isdir(src_sub):
                    shutil.copytree(src_sub, os.path.join(sysroot_abs, sub),
                                    dirs_exist_ok=True)
            shutil.copy2(manifest_path,
                         os.path.join(sysroot_abs, "manifest.json"))

        combo = {
            "host": host,
            "target": target,
            "binary": os.path.join("natives", host, "toolchain", target,
                                   bin_name).replace(os.sep, "/"),
            "sysroot": sysroot_rel.replace(os.sep, "/"),
            "isa": tman.get("isa"),
            "status": "included",
        }
        # Output formats are defined once, in the target's dist
        # manifest ("outputs"); package manifests copy the value
        # verbatim so every host and every package agrees on what a
        # target can generate. Copied only when present: absence is
        # caught by tests/check_outputs_consistency.py.
        if tman.get("outputs") is not None:
            combo["outputs"] = tman["outputs"]
        combos.append(combo)
        print(f"staged {host} x {target}")

    if not combos:
        sys.exit(f"no combinations staged for host {host} from {args.dist}")

    staged_hosts = {c["host"] for c in combos}
    pending = [{"host": h, "target": h, "status": "pending"}
               for h in PLANNED_HOSTS if h not in staged_hosts]
    manifest = {
        "name": "tinyc",
        "layout": 1,
        "kernelVersion": kernel_version,
        "combinations": combos,
        "pending": pending,
    }
    with open(os.path.join(root, "packaging-manifest.json"), "w") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)
        f.write("\n")
    print(f"wrote {os.path.join(root, 'packaging-manifest.json')} "
          f"({len(combos)} combinations, {len(pending)} pending)")


if __name__ == "__main__":
    main()
