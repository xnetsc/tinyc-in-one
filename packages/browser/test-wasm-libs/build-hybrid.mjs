/* Build the hybrid test units (libhybrid.tinyc.js and the
 * duplicate-definition variant libhybridbad.tinyc.js) from
 * guests/hybrid.c + guests/hybrid.js: compile the C side as a
 * dylink side module, then merge the JS part into its
 * manifest — kind "mixed", the JS source in "js"/"js_hash",
 * and one export entry per JS function with origin "js" and
 * table values continuing the wasm body's table. Shared by
 * the Node and page runners. */
import { createHash } from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';

function parseUnitText(text) {
  const start = text.indexOf(' = ') + 3;
  const end = text.lastIndexOf(';');
  return JSON.parse(text.slice(start, end));
}

function envelopeText(fileName, manifest) {
  return 'globalThis.__TINYC_DYLINK_REGISTRY__ ??= Object.create(null);\n' +
    `globalThis.__TINYC_DYLINK_REGISTRY__[${JSON.stringify(fileName)}] = ` +
    `${JSON.stringify(manifest)};\n`;
}

export function buildHybridUnits({ tcc, includes, guestsDir, buildDir }) {
  const sidePath = path.join(buildDir, 'hybrid-side');
  execFileSync(tcc, [
    '-nostdlib', ...includes, '-wasm-dylink=side',
    '-o', sidePath, path.join(guestsDir, 'hybrid.c'),
  ], { stdio: 'inherit' });
  const manifest = parseUnitText(fs.readFileSync(sidePath, 'utf8'));

  const jsSource = fs.readFileSync(path.join(guestsDir, 'hybrid.js'), 'utf8');
  const module = { exports: {} };
  new Function('module', 'exports', jsSource)(module, module.exports);
  const jsExports = module.exports.__exports;
  if (!Array.isArray(jsExports)) throw new Error('hybrid.js: no __exports');

  const makeUnit = (extraJsExports, fileName) => {
    const m = JSON.parse(JSON.stringify(manifest));
    const all = [...jsExports, ...extraJsExports];
    m.kind = 'mixed';
    m.js = Buffer.from(jsSource, 'utf8').toString('base64');
    m.js_hash = createHash('sha256').update(jsSource, 'utf8').digest('hex');
    const base = m.table_size;
    all.forEach((e, i) => {
      m.exports.push({
        name: e.name, kind: 0, value: base + i, origin: 'js', sig: e.sig,
      });
    });
    m.table_size = base + all.length;
    const out = path.join(buildDir, fileName);
    fs.writeFileSync(out, envelopeText(fileName, m));
    return out;
  };

  const good = makeUnit([], 'libhybrid.tinyc.js');
  // The bad variant: the JS part also claims wasm_double,
  // which the wasm body already provides — a duplicate
  // definition the loader must refuse by name.
  const bad = makeUnit(
    [{ name: 'wasm_double', sig: { params: ['i32'], results: ['i32'] } }],
    'libhybridbad.tinyc.js');
  return { good, bad };
}
