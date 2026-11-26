#!/usr/bin/env node
/* Assemble sysroot asset envelopes from a set of sysroot manifests.
 *
 * The envelope for a target carries exactly the files its manifest
 * lists — the manifest in the packaged artifact is the authority
 * for what the sysroot contains, and this script is how a consumer
 * of the artifact (and the artifact gate) materializes it. Sources
 * resolve the same way the package's own bundling resolves them:
 * include/<name> is looked up in each --include-root in order, and
 * tcclib.h comes from the path given by --tcclib. A manifest entry
 * whose source cannot be found is a hard failure, named.
 *
 * The libtcc1.a entry (lib/<target>/libtcc1.a) is taken from the
 * --lib <target>=<path> map. Without --require-libs a target with
 * no library in the map gets a headers-only envelope and the
 * omission is printed; with --require-libs it is a hard failure.
 * A manifest's remaining lib/<target>/ entries (crt objects,
 * stub libraries, .def files, the darwin .tbd) are committed
 * sources, resolved under --sysroot-lib the way the package's
 * own bundling resolves them; a missing one is a hard failure,
 * named.
 *
 * The envelope form is the package's asset-envelope format: a
 * classic script registering { files } under the key
 * `sysroot-<target>` in __TINYC_ASSET_REGISTRY__, each file's
 * bytes base64-encoded under its manifest source.
 *
 * Usage:
 *   node assemble_sysroot.mjs --manifests <dir> --out <dir>
 *     --include-root <dir> [--include-root <dir> ...]
 *     --tcclib <path> --sysroot-lib <dir>
 *     [--lib <target>=<path> ...] [--require-libs]
 */
import fs from 'node:fs';
import path from 'node:path';

function parseArgs(argv) {
  const out = { includeRoots: [], libs: new Map(), requireLibs: false };
  for (let i = 0; i < argv.length; i += 1) {
    const a = argv[i];
    const next = () => {
      i += 1;
      if (i >= argv.length) throw new Error(`${a} needs a value`);
      return argv[i];
    };
    if (a === '--manifests') out.manifests = next();
    else if (a === '--out') out.outDir = next();
    else if (a === '--include-root') out.includeRoots.push(next());
    else if (a === '--tcclib') out.tcclib = next();
    else if (a === '--sysroot-lib') out.sysrootLib = next();
    else if (a === '--lib') {
      const v = next();
      const eq = v.indexOf('=');
      if (eq < 0) throw new Error(`--lib expects <target>=<path>, got: ${v}`);
      out.libs.set(v.slice(0, eq), v.slice(eq + 1));
    } else if (a === '--require-libs') out.requireLibs = true;
    else throw new Error(`unknown argument: ${a}`);
  }
  for (const k of ['manifests', 'outDir', 'tcclib']) {
    if (!out[k]) throw new Error(`--${k === 'outDir' ? 'out' : k} is required`);
  }
  if (!out.includeRoots.length) throw new Error('at least one --include-root is required');
  return out;
}

function envelopeFile(key, payload) {
  return (
    'globalThis.__TINYC_ASSET_REGISTRY__ ??= Object.create(null);\n' +
    `globalThis.__TINYC_ASSET_REGISTRY__[${JSON.stringify(key)}] = ` +
    `${JSON.stringify(payload)};\n`
  );
}

const args = parseArgs(process.argv.slice(2));
fs.mkdirSync(args.outDir, { recursive: true });

const problems = [];
const manifestFiles = fs.readdirSync(args.manifests)
  .filter((f) => f.endsWith('.json'))
  .sort();

for (const mf of manifestFiles) {
  const manifest = JSON.parse(fs.readFileSync(path.join(args.manifests, mf), 'utf8'));
  const target = manifest.target;
  const files = {};
  let omittedLibs = 0;
  for (const entry of manifest.includeFiles ?? []) {
    let src = null;
    if (entry.source.startsWith('include/')) {
      const name = entry.source.slice('include/'.length);
      for (const root of args.includeRoots) {
        const p = path.join(root, name);
        if (fs.existsSync(p)) { src = p; break; }
      }
    } else if (entry.source === 'tcclib.h') {
      if (fs.existsSync(args.tcclib)) src = args.tcclib;
    }
    if (!src) {
      problems.push(`${target}: sysroot source not found for manifest entry "${entry.source}"`);
      continue;
    }
    files[entry.source] = fs.readFileSync(src).toString('base64');
  }
  for (const entry of manifest.libFiles ?? []) {
    if (entry.source !== `lib/${target}/libtcc1.a`) {
      const p = args.sysrootLib && path.join(args.sysrootLib, target,
        entry.source.slice(`lib/${target}/`.length));
      if (p && fs.existsSync(p)) {
        files[entry.source] = fs.readFileSync(p).toString('base64');
      } else {
        problems.push(`${target}: sysroot-lib source not found for manifest entry "${entry.source}"`);
      }
      continue;
    }
    const lib = args.libs.get(target);
    if (lib && fs.existsSync(lib)) {
      files[entry.source] = fs.readFileSync(lib).toString('base64');
    } else if (args.requireLibs) {
      problems.push(`${target}: library source not found for manifest entry "${entry.source}"`);
    } else {
      omittedLibs += 1;
    }
  }
  const out = path.join(args.outDir, `sysroot-${target}.tinyc.js`);
  fs.writeFileSync(out, envelopeFile(`sysroot-${target}`, { files }));
  console.log(`assembled ${out}: ${Object.keys(files).length} files` +
    (omittedLibs ? ` (${omittedLibs} library entries omitted: no --lib source for this target)` : ''));
}

if (problems.length) {
  for (const p of problems) console.log(`::error::SYSROOT ASSEMBLY GAP: ${p}`);
  process.exit(1);
}
