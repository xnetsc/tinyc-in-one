'use strict';
const { test } = require('node:test');
const assert = require('node:assert');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { execFileSync } = require('child_process');
const tcc = require('../index.js');

function tmpdir() { return fs.mkdtempSync(path.join(os.tmpdir(), 'tinyc-test-')); }

test('cli: --version via exported main returns 0 and does not exit', () => {
  const [code, out] = tcc.runCliCapture(['--version']);
  assert.strictEqual(code, 0);
  assert.match(out.toString(), /tcc version/);
});

test('cli: -dumpversion returns 0 (no host exit)', () => {
  const [code, out] = tcc.runCliCapture(['-dumpversion']);
  assert.strictEqual(code, 0);
  assert.match(out.toString(), /0\.9\.28/);
});

test('cli: compile and run an executable', () => {
  const d = tmpdir();
  const src = path.join(d, 'hello.c');
  const exe = path.join(d, 'hello');
  fs.writeFileSync(src, '#include <stdio.h>\nint main(){ printf("hi-js\\n"); return 7; }\n');
  const [code, out] = tcc.runCliCapture([src, '-o', exe]);
  assert.strictEqual(code, 0, out.toString());
  const r = require('child_process').spawnSync(exe, [], { encoding: 'buffer' });
  assert.strictEqual(r.status, 7);
  // A Windows build of the product prints CRLF (CRT text mode);
  // normalize line endings, compare everything else byte-exactly.
  assert.strictEqual(r.stdout.toString().replace(/\r\n/g, '\n'), 'hi-js\n');
});

test('cli: bad source returns nonzero and captures the error', () => {
  const d = tmpdir();
  const src = path.join(d, 'bad.c');
  fs.writeFileSync(src, 'int main( { nope }\n');
  const [code, out] = tcc.runCliCapture(['-c', src, '-o', path.join(d, 'bad.o')]);
  assert.notStrictEqual(code, 0);
  assert.match(out.toString(), /error/);
});

test('cli: -E output is routed through IO capture', () => {
  const d = tmpdir();
  const src = path.join(d, 'pp.c');
  fs.writeFileSync(src, '#define VAL 4242\nint x = VAL;\n');
  const [code, out] = tcc.runCliCapture(['-E', src]);
  assert.strictEqual(code, 0, out.toString());
  assert.match(out.toString(), /int x = 4242;/);
});

test('cli: dynamic add of a local include dir and a local static library', () => {
  const d = tmpdir();
  const inc = path.join(d, 'inc'); fs.mkdirSync(inc);
  fs.writeFileSync(path.join(inc, 'dyn.h'), '#define DYNGREET 99\n');
  const libsrc = path.join(d, 'dyn.c');
  fs.writeFileSync(libsrc, 'int dynval(void){ return 1234; }\n');
  let [code, out] = tcc.runCliCapture(['-c', libsrc, '-o', path.join(d, 'dyn.o')]);
  assert.strictEqual(code, 0, out.toString());
  execFileSync('ar', ['rcs', path.join(d, 'libdyn.a'), path.join(d, 'dyn.o')]);
  const main = path.join(d, 'main.c');
  fs.writeFileSync(main, '#include "dyn.h"\n#include <stdio.h>\nextern int dynval(void);\n' +
    'int main(){ printf("dyn %d %d\\n", DYNGREET, dynval()); return 0; }\n');
  const exe = path.join(d, 'prog');
  [code, out] = tcc.runCliCapture(['-I' + inc, main, path.join(d, 'libdyn.a'), '-o', exe]);
  assert.strictEqual(code, 0, out.toString());
  assert.strictEqual(execFileSync(exe).toString().replace(/\r\n/g, '\n'), 'dyn 99 1234\n');
});

test('api: compileString and call the compiled function', () => {
  const s = new tcc.State();
  s.compileString('int add(int a, int b){ return a + b; }');
  const add = s.function('add', 'int', ['int', 'int']);
  assert.strictEqual(add(19, 23), 42);
  s.close();
});

test('api: compile errors are collected, not printed', () => {
  const s = new tcc.State();
  assert.throws(() => s.compileString('int broken( {'), tcc.TinyCError);
  assert.ok(s.errors.some((e) => e.includes('error')));
  s.close();
});

test('api: dynamic add include path via API', () => {
  const d = tmpdir();
  const inc = path.join(d, 'inc'); fs.mkdirSync(inc);
  fs.writeFileSync(path.join(inc, 'cfg.h'), '#define CFGVAL 77\n');
  const s = new tcc.State();
  s.addIncludePath(inc);
  s.compileString('#include "cfg.h"\nint getcfg(void){ return CFGVAL; }');
  assert.strictEqual(s.function('getcfg', 'int', [])(), 77);
  s.close();
});

test('api: defineSymbol', () => {
  const s = new tcc.State();
  s.defineSymbol('MAGIC', '123');
  s.compileString('int magic(void){ return MAGIC; }');
  assert.strictEqual(s.function('magic', 'int', [])(), 123);
  s.close();
});

test('api: addSymbol injects a JS function callable from compiled code', () => {
  const calls = [];
  const s = new tcc.State();
  s.addSymbol('host_mul', (a, b) => { calls.push([a, b]); return a * b; },
    'int', ['int', 'int']);
  s.compileString('extern int host_mul(int, int);\n' +
    'int use_host(void){ return host_mul(6, 7); }');
  assert.strictEqual(s.function('use_host', 'int', [])(), 42);
  assert.deepStrictEqual(calls, [[6, 7]]);
  s.close();
});

test('api: outputFile after addSymbol throws', () => {
  const d = tmpdir();
  const obj = path.join(d, 'x.o');
  const s = new tcc.State(tcc.TCC_OUTPUT_OBJ);
  s.addSymbol('host_mul', (a, b) => a * b, 'int', ['int', 'int']);
  s.compileString('extern int host_mul(int, int);\n' +
    'int xf(void){ return host_mul(2, 3); }');
  assert.throws(() => s.outputFile(obj), /addSymbol/);
  s.close();
  assert.strictEqual(fs.existsSync(obj), false);
});

test('io: JS callback receives compiler output', () => {
  const chunks = [];
  const io = tcc.IO.callback({ out: (b) => chunks.push(b), err: (b) => chunks.push(b) });
  const code = tcc.runCli(['--version'], { io });
  assert.strictEqual(code, 0);
  assert.match(Buffer.concat(chunks).toString(), /tcc version/);
  io.close();
});

test('io: per-state IOs stay separate when states interleave', () => {
  // -E output does not go through the error callback, so it is the
  // channel that proves each state routes through its own IO.
  const a = new tcc.State(tcc.TCC_OUTPUT_PREPROCESS);
  const b = new tcc.State(tcc.TCC_OUTPUT_PREPROCESS);
  const ioA = tcc.IO.buffer();
  const ioB = tcc.IO.buffer();
  try {
    a.setIO(ioA);
    b.setIO(ioB);
    a.compileString('#define VA 111\nint xa = VA;\n');
    assert.match(ioA.out.toString(), /int xa = 111;/);
    assert.strictEqual(ioB.out.length, 0);
    b.compileString('#define VB 222\nint xb = VB;\n');
    assert.match(ioB.out.toString(), /int xb = 222;/);
    assert.doesNotMatch(ioA.out.toString(), /xb/);
    // A CLI capture in between installs nothing on either state.
    const [code, out] = tcc.runCliCapture(['--version']);
    assert.strictEqual(code, 0);
    assert.match(out.toString(), /tcc version/);
    a.compileString('#define VA2 333\nint xa2 = VA2;\n');
    assert.match(ioA.out.toString(), /int xa2 = 333;/);
    assert.doesNotMatch(ioB.out.toString(), /xa2/);
  } finally {
    a.close(); b.close(); ioA.close(); ioB.close();
  }
});

/* ------------------------------------------- host / target resolution
 * Host simulation runs in child processes: TINYC_HOST_TRIPLE overrides
 * detection, and each child gets a fresh module registry. */
function runChild(env, script) {
  return require('child_process').spawnSync(process.execPath, ['-e', script], {
    env: { ...process.env, ...env },
    cwd: path.join(__dirname, '..'),
    encoding: 'utf8',
  });
}

const RESOLVE_OR_ERR = `
  const tcc = require('./index.js');
  try { const r = tcc.resolveNative(); console.log('RESOLVED ' + r.host + ' x ' + r.target + ' ' + r.binaryPath); }
  catch (e) { console.log('ERR ' + e.message); }
`;

function packageManifest() {
  return JSON.parse(fs.readFileSync(
    path.join(__dirname, '..', 'packaging-manifest.json'), 'utf8'));
}

test('host: TINYC_HOST_TRIPLE=x86_64-linux-gnu resolves and compiles', () => {
  // The expectation follows the package manifest (the same pattern
  // as the Python and Java host-simulation tests): when the
  // simulated combination is staged in this package it resolves and
  // compiles; when it is not (a package built for another host), the
  // resolution error must name the simulated host and list the
  // available combinations.
  const present = (packageManifest().combinations || []).some(
    (c) => c.host === 'x86_64-linux-gnu' && c.target === 'x86_64-linux-gnu');
  if (!present) {
    const r = runChild({ TINYC_HOST_TRIPLE: 'x86_64-linux-gnu' }, RESOLVE_OR_ERR);
    assert.strictEqual(r.status, 0, r.stderr);
    assert.match(r.stdout, /ERR tinyc: no native build for host x86_64-linux-gnu \/ target x86_64-linux-gnu/);
    assert.match(r.stdout, /available combinations: /);
    return;
  }
  const script = `
    const tcc = require('./index.js');
    const r = tcc.resolveNative();
    if (!r.binaryPath.includes('natives/x86_64-linux-gnu/toolchain/x86_64-linux-gnu')) {
      throw new Error('unexpected binary path: ' + r.binaryPath);
    }
    const [code, out] = tcc.runCliCapture(['-dumpversion']);
    if (code !== 0) throw new Error('dumpversion rc=' + code);
    console.log('VERSION ' + out.toString().trim());
  `;
  const r = runChild({ TINYC_HOST_TRIPLE: 'x86_64-linux-gnu' }, script);
  assert.strictEqual(r.status, 0, r.stderr);
  assert.match(r.stdout, /VERSION 0\.9\.28/);
});

test('host: combination missing from the package names host and available combinations', () => {
  // The simulated host is one this package does not stage; the
  // available list in the error must name every combination the
  // package manifest actually carries.
  const manifest = packageManifest();
  const stagedHosts = new Set((manifest.combinations || []).map((c) => c.host));
  const absentHost = ['x86_64-w64-mingw32', 'aarch64-linux-gnu', 'x86_64-linux-gnu']
    .find((h) => !stagedHosts.has(h));
  const r = runChild({ TINYC_HOST_TRIPLE: absentHost }, RESOLVE_OR_ERR);
  assert.strictEqual(r.status, 0, r.stderr);
  assert.match(r.stdout, new RegExp(
    `ERR tinyc: no native build for host ${absentHost} \\/ target ${absentHost}`));
  for (const c of manifest.combinations || []) {
    assert.ok(r.stdout.includes(`${c.host} x ${c.target}`),
      `available list missing ${c.host} x ${c.target}: ${r.stdout}`);
  }
});

test('host: 32-bit host triple is rejected, no fallback', () => {
  const r = runChild({ TINYC_HOST_TRIPLE: 'i386-linux-gnu' }, RESOLVE_OR_ERR);
  assert.strictEqual(r.status, 0, r.stderr);
  assert.match(r.stdout, /ERR tinyc: 32-bit hosts are not supported \(host triple: i386-linux-gnu\)/);
});

test('host: both darwin hosts resolve through the single merged subpackage', () => {
  // The merged subpackage tinyc-native-apple-darwin serves both
  // macOS hosts: each simulated darwin host must resolve its native
  // combination from the same subpackage root. The stand-in root is
  // built here (manifest + placeholder binaries) and pointed at with
  // TINYC_DIST; resolveNative only reads the manifest, so no real
  // Mach-O binary is needed to prove the routing.
  const d = tmpdir();
  const sub = path.join(d, 'tinyc-native-apple-darwin');
  for (const h of ['arm64-apple-darwin', 'x86_64-apple-darwin']) {
    const binDir = path.join(sub, 'natives', h, 'toolchain', h);
    fs.mkdirSync(binDir, { recursive: true });
    fs.writeFileSync(path.join(binDir, 'tinyc'), '');
    const sys = path.join(sub, 'sysroot', h);
    fs.mkdirSync(path.join(sys, 'lib', 'tcc'), { recursive: true });
  }
  const combos = ['arm64-apple-darwin', 'x86_64-apple-darwin'].map((h) => ({
    host: h, target: h,
    binary: `natives/${h}/toolchain/${h}/tinyc`,
    sysroot: `sysroot/${h}`,
    libPath: `sysroot/${h}/lib/tcc`,
    outputs: ['exe', 'obj', 'dll'],
  }));
  fs.writeFileSync(path.join(sub, 'packaging-manifest.json'), JSON.stringify({
    hosts: ['arm64-apple-darwin', 'x86_64-apple-darwin'],
    combinations: combos,
    pending: [],
  }, null, 2));
  for (const h of ['arm64-apple-darwin', 'x86_64-apple-darwin']) {
    const r = runChild(
      { TINYC_HOST_TRIPLE: h, TINYC_DIST: sub }, RESOLVE_OR_ERR);
    assert.strictEqual(r.status, 0, r.stderr);
    assert.match(r.stdout, new RegExp(
      `RESOLVED ${h} x ${h} .*tinyc-native-apple-darwin` +
      `/natives/${h}/toolchain/${h}/tinyc`));
  }
});

test('target: TINYC_TARGET_TRIPLE selects the cross variant and compiles for it', (t) => {
  // Pick a cross target the package actually contains for its own
  // host (the set of staged targets follows the kernel matrix build).
  const manifest = packageManifest();
  const ownHost = tcc.resolveNative().host;
  const MACHINES = { 'aarch64-linux-gnu': /AArch64/, 'riscv64-linux-gnu': /RISC-V/ };
  const combo = manifest.combinations.find(
    (c) => c.host === ownHost && c.target !== ownHost && MACHINES[c.target]);
  if (!combo) { t.skip('no cross target staged in this package'); return; }
  const d = tmpdir();
  const obj = path.join(d, 'x.o');
  const src = path.join(d, 'x.c');
  const script = `
    const fs = require('fs');
    const tcc = require('./index.js');
    const r = tcc.resolveNative();
    if (r.target !== ${JSON.stringify(combo.target)}) throw new Error('target ' + r.target);
    fs.writeFileSync(${JSON.stringify(src)}, 'int add(int a, int b){ return a + b; }\\n');
    const [code, out] = tcc.runCliCapture(['-c', ${JSON.stringify(src)}, '-o', ${JSON.stringify(obj)}]);
    if (code !== 0) throw new Error('compile rc=' + code + ' ' + out);
    console.log('COMPILED');
  `;
  const r = runChild({ TINYC_TARGET_TRIPLE: combo.target }, script);
  assert.strictEqual(r.status, 0, r.stderr);
  assert.match(r.stdout, /COMPILED/);
  const hdr = execFileSync('readelf', ['-h', obj]).toString();
  assert.match(hdr, new RegExp('Machine:\\s+' + MACHINES[combo.target].source));
});

test('manifest: outputs are read from packaging-manifest, not hardcoded', () => {
  const r = tcc.resolveNative();
  const mf = JSON.parse(fs.readFileSync(path.join(r.root, 'packaging-manifest.json'), 'utf8'));
  const combo = mf.combinations.find((c) => c.host === r.host && c.target === r.target);
  assert.ok(combo, 'resolved combination is listed in the manifest');
  assert.deepStrictEqual(r.outputs, combo.outputs ?? null);
});

test('io: file implementation', () => {
  const d = tmpdir();
  const log = path.join(d, 'log.txt');
  const io = tcc.IO.file(log);
  const code = tcc.runCli(['--version'], { io });
  io.close();
  assert.strictEqual(code, 0);
  assert.match(fs.readFileSync(log, 'utf8'), /tcc version/);
});
