#!/usr/bin/env python3
"""Tests for packaging/stage_layout.py.

Run: python3 packaging/test_stage_layout.py

Covers the two full-matrix prerequisites:
  1. Mach-O cputype recognition - a Mach-O binary is staged only
     under the mac host whose CPU type matches its header (arm64
     Mach-O under arm64-apple-darwin, x86_64 Mach-O under
     x86_64-apple-darwin, never crossed). Both mis-staging (wrong
     arch staged) and missed staging (right arch skipped) are
     asserted.
  2. Multi-host staging mode - one invocation with several --host
     flags produces exactly the union of staging each host alone
     (same combination objects, same pending accounting).

Fixtures are synthetic dist trees: binaries are minimal headers
with the correct magic / machine / cputype bytes, manifests are
copies of the real dist manifests when a real dist is available
(repo build/dist, $TINYC_TEST_DIST, or the cross-matrix worktree),
else a minimal inline manifest with the same fields.
"""
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, HERE)
import stage_layout  # noqa: E402

STAGE = os.path.join(HERE, "stage_layout.py")

# The nine targets of the full matrix.
ALL_TARGETS = [
    "aarch64-linux-android",
    "aarch64-linux-gnu",
    "arm64-apple-darwin",
    "riscv64-linux-gnu",
    "wasm32-wasi",
    "x86_64-apple-darwin",
    "x86_64-linux-android",
    "x86_64-linux-gnu",
    "x86_64-w64-mingw32",
]

# A build-host dist holds cross compilers: every target's compiler
# binary runs on the BUILD host, so on a Linux x86-64 build host all
# nine are ELF x86-64 (even the aarch64 / riscv64 / darwin / mingw32
# compilers - they only *emit* those targets). A host-variant dist
# (build/dist-host) instead holds binaries that run on the target
# platform itself: a PE file for mingw32, Mach-O files for darwin.
def linux_host_dist_blobs():
    return {t: elf_header(62) for t in ALL_TARGETS}


def elf_header(e_machine):
    return b"\x7fELF" + b"\x00" * 14 + struct.pack("<H", e_machine)


def pe_header():
    return b"MZ" + b"\x00" * 18


def macho_header(arch, endian="<"):
    """Thin Mach-O 64-bit header: magic + cputype, little-endian
    (the form real macOS files take) unless endian='>'."""
    return (struct.pack(endian + "II", 0xFEEDFACF,
                        stage_layout.MACHO_CPUTYPE[arch])
            + b"\x00" * 24)


def fat_header(arches, endian="<"):
    """Universal (fat) Mach-O header naming the given arches."""
    magic = b"\xbf\xba\xfe\xca" if endian == "<" else b"\xca\xfe\xba\xbe"
    out = magic + struct.pack(endian + "I", len(arches))
    offset = 8 + 20 * len(arches)
    for arch in arches:
        out += struct.pack(endian + "IIIII",
                           stage_layout.MACHO_CPUTYPE[arch], 3,
                           offset, 32, 0)
        offset += 32
    return out


class StageLayoutTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.real_dist = None
        for cand in (os.environ.get("TINYC_TEST_DIST"),
                     os.path.join(ROOT, "build", "dist"),
                     os.path.expanduser(
                         "~/workspace/tinyc-cross-matrix/build/dist")):
            if cand and os.path.isdir(cand):
                cls.real_dist = cand
                break

    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="stage-test-")

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    # ---- fixture helpers ----

    def _manifest_for(self, target):
        if self.real_dist:
            src = os.path.join(self.real_dist, target, "manifest.json")
            if os.path.exists(src):
                with open(src) as f:
                    return json.load(f)
        fmt = {"arm64-apple-darwin": "Mach-O executable",
               "x86_64-apple-darwin": "Mach-O executable",
               "x86_64-w64-mingw32": "PE executable"}.get(
            target, "ELF executable")
        return {"name": "tinyc", "version": "0.9.28rc-test",
                "target": target, "isa": "test",
                "outputs": {"exe": {"format": fmt, "ext": ""}},
                "dual_file": "bin/tinyc",
                "libtcc1": "lib/tcc/libtcc1.a"}

    def make_dist(self, target_blobs, name="dist"):
        """target_blobs: {triple: binary bytes}. Returns the dist dir."""
        dist = os.path.join(self.tmp, name)
        for target, blob in target_blobs.items():
            tdir = os.path.join(dist, target)
            os.makedirs(os.path.join(tdir, "bin"))
            os.makedirs(os.path.join(tdir, "include"))
            os.makedirs(os.path.join(tdir, "lib"))
            with open(os.path.join(tdir, "bin", "tinyc"), "wb") as f:
                f.write(blob)
            with open(os.path.join(tdir, "manifest.json"), "w") as f:
                json.dump(self._manifest_for(target), f)
        return dist

    def stage(self, dists, hosts, root=None):
        if isinstance(dists, str):
            dists = [dists]
        root = root or os.path.join(self.tmp, "pkg")
        cmd = [sys.executable, STAGE, "--root", root]
        for d in dists:
            cmd += ["--dist", d]
        for h in hosts:
            cmd += ["--host", h]
        r = subprocess.run(cmd, capture_output=True, text=True)
        manifest = None
        mpath = os.path.join(root, "packaging-manifest.json")
        if os.path.exists(mpath):
            with open(mpath) as f:
                manifest = json.load(f)
        return r, manifest

    def combos_by_host(self, manifest):
        by_host = {}
        for c in manifest["combinations"]:
            by_host.setdefault(c["host"], []).append(c)
        return by_host

    # ---- 1. Mach-O cputype recognition ----

    def test_binary_machine_macho_thin(self):
        for arch in ("arm64", "x86_64"):
            for endian in ("<", ">"):
                p = os.path.join(self.tmp, f"macho-{arch}-{endian}")
                with open(p, "wb") as f:
                    f.write(macho_header(arch, endian))
                fmt, arches = stage_layout.binary_machine(p)
                self.assertEqual(fmt, "macho")
                self.assertEqual(arches, frozenset([arch]))
        # Unrecognised cputype: recognised as Mach-O, no known arch.
        p = os.path.join(self.tmp, "macho-unknown")
        with open(p, "wb") as f:
            f.write(struct.pack("<II", 0xFEEDFACF, 0x01000009)
                    + b"\x00" * 24)
        self.assertEqual(stage_layout.binary_machine(p), ("macho", None))
        # 32-bit Mach-O magic is not a kind this layout ships.
        p = os.path.join(self.tmp, "macho-32")
        with open(p, "wb") as f:
            f.write(struct.pack("<II", 0xFEEDFACE, 7) + b"\x00" * 24)
        self.assertEqual(stage_layout.binary_machine(p),
                         ("unknown", None))

    def test_binary_machine_macho_fat(self):
        p = os.path.join(self.tmp, "fat-both")
        with open(p, "wb") as f:
            f.write(fat_header(["arm64", "x86_64"]))
        fmt, arches = stage_layout.binary_machine(p)
        self.assertEqual(fmt, "macho")
        self.assertEqual(arches, frozenset(["arm64", "x86_64"]))
        p = os.path.join(self.tmp, "fat-arm64")
        with open(p, "wb") as f:
            f.write(fat_header(["arm64"], endian=">"))
        self.assertEqual(stage_layout.binary_machine(p)[1],
                         frozenset(["arm64"]))

    def test_binary_machine_elf_pe(self):
        for machine, want in ((62, 62), (183, 183), (243, 243)):
            p = os.path.join(self.tmp, f"elf-{machine}")
            with open(p, "wb") as f:
                f.write(elf_header(machine))
            self.assertEqual(stage_layout.binary_machine(p),
                             ("elf", want))
        p = os.path.join(self.tmp, "pe")
        with open(p, "wb") as f:
            f.write(pe_header())
        self.assertEqual(stage_layout.binary_machine(p), ("pe", None))

    def test_host_accepts_macho(self):
        arm64 = frozenset(["arm64"])
        x64 = frozenset(["x86_64"])
        both = frozenset(["arm64", "x86_64"])
        self.assertTrue(stage_layout.host_accepts(
            "arm64-apple-darwin", "macho", arm64))
        self.assertFalse(stage_layout.host_accepts(
            "arm64-apple-darwin", "macho", x64))
        self.assertTrue(stage_layout.host_accepts(
            "x86_64-apple-darwin", "macho", x64))
        self.assertFalse(stage_layout.host_accepts(
            "x86_64-apple-darwin", "macho", arm64))
        # A universal binary is accepted by either arch host.
        self.assertTrue(stage_layout.host_accepts(
            "arm64-apple-darwin", "macho", both))
        self.assertTrue(stage_layout.host_accepts(
            "x86_64-apple-darwin", "macho", both))
        # Mach-O never runs on a Linux host; unknown cputype is
        # never staged blindly.
        self.assertFalse(stage_layout.host_accepts(
            "x86_64-linux-gnu", "macho", arm64))
        self.assertFalse(stage_layout.host_accepts(
            "arm64-apple-darwin", "macho", None))

    def test_macho_staging_discrimination(self):
        """Each mac host stages only its own arch's Mach-O compiler."""
        dist = self.make_dist({
            "arm64-apple-darwin": macho_header("arm64"),
            "x86_64-apple-darwin": macho_header("x86_64"),
        })
        r, arm_manifest = self.stage(dist, ["arm64-apple-darwin"])
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual([c["target"] for c in
                          arm_manifest["combinations"]],
                         ["arm64-apple-darwin"])
        self.assertFalse(os.path.exists(os.path.join(
            self.tmp, "pkg", "natives", "arm64-apple-darwin",
            "toolchain", "x86_64-apple-darwin")))
        r, x64_manifest = self.stage(
            dist, ["x86_64-apple-darwin"],
            root=os.path.join(self.tmp, "pkg2"))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual([c["target"] for c in
                          x64_manifest["combinations"]],
                         ["x86_64-apple-darwin"])
        self.assertFalse(os.path.exists(os.path.join(
            self.tmp, "pkg2", "natives", "x86_64-apple-darwin",
            "toolchain", "arm64-apple-darwin")))

    # ---- 2. Linux 9/9 staging unchanged ----

    def test_linux_nine_unchanged(self):
        if not self.real_dist:
            self.skipTest("no real dist available "
                          "(build/dist or $TINYC_TEST_DIST)")
        r, manifest = self.stage(self.real_dist, ["x86_64-linux-gnu"])
        self.assertEqual(r.returncode, 0, r.stderr)
        targets = sorted(c["target"] for c in manifest["combinations"])
        self.assertEqual(targets, sorted(ALL_TARGETS))
        self.assertTrue(all(c["status"] == "included"
                            for c in manifest["combinations"]))
        self.assertEqual(manifest["hosts"], ["x86_64-linux-gnu"])
        self.assertEqual(len(manifest["pending"]), 4)

    # ---- 3. Multi-host mode ----

    def test_multi_host_equals_union_of_singles(self):
        # A dist holding binaries for four hosts: the nine Linux-hosted
        # cross compilers, except the two darwin compilers are Mach-O
        # (as a mac build's dist would hold them) and the mingw32
        # compiler is a PE file (as a host-variant dist holds it).
        blobs = linux_host_dist_blobs()
        blobs["arm64-apple-darwin"] = macho_header("arm64")
        blobs["x86_64-apple-darwin"] = macho_header("x86_64")
        blobs["x86_64-w64-mingw32"] = pe_header()
        dist = self.make_dist(blobs)
        hosts = ["x86_64-linux-gnu", "x86_64-w64-mingw32",
                 "arm64-apple-darwin", "x86_64-apple-darwin"]
        singles = {}
        for h in hosts:
            r, manifest = self.stage(
                dist, [h], root=os.path.join(self.tmp, f"single-{h}"))
            self.assertEqual(r.returncode, 0, r.stderr)
            singles[h] = manifest
        r, merged = self.stage(dist, hosts,
                               root=os.path.join(self.tmp, "merged"))
        self.assertEqual(r.returncode, 0, r.stderr)
        merged_by_host = self.combos_by_host(merged)
        self.assertEqual(sorted(merged_by_host), sorted(hosts))
        for h in hosts:
            self.assertEqual(merged_by_host[h],
                             singles[h]["combinations"],
                             f"host {h} differs between multi-host "
                             f"and single-host staging")
        self.assertEqual(merged["hosts"], hosts)
        # Four of five planned hosts staged: one pending slot left.
        self.assertEqual(len(merged["pending"]), 1)
        self.assertEqual(merged["pending"][0]["host"],
                         "aarch64-linux-gnu")
        # Duplicate --host flags collapse to one staging of that host.
        r, dup = self.stage(dist, ["arm64-apple-darwin",
                                   "arm64-apple-darwin"],
                            root=os.path.join(self.tmp, "dup"))
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(dup["combinations"],
                         singles["arm64-apple-darwin"]["combinations"])

    def test_multi_dist_multi_host(self):
        """Release-aggregation shape: a cross-compiler dist plus a
        host-variant dist, staged for four hosts in one run."""
        # Dist A: Linux-hosted cross compilers (all nine targets, all
        # ELF x86-64 - as build/dist is on a Linux build host).
        dist_a = self.make_dist(linux_host_dist_blobs(), name="dist")
        # Dist B: host variants (as build/dist-host is): a PE mingw32
        # compiler and Mach-O compilers for the two mac hosts.
        blobs_b = {
            "x86_64-w64-mingw32": pe_header(),
            "arm64-apple-darwin": macho_header("arm64"),
            "x86_64-apple-darwin": macho_header("x86_64"),
        }
        dist_b = self.make_dist(blobs_b, name="dist-host")
        hosts = ["x86_64-linux-gnu", "x86_64-w64-mingw32",
                 "arm64-apple-darwin", "x86_64-apple-darwin"]
        singles = {}
        for h in hosts:
            r, manifest = self.stage(
                [dist_a, dist_b], [h],
                root=os.path.join(self.tmp, f"single-{h}"))
            self.assertEqual(r.returncode, 0, r.stderr)
            singles[h] = manifest
        r, merged = self.stage([dist_a, dist_b], hosts,
                               root=os.path.join(self.tmp, "merged2"))
        self.assertEqual(r.returncode, 0, r.stderr)
        merged_by_host = self.combos_by_host(merged)
        self.assertEqual(sorted(merged_by_host), sorted(hosts))
        for h in hosts:
            self.assertEqual(merged_by_host[h],
                             singles[h]["combinations"],
                             f"host {h} differs between multi-host "
                             f"and single-host staging")
        # Linux host takes all nine from dist A; each other host
        # takes its native compiler from dist B.
        self.assertEqual(len(merged["combinations"]), 12)
        self.assertEqual(len(merged_by_host["x86_64-linux-gnu"]), 9)
        self.assertEqual(merged["pending"][0]["host"],
                         "aarch64-linux-gnu")
        # Dist precedence: the mingw32 compiler staged for the
        # Windows host is the PE file from dist B, not dist A's ELF.
        staged_pe = os.path.join(
            self.tmp, "merged2", "natives", "x86_64-w64-mingw32",
            "toolchain", "x86_64-w64-mingw32", "tinyc")
        with open(staged_pe, "rb") as f:
            self.assertEqual(f.read(2), b"MZ")

    def test_multi_host_partial_empty_host_warns(self):
        # A dist with only Mach-O arm64: the x86_64 mac host finds
        # nothing (warned, not fatal) while the arm64 host stages one.
        dist = self.make_dist({
            "arm64-apple-darwin": macho_header("arm64"),
        })
        r, manifest = self.stage(
            dist, ["arm64-apple-darwin", "x86_64-apple-darwin"])
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(len(manifest["combinations"]), 1)
        self.assertEqual(manifest["combinations"][0]["host"],
                         "arm64-apple-darwin")
        # A single host with nothing to stage still fails as before.
        r, manifest = self.stage(
            dist, ["x86_64-apple-darwin"],
            root=os.path.join(self.tmp, "empty"))
        self.assertNotEqual(r.returncode, 0)
        self.assertIsNone(manifest)


if __name__ == "__main__":
    unittest.main(verbosity=2)
