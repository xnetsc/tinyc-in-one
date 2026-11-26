// Tests for the classic-script bundle (tinyc-web.js, built by
// build-web-bundle.mjs from web-entry.mjs). The bundle is loaded in
// a Node vm context dressed up as a browser page — window/self
// pointing at the context global, a fake document.currentScript and
// location — and exercised there: the TinycWeb global, the asset
// base-URL rules of the script scenario, the inlined sysroot
// manifests, and the same mock-module / hand-written-wasm paths
// test.mjs uses for the ES module build.
//
// Run with: npm run test:web   (node --test test-web-bundle.mjs)
import { test, before } from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { fileURLToPath } from 'node:url';
import * as esm from './index.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const bundlePath = path.join(here, 'tinyc-web.js');

before(() => {
  execFileSync(process.execPath, [path.join(here, 'build-web-bundle.mjs')], {
    stdio: 'pipe',
  });
});

// Load the bundle as a classic script in a fresh fake page and
// return the page global. scriptSrc is the URL the <script> tag
// points at (null = no currentScript, exercising the fallback).
function loadBundle({ scriptSrc = null, pageHref = 'https://example.com/app/page.html' } = {}) {
  const sandbox = {};
  sandbox.window = sandbox;
  sandbox.self = sandbox;
  sandbox.console = console;
  sandbox.URL = URL;
  sandbox.TextEncoder = TextEncoder;
  sandbox.TextDecoder = TextDecoder;
  sandbox.btoa = btoa;
  sandbox.atob = atob;
  sandbox.Blob = Blob;
  sandbox.File = File;
  sandbox.fetch = fetch;
  sandbox.performance = performance;
  sandbox.crypto = crypto;
  sandbox.setTimeout = setTimeout;
  sandbox.clearTimeout = clearTimeout;
  sandbox.queueMicrotask = queueMicrotask;
  sandbox.location = { href: pageHref };
  sandbox.document = { currentScript: scriptSrc ? { src: scriptSrc } : null };
  vm.createContext(sandbox);
  vm.runInContext(fs.readFileSync(bundlePath, 'utf8'), sandbox, {
    filename: 'tinyc-web.js',
    // A real page imports the module URLs the envelope glue is
    // built from; the vm page delegates dynamic import to the host
    // loader. (Needs node --experimental-vm-modules — see the
    // test:web script in package.json.)
    importModuleDynamically: (specifier) => import(specifier),
  });
  return sandbox;
}

// ---------------------------------------------------------------------
// Mock native module: same export names as libtcc.h, JS values pass
// through as-is (mock mode in the bridge). Records every call.
// (Same technique as test.mjs.)
// ---------------------------------------------------------------------
function makeMock(overrides = {}) {
  const calls = [];
  const mock = {
    calls,
    main(argc, argv) { calls.push(['main', argc, argv]); return 3; },
    tinyc_main(argc, argv) { calls.push(['tinyc_main', argc, argv]); return 7; },
    tcc_new() { calls.push(['tcc_new']); return { fakeState: true }; },
    tcc_delete(h) { calls.push(['tcc_delete', h]); },
    tcc_set_lib_path(h, p) { calls.push(['tcc_set_lib_path', h, p]); },
    tcc_set_error_func(h, opaque, fn) { calls.push(['tcc_set_error_func', h]); },
    tcc_set_options(h, s) { calls.push(['tcc_set_options', h, s]); return 0; },
    tcc_add_include_path(h, p) { calls.push(['tcc_add_include_path', h, p]); return 0; },
    tcc_add_sysinclude_path(h, p) { calls.push(['tcc_add_sysinclude_path', h, p]); return 0; },
    tcc_define_symbol(h, s, v) { calls.push(['tcc_define_symbol', h, s, v]); },
    tcc_undefine_symbol(h, s) { calls.push(['tcc_undefine_symbol', h, s]); },
    tcc_add_file(h, f) { calls.push(['tcc_add_file', h, f]); return 0; },
    tcc_compile_string(h, src) { calls.push(['tcc_compile_string', h, src]); return 0; },
    tcc_set_output_type(h, t) { calls.push(['tcc_set_output_type', h, t]); return 0; },
    tcc_add_library_path(h, p) { calls.push(['tcc_add_library_path', h, p]); return 0; },
    tcc_add_library(h, l) { calls.push(['tcc_add_library', h, l]); return 0; },
    tcc_output_file(h, f) { calls.push(['tcc_output_file', h, f]); return 0; },
    tcc_relocate(h) { calls.push(['tcc_relocate', h]); return 0; },
    tcc_get_symbol(h, n) { calls.push(['tcc_get_symbol', h, n]); return 0; },
  };
  return Object.assign(mock, overrides);
}

/** The asset envelope file's text form, exactly as
 *  build-asset-envelopes.mjs writes it. (Same helper as test.mjs.) */
function envelopeText(key, payload) {
  return (
    'globalThis.__TINYC_ASSET_REGISTRY__ ??= Object.create(null);\n' +
    `globalThis.__TINYC_ASSET_REGISTRY__[${JSON.stringify(key)}] = ` +
    `${JSON.stringify(payload)};\n`
  );
}

/** The envelope content a loadModule hook returns in these tests:
 *  a compiler envelope whose glue factory hands back `mock`. The
 *  factory runs in the host realm (the page's dynamic imports are
 *  host-loaded), so the mock is stashed on the host global. */
function mockEnvelope(mock) {
  globalThis.__WEB_BUNDLE_MOCK__ = mock;
  return envelopeText('tcc-wasm32-wasi', {
    glue: 'export default function factory() { return globalThis.__WEB_BUNDLE_MOCK__; }',
    wasm: Buffer.from([0, 97, 115, 109]).toString('base64'),
  });
}

// (module (func (export "add") (param i32 i32) (result i32)
//   local.get 0 local.get 1 i32.add))  — same bytes as test.mjs.
const WASM_ADD = [
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
  0x01, 0x07, 0x01, 0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f,
  0x03, 0x02, 0x01, 0x00,
  0x07, 0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00,
  0x0a, 0x09, 0x01, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b,
];

test('web bundle: TinycWeb global carries the full public API of index.mjs', () => {
  const page = loadBundle({ scriptSrc: 'https://cdn.example.com/tinyc/tinyc-web.js' });
  const api = page.TinycWeb;
  assert.ok(api && typeof api === 'object', 'TinycWeb global exists');
  for (const key of Object.keys(esm)) {
    assert.ok(key in api, `TinycWeb.${key} present`);
  }
  assert.equal(api.default, api.loadTinyc);
  assert.equal(typeof api.loadTinyc, 'function');
  assert.equal(typeof api.TinycState, 'function');
  assert.equal(typeof api.Tinyc, 'function');
  assert.equal(typeof api.VirtualFS, 'function');
  assert.equal(typeof api.runWasmModule, 'function');
  // (instanceof against the bundle's own classes: the page realm has
  // its own Error intrinsic, so host-realm Error does not apply.)
  const err = new api.TinycError('x');
  assert.ok(err instanceof api.TinycError);
  assert.equal(err.name, 'TinycError');
  assert.equal(err.message, 'x');
  assert.ok(new api.TinycMissingImportError(['env.x']) instanceof api.TinycError);
  assert.equal(api.SUPPORTED_TARGETS.length, 9);
  assert.equal(api.sysrootManifest.target, 'wasm32-wasi');
});

test('web bundle: sysroot manifests are inlined for every target', async () => {
  const page = loadBundle({ scriptSrc: 'https://cdn.example.com/tinyc/tinyc-web.js' });
  for (const target of page.TinycWeb.SUPPORTED_TARGETS) {
    const manifest = await page.TinycWeb.loadSysrootManifest(target);
    assert.equal(manifest.target, target);
  }
});

test('web bundle: runWasmModule runs a hand-written module', async () => {
  const page = loadBundle({ scriptSrc: 'https://cdn.example.com/tinyc/tinyc-web.js' });
  // Build the bytes inside the page realm so realm-sensitive checks
  // in the bundle see its own Uint8Array.
  const bytes = vm.runInContext(`new Uint8Array(${JSON.stringify(WASM_ADD)})`, page);
  const result = await page.TinycWeb.runWasmModule(bytes, { entry: 'add', args: [19, 23] });
  assert.equal(result, 42);
});

test('web bundle: loadTinyc builds the module from the envelope the loadModule hook returns', async () => {
  const page = loadBundle({ scriptSrc: 'https://cdn.example.com/tinyc/tinyc-web.js' });
  const mock = makeMock();
  const tinyc = await page.TinycWeb.loadTinyc({ loadModule: async () => mockEnvelope(mock) });
  assert.ok(tinyc instanceof page.TinycWeb.Tinyc);
  assert.ok(tinyc.fs instanceof page.TinycWeb.VirtualFS);
  assert.equal(tinyc.target, 'wasm32-wasi');
  const rc = tinyc.compileString('int answer() { return 42; }');
  assert.equal(rc, 0);
  const compiled = mock.calls.filter((c) => c[0] === 'tcc_compile_string');
  assert.equal(compiled.length, 1);
  assert.equal(compiled[0][2], 'int answer() { return 42; }');
});

test('web bundle: default envelope URL resolves against the script directory', async () => {
  const page = loadBundle({ scriptSrc: 'https://cdn.example.com/libs/tinyc-web.js' });
  let captured;
  await page.TinycWeb.loadTinyc({
    loadModule: async (target, files) => { captured = files; return mockEnvelope(makeMock()); },
  });
  assert.equal(captured.envelopeFile, 'tcc-wasm32-wasi.tinyc.js');
  assert.equal(captured.envelopeUrl, 'https://cdn.example.com/libs/tcc-wasm32-wasi.tinyc.js');
});

test('web bundle: without a script URL, URLs fall back to the page directory', async () => {
  const page = loadBundle({ scriptSrc: null, pageHref: 'https://example.com/app/page.html' });
  let captured;
  await page.TinycWeb.loadTinyc({
    loadModule: async (target, files) => { captured = files; return mockEnvelope(makeMock()); },
  });
  assert.equal(captured.envelopeUrl, 'https://example.com/app/tcc-wasm32-wasi.tinyc.js');
});

test('web bundle: baseUrl overrides the envelope base', async () => {
  const page = loadBundle({ scriptSrc: 'https://cdn.example.com/libs/tinyc-web.js' });
  let captured;
  await page.TinycWeb.loadTinyc({
    baseUrl: 'https://other.example/x',
    loadModule: async (target, files) => { captured = files; return mockEnvelope(makeMock()); },
  });
  assert.equal(captured.envelopeUrl, 'https://other.example/x/tcc-wasm32-wasi.tinyc.js');
});
