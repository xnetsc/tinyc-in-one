#!/usr/bin/env node
// Build tinyc-web.js: the classic-script bundle of this package.
//
// Bundles index.mjs + runtime-shims.mjs + every sysroot-manifests/*.json
// (statically imported by index.mjs, so the JSON is inlined) into one
// IIFE file that exposes the package's public API as the global
// `TinycWeb`. The per-target native files are NOT inlined: each
// target's asset envelope tcc-<target>.tinyc.js is retrieved at
// runtime through the package's transport (fetched as text or
// injected as a script), resolved relative to the bundle file's own
// URL (see the "Asset base URL" section in index.mjs).
//
// Uses esbuild: a locally installed copy when present, otherwise
// `npx esbuild@0.25.8`. Run directly (node build-web-bundle.mjs) or
// via `npm run build:web`. This is independent of build.sh, which
// builds the native modules with Emscripten.
import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const entry = path.join(here, 'web-entry.mjs');
const outfile = path.join(here, 'tinyc-web.js');

const buildOptions = {
  entryPoints: [entry],
  bundle: true,
  format: 'iife',
  globalName: 'TinycWeb',
  platform: 'browser',
  outfile,
  logLevel: 'info',
};

let esbuild = null;
try {
  esbuild = await import('esbuild');
} catch {
  // No local install; fall back to npx below.
}

if (esbuild) {
  await esbuild.build(buildOptions);
} else {
  const args = [
    '--yes', 'esbuild@0.25.8', entry,
    '--bundle', '--format=iife', '--global-name=TinycWeb',
    '--platform=browser', `--outfile=${outfile}`,
  ];
  const res = spawnSync('npx', args, { stdio: 'inherit' });
  if (res.status !== 0) process.exit(res.status ?? 1);
}

const { size } = fs.statSync(outfile);
console.log(`tinyc-web.js built: ${size} bytes`);
