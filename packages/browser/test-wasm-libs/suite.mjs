/* The wasm-libs suite, shared by the Node runner (simulated
 * tier) and the page runner (real tier in Chromium, simulated
 * on file://). The environment supplies:
 *   createProcess(options) — the package's createProcess
 *   guestBytes(name)       — unit-file bytes of a compiled guest
 *   units                  — Map: unit basename -> bytes, for
 *                            every loadable unit (the three
 *                            js-kind libraries and the hybrid
 *                            units)
 *   baseUrl                — where units are also served over
 *                            http (the remote-load cases)
 *   tier                   — 'simulated' | 'real' to force
 * Every case runs in a fresh process. Guests print "ok <name>"
 * / "FAIL <name>" lines and exit with their failure count; a
 * case passes when the run exits 0 with no FAIL line.
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

  const freshProcess = (excluded) => {
    let text = '';
    const proc = env.createProcess({
      threads: true,
      threadTier: env.tier,
      baseUrl: env.baseUrl ?? undefined,
      readModule: readModuleExcluding(excluded),
      io: {
        out: (t) => { text += t; },
        err: (t) => { text += t; },
      },
    });
    return { proc, text: () => text };
  };

  const standard = (guest, excluded) => async () => {
    const { proc, text } = freshProcess(excluded);
    const res = await proc.runMainModule(await env.guestBytes(guest));
    if (res.exitCode !== 0) {
      return `exit ${res.exitCode}, want 0; output: ${text()}`;
    }
    if (/^FAIL /m.test(text())) return `guest reported failures: ${text()}`;
    if (!/^done: 0 failures$/m.test(text())) {
      return `guest did not complete cleanly: ${text()}`;
    }
    return null;
  };

  const expectReject = (guest, patterns, excluded) => async () => {
    const { proc, text } = freshProcess(excluded);
    let err = null;
    try {
      await proc.runMainModule(await env.guestBytes(guest));
    } catch (e) {
      err = e;
    }
    if (!err) return `run unexpectedly succeeded; output: ${text()}`;
    const msg = String(err.message ?? err);
    for (const p of patterns) {
      if (!msg.includes(p)) {
        return `rejection does not name ${JSON.stringify(p)}: ${msg}`;
      }
    }
    return null;
  };

  // The three libraries, both usage forms.
  await runCase('crypto-needed', standard('crypto_need'));
  await runCase('crypto-dlopen', standard('crypto_dlopen'));
  await runCase('regex-needed', standard('regex_need'));
  await runCase('regex-dlopen', standard('regex_dlopen'));
  await runCase('curl-needed', standard('curl_need'));
  await runCase('curl-dlopen', standard('curl_dlopen'));
  await runCase('curl-websocket', standard('curl_ws'));

  // No declaration and no dlopen: the symbols have no
  // provider, and the failure names the missing function.
  await runCase('curl-undeclared-fails',
    expectReject('curl_nodecl', ['curl_easy_init']));

  // The hybrid (mixed-kind) unit: both kinds, both call
  // directions, function pointers across kinds, a callback
  // into the guest, and data shared through the module's
  // memory — in both usage forms.
  await runCase('hybrid-needed', standard('hybrid_need'));
  await runCase('hybrid-dlopen', standard('hybrid_dlopen'));

  // A duplicate definition (the same export from both kinds)
  // is refused at load, naming the export.
  await runCase('hybrid-duplicate-refused',
    expectReject('hybrid_bad_need', ['wasm_double', 'defined twice']));

  // Remote js-kind units load like local ones: by dlopen
  // fetch, and as a needed dependency once the host has
  // retrieved the unit from the base URL into the registry
  // (the unit-file evaluation that is the registry's source).
  await runCase('remote-js-dlopen',
    standard('remote_dlopen', new Set(['libpcre2.tinyc.js'])));
  await runCase('remote-js-needed', async () => {
    const { proc, text } = freshProcess(new Set(['libcurl.tinyc.js']));
    await env.prefetchRemote('libcurl.tinyc.js');
    const res = await proc.runMainModule(await env.guestBytes('curl_need'));
    if (res.exitCode !== 0) {
      return `exit ${res.exitCode}, want 0; output: ${text()}`;
    }
    if (/^FAIL /m.test(text())) return `guest reported failures: ${text()}`;
    return null;
  });

  return results;
}

export function summarize(results) {
  let ok = true;
  for (const r of results) {
    console.log(`${r.ok ? 'PASS' : 'FAIL'} ${r.name}${r.ok ? '' : ` — ${r.detail}`}`);
    ok = ok && r.ok;
  }
  const passed = results.filter((r) => r.ok).length;
  console.log(`wasm-libs suite: ${passed}/${results.length} cases passed`);
  return ok;
}
