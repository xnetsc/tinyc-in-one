#!/usr/bin/env node
/*
 * Generate the wasm32 sysroot's libtcc1 unit:
 * sysroot-lib/wasm32-wasi/libtcc1.tinyc.js.
 *
 * The unit is TCC's own runtime library (lib/libtcc1.c — the
 * __divdi3/__moddi3/__udivdi3/__umoddi3 division helpers and the
 * variable-count 64-bit shifts, plus lib/builtin.c's __builtin_*
 * fallback functions) compiled by the wasm32 compiler as a
 * dylink side module.  It is the wasm form of the libtcc1 archive
 * every native target links into its products unconditionally:
 * TinycProcess loads it into every process before the main
 * module (RTLD_GLOBAL), so ordinary code that divides or shifts
 * long longs resolves its helpers from it, and the standalone
 * run paths draw the same exports from a private instance.
 *
 * The sources are the PATCHED copies in the wasm backend's
 * build tree (wasm-backend/build-src/lib/): the di3 section is
 * enabled for __wasm32__ by wasm-backend/patches, so the vendor
 * copies will not do.  Build the backend first
 * (wasm-backend/build.sh).  The compiler is taken from
 * $TINYC_WASM32_TCC when set, else the backend's build tree.
 *
 * The output is a committed file; this script is run by hand
 * after a backend rebuild, not by the sysroot envelope build.
 *
 * Usage: node gen-libtcc1-unit.mjs [--check]
 *   --check: verify the committed unit matches a fresh build
 *   without writing (exit 1 on drift).
 */
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, '../..');
const out = path.join(here, 'sysroot-lib', 'wasm32-wasi', 'libtcc1.tinyc.js');
const checkOnly = process.argv.includes('--check');

const tcc = process.env.TINYC_WASM32_TCC ??
  path.join(repo, 'wasm-backend', 'build-src', 'wasm32-tcc');
if (!fs.existsSync(tcc)) {
  console.error(`gen-libtcc1-unit: wasm32-tcc not found at ${tcc}; ` +
    'run wasm-backend/build.sh first');
  process.exit(2);
}
const libDir = path.join(repo, 'wasm-backend', 'build-src', 'lib');
for (const name of ['libtcc1.c', 'builtin.c']) {
  if (!fs.existsSync(path.join(libDir, name))) {
    console.error(`gen-libtcc1-unit: ${name} not found in ${libDir}`);
    process.exit(2);
  }
}

const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'tinyc-libtcc1-'));
try {
  const built = path.join(tmp, 'libtcc1.tinyc.js');
  // The envelope registers itself under the -o spelling, so
  // compile from inside the temp dir with a bare file name:
  // the committed unit's registry key is "libtcc1.tinyc.js",
  // and the build is byte-for-byte reproducible.
  execFileSync(tcc, [
    '-nostdlib', '-wasm-dylink=side',
    path.join(libDir, 'libtcc1.c'),
    path.join(libDir, 'builtin.c'),
    '-o', 'libtcc1.tinyc.js',
  ], { stdio: 'inherit', cwd: tmp });
  const text = fs.readFileSync(built, 'utf8');
  const meta = JSON.parse(text.slice(text.indexOf(' = ') + 3,
                                     text.lastIndexOf(';')));
  const required = [
    '__divdi3', '__moddi3', '__udivdi3', '__umoddi3',
    '__ashldi3', '__ashrdi3', '__lshrdi3',
    '__builtin_bswap16', '__builtin_bswap32', '__builtin_bswap64',
  ];
  const exported = new Set(meta.exports.map((e) => e.name));
  const missing = required.filter((n) => !exported.has(n));
  if (missing.length) {
    console.error(`gen-libtcc1-unit: unit is missing exports: ${missing.join(' ')}`);
    process.exit(1);
  }
  if (checkOnly) {
    const committed = fs.existsSync(out) ? fs.readFileSync(out, 'utf8') : null;
    if (committed !== text) {
      console.error('gen-libtcc1-unit: committed libtcc1.tinyc.js has drifted; ' +
        'regenerate with: node gen-libtcc1-unit.mjs');
      process.exit(1);
    }
    console.log('gen-libtcc1-unit: committed unit is current');
  } else {
    fs.writeFileSync(out, text);
    console.log(`gen-libtcc1-unit: wrote ${out} (${text.length} bytes, ` +
      `${meta.exports.length} exports)`);
  }
} finally {
  fs.rmSync(tmp, { recursive: true, force: true });
}
