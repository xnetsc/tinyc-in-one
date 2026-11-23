#!/usr/bin/env node
/* Stage the unified (host x target) layout into this package and into
 * the esbuild-style platform subpackage @xnetx/tinyc-native-<host-triple>.
 *
 *   node bundle_native.js [host-triple]     (default: current host)
 *
 * Produces, in both the main package and the subpackage:
 *   packaging-manifest.json
 *   natives/<host>/toolchain/<target>/tinyc
 *   sysroot/<target>/...
 * and refreshes node_modules/@xnetx/tinyc-native-<host> so the subpackage
 * resolution path works in this repo. Run after ../../build.sh.
 */
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');

function detectHost() {
  const p = process.platform, a = process.arch;
  if (p === 'linux' && a === 'x64') return 'x86_64-linux-gnu';
  if (p === 'linux' && a === 'arm64') return 'aarch64-linux-gnu';
  if (p === 'linux' && a === 'riscv64') return 'riscv64-linux-gnu';
  if (p === 'win32' && a === 'x64') return 'x86_64-w64-mingw32';
  if (p === 'darwin' && a === 'arm64') return 'arm64-apple-darwin';
  if (p === 'darwin' && a === 'x64') return 'x86_64-apple-darwin';
  throw new Error(`cannot detect a 64-bit host triple for ${p}/${a}`);
}

const NPM_OS_CPU = {
  'x86_64-linux-gnu': { os: ['linux'], cpu: ['x64'] },
  'aarch64-linux-gnu': { os: ['linux'], cpu: ['arm64'] },
  'riscv64-linux-gnu': { os: ['linux'], cpu: ['riscv64'] },
  'x86_64-w64-mingw32': { os: ['win32'], cpu: ['x64'] },
  'arm64-apple-darwin': { os: ['darwin'], cpu: ['arm64'] },
  'x86_64-apple-darwin': { os: ['darwin'], cpu: ['x64'] },
};

const host = process.argv[2] || detectHost();
const stage = path.join(__dirname, '..', '..', 'packaging', 'stage_layout.py');

function stageInto(root) {
  for (const name of ['natives', 'sysroot', 'packaging-manifest.json', 'native']) {
    fs.rmSync(path.join(root, name), { recursive: true, force: true });
  }
  execFileSync('python3', [stage, '--root', root, '--host', host],
               { stdio: 'inherit' });
}

stageInto(__dirname);

// ---- platform subpackage skeleton (@xnetx/tinyc-native-<host>) ----
const mainPkg = JSON.parse(fs.readFileSync(path.join(__dirname, 'package.json'), 'utf8'));
const subName = `@xnetx/tinyc-native-${host}`;
const subDir = path.join(__dirname, 'native-packages', '@xnetx', `tinyc-native-${host}`);
fs.mkdirSync(subDir, { recursive: true });
stageInto(subDir);
const osCpu = NPM_OS_CPU[host] || {};
fs.writeFileSync(path.join(subDir, 'package.json'), JSON.stringify({
  name: subName,
  version: mainPkg.version,
  description: `TinyC native binaries for host ${host} (dual-form TCC builds and target sysroots).`,
  license: mainPkg.license,
  os: osCpu.os,
  cpu: osCpu.cpu,
  files: ['natives', 'sysroot', 'packaging-manifest.json'],
}, null, 2) + '\n');
console.log(`subpackage ${subName} -> ${subDir}`);

// ---- make the subpackage resolvable from this repo ----
const nmDir = path.join(__dirname, 'node_modules', '@xnetx', `tinyc-native-${host}`);
fs.rmSync(nmDir, { recursive: true, force: true });
fs.mkdirSync(path.dirname(nmDir), { recursive: true });
fs.cpSync(subDir, nmDir, { recursive: true });
console.log(`linked ${subName} into node_modules for in-repo resolution`);
