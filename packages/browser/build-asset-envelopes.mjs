#!/usr/bin/env node
/* Build the compiler asset envelopes (design §14: every byte the
 * package loads from its origin — library unit files and the
 * compiler's own assets alike — goes through one transport and
 * ships in one form, the envelope file: under file: the transport
 * is classic script injection, so each asset travels as an envelope
 * file that registers its payload in __TINYC_ASSET_REGISTRY__;
 * under http: the same envelope is fetched as text and parsed
 * without being executed. The envelopes are the only shipped form
 * of these assets — no standalone .wasm or glue .mjs is served.
 *
 * Two envelopes are produced:
 *
 *   tcc-<target>.tinyc.js — written next to the compiler module's
 *     intermediate pair. Registers under the key `tcc-<target>`:
 *     { glue, wasm } — the Emscripten glue source text and the
 *     module bytes as base64. loadTinyc builds the module factory
 *     from the glue text and hands the factory the bytes, so
 *     nothing else is fetched.
 *
 *   sysroot-<target>.tinyc.js — written into the served sysroot
 *     directory, next to the files it replaces. Registers under the
 *     key `sysroot-<target>`: { files } — each sysroot file's bytes
 *     as base64, keyed by its manifest source. loadSysroot reads
 *     the entries it needs out of the envelope under both
 *     transports; the loose sysroot files themselves are not
 *     retrieved at runtime.
 *
 * Usage:
 *   node build-asset-envelopes.mjs [--target <target>]
 *     [--assets-dir <dir>]      where tcc-<target>.mjs/.wasm live
 *                               and the compiler envelope is written
 *                               (default: this package's directory)
 *     [--sysroot-src <dir>]     root the manifest sources resolve
 *                               against (default: the same layout
 *                               the sysroot is served from — the
 *                               directory given by --sysroot-out)
 *     [--sysroot-out <dir>]     the served sysroot directory the
 *                               sysroot envelope is written into;
 *                               without it no sysroot envelope is
 *                               built
 *     [--compiler-only]         skip the sysroot envelope
 *     [--sysroot-only]          skip the compiler envelope (for
 *                               callers that only need the sysroot
 *                               envelope and have no compiler module
 *                               pair, e.g. the release job
 *                               regenerating the macOS sysroot
 *                               envelopes from the macOS jobs' dist
 *                               tarballs)
 *
 * build.sh runs this for the compiler envelope after each target
 * build. The sysroot envelope is built wherever the served sysroot
 * is assembled, e.g.:
 *   node build-asset-envelopes.mjs --target wasm32-wasi \
 *     --sysroot-src /path/to/sysroot-sources \
 *     --sysroot-out /path/to/served/sysroot --compiler-only=false
 */
import { readFileSync, writeFileSync, existsSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));

function parseArgs(argv) {
  const out = { target: 'wasm32-wasi', assetsDir: here, compilerOnly: false, sysrootOnly: false };
  for (let i = 0; i < argv.length; i += 1) {
    const a = argv[i];
    const next = () => {
      i += 1;
      if (i >= argv.length) throw new Error(`${a} needs a value`);
      return argv[i];
    };
    if (a === '--target') out.target = next();
    else if (a === '--assets-dir') out.assetsDir = next();
    else if (a === '--sysroot-src') out.sysrootSrc = next();
    else if (a === '--sysroot-out') out.sysrootOut = next();
    else if (a === '--compiler-only') out.compilerOnly = true;
    else if (a.startsWith('--compiler-only=')) {
      out.compilerOnly = a.endsWith('=true');
    } else if (a === '--sysroot-only') out.sysrootOnly = true;
    else if (a.startsWith('--sysroot-only=')) {
      out.sysrootOnly = a.endsWith('=true');
    } else {
      throw new Error(`unknown argument: ${a}`);
    }
  }
  return out;
}

function envelopeFile(key, payload) {
  return (
    'globalThis.__TINYC_ASSET_REGISTRY__ ??= Object.create(null);\n' +
    `globalThis.__TINYC_ASSET_REGISTRY__[${JSON.stringify(key)}] = ` +
    `${JSON.stringify(payload)};\n`
  );
}

function main() {
  const args = parseArgs(process.argv.slice(2));
  if (args.compilerOnly && args.sysrootOnly) {
    throw new Error('--compiler-only and --sysroot-only are mutually exclusive');
  }
  const target = args.target;

  if (!args.sysrootOnly) {
    const gluePath = join(args.assetsDir, `tcc-${target}.mjs`);
    const wasmPath = join(args.assetsDir, `tcc-${target}.wasm`);
    for (const p of [gluePath, wasmPath]) {
      if (!existsSync(p)) {
        throw new Error(`compiler asset not found: ${p} (build it with build.sh)`);
      }
    }
    const compilerOut = join(args.assetsDir, `tcc-${target}.tinyc.js`);
    writeFileSync(compilerOut, envelopeFile(`tcc-${target}`, {
      glue: readFileSync(gluePath, 'utf8'),
      wasm: readFileSync(wasmPath).toString('base64'),
    }));
    console.log(`built: ${compilerOut}`);
  }

  if (args.compilerOnly) return;
  if (!args.sysrootOut) {
    console.log('no --sysroot-out given: sysroot envelope skipped');
    return;
  }
  const manifestPath = join(here, 'sysroot-manifests', `${target}.json`);
  if (!existsSync(manifestPath)) {
    throw new Error(`sysroot manifest not found: ${manifestPath}`);
  }
  const manifest = JSON.parse(readFileSync(manifestPath, 'utf8'));
  const srcRoot = args.sysrootSrc ?? args.sysrootOut;
  const files = {};
  const entries = [
    ...(manifest.includeFiles ?? []),
    ...(manifest.libFiles ?? []),
  ];
  for (const entry of entries) {
    const p = join(srcRoot, entry.source);
    if (!existsSync(p)) {
      // A missing library entry leaves a headers-only envelope:
      // the library is a build output that may not exist where
      // the envelope is generated (the macOS targets' libtcc1.a
      // is built on macOS hosts). The omission is printed, and
      // callers that stage the sources (bundle_assets.py,
      // build-sysroot-envelopes.mjs) report or reject it at
      // their own layer. A missing header is a broken sysroot
      // and stays fatal.
      if (entry.source.startsWith('lib/')) {
        console.log(`notice: sysroot source not staged, entry omitted from the envelope: ${p} (entry ${entry.source})`);
        continue;
      }
      throw new Error(`sysroot source not found: ${p} (entry ${entry.source})`);
    }
    files[entry.source] = readFileSync(p).toString('base64');
  }
  const sysrootOut = join(args.sysrootOut, `sysroot-${target}.tinyc.js`);
  writeFileSync(sysrootOut, envelopeFile(`sysroot-${target}`, { files }));
  console.log(`built: ${sysrootOut} (${entries.length} files)`);
}

main();
