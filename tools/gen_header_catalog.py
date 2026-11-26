#!/usr/bin/env python3
"""Generate the TinyC header catalog (tinyc-headers-<version>.md).

The catalog answers one question for people writing code against
these packages: which headers can my program #include, per target?
It is generated from the header sets the release actually ships -
never from a hand-maintained list:

  Browser / wasm compiler targets (tinyc-web, the Pyodide wheel):
  the per-target sysroot manifests in
  packages/browser/sysroot-manifests/<target>.json are the
  authority for which headers each target's sysroot carries.
  A manifest entry's content resolves the same way the sysroot
  envelopes are generated (packages/pyodide/bundle_assets.py):
  include/<name> is looked up in wasm-backend/include first and
  vendor-tinycc/include second, and tcclib.h is
  vendor-tinycc/tcclib.h.

  Native packages: the builtin include directory of each triple's
  dist as it ships - lib/tcc/include, or lib/tcc/win32/include
  for the MinGW triple - plus the dist's toolchain include/
  directory (libtcc.h, tinyc_io.h). The release workflow
  assembles these from the shipped dist tarballs; a local run
  reads build/dist by default.

Every per-header summary is extracted mechanically from the file
itself: its first block comment, its line count, its #define
macro count (function-like macros counted separately - in the
SIMD headers those are the intrinsics), its typedef count and
its #include list. Nothing is written by hand.

Usage:
  python3 tools/gen_header_catalog.py [--repo DIR]
      [--manifests DIR] [--native-root DIR] [--version V]
      [--out FILE]
"""
import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_REPO = os.path.abspath(os.path.join(HERE, ".."))

# Headers whose function-like macros are the intrinsics they ship.
SIMD_HEADER_NAMES = {"arm_neon.h", "riscv_vector.h", "wasm_simd128.h"}

# Common hosted libc headers, used only to state in the overview
# whether any of them are bundled (checked against the actual
# sets, never assumed).
LIBC_HEADER_NAMES = [
    "stdio.h", "stdlib.h", "string.h", "math.h", "ctype.h",
    "errno.h", "assert.h", "time.h", "signal.h", "setjmp.h",
]

DEFINE_RE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)(\()?", re.M)
TYPEDEF_RE = re.compile(r"^\s*typedef\b", re.M)
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)


def is_simd_header(name):
    return name in SIMD_HEADER_NAMES or name.endswith("intrin.h")


def first_block_comment(text, max_lines=80):
    """The file's first /* ... */ comment starting within the
    first max_lines lines, collapsed to one line. None when the
    file opens without one."""
    lines = text.splitlines()
    for i, line in enumerate(lines[:max_lines]):
        start = line.find("/*")
        if start < 0:
            continue
        raw = [line[start + 2:]]
        j = i
        while "*/" not in raw[-1] and j + 1 < len(lines):
            j += 1
            raw.append(lines[j])
        pieces = []
        for seg in raw:
            if "*/" in seg:
                seg = seg.split("*/", 1)[0]
            seg = seg.strip()
            if seg.startswith("*"):
                seg = seg[1:].strip()
            if seg and re.search(r"[A-Za-z0-9]", seg):
                pieces.append(seg)
        body = " ".join(" ".join(pieces).split())
        if not body:
            return None
        if len(body) > 200:
            body = body[:197].rstrip() + "..."
        return body
    return None


def analyze_header(path):
    with open(path, "rb") as f:
        raw = f.read()
    text = raw.decode("utf-8", errors="replace")
    defines = DEFINE_RE.findall(text)
    info = {
        "bytes": len(raw),
        "lines": text.count("\n") + (0 if text.endswith("\n") or not text else 1),
        "comment": first_block_comment(text),
        "macros": len(defines),
        "func_macros": sum(1 for _, paren in defines if paren),
        "typedefs": len(TYPEDEF_RE.findall(text)),
        "includes": INCLUDE_RE.findall(text),
    }
    return info


def _plural(n, word):
    return f"{n} {word}" if n == 1 else f"{n} {word}s"


def summary_text(name, info):
    parts = []
    if info["comment"]:
        parts.append(info["comment"].replace("|", "\\|"))
    stats = [_plural(info["lines"], "line")]
    if is_simd_header(name):
        stats.append(f"{_plural(info['func_macros'], 'intrinsic macro')} "
                     f"({_plural(info['macros'], 'macro')} in total)")
    else:
        stats.append(f"{_plural(info['macros'], 'macro')} "
                     f"({info['func_macros']} function-like)")
    stats.append(_plural(info["typedefs"], "typedef"))
    if info["includes"]:
        incs = info["includes"]
        shown = incs[:8]
        suffix = f", +{len(incs) - 8} more" if len(incs) > 8 else ""
        stats.append("includes " + ", ".join(shown) + suffix)
    parts.append(" · ".join(stats))
    return " — ".join(parts) if info["comment"] else " · ".join(stats)


def header_table(rows):
    """rows: list of (name, origin, info)."""
    out = ["| Header | Source | Summary |", "| --- | --- | --- |"]
    for name, origin, info in rows:
        out.append(f"| `{name}` | {origin} | {summary_text(name, info)} |")
    return out


# ----------------------------------------------------------------
# Browser / wasm sysroot sets, from the per-target manifests.
# ----------------------------------------------------------------

def resolve_manifest_source(source, repo):
    """Resolve one sysroot manifest source to (path, origin label),
    with the lookup order the sysroot envelopes are generated by."""
    if source.startswith("include/"):
        name = source[len("include/"):]
        for sub in ("wasm-backend/include", "vendor-tinycc/include"):
            p = os.path.join(repo, sub, name)
            if os.path.exists(p):
                return p, f"`{sub}/`"
        return None, None
    if source == "tcclib.h":
        p = os.path.join(repo, "vendor-tinycc", "tcclib.h")
        if os.path.exists(p):
            return p, "`vendor-tinycc/`"
    return None, None


def browser_sections(repo, manifests_dir):
    targets = sorted(f[:-5] for f in os.listdir(manifests_dir)
                     if f.endswith(".json"))
    if not targets:
        raise SystemExit(f"no sysroot manifests in {manifests_dir}")
    sections = []
    sets = {}
    for target in targets:
        with open(os.path.join(manifests_dir, f"{target}.json")) as f:
            manifest = json.load(f)
        rows = []
        names = []
        for entry in manifest.get("includeFiles") or []:
            path, origin = resolve_manifest_source(entry["source"], repo)
            if path is None:
                raise SystemExit(
                    f"{target}: sysroot source not found: {entry['source']}")
            name = os.path.basename(entry["path"])
            names.append(name)
            rows.append((name, origin, analyze_header(path)))
        sets[target] = names
        sections.append((target, rows))
    return sections, sets


# ----------------------------------------------------------------
# Native dist sets.
# ----------------------------------------------------------------

def native_sections(repo, native_root):
    vendor_names = set(os.listdir(os.path.join(repo, "vendor-tinycc",
                                               "include")))
    tcc_own = vendor_names | {"tcclib.h"}
    triples = sorted(d for d in os.listdir(native_root)
                     if os.path.isdir(os.path.join(native_root, d)))
    sections = []
    for triple in triples:
        dist = os.path.join(native_root, triple)
        win32_inc = os.path.join(dist, "lib", "tcc", "win32", "include")
        plain_inc = os.path.join(dist, "lib", "tcc", "include")
        section = {"triple": triple}
        if os.path.isdir(win32_inc):
            section["layout"] = "lib/tcc/win32/include"
            rows = []
            tree_files = []
            for dirpath, dirnames, filenames in os.walk(win32_inc):
                dirnames.sort()
                for fn in sorted(filenames):
                    full = os.path.join(dirpath, fn)
                    rel = os.path.relpath(full, win32_inc)
                    if os.path.dirname(rel) == "" and fn in tcc_own:
                        rows.append((fn, analyze_header(full)))
                    else:
                        tree_files.append(rel)
            section["rows"] = rows
            by_dir = {}
            for rel in tree_files:
                d = os.path.dirname(rel) or "."
                by_dir[d] = by_dir.get(d, 0) + 1
            section["tree"] = {"total": len(tree_files), "by_dir": by_dir}
        elif os.path.isdir(plain_inc):
            section["layout"] = "lib/tcc/include"
            rows = []
            for fn in sorted(os.listdir(plain_inc)):
                if fn.endswith(".h"):
                    rows.append((fn, analyze_header(
                        os.path.join(plain_inc, fn))))
            section["rows"] = rows
            section["tree"] = None
        else:
            raise SystemExit(
                f"{triple}: no lib/tcc/include or lib/tcc/win32/include "
                f"under {dist}")
        toolchain = os.path.join(dist, "include")
        tc_rows = []
        if os.path.isdir(toolchain):
            for fn in sorted(os.listdir(toolchain)):
                if fn.endswith(".h"):
                    tc_rows.append((fn, analyze_header(
                        os.path.join(toolchain, fn))))
        section["toolchain_rows"] = tc_rows
        sections.append(section)
    return sections


# ----------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Generate the TinyC header catalog from the "
                    "shipped header sets.")
    parser.add_argument("--repo", default=DEFAULT_REPO,
                        help="repository root (default: this "
                             "script's repo)")
    parser.add_argument("--manifests",
                        help="browser sysroot manifests directory "
                             "(default: <repo>/packages/browser/"
                             "sysroot-manifests)")
    parser.add_argument("--native-root",
                        help="assembled native dists, one directory "
                             "per triple in dist layout (default: "
                             "<repo>/build/dist)")
    parser.add_argument("--version",
                        help="version string for the title "
                             "(default: packages/npm/package.json)")
    parser.add_argument("--out", help="output file (default: stdout)")
    args = parser.parse_args()

    repo = os.path.abspath(args.repo)
    manifests_dir = args.manifests or os.path.join(
        repo, "packages", "browser", "sysroot-manifests")
    native_root = args.native_root or os.path.join(repo, "build", "dist")
    version = args.version
    if not version:
        with open(os.path.join(repo, "packages", "npm",
                               "package.json")) as f:
            version = json.load(f)["version"]
    if not os.path.isdir(native_root):
        raise SystemExit(f"native dist root not found: {native_root}")

    b_sections, b_sets = browser_sections(repo, manifests_dir)
    n_sections = native_sections(repo, native_root)

    # Facts the overview states, computed from the actual sets.
    browser_sets = list(b_sets.values())
    browser_uniform = all(s == browser_sets[0] for s in browser_sets)
    bundled_names = set(browser_sets[0]) if browser_uniform else set()
    for s in browser_sets:
        bundled_names |= set(s)
    native_names = set()
    for sec in n_sections:
        native_names |= {name for name, _ in sec["rows"]}
    libc_bundled = sorted(n for n in LIBC_HEADER_NAMES
                          if n in bundled_names or n in native_names)

    out = []
    out.append(f"# TinyC header catalog — {version}")
    out.append("")
    out.append("Generated by `tools/gen_header_catalog.py` from the "
               "header sets this release ships: the browser sysroot "
               "manifests and the native dists' builtin include "
               "directories. It lists, per target, exactly the "
               "headers a program compiled with this release's TCC "
               "can `#include` from the package itself.")
    out.append("")
    out.append("## Overview")
    out.append("")
    if browser_uniform:
        out.append(f"All {len(b_sections)} browser/wasm targets ship "
                   f"the same sysroot header set: the "
                   f"{len(browser_sets[0])} headers listed in each "
                   "browser section below. That set is the whole "
                   "sysroot - the environment is freestanding, and "
                   "no libc headers are bundled with it.")
    else:
        counts = ", ".join(f"{t}: {len(s)}"
                           for t, s in sorted(b_sets.items()))
        out.append("The browser/wasm targets' sysroot header sets "
                   f"differ per target ({counts}); each browser "
                   "section below lists its target's whole set. "
                   "The environment is freestanding, and no libc "
                   "headers are bundled with it.")
    out.append("")
    out.append("The native packages bundle the builtin header set "
               "listed in each native section below (TCC's own "
               "headers, including the SIMD intrinsic headers) "
               "plus, per package, the toolchain headers in its "
               "`include/` directory. Hosted libc headers are not "
               "part of the native packages: a native target's "
               "system headers come from the target platform "
               "itself. The one exception is the MinGW package, "
               "which bundles the win32 header tree summarized in "
               "its section.")
    if libc_bundled:
        out.append("")
        out.append("Of the common libc headers, only these appear "
                   "in the bundled builtin sets: "
                   + ", ".join(f"`{n}`" for n in libc_bundled) + ".")
    out.append("")
    out.append("Headers with the same name in different sections "
               "are the files those sections' packages actually "
               "ship; where their contents differ, each section's "
               "summary is generated from its own file.")
    out.append("")
    out.append("## Browser / wasm targets (tinyc-web sysroots)")
    out.append("")
    out.append("The sysroot of the compiler the browser package "
               "runs for each target, as listed by that target's "
               "sysroot manifest. Source names the repository tree "
               "each shipped file is generated from.")
    for target, rows in b_sections:
        out.append("")
        out.append(f"### {target}")
        out.append("")
        out.extend(header_table(rows))
    out.append("")
    out.append("## Native packages")
    out.append("")
    out.append("The builtin include directory of each triple's "
               "dist, as shipped in the release tarballs.")
    for sec in n_sections:
        out.append("")
        out.append(f"### {sec['triple']}")
        out.append("")
        out.append(f"Builtin headers, in `{sec['layout']}`:")
        out.append("")
        rows = [(name, "shipped dist", info)
                for name, info in sec["rows"]]
        out.extend(header_table(rows))
        if sec["tree"]:
            tree = sec["tree"]
            out.append("")
            out.append(f"The same directory also carries the "
                       f"bundled win32 header tree: {tree['total']} "
                       "further headers, not listed individually. "
                       "Files per directory:")
            out.append("")
            for d in sorted(tree["by_dir"]):
                label = "(top level)" if d == "." else f"`{d}/`"
                out.append(f"- {label}: {tree['by_dir'][d]}")
        if sec["toolchain_rows"]:
            out.append("")
            out.append("Toolchain headers, in the dist's `include/` "
                       "(the embedding API surface, for hosts "
                       "driving the compiler):")
            out.append("")
            rows = [(name, "shipped dist", info)
                    for name, info in sec["toolchain_rows"]]
            out.extend(header_table(rows))
    out.append("")

    text = "\n".join(out)
    if args.out:
        with open(args.out, "w") as f:
            f.write(text)
        print(f"wrote {args.out}: {len(b_sections)} browser targets, "
              f"{len(n_sections)} native triples", file=sys.stderr)
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
