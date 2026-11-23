#!/usr/bin/env python3
"""Output-format consistency check.

Rule: for any given target, the set of output file formats its TCC
variant can generate is identical in every host build and every
package. The single source of truth is the "outputs" field of the
target's dist manifest (build/dist/<target>/manifest.json); every
other manifest copies the value verbatim, and package loaders read
it from their manifest instead of hardcoding a format table.

For each target triple this script compares "outputs" in:
  - build/dist/<target>/manifest.json            (the authority)
  - packages/browser/sysroot-manifests/<target>.json
        (only when the browser package carries that target;
         its x86_64-windows is the alias of x86_64-w64-mingw32)
  - packages/npm/packaging-manifest.json
  - packages/npm/native-packages/@xnetx/tinyc-native-*/packaging-manifest.json
  - packages/python/tinyc/packaging-manifest.json
  - packages/java/dist/tinyc.jar  (its packaging-manifest.json)

A location that carries the target but has a missing or different
"outputs" fails the check and is named. A target a package manifest
carries that dist does not have (stale staging) also fails.
Exit code 0 means every comparison was identical.
"""
import glob
import json
import os
import sys
import zipfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

BROWSER_ALIAS = {"x86_64-w64-mingw32": "x86_64-windows"}

failures = []
notes = []


def canon(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=False)


def fail(where, target, detail):
    failures.append(f"{where} [{target}]: {detail}")


def load_json(path):
    with open(path) as f:
        return json.load(f)


def dist_outputs():
    """target -> outputs value (or None when the field is absent)."""
    out = {}
    dist = os.path.join(ROOT, "build", "dist")
    for target in sorted(os.listdir(dist)):
        mf = os.path.join(dist, target, "manifest.json")
        if not os.path.isdir(os.path.join(dist, target)):
            continue
        if not os.path.exists(mf):
            fail("dist", target, f"manifest missing: {mf}")
            continue
        out[target] = load_json(mf).get("outputs")
    return out


def check_package_manifest(where, path, authority):
    if not os.path.exists(path):
        fail(where, "-", f"manifest missing: {path}")
        return
    manifest = load_json(path)
    seen = {}
    for combo in manifest.get("combinations", []):
        target = combo.get("target")
        if target not in authority:
            fail(where, target, "combination staged but unknown to dist "
                                "(stale staging?)")
            continue
        seen.setdefault(target, set()).add(canon(combo.get("outputs")))
    for target, expected in authority.items():
        values = seen.get(target)
        if not values:
            fail(where, target, "no combination carries this target")
        elif len(values) > 1:
            fail(where, target, "combinations disagree on outputs: "
                                + " vs ".join(sorted(values)))
        elif expected is not None and values.pop() != canon(expected):
            fail(where, target, "outputs differ from dist manifest")


def check_browser(authority):
    bdir = os.path.join(ROOT, "packages", "browser", "sysroot-manifests")
    for target, expected in authority.items():
        name = BROWSER_ALIAS.get(target, target)
        path = os.path.join(bdir, name + ".json")
        if not os.path.exists(path):
            notes.append(f"browser [{target}]: no sysroot manifest "
                         f"({name}.json) - target not carried, skipped")
            continue
        actual = load_json(path).get("outputs")
        if expected is None:
            continue  # dist failure already recorded; nothing to compare
        if actual is None:
            fail("browser", target, f"outputs missing in {path}")
        elif canon(actual) != canon(expected):
            fail("browser", target,
                 f"outputs differ from dist manifest: {canon(actual)} "
                 f"!= {canon(expected)}")


def check_java_jar(authority):
    jar = os.path.join(ROOT, "packages", "java", "dist", "tinyc.jar")
    if not os.path.exists(jar):
        fail("java", "-", f"jar missing: {jar} (run packages/java/build.sh)")
        return
    with zipfile.ZipFile(jar) as z:
        try:
            manifest = json.loads(z.read("packaging-manifest.json"))
        except KeyError:
            fail("java", "-", "packaging-manifest.json missing in jar")
            return
    tmp = os.path.join(ROOT, "packages", "java", "build", ".jar-manifest-check.json")
    with open(tmp, "w") as f:
        json.dump(manifest, f)
    try:
        check_package_manifest("java", tmp, authority)
    finally:
        os.remove(tmp)


def main():
    authority = dist_outputs()
    if not authority:
        print("FAIL: no dist manifests found under build/dist")
        return 1
    for target, value in authority.items():
        if value is None:
            fail("dist", target,
                 "outputs field missing in build/dist manifest "
                 "(the matrix build defines it; it is the authority)")

    check_browser(authority)
    check_package_manifest(
        "npm", os.path.join(ROOT, "packages", "npm", "packaging-manifest.json"),
        authority)
    for sub in sorted(glob.glob(os.path.join(
            ROOT, "packages", "npm", "native-packages", "@xnetx", "*",
            "packaging-manifest.json"))):
        check_package_manifest("npm-subpackage", sub, authority)
    check_package_manifest(
        "python", os.path.join(ROOT, "packages", "python", "tinyc",
                               "packaging-manifest.json"), authority)
    check_java_jar(authority)

    for target in sorted(authority):
        state = "OK" if not any(f"[{target}]" in x for x in failures) else "FAIL"
        print(f"{state}: {target} outputs = {canon(authority[target])}")
    for n in notes:
        print("NOTE:", n)
    if failures:
        print("\nOUTPUTS CONSISTENCY FAILURES:")
        for f in failures:
            print("  FAIL:", f)
        return 1
    print("\noutputs consistent for all targets across dist, browser, "
          "npm, python, java")
    return 0


if __name__ == "__main__":
    sys.exit(main())
