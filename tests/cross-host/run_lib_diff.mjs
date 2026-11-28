#!/usr/bin/env node
/* Cross-host library- and header-layer comparison.
 *
 * Companion to run_diff.mjs (which compares compiled
 * products). This script compares the inputs every host
 * package carries for the same target:
 *
 *   libs     the link inputs (libtcc1.a, crt objects, stub
 *            libraries, .def files, .tbd stubs, default-lib
 *            archives) in the native dist, the browser
 *            sysroot envelope, the stage_layout host packages
 *            and the Android AAR assets -- byte for byte.
 *   headers  the header file sets (names + hashes) of the
 *            same sources: the dist's merged include view
 *            (include-libc taking precedence over include,
 *            the order the native compiler searches) versus
 *            the browser sysroot envelope's include set.
 *   ar       every .a in the dist is parsed for archive
 *            member header timestamps: a nonzero member
 *            mtime is a host/time-dependent byte inside a
 *            shipped library file, reported with its file.
 *
 * A file present in one source and absent in another is a
 * finding, reported as such -- set equality is part of the
 * comparison, not an assumption.
 *
 * Usage:
 *   node run_lib_diff.mjs [--targets a,b,...]
 *     [--stage-root <dir>] [--skip-stage] [--skip-aar]
 *     [--json <path>]
 * Exit status: 0 when all compared sets are byte-identical,
 * 1 on any difference, 2 on setup errors.
 */
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import vm from 'node:vm';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, '..', '..');
const pkg = path.join(repo, 'packages', 'browser');

const DIST_TRIPLE = {
  'x86_64-linux-gnu': 'x86_64-linux-gnu',
  'aarch64-linux-gnu': 'aarch64-linux-gnu',
  'riscv64-linux-gnu': 'riscv64-linux-gnu',
  'x86_64-windows': 'x86_64-w64-mingw32',
  'aarch64-linux-android': 'aarch64-linux-android',
  'x86_64-linux-android': 'x86_64-linux-android',
  'x86_64-apple-darwin': 'x86_64-apple-darwin',
  'arm64-apple-darwin': 'arm64-apple-darwin',
  'wasm32-wasi': 'wasm32-wasi',
};
const ALL_TARGETS = Object.keys(DIST_TRIPLE);

function parseArgs(argv) {
  const out = { targets: ALL_TARGETS, stage: true, aar: true, json: null, stageRoot: path.join(repo, 'build', 'cross-host-stage') };
  for (let i = 0; i < argv.length; i += 1) {
    const a = argv[i];
    const next = () => { i += 1; if (i >= argv.length) throw new Error(`${a} needs a value`); return argv[i]; };
    if (a === '--targets') out.targets = next().split(',');
    else if (a === '--skip-stage') out.stage = false;
    else if (a === '--skip-aar') out.aar = false;
    else if (a === '--stage-root') out.stageRoot = path.resolve(next());
    else if (a === '--json') out.json = path.resolve(next());
    else throw new Error(`unknown argument: ${a}`);
  }
  return out;
}
const args = parseArgs(process.argv.slice(2));

const sha = (buf) => crypto.createHash('sha256').update(buf).digest('hex');

function walk(dir, base = dir, out = new Map()) {
  if (!fs.existsSync(dir)) return out;
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    const p = path.join(dir, e.name);
    if (e.isDirectory()) walk(p, base, out);
    else if (e.isFile()) out.set(path.relative(base, p), fs.readFileSync(p));
  }
  return out;
}

/* basename -> {hash, size, where} with collision detection */
function byBasename(files, where) {
  const m = new Map();
  for (const [rel, buf] of files) {
    const b = path.basename(rel);
    if (m.has(b)) m.get(b).push({ rel, hash: sha(buf), size: buf.length, where });
    else m.set(b, [{ rel, hash: sha(buf), size: buf.length, where }]);
  }
  return m;
}

/* ---------------- sources ---------------- */

function distLibs(target) {
  const root = path.join(repo, 'build', 'dist', DIST_TRIPLE[target], 'lib');
  const all = walk(root);
  const libs = new Map();
  for (const [rel, buf] of all) {
    if (rel.startsWith('tcc' + path.sep + 'include' + path.sep) ||
        rel.startsWith('tcc' + path.sep + 'include-libc' + path.sep) ||
        rel.startsWith('tcc' + path.sep + 'win32' + path.sep + 'include' + path.sep)) continue;
    libs.set(rel, buf);
  }
  return libs;
}

function distHeaders(target) {
  /* The native compiler's merged include view: include-libc
   * is searched before include, so on a name collision the
   * libc tree's file is the one a compilation sees. The dist's
   * top-level include/ (libtcc.h, tinyc_io.h -- the embedder
   * headers stage_layout copies into packages alongside the
   * sysroot) fills any remaining gaps. */
  const root = path.join(repo, 'build', 'dist', DIST_TRIPLE[target], 'lib', 'tcc');
  const merged = new Map();
  for (const [rel, buf] of walk(path.join(root, 'include'))) merged.set(rel, buf);
  for (const [rel, buf] of walk(path.join(root, 'include-libc'))) merged.set(rel, buf);
  const win32inc = path.join(root, 'win32', 'include');
  if (fs.existsSync(win32inc)) for (const [rel, buf] of walk(win32inc)) if (!merged.has(rel)) merged.set(rel, buf);
  const topinc = path.join(repo, 'build', 'dist', DIST_TRIPLE[target], 'include');
  if (fs.existsSync(topinc)) for (const [rel, buf] of walk(topinc)) if (!merged.has(rel)) merged.set(rel, buf);
  return merged;
}

function envelopeFiles(target, kind) {
  const p = path.join(pkg, 'sysroot', `sysroot-${target}.tinyc.js`);
  if (!fs.existsSync(p)) return null;
  const ctx = { globalThis: {} };
  vm.createContext(ctx);
  vm.runInContext(fs.readFileSync(p, 'utf8'), ctx);
  const reg = ctx.globalThis.__TINYC_ASSET_REGISTRY__;
  const env = reg && reg[`sysroot-${target}`];
  if (!env || !env.files) return null;
  const out = new Map();
  for (const [source, b64] of Object.entries(env.files)) {
    if (kind === 'lib' && source.startsWith(`lib/${target}/`)) out.set(source.slice(`lib/${target}/`.length), Buffer.from(b64, 'base64'));
    if (kind === 'include' && source.startsWith('include/')) out.set(source.slice('include/'.length), Buffer.from(b64, 'base64'));
    if (kind === 'include' && source === 'tcclib.h') out.set('tcclib.h', Buffer.from(b64, 'base64'));
  }
  return out;
}

/* Normalize a dist-shaped sysroot tree (the layout stage_layout
 * and the AAR both carry: <root>/include/** plus
 * <root>/lib/tcc/{include,include-libc,win32/include}/**) into
 * the dist's own coordinate system: libs keyed by their path
 * under lib/, headers keyed by their path in the merged include
 * view (include-libc over include, win32/include filling gaps). */
function normalizeDistShaped(root) {
  const libs = new Map();
  const headers = new Map();
  const all = walk(root);
  const setH = (rel, buf, overwrite) => { if (overwrite || !headers.has(rel)) headers.set(rel, buf); };
  for (const [rel0, buf] of all) {
    const rel = rel0.split(path.sep).join('/');
    if (rel.startsWith('include/')) setH(rel.slice(8), buf, false);
    else if (rel.startsWith('lib/tcc/include-libc/')) setH(rel.slice(21), buf, true);
    else if (rel.startsWith('lib/tcc/include/')) setH(rel.slice(16), buf, false);
    else if (rel.startsWith('lib/tcc/win32/include/')) setH(rel.slice(22), buf, false);
    else if (rel.startsWith('lib/')) libs.set(rel.slice(4), buf);
  }
  return { libs, headers };
}

function aarFiles(target, kind) {
  const assets = path.join(repo, 'packages', 'android', 'src', 'main', 'assets');
  const merged = new Map();
  for (const sub of ['sysroot', 'toolchain']) {
    const { libs, headers } = normalizeDistShaped(path.join(assets, sub, target));
    for (const [k, v] of libs) if (!merged.has('L' + k)) merged.set('L' + k, v);
    for (const [k, v] of headers) if (!merged.has('H' + k)) merged.set('H' + k, v);
  }
  const out = new Map();
  for (const [k, v] of merged) {
    if (kind === 'lib' && k.startsWith('L')) out.set(k.slice(1), v);
    if (kind === 'include' && k.startsWith('H')) out.set(k.slice(1), v);
  }
  return out;
}

/* ---------------- comparison ---------------- */

const findings = [];
function compareSets(label, target, aName, aFiles, bName, bFiles, keyMode = 'basename') {
  if (keyMode === 'path') {
    let same = 0, diff = 0;
    const ha = new Map([...aFiles].map(([k, v]) => [k.split(path.sep).join('/'), sha(v)]));
    const hb = new Map([...bFiles].map(([k, v]) => [k.split(path.sep).join('/'), sha(v)]));
    for (const name of new Set([...ha.keys(), ...hb.keys()])) {
      if (!ha.has(name)) { findings.push({ label, target, file: name, kind: `only-in-${bName}` }); diff += 1; continue; }
      if (!hb.has(name)) { findings.push({ label, target, file: name, kind: `only-in-${aName}` }); diff += 1; continue; }
      if (ha.get(name) === hb.get(name)) same += 1;
      else { findings.push({ label, target, file: name, kind: 'bytes-differ', a: [ha.get(name).slice(0, 12)], b: [hb.get(name).slice(0, 12)] }); diff += 1; }
    }
    console.log(`${label} ${target}: ${aName} vs ${bName}: ${same} identical, ${diff} different/missing`);
    return diff;
  }
  const a = byBasename(aFiles, aName);
  const b = byBasename(bFiles, bName);
  let same = 0, diff = 0;
  for (const name of new Set([...a.keys(), ...b.keys()])) {
    const ea = a.get(name), eb = b.get(name);
    if (!ea) { findings.push({ label, target, file: name, kind: `only-in-${bName}` }); diff += 1; continue; }
    if (!eb) { findings.push({ label, target, file: name, kind: `only-in-${aName}` }); diff += 1; continue; }
    const ha = new Set(ea.map((x) => x.hash)), hb = new Set(eb.map((x) => x.hash));
    if (ha.size === 1 && hb.size === 1 && [...ha][0] === [...hb][0]) same += 1;
    else { findings.push({ label, target, file: name, kind: 'bytes-differ', a: ea.map((x) => `${x.rel}:${x.hash.slice(0, 12)}`), b: eb.map((x) => `${x.rel}:${x.hash.slice(0, 12)}`) }); diff += 1; }
  }
  console.log(`${label} ${target}: ${aName} vs ${bName}: ${same} identical, ${diff} different/missing`);
  return diff;
}

/* ar member timestamp scan */
function arScan(target, files) {
  const bad = [];
  for (const [rel, buf] of files) {
    if (!rel.endsWith('.a')) continue;
    if (buf.slice(0, 8).toString('latin1') !== '!<arch>\n') { bad.push(`${rel}: not an ar archive`); continue; }
    let off = 8;
    while (off + 60 <= buf.length) {
      const hdr = buf.slice(off, off + 60);
      if (hdr[58] !== 0x60 || hdr[59] !== 0x0a) break;
      const name = hdr.slice(0, 16).toString('latin1').trim();
      const mtime = hdr.slice(16, 28).toString('latin1').trim();
      const size = parseInt(hdr.slice(48, 58).toString('latin1').trim(), 10);
      if (mtime !== '0' && mtime !== '') bad.push(`${rel}: member ${name} mtime=${mtime}`);
      off += 60 + size + (size % 2);
    }
  }
  return bad;
}

/* ---------------- staging ---------------- */

let stageRoot = null;
if (args.stage) {
  stageRoot = args.stageRoot;
  fs.rmSync(stageRoot, { recursive: true, force: true });
  fs.mkdirSync(stageRoot, { recursive: true });
  const hosts = ['x86_64-linux-gnu', 'x86_64-w64-mingw32', 'arm64-apple-darwin', 'x86_64-apple-darwin', 'aarch64-linux-android'];
  const r = spawnSync('python3', [path.join(repo, 'packaging', 'stage_layout.py'),
    '--root', stageRoot, ...hosts.flatMap((h) => ['--host', h]),
    '--dist', path.join(repo, 'build', 'dist')], { encoding: 'utf8' });
  if (r.status !== 0) { console.error('stage_layout failed:', r.stderr || r.stdout); process.exit(2); }
  console.log(`staged host packages for: ${hosts.join(', ')}`);
}

let totalDiff = 0;
const arFindings = [];
for (const target of args.targets) {
  const dLibs = distLibs(target);
  const dHdrs = distHeaders(target);
  if (!dLibs.size && !dHdrs.size) { console.log(`SKIP ${target}: no dist`); continue; }
  arFindings.push(...arScan(target, dLibs).map((s) => `${target}: ${s}`));

  const bLibs = envelopeFiles(target, 'lib');
  if (bLibs) totalDiff += compareSets('libs', target, 'dist', dLibs, 'browser-sysroot', bLibs);
  else console.log(`libs ${target}: browser sysroot envelope missing`);
  const bHdrs = envelopeFiles(target, 'include');
  if (bHdrs) totalDiff += compareSets('headers', target, 'dist', dHdrs, 'browser-sysroot', bHdrs);

  if (args.aar) {
    /* The AAR carries the merged header view as one include/
     * tree (path-compared against the dist's merged view) but
     * flattens its lib/ layout, so its libs compare by basename
     * like the browser envelope's. */
    const aLibs = aarFiles(target, 'lib');
    if (aLibs.size) totalDiff += compareSets('libs', target, 'dist', dLibs, 'aar-assets', aLibs);
    const aHdrs = aarFiles(target, 'include');
    if (aHdrs.size) totalDiff += compareSets('headers', target, 'dist', dHdrs, 'aar-assets', aHdrs, 'path');
  }
  if (stageRoot) {
    /* stage_layout lays a staged package out as
     * <root>/sysroot/<dist-triple>/{include,lib} with the
     * sysroot shared across hosts (only the compiler binaries
     * under natives/<host>/ differ per host), so the staged
     * comparison is per target: the staged lib tree against
     * the dist lib tree it was copied from. */
    const { libs: sLibs, headers: sHdrs } = normalizeDistShaped(path.join(stageRoot, 'sysroot', DIST_TRIPLE[target]));
    if (sLibs.size) totalDiff += compareSets('libs', target, 'dist', dLibs, 'staged', sLibs, 'path');
    if (sHdrs.size) totalDiff += compareSets('headers', target, 'dist', dHdrs, 'staged', sHdrs, 'path');
  }
}

console.log('\nar member timestamp scan:', arFindings.length ? '' : 'all zero (deterministic)');
for (const s of arFindings) console.log('  AR:', s);
console.log(`\ntotal differing/missing entries: ${totalDiff + arFindings.length}`);
if (args.json) fs.writeFileSync(args.json, JSON.stringify({ findings, arFindings }, null, 1));
process.exit(totalDiff + arFindings.length ? 1 : 0);
