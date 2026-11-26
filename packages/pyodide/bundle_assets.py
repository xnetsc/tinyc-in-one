#!/usr/bin/env python3
"""Bundle the tinyc-web build artifacts into tinyc_web/assets/.

The Python package is self-contained: it ships the tinyc-web JS
build and, per target, the compiler asset envelope and the sysroot
asset envelope. Nothing is written back into packages/browser: the
envelope generator (packages/browser/build-asset-envelopes.mjs)
runs against a staging directory, and the sysroot sources are the
repository's headers plus one libtcc1.a per target.

Two modes:

  python3 bundle_assets.py
      From this repository's browser package: the JS bundle is
      rebuilt from its sources with esbuild, and each target's
      compiler module pair (tcc-<target>.mjs + tcc-<target>.wasm)
      is taken from packages/browser. A target is bundled when its
      pair exists there and every sysroot source of its manifest
      is found; targets without a local compiler build are skipped
      and reported.

  python3 bundle_assets.py --from-dir <dir> --libtcc1-dir <dir>
      From a built browser package directory (for example an
      unpacked release tinyc-web.zip): the compiler envelopes are
      copied from that directory verbatim, and the sysroot
      envelopes are generated from its target manifests. The
      per-target libtcc1.a files come from --libtcc1-dir, laid out
      as <target>/libtcc1.a. tinyc-web.js is built from this
      repository's browser sources with esbuild, as in the default
      mode.
"""
import argparse
import base64
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
BROWSER = os.path.join(REPO, "packages", "browser")
ASSETS = os.path.join(HERE, "tinyc_web", "assets")
ESBUILD_VERSION = "0.25.8"

# Where each sysroot source is looked up, in order. The browser
# manifests name sources as include/<name>, tcclib.h and
# lib/<target>/libtcc1.a.
INCLUDE_DIRS = [
    os.path.join(REPO, "wasm-backend", "include"),
    os.path.join(REPO, "vendor-tinycc", "include"),
]


def find_include(name):
    for d in INCLUDE_DIRS:
        p = os.path.join(d, name)
        if os.path.exists(p):
            return p
    return None


def find_libtcc1(target):
    candidates = [
        os.path.join(BROWSER, "build", "wasm32-src", "libtcc1.a")
        if target == "wasm32-wasi" else None,
        os.path.join(REPO, "packages", "python", "tinyc", "sysroot",
                     target, "lib", "tcc", "libtcc1.a"),
        os.path.join(REPO, "wasm-backend", "build-install", target,
                     "lib", "tcc", "libtcc1.a"),
    ]
    for p in candidates:
        if p and os.path.exists(p):
            return p
    return None


def build_js_bundle():
    out = os.path.join(ASSETS, "tinyc-web.js")
    cmd = ["npx", "--yes", f"esbuild@{ESBUILD_VERSION}",
           os.path.join(BROWSER, "web-entry.mjs"),
           "--bundle", "--format=iife", "--global-name=TinycWeb",
           "--platform=browser", f"--outfile={out}"]
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)
    return out


def stage_sysroot(stage, target, manifest, libtcc1_override=None):
    """Copy the manifest's sysroot sources into stage/sysroot-src.
    Returns a list of missing sources (empty when complete)."""
    src_root = os.path.join(stage, "sysroot-src")
    missing = []
    entries = (manifest.get("includeFiles") or []) + \
              (manifest.get("libFiles") or [])
    for entry in entries:
        source = entry["source"]
        if source.startswith("include/"):
            found = find_include(source[len("include/"):])
        elif source == "tcclib.h":
            p = os.path.join(REPO, "vendor-tinycc", "tcclib.h")
            found = p if os.path.exists(p) else None
        elif source == f"lib/{target}/libtcc1.a":
            found = libtcc1_override or find_libtcc1(target)
            if found and not os.path.exists(found):
                found = None
        elif source.startswith(f"lib/{target}/"):
            # The sysroot's link inputs (crt objects, stub
            # libraries, import definitions, the Darwin text
            # stub) are committed under packages/browser/
            # sysroot-lib/<target>/; see that directory's
            # README.md and gen_sysroot_lib.py.
            p = os.path.join(BROWSER, "sysroot-lib", target,
                             source[len(f"lib/{target}/"):])
            found = p if os.path.exists(p) else None
        else:
            found = None
        if not found:
            missing.append(source)
            continue
        dst = os.path.join(src_root, source)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(found, dst)
    return missing


def generate_sysroot_envelope(target, manifest, stage,
                              libtcc1_override=None):
    """Stage the sysroot sources and run the envelope generator.
    The compiler module pair must already be in the stage directory;
    the generator writes the sysroot envelope into ASSETS (and a
    compiler envelope into the stage directory). Returns a list of
    missing sysroot sources, or None on success."""
    missing = stage_sysroot(stage, target, manifest, libtcc1_override)
    if missing:
        return missing
    cmd = ["node", os.path.join(BROWSER, "build-asset-envelopes.mjs"),
           "--target", target,
           "--assets-dir", stage,
           "--sysroot-src", os.path.join(stage, "sysroot-src"),
           "--sysroot-out", ASSETS]
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)
    return None


def load_manifest(manifest_dir, target):
    with open(os.path.join(manifest_dir, f"{target}.json")) as f:
        return json.load(f)


def bundle_target(target):
    """Produce assets/tcc-<target>.tinyc.js and
    assets/sysroot-<target>.tinyc.js from packages/browser.
    Returns True when bundled."""
    glue = os.path.join(BROWSER, f"tcc-{target}.mjs")
    wasm = os.path.join(BROWSER, f"tcc-{target}.wasm")
    if not (os.path.exists(glue) and os.path.exists(wasm)):
        print(f"skip {target}: no compiler module pair in packages/browser")
        return False
    manifest = load_manifest(os.path.join(BROWSER, "sysroot-manifests"),
                             target)
    with tempfile.TemporaryDirectory(prefix="tinyc-web-bundle-") as stage:
        shutil.copyfile(glue, os.path.join(stage, f"tcc-{target}.mjs"))
        shutil.copyfile(wasm, os.path.join(stage, f"tcc-{target}.wasm"))
        missing = generate_sysroot_envelope(target, manifest, stage)
        if missing:
            print(f"skip {target}: sysroot sources missing: "
                  + ", ".join(missing))
            return False
        shutil.copyfile(os.path.join(stage, f"tcc-{target}.tinyc.js"),
                        os.path.join(ASSETS, f"tcc-{target}.tinyc.js"))
    return True


def decode_envelope_pair(envelope_path, stage, target):
    """Write the compiler module pair (tcc-<target>.mjs/.wasm) held
    by a compiler asset envelope into the stage directory."""
    with open(envelope_path) as f:
        text = f.read()
    m = re.search(r"= (\{.*\});?\s*$", text, re.S)
    if not m:
        raise ValueError(f"cannot parse envelope: {envelope_path}")
    payload = json.loads(m.group(1))
    with open(os.path.join(stage, f"tcc-{target}.mjs"), "w") as f:
        f.write(payload["glue"])
    with open(os.path.join(stage, f"tcc-{target}.wasm"), "wb") as f:
        f.write(base64.b64decode(payload["wasm"]))


def bundle_target_from_package(target, pkg_dir, libtcc1_dir):
    """Produce both envelopes for one target of a built browser
    package directory: the compiler envelope is copied from the
    package verbatim, the sysroot envelope is generated from the
    package's manifest with the libtcc1.a held in libtcc1_dir.
    Returns True when bundled."""
    envelope = os.path.join(pkg_dir, f"tcc-{target}.tinyc.js")
    if not os.path.exists(envelope):
        print(f"skip {target}: no compiler envelope in {pkg_dir}")
        return False
    manifest = load_manifest(os.path.join(pkg_dir, "sysroot-manifests"),
                             target)
    libtcc1 = os.path.join(libtcc1_dir, target, "libtcc1.a")
    with tempfile.TemporaryDirectory(prefix="tinyc-web-bundle-") as stage:
        decode_envelope_pair(envelope, stage, target)
        missing = generate_sysroot_envelope(target, manifest, stage,
                                            libtcc1_override=libtcc1)
        if missing:
            print(f"skip {target}: sysroot sources missing: "
                  + ", ".join(missing))
            return False
    shutil.copyfile(envelope, os.path.join(ASSETS,
                                           f"tcc-{target}.tinyc.js"))
    return True


def main():
    parser = argparse.ArgumentParser(
        description="Bundle the tinyc-web build artifacts into "
                    "tinyc_web/assets/.")
    parser.add_argument(
        "--from-dir", metavar="DIR",
        help="take tinyc-web.js, the compiler envelopes and the "
             "target manifests from this built browser package "
             "directory instead of building from packages/browser")
    parser.add_argument(
        "--libtcc1-dir", metavar="DIR",
        help="directory holding one libtcc1.a per target as "
             "<target>/libtcc1.a (required with --from-dir)")
    args = parser.parse_args()
    if args.from_dir and not args.libtcc1_dir:
        parser.error("--from-dir requires --libtcc1-dir")

    os.makedirs(ASSETS, exist_ok=True)
    for name in os.listdir(ASSETS):
        if name.endswith(".tinyc.js"):
            os.remove(os.path.join(ASSETS, name))
    build_js_bundle()
    if args.from_dir:
        manifest_dir = os.path.join(args.from_dir, "sysroot-manifests")
        if not os.path.isdir(manifest_dir):
            manifest_dir = os.path.join(BROWSER, "sysroot-manifests")
    else:
        manifest_dir = os.path.join(BROWSER, "sysroot-manifests")
    manifests = sorted(f[:-5] for f in os.listdir(manifest_dir)
                       if f.endswith(".json"))
    bundled = []
    for target in manifests:
        if args.from_dir:
            ok = bundle_target_from_package(target, args.from_dir,
                                            args.libtcc1_dir)
        else:
            ok = bundle_target(target)
        if ok:
            bundled.append({
                "target": target,
                "compiler": f"tcc-{target}.tinyc.js",
                "sysroot": f"sysroot-{target}.tinyc.js",
            })
    with open(os.path.join(ASSETS, "targets.json"), "w") as f:
        json.dump({"targets": bundled}, f, indent=2)
        f.write("\n")
    print("bundled targets:", ", ".join(t["target"] for t in bundled)
          or "none")
    if not bundled:
        print("error: no target could be bundled", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
