/* The §11 guest suite, shared by the Node runner and the
 * browser page runners. The environment supplies:
 *   createProcess(options)  — the package's createProcess
 *   guestBytes(name)        — unit-file bytes of a compiled guest
 *   baseUrl                 — where absent side modules are
 *                             fetched from (null: local only)
 *   tier                    — 'simulated' | 'real' to force
 *   includeDeadlock         — the deadlock case (simulated only)
 *   includeNoJspi           — handled by the Node runner itself
 * Every case runs in a fresh process and is checked on its
 * printed result lines and its exit code.
 */

const EINVAL = 22, EDEADLK = 35, ESRCH = 3, EBUSY = 16,
  EPERM = 1, ETIMEDOUT = 110;
const RTLD_NOW_GLOBAL = 2 | 4;

function parseLines(text) {
  const out = new Map();
  for (const line of text.split('\n')) {
    const m = /^([a-z0-9_]+): (-?\d+) (-?\d+) (-?\d+) (-?\d+)\s*$/.exec(line.trim());
    if (m) out.set(m[1], [Number(m[2]), Number(m[3]), Number(m[4]), Number(m[5])]);
  }
  return out;
}

function checkLines(lines, expected) {
  for (const [name, want] of Object.entries(expected)) {
    const got = lines.get(name);
    if (!got) return `missing result line "${name}"`;
    for (let i = 0; i < 4; i++) {
      const w = want[i];
      if (Array.isArray(w)) {
        if (got[i] < w[0] || got[i] > w[1]) {
          return `${name}[${i}] = ${got[i]}, want in [${w[0]}, ${w[1]}]`;
        }
      } else if (got[i] !== w) {
        return `${name}[${i}] = ${got[i]}, want ${w}`;
      }
    }
  }
  return null;
}

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

  const freshProcess = (extra = {}) => {
    let text = '';
    const proc = env.createProcess({
      threads: true,
      threadTier: env.tier,
      baseUrl: env.baseUrl ?? undefined,
      readModule: env.readModule,
      io: {
        out: (t) => { text += t; },
        err: (t) => { text += t; },
      },
      ...extra,
    });
    return { proc, text: () => text };
  };

  const standard = (guest, expected, exitCode = 0) => async () => {
    const { proc, text } = freshProcess();
    const res = await proc.runMainModule(await env.guestBytes(guest));
    if (res.exitCode !== exitCode) {
      return `exit ${res.exitCode}, want ${exitCode}; output: ${text()}`;
    }
    return checkLines(parseLines(text()), expected);
  };

  await runCase('create_join', standard('create_join', {
    create_join: [1000, 46, EINVAL, EINVAL],
    create_join2: [EDEADLK, ESRCH, 0, 0],
  }));
  await runCase('mutex_excl', standard('mutex_excl', {
    mutex_excl: [20000, 20000, 0, 0],
  }));
  await runCase('mutex_kinds', standard('mutex_kinds', {
    mutex_kinds: [EDEADLK, EBUSY, EPERM, EBUSY],
  }));
  await runCase('cond_buffer', standard('cond_buffer', {
    cond_buffer: [20, 210, ETIMEDOUT, [800, 8000]],
    cond_bcast: [3, 1, 1, 1],
  }));
  await runCase('once', standard('once', { once: [1, 1, 1, 0] }));
  await runCase('spin', standard('spin', { spin: [20000, 20000, 0, 0] }));
  await runCase('key_tls', standard('key_tls', {
    key_tls: [100, 101, 9, 0],
    key_dtor: [2, 2, 0, 0],
  }));
  await runCase('thread_var', async () => {
    // libtls is a needed dependency of the main module
    // (compiled with --dylink-need), present locally through
    // the harness's readModule.
    const { proc, text } = freshProcess();
    const res = await proc.runMainModule(await env.guestBytes('thread_var'));
    if (res.exitCode !== 0) return `exit ${res.exitCode}; output: ${text()}`;
    return checkLines(parseLines(text()), {
      thread_var: [41, 55, 7, 0],
      thread_seen: [100, 101, 50, 51],
      thread_ext: [200, 201, 0, 0],
    });
  });
  await runCase('atomics_storm', standard('atomics_storm', {
    atomics: [20000, 20000, 20000, 0],
  }));
  await runCase('dlopen_thread', standard('dlopen_thread', {
    dlopen_thread: [42, 43, 0, 0],
  }));
  await runCase('exit7', async () => {
    const { proc } = freshProcess();
    const res = await proc.runMainModule(await env.guestBytes('exit7'));
    return res.exitCode === 7 ? null : `exit ${res.exitCode}, want 7`;
  });
  await runCase('main_exit', async () => {
    const { proc } = freshProcess();
    const res = await proc.runMainModule(await env.guestBytes('main_exit'));
    if (res.exitCode !== 0) return `exit ${res.exitCode}, want 0`;
    const v = proc.readExportI32(proc.moduleHandle('/a.out'), 'worker_result');
    return v === 5050 ? null : `worker_result = ${v}, want 5050`;
  });
  await runCase('plain_dylink_threaded', standard('plain_dylink', {
    plain: [42, 0, 0, 0],
  }));
  await runCase('plain_dylink_unthreaded', async () => {
    let text = '';
    const proc = env.createProcess({
      baseUrl: env.baseUrl ?? undefined,
      readModule: env.readModule,
      io: { out: (t) => { text += t; }, err: (t) => { text += t; } },
    });
    if (proc.threadTier !== null) return 'unthreaded process has a tier';
    // The unthreaded synchronous dlopen needs the module
    // already local (§9.3's rule, unchanged from 0.4.0);
    // readModule is the local source here.
    const res = await proc.runMainModule(await env.guestBytes('plain_dylink'));
    if (res.exitCode !== 0) return `exit ${res.exitCode}; output: ${text}`;
    return checkLines(parseLines(text), { plain: [42, 0, 0, 0] });
  });

  if (env.includeDeadlock) {
    await runCase('deadlock', async () => {
      const { proc } = freshProcess();
      try {
        await proc.runMainModule(await env.guestBytes('deadlock'));
        return 'the run completed; a deadlock error was expected';
      } catch (err) {
        const msg = String(err && err.message || err);
        if (!/deadlock/i.test(msg)) return `error does not name a deadlock: ${msg}`;
        if (!/mutex/.test(msg)) return `error names no mutex: ${msg}`;
        if (!/thread 2/.test(msg) || !/thread 3/.test(msg)) {
          return `error does not name both threads: ${msg}`;
        }
        return null;
      }
    });
  }

  return results;
}

export function summarize(results) {
  const failed = results.filter((r) => !r.ok);
  for (const r of results) {
    console.log(`${r.ok ? 'PASS' : 'FAIL'} ${r.name}${r.detail ? ` — ${r.detail}` : ''}`);
  }
  console.log(`${results.length - failed.length}/${results.length} cases passed`);
  return failed.length === 0;
}
