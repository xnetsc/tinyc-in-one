'use strict';
/*
 * npm consumer test: runs against the INSTALLED tinyc package, never
 * the repository sources. Usage (in a clean directory):
 *
 *   npm install <tinyc tarball> <native subpackage tarball>
 *   node npm-consumer.test.js
 *
 * Exercises the package's public API only: the CLI entry points with
 * captured IO, compile + run of a hello program through the CLI, and
 * the fine-grained State API compiling and calling a function in
 * process. Any assertion failure exits non-zero.
 */
const assert = require('node:assert');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { execFileSync } = require('node:child_process');

const tinyc = require('tinyc');

let passed = 0;
function ok(name, fn) {
  fn();
  passed++;
  console.log('PASS:', name);
}

const isWindows = process.platform === 'win32';
const exeName = isWindows ? 'hello.exe' : 'hello';

const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'tinyc-consumer-'));
try {
  ok('runCliCapture --version reports the TCC version', () => {
    const [code, out] = tinyc.runCliCapture(['--version']);
    assert.strictEqual(code, 0);
    assert.match(String(out), /tcc version/);
  });

  ok('CLI compiles hello.c and the product runs with expected output', () => {
    const src = path.join(tmp, 'hello.c');
    fs.writeFileSync(src, '#include <stdio.h>\nint main(void) { printf("hello from tinyc\\n"); return 0; }\n');
    const exe = path.join(tmp, exeName);
    const [code] = tinyc.runCliCapture([src, '-o', exe]);
    assert.strictEqual(code, 0);
    const out = execFileSync(exe, [], { encoding: 'utf8' });
    // The product is a native executable: on Windows its printf emits
    // CRLF (CRT text mode, the same as MSVC-produced programs), so
    // compare the content exactly, modulo line endings only.
    assert.strictEqual(out.replace(/\r\n/g, '\n'), 'hello from tinyc\n');
  });

  ok('State API compiles a function and calls it in process', () => {
    const s = new tinyc.State();
    try {
      s.compileString('int add(int a, int b) { return a + b; }');
      const add = s.function('add', 'int', ['int', 'int']);
      assert.strictEqual(add(19, 23), 42);
    } finally {
      s.close();
    }
  });

  ok('compile error surfaces as TinyCError naming the problem', () => {
    const s = new tinyc.State();
    try {
      assert.throws(() => s.compileString('int broken( {'), tinyc.TinyCError);
    } finally {
      s.close();
    }
  });
} finally {
  fs.rmSync(tmp, { recursive: true, force: true });
}

console.log(`NPM CONSUMER TEST: ${passed} checks PASS`);
