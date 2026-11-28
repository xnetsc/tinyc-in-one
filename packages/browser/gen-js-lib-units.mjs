#!/usr/bin/env node
/*
 * Generate the js-kind library units of the wasm32 sysroot:
 * libcurl.tinyc.js, libpcre2.tinyc.js, libmbedtls.tinyc.js.
 *
 * Each unit's implementation source lives in libsrc/ and
 * declares its own export table (module.exports.__exports:
 * name, signature, async flag), so the manifest written here
 * is derived from the implementation itself — there is no
 * second symbol list to drift. The unit file is the standard
 * dylink envelope (the manifest JSON registered under the unit
 * file name), with "kind":"js": the "js" field carries the
 * implementation source (base64) and "js_hash" its SHA-256,
 * in the same discipline as the wasm kind's body/hash.
 *
 * The outputs are committed files under
 * sysroot-lib/wasm32-wasi/; build-sysroot-envelopes.mjs runs
 * this script before staging the wasm32-wasi target so a
 * sysroot rebuild always carries the current sources.
 *
 * Usage: node gen-js-lib-units.mjs [--check]
 *   --check: verify the committed units match the sources
 *   without writing (exit 1 on drift).
 */
import { createHash } from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const LIBS = ['curl', 'pcre2', 'mbedtls'];
const checkOnly = process.argv.includes('--check');

function buildUnit(name) {
  const sourcePath = path.join(here, 'libsrc', `lib${name}.js`);
  const source = fs.readFileSync(sourcePath, 'utf8');
  const module = { exports: {} };
  new Function('module', 'exports', source)(module, module.exports);
  const table = module.exports.__exports;
  if (!Array.isArray(table) || table.length === 0) {
    throw new Error(`${sourcePath}: no __exports table`);
  }
  const exportsList = table.map((e, i) => {
    if (typeof module.exports[e.name] !== 'function') {
      throw new Error(`${sourcePath}: __exports names "${e.name}", ` +
        'which the source does not provide as a function');
    }
    const entry = {
      name: e.name, kind: 0, value: i, origin: 'js', sig: e.sig,
    };
    if (e.async) entry.async = 1;
    return entry;
  });
  const sourceBytes = Buffer.from(source, 'utf8');
  const manifest = {
    version: 1,
    kind: 'js',
    flavor: 0,
    mem_size: 0,
    mem_align: 16,
    bss_off: 0,
    bss_size: 0,
    table_size: exportsList.length,
    needed: [],
    search_dirs: ['$ORIGIN', 'lib', '.'],
    data_got_func: [],
    exports: exportsList,
    js: sourceBytes.toString('base64'),
    js_hash: createHash('sha256').update(sourceBytes).digest('hex'),
  };
  const fileName = `lib${name}.tinyc.js`;
  const text =
    'globalThis.__TINYC_DYLINK_REGISTRY__ ??= Object.create(null);\n' +
    `globalThis.__TINYC_DYLINK_REGISTRY__[${JSON.stringify(fileName)}] = ` +
    `${JSON.stringify(manifest)};\n`;
  return { fileName, text };
}

let drift = 0;
for (const name of LIBS) {
  const { fileName, text } = buildUnit(name);
  const outPath = path.join(here, 'sysroot-lib', 'wasm32-wasi', fileName);
  if (checkOnly) {
    const current = fs.existsSync(outPath) ? fs.readFileSync(outPath, 'utf8') : null;
    if (current !== text) {
      console.error(`gen-js-lib-units: ${fileName} is out of date with its libsrc source`);
      drift++;
    }
    continue;
  }
  fs.mkdirSync(path.dirname(outPath), { recursive: true });
  fs.writeFileSync(outPath, text);
  console.log(`gen-js-lib-units: wrote sysroot-lib/wasm32-wasi/${fileName} (${text.length} bytes)`);
}
if (checkOnly) {
  if (drift) process.exit(1);
  console.log('gen-js-lib-units: all units match their sources');
}
