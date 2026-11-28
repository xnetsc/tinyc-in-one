#!/usr/bin/env node
/* Regenerate the per-target sysroot manifests
 * (sysroot-manifests/<target>.json) for the eight native
 * targets from the same sources the native dist is assembled
 * from, so the browser sysroot cannot drift from the dist
 * sysroot:
 *
 *   - includeFiles: the shared TCC builtin set (TCC's own
 *     headers from vendor-tinycc/include and
 *     wasm-backend/include, plus tcclib.h) overlaid by the
 *     target's libc header tree.
 *     The overlay wins on a shared path, mirroring the dist's
 *     search order (include-libc before include). The tree is
 *     the one build.sh copies into the dist's include-libc:
 *       *-linux-gnu     vendor-glibc/<target>/include
 *       *-linux-android vendor-android/include
 *       *-apple-darwin  vendor-darwin/include
 *       x86_64-windows  vendor-tinycc/win32/include
 *     The manifest records the tree's repo-relative path in
 *     libcIncludeRoot; the envelope builders resolve an
 *     include/<name> entry in the target's libc root first,
 *     then wasm-backend/include, then vendor-tinycc/include.
 *
 *   - libFiles: libtcc1.a (resolved from the dist build at
 *     envelope time, as before) plus every file of
 *     sysroot-lib/<target>/ -- the generated link inputs whose
 *     own generator (gen_sysroot_lib.py) draws them from the
 *     same platform sources the dist bundles.
 *
 * The wasm32-wasi manifest is not touched: its sysroot is the
 * wasm backend's own header set, not a native target's.
 *
 * Usage: node gen-sysroot-manifests.mjs [--check]
 *   --check: fail if a committed manifest differs from the
 *   generated one, without writing.
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, '..', '..');
const manifestsDir = path.join(here, 'sysroot-manifests');
const check = process.argv.includes('--check');

const LIBC_INCLUDE_ROOTS = {
  'x86_64-linux-gnu': 'vendor-glibc/x86_64-linux-gnu/include',
  'aarch64-linux-gnu': 'vendor-glibc/aarch64-linux-gnu/include',
  'riscv64-linux-gnu': 'vendor-glibc/riscv64-linux-gnu/include',
  'aarch64-linux-android': 'vendor-android/include',
  'x86_64-linux-android': 'vendor-android/include',
  'arm64-apple-darwin': 'vendor-darwin/include',
  'x86_64-apple-darwin': 'vendor-darwin/include',
  'x86_64-windows': 'vendor-tinycc/win32/include',
};

function walk(dir, base = dir, out = []) {
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    const p = path.join(dir, e.name);
    if (e.isDirectory()) walk(p, base, out);
    else if (e.isFile()) out.push(path.relative(base, p).split(path.sep).join('/'));
  }
  return out;
}

function loadManifest(target) {
  return JSON.parse(fs.readFileSync(path.join(manifestsDir, `${target}.json`), 'utf8'));
}

/* The shared builtin set: TCC's own headers -- every file of
 * vendor-tinycc/include (the directory the native dist ships
 * as its include/) plus tcclib.h -- and iso646.h, a C standard
 * header the libc trees below mostly do not carry (the native
 * dist lacks it for the same reason; closing that gap on the
 * native side is the standard-header work's, the browser set
 * keeps the header so this host is not the one missing it).
 * Their sources resolve in the target's libc root and the two
 * builtin roots; the order is each manifest's own (see
 * builtinHeadersFirst in gen-sysroot-manifests.mjs consumers):
 * the libc root first, then vendor-tinycc/include, then
 * wasm-backend/include -- except the Android targets, whose
 * dist compilers search the builtin directory before the libc
 * tree, so their manifests set builtinHeadersFirst and
 * vendor-tinycc/include goes first (its stdatomic.h is the one
 * name both trees carry with different content). limits.h,
 * math.h and stdint.h are not listed here: every libc tree
 * carries its own, and the overlay below supplies them. */
const BUILTIN_INCLUDE_FILES = [
  ...fs.readdirSync(path.join(repo, 'vendor-tinycc', 'include')).sort(),
  'iso646.h',
];
const builtinEntries = [
  ...new Set(BUILTIN_INCLUDE_FILES)].map((n) => ({
    path: `/sysroot/include/${n}`, source: `include/${n}`,
  })).concat([
  { path: '/sysroot/include/tcclib.h', source: 'tcclib.h' },
]);

let failed = false;
for (const [target, root] of Object.entries(LIBC_INCLUDE_ROOTS)) {
  const manifest = loadManifest(target);
  const absRoot = path.join(repo, root);
  if (!fs.existsSync(absRoot)) {
    console.error(`${target}: libc include root missing: ${root}`);
    failed = true;
    continue;
  }
  const libcPaths = new Set(walk(absRoot));
  const byPath = new Map();
  for (const e of builtinEntries) byPath.set(e.path, e);
  for (const rel of [...libcPaths].sort()) {
    byPath.set(`/sysroot/include/${rel}`, {
      path: `/sysroot/include/${rel}`,
      source: `include/${rel}`,
    });
  }
  manifest.includeFiles = [...byPath.values()].sort(
    (a, b) => (a.path < b.path ? -1 : a.path > b.path ? 1 : 0));
  manifest.libcIncludeRoot = root;
  /* The Android dist compilers bake their include search as
   * builtin directory, libc tree, per-triple libc subtree; the
   * other targets' put the libc tree first. The flag records
   * the Android order in the manifest; the envelope builders
   * read it when they resolve an include/<name> entry. */
  if (target.endsWith('-linux-android')) manifest.builtinHeadersFirst = true;
  else delete manifest.builtinHeadersFirst;
  /* The bionic tree keeps the kernel asm headers in a
   * per-triple subtree (<triple>/asm/...), which the Android
   * dist compilers carry as a third search root
   * (include-libc/<triple>); the manifest records the matching
   * sysroot directory as an extra include dir for the glue to
   * register next to includeDir. */
  if (target.endsWith('-linux-android')) {
    manifest.extraIncludeDirs = [`/sysroot/include/${target}`];
  } else delete manifest.extraIncludeDirs;

  const sysrootLibDir = path.join(here, 'sysroot-lib', target);
  const libNames = fs.readdirSync(sysrootLibDir).sort();
  manifest.libFiles = [
    { binary: true, path: '/sysroot/lib/libtcc1.a', source: `lib/${target}/libtcc1.a` },
    ...libNames.map((n) => ({
      binary: true, path: `/sysroot/lib/${n}`, source: `lib/${target}/${n}`,
    })),
  ];

  const text = JSON.stringify(manifest, null, 2) + '\n';
  const file = path.join(manifestsDir, `${target}.json`);
  if (check) {
    if (fs.readFileSync(file, 'utf8') !== text) {
      console.error(`${target}: manifest differs from its generated form`);
      failed = true;
    }
  } else {
    fs.writeFileSync(file, text);
    console.log(`${target}: ${manifest.includeFiles.length} include files, ` +
      `${manifest.libFiles.length} lib files, libc root ${root}`);
  }
}
process.exit(failed ? 1 : 0);
