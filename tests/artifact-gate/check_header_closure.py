#!/usr/bin/env python3
"""Header closure audit for the shipped header sets.

Every header a package ships is scanned for the headers it names
in #include / #include_next directives (the union over all
conditional branches: a reference that only one configuration
reaches is still a reference). Each named header must resolve to
one of:

  * another header in the same shipped set (by path relative to
    the including file, by its path in the set, or by basename —
    the include search tries each include directory in turn), or
  * a header registered in header_host_provided.json for that
    scope, which names the provider the package relies on.

An #include_next naming the including header itself is a chain
to the next include directory's copy of the same header, not a
dependency on a different header, and is accepted as such.

Scopes:
  * browser:<target> — the per-target sysroot manifests of the
    browser package. The sysroot is freestanding, so the registry
    carries no host-provided headers for this scope: the set must
    be self-contained.
  * native:<triple> — a native dist's shipped include set: the
    bundled glibc tree (lib/tcc/include-libc, linux-gnu triples;
    the compiler searches it ahead of the builtin set), the
    builtin include directory (lib/tcc/include, plus
    lib/tcc/win32/include for the MinGW triple), and the dist's
    toolchain include directory. Triples whose dist carries no
    C library headers of its own rely on the target platform's,
    and the registry names those headers with their provider.

Usage:
  python3 check_header_closure.py [--registry <json>]
      [--manifests <dir> --include-root <dir> [--include-root <dir>]
       --tcclib <path>]
      [--native-dist <triple>=<dist-dir> ...]

The browser audit runs when --manifests is given; the native
audit runs once per --native-dist. Exit status is nonzero when
any reference is unaccounted for, with each gap named.
"""
import argparse
import json
import os
import posixpath
import re
import sys

INCLUDE_RE = re.compile(r'^\s*#\s*include(_next)?\s*[<"]([^>"]+)[>"]', re.M)

HERE = os.path.dirname(os.path.abspath(__file__))


def resolve_in_set(inc, includer_rel, shipped):
    """True when `inc`, referenced from the shipped file at
    `includer_rel`, names a file of the shipped set."""
    if inc in shipped:
        return True
    rel = posixpath.normpath(posixpath.join(posixpath.dirname(includer_rel), inc))
    if rel in shipped:
        return True
    base = posixpath.basename(inc)
    return any(posixpath.basename(k) == base for k in shipped)


def audit_set(scope, shipped, allowed_external):
    """shipped: {set-relative path: absolute path}. Returns
    (violations, used_external)."""
    violations = []
    used_external = set()
    for rel in sorted(shipped):
        try:
            with open(shipped[rel], encoding="utf-8", errors="replace") as f:
                text = f.read()
        except OSError as e:
            violations.append(f"{scope}: shipped header {rel} cannot be read: {e}")
            continue
        for is_next, inc in INCLUDE_RE.findall(text):
            if is_next and (inc == rel or inc == posixpath.basename(rel)):
                continue  # #include_next chain to the same header
            if resolve_in_set(inc, rel, shipped):
                continue
            if inc in allowed_external:
                used_external.add(inc)
                continue
            violations.append(
                f"{scope}: {rel} includes '{inc}', which is neither in the "
                f"shipped set nor in the host-provided registry")
    return violations, used_external


def browser_sets(manifests_dir, include_roots, tcclib):
    """Yield (target, shipped) per manifest: the set is exactly the
    manifest's includeFiles, contents resolved the way the sysroot
    envelopes resolve them (each --include-root in order, tcclib.h
    from --tcclib)."""
    for fn in sorted(os.listdir(manifests_dir)):
        if not fn.endswith(".json"):
            continue
        with open(os.path.join(manifests_dir, fn), encoding="utf-8") as f:
            manifest = json.load(f)
        shipped = {}
        missing = []
        for entry in manifest.get("includeFiles") or []:
            source = entry["source"]
            found = None
            if source.startswith("include/"):
                name = source[len("include/"):]
                for root in include_roots:
                    p = os.path.join(root, name)
                    if os.path.exists(p):
                        found = p
                        break
            elif source == "tcclib.h":
                if os.path.exists(tcclib):
                    found = tcclib
            if found is None:
                missing.append(source)
            else:
                shipped[posixpath.basename(entry["path"])] = found
        yield manifest["target"], shipped, missing


def native_set(dist_dir):
    """The shipped include set of one native dist: its bundled
    glibc tree (linux-gnu triples), its builtin include
    directories, and its toolchain include directory, in the
    order the compiler searches them."""
    shipped = {}
    roots = []
    for cand in (os.path.join(dist_dir, "lib", "tcc", "include-libc"),
                 os.path.join(dist_dir, "lib", "tcc", "include"),
                 os.path.join(dist_dir, "lib", "tcc", "win32", "include"),
                 os.path.join(dist_dir, "include")):
        if os.path.isdir(cand):
            roots.append(cand)
    for root in roots:
        for dirpath, _dirnames, filenames in os.walk(root):
            for fn in filenames:
                full = os.path.join(dirpath, fn)
                rel = os.path.relpath(full, root).replace(os.sep, "/")
                shipped.setdefault(rel, full)
    return shipped


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--registry", default=os.path.join(HERE, "header_host_provided.json"))
    ap.add_argument("--manifests")
    ap.add_argument("--include-root", action="append", default=[])
    ap.add_argument("--tcclib")
    ap.add_argument("--native-dist", action="append", default=[],
                    metavar="TRIPLE=DIR")
    args = ap.parse_args()

    with open(args.registry, encoding="utf-8") as f:
        registry = json.load(f)["scopes"]

    all_violations = []
    total_sets = 0

    if args.manifests:
        if not args.tcclib:
            ap.error("--tcclib is required with --manifests")
        for target, shipped, missing in browser_sets(
                args.manifests, args.include_root, args.tcclib):
            allowed = {e["header"] for e in registry.get("browser", [])}
            allowed |= {e["header"] for e in registry.get(f"browser:{target}", [])}
            total_sets += 1
            for source in missing:
                all_violations.append(
                    f"browser:{target}: manifest entry '{source}' has no "
                    f"source in the include roots")
            violations, _used = audit_set(f"browser:{target}", shipped, allowed)
            all_violations.extend(violations)
            print(f"browser:{target}: {len(shipped)} shipped headers audited")

    for spec in args.native_dist:
        triple, eq, dist = spec.partition("=")
        if not eq:
            ap.error(f"--native-dist expects TRIPLE=DIR, got: {spec}")
        total_sets += 1
        allowed = {}
        for scope in ("native:*", f"native:{triple}"):
            for e in registry.get(scope, []):
                allowed[e["header"]] = e["provider"]
        shipped = native_set(dist)
        if not shipped:
            all_violations.append(f"native:{triple}: no include set found under {dist}")
            continue
        violations, _used = audit_set(f"native:{triple}", shipped, allowed)
        all_violations.extend(violations)
        print(f"native:{triple}: {len(shipped)} shipped headers audited")

    if not total_sets:
        ap.error("nothing to audit: pass --manifests and/or --native-dist")

    if all_violations:
        for v in all_violations:
            print(f"::error::HEADER CLOSURE GAP: {v}")
        print(f"header closure audit: {len(all_violations)} unaccounted reference(s)")
        return 1
    print("header closure audit: every shipped header reference is in-set or registered")
    return 0


if __name__ == "__main__":
    sys.exit(main())
