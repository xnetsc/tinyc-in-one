// Worker half of the Node sync-tier test: Pyodide runs in a
// worker_threads worker, the tier global is set to "sync" before
// Python starts, the tinyc_web package is copied from this
// repository into Pyodide's site-packages, and the shared case
// list is driven through callPromising — the entry Pyodide's
// run_sync contract requires.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parentPort } from 'node:worker_threads';
import { loadPyodide } from 'pyodide';

const here = path.dirname(fileURLToPath(import.meta.url));
const pkgRoot = path.resolve(here, '..');

globalThis.__TINYC_WEB_TIER__ = 'sync';
globalThis.__TINYC_WEB_TEST_TIER__ = 'sync';

// Explicit stdio callbacks: Pyodide's default Node stream wiring
// has no usable fd for Python's stdout inside a worker thread.
function byteSink(write) {
  let pending = '';
  const dec = new TextDecoder();
  return (byte) => {
    pending += dec.decode(new Uint8Array([byte]), { stream: true });
    let idx;
    while ((idx = pending.indexOf('\n')) >= 0) {
      write(pending.slice(0, idx));
      pending = pending.slice(idx + 1);
    }
  };
}
const pyodide = await loadPyodide({
  stdout: byteSink((line) => console.log(line)),
  stderr: byteSink((line) => console.error(line)),
});
const purelib = pyodide.runPython(
  'import sysconfig; sysconfig.get_paths()["purelib"]');

function copyInto(localDir, vfsDir) {
  pyodide.FS.mkdirTree(vfsDir);
  for (const entry of fs.readdirSync(localDir, { withFileTypes: true })) {
    const local = path.join(localDir, entry.name);
    const vfs = path.join(vfsDir, entry.name);
    if (entry.isDirectory()) copyInto(local, vfs);
    else pyodide.FS.writeFile(vfs, fs.readFileSync(local));
  }
}
copyInto(path.join(pkgRoot, 'tinyc_web'), path.join(purelib, 'tinyc_web'));
pyodide.FS.writeFile('/cases.py', fs.readFileSync(path.join(here, 'cases.py')));
pyodide.runPython('import sys; sys.path.insert(0, "/")');

try {
  const entry = pyodide.runPython('import cases; cases.entry_sync');
  const summary = await entry.callPromising();
  const failed = summary.toJs().failed;
  // The per-case lines are written to a log file directly: Python's
  // print output is not reliably forwarded through the worker's
  // piped stdout before the parent exits.
  const lines = pyodide.runPython('import cases; cases.RESULTS')
    .toJs().map(([name, ok]) => `${ok ? 'PASS' : 'FAIL'} ${name}`);
  lines.push(`CASES DONE ${lines.filter((l) => l.startsWith('PASS')).length}/${lines.length}`);
  fs.writeFileSync(path.join(here, 'node-sync-tier.log'),
    lines.join('\n') + '\n');
  // Let the worker's piped stdio drain before the parent exits.
  await new Promise((r) => setTimeout(r, 1000));
  parentPort.postMessage({ done: true, failed });
} catch (err) {
  try {
    console.error('RESULTS SO FAR:',
      pyodide.runPython('import cases; repr(cases.RESULTS)'));
  } catch { /* cases may not have imported */ }
  const text = String((err && (err.stack || err.message)) || err);
  for (const line of text.split('\n')) {
    for (let i = 0; i < line.length; i += 280) {
      console.error('NODE SYNC TIER ERROR:', line.slice(i, i + 280));
    }
  }
  await new Promise((r) => setTimeout(r, 1000));
  parentPort.postMessage({ done: true, failed: ['<harness exception>'] });
}
