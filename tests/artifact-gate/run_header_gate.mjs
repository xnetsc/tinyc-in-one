#!/usr/bin/env node
/* Shipped-header gate and real-compile suite for the tinyc-web
 * package, run against an unpacked packaged artifact.
 *
 * Everything under test comes out of the artifact directory: the
 * package's own index.mjs, the per-target compiler envelopes, and
 * the per-target sysroot manifests. The sysroot itself is the
 * envelope set assembled from those manifests (see
 * assemble_sysroot.mjs) and served from --sysroot-dir; nothing is
 * imported from a source tree and no part of the compiler is
 * mocked.
 *
 * For every target the gate:
 *   1. loads the target's compiler from its shipped envelope and
 *      the target's sysroot from the assembled envelope;
 *   2. compiles a translation unit that #includes each header the
 *      manifest ships, one header at a time. A header must compile
 *      standalone, except the registered expectations in
 *      header_gate_expectations.json, which must fail with the
 *      registered diagnostic signature instead. The include form
 *      is the manifest path relative to the sysroot include
 *      directory, so a header in a subdirectory (sys/types.h,
 *      curl/curl.h, mbedtls/aes.h) is tested under the name a
 *      program actually includes it under.
 *
 * For wasm32-wasi (the runnable target) it additionally compiles
 * and runs whole programs through the packaged API: a plain
 * function, an <stdatomic.h> program, and a pthread program whose
 * threads accumulate into a shared atomic counter. These run in
 * the process layer's simulated tier, so the Node running this
 * script needs JSPI (the workflow runs it under the emsdk Node
 * with --experimental-wasm-jspi). --headers-only skips them.
 *
 * Usage:
 *   node run_header_gate.mjs --pkg <unpacked-package-dir>
 *     --sysroot-dir <assembled-sysroot-dir>
 *     [--targets <target>[,<target>...]] [--headers-only]
 *     [--expectations <path>]
 */
import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

function parseArgs(argv) {
  const out = { headersOnly: false };
  for (let i = 0; i < argv.length; i += 1) {
    const a = argv[i];
    const next = () => {
      i += 1;
      if (i >= argv.length) throw new Error(`${a} needs a value`);
      return argv[i];
    };
    if (a === '--pkg') out.pkg = next();
    else if (a === '--sysroot-dir') out.sysrootDir = next();
    else if (a === '--targets') out.targets = next().split(',');
    else if (a === '--expectations') out.expectations = next();
    else if (a === '--headers-only') out.headersOnly = true;
    else throw new Error(`unknown argument: ${a}`);
  }
  if (!out.pkg) throw new Error('--pkg is required');
  if (!out.sysrootDir) throw new Error('--sysroot-dir is required');
  return out;
}

const args = parseArgs(process.argv.slice(2));
const pkgDir = path.resolve(args.pkg);
const expectationsPath = args.expectations ??
  path.join(path.dirname(new URL(import.meta.url).pathname), 'header_gate_expectations.json');
const { expectations } = JSON.parse(fs.readFileSync(expectationsPath, 'utf8'));

const { loadTinyc } = await import(pathToFileURL(path.join(pkgDir, 'index.mjs')).href);

const manifestsDir = path.join(pkgDir, 'sysroot-manifests');
const allTargets = fs.readdirSync(manifestsDir)
  .filter((f) => f.endsWith('.json'))
  .map((f) => f.slice(0, -'.json'.length))
  .sort();
const targets = args.targets ?? allTargets;

let failures = 0;
let checks = 0;
function report(ok, label, detail = '') {
  checks += 1;
  if (!ok) failures += 1;
  console.log(`${ok ? 'PASS' : 'FAIL'} ${label}${detail ? ` — ${detail}` : ''}`);
}

function expectationFor(header, target) {
  for (const e of expectations) {
    // An entry whose header names a path (contains '/') matches
    // that include form exactly: a fragment in a subdirectory
    // (bits/byteswap.h) is a different assertion from a public
    // header of the same basename (byteswap.h). A bare basename
    // entry matches any include form with that basename.
    if (e.header.includes('/')) {
      if (e.header !== header) continue;
    } else if (e.header !== header.split('/').pop()) continue;
    if (e.targets === 'all') return e;
    if (e.targets.startsWith('all-except:')) {
      if (!e.targets.slice('all-except:'.length).split(',').includes(target)) return e;
    } else if (e.targets.split(',').includes(target)) return e;
  }
  return null;
}

const dec = new TextDecoder();

for (const target of targets) {
  const manifestPath = path.join(manifestsDir, `${target}.json`);
  if (!fs.existsSync(manifestPath)) {
    report(false, `${target}: sysroot manifest present in the artifact`, 'manifest file missing');
    continue;
  }
  const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
  const compilerEnvelopePath = path.join(pkgDir, `tcc-${target}.tinyc.js`);
  if (!fs.existsSync(compilerEnvelopePath)) {
    report(false, `${target}: compiler envelope present in the artifact`, 'envelope file missing');
    continue;
  }
  const sysrootEnvelopePath = path.join(args.sysrootDir, `sysroot-${target}.tinyc.js`);
  if (!fs.existsSync(sysrootEnvelopePath)) {
    report(false, `${target}: assembled sysroot envelope present`, 'envelope file missing');
    continue;
  }
  const compilerEnvelope = fs.readFileSync(compilerEnvelopePath, 'utf8');
  const sysrootEnvelope = fs.readFileSync(sysrootEnvelopePath, 'utf8');

  let errText = '';
  const io = {
    err: (b) => { errText += typeof b === 'string' ? b : dec.decode(b, { stream: true }); },
  };
  let tinyc;
  try {
    tinyc = await loadTinyc({
      target,
      io,
      loadModule: async () => compilerEnvelope,
      sysroot: {
        manifest,
        fetchImpl: async () => ({
          ok: true,
          status: 200,
          text: async () => sysrootEnvelope,
        }),
      },
    });
  } catch (e) {
    report(false, `${target}: compiler and sysroot load from the artifact`, String(e && e.message || e));
    continue;
  }
  report(true, `${target}: compiler and sysroot load from the artifact`);

  async function tryCompile(source) {
    errText = '';
    const st = tinyc.createState();
    try {
      st.compileString(source);
      return { ok: true, diagnostic: '' };
    } catch (e) {
      const lines = errText.trim().split('\n').filter(Boolean);
      return { ok: false, diagnostic: lines.length ? lines[lines.length - 1] : String(e && e.message || e), all: errText };
    } finally {
      st.delete();
    }
  }

  for (const entry of manifest.includeFiles ?? []) {
    // The include form is the manifest path relative to the
    // sysroot include directory, not the basename: a header in a
    // subdirectory is only includable under its relative path
    // (sys/types.h, curl/curl.h, mbedtls/aes.h), and the basename
    // form can silently test a different top-level header that
    // shares the name (sys/time.h vs time.h). Top-level headers
    // are unaffected: their relative path is the basename.
    const includeForm = entry.path.startsWith('/sysroot/include/')
      ? entry.path.slice('/sysroot/include/'.length)
      : entry.path.split('/').pop();
    const exp = expectationFor(includeForm, target);
    const r = await tryCompile(`#include <${includeForm}>\nint tinyc_gate_marker;`);
    if (!exp) {
      report(r.ok, `${target}: #include <${includeForm}> compiles standalone`,
        r.ok ? '' : r.diagnostic);
    } else if (exp.expect === 'error') {
      const sigOk = !r.ok && (r.all ?? r.diagnostic).includes(exp.diagnostic_includes);
      report(sigOk, `${target}: #include <${includeForm}> fails with its registered signature`,
        r.ok
          ? 'compiled successfully; the registered expectation says it must not'
          : sigOk
            ? ''
            : `diagnostic does not carry the registered signature "${exp.diagnostic_includes}": ${r.diagnostic}`);
    }
  }

  if (!args.headersOnly && target === 'wasm32-wasi') {
    // Whole programs, compiled and run through the packaged API.
    {
      const st = tinyc.createState();
      let value;
      try {
        st.compileString('int answer(void) { return 42; }');
        value = await st.run([], { entry: 'answer' });
      } catch (e) {
        value = `error: ${e && e.message || e}`;
      } finally {
        st.delete();
      }
      report(value === 42, `${target}: program compiled and run (answer() == 42)`,
        value === 42 ? '' : `got ${value}`);
    }
    {
      const st = tinyc.createState();
      let value;
      try {
        st.compileString([
          '#include <stdatomic.h>',
          'static _Atomic int counter;',
          'int main(void) {',
          '  for (int i = 0; i < 100; i++) atomic_fetch_add(&counter, 1);',
          '  return atomic_load(&counter) == 100 ? 42 : 2;',
          '}',
        ].join('\n'));
        value = await st.run([]);
      } catch (e) {
        value = `error: ${e && e.message || e}`;
      } finally {
        st.delete();
      }
      report(value === 42, `${target}: <stdatomic.h> program runs (counter == 100)`,
        value === 42 ? '' : `got ${value}`);
    }
    {
      const st = tinyc.createState();
      let value;
      try {
        st.compileString([
          '#include <pthread.h>',
          '#include <stdatomic.h>',
          'static _Atomic int shared;',
          'static void *work(void *p) {',
          '  (void)p;',
          '  for (int i = 0; i < 25000; i++) atomic_fetch_add(&shared, 1);',
          '  return 0;',
          '}',
          'int main(void) {',
          '  pthread_t th[4];',
          '  for (int i = 0; i < 4; i++)',
          '    if (pthread_create(&th[i], 0, work, 0)) return 1;',
          '  for (int i = 0; i < 4; i++) pthread_join(th[i], 0);',
          '  return atomic_load(&shared) == 100000 ? 42 : 2;',
          '}',
        ].join('\n'));
        value = await st.run([]);
      } catch (e) {
        value = `error: ${e && e.message || e}`;
      } finally {
        st.delete();
      }
      report(value === 42, `${target}: pthread program runs (4 threads x 25000 == 100000)`,
        value === 42 ? '' : `got ${value}`);
    }
  }
}

console.log(`HEADER GATE: ${checks - failures}/${checks} checks passed`);
process.exit(failures ? 1 : 0);
