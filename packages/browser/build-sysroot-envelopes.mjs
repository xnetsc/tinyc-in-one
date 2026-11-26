#!/usr/bin/env node
/* Build the sysroot asset envelopes the package ships.
 *
 * loadSysroot reads one envelope per target, sysroot-<target>.tinyc.js,
 * from the package's sysroot/ directory (see index.mjs: the sysroot
 * base URL defaults to ./sysroot/ next to the compiler envelopes).
 * This script produces those envelopes: for each target it stages
 * the sources its sysroot manifest (sysroot-manifests/<target>.json)
 * lists into build/sysroot-stage/<target>/sysroot-src — laid out by
 * manifest source, the staging layout bundle_assets.py uses for the
 * Pyodide package — and runs build-asset-envelopes.mjs, the one
 * envelope generator, to write sysroot/sysroot-<target>.tinyc.js.
 *
 * Source resolution is the package's canonical one, shared with
 * packages/pyodide/bundle_assets.py and the artifact gate's
 * assemble_sysroot.mjs: an include/<name> entry is looked up in
 * ../../wasm-backend/include first, then ../../vendor-tinycc/include;
 * tcclib.h comes from ../../vendor-tinycc/tcclib.h. The libtcc1.a
 * entry, lib/<target>/libtcc1.a, resolves through --lib or the
 * candidate build outputs, in order: the wasm32 browser build's own
 * native make (build/wasm32-src, wasm32-wasi only), the repository
 * dist build (../../build/dist), the Python package's staged sysroot
 * (../../packages/python/tinyc/sysroot), and the wasm-backend install
 * tree (../../wasm-backend/build-install). The dist triple for
 * x86_64-windows is x86_64-w64-mingw32, whose libtcc1.a sits under
 * lib/tcc/win32/lib. A manifest's remaining lib/<target>/ entries
 * (crt objects, stub libraries, .def files, the darwin .tbd) are
 * committed sources under ./sysroot-lib/<target>/, resolved the
 * same way; a missing one fails like a missing header.
 *
 * The generator is invoked with --sysroot-only: this script's job
 * is the sysroot envelope alone. The compiler envelope is built
 * by build.sh (--compiler-only) before this runs, and callers
 * like the release job's macOS completion step have no compiler
 * module pair at all — the release job works from a fresh
 * checkout, where tcc-<target>.mjs/.wasm do not exist.
 *
 * A header source that cannot be found fails the build, named: a
 * sysroot envelope with a header missing is a broken package. A
 * libtcc1.a that cannot be found is reported by target and the
 * envelope is written with its headers only — the dist for the
 * macOS targets is built on macOS hosts, so a Linux build of this
 * package has no local libtcc1.a for them; --require-libs turns
 * that report into a failure for contexts that must ship complete
 * envelopes.
 *
 * Usage:
 *   node build-sysroot-envelopes.mjs [--target <target>]...
 *     [--lib <target>=<path>]... [--require-libs]
 * Without --target, every target in sysroot-manifests/ is built.
 * build.sh runs this for its target after the compiler envelope.
 */
import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, '..', '..');

function parseArgs(argv) {
  const out = { targets: [], libs: new Map(), requireLibs: false };
  for (let i = 0; i < argv.length; i += 1) {
    const a = argv[i];
    const next = () => {
      i += 1;
      if (i >= argv.length) throw new Error(`${a} needs a value`);
      return argv[i];
    };
    if (a === '--target') out.targets.push(next());
    else if (a === '--lib') {
      const v = next();
      const eq = v.indexOf('=');
      if (eq < 0) throw new Error(`--lib expects <target>=<path>, got: ${v}`);
      out.libs.set(v.slice(0, eq), v.slice(eq + 1));
    } else if (a === '--require-libs') out.requireLibs = true;
    else throw new Error(`unknown argument: ${a}`);
  }
  return out;
}

const args = parseArgs(process.argv.slice(2));

const manifestsDir = path.join(here, 'sysroot-manifests');
const allTargets = fs.readdirSync(manifestsDir)
  .filter((f) => f.endsWith('.json'))
  .map((f) => f.slice(0, -'.json'.length))
  .sort();
const targets = args.targets.length ? args.targets : allTargets;

const includeRoots = [
  path.join(repo, 'wasm-backend', 'include'),
  path.join(repo, 'vendor-tinycc', 'include'),
];
const tcclibPath = path.join(repo, 'vendor-tinycc', 'tcclib.h');

function distTriple(target) {
  return target === 'x86_64-windows' ? 'x86_64-w64-mingw32' : target;
}

function findInclude(name) {
  for (const root of includeRoots) {
    const p = path.join(root, name);
    if (fs.existsSync(p)) return p;
  }
  return null;
}

function findLibtcc1(target) {
  const dt = distTriple(target);
  const win32 = target === 'x86_64-windows';
  const candidates = [];
  if (args.libs.has(target)) candidates.push(args.libs.get(target));
  if (target === 'wasm32-wasi') {
    candidates.push(path.join(here, 'build', 'wasm32-src', 'libtcc1.a'));
  }
  candidates.push(path.join(
    repo, 'build', 'dist', dt, 'lib', 'tcc',
    ...(win32 ? ['win32', 'lib'] : []), 'libtcc1.a'));
  candidates.push(path.join(
    repo, 'packages', 'python', 'tinyc', 'sysroot', dt, 'lib', 'tcc',
    ...(win32 ? ['win32', 'lib'] : []), 'libtcc1.a'));
  candidates.push(path.join(
    repo, 'wasm-backend', 'build-install', target, 'lib', 'tcc', 'libtcc1.a'));
  for (const p of candidates) {
    if (p && fs.existsSync(p)) return p;
  }
  return null;
}

const sysrootOut = path.join(here, 'sysroot');
fs.mkdirSync(sysrootOut, { recursive: true });

const problems = [];
const libNotices = [];

for (const target of targets) {
  const manifestPath = path.join(manifestsDir, `${target}.json`);
  if (!fs.existsSync(manifestPath)) {
    problems.push(`${target}: sysroot manifest not found: ${manifestPath}`);
    continue;
  }
  const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
  const stage = path.join(here, 'build', 'sysroot-stage', target);
  const srcRoot = path.join(stage, 'sysroot-src');
  fs.rmSync(stage, { recursive: true, force: true });
  fs.mkdirSync(srcRoot, { recursive: true });

  const entries = [
    ...(manifest.includeFiles ?? []),
    ...(manifest.libFiles ?? []),
  ];
  let staged = true;
  for (const entry of entries) {
    let src = null;
    if (entry.source.startsWith('include/')) {
      src = findInclude(entry.source.slice('include/'.length));
    } else if (entry.source === 'tcclib.h') {
      if (fs.existsSync(tcclibPath)) src = tcclibPath;
    } else if (entry.source === `lib/${target}/libtcc1.a`) {
      src = findLibtcc1(target);
      if (!src) {
        libNotices.push(
          `${target}: libtcc1.a not found in any candidate location; ` +
          'the sysroot envelope carries its headers only');
        continue;
      }
    } else if (entry.source.startsWith(`lib/${target}/`)) {
      // The target's remaining link inputs (crt objects, stub
      // libraries, .def files, the darwin .tbd) are committed
      // sources under sysroot-lib/<target>/ — the same
      // resolution bundle_assets.py uses for the Pyodide
      // package. A missing one is a broken tree, not an
      // omission: it fails below like a missing header.
      const p = path.join(here, 'sysroot-lib', target,
        entry.source.slice(`lib/${target}/`.length));
      if (fs.existsSync(p)) src = p;
    }
    if (!src) {
      problems.push(`${target}: sysroot source not found for manifest entry "${entry.source}"`);
      staged = false;
      continue;
    }
    const dst = path.join(srcRoot, entry.source);
    fs.mkdirSync(path.dirname(dst), { recursive: true });
    fs.copyFileSync(src, dst);
  }
  if (!staged) continue;

  const gen = spawnSync(process.execPath, [
    path.join(here, 'build-asset-envelopes.mjs'),
    '--target', target,
    '--sysroot-only',
    '--assets-dir', here,
    '--sysroot-src', srcRoot,
    '--sysroot-out', sysrootOut,
  ], { stdio: 'inherit' });
  if (gen.status !== 0) {
    problems.push(`${target}: build-asset-envelopes.mjs failed (exit ${gen.status})`);
  }
}

for (const n of libNotices) console.log(`notice: ${n}`);
if (args.requireLibs && libNotices.length) {
  for (const n of libNotices) problems.push(n.replace('not found', 'required but not found'));
}
if (problems.length) {
  for (const p of problems) console.log(`::error::SYSROOT ENVELOPE GAP: ${p}`);
  process.exit(1);
}
console.log(`sysroot envelopes built for: ${targets.join(', ')}`);
