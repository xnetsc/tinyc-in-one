#!/usr/bin/env node
/* Cross-host differential acceptance for the tinyc packages.
 *
 * The settled rule: the same source, compiled for the same
 * target, must produce the same bytes no matter which host
 * runs the compiler -- a wasm-host TCC (the browser package's
 * compiler envelope, driven here through Node) versus a
 * native-host TCC (the dist binary for that target). Any byte
 * difference, and any compile that succeeds on one host and
 * fails on the other, is a defect in the environment, not in
 * the probe: the probes are ordinary user programs and are
 * never adjusted to make a side pass.
 *
 * Method, per target and probe:
 *   1. determinism self-check: each side compiles the probe
 *      twice (the native side from two different working
 *      directories); the two outputs of one side must already
 *      be identical, or byte comparison across hosts is
 *      meaningless for that cell and the cell reports
 *      SELF-DIFF with the differing offsets.
 *   2. cross-host compare: sha256 of the two products; on a
 *      mismatch the report records sizes and the first
 *      differing byte ranges, and both products are kept
 *      under the output directory for inspection.
 *
 * Modes: obj (-c) for every probe; exe (hosted link) for the
 * probes in EXE_PROBES. The wasm32-wasi target has no object
 * output (its backend rejects -c), so it is compared in exe
 * mode only, against the natively built wasm32-tcc.
 *
 * Input-name control: an object file's STT_FILE symbol records
 * the name under which the source was presented to the
 * compiler -- an input to the compilation, not a property of
 * the host (verified on one host: presenting the same source
 * under two different names changes only that symbol). The
 * wasm side compiles through tcc_compile_string, whose input
 * name is "<string>"; the native side therefore presents the
 * probe as a file with that literal name, so the recorded
 * name is a controlled constant on both sides and any
 * remaining byte difference is a compiler difference.
 *
 * Prerequisites: the native dist (../../build.sh) and the
 * browser package's compiler envelopes (packages/browser/
 * build.sh per target) must be built. The sysroot envelopes
 * are (re)built from the package manifests unless --no-sysroot
 * is given.
 *
 * Usage:
 *   node run_diff.mjs [--targets a,b,...] [--probes a,b,...]
 *     [--out <dir>] [--no-sysroot] [--json <path>]
 * Exit status: 0 when every compared cell matches, 1 when any
 * cell differs or fails asymmetrically, 2 on setup errors.
 */
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { spawnSync } from 'node:child_process';
import { fileURLToPath, pathToFileURL } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, '..', '..');
const pkg = path.join(repo, 'packages', 'browser');
const probesDir = path.join(here, 'probes');

/* Browser target name -> dist triple (the dist uses the
 * mingw name for the windows target). */
const DIST_TRIPLE = {
  'x86_64-linux-gnu': 'x86_64-linux-gnu',
  'aarch64-linux-gnu': 'aarch64-linux-gnu',
  'riscv64-linux-gnu': 'riscv64-linux-gnu',
  'x86_64-windows': 'x86_64-w64-mingw32',
  'aarch64-linux-android': 'aarch64-linux-android',
  'x86_64-linux-android': 'x86_64-linux-android',
  'x86_64-apple-darwin': 'x86_64-apple-darwin',
  'arm64-apple-darwin': 'arm64-apple-darwin',
  'wasm32-wasi': null,
};
const ALL_TARGETS = Object.keys(DIST_TRIPLE);
const EXE_PROBES = new Set(['hello']);

function parseArgs(argv) {
  const out = { targets: ALL_TARGETS, probes: null, sysroot: true, outDir: path.join(repo, 'build', 'cross-host-out'), json: null };
  for (let i = 0; i < argv.length; i += 1) {
    const a = argv[i];
    const next = () => { i += 1; if (i >= argv.length) throw new Error(`${a} needs a value`); return argv[i]; };
    if (a === '--targets') out.targets = next().split(',');
    else if (a === '--probes') out.probes = next().split(',');
    else if (a === '--out') out.outDir = path.resolve(next());
    else if (a === '--json') out.json = path.resolve(next());
    else if (a === '--no-sysroot') out.sysroot = false;
    else throw new Error(`unknown argument: ${a}`);
  }
  return out;
}
const args = parseArgs(process.argv.slice(2));

const probeNames = (args.probes ?? fs.readdirSync(probesDir)
  .filter((f) => f.endsWith('.c')).map((f) => f.slice(0, -2)).sort());

function sha256(buf) { return crypto.createHash('sha256').update(buf).digest('hex'); }

function diffRanges(a, b, maxRanges = 6) {
  const ranges = [];
  const n = Math.min(a.length, b.length);
  let i = 0;
  while (i < n && ranges.length < maxRanges) {
    if (a[i] !== b[i]) {
      let j = i;
      while (j < n && a[j] !== b[j]) j += 1;
      ranges.push([i, j - i]);
      i = j;
    } else i += 1;
  }
  return { ranges, commonPrefix: (() => { let k = 0; while (k < n && a[k] === b[k]) k += 1; return k; })() };
}

/* ---------------- native side ---------------- */

function nativeCompiler(target) {
  if (target === 'wasm32-wasi') {
    const p = path.join(repo, 'build', 'dist', 'wasm32-wasi', 'bin', 'tcc');
    return fs.existsSync(p) ? p : null;
  }
  const p = path.join(repo, 'build', 'dist', DIST_TRIPLE[target], 'bin', 'tcc');
  return fs.existsSync(p) ? p : null;
}

function nativeCompile(target, probeName, mode, workDir) {
  const tcc = nativeCompiler(target);
  if (!tcc) return { status: 'setup-missing' };
  fs.mkdirSync(workDir, { recursive: true });
  /* Present the source under the literal name "<string>", the
   * name tcc_compile_string records on the wasm side (see the
   * header comment on input-name control). */
  const presented = path.join(workDir, '<string>');
  fs.copyFileSync(path.join(probesDir, `${probeName}.c`), presented);
  const outName = mode === 'obj' ? 'out.o' : 'out';
  const out = path.join(workDir, outName);
  const argv = [];
  if (target === 'wasm32-wasi') argv.push('-nostdlib');
  if (mode === 'obj') argv.push('-c');
  argv.push('<string>', '-o', outName);
  const r = spawnSync(tcc, argv, { cwd: workDir, encoding: 'utf8', timeout: 120000 });
  if (r.error) return { status: 'error', diagnostic: String(r.error) };
  let outPath = out;
  if (!fs.existsSync(outPath) && fs.existsSync(`${out}.exe`)) outPath = `${out}.exe`;
  if (r.status !== 0 || !fs.existsSync(outPath)) {
    const lines = (r.stderr || '').trim().split('\n').filter(Boolean);
    return { status: 'fail', diagnostic: lines.length ? lines[lines.length - 1] : `exit ${r.status}` };
  }
  return { status: 'ok', bytes: fs.readFileSync(outPath) };
}

/* ---------------- wasm side ---------------- */

let loadTinyc = null;
let TCC_OUTPUT = null;
const wasmInstances = new Map();

async function wasmInstance(target) {
  if (wasmInstances.has(target)) return wasmInstances.get(target);
  const envelopePath = path.join(pkg, `tcc-${target}.tinyc.js`);
  if (!fs.existsSync(envelopePath)) return null;
  const manifest = JSON.parse(fs.readFileSync(path.join(pkg, 'sysroot-manifests', `${target}.json`), 'utf8'));
  const sysrootPath = path.join(pkg, 'sysroot', `sysroot-${target}.tinyc.js`);
  if (!fs.existsSync(sysrootPath)) return null;
  const compilerEnvelope = fs.readFileSync(envelopePath, 'utf8');
  const sysrootEnvelope = fs.readFileSync(sysrootPath, 'utf8');
  let errText = '';
  const dec = new TextDecoder();
  const io = { err: (b) => { errText += typeof b === 'string' ? b : dec.decode(b, { stream: true }); } };
  const tinyc = await loadTinyc({
    target,
    io,
    loadModule: async () => compilerEnvelope,
    sysroot: {
      manifest,
      includeBinaries: true,
      fetchImpl: async () => ({ ok: true, status: 200, text: async () => sysrootEnvelope }),
    },
  });
  const entry = { tinyc, getErr: () => errText, clearErr: () => { errText = ''; } };
  wasmInstances.set(target, entry);
  return entry;
}

async function wasmCompile(target, probeName, mode) {
  const inst = await wasmInstance(target);
  if (!inst) return { status: 'setup-missing' };
  const source = fs.readFileSync(path.join(probesDir, `${probeName}.c`), 'utf8');
  inst.clearErr();
  const st = inst.tinyc.createState();
  try {
    st.setOutputType(mode === 'obj' ? TCC_OUTPUT.OBJ : TCC_OUTPUT.EXE);
    st.compileString(source);
    st.outputFile('/__diff_out');
    return { status: 'ok', bytes: Buffer.from(inst.tinyc.exportFile('/__diff_out')) };
  } catch (e) {
    const lines = inst.getErr().trim().split('\n').filter(Boolean);
    return { status: 'fail', diagnostic: lines.length ? lines[lines.length - 1] : String(e && e.message || e) };
  } finally {
    st.delete();
  }
}

/* ---------------- driver ---------------- */

if (args.sysroot) {
  const r = spawnSync(process.execPath, [path.join(pkg, 'build-sysroot-envelopes.mjs')], { stdio: 'inherit' });
  if (r.status !== 0) { console.error('sysroot envelope build failed'); process.exit(2); }
}
({ loadTinyc, TCC_OUTPUT } = await import(pathToFileURL(path.join(pkg, 'index.mjs')).href));

fs.mkdirSync(args.outDir, { recursive: true });
const cells = [];
let bad = 0;

for (const target of args.targets) {
  for (const probe of probeNames) {
    const modes = target === 'wasm32-wasi' ? ['exe'] : (EXE_PROBES.has(probe) ? ['obj', 'exe'] : ['obj']);
    for (const mode of modes) {
      const cell = { target, probe, mode };
      /* determinism self-check, both sides */
      const n1 = nativeCompile(target, probe, mode, path.join(args.outDir, 'scratch', target, 'n1'));
      const n2 = nativeCompile(target, probe, mode, path.join(args.outDir, 'scratch', target, 'n2'));
      const w1 = await wasmCompile(target, probe, mode);
      const w2 = await wasmCompile(target, probe, mode);
      const selfN = n1.status === 'ok' && n2.status === 'ok'
        ? (sha256(n1.bytes) === sha256(n2.bytes) ? 'stable' : 'SELF-DIFF') : n1.status;
      const selfW = w1.status === 'ok' && w2.status === 'ok'
        ? (sha256(w1.bytes) === sha256(w2.bytes) ? 'stable' : 'SELF-DIFF') : w1.status;
      cell.nativeSelf = selfN;
      cell.wasmSelf = selfW;
      if (n1.status === 'ok' && w1.status === 'ok') {
        cell.nativeSha = sha256(n1.bytes);
        cell.wasmSha = sha256(w1.bytes);
        cell.nativeSize = n1.bytes.length;
        cell.wasmSize = w1.bytes.length;
        if (cell.nativeSha === cell.wasmSha) {
          cell.result = 'MATCH';
        } else {
          cell.result = 'DIFF';
          cell.diff = diffRanges(n1.bytes, w1.bytes);
          const dir = path.join(args.outDir, target, probe, mode);
          fs.mkdirSync(dir, { recursive: true });
          fs.writeFileSync(path.join(dir, 'native.bin'), n1.bytes);
          fs.writeFileSync(path.join(dir, 'wasm.bin'), w1.bytes);
          bad += 1;
        }
      } else if (n1.status === 'fail' && w1.status === 'fail') {
        cell.result = 'BOTH-FAIL';
        cell.nativeDiag = n1.diagnostic;
        cell.wasmDiag = w1.diagnostic;
      } else if (n1.status === 'setup-missing' || w1.status === 'setup-missing') {
        cell.result = 'SETUP-MISSING';
        cell.nativeDiag = n1.status;
        cell.wasmDiag = w1.status;
        bad += 1;
      } else {
        cell.result = 'ASYMMETRIC';
        cell.nativeDiag = n1.status === 'ok' ? 'ok' : n1.diagnostic;
        cell.wasmDiag = w1.status === 'ok' ? 'ok' : w1.diagnostic;
        bad += 1;
      }
      if (selfN === 'SELF-DIFF' || selfW === 'SELF-DIFF') { cell.result += '+SELF-DIFF'; bad += 1; }
      cells.push(cell);
      console.log(`${cell.result.padEnd(18)} ${target} ${probe} ${mode}` +
        (cell.result === 'DIFF' ? ` native=${cell.nativeSize}B wasm=${cell.wasmSize}B firstDiff@${cell.diff.commonPrefix} ranges=${JSON.stringify(cell.diff.ranges)}` : '') +
        (cell.result === 'ASYMMETRIC' ? ` native: ${cell.nativeDiag} | wasm: ${cell.wasmDiag}` : '') +
        (cell.result === 'BOTH-FAIL' ? ` native: ${cell.nativeDiag} | wasm: ${cell.wasmDiag}` : ''));
    }
  }
}

const summary = {};
for (const c of cells) summary[c.result] = (summary[c.result] ?? 0) + 1;
console.log('\nsummary:', JSON.stringify(summary));
if (args.json) fs.writeFileSync(args.json, JSON.stringify({ cells, summary }, null, 1));
process.exit(bad ? 1 : 0);
