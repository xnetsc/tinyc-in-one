/* The cstd2 suite, shared by the Node runner (simulated tier)
 * and the page runner (real tier in Chromium, simulated on
 * file://). It covers the wasm32 standard-surface batch:
 *   ④ the libtcc1 unit supplying the compiler's runtime
 *      helpers (__divdi3 and family) to processes and to
 *      standalone runs;
 *   ⑤ taking an imported function's address in standalone
 *      output;
 *   ⑥ time.h declaring time / clock / difftime;
 *   ⑦ errno as a working lvalue over the layer's accessor;
 *   ⑧ getenv wired to the process environment, system with
 *      no-shell semantics;
 *   ⑨ structs passed by value in every size class.
 * The environment supplies:
 *   createProcess(options) — the package's createProcess
 *   guestBytes(name)       — unit-file bytes of a compiled guest
 *   units                  — Map: unit basename -> bytes for
 *                            readModule (libtcc1.tinyc.js)
 *   runStandalone(bytes, options) — run a standalone module
 *                            through a Tinyc instance carrying
 *                            the real sysroot (its libtcc1 unit
 *                            pre-instantiated); resolves to
 *                            { exitCode, out, err }
 *   tier                   — 'simulated' | 'real' to force
 * Guests print "ok <name>" / "FAIL <name>" lines and exit with
 * their failure count; a case passes when the run exits 0 with
 * no FAIL line.
 */

export async function runSuite(env) {
  const results = [];
  const runCase = async (name, fn) => {
    if (env.onCase) env.onCase(name);
    try {
      const detail = await fn();
      results.push({ name, ok: !detail, detail: detail ?? '' });
    } catch (err) {
      results.push({ name, ok: false, detail: String(err && err.message || err) });
    }
  };

  const readModuleExcluding = (excluded) => (canonical) => {
    const base = canonical.slice(canonical.lastIndexOf('/') + 1);
    if (excluded && excluded.has(base)) return undefined;
    return env.units.get(base);
  };

  const freshProcess = (excluded, extra = {}) => {
    let out = '';
    let err = '';
    const proc = env.createProcess({
      threads: true,
      threadTier: env.tier,
      readModule: readModuleExcluding(excluded),
      io: {
        out: (t) => { out += t; },
        err: (t) => { err += t; },
      },
      ...extra,
    });
    return { proc, out: () => out, err: () => err };
  };

  const checkGuestOutput = (out, exitCode) => {
    if (exitCode !== 0) return `exit ${exitCode}, want 0; output: ${out}`;
    if (/^FAIL /m.test(out)) return `guest reported failures: ${out}`;
    if (!/^done: 0 failures$/m.test(out)) {
      return `guest did not complete cleanly: ${out}`;
    }
    return null;
  };

  const standardProcess = (guest, extra) => async () => {
    const { proc, out } = freshProcess(null, extra);
    const res = await proc.runMainModule(await env.guestBytes(guest));
    return checkGuestOutput(out(), res.exitCode);
  };

  const standardStandalone = (guest, options) => async () => {
    const res = await env.runStandalone(await env.guestBytes(guest), options);
    return checkGuestOutput(res.out, res.exitCode);
  };

  // ④ in a process: long long division, remainder and shifts
  // resolve from the libtcc1 unit the loader places first.
  await runCase('lldiv-process', standardProcess('lldiv_probe'));

  // ④ negative control: with the unit withheld, the same guest
  // is refused and the refusal names the missing helper — the
  // unit is what supplies it, not something else in the stack.
  await runCase('lldiv-process-without-unit', async () => {
    const { proc } = freshProcess(new Set(['libtcc1.tinyc.js']));
    let err = null;
    try {
      await proc.runMainModule(await env.guestBytes('lldiv_probe'));
    } catch (e) {
      err = e;
    }
    if (!err) return 'run unexpectedly succeeded without the unit';
    const msg = String(err.message ?? err);
    if (!msg.includes('__divdi3')) {
      return `rejection does not name __divdi3: ${msg}`;
    }
    return null;
  });

  // ④ standalone: the same helpers from the instance's private
  // pre-instantiation of the unit.
  await runCase('lldiv-standalone', standardStandalone('lldiv_probe_sa'));

  // ⑤ in both output forms.
  await runCase('addr-process', standardProcess('addr_probe'));
  await runCase('addr-standalone', standardStandalone('addr_probe_sa'));

  // ⑥ time declarations.
  await runCase('time-process', standardProcess('time_probe'));

  // ⑦ errno, including perror's message text.
  await runCase('errno-process', async () => {
    const { proc, out, err } = freshProcess(null);
    const res = await proc.runMainModule(await env.guestBytes('errno_probe'));
    const bad = checkGuestOutput(out(), res.exitCode);
    if (bad) return bad;
    if (!err().includes('cstd2-perror: No such file or directory')) {
      return `perror text missing or wrong: ${JSON.stringify(err())}`;
    }
    return null;
  });

  // ⑧ getenv / system, in a process created with an environment
  // and in a standalone run handed the same environment.
  const envOption = { env: { CST2_GREETING: 'hello-42' } };
  await runCase('env-process', standardProcess('env_probe', envOption));
  await runCase('env-standalone', standardStandalone('env_probe_sa', envOption));

  // ⑨ structs by value.
  await runCase('struct-process', standardProcess('struct_probe'));

  return results;
}

export function summarize(results) {
  let failed = 0;
  for (const r of results) {
    console.log(`${r.ok ? 'PASS' : 'FAIL'} ${r.name}${r.ok ? '' : `: ${r.detail}`}`);
    if (!r.ok) failed++;
  }
  console.log(`cstd2 suite: ${results.length - failed}/${results.length} passed`);
  return failed === 0;
}

/* A mock native module with the libtcc export names, for
 * loadTinycWithNative: the standalone cases need a Tinyc
 * instance (for its sysroot staging and its pre-instantiated
 * libtcc1 unit), not the compiler itself. */
export function makeMockNative() {
  return {
    main() { return 3; },
    tinyc_main() { return 7; },
    tcc_new() { return { fakeState: true }; },
    tcc_delete() {},
    tcc_set_lib_path() {},
    tcc_set_error_func() {},
    tcc_set_options() { return 0; },
    tcc_add_include_path() { return 0; },
    tcc_add_sysinclude_path() { return 0; },
    tcc_define_symbol() {},
    tcc_undefine_symbol() {},
    tcc_add_file() { return 0; },
    tcc_compile_string() { return 0; },
    tcc_set_output_type() { return 0; },
    tcc_add_library_path() { return 0; },
    tcc_add_library() { return 0; },
    tcc_output_file() { return 0; },
    tcc_relocate() { return 0; },
    tcc_get_symbol() { return 0; },
  };
}

/* Build the standalone runner over one Tinyc instance loaded
 * with the real sysroot envelope (fetchImpl serves it), so the
 * instance's _initLibtcc1 sees the shipped libtcc1 unit exactly
 * as a page load would stage it. The manifest is narrowed to
 * the headers plus the libtcc1 unit: the envelope a local build
 * assembles carries no libtcc1.a (that artifact exists only in
 * the full package build), and the standalone paths never read
 * it — the unit is its wasm form. */
export async function createStandaloneRunner(indexModule, fetchImpl) {
  const full = indexModule.sysrootManifest;
  const manifest = {
    ...full,
    libFiles: full.libFiles.filter(
      (e) => e.path === '/sysroot/lib/libtcc1.tinyc.js'),
  };
  const tinyc = await indexModule.loadTinycWithNative(makeMockNative(), {
    sysroot: { manifest, fetchImpl, includeBinaries: true },
  });
  let n = 0;
  return async (bytes, options = {}) => {
    let out = '';
    let err = '';
    const path = `/cstd2-probe-${n++}.wasm`;
    await tinyc.addFile(path, bytes);
    const exitCode = tinyc.runWasmFile(path, {
      entry: 'main',
      callArgs: [0, 0],
      env: options.env,
      io: {
        out: (t) => { out += t; },
        err: (t) => { err += t; },
      },
    });
    return { exitCode, out, err };
  };
}
