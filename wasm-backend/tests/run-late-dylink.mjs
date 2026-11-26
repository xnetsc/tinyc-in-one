// run-late-dylink.mjs - assertions for the modules that
// test-late-dylink.c produced from the browser's TinycState.run
// sequence (MEMORY at compile time, flavor and EXE at run time).
//   early:      the dylink main module must validate, instantiate
//               with the process imports, and answer() == 42.
//   late:       the run must have failed with the explicit
//               ordering error, and produced no output file.
//   standalone: the bare module must validate and answer() == 42.
// The late case is the CI run 45746072616 failure: a module whose
// bodies were generated standalone but emitted as dylink failed
// WebAssembly.Module validation (immutable global #0 cannot be
// assigned). That combination must now be refused at output time,
// and the early sequence must produce a module the browser accepts.
import { spawnSync } from 'node:child_process';
import { existsSync, readFileSync, rmSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';

const dir = dirname(fileURLToPath(import.meta.url));
let failures = 0;
function check(name, cond) {
  console.log((cond ? 'PASS: ' : 'FAIL: ') + name);
  if (!cond) failures++;
}

function runMode(mode, out) {
  rmSync(out, { force: true });
  const r = spawnSync(join(dir, 'test-late-dylink'), [mode, out],
    { encoding: 'utf8' });
  return { rc: r.status, stdout: r.stdout, stderr: r.stderr };
}

/** Decode a dylink unit file: the envelope registers one JSON
 *  object (the text between the first "{" and the last "}") whose
 *  "body" field is the module's base64 and whose "table_size" is
 *  the number of function-table slots the module reserves past
 *  the reserved slot 0. Returns { bytes, tableSize } or null. */
function unitBody(path) {
  const text = readFileSync(path, 'utf8');
  const start = text.indexOf('{');
  const end = text.lastIndexOf('}');
  if (start < 0 || end <= start) return null;
  const meta = JSON.parse(text.slice(start, end + 1));
  if (typeof meta.body !== 'string') return null;
  return {
    bytes: Buffer.from(meta.body, 'base64'),
    tableSize: meta.table_size | 0,
  };
}

const earlyOut = join(dir, 'ld-early.wasm');
const early = runMode('early', earlyOut);
check('early: output succeeds', early.rc === 0);
check('early: output file exists', existsSync(earlyOut));
let earlyUnit = null;
try {
  earlyUnit = unitBody(earlyOut);
} catch { /* null */ }
check('early: unit file parses to a body', earlyUnit !== null);
if (earlyUnit) {
  const earlyBytes = earlyUnit.bytes;
  let mod = null;
  try { mod = new WebAssembly.Module(earlyBytes); } catch { /* null */ }
  check('early: dylink main module validates', mod !== null);
  if (mod) {
    const memory = new WebAssembly.Memory({ initial: 2 });
    // Slot 0 is reserved; the module's functions land at
    // __table_base (1) and up, table_size slots in all.
    const table = new WebAssembly.Table({
      initial: 1 + earlyUnit.tableSize, element: 'anyfunc' });
    const inst = new WebAssembly.Instance(mod, {
      env: {
        memory,
        __indirect_function_table: table,
        __memory_base: new WebAssembly.Global(
          { value: 'i32', mutable: false }, 0),
        __table_base: new WebAssembly.Global(
          { value: 'i32', mutable: false }, 1),
      },
    });
    inst.exports.__tinyc_dylink_init();
    check('early: answer() == 42', inst.exports.answer() === 42);
  }
}

const lateOut = join(dir, 'ld-late.wasm');
const late = runMode('late', lateOut);
// The harness exits 0 when the refusal happened as expected.
check('late: output is refused', late.rc === 0
  && /output refused as expected/.test(late.stdout));
check('late: error names the ordering',
  /was set after compiling/.test(late.stderr));
check('late: no output file is produced', !existsSync(lateOut));

const saOut = join(dir, 'ld-standalone.wasm');
const sa = runMode('standalone', saOut);
check('standalone: output succeeds', sa.rc === 0);
check('standalone: output file exists', existsSync(saOut));
if (existsSync(saOut)) {
  const bytes = readFileSync(saOut);
  let mod = null;
  try { mod = new WebAssembly.Module(bytes); } catch { /* null */ }
  check('standalone: bare module validates', mod !== null);
  if (mod) {
    const inst = new WebAssembly.Instance(mod, {
      env: new Proxy({}, { get: () => () => 0 }),
    });
    check('standalone: answer() == 42', inst.exports.answer() === 42);
  }
}

console.log(failures
  ? `run-late-dylink: ${failures} failure(s)`
  : 'run-late-dylink: all assertions passed');
process.exit(failures ? 1 : 0);
