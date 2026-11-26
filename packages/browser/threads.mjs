/* Thread support for dylink processes (threads design): the
 * simulated tier's scheduler, the pthread implementation shared
 * by both tiers' shim sets, and — for the real tier — the
 * worker-side runtime and coordinator protocol helpers.
 *
 * The pthread implementation lives here, in the process layer,
 * as functions over guest addresses and thread ids (design
 * §4.5/§10): direct modules reach it through the env shims
 * built below, and a translated bundle's runtime reaches the
 * same functions through the process services.
 *
 * Simulated-tier execution model (§4): exactly one guest
 * thread runs at a time. A thread's stack is parked by JSPI —
 * its entry is invoked through WebAssembly.promising and a
 * blocking shim returns a promise (through WebAssembly.
 * Suspending) exactly when it blocks. The scheduler's pump
 * resolves a parked thread's promise only when the rotation
 * selects that thread: wake and selection are separate steps,
 * so a wake recorded by one thread never resumes another
 * thread inside the first thread's operation.
 */

import { createDefaultShims, ShimExit } from './runtime-shims.mjs';

// -- error codes (the values the pthread.h of this ABI fixes) --
export const EPERM = 1, ESRCH = 3, EAGAIN = 11, EBUSY = 16,
  EINVAL = 22, EDEADLK = 35, ETIMEDOUT = 110;

// -- reserved header words (design §5.6) --
export const HDR_KEY_COUNTER = 524, HDR_TID_COUNTER = 528;

const PTHREAD_KEYS_MAX = 128;
const PTHREAD_DESTRUCTOR_ITERATIONS = 4;
const DESTRUCTOR_ITERATIONS = 4;
const SLICE_MS = 16;          // §4.1 rotation slice budget
const SPIN_BUDGET = 2048;     // §4.4 spin iterations before a yield

/** Thrown by the pthread_exit shim to unwind the calling
 *  thread's stack; the entry wrapper turns it into the
 *  thread's exit value. Distinct from the exit() path: a
 *  thread that pthread_exits runs its TLS destructors, a
 *  thread caught by exit() does not (§2.3). */
export class ThreadExitSignal extends Error {
  constructor(value) {
    super('tinyc: pthread_exit');
    this.name = 'ThreadExitSignal';
    this.value = value | 0;
  }
}

function jspiError(what) {
  return new Error(
    `tinyc: ${what}: the simulated thread tier needs to park a ` +
    `direct module's stack, which requires WebAssembly.promising ` +
    `(JSPI); this engine does not provide it`);
}

let nextLocalThreadKey = 1;

export class ThreadRecord {
  constructor(id) {
    this.id = id;
    this.state = 'runnable';   // runnable | running | blocked | exited
    this.detached = false;
    this.joined = false;       // a join has been claimed
    this.exitValue = 0;
    this.joiners = [];         // threads blocked joining this one
    this.waitInfo = null;      // what a blocked thread waits on
    this.parkResolve = null;   // resolves the shim's park promise
    this.wakeValue = undefined;
    this.timer = null;         // pending timed-wait timer handle
    this.stepSettle = null;    // resolves the pump's step await
    this.savedSp = new Map();  // record -> saved stack pointer
    this.keyValuesAddr = 0;    // this thread's pthread_key value array
    this.invoke = null;        // () -> promise of the entry outcome
    this.started = false;
    this.localKey = nextLocalThreadKey++;
  }
}

/** The simulated tier's scheduler (design §4). Owned by the
 *  process; drives thread steps and host operations from one
 *  pump loop for the process's whole life. */
export class Scheduler {
  constructor(proc) {
    this.proc = proc;
    this.threads = new Map();
    this.joinedIds = new Set();  // tombstones for §6.2's EINVAL
    this.runnable = [];
    this.hostOps = [];
    this.current = null;       // ThreadRecord | 'host' | null
    this.hostDriving = false;  // a host op runs on thread 1's context
    this.contextOwner = null;  // thread whose sp/TLS are installed
    this.mainThread = null;
    this.mutexQueues = new Map();  // mutex addr -> [ThreadRecord]
    this.condQueues = new Map();   // cond addr -> [ThreadRecord]
    this.onceQueues = new Map();   // once addr -> [ThreadRecord]
    this.keyDtors = new Array(PTHREAD_KEYS_MAX).fill(0);
    this.keyLive = new Array(PTHREAD_KEYS_MAX).fill(false);
    this.processEnd = null;    // {type:'exit'|'entry'|'all-exited'|'fault'|'deadlock', ...}
    this.drivingRun = false;
    this._completion = null;   // {promise, resolve} for the run driver
    this._pumpRunning = false;
    this._nudgeResolve = null;
    this._sliceStart = 0;
  }

  // -- registry ---------------------------------------------------

  createMainThread() {
    const t = new ThreadRecord(1);
    this.mainThread = t;
    this.threads.set(1, t);
    this.contextOwner = t;
    return t;
  }

  /** The thread whose identity the current context acts with:
   *  the running thread, or the main thread while a host
   *  operation drives its context. Primitives that do not need
   *  to block work from either; park() is the single gate that
   *  fails by name when a host context would have to block
   *  (§4.1: the host has no stack to park). */
  actingThread() {
    if (this.current instanceof ThreadRecord) return this.current;
    return this.mainThread;
  }

  // -- pump ---------------------------------------------------------

  /** Start the pump if it is not running. The pump is a
   *  process-lifetime async loop; it idles on a nudge promise
   *  when there is nothing to do. */
  kick() {
    if (this._pumpRunning) return;
    this._pumpRunning = true;
    this._sliceStart = performance.now();
    (async () => {
      try {
        await this._pumpLoop();
      } catch (err) {
        if (!this.processEnd) {
          this.processEnd = { type: 'fault', error: err };
        }
        this._settleRun();
      } finally {
        this._pumpRunning = false;
      }
    })();
  }

  _nudge() {
    const r = this._nudgeResolve;
    this._nudgeResolve = null;
    if (r) r();
  }

  _waitNudge() {
    return new Promise((res) => { this._nudgeResolve = res; });
  }

  async _pumpLoop() {
    for (;;) {
      if (this.processEnd) { this._settleRun(); return; }
      const op = this.hostOps.shift();
      if (op) { await this._runHostOp(op); continue; }
      const thread = this.runnable.shift();
      if (thread) {
        if (thread.state === 'runnable') {
          if (performance.now() - this._sliceStart > SLICE_MS) {
            // §4.1: the slice is exhausted while threads remain
            // runnable — return to the outer layer briefly so
            // the host event loop is not starved.
            await new Promise((r) => setTimeout(r, 0));
            this._sliceStart = performance.now();
          }
          await this._step(thread);
          continue;
        }
        continue;
      }
      // Standstill: nobody runnable, no host op pending.
      if (this.drivingRun) {
        const cls = this._classifyStandstill();
        if (cls === 'deadlock') {
          this.processEnd = { type: 'deadlock', error: this._deadlockError() };
          this._settleRun();
          return;
        }
        if (cls === 'all-exited') {
          this.processEnd = { type: 'all-exited' };
          this._settleRun();
          return;
        }
      }
      await this._waitNudge();
    }
  }

  /** One rotation step: install the thread's context, start or
   *  resume it, and wait until it parks, yields or settles. */
  async _step(thread) {
    this.current = thread;
    thread.state = 'running';
    this.proc._installThreadContext(thread);
    const stepPromise = new Promise((res) => { thread.stepSettle = res; });
    if (!thread.started) {
      thread.started = true;
      let p;
      try {
        p = thread.invoke();
      } catch (err) {
        p = Promise.reject(err);
      }
      Promise.resolve(p).then(
        (value) => this._entrySettled(thread, { ok: true, value }),
        (err) => this._entrySettled(thread, { ok: false, err }));
    } else {
      const res = thread.parkResolve;
      thread.parkResolve = null;
      const wake = thread.wakeValue;
      thread.wakeValue = undefined;
      if (res) res(wake);
    }
    await stepPromise;
    thread.stepSettle = null;
    this.current = null;
  }

  _settleStep(thread) {
    const s = thread.stepSettle;
    thread.stepSettle = null;
    if (s) s();
  }

  /** The entry invocation of a thread returned or threw. The
   *  outcome is classified here, once: exit() from any thread
   *  ends the process (rule 1), pthread_exit and plain returns
   *  are per-thread exits, the main thread's entry return ends
   *  the process (rule 2), anything else is a process fault. */
  _entrySettled(thread, outcome) {
    if (thread.state === 'exited') { this._settleStep(thread); return; }
    if (!outcome.ok) {
      const err = outcome.err;
      if (err instanceof ThreadExitSignal) {
        this._exitThread(thread, err.value);
      } else if (isProcessExit(err)) {
        if (!this.processEnd) {
          this.processEnd = { type: 'exit', code: err.exitCode | 0 };
        }
        thread.state = 'exited';
        this._settleRun();
      } else {
        if (!this.processEnd) {
          this.processEnd = {
            type: 'fault',
            error: err instanceof WebAssembly.RuntimeError
              ? new Error(`tinyc: thread ${thread.id} trapped: ${err.message}`)
              : err,
          };
        }
        thread.state = 'exited';
        this._settleRun();
      }
      this._settleStep(thread);
      return;
    }
    // The entry returned a value.
    if (thread === this.mainThread && thread.mainEntry) {
      if (!this.processEnd) {
        this.processEnd = { type: 'entry', result: outcome.value };
      }
      thread.state = 'exited';
      this._settleRun();
      this._settleStep(thread);
      return;
    }
    this._exitThread(thread, outcome.value | 0);
    this._settleStep(thread);
  }

  /** Per-thread exit processing (§6.6), in order: TLS key
   *  destructors on the thread's own context, exit value
   *  stored, joiners woken, detached records reaped. */
  _exitThread(thread, value) {
    this.proc._installThreadContext(thread);
    this.current = thread;
    try {
      this._runKeyDestructors(thread);
    } catch (err) {
      if (!this.processEnd) {
        this.processEnd = { type: 'fault', error: err };
      }
      this._settleRun();
    } finally {
      this.current = null;
    }
    thread.exitValue = value | 0;
    thread.state = 'exited';
    const joiners = thread.joiners.splice(0);
    for (const j of joiners) this.wake(j, 'joined');
    if (thread.detached) this._reapThread(thread);
    // Rule 3 bookkeeping: if every thread has now exited, the
    // pump's standstill classification ends the process.
    this._nudge();
  }

  _reapThread(thread) {
    this.threads.delete(thread.id);
    // Stack regions and TLS areas are retired, never reused:
    // forgetting the record is the whole of reaping (§3.1).
    for (const record of this.proc._loadOrder) {
      record.stackRegions?.delete(thread.id);
      record.tlsAreas?.delete(thread.id);
    }
  }

  _runKeyDestructors(thread) {
    const proc = this.proc;
    if (!thread.keyValuesAddr) return;
    for (let pass = 0; pass < DESTRUCTOR_ITERATIONS; pass++) {
      let called = false;
      for (let key = 0; key < PTHREAD_KEYS_MAX; key++) {
        const dtor = this.keyDtors[key];
        if (!dtor) continue;
        const value = proc._readI32(thread.keyValuesAddr + key * 4);
        if (value === 0) continue;
        proc._writeI32(thread.keyValuesAddr + key * 4, 0);
        called = true;
        const fn = proc.table.get(dtor);
        if (typeof fn === 'function') fn(value);
      }
      if (!called) break;
    }
  }

  // -- parking and waking ------------------------------------------

  /** Park the calling thread on a wait the caller has already
   *  queued it for. Returns the promise the blocking shim hands
   *  to the engine; it resolves with the wake value when the
   *  rotation selects the thread again. */
  park(thread, waitInfo) {
    if (this.hostDriving || !(thread instanceof ThreadRecord)) {
      throw new Error(
        'tinyc: a blocking primitive was reached from the host ' +
        'context, which has no stack to park (threads design §4.1)');
    }
    if (!this.proc._jspi) throw jspiError('blocking primitive');
    thread.state = 'blocked';
    thread.waitInfo = waitInfo;
    thread.parkToken = (thread.parkToken ?? 0) + 1;
    const p = new Promise((res) => { thread.parkResolve = res; });
    this._settleStep(thread);
    return p;
  }

  /** Park with a deadline: whichever of wake and timeout comes
   *  first resumes the thread; the wake value says which. The
   *  deadline survives queue transfers (a cond waiter moved to
   *  its mutex's queue keeps the same park token). */
  parkTimed(thread, waitInfo, deadlineMs) {
    const p = this.park(thread, waitInfo);
    const token = thread.parkToken;
    const delay = Math.max(0, deadlineMs - Date.now());
    thread.timer = setTimeout(() => {
      thread.timer = null;
      if (thread.state === 'blocked' && thread.parkToken === token) {
        this._withdraw(thread);
        this.wake(thread, 'timeout');
      }
    }, delay);
    return p;
  }

  /** Park on a host resource (a retrieval). The resource's
   *  completion wakes the thread; the pump counts the wait as
   *  outstanding, never as deadlock. */
  parkOnHost(thread, startResource) {
    const p = this.park(thread, { kind: 'host' });
    let resource;
    try {
      resource = startResource();
    } catch (err) {
      resource = Promise.reject(err);
    }
    Promise.resolve(resource).then(
      () => this.wake(thread, 'host'),
      () => this.wake(thread, 'host'));
    return p;
  }

  /** Record a wake: the thread becomes runnable; the pump
   *  resumes it when the rotation reaches it. Never resumes
   *  inline (§4.1). */
  wake(thread, value) {
    if (thread.state !== 'blocked') return;
    if (thread.timer) { clearTimeout(thread.timer); thread.timer = null; }
    thread.state = 'runnable';
    thread.waitInfo = null;
    thread.wakeValue = value;
    this.runnable.push(thread);
    this._nudge();
  }

  /** A yield: the step ends and the thread re-queues at the
   *  tail (§4.4). The returned promise resolves when the
   *  rotation selects the thread again. */
  yieldThread(thread) {
    if (this.hostDriving || !(thread instanceof ThreadRecord)) {
      throw new Error(
        'tinyc: a yield was reached from the host context, which ' +
        'has no stack to park (threads design §4.1)');
    }
    if (!this.proc._jspi) throw jspiError('sched_yield');
    thread.state = 'runnable';
    this.runnable.push(thread);
    const p = new Promise((res) => { thread.parkResolve = res; });
    this._settleStep(thread);
    this._nudge();
    return p;
  }

  /** Remove a blocked thread from whatever queue holds it
   *  (timeout withdrawal, §6.3). Queue entries are cleaned
   *  lazily elsewhere; here the waitInfo identity is what
   *  matters. */
  _withdraw(thread) {
    const info = thread.waitInfo;
    if (!info) return;
    const queue = info.kind === 'mutex' ? this.mutexQueues.get(info.addr)
      : info.kind === 'cond' ? this.condQueues.get(info.addr)
      : info.kind === 'once' ? this.onceQueues.get(info.addr)
      : null;
    if (queue) {
      const i = queue.indexOf(thread);
      if (i >= 0) queue.splice(i, 1);
    }
    if (info.kind === 'join' && info.target) {
      const i = info.target.joiners.indexOf(thread);
      if (i >= 0) info.target.joiners.splice(i, 1);
    }
  }

  enqueue(queueMap, addr, thread) {
    let q = queueMap.get(addr);
    if (!q) { q = []; queueMap.set(addr, q); }
    q.push(thread);
  }

  dequeue(queueMap, addr) {
    const q = queueMap.get(addr);
    if (!q) return null;
    while (q.length > 0) {
      const t = q.shift();
      if (t.state === 'blocked' && t.waitInfo) return t;
    }
    return null;
  }

  // -- host operations (§4.1) ---------------------------------------

  /** Run a host operation that executes guest code through the
   *  rotation, on the main thread's context. Returns a promise
   *  for the operation's result. */
  enqueueHostOp(fn) {
    return new Promise((resolve, reject) => {
      if (this.processEnd) {
        reject(new Error('tinyc: the process has ended'));
        return;
      }
      this.hostOps.push({ fn, resolve, reject });
      this.kick();
      this._nudge();
    });
  }

  async _runHostOp(op) {
    const main = this.mainThread;
    this.current = main;
    this.hostDriving = true;
    this.proc._installThreadContext(main);
    try {
      const value = await op.fn();
      op.resolve(value);
    } catch (err) {
      if (isProcessExit(err) && !this.processEnd) {
        this.processEnd = { type: 'exit', code: err.exitCode | 0 };
        this._settleRun();
      }
      op.reject(err);
    } finally {
      this.hostDriving = false;
      this.current = null;
    }
  }

  // -- run driving ----------------------------------------------------

  /** The promise a threaded runMainModule awaits: resolves
   *  with the processEnd record. */
  runCompletion() {
    if (!this._completion) {
      let resolve;
      const promise = new Promise((res) => { resolve = res; });
      this._completion = { promise, resolve };
    }
    return this._completion.promise;
  }

  _settleRun() {
    if (this._completion && this.processEnd) {
      const c = this._completion;
      this._completion = null;
      c.resolve(this.processEnd);
    }
    // Host operations still queued at process end fail by name.
    if (this.processEnd) {
      for (const op of this.hostOps.splice(0)) {
        op.reject(new Error('tinyc: the process has ended'));
      }
    }
    this._nudge();
  }

  // -- standstill classification (§4.6) -------------------------------

  _classifyStandstill() {
    let live = 0;
    let syncBlocked = 0;
    for (const t of this.threads.values()) {
      if (t.state === 'exited') continue;
      live++;
      if (t.state === 'blocked') {
        const kind = t.waitInfo?.kind;
        if (kind === 'host' || t.timer) return 'waiting';
        syncBlocked++;
      }
    }
    if (live === 0) return 'all-exited';
    if (syncBlocked === live) return 'deadlock';
    return 'waiting';
  }

  _deadlockError() {
    const parts = [];
    for (const t of this.threads.values()) {
      if (t.state !== 'blocked' || !t.waitInfo) continue;
      const info = t.waitInfo;
      if (info.kind === 'mutex') {
        const owner = this.proc._readI32(info.addr + 4);
        parts.push(
          `thread ${t.id} waits on the mutex at 0x${info.addr.toString(16)} ` +
          `(owner: thread ${owner})`);
      } else if (info.kind === 'cond') {
        parts.push(
          `thread ${t.id} waits on the condition variable at ` +
          `0x${info.addr.toString(16)}`);
      } else if (info.kind === 'join') {
        parts.push(`thread ${t.id} waits to join thread ${info.target.id}`);
      } else if (info.kind === 'once') {
        parts.push(
          `thread ${t.id} waits on the pthread_once control at ` +
          `0x${info.addr.toString(16)}`);
      } else {
        parts.push(`thread ${t.id} waits (${info.kind})`);
      }
    }
    return new Error(
      `tinyc: deadlock: no thread can run and nothing is ` +
      `outstanding: ${parts.join('; ')}`);
  }
}

function isProcessExit(err) {
  return err && (err.name === 'ShimExit' || err.name === 'WasiExit') &&
    typeof err.exitCode === 'number';
}

// ---------------------------------------------------------------------
// The pthread implementation (design §6), as process-layer
// functions over a context { proc, scheduler } — the simulated
// tier's realization of every wait is the scheduler; the object
// words live in guest memory in both tiers.
// ---------------------------------------------------------------------

const MUTEX_STATE = 0, MUTEX_OWNER = 4, MUTEX_COUNT = 8, MUTEX_KIND = 12;
const COND_SEQ = 0, COND_WAITERS = 4, COND_CLOCK = 8;

function mutexAttempt(ctx, thread, m) {
  const proc = ctx.proc;
  const kind = proc._readI32(m + MUTEX_KIND);
  if (kind === -1) return { code: EINVAL };
  const state = proc._readI32(m + MUTEX_STATE);
  if (state === 0) {
    proc._writeI32(m + MUTEX_STATE, 1);
    proc._writeI32(m + MUTEX_OWNER, thread.id);
    proc._writeI32(m + MUTEX_COUNT, 1);
    return { code: 0 };
  }
  if (proc._readI32(m + MUTEX_OWNER) === thread.id) {
    if (kind === 2) {
      const count = proc._readI32(m + MUTEX_COUNT);
      if (count >= 0x7fffffff) return { code: EDEADLK };
      proc._writeI32(m + MUTEX_COUNT, count + 1);
      return { code: 0 };
    }
    if (kind === 1) return { code: EDEADLK };
    // A normal mutex relocked by its owner deadlocks against
    // itself; the wait below simply never wakes, and the
    // simulated tier's deadlock detector names it (§6.3).
    return { block: true };
  }
  proc._writeI32(m + MUTEX_STATE, 2);
  return { block: true };
}

/** Release a held mutex without the wake; returns an error
 *  code or 0. Used by unlock and by cond wait's release step. */
function mutexRelease(ctx, thread, m) {
  const proc = ctx.proc;
  const kind = proc._readI32(m + MUTEX_KIND);
  if (kind === -1) return EINVAL;
  if (proc._readI32(m + MUTEX_STATE) === 0) return EINVAL;
  if (proc._readI32(m + MUTEX_OWNER) !== thread.id) return EPERM;
  if (kind === 2) {
    const count = proc._readI32(m + MUTEX_COUNT) - 1;
    proc._writeI32(m + MUTEX_COUNT, count);
    if (count > 0) return 0;
  }
  proc._writeI32(m + MUTEX_STATE, 0);
  proc._writeI32(m + MUTEX_OWNER, 0);
  proc._writeI32(m + MUTEX_COUNT, 0);
  return 0;
}

function mutexWakeOne(ctx, m) {
  const sched = ctx.scheduler;
  const waiter = sched.dequeue(sched.mutexQueues, m);
  if (waiter) sched.wake(waiter, 'mutex');
}

function abstimeToDateMs(proc, abstimePtr) {
  const sec = proc._readI32(abstimePtr);
  const nsec = proc._readI32(abstimePtr + 4);
  return sec * 1000 + Math.floor(nsec / 1e6);
}

/** Build the pthread env shim set for the simulated tier.
 *  Blocking shims return a promise exactly when they block;
 *  index.mjs wraps the blocking subset in WebAssembly.
 *  Suspending for the process's modules. */
export function createSimPthreadShims(proc, scheduler) {
  const ctx = { proc, scheduler };
  const cur = () => scheduler.actingThread();
  // A thread's key value array (§7.1), allocated on first use:
  // before the main module loads, the allocator frontier is
  // still inside the span its data image lands on.
  const keyArray = (thread) => {
    if (!thread.keyValuesAddr) {
      thread.keyValuesAddr = proc._reserveStatic(PTHREAD_KEYS_MAX * 4, 4);
    }
    return thread.keyValuesAddr;
  };

  function mutexLockLoop(thread, m, deadlineMs) {
    const attempt = mutexAttempt(ctx, thread, m);
    if (!attempt.block) return attempt.code;
    scheduler.enqueue(scheduler.mutexQueues, m, thread);
    const info = { kind: 'mutex', addr: m };
    const parked = deadlineMs === null
      ? scheduler.park(thread, info)
      : scheduler.parkTimed(thread, info, deadlineMs);
    return parked.then((wake) => {
      if (wake === 'timeout') return ETIMEDOUT;
      return mutexLockLoop(thread, m, deadlineMs);
    });
  }

  function condWaitImpl(thread, c, m, deadlineMs) {
    // §6.4, in order: record seq, count the waiter, release
    // the mutex (a recursive mutex is fully released and its
    // count restored on re-acquire), wait, re-acquire.
    const savedCount = proc._readI32(m + MUTEX_KIND) === 2
      ? proc._readI32(m + MUTEX_COUNT) : 1;
    proc._writeI32(c + COND_WAITERS, proc._readI32(c + COND_WAITERS) + 1);
    proc._writeI32(m + MUTEX_STATE, 0);
    proc._writeI32(m + MUTEX_OWNER, 0);
    proc._writeI32(m + MUTEX_COUNT, 0);
    mutexWakeOne(ctx, m);
    scheduler.enqueue(scheduler.condQueues, c, thread);
    const info = { kind: 'cond', addr: c, mutex: m };
    const parked = deadlineMs === null
      ? scheduler.park(thread, info)
      : scheduler.parkTimed(thread, info, deadlineMs);
    return parked.then((wake) => {
      proc._writeI32(c + COND_WAITERS, proc._readI32(c + COND_WAITERS) - 1);
      const code = mutexLockLoop(thread, m, null);
      const restore = () => {
        if (proc._readI32(m + MUTEX_KIND) === 2) {
          proc._writeI32(m + MUTEX_COUNT, savedCount);
        }
        return wake === 'timeout' ? ETIMEDOUT : 0;
      };
      if (code && typeof code.then === 'function') return code.then(restore);
      return restore();
    });
  }

  const shims = {
    // -- threads --
    pthread_create(threadPtr, attrPtr, startSlot, arg) {
      if (scheduler.hostDriving) {
        throw new Error(
          'tinyc: pthread_create: the host context cannot create ' +
          'a thread (threads design §4.1)');
      }
      if (!proc._jspi) throw jspiError('pthread_create');
      let detached = false;
      if (attrPtr) {
        const ds = proc._readI32(attrPtr);
        if (ds !== 0 && ds !== 1) return EINVAL;
        detached = ds === 1;
      }
      let id;
      try {
        id = proc._createThread(startSlot | 0, arg | 0, detached);
      } catch (err) {
        return EAGAIN;
      }
      proc._writeI32(threadPtr, id);
      return 0;
    },
    pthread_join(targetId, retvalPtr) {
      const thread = cur();
      const target = scheduler.threads.get(targetId | 0);
      if (!target) {
        // A reaped record leaves a tombstone when the reap was
        // a join: the design's table gives EINVAL for an
        // already-joined thread, ESRCH only for an id that
        // never existed (§6.2). Ids are never reused, so the
        // tombstone is exact.
        return scheduler.joinedIds.has(targetId | 0) ? EINVAL : ESRCH;
      }
      if (target === thread) return EDEADLK;
      if (target.detached || target.joined) return EINVAL;
      target.joined = true;
      const finish = () => {
        if (retvalPtr) proc._writeI32(retvalPtr, target.exitValue);
        scheduler.joinedIds.add(target.id);
        scheduler._reapThread(target);
        return 0;
      };
      if (target.state === 'exited') return finish();
      target.joiners.push(thread);
      return scheduler.park(thread, { kind: 'join', target })
        .then(() => finish());
    },
    pthread_detach(targetId) {
      const target = scheduler.threads.get(targetId | 0);
      if (!target) return ESRCH;
      if (target.detached) return EINVAL;
      target.detached = true;
      if (target.state === 'exited') scheduler._reapThread(target);
      return 0;
    },
    pthread_self() {
      return scheduler.actingThread().id;
    },
    pthread_equal(a, b) {
      return a === b ? 1 : 0;
    },
    pthread_exit(value) {
      if (scheduler.hostDriving) {
        throw new Error(
          'tinyc: pthread_exit: the host context has no thread ' +
          'to exit (threads design §4.1)');
      }
      throw new ThreadExitSignal(value | 0);
    },

    // -- thread attributes: detach state is honored; the rest
    // round-trip without effect (§6.2) --
    pthread_attr_init(attr) {
      // The header's pthread_attr_t is four words: detachstate
      // plus three reserved. Writing past it corrupts the
      // caller's frame — the object layout is the ABI.
      proc._writeI32(attr, 0);
      proc._writeI32(attr + 4, 0);
      proc._writeI32(attr + 8, 0);
      proc._writeI32(attr + 12, 0);
      return 0;
    },
    pthread_attr_destroy() { return 0; },
    pthread_attr_setdetachstate(attr, state) {
      if (state !== 0 && state !== 1) return EINVAL;
      proc._writeI32(attr, state);
      return 0;
    },
    pthread_attr_getdetachstate(attr, out) {
      proc._writeI32(out, proc._readI32(attr));
      return 0;
    },

    // -- mutexes --
    pthread_mutex_init(m, attrPtr) {
      let kind = 0;
      if (attrPtr) {
        kind = proc._readI32(attrPtr);
        if (kind < 0 || kind > 2) return EINVAL;
      }
      proc._writeI32(m + MUTEX_STATE, 0);
      proc._writeI32(m + MUTEX_OWNER, 0);
      proc._writeI32(m + MUTEX_COUNT, 0);
      proc._writeI32(m + MUTEX_KIND, kind);
      return 0;
    },
    pthread_mutex_destroy(m) {
      if (proc._readI32(m + MUTEX_KIND) === -1) return EINVAL;
      if (proc._readI32(m + MUTEX_STATE) !== 0) return EBUSY;
      const q = scheduler.mutexQueues.get(m);
      if (q && q.some((t) => t.state === 'blocked')) return EBUSY;
      proc._writeI32(m + MUTEX_KIND, -1);
      return 0;
    },
    pthread_mutex_lock(m) {
      return mutexLockLoop(cur(), m, null);
    },
    pthread_mutex_trylock(m) {
      const thread = cur();
      const kind = proc._readI32(m + MUTEX_KIND);
      if (kind === -1) return EINVAL;
      if (proc._readI32(m + MUTEX_STATE) === 0) {
        proc._writeI32(m + MUTEX_STATE, 1);
        proc._writeI32(m + MUTEX_OWNER, thread.id);
        proc._writeI32(m + MUTEX_COUNT, 1);
        return 0;
      }
      if (proc._readI32(m + MUTEX_OWNER) === thread.id && kind === 2) {
        const count = proc._readI32(m + MUTEX_COUNT);
        if (count >= 0x7fffffff) return EDEADLK;
        proc._writeI32(m + MUTEX_COUNT, count + 1);
        return 0;
      }
      return EBUSY;
    },
    pthread_mutex_timedlock(m, abstimePtr) {
      return mutexLockLoop(cur(), m, abstimeToDateMs(proc, abstimePtr));
    },
    pthread_mutex_unlock(m) {
      const thread = cur();
      const rc = mutexRelease(ctx, thread, m);
      if (rc === 0) mutexWakeOne(ctx, m);
      return rc;
    },
    pthread_mutexattr_init(attr) { proc._writeI32(attr, 0); return 0; },
    pthread_mutexattr_destroy() { return 0; },
    pthread_mutexattr_settype(attr, type) {
      if (type < 0 || type > 2) return EINVAL;
      proc._writeI32(attr, type);
      return 0;
    },
    pthread_mutexattr_gettype(attr, out) {
      proc._writeI32(out, proc._readI32(attr));
      return 0;
    },

    // -- condition variables --
    pthread_cond_init(c, attrPtr) {
      let clock = 0;
      if (attrPtr) {
        clock = proc._readI32(attrPtr);
        if (clock !== 0 && clock !== 1) return EINVAL;
      }
      proc._writeI32(c + COND_SEQ, 0);
      proc._writeI32(c + COND_WAITERS, 0);
      proc._writeI32(c + COND_CLOCK, clock);
      proc._writeI32(c + 12, 0);
      return 0;
    },
    pthread_cond_destroy(c) {
      if (proc._readI32(c + COND_WAITERS) > 0) return EBUSY;
      const q = scheduler.condQueues.get(c);
      if (q && q.some((t) => t.state === 'blocked')) return EBUSY;
      return 0;
    },
    pthread_cond_wait(c, m) {
      return condWaitImpl(cur(), c, m, null);
    },
    pthread_cond_timedwait(c, m, abstimePtr) {
      const thread = cur();
      // The abstime is converted once, at entry, against the
      // cond's clock (§6.4), into the Date.now() frame the
      // scheduler's timers run in: CLOCK_REALTIME abstimes are
      // already epoch milliseconds; CLOCK_MONOTONIC abstimes
      // are in the performance clock's milliseconds.
      const sec = proc._readI32(abstimePtr);
      const nsec = proc._readI32(abstimePtr + 4);
      const abstimeMs = sec * 1000 + nsec / 1e6;
      const clock = proc._readI32(c + COND_CLOCK);
      const deadlineMs = clock === 1
        ? Date.now() + (abstimeMs - performance.now())
        : abstimeMs;
      return condWaitImpl(thread, c, m, deadlineMs);
    },
    pthread_cond_signal(c) {
      proc._writeI32(c + COND_SEQ, proc._readI32(c + COND_SEQ) + 1);
      const waiter = scheduler.dequeue(scheduler.condQueues, c);
      if (waiter) transferCondWaiter(ctx, waiter);
      return 0;
    },
    pthread_cond_broadcast(c) {
      proc._writeI32(c + COND_SEQ, proc._readI32(c + COND_SEQ) + 1);
      let waiter;
      while ((waiter = scheduler.dequeue(scheduler.condQueues, c))) {
        transferCondWaiter(ctx, waiter);
      }
      return 0;
    },
    pthread_condattr_init(attr) { proc._writeI32(attr, 0); return 0; },
    pthread_condattr_destroy() { return 0; },
    pthread_condattr_setclock(attr, clock) {
      if (clock !== 0 && clock !== 1) return EINVAL;
      proc._writeI32(attr, clock);
      return 0;
    },
    pthread_condattr_getclock(attr, out) {
      proc._writeI32(out, proc._readI32(attr));
      return 0;
    },

    // -- once --
    pthread_once(ctrl, initSlot) {
      const thread = cur();
      const state = proc._readI32(ctrl);
      if (state === 2) return 0;
      if (state === 0) {
        proc._writeI32(ctrl, 1);
        const fn = proc.table.get(initSlot);
        if (typeof fn === 'function') fn();
        proc._writeI32(ctrl, 2);
        const q = scheduler.onceQueues.get(ctrl);
        if (q) {
          for (const w of q.splice(0)) scheduler.wake(w, 'once');
        }
        return 0;
      }
      scheduler.enqueue(scheduler.onceQueues, ctrl, thread);
      return scheduler.park(thread, { kind: 'once', addr: ctrl })
        .then(() => 0);
    },

    // -- spin locks --
    pthread_spin_init(s) { proc._writeI32(s, 0); return 0; },
    pthread_spin_destroy(s) {
      return proc._readI32(s) !== 0 ? EBUSY : 0;
    },
    pthread_spin_lock(s) {
      const thread = scheduler.actingThread();
      const spin = () => {
        const view = new Int32Array(proc.memory.buffer);
        let budget = SPIN_BUDGET;
        while (budget-- > 0) {
          if (Atomics.compareExchange(view, s / 4, 0, 1) === 0) return 0;
        }
        // §4.4: the budget is exhausted — yield and resume
        // spinning when next selected.
        return scheduler.yieldThread(thread).then(() => spin());
      };
      return spin();
    },
    pthread_spin_trylock(s) {
      const view = new Int32Array(proc.memory.buffer);
      return Atomics.compareExchange(view, s / 4, 0, 1) === 0 ? 0 : EBUSY;
    },
    pthread_spin_unlock(s) {
      const view = new Int32Array(proc.memory.buffer);
      Atomics.store(view, s / 4, 0);
      return 0;
    },

    // -- thread-local storage (pthread_key) --
    pthread_key_create(keyPtr, dtorSlot) {
      const word = proc._readI32(HDR_KEY_COUNTER);
      if (word >= PTHREAD_KEYS_MAX) return EAGAIN;
      proc._writeI32(HDR_KEY_COUNTER, word + 1);
      scheduler.keyDtors[word] = dtorSlot | 0;
      scheduler.keyLive[word] = true;
      proc._writeI32(keyPtr, word);
      return 0;
    },
    pthread_key_delete(key) {
      key |= 0;
      if (key < 0 || key >= PTHREAD_KEYS_MAX || !scheduler.keyLive[key]) {
        return EINVAL;
      }
      scheduler.keyLive[key] = false;
      scheduler.keyDtors[key] = 0;
      return 0;
    },
    pthread_setspecific(key, value) {
      key |= 0;
      if (key < 0 || key >= PTHREAD_KEYS_MAX || !scheduler.keyLive[key]) {
        return EINVAL;
      }
      const thread = scheduler.actingThread();
      proc._writeI32(keyArray(thread) + key * 4, value | 0);
      return 0;
    },
    pthread_getspecific(key) {
      key |= 0;
      if (key < 0 || key >= PTHREAD_KEYS_MAX || !scheduler.keyLive[key]) {
        return 0;
      }
      const thread = scheduler.actingThread();
      return proc._readI32(keyArray(thread) + key * 4);
    },

    // -- scheduling --
    sched_yield() {
      const thread = scheduler.current;
      if (thread instanceof ThreadRecord && !scheduler.hostDriving) {
        return scheduler.yieldThread(thread);
      }
      return 0;
    },
  };

  /** §6.4: a signal moves the cond queue's head to its mutex's
   *  queue — the waiter contends for the mutex as any locker
   *  and its wait returns only once it holds it. When the mutex
   *  is free the waiter is simply made runnable. */
  function transferCondWaiter(ctx2, waiter) {
    const m = waiter.waitInfo.mutex;
    waiter.waitInfo = { kind: 'mutex', addr: m };
    if (proc._readI32(m + MUTEX_STATE) === 0) {
      scheduler.wake(waiter, 'cond');
    } else {
      proc._writeI32(m + MUTEX_STATE, 2);
      scheduler.enqueue(scheduler.mutexQueues, m, waiter);
    }
  }

  return shims;
}


/** The pthread surface names, for the coordinator's binding
 *  set. Kept beside the implementations; a name missing here
 *  would surface as an unresolved import in the real tier. */
export const PTHREAD_SURFACE_NAMES = [
  'pthread_create', 'pthread_join', 'pthread_detach',
  'pthread_self', 'pthread_equal', 'pthread_exit',
  'pthread_attr_init', 'pthread_attr_destroy',
  'pthread_attr_setdetachstate', 'pthread_attr_getdetachstate',
  'pthread_mutex_init', 'pthread_mutex_destroy',
  'pthread_mutex_lock', 'pthread_mutex_trylock',
  'pthread_mutex_timedlock', 'pthread_mutex_unlock',
  'pthread_mutexattr_init', 'pthread_mutexattr_destroy',
  'pthread_mutexattr_settype', 'pthread_mutexattr_gettype',
  'pthread_cond_init', 'pthread_cond_destroy',
  'pthread_cond_wait', 'pthread_cond_timedwait',
  'pthread_cond_signal', 'pthread_cond_broadcast',
  'pthread_condattr_init', 'pthread_condattr_destroy',
  'pthread_condattr_setclock', 'pthread_condattr_getclock',
  'pthread_once',
  'pthread_spin_init', 'pthread_spin_destroy',
  'pthread_spin_lock', 'pthread_spin_trylock',
  'pthread_spin_unlock',
  'pthread_key_create', 'pthread_key_delete',
  'pthread_setspecific', 'pthread_getspecific',
  'sched_yield',
];

/** The real-tier coordinator's pthread set: the coordinator's
 *  replicas exist so layout, GOT and lifecycle have a local
 *  instance (§5.1) — they never run program code, so these
 *  shims only satisfy binding. A call means an init ran on
 *  the coordinator that reached for a thread primitive,
 *  which the tier does not support; it fails by name. */
export function createCoordinatorPthreadShims() {
  const shims = {};
  for (const name of PTHREAD_SURFACE_NAMES) {
    shims[name] = () => {
      throw new Error(
        `tinyc: ${name}: a thread primitive was reached on the ` +
        `coordinator's replica, which never runs program code ` +
        `(threads design §5.1)`);
    };
  }
  return shims;
}

// ======================================================================
// The real tier: worker-side runtime (threads design §5)
// ======================================================================
//
// A thread worker holds replicas of the process's modules over
// the shared memory: its own table (a layout copy — slot
// numbers are the function-pointer values, equal across
// threads), its own GOT globals, its own shim set. It never
// runs a module's init for a module the primary context
// initialized; for a module it loads itself as the §5.4
// requester, it runs init once every replica exists.
//
// Blocking is a bounded poll (§5.3): wait on the primitive's
// word with a few-millisecond timeout, and on each iteration
// service whatever the coordinator posted — load tasks and,
// for the main thread's worker, host callExport requests. A
// running worker therefore services loads at its next shim
// call or wait point, which is the design's stated pause.

const CB_LOAD_SEQ = 0, CB_ACKS_DONE = 4, CB_LOAD_DONE = 8,
  CB_TASK_ADDR = 12, CB_CALL_SEQ = 16, CB_CALL_TASK_ADDR = 20,
  CB_CALL_DONE = 24, CB_STDIN_CURSOR = 28, CB_STDIN_ADDR = 32,
  CB_STDIN_LEN = 36, CB_LOAD_FAILED = 40;
const TR_STATE = 0, TR_EXIT = 4, TR_ACK = 8, TR_DETACHED = 12,
  TR_JOIN_CLAIMED = 16;
const POLL_MS = 4;
const RT_DEFAULT_TYPE = { params: ['i32', 'i32', 'i32', 'i32'], results: ['i32'] };
const rtEncoder = new TextEncoder();
const rtDecoder = new TextDecoder();

function isExitSignal(err) {
  return err instanceof ShimExit ||
    (err && (err.name === 'ShimExit' || err.name === 'WasiExit') &&
     typeof err.exitCode === 'number');
}

export function startThreadWorker(boot, helpers, port) {
  const rt = {
    boot, helpers, port,
    memory: boot.memory,
    table: new WebAssembly.Table({ initial: 2, element: 'anyfunc' }),
    holder: null,
    records: new Map(),      // canonical -> replica record
    byHandle: new Map(),     // handle -> replica record
    got: new Map(),          // 'kind:name' -> { global, definer, offset }
    symbols: new Map(),      // name -> { wrec, entry }
    slotDesc: new Map(),     // slot -> descriptor
    lastTaskSeq: 0,
    lastCallSeq: 0,
    lastTask: null,          // { seq, requester, handle, failed, error }
    pendingMainSp: [],       // tasks whose baked tops the main
                             // replica still has to adopt
    my: {
      id: boot.threadId,
      recordAddr: boot.recordAddr,
      keyArrayAddr: boot.keyArrayAddr || 0,
      regions: boot.regions || {}, // canonical -> { stackBase, tlsArea }
    },
    joined: new Set(),
    detachedLocal: new Set(),
    dlerror: '',
    mainCanonical: null,
    alloc: null,             // { reserve(size, align) }
    shimEnv: null,           // module name -> { name -> fn }
    importTypesCache: new Map(),
  };
  rt.holder = { memory: rt.memory, instance: null };
  rt.readI32 = (addr) => new DataView(rt.memory.buffer).getInt32(addr, true);
  rt._readI32 = rt.readI32;
  rt.writeI32 = (addr, v) =>
    new DataView(rt.memory.buffer).setInt32(addr, v | 0, true);
  rt.view = () => new Int32Array(rt.memory.buffer);

  // -- shim universe (this worker's own set, §5.1) --
  const byteIO = {
    out: (bytes) => port.postMessage(
      { type: 'io', stream: 'out', bytes: new Uint8Array(bytes) }),
    err: (bytes) => port.postMessage(
      { type: 'io', stream: 'err', bytes: new Uint8Array(bytes) }),
    in: () => rtStdinPull(rt),
  };
  const def = createDefaultShims({
    memory: () => rt.memory,
    holder: rt.holder,
    io: byteIO,
    memoryAllocator: true,
    processTable: rt.table,
  });
  rt.alloc = { reserve: (size, align) => def.reserveStatic(size, align) };
  const shimCtx = helpers.createShimContext(rt.holder, byteIO);
  const wasi = helpers.createWasiShim(
    { args: boot.args, env: boot.env, stdin: new Uint8Array(0) },
    rt.holder, byteIO);
  const pthreadEnv = createRealPthreadShims(rt);
  const dlfcnEnv = createWorkerDlfcn(rt);
  rt.shimEnv = {
    env: { ...def.env, ...pthreadEnv, ...dlfcnEnv },
    wasi_snapshot_preview1: wasi,
  };
  rt.shimCtx = shimCtx;

  // -- boot: GOT, slots, replicas, per-thread state --
  // A failure here must reach the coordinator as a fault;
  // a worker that dies silently would leave it waiting.
  const wrecs = [];
  try {
  for (const g of boot.got) rtApplyGot(rt, g);
  for (const s of boot.slots) rtBuildSlot(rt, s);
  for (const m of boot.modules) wrecs.push(rtAddReplica(rt, m));
  for (const wrec of wrecs) rtEnsureRegions(rt, wrec);
  rtRefreshGotTls(rt);
  } catch (err) {
    port.postMessage({
      type: 'thread-fault',
      message: err && err.message ? err.message : String(err),
    });
    self.close();
    return;
  }

  const finishBootThreadExit = (value) => rtThreadExit(rt, value);
  try {
    if (boot.entry.kind === 'main') {
      rt.mainCanonical = boot.entry.canonical;
      // The main worker is the primary context for the initial
      // load: it runs the inits in load order (§5.4), then the
      // entry.
      for (const wrec of wrecs) rtRunInit(rt, wrec);
      const main = rt.records.get(boot.entry.canonical);
      rt.holder.instance = main.instance;
      const result = rtRunMainEntry(rt, main, boot.entry.runOptions);
      port.postMessage({ type: 'main-returned', result });
    } else {
      // A created thread: install its per-module stack tops and
      // TLS bases, then call the start routine through its own
      // table (§5.1). Replicas are not initialized here — the
      // primary context did that before this thread existed.
      for (const wrec of wrecs) rtInstallThreadState(rt, wrec);
      const fn = rt.table.get(boot.entry.slot);
      const value = fn(boot.entry.arg);
      finishBootThreadExit(value);
    }
  } catch (err) {
    if (err instanceof ThreadExitSignal) {
      finishBootThreadExit(err.value);
    } else if (isExitSignal(err)) {
      port.postMessage({ type: 'process-exit', code: err.exitCode | 0 });
      self.close();
    } else {
      port.postMessage({
        type: 'thread-fault',
        message: err && err.message ? err.message : String(err),
      });
      self.close();
    }
  }
}

/** stdin: the creation-time byte string sits in shared memory
 *  and is consumed through the control block's atomic cursor,
 *  so concurrent readers split it rather than duplicate it
 *  (§5.6). One pull claims at most a chunk. */
function rtStdinPull(rt) {
  const cb = rt.boot.controlAddr;
  const addr = rt.readI32(cb + CB_STDIN_ADDR);
  const len = rt.readI32(cb + CB_STDIN_LEN);
  if (!addr || len === 0) return null;
  const view = rt.view();
  const want = 4096;
  const start = Atomics.add(view, (cb + CB_STDIN_CURSOR) / 4, want);
  if (start >= len) return null;
  const take = Math.min(want, len - start);
  return new Uint8Array(rt.memory.buffer).slice(addr + start, addr + start + take);
}

function rtGotEntry(rt, kind, name) {
  const key = `${kind}:${name}`;
  let e = rt.got.get(key);
  if (!e) {
    e = {
      global: new WebAssembly.Global({ value: 'i32', mutable: true }, 0),
      definer: null, offset: 0,
    };
    rt.got.set(key, e);
  }
  return e;
}

/** Apply one GOT fill from the coordinator. First fill wins,
 *  mirroring the coordinator's own rule. */
function rtApplyGot(rt, g) {
  const e = rtGotEntry(rt, g.kind, g.name);
  if (g.kind === 'tls') {
    if (!e.definer) { e.definer = g.definer; e.offset = g.offset; }
  } else if (e.global.value === 0) {
    // A GOT global goes from zero to its fill exactly once,
    // as on the coordinator.
    e.global.value = g.value;
  }
}

function rtRefreshGotTls(rt) {
  for (const [key, e] of rt.got) {
    if (!key.startsWith('tls:') || !e.definer) continue;
    const definer = rt.records.get(e.definer);
    if (!definer) continue;
    e.global.value = rtTlsArea(rt, definer) + e.offset;
  }
}

/** Build the table slot a coordinator descriptor names, at the
 *  same index: a stub traps by name, a shim is this worker's
 *  own shim of that name (wrapped to the descriptor's type), a
 *  host function does not exist in a worker and traps by name
 *  (§5.1's stated boundary). */
function rtBuildSlot(rt, desc) {
  rt.slotDesc.set(desc.slot, desc);
  while (rt.table.length <= desc.slot) rt.table.grow(1);
  // A WebAssembly table holds only wasm functions: every
  // slot's JS function goes through the wrapper generator,
  // exactly as on the coordinator.
  const type = { params: desc.params, results: desc.results };
  let raw;
  if (desc.kind === 'shim') {
    raw = rt.shimEnv.env[desc.name] ??
      rt.shimEnv.wasi_snapshot_preview1[desc.name];
  }
  if (!raw) {
    const message = desc.kind === 'host'
      ? `tinyc: call to host function "${desc.name}", which is not ` +
        `available inside a thread worker`
      : `tinyc: call to unresolved function "${desc.name}"`;
    raw = () => { throw new Error(message); };
  }
  rt.table.set(desc.slot,
    rt.helpers.makeWrapperFunction(raw, type));
}

/** Instantiate one replica from its boot/task description and
 *  register its exports (first wins, as on the coordinator). */
function rtAddReplica(rt, m) {
  // manifestToLibrary returns the library object; the layout
  // fields live on its .meta.
  const meta = rt.helpers.manifestToLibrary(m.manifest, m.canonical).meta;
  const wrec = {
    canonical: m.canonical,
    memoryBase: m.memoryBase,
    tableBase: m.tableBase,
    handle: m.handle,
    global: Boolean(m.global),
    meta,
    manifest: m.manifest,
    bodyBytes: m.bodyBytes,
    // Needed names arrive as recorded (a raw --dylink-need
    // spelling); canonicalize them against this module's own
    // directory — the coordinator's rule — so visibility
    // checks against replica records compare like with like.
    needed: (m.needed ?? meta.needed ?? []).map((n) =>
      rt.helpers.canonicalModulePath(n,
        m.canonical.slice(0, m.canonical.lastIndexOf('/')) || '/')),
    exports: new Map(m.exports.map(([n, e]) => [n, e])),
    instance: null,
    tlsBaseGlobal: null,
    importTypes: null,
  };
  const compiled = new WebAssembly.Module(m.bodyBytes);
  wrec.importTypes = rt.helpers.parseImportTypes(m.bodyBytes);
  const instance = new WebAssembly.Instance(
    compiled, rtReplicaImports(rt, wrec, compiled));
  wrec.instance = instance;
  rt.records.set(wrec.canonical, wrec);
  rt.byHandle.set(wrec.handle, wrec);
  for (const [name, e] of wrec.exports) {
    if (!rt.symbols.has(name)) rt.symbols.set(name, { wrec, entry: e });
  }
  return wrec;
}

/** This thread's regions for a module, allocated on the later
 *  of thread creation and module arrival (§3.1, §7.2). The
 *  main thread's stack is the module's embedded span (region
 *  base null); its TLS area is allocated like any thread's. */
function rtEnsureRegions(rt, wrec) {
  let region = rt.my.regions[wrec.canonical];
  if (!region) {
    region = { stackBase: null, tlsArea: 0 };
    rt.my.regions[wrec.canonical] = region;
    if (rt.my.id !== 1 && wrec.meta.stackSize !== undefined) {
      region.stackBase = rt.alloc.reserve(wrec.meta.stackSize, 16);
    }
  }
  if (wrec.meta.tlsSize > 0 && !region.tlsArea) {
    region.tlsArea = rtTlsAlloc(rt, wrec);
  }
  return region;
}

function rtTlsAlloc(rt, wrec) {
  const addr = rt.alloc.reserve(wrec.meta.tlsSize, wrec.meta.tlsAlign);
  const m = new Uint8Array(rt.memory.buffer);
  m.fill(0, addr, addr + wrec.meta.tlsSize);
  if (wrec.meta.tlsTemplate) m.set(wrec.meta.tlsTemplate, addr);
  return addr;
}

function rtTlsArea(rt, wrec) {
  const region = rtEnsureRegions(rt, wrec);
  return region.tlsArea;
}

/** Point a replica's per-thread state at this thread: the
 *  stack pointer (through the swap export) and the TLS base. */
function rtInstallThreadState(rt, wrec) {
  const region = rtEnsureRegions(rt, wrec);
  if (wrec.meta.stackSize !== undefined && region.stackBase !== null) {
    wrec.instance.exports.__tinyc_dylink_swap_sp(
      region.stackBase + wrec.meta.stackSize);
  }
  if (wrec.tlsBaseGlobal) {
    wrec.tlsBaseGlobal.value = region.tlsArea;
  }
}

/** Run a replica's init on this thread's context. Only the
 *  primary context for a load calls this (§5.4). */
function rtRunInit(rt, wrec) {
  rtInstallThreadState(rt, wrec);
  const rc = wrec.instance.exports.__tinyc_dylink_init(wrec.handle);
  if (rc !== 0) {
    throw new Error(
      `tinyc: initialization of "${wrec.canonical}" failed: ` +
      `its DllMain returned FALSE`);
  }
}

/** The main worker's entry dispatch, mirroring the
 *  single-context runMainModule's shapes. */
function rtRunMainEntry(rt, main, runOptions) {
  const ex = main.instance.exports;
  if (runOptions.entry) {
    const fn = ex[runOptions.entry];
    if (typeof fn !== 'function') {
      throw new Error(`tinyc: entry point "${runOptions.entry}" not found`);
    }
    return fn(...(runOptions.callArgs ?? []));
  }
  if (typeof ex._start === 'function') {
    ex._start();
    return { exitCode: 0 };
  }
  if (typeof ex.main === 'function') {
    const args = rt.boot.args;
    const enc = rtEncoder;
    const ptrs = [];
    let size = 0;
    for (const a of args) { const n = enc.encode(a).length + 1; ptrs.push(n); size += n; }
    const argv = rt.alloc.reserve(size + (args.length + 1) * 4, 16);
    const m = new Uint8Array(rt.memory.buffer);
    const dv = new DataView(rt.memory.buffer);
    let at = argv + (args.length + 1) * 4;
    args.forEach((a, i) => {
      dv.setInt32(argv + i * 4, at, true);
      const b = enc.encode(a);
      m.set(b, at);
      m[at + b.length] = 0;
      at += b.length + 1;
    });
    dv.setInt32(argv + args.length * 4, 0, true);
    const rc = ex.main(args.length, argv);
    return { exitCode: rc | 0 };
  }
  throw new Error('tinyc: the main module exports no entry point');
}

/** Assemble a replica's imports from this worker's universe:
 *  its shim set first, then the process symbol table (the
 *  replica's closure, then global modules), then a stub that
 *  names the missing function if it is ever called. */
function rtReplicaImports(rt, wrec, compiled) {
  const imports = {};
  const add = (mod, name, value) => { (imports[mod] ??= {})[name] = value; };
  const needed = new Set([wrec.canonical, ...(wrec.needed ?? [])]);
  const visible = (cand) =>
    needed.has(cand.canonical) || cand.global ||
    cand.canonical === rt.mainCanonical;
  for (const imp of WebAssembly.Module.imports(compiled)) {
    if (imp.module === 'env' && imp.name === 'memory') {
      add('env', 'memory', rt.memory); continue;
    }
    if (imp.module === 'env' && imp.name === '__indirect_function_table') {
      add('env', '__indirect_function_table', rt.table); continue;
    }
    if (imp.module === 'env' && imp.name === '__memory_base') {
      add('env', '__memory_base',
        new WebAssembly.Global({ value: 'i32' }, wrec.memoryBase)); continue;
    }
    if (imp.module === 'env' && imp.name === '__table_base') {
      add('env', '__table_base',
        new WebAssembly.Global({ value: 'i32' }, wrec.tableBase)); continue;
    }
    if (imp.module === 'env' && imp.name === '__tls_base') {
      const g = new WebAssembly.Global({ value: 'i32', mutable: true }, 0);
      wrec.tlsBaseGlobal = g;
      add('env', '__tls_base', g); continue;
    }
    if (imp.module === 'GOT.func' || imp.module === 'GOT.mem' ||
        imp.module === 'GOT.tls') {
      const kind = imp.module === 'GOT.func' ? 'func'
        : imp.module === 'GOT.mem' ? 'mem' : 'tls';
      add(imp.module, imp.name, rtGotEntry(rt, kind, imp.name).global);
      continue;
    }
    if (imp.kind === 'function') {
      add(imp.module, imp.name, rtFunctionImport(rt, wrec, imp, visible));
      continue;
    }
    if (imp.kind === 'global') {
      add(imp.module, imp.name,
        new WebAssembly.Global({ value: imp.type.value },
          imp.type.value === 'i64' ? 0n : 0));
      continue;
    }
    if (imp.kind === 'memory') { add(imp.module, imp.name, rt.memory); continue; }
    if (imp.kind === 'table') { add(imp.module, imp.name, rt.table); continue; }
    throw new Error(
      `tinyc: unsupported import kind "${imp.kind}" for ` +
      `"${imp.module}.${imp.name}" in a thread worker`);
  }
  return imports;
}

function rtFunctionImport(rt, wrec, imp, visible) {
  const type = wrec.importTypes.get(`${imp.module}.${imp.name}`) ?? RT_DEFAULT_TYPE;
  const shimLayer = rt.shimEnv[imp.module];
  const raw = shimLayer ? shimLayer[imp.name] : undefined;
  if (raw !== undefined) {
    return typeof raw === 'function'
      ? rt.helpers.makeWrapperFunction(raw, type) : raw;
  }
  const sym = rt.symbols.get(imp.name);
  if (sym && sym.entry.kind === 0 && visible(sym.wrec)) {
    if (sym.wrec.instance) {
      const target = sym.wrec.instance.exports[imp.name];
      if (typeof target === 'function') return target;
    }
    // The defining replica is built after this one (a module
    // is instantiated before its dependencies in boot order):
    // resolve the target at call time, when every replica of
    // the process exists.
    const name = imp.name;
    const owner = sym.wrec;
    return rt.helpers.makeWrapperFunction((...args) => {
      const target = owner.instance && owner.instance.exports[name];
      if (typeof target !== 'function') {
        throw new Error(`tinyc: call to unresolved function "${name}"`);
      }
      return target(...args);
    }, type);
  }
  const name = imp.name;
  return rt.helpers.makeWrapperFunction(() => {
    throw new Error(`tinyc: call to unresolved function "${name}"`);
  }, type);
}

// -- the §5.3 wait loop and §5.4 task service --

/** Bounded poll: service pending coordinator work, then wait
 *  briefly on the control block's load word (any load or call
 *  activity notifies it), until `cond` holds. */
function rtWaitLoop(rt, cond) {
  const view = rt.view();
  const idx = rt.boot.controlAddr / 4;
  while (!cond()) {
    rtServicePending(rt);
    if (cond()) break;
    const v = Atomics.load(view, idx);
    Atomics.wait(view, idx, v, POLL_MS);
  }
}

/** One bounded step of the same loop, for timed waits that
 *  compose their own condition. Returns cond()'s value. */
function rtWaitStep(rt, cond) {
  rtServicePending(rt);
  if (cond()) return true;
  const view = rt.view();
  const idx = rt.boot.controlAddr / 4;
  const v = Atomics.load(view, idx);
  Atomics.wait(view, idx, v, POLL_MS);
  return cond();
}

function rtServicePending(rt) {
  const cb = rt.boot.controlAddr;
  const seq = rt.readI32(cb + CB_LOAD_SEQ);
  if (seq > rt.lastTaskSeq) {
    const payloadAddr = rt.readI32(cb + CB_TASK_ADDR);
    if (rt.readI32(cb + CB_LOAD_FAILED) === seq) {
      const requester = rt.readI32(payloadAddr + 8);
      const errLen = rt.readI32(payloadAddr + 40);
      const errBytes = new Uint8Array(rt.memory.buffer)
        .subarray(payloadAddr + 44, payloadAddr + 44 + errLen);
      rt.lastTask = {
        seq, requester, handle: 0, failed: true,
        error: rtDecoder.decode(errBytes.slice()),
      };
      rt.lastTaskSeq = seq;
    } else {
      rtApplyLoadTask(rt, payloadAddr);
    }
  }
  if (rt.my.id === 1) {
    const cseq = rt.readI32(cb + CB_CALL_SEQ);
    if (cseq > rt.lastCallSeq) {
      rt.lastCallSeq = cseq;
      rtServiceCall(rt, rt.readI32(cb + CB_CALL_TASK_ADDR));
    }
  }
}

/** Apply one load task: replicas, GOT fills, table slots and
 *  this thread's regions for the new modules; then ack. If
 *  this worker is the §5.4 requester, it also runs the inits
 *  once every context has acked, publishes the baked stack
 *  tops, and sets the load-done word. */
function rtApplyLoadTask(rt, payloadAddr) {
  const dv = new DataView(rt.memory.buffer);
  const m = new Uint8Array(rt.memory.buffer);
  let p = payloadAddr;
  const rd = () => { const v = dv.getInt32(p, true); p += 4; return v; };
  const rstr = () => {
    const len = rd();
    const s = rtDecoder.decode(m.slice(p, p + len));
    p += len + ((4 - (len % 4)) % 4);
    return s;
  };
  const seq = rd();
  const initBy = rd();
  const requester = rd();
  const handle = rd();
  const moduleCount = rd();
  const tableFree = rd();
  const gotCount = rd();
  const slotCount = rd();
  rd(); // reserved
  rd(); // baked-tops block address (filled by whoever inits)
  rd(); // error text length (the error text itself sits here,
        // after the header, when the load has failed)
  const gots = [];
  for (let i = 0; i < gotCount; i++) {
    const kindWord = rd();
    const name = rstr();
    const value = rd();
    const definer = rstr();
    const offset = rd();
    gots.push({
      kind: kindWord === 0 ? 'func' : kindWord === 1 ? 'mem' : 'tls',
      name, value, definer, offset,
    });
  }
  const slotDescs = [];
  for (let i = 0; i < slotCount; i++) {
    const slot = rd();
    const kindWord = rd();
    const name = rstr();
    const params = rstr();
    const results = rstr();
    slotDescs.push({
      slot, name,
      kind: kindWord === 0 ? 'stub' : kindWord === 1 ? 'shim' : 'host',
      params: params ? params.split(',') : [],
      results: results ? results.split(',') : [],
    });
  }
  const mods = [];
  for (let i = 0; i < moduleCount; i++) {
    const canonical = rstr();
    const memoryBase = rd();
    const tableBase = rd();
    const modHandle = rd();
    const global = rd();
    const manifestLen = rd();
    const manifest = JSON.parse(
      rtDecoder.decode(m.slice(p, p + manifestLen)));
    p += manifestLen + ((4 - (manifestLen % 4)) % 4);
    const bodyLen = rd();
    const bodyBytes = m.slice(p, p + bodyLen);
    p += bodyLen + ((4 - (bodyLen % 4)) % 4);
    mods.push({
      canonical, memoryBase, tableBase, handle: modHandle,
      global, manifest, bodyBytes,
      exports: (Array.isArray(manifest.exports) ? manifest.exports : [])
        .map((e) => [String(e.name), {
          kind: e.kind | 0, value: e.value | 0, tls: e.tls ? 1 : 0,
        }]),
      needed: manifest.needed ?? [],
    });
  }
  rt.lastTask = { seq, requester, handle, failed: false, error: '' };
  for (const g of gots) rtApplyGot(rt, g);
  for (const d of slotDescs) {
    if (!rt.slotDesc.has(d.slot)) rtBuildSlot(rt, d);
  }
  const newWrecs = [];
  for (const mod of mods) {
    if (!rt.records.has(mod.canonical)) newWrecs.push(rtAddReplica(rt, mod));
  }
  for (const wrec of newWrecs) {
    rtEnsureRegions(rt, wrec);
    if (rt.my.id !== 1) rtInstallThreadState(rt, wrec);
  }
  rtRefreshGotTls(rt);
  // Acknowledge: this context now holds its replicas.
  if (rt.my.recordAddr) {
    const view = rt.view();
    Atomics.store(view, (rt.my.recordAddr + TR_ACK) / 4, seq);
    Atomics.notify(view, (rt.my.recordAddr + TR_ACK) / 4);
  }
  rt.lastTaskSeq = seq;
  if (initBy === 1 && requester === rt.my.id) {
    // Wait for every context's ack, then run the inits here.
    const cb = rt.boot.controlAddr;
    rtWaitLoop(rt, () => rt.readI32(cb + CB_ACKS_DONE) >= seq);
    try {
      const topsAddr = rt.alloc.reserve(mods.length * 4, 4);
      mods.forEach((mod, i) => {
        const wrec = rt.records.get(mod.canonical);
        rtRunInit(rt, wrec);
        let baked = 0;
        const swap = wrec.instance.exports.__tinyc_dylink_swap_sp;
        if (typeof swap === 'function') {
          baked = swap(0);
          // The requester's own stack for the new module is
          // its region (the main thread keeps the baked span).
          if (rt.my.id !== 1) {
            const region = rtEnsureRegions(rt, wrec);
            swap(region.stackBase + wrec.meta.stackSize);
          } else {
            swap(baked);
          }
        }
        rt.writeI32(topsAddr + i * 4, baked);
      });
      rt.writeI32(payloadAddr + 36, topsAddr);
      const view = rt.view();
      Atomics.store(view, (cb + CB_LOAD_DONE) / 4, seq);
      Atomics.notify(view, (cb + CB_LOAD_DONE) / 4);
    } catch (err) {
      const bytes = rtEncoder.encode(
        err && err.message ? err.message : String(err));
      const m2 = new Uint8Array(rt.memory.buffer);
      rt.writeI32(payloadAddr + 40, bytes.length);
      m2.set(bytes, payloadAddr + 44);
      const view = rt.view();
      Atomics.store(view, (rt.boot.controlAddr + CB_LOAD_FAILED) / 4, seq);
      Atomics.notify(view, (rt.boot.controlAddr + CB_LOAD_FAILED) / 4);
      rt.lastTask.failed = true;
      rt.lastTask.error = rtDecoder.decode(bytes.slice());
    }
  } else if (rt.my.id === 1) {
    // The main replica adopts each new module's baked stack
    // top once the load completes (§3.1); until then its stack
    // pointer for the module stays unset and nothing may call
    // in — the handle only escapes once the load completes.
    rt.pendingMainSp.push({ seq, payloadAddr, mods });
  }
  rtAdoptPendingMainSp(rt);
}

function rtAdoptPendingMainSp(rt) {
  if (rt.my.id !== 1 || rt.pendingMainSp.length === 0) return;
  const cb = rt.boot.controlAddr;
  const done = rt.readI32(cb + CB_LOAD_DONE);
  rt.pendingMainSp = rt.pendingMainSp.filter((task) => {
    if (done < task.seq) return true;
    const topsAddr = rt.readI32(task.payloadAddr + 36);
    task.mods.forEach((mod, i) => {
      const wrec = rt.records.get(mod.canonical);
      if (!wrec || !topsAddr) return;
      const swap = wrec.instance.exports.__tinyc_dylink_swap_sp;
      if (typeof swap === 'function') {
        swap(rt.readI32(topsAddr + i * 4));
      }
    });
    return false;
  });
}

/** A host callExport delegated to the main thread's worker
 *  (§5.5): run it on this context and report by message. */
function rtServiceCall(rt, payloadAddr) {
  const dv = new DataView(rt.memory.buffer);
  const m = new Uint8Array(rt.memory.buffer);
  let p = payloadAddr;
  const rd = () => { const v = dv.getInt32(p, true); p += 4; return v; };
  const rstr = () => {
    const len = rd();
    const s = rtDecoder.decode(m.slice(p, p + len));
    p += len + ((4 - (len % 4)) % 4);
    return s;
  };
  const seq = rd();
  rd(); // result word (unused; the reply travels by message)
  const canonical = rstr();
  const name = rstr();
  const argc = rd();
  const args = [];
  for (let i = 0; i < argc; i++) args.push(rd());
  const wrec = rt.records.get(canonical);
  try {
    const fn = wrec && wrec.instance.exports[name];
    if (typeof fn !== 'function') {
      throw new Error(`tinyc: export "${name}" not found in "${canonical}"`);
    }
    const value = fn(...args);
    rt.port.postMessage({ type: 'call-done', seq, ok: true, value });
  } catch (err) {
    if (isExitSignal(err)) {
      rt.port.postMessage({ type: 'process-exit', code: err.exitCode | 0 });
      self.close();
      return;
    }
    if (err instanceof ThreadExitSignal) {
      rtThreadExit(rt, err.value);
      return;
    }
    rt.port.postMessage({
      type: 'call-done', seq, ok: false,
      message: err && err.message ? err.message : String(err),
    });
  }
}

// -- thread exit (§6.6) --

function rtRunKeyDestructors(rt) {
  const keyTable = rt.boot.keyTableAddr;
  if (!rt.my.keyArrayAddr) return;
  const view = rt.view();
  for (let pass = 0; pass < PTHREAD_DESTRUCTOR_ITERATIONS; pass++) {
    let called = false;
    for (let k = 0; k < PTHREAD_KEYS_MAX; k++) {
      const value = rt.readI32(rt.my.keyArrayAddr + k * 4);
      if (value === 0) continue;
      const dtor = Atomics.load(view, (keyTable + k * 4) / 4);
      if (!dtor) continue;
      rt.writeI32(rt.my.keyArrayAddr + k * 4, 0);
      const fn = rt.table.get(dtor);
      if (typeof fn === 'function') { fn(value); called = true; }
    }
    if (!called) break;
  }
}

function rtThreadExit(rt, value) {
  try { rtRunKeyDestructors(rt); } catch { /* exit proceeds */ }
  if (rt.my.recordAddr) {
    const view = rt.view();
    Atomics.store(view, (rt.my.recordAddr + TR_EXIT) / 4, value | 0);
    Atomics.store(view, (rt.my.recordAddr + TR_STATE) / 4, 1);
    Atomics.notify(view, (rt.my.recordAddr + TR_STATE) / 4);
  }
  rt.port.postMessage(
    { type: 'thread-exit', threadId: rt.my.id, exitValue: value | 0 });
  self.close();
}

// -- the real tier's pthread set (§6 over shared words) --

function createRealPthreadShims(rt) {
  const view = () => rt.view();
  const load = (addr) => Atomics.load(view(), addr / 4);
  const store = (addr, v) => Atomics.store(view(), addr / 4, v | 0);
  const cas = (addr, expect, repl) =>
    Atomics.compareExchange(view(), addr / 4, expect, repl);

  function keyArray() {
    if (!rt.my.keyArrayAddr) {
      rt.my.keyArrayAddr = rt.alloc.reserve(PTHREAD_KEYS_MAX * 4, 4);
    }
    return rt.my.keyArrayAddr;
  }

  // The mutex algorithm of §6.3, over shared words with the
  // §5.3 wait loop. Returns 0 or an error code; a deadline, when
  // given, is a Date.now() millisecond instant.
  function mutexLockLoop(addr, deadline) {
    const kind = load(addr + 12);
    if (kind === -1) return EINVAL;
    for (;;) {
      rtServicePending(rt);
      const state = load(addr);
      if (state === 0) {
        if (cas(addr, 0, 1) === 0) {
          store(addr + 4, rt.my.id);
          store(addr + 8, 1);
          return 0;
        }
        continue;
      }
      if (load(addr + 4) === rt.my.id) {
        if (kind === 2) {
          const count = load(addr + 8) + 1;
          if (count <= 0) return EDEADLK;
          store(addr + 8, count);
          return 0;
        }
        if (kind === 1) return EDEADLK;
        // Normal mutex, self-held: POSIX leaves this undefined;
        // the wait below never completes (the real tier makes
        // no deadlock promise, §12).
      } else if (state === 1) {
        // Mark contention with a CAS: a blind store could
        // stamp 2 onto a mutex the owner has just released,
        // stranding it ownerless with nobody left to notify.
        cas(addr, 1, 2);
      }
      if (deadline !== null && Date.now() >= deadline) return ETIMEDOUT;
      const v = load(addr);
      Atomics.wait(view(), addr / 4, v, POLL_MS);
    }
  }

  function mutexRelease(addr) {
    // Clear owner and count while the state still reads
    // held, then CAS the state itself to 0 — retrying across
    // a waiter's 1 -> 2 contention mark — so no interleaving
    // can erase a fresh claim or strand the mutex at 2 with
    // no owner.
    store(addr + 8, 0);
    store(addr + 4, 0);
    let prev = load(addr);
    for (;;) {
      const got = cas(addr, prev, 0);
      if (got === prev) break;
      prev = got;
    }
    if (prev === 2) Atomics.notify(view(), addr / 4, 1);
  }

  function mutexUnlockImpl(addr) {
    const kind = load(addr + 12);
    if (kind === -1) return EINVAL;
    if (load(addr) === 0) return EINVAL;
    if (load(addr + 4) !== rt.my.id) return EPERM;
    if (kind === 2) {
      const count = load(addr + 8) - 1;
      if (count > 0) { store(addr + 8, count); return 0; }
    }
    mutexRelease(addr);
    return 0;
  }

  function condWaitImpl(condAddr, mutexAddr, deadline) {
    const mtxKind = load(mutexAddr + 12);
    const savedCount = mtxKind === 2 ? load(mutexAddr + 8) : 1;
    const seq0 = load(condAddr);
    Atomics.add(view(), (condAddr + 4) / 4, 1);
    // Release the mutex whole, as one step (§6.4).
    mutexRelease(mutexAddr);
    let timedOut = false;
    for (;;) {
      if (load(condAddr) !== seq0) break;
      if (deadline !== null && Date.now() >= deadline) {
        timedOut = load(condAddr) === seq0;
        break;
      }
      if (rtWaitStep(rt, () => load(condAddr) !== seq0)) break;
    }
    Atomics.sub(view(), (condAddr + 4) / 4, 1);
    const rc = mutexLockLoop(mutexAddr, null);
    if (rc !== 0) return rc;
    if (mtxKind === 2) store(mutexAddr + 8, savedCount);
    return timedOut ? ETIMEDOUT : 0;
  }

  const shims = {
    pthread_create(threadPtr, attr, startSlot, arg) {
      rtServicePending(rt);
      let detached = 0;
      if (attr) {
        detached = rt.readI32(attr);
        if (detached !== 0 && detached !== 1) return EINVAL;
      }
      let id, recordAddr, keyAddr;
      const regions = {};
      try {
        id = Atomics.add(view(), HDR_TID_COUNTER / 4, 1) + 1;
        recordAddr = rt.alloc.reserve(32, 4);
        for (let i = 0; i < 32; i += 4) rt.writeI32(recordAddr + i, 0);
        if (detached) rt.writeI32(recordAddr + TR_DETACHED, 1);
        keyAddr = rt.alloc.reserve(PTHREAD_KEYS_MAX * 4, 4);
        for (const wrec of rt.records.values()) {
          const region = { stackBase: null, tlsArea: 0 };
          if (wrec.meta.stackSize !== undefined) {
            region.stackBase = rt.alloc.reserve(wrec.meta.stackSize, 16);
          }
          if (wrec.meta.tlsSize > 0) region.tlsArea = rtTlsAlloc(rt, wrec);
          regions[wrec.canonical] = region;
        }
      } catch {
        return EAGAIN;
      }
      if (id < 8192) rt.writeI32(rt.boot.dirAddr + id * 4, recordAddr);
      rt.port.postMessage({
        type: 'thread-spawn', threadId: id, slot: startSlot, arg,
        detached, recordAddr, keyArrayAddr: keyAddr, regions,
      });
      rt.writeI32(threadPtr, id);
      return 0;
    },
    pthread_join(target, retvalPtr) {
      if (target === rt.my.id) return EDEADLK;
      if (rt.joined.has(target)) return EINVAL;
      const recordAddr = target < 8192
        ? rt.readI32(rt.boot.dirAddr + target * 4) : 0;
      if (!recordAddr) return ESRCH;
      if (rt.detachedLocal.has(target) ||
          rt.readI32(recordAddr + TR_DETACHED) !== 0) return EINVAL;
      // One joiner process-wide (§6.2): the claim word is
      // CASed, so a second joiner — in any worker — gets
      // EINVAL, and the tombstone outlives the record's reap.
      if (cas(recordAddr + TR_JOIN_CLAIMED, 0, 1) !== 0) return EINVAL;
      rt.joined.add(target);
      rtWaitLoop(rt, () => load(recordAddr + TR_STATE) === 1);
      if (retvalPtr) rt.writeI32(retvalPtr, load(recordAddr + TR_EXIT));
      rt.port.postMessage({ type: 'thread-reap', threadId: target });
      return 0;
    },
    pthread_detach(target) {
      const recordAddr = target < 8192
        ? rt.readI32(rt.boot.dirAddr + target * 4) : 0;
      if (!recordAddr) return ESRCH;
      if (rt.detachedLocal.has(target)) return EINVAL;
      rt.detachedLocal.add(target);
      store(recordAddr + TR_DETACHED, 1);
      if (load(recordAddr + TR_STATE) === 1) {
        rt.port.postMessage({ type: 'thread-reap', threadId: target });
      }
      return 0;
    },
    pthread_self() { return rt.my.id; },
    pthread_equal(a, b) { return a === b ? 1 : 0; },
    pthread_exit(value) { throw new ThreadExitSignal(value | 0); },

    pthread_mutex_init(addr, attr) {
      let kind = 0;
      if (attr) {
        kind = rt.readI32(attr);
        if (kind !== 0 && kind !== 1 && kind !== 2) return EINVAL;
      }
      store(addr, 0); store(addr + 4, 0); store(addr + 8, 0);
      store(addr + 12, kind);
      return 0;
    },
    pthread_mutex_destroy(addr) {
      if (load(addr) !== 0) return EBUSY;
      store(addr + 12, -1);
      return 0;
    },
    pthread_mutex_lock(addr) { return mutexLockLoop(addr, null); },
    pthread_mutex_timedlock(addr, abstimePtr) {
      if (!abstimePtr) return EINVAL;
      return mutexLockLoop(addr, abstimeToDateMs(rt, abstimePtr));
    },
    pthread_mutex_trylock(addr) {
      const kind = load(addr + 12);
      if (kind === -1) return EINVAL;
      const state = load(addr);
      if (state === 0) {
        if (cas(addr, 0, 1) === 0) {
          store(addr + 4, rt.my.id);
          store(addr + 8, 1);
          return 0;
        }
        return EBUSY;
      }
      if (load(addr + 4) === rt.my.id && kind === 2) {
        const count = load(addr + 8) + 1;
        if (count <= 0) return EDEADLK;
        store(addr + 8, count);
        return 0;
      }
      return EBUSY;
    },
    pthread_mutex_unlock(addr) { return mutexUnlockImpl(addr); },

    pthread_cond_init(addr, attr) {
      let clock = 0;
      if (attr) {
        clock = rt.readI32(attr);
        if (clock !== 0 && clock !== 1) return EINVAL;
      }
      store(addr, 0); store(addr + 4, 0); store(addr + 8, clock);
      store(addr + 12, 0);
      return 0;
    },
    pthread_cond_destroy(addr) {
      if (load(addr + 4) !== 0) return EBUSY;
      return 0;
    },
    pthread_cond_wait(condAddr, mutexAddr) {
      return condWaitImpl(condAddr, mutexAddr, null);
    },
    pthread_cond_timedwait(condAddr, mutexAddr, abstimePtr) {
      if (!abstimePtr) return EINVAL;
      const clock = load(condAddr + 8);
      const abstimeMs = abstimeToDateMs(rt, abstimePtr);
      const deadline = clock === 1
        ? Date.now() + (abstimeMs - performance.now()) : abstimeMs;
      return condWaitImpl(condAddr, mutexAddr, deadline);
    },
    pthread_cond_signal(addr) {
      Atomics.add(view(), addr / 4, 1);
      Atomics.notify(view(), addr / 4, 1);
      return 0;
    },
    pthread_cond_broadcast(addr) {
      Atomics.add(view(), addr / 4, 1);
      Atomics.notify(view(), addr / 4, Number.MAX_SAFE_INTEGER);
      return 0;
    },

    pthread_once(addr, routineSlot) {
      const state = load(addr);
      if (state === 2) return 0;
      if (state === 0 && cas(addr, 0, 1) === 0) {
        const fn = rt.table.get(routineSlot);
        if (typeof fn === 'function') fn();
        store(addr, 2);
        Atomics.notify(view(), addr / 4, Number.MAX_SAFE_INTEGER);
        return 0;
      }
      rtWaitLoop(rt, () => load(addr) === 2);
      return 0;
    },

    pthread_spin_init(addr) { store(addr, 0); return 0; },
    pthread_spin_destroy(addr) {
      if (load(addr) !== 0) return EBUSY;
      return 0;
    },
    pthread_spin_lock(addr) {
      let spins = 0;
      for (;;) {
        if (cas(addr, 0, 1) === 0) return 0;
        // A real spin, with an occasional bounded pause (§6.5).
        if (++spins >= 2048) {
          spins = 0;
          rtServicePending(rt);
          const v = load(addr);
          if (v !== 0) Atomics.wait(view(), addr / 4, v, 1);
        }
      }
    },
    pthread_spin_trylock(addr) {
      return cas(addr, 0, 1) === 0 ? 0 : EBUSY;
    },
    pthread_spin_unlock(addr) { store(addr, 0); return 0; },

    pthread_key_create(keyPtr, dtorSlot) {
      const idx = Atomics.add(view(), HDR_KEY_COUNTER / 4, 1);
      if (idx >= PTHREAD_KEYS_MAX) {
        Atomics.sub(view(), HDR_KEY_COUNTER / 4, 1);
        return EAGAIN;
      }
      if (dtorSlot) {
        Atomics.store(view(), (rt.boot.keyTableAddr + idx * 4) / 4, dtorSlot);
      }
      rt.writeI32(keyPtr, idx);
      return 0;
    },
    pthread_key_delete(key) {
      if (key < 0 || key >= PTHREAD_KEYS_MAX) return EINVAL;
      if (Atomics.load(view(), (rt.boot.keyTableAddr + key * 4) / 4) === 0 &&
          key >= load(HDR_KEY_COUNTER)) return EINVAL;
      Atomics.store(view(), (rt.boot.keyTableAddr + key * 4) / 4, 0);
      return 0;
    },
    pthread_setspecific(key, value) {
      if (key < 0 || key >= PTHREAD_KEYS_MAX) return EINVAL;
      rt.writeI32(keyArray() + key * 4, value | 0);
      return 0;
    },
    pthread_getspecific(key) {
      if (key < 0 || key >= PTHREAD_KEYS_MAX) return 0;
      if (!rt.my.keyArrayAddr) return 0;
      return rt.readI32(rt.my.keyArrayAddr + key * 4);
    },

    pthread_attr_init(attr) {
      for (let i = 0; i < 16; i += 4) rt.writeI32(attr + i, 0);
      return 0;
    },
    pthread_attr_destroy() { return 0; },
    pthread_attr_setdetachstate(attr, value) {
      if (value !== 0 && value !== 1) return EINVAL;
      rt.writeI32(attr, value);
      return 0;
    },
    pthread_attr_getdetachstate(attr, ptr) {
      rt.writeI32(ptr, rt.readI32(attr) ? 1 : 0);
      return 0;
    },
    pthread_mutexattr_init(attr) { rt.writeI32(attr, 0); return 0; },
    pthread_mutexattr_destroy() { return 0; },
    pthread_mutexattr_settype(attr, kind) {
      if (kind !== 0 && kind !== 1 && kind !== 2) return EINVAL;
      rt.writeI32(attr, kind);
      return 0;
    },
    pthread_mutexattr_gettype(attr, ptr) {
      rt.writeI32(ptr, rt.readI32(attr));
      return 0;
    },
    pthread_condattr_init(attr) { rt.writeI32(attr, 0); return 0; },
    pthread_condattr_destroy() { return 0; },
    pthread_condattr_setclock(attr, clock) {
      if (clock !== 0 && clock !== 1) return EINVAL;
      rt.writeI32(attr, clock);
      return 0;
    },
    pthread_condattr_getclock(attr, ptr) {
      rt.writeI32(ptr, rt.readI32(attr));
      return 0;
    },
    sched_yield() { return 0; },
  };
  return shims;
}

// -- the worker's dlfcn (§5.4 from the requesting side) --

function createWorkerDlfcn(rt) {
  const ctx = () => rt.shimCtx;
  function setDlerror(message) { rt.dlerror = message; }
  function writeDlerror() {
    const m = new Uint8Array(rt.memory.buffer);
    const bytes = rtEncoder.encode(rt.dlerror);
    const n = Math.min(bytes.length, 255);
    m.set(bytes.subarray(0, n), 256);
    m[256 + n] = 0;
  }
  return {
    dlopen(pathPtr, flags) {
      if (!pathPtr) {
        const main = rt.mainCanonical
          ? rt.records.get(rt.mainCanonical) : null;
        return main ? main.handle : 0;
      }
      const path = ctx().readCString(pathPtr);
      const canonical = rt.helpers.canonicalModulePath(path, '/sysroot');
      const existing = rt.records.get(canonical);
      if (existing) return existing.handle;
      // Ask the coordinator to lay the module out; this call
      // waits in the §5.3 loop, servicing the very task it
      // waits for, and returns only once every context holds
      // its replica and the inits have run (§5.4).
      const cb = rt.boot.controlAddr;
      const prevSeq = rt.readI32(cb + CB_LOAD_SEQ);
      rt.port.postMessage({
        type: 'dlopen-request', path, flags, fromTid: rt.my.id,
      });
      for (;;) {
        rtServicePending(rt);
        const task = rt.lastTask;
        if (task && task.seq > prevSeq && task.requester === rt.my.id) {
          if (task.failed) { setDlerror(task.error); return 0; }
          if (rt.readI32(cb + CB_LOAD_DONE) >= task.seq) {
            const wrec = rt.records.get(canonical);
            if (wrec) return wrec.handle;
            setDlerror(`tinyc: dlopen: "${path}" did not load`);
            return 0;
          }
        }
        const view = rt.view();
        const v = Atomics.load(view, cb / 4);
        Atomics.wait(view, cb / 4, v, POLL_MS);
      }
    },
    dlsym(handle, namePtr) {
      const name = ctx().readCString(namePtr);
      let found = null;
      if (handle === 0) {
        for (const wrec of rt.records.values()) {
          if (!wrec.global && wrec.canonical !== rt.mainCanonical) continue;
          const e = wrec.exports.get(name);
          if (e) { found = { wrec, entry: e }; break; }
        }
      } else {
        const wrec = rt.byHandle.get(handle);
        const e = wrec && wrec.exports.get(name);
        if (e) found = { wrec, entry: e };
      }
      if (!found) {
        setDlerror(`tinyc: dlsym: symbol "${name}" not found`);
        return 0;
      }
      const { wrec, entry } = found;
      if (entry.tls) return rtTlsArea(rt, wrec) + entry.value;
      return entry.kind === 0
        ? wrec.tableBase + entry.value
        : wrec.memoryBase + entry.value;
    },
    dlclose(handle) {
      // Accounting and finalization are the coordinator's
      // (§5.1); the note is fire-and-forget.
      rt.port.postMessage({ type: 'dlclose-note', handle });
      return 0;
    },
    dlerror() {
      if (!rt.dlerror) return 0;
      writeDlerror();
      rt.dlerror = '';
      return 256;
    },
  };
}
