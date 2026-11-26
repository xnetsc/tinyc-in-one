#!/usr/bin/env node
/* Shipped-sysroot check for an unpacked tinyc-web package directory.
 *
 * The package ships one sysroot asset envelope per target under
 * sysroot/ (sysroot-<target>.tinyc.js); loadSysroot fetches exactly
 * that file, so an artifact without it installs a compiler whose
 * sysroot cannot be loaded. For every target manifest in the
 * artifact this check asserts, by name:
 *   - the envelope file is present in sysroot/;
 *   - it registers a payload under the key `sysroot-<target>` whose
 *     files map carries every entry the manifest lists — every
 *     include file, and the target's libtcc1.a;
 *   - every carried file's base64 decodes to at least one byte.
 *
 * The one registered exception is the libtcc1.a of the two macOS
 * targets: their dist is built on macOS hosts, so a package packed
 * on Linux legitimately carries those two envelopes with headers
 * only. A missing library entry for them is reported as a notice,
 * not a failure; a missing library entry for any other target, or
 * a missing header entry for any target, fails. --require-libs
 * lifts the exception: the release job completes the two macOS
 * envelopes from the macOS jobs' dist tarballs before publishing
 * and gates the published zip with --require-libs, so a macOS
 * envelope without its libtcc1.a fails there.
 *
 * Usage: node check_sysroot_envelopes.mjs <unpacked-package-dir>
 *          [--require-libs]
 */
import fs from 'node:fs';
import path from 'node:path';

const argv = process.argv.slice(2);
const requireLibs = argv.includes('--require-libs');
const root = argv.find((a) => !a.startsWith('--'));
if (!root) {
  console.error('usage: node check_sysroot_envelopes.mjs <unpacked-package-dir> [--require-libs]');
  process.exit(2);
}

const HEADERS_ONLY_LIB_TARGETS = new Set([
  'arm64-apple-darwin',
  'x86_64-apple-darwin',
]);

const manifestsDir = path.join(root, 'sysroot-manifests');
if (!fs.existsSync(manifestsDir)) {
  console.log('::error::SYSROOT ENVELOPE GAP: sysroot-manifests/ is missing from the packaged artifact.');
  process.exit(1);
}
const targets = fs.readdirSync(manifestsDir)
  .filter((f) => f.endsWith('.json'))
  .map((f) => f.slice(0, -'.json'.length))
  .sort();

let failures = 0;
for (const target of targets) {
  const manifest = JSON.parse(
    fs.readFileSync(path.join(manifestsDir, `${target}.json`), 'utf8'));
  const envelopePath = path.join(root, 'sysroot', `sysroot-${target}.tinyc.js`);
  if (!fs.existsSync(envelopePath)) {
    console.log(`::error::SYSROOT ENVELOPE GAP: 'sysroot/sysroot-${target}.tinyc.js' is missing from the packaged artifact; loadSysroot has no sysroot to load for target "${target}". Build it with packages/browser/build-sysroot-envelopes.mjs (build.sh runs it per target) and ship the sysroot/ directory.`);
    failures += 1;
    continue;
  }
  const text = fs.readFileSync(envelopePath, 'utf8');
  const m = text.match(
    /__TINYC_ASSET_REGISTRY__\[("(?:[^"\\]|\\.)*")\]\s*=\s*(\{[\s\S]*\});?\s*$/);
  let payload = null;
  if (m) {
    try {
      const key = JSON.parse(m[1]);
      if (key === `sysroot-${target}`) payload = JSON.parse(m[2]);
    } catch {
      payload = null;
    }
  }
  if (!payload || typeof payload.files !== 'object' || !payload.files) {
    console.log(`::error::SYSROOT ENVELOPE GAP: 'sysroot/sysroot-${target}.tinyc.js' does not register a sysroot payload under the key "sysroot-${target}".`);
    failures += 1;
    continue;
  }
  const entries = [
    ...(manifest.includeFiles ?? []),
    ...(manifest.libFiles ?? []),
  ];
  for (const entry of entries) {
    const b64 = payload.files[entry.source];
    const present = typeof b64 === 'string' &&
      Buffer.from(b64, 'base64').length > 0;
    if (present) continue;
    if (entry.source === `lib/${target}/libtcc1.a` &&
        HEADERS_ONLY_LIB_TARGETS.has(target) && !requireLibs) {
      console.log(`notice: ${target}: sysroot envelope carries no "${entry.source}" (its dist is built on a macOS host; the envelope ships headers only).`);
      continue;
    }
    console.log(`::error::SYSROOT ENVELOPE GAP: 'sysroot/sysroot-${target}.tinyc.js' carries no usable "${entry.source}", which the target's shipped manifest lists.`);
    failures += 1;
  }
  console.log(`${target}: sysroot envelope present, ${entries.length} manifest entries checked.`);
}

if (failures) process.exit(1);
console.log(`sysroot envelopes OK: all ${targets.length} targets carry their shipped sysroot.`);
