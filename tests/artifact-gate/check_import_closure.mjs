#!/usr/bin/env node
/* Import-closure check for an unpacked tinyc-web package directory.
 *
 * The package's ESM entry points (index.mjs, worker.mjs) must be
 * importable from the packaged artifact alone: every relative
 * import they name, transitively, plus every relative URL they
 * construct with new URL(...), must be present in the directory.
 * A file that exists in the source tree but is missing from the
 * packaging manifest (package.json "files", the release zip list)
 * fails here, named.
 *
 * Usage: node check_import_closure.mjs <unpacked-package-dir>
 */
import fs from 'node:fs';
import path from 'node:path';

const root = process.argv[2];
if (!root) {
  console.error('usage: node check_import_closure.mjs <unpacked-package-dir>');
  process.exit(2);
}

const seen = new Set();
const missing = [];
const queue = ['index.mjs', 'worker.mjs'].map((e) => path.join(root, e));
while (queue.length) {
  const file = queue.pop();
  const rel = path.relative(root, file);
  if (seen.has(rel)) continue;
  seen.add(rel);
  if (!fs.existsSync(file)) { missing.push(rel); continue; }
  if (!file.endsWith('.mjs') && !file.endsWith('.js')) continue;
  const text = fs.readFileSync(file, 'utf8');
  const re = /(?:from|import)\s*['"](\.\/[^'"]+)['"]|new URL\(\s*['"](\.\/[^'"]+)['"]/g;
  let m;
  while ((m = re.exec(text))) queue.push(path.join(path.dirname(file), m[1] || m[2]));
}
if (missing.length) {
  for (const f of missing)
    console.log(`::error::PACKAGE CONTENT GAP: '${f}' is imported by the tinyc-web ESM entry points but is missing from the packaged artifact. Add it to the packaging manifest (package.json "files" / the release zip list).`);
  process.exit(1);
}
console.log(`package contents OK: all ${seen.size} files in the ESM import closure are present.`);
