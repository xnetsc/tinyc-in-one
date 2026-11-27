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
that host: an x86-64 ELF is never staged as an aarch64 host build, and
a Mach-O binary is staged only under the host whose CPU type matches
its cputype (arm64 Mach-O under arm64-apple-darwin, x86_64 Mach-O
under x86_64-apple-darwin - never crossed).
Sysroots are target data, host-independent, staged once per target.

Multi-host mode: --host may be repeated. One invocation stages every
given host into the same root and writes a single merged manifest:
the combinations of each host are identical to staging that host
alone, each combination carries its home host in its "host" field, and
the manifest's "hosts" field lists the staged hosts. --dist may also
be repeated: dist directories are consulted in order (cross-compiler
dist first, then host-variant dists such as build/dist-host), and for
each (host, target) the first dist whose compiler binary runs on the
host wins. The "pending" field is the host-slot accounting: one entry
per planned host whose native (host == target) combination has not
been staged in this root yet - a single-host run stages one host and
leaves the other four pending; a full aggregation run stages all five
hosts and leaves none pending. Pending entries are host slots, not
target combinations.

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

# Mach-O CPU types (mach/machine.h): only the two 64-bit kinds this
# layout ships. A Mach-O file names its CPU in the header, so staging
# must match on it exactly - the file format alone ("macho") says
# nothing about which mac host the binary runs on.
MACHO_CPUTYPE = {"x86_64": 0x01000007, "arm64": 0x0100000C}
MACHO_CPUTYPE_REV = {v: k for k, v in MACHO_CPUTYPE.items()}


def _macho_arches(f, head):
    """Architectures named in a Mach-O header.

    Returns a frozenset of {"arm64", "x86_64"} - one member for a thin
    binary, several for a universal (fat) binary - or None when the
    header names no known 64-bit CPU. 32-bit Mach-O is not a kind this
    layout ships and is not recognised.
    """
    magic = head[:4]
    if magic == b"\xfe\xed\xfa\xcf":       # MH_MAGIC_64, big-endian
        endian = ">"
    elif magic == b"\xcf\xfa\xed\xfe":     # MH_CIGAM_64, little-endian
        endian = "<"
    elif magic in (b"\xca\xfe\xba\xbe", b"\xbf\xba\xfe\xca"):
        # FAT_MAGIC / FAT_CIGAM: a universal binary. The fat header
        # and fat_arch entries use the magic's byte order.
        endian = ">" if magic == b"\xca\xfe\xba\xbe" else "<"
        f.seek(0)
        fat = f.read(8 + 20 * 8)  # room for 8 arch entries
        narch = struct.unpack(endian + "I", fat[4:8])[0]
        arches = set()
        for i in range(min(narch, 8)):
            cputype = struct.unpack(endian + "I",
                                    fat[8 + 20 * i:12 + 20 * i])[0]
            arch = MACHO_CPUTYPE_REV.get(cputype)
            if arch:
                arches.add(arch)
        return frozenset(arches) if arches else None
    else:
        return None
    cputype = struct.unpack(endian + "I", head[4:8])[0]
    arch = MACHO_CPUTYPE_REV.get(cputype)
    return frozenset([arch]) if arch else None


def binary_machine(path):
    """Return ('elf', e_machine), ('pe', None), ('macho', arches) or
    ('unknown', None). For Mach-O, arches is the frozenset of CPU
    architectures the binary contains, or None when the header names
    no known 64-bit CPU."""
    with open(path, "rb") as f:
        head = f.read(20)
        if head[:2] == b"MZ":
            return ("pe", None)
        if head[:4] == b"\x7fELF":
            e_machine = struct.unpack("<H", head[18:20])[0]
            return ("elf", e_machine)
        arches = _macho_arches(f, head)
        if arches is not None:
            return ("macho", arches)
        if head[:4] in (b"\xfe\xed\xfa\xcf", b"\xcf\xfa\xed\xfe",
                        b"\xca\xfe\xba\xbe", b"\xbf\xba\xfe\xca"):
            return ("macho", None)
    return ("unknown", None)


def _fmt_desc(fmt, machine):
    if isinstance(machine, frozenset):
        return f"{fmt}/{{{','.join(sorted(machine))}}}"
    return f"{fmt}/{machine}"


def host_accepts(host, fmt, machine):
    arch = host.split("-")[0]
    if fmt == "elf":
        return "linux" in host and ELF_MACHINE.get(arch) == machine
    if fmt == "pe":
        return "windows" in host or "mingw" in host
    if fmt == "macho":
        # The CPU type is the discriminator: an arm64 Mach-O binary
        # must never be staged under an x86_64 mac host, and vice
        # versa. A Mach-O file with an unrecognised cputype is never
        # staged blindly.
        if "apple" not in host and "darwin" not in host:
            return False
        if machine is None:
            return False
        return arch in machine
    return False


def stage_host(root, host, dists):
    """Stage one host's combinations from a list of dist directories.

    Dist directories are consulted in order; for each (host, target)
    the first dist whose compiler binary runs on the host wins, and
    the sysroot is staged from that same dist. This is how the
    release aggregation step feeds the mode: the cross-compiler dist
    (binaries that run on the build host) plus the host-variant dists
    (build/dist-host, whose binaries run on the target platform
    itself - e.g. the mingw32 PE tinyc.exe). A target whose binary
    runs on no given host is skipped, never staged blindly.

    Returns (combos, kernel_version). The combination objects are
    built from (host, target) alone, so a host's combinations are
    identical whether it is staged alone or as part of a multi-host
    run. Sysroots are staged once per target, shared across hosts.
    """
    combos = []
    kernel_version = None
    targets = set()
    for dist in dists:
        if os.path.isdir(dist):
            targets.update(os.listdir(dist))
    for target in sorted(targets):
        picked = None  # (tdir, tman, src_bin)
        for dist in dists:
            tdir = os.path.join(dist, target)
            manifest_path = os.path.join(tdir, "manifest.json")
            if not os.path.isdir(tdir) or not os.path.exists(manifest_path):
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
                continue
            picked = (tdir, tman, src_bin, dual)
            break
        if picked is None:
            present = any(os.path.isdir(os.path.join(d, target))
                          for d in dists)
            if present:
                print(f"skip {target}: no binary in the given dists "
                      f"runs on host {host}")
            else:
                print(f"WARNING: dist target {target} has no manifest.json "
                      f"(build incomplete?) - not staged", file=sys.stderr)
            continue
        tdir, tman, src_bin, dual = picked
        manifest_path = os.path.join(tdir, "manifest.json")
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
        # The runtime directory a package registers with
        # tcc_set_lib_path (the {B} base) is defined once, here, from
        # the target's dist manifest: its "libtcc1" entry locates the
        # support library inside the sysroot, and libtcc's search
        # rules place it directly in {B} on ELF/Mach-O targets but in
        # {B}/lib on PE targets - so the PE runtime root is one level
        # up (the win32 directory), not lib/tcc itself. Packages copy
        # "libPath" verbatim, like "outputs"; "libtcc1" is the
        # package-root-relative path of the support library itself.
        libtcc1_rel = tman.get("libtcc1")
        if libtcc1_rel:
            parts = libtcc1_rel.replace(os.sep, "/").split("/")
            # PE form is a property of the target backend (tcc.h keys
            # the search paths on TCC_TARGET_PE), not of the staged
            # binary's own file format: a Linux-hosted cross binary
            # for the PE target is an ELF file with PE search rules.
            # The dist manifest's outputs carry the target's form.
            exe_format = (tman.get("outputs") or {}).get("exe", {})
            pe_target = str(exe_format.get("format", "")).startswith("PE")
            runtime_parts = parts[:-2] if pe_target else parts[:-1]
            combo["libPath"] = (sysroot_rel.replace(os.sep, "/") + "/"
                                + "/".join(runtime_parts))
            combo["libtcc1"] = (sysroot_rel.replace(os.sep, "/") + "/"
                                + "/".join(parts))
        # Output formats are defined once, in the target's dist
        # manifest ("outputs"); package manifests copy the value
        # verbatim so every host and every package agrees on what a
        # target can generate. Copied only when present: absence is
        # caught by tests/check_outputs_consistency.py.
        if tman.get("outputs") is not None:
            combo["outputs"] = tman["outputs"]
        combos.append(combo)
        print(f"staged {host} x {target}")
    print(f"staged {len(combos)} combinations for host {host}")
    return combos, kernel_version


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True, help="package root to stage into")
    ap.add_argument("--host", required=True, action="append",
                    help="host triple the binaries run on; repeat for "
                         "multi-host staging (one merged manifest)")
    ap.add_argument("--dist", default=None, action="append",
                    help="build/dist directory (repeatable; consulted in "
                         "order - cross-compiler dist first, then "
                         "host-variant dists such as build/dist-host). "
                         "Defaults to $TINYC_DIST_DIR or build/dist.")
    args = ap.parse_args()

    root = os.path.abspath(args.root)
    # Dedupe, preserving command-line order.
    hosts = list(dict.fromkeys(args.host))
    dists = args.dist or [os.environ.get("TINYC_DIST_DIR", DEFAULT_DIST)]

    for dist in dists:
        if not os.path.isdir(dist):
            sys.exit(f"dist directory not found: {dist}")

    combos = []
    kernel_version = None
    for host in hosts:
        host_combos, host_version = stage_host(root, host, dists)
        if not host_combos:
            print(f"WARNING: no combinations staged for host {host} "
                  f"from {', '.join(dists)}", file=sys.stderr)
        kernel_version = kernel_version or host_version
        combos.extend(host_combos)

    if not combos:
        sys.exit(f"no combinations staged for hosts {', '.join(hosts)} "
                 f"from {', '.join(dists)}")

    staged_hosts = [h for h in hosts
                    if any(c["host"] == h for c in combos)]
    pending = [{"host": h, "target": h, "status": "pending"}
               for h in PLANNED_HOSTS if h not in staged_hosts]
    manifest = {
        "name": "tinyc",
        "layout": 1,
        "kernelVersion": kernel_version,
        "hosts": staged_hosts,
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
