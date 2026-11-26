# Threads for wasm32 processes (pthread surface, two tiers) — design

Scope: a threading capability for the wasm32 target of the
browser package, built on the process model of
`wasm-dylink-design.md`. Guest C code compiled by TCC calls
the pthread API — declared in a shipped `pthread.h`, supplied
by the process exactly as `dlfcn.h`'s functions are — and runs
several threads inside one process, with the semantics the C
program expects: threads share the process's memory and see
each other's writes, a mutex excludes, a condition variable
waits and wakes, thread-local storage is per thread, and
`pthread_join` returns the value the thread exited with. The
primary service target is ordinary C source compiled by TCC
itself; translated-library bundles (the module kind of the
dylink design's §7) reuse the same process-layer scheduling
through the process services, and never carry a scheduler of
their own.

There is **one pthread surface and two execution tiers**. A
threaded process is assigned its tier once, at creation, by
capability, and never changes or mixes tiers:

- the **real tier** — one Web Worker per guest thread, one
  shared `WebAssembly.Memory`, blocking by `Atomics.wait` —
  when the creating context can supply a shared memory;
- the **simulated tier** — cooperative virtual threads inside
  the creating context, driven by one scheduler — everywhere
  else, which includes every `file:` context unconditionally.

The guest-visible ABI is identical in both tiers: the same
module form, the same pthread object layouts, the same error
codes. A module compiled once runs in either tier; nothing in
a module's bytes records or depends on the tier of the process
that loads it. Where this document speaks of an object the
dylink design already defines — the loader, the GOT, the
metadata, the handle space — that design's rules stand unless
this document says otherwise.

All file/line references are to the repository root unless a
path is given in full, as of this writing.

## 1. Current state

- **The process is single-threaded.** The dylink design
  lists threads among its non-goals (§11 there): the shared
  memory is "one module memory, not a wasm shared memory".
  `TinycProcess` creates a plain memory
  (`new WebAssembly.Memory({ initial: 1 })`,
  `packages/browser/index.mjs`, constructor) and one table,
  and every load, init and guest call runs on the creating
  context's one JS stack.
- **There is no pthread surface.** No `pthread.h` exists in
  the guest sysroot or in `wasm-backend/include/`; a program
  naming `pthread_create` fails to load with an unresolved
  function, in the package's named-gap style.
- **The backend emits no atomic instructions.** The lowering
  chain for C11 atomics dead-ends: `stdatomic.h` (shipped)
  maps `atomic_*` onto the `__atomic_*` builtins; the
  frontend lowers those to helper calls named
  `__atomic_<op>_<size>` (`vendor-tinycc/tccgen.c`, the
  `__atomic` template path ending in
  `sprintf(buf, "%s_%d", ...)` and `gfunc_call`); in wasm
  output those names are undefined symbols and become `env`
  imports no shim provides, so a program using
  `atomic_fetch_add` does not run. Plain `_Atomic` loads and
  stores fare no better semantically: the type bit is parsed
  (`VT_ATOMIC`, tccgen.c) but the backend's `load` / `store`
  (`wasm-backend/src/wasm-gen.c:1270`, `:1383`) ignore it and
  emit ordinary loads and stores. `atomic_thread_fence` is a
  plain extern function in the shipped `stdatomic.h`, i.e.
  another unresolved import. `__sync_*` builtins do not exist
  in this TCC at all — there are no such tokens — so no
  surface is owed for them.
- **`__thread` variables are silently ordinary variables.**
  `_Thread_local` / `__thread` parse to the `VT_TLS` type bit
  (tccgen.c), and the wasm backend never reads that bit: a
  `__thread` variable is laid out as a normal static, shared
  by any future threads.
- **`dlopen` is synchronous and local-only.** The guest
  `dlopen` shim calls the synchronous loader directly
  (index.mjs, `_dlfcnFor`); a library that is not already
  local fails with "not present locally", because retrieval
  (the transport of the dylink design's §13/§14) is
  asynchronous and a synchronous shim cannot await it. The
  host-side `loadModule` hides the seam by prefetching the
  closure first (`_prefetchClosure`, index.mjs). This is the
  suspended point §9 of this design closes for threaded
  processes.
- **The shim allocator keeps its state in JS.** The free
  list's links live in linear memory, but `heapTop` and
  `freeHead` are fields of a JS object
  (`packages/browser/runtime-shims.mjs`, `allocState`), one
  per shim set — a shape that cannot survive two execution
  contexts over one memory (§5.6).
- **Stacks are per module.** A dylink module's static layout
  contains a 64 KiB stack span, and its synthesized init sets
  its own `$__stack_pointer` to that span's top (dylink
  design §1.2, §3.3). One module therefore has exactly one
  stack, however many threads run its code. §3 is devoted to
  this.
- **JSPI is unused.** `WebAssembly.promising` /
  `WebAssembly.Suspending` appear nowhere in the package's
  guest execution paths. Workers exist only as `TinycWorker`,
  which hosts the compiler, never guest code.

## 2. Tiers and process creation

### 2.1 Determination

A process is created threaded unless it is created with
`threads: false`, which yields exactly the process of the
dylink design — no pthread imports are registered, and
everything in this document is inert. The run paths
(`TinycState.run`, the `-run` path of `Tinyc.callMain`)
always create threaded processes; a caller who supplies a
process to a run path chooses through that process's own
flag. A threaded process determines its tier at creation,
by probing, in the creating context:

The tier is **real** when all of the following hold, and
**simulated** otherwise:

- `SharedArrayBuffer` is available in the context;
- a shared memory can actually be constructed — the probe
  `new WebAssembly.Memory({ initial: 1, maximum: 65536,
  shared: true })` succeeds. The construction is the
  authoritative test; the context flags that normally gate
  it (`crossOriginIsolated`, i.e. an https or localhost
  origin served with COOP/COEP) are the explanation, not the
  criterion;
- the context can host workers (the `Worker` constructor
  exists) — the real tier is one worker per thread (§5).

A `file:` context never satisfies the first two conditions —
a `file:` page is not cross-origin isolated and has no
`SharedArrayBuffer` — so a `file:` process is always
simulated. That is a consequence of the probe, not a special
case in the code. The determined tier is exposed read-only
as `process.threadTier` (`'real'` or `'simulated'`; `null`
when `threads: false`). A creation option may force
`'simulated'` for testing (§11); forcing `'real'` where the
probe fails is a creation-time error naming the failed
condition.

### 2.2 What creation builds

Both tiers keep the `TinycProcess` object and the whole
loader apparatus of the dylink design in the creating
context: the module registry, the global symbol table, the
GOT maps, the handle table, layout assignment. What differs
is where guest code executes and what the memory is:

- **Simulated tier.** The memory is a plain
  `WebAssembly.Memory` as today, the table is the process's
  one table as today, and instantiation is the existing
  single-context path. The process gains a **scheduler**
  (§4) and registers the pthread shim set (§6) in its import
  layers. Creation also probes JSPI
  (`WebAssembly.promising`): the result is recorded, not
  required — it is needed only when a direct module first
  creates a thread or suspends (§4.2).
- **Real tier.** The memory is created shared, with
  `maximum: 65536` pages (the wasm32 4 GiB limit; a shared
  memory must name a maximum, and this one keeps growth
  effectively unbounded as today). The `TinycProcess` object
  becomes the **coordinator** (§5.1): it owns layout and the
  registry, and keeps a replica instance set used only for
  lifecycle operations. Guest code — the main thread
  included — runs in workers (§5.2).

In both tiers the allocator state moves into the memory
itself (§5.6): two words of the reserved `[0, 1024)` header
span hold the heap top and the free-list head, so the state
has one residence whatever the tier. Standalone
`runWasmModule` execution, which is not a process, keeps the
current JS-side allocator state unchanged.

### 2.3 The host run surface in a threaded process

Running guest code is scheduler-mediated (simulated) or
worker-mediated (real), so in a threaded process every host
operation that executes guest code is asynchronous:
`runMainModule` returns a promise for the same result shape
it returns synchronously today, and `TinycState.run` /
`callMain` propagate it. Host `loadModule` is already
asynchronous and keeps its shape; the synchronous host
`dlopen` throws a named error in a threaded process,
directing the caller to `loadModule`, because a load can now
run constructors on another context and wait for replicas
(§5.4). Pure bookkeeping and memory reads — `moduleHandle`,
host `dlsym`, `readExportI32` / `writeExportI32` — stay
synchronous: they run no guest code. `callExport` in a
threaded process returns a promise: in the simulated tier
the call goes through the scheduler as a host operation
(§4.1), in the real tier it is delegated to the main
thread's worker (§5.5).

**Process end** extends the dylink design's rule. A threaded
process ends when the first of these occurs:

1. any thread calls `exit` / `proc_exit` — the process ends
   with that code; other threads stop where they stand and
   their TLS destructors do not run, as under POSIX `exit`;
2. the main module's entry returns — the process ends with
   the entry's result, exactly as today, and any threads
   still running are terminated;
3. the main thread called `pthread_exit` and every other
   thread has since exited — the process ends with
   `{ exitCode: 0 }`, unless rule 1 fired first.

Teardown — module finalization in reverse load order —
follows the dylink design's §2/§8 unchanged; §5.5 states in
which context finalizers run in the real tier.

## 3. Per-thread execution state

Threads multiply the execution contexts over one set of
modules, and two pieces of per-module state are really
per-execution state: the stack pointer and the TLS base.
This section fixes both, plus the module-form revision they
require.

### 3.1 Stacks are per (thread, module)

The main thread (thread 1, §6.6) uses each module's baked
stack span — the span inside its static layout, whose top
its init already installs. Every further thread gets, **for
each loaded module, its own stack region** of the module's
stack size, taken from the reclaimable region allocator
(§5.6) when the later of the two events — thread creation,
module load — occurs. A thread's frames in a module
therefore live in that thread's region for the module; a
pointer to a stack local remains an ordinary shared-memory
address, readable by any thread that is handed it, for
exactly as long as native semantics say it is valid.

The runtime sets a module instance's stack pointer through
one new synthesized export, **`__tinyc_dylink_swap_sp`**,
present in every dylink module beside `__tinyc_dylink_init`
/ `__tinyc_dylink_fini`: it takes the new stack pointer and
returns the previous one. Swap is the only operation either
tier needs — the simulated scheduler saves and restores
around every switch (§4.3), a real-tier worker installs its
thread's tops at thread start and when a module arrives
(§5.2, §5.4) — and a single export serves both. A threaded
process requires the export: loading a module that lacks it
fails by name (§3.3). In a non-threaded process nothing
calls it and nothing changes.

A thread's regions are **returned to the region free lists**
when the thread exits (§5.5, §6.6): its per-module stacks,
its per-module TLS areas, its key array and its proxy block
(§5.7) go back to the exact-size class they came from, and
the record itself is freed once the exit value has been
taken (a join) or no joiner can come (detached). The main
thread's regions live to teardown. Thread ids and directory
slots are never reused — only the large regions recycle —
so a join can never observe another thread's record under
an old id. Module static regions are different: they are
reserved through `reserveStatic` and never freed, for the
module's life (dylink design §8); the reclaimable path is
for thread state only.

### 3.2 The metadata gains `stack_size`

Per-thread regions need the module's stack span as a number
separate from `mem_size`. The metadata (dylink design §3.4)
gains one field:

| field | content |
|---|---|
| `stack_size` | the module's stack span in bytes (64 KiB as the backend lays out today), included in `mem_size` |

The field is additive: a loader that does not know it
ignores it. It doubles as the revision marker of §3.3 — a
module whose metadata carries `stack_size` is a module of
the threaded revision.

### 3.3 Data segments become passive in the dylink flavors

In the real tier every worker instantiates every module
(§5.2). A module's active data segment writes its initial
bytes into the shared memory at instantiation — correct for
the first instantiation, and corruption for every later one:
a worker created mid-run would re-apply the initial values
over data the program has already mutated. The dylink
flavors therefore change how the initial bytes are applied:

- the data segment is emitted **passive**;
- `__tinyc_dylink_init` applies it — `memory.init` followed
  by `data.drop` — as its first step, before the stack
  pointer installation and the `.bss` zeroing of the dylink
  design's §3.3. The observable order is unchanged: the
  bytes are in place before any hook runs, exactly as
  instantiation used to guarantee;
- replica instantiation (§5.2) never calls init, so the
  bytes are applied exactly once per process, by the primary
  context's init;
- the element segment stays active: a table is per execution
  context in the real tier (§5.2), and each context needs
  the segment applied to its own table at instantiation.

The standalone flavor is untouched, byte-for-byte, as the
dylink design's §6 item 9 requires. A pre-revision loader
loads a revised module correctly — it calls init as it
always did, and init now applies the data — so the revision
is one-directional: a **threaded** process refuses a module
whose metadata lacks `stack_size`, naming the module and
the missing revision, because such a module's active
segments make replica instantiation unsafe. A non-threaded
process accepts both forms, as today.

## 4. The simulated tier

### 4.1 The scheduler and the rotation

The scheduler is a JS object owned by the process. It holds
the thread registry (§6.6), a FIFO queue of runnable
threads, the wait queues of §6, and the process's pending
host waits. It has two layers, and the distinction is the
tier's whole shape:

- the **inner rotation** is synchronous JS. It takes the
  head of the runnable queue, makes that thread's execution
  state current (§4.3), and resumes it. The thread runs
  until it exits, blocks in a shim, yields (§4.4), or the
  rotation's slice expires; the outcome is recorded and the
  rotation takes the next runnable thread.
- the **outer layer** is the asynchronous host world:
  transport retrievals, IndexedDB completions, timers. The
  rotation returns control to it in exactly three cases —
  every live thread is blocked; progress requires a host
  resource (a retrieval, a timed wait's timer); or the
  rotation's slice — a time budget, an implementation
  constant — is exhausted while threads remain runnable, so
  a process full of runnable threads cannot starve the host
  event loop. The run's promise stays pending across the
  return and the rotation re-enters from the outer layer
  when the awaited event lands or the slice renews.

A wake recorded against a blocked thread moves it to the
runnable queue's tail; a thread that yields appends itself
to the tail. Promise resolutions that resume parked stacks
happen **only inside the rotation** — shims record wakes,
they never resolve another thread's suspension — so exactly
one guest thread executes at any moment, however many places
a wake can originate.

The host is a caller, not a thread: a host operation that
executes guest code (a threaded `runMainModule`, a
`callExport`) is driven by the rotation like a thread's
step, and a blocking primitive reached from the host context
fails by name — the host has no stack to park.

### 4.2 How a direct module's thread runs: JSPI

A TCC-compiled module has a real call stack, so parking it
means parking a wasm stack. The mechanism is JSPI, and only
JSPI:

- a thread's entry — the main module's entry for thread 1, a
  start routine reached through the process table for every
  created thread — is invoked through
  `WebAssembly.promising`;
- every shim that can block (§6's primitives, `dlopen`,
  §9.1) is supplied to the process's modules wrapped in
  `WebAssembly.Suspending`. A blocking shim returns a
  promise exactly when it blocks and a plain value
  otherwise; the engine parks the calling stack until the
  scheduler resolves the promise, which happens only when
  the thread has been woken **and** the rotation selects it
  (§4.1) — wake and selection are separate steps, so a woken
  thread never resumes inside another thread's operation.

The main thread's first step includes loading the main
module and its closure, so even load-time constructors run
on a parkable stack and may themselves block or `dlopen`.

Where JSPI is absent (the creation probe, §2.2), the tier
still works for everything that never parks a direct stack:
a direct module's `pthread_create`, and a `dlopen` that
would have to suspend, fail **at the call**, by name, stating
that the engine lacks `WebAssembly.promising`. Nothing is
approximated in its place. Translated modules are
unaffected (§4.5).

### 4.3 Context switches

There is one instance set. Whenever the rotation changes
the running thread, it first swaps the execution state:

- for every loaded module, `__tinyc_dylink_swap_sp` with the
  incoming thread's saved value for that module; the
  returned values become the outgoing thread's saved state;
- for every instance importing a TLS base (§7.2), the
  loader-held `WebAssembly.Global` is set to the incoming
  thread's value.

A switch therefore costs O(modules) export calls, and
switches happen only at block, yield and exit points — never
per guest call. While a thread runs, the instance set's
stack pointers and TLS bases are that thread's; its stack
contents live in its own regions (§3.1) and are untouched by
other threads' execution.

### 4.4 Spin loops and forced yield

Cooperation has one sharp edge: a thread that never blocks
never yields. The shims close the part of that edge that
belongs to the surface: `pthread_spin_lock` (§6.5) spins in
the shim, counting iterations against a fixed budget (an
implementation constant); on exhausting it the shim yields
— the thread re-queues at the tail and the rotation moves
on — and resumes spinning when next selected. The same
counting is what a translated module's runtime does at its
own block boundaries, through the process services (§10).

A guest loop spinning on raw `_Atomic` variables in its own
code has no shim in it and no yield point; it starves the
simulated process until it blocks or exits. That is stated
as the tier's boundary (§12), not papered over: preemption
of arbitrary guest code does not exist in a cooperative
tier, and the real tier — where the same loop is merely a
busy worker — is the answer where it matters.

### 4.5 Translated modules' threads

A translated module's thread state is data — virtual
registers, a stack in memory, a next-block pointer — so its
runtime needs no stack parking: the scheduler drives it by
**step calls**. A stepped thread's step runs until the
thread blocks, yields (its runtime counts blocks against
the same kind of budget as §4.4), or exits, and reports the
outcome and the resource it blocked on. The scheduler's
registry, queues and primitives are shared verbatim between
stackful and stepped threads: a stepped thread waiting on a
mutex and a stackful thread waiting on the same mutex are
two entries in one queue, woken by one rule (§6). This is
the concrete content of "one scheduler, not one per module
kind": the pthread implementation lives in the process layer
(§10), and both thread kinds reach it — direct modules
through the `env` shims, bundle runtimes through the process
services.

### 4.6 Deadlock

When the rotation finds no runnable thread, the scheduler
classifies the standstill before returning to the outer
layer (§4.1): if any blocked thread waits on a host resource
or a live timer, it is not deadlock — the outer layer awaits
the earliest such completion. If every live thread is
blocked on another thread or on a synchronization object —
mutex, condition variable, join — and no host wait and no
timer is pending, it is a **true deadlock**, and the run
fails with an error naming every blocked thread and what it
waits on: for a mutex, its address and its owner's thread
id; for a condition variable, its address; for a join, the
target thread. Detection happens at exactly this point —
the no-runnable-thread state with nothing outstanding — so
it has no false positives from slow resources.

A trap in any thread is a process fault: the run fails with
the trap, naming the thread, and the process ends.

## 5. The real tier

### 5.1 Contexts and the coordinator

The creating context holds the coordinator: the
`TinycProcess` object with the registry, the symbol table,
the GOT maps and layout assignment, exactly as §2.2 states.
Guest threads execute in workers — **one worker per guest
thread at a time**. The main guest thread runs in a
dedicated worker when the process is created from a page's
main thread, because that context cannot block and this
tier's contract is that blocking is real; when the creating
context is itself a worker (a `TinycWorker` hosting a
process, a Node worker), the main thread may run in the
creating context, which can block. A worker is not reused
after its thread exits (§12).

The coordinator keeps a **replica instance set** of its own:
every loaded module, instantiated in the coordinator's
context, used only for lifecycle operations — the init of a
host-initiated load (§5.4) and finalization at teardown
(§5.5) — never to run program code. Replica instantiation
never runs init (§3.3); before a coordinator finalizer call,
the coordinator installs the module's baked stack top
through `__tinyc_dylink_swap_sp`, the main thread being gone
or the load being host-owned, so the finalizer runs on a
valid stack over the process's real statics.

Host functions — the JS functions the host supplies through
`options.shims` / `registerGlobalShim` — exist only in the
coordinator's context. A worker thread's call to one
crosses through the §5.7 proxy; the coordinator's own
replicas bind the functions directly, as does every
non-threaded and simulated-tier call.

### 5.2 Worker bootstrap and replica layout

Thread workers boot through the package's existing worker
entry (`packages/browser/worker.mjs`, the `TinycWorker`
entry) with a thread-mode init message, so no new shipped
file or URL derivation appears. The coordinator's bootstrap
message carries: the shared memory; the process's args and
env; and, per loaded module, its canonical path, its
assigned memory and table bases, its GOT fills, and its
compiled `WebAssembly.Module`. The worker then builds its
**replica**:

- it instantiates each module with its own import object —
  the shared memory; **its own table**, grown to the
  process's table layout; the assigned bases; its own GOT
  `WebAssembly.Global` objects set to the assigned values;
  function imports bound to its own instances' exports and
  its own shim set;
- it installs, per module, its thread's stack top
  (`__tinyc_dylink_swap_sp`) and TLS base (§7.2);
- it acknowledges readiness to the coordinator, and only
  then runs its thread's entry, called through its own
  table so the entry's identity is the process-wide slot.

**The table is replicated, not shared.** The dylink
design's "one table per process" becomes, in this tier, one
table *layout* with one table object per execution context.
A slot must hold the executing context's own instance
closures: a wasm function runs with its defining instance's
globals, and the stack pointer and TLS base are per-thread
state — a slot holding another worker's closure would run
the callee on that worker's stack. What C semantics depends
on is the *value*: the same slot number names the same
function in every context, so function pointers are
process-wide and compare equal across threads, exactly as
§3 of the dylink design requires. In the simulated tier the
table remains literally one object, as today.

The coordinator is the only layout authority. Bases, GOT
values, symbol decisions and reference counts are computed
once, there; a worker's replica differs from another's only
in which instance closures and shim wrappers fill the
agreed slots.

### 5.3 Blocking, and the polling wait

A worker may block: `Atomics.wait` is legal there. The
pthread primitives' waits are futex waits over words in the
guest objects themselves (§6.3, §6.4) — no JS wait queue
exists or could exist across workers.

One constraint shapes every wait in this tier: a worker
parked in a wait must still be able to instantiate a
replica when another thread loads a module (§5.4). Every
blocking wait a shim performs is therefore a **timed wait
in a loop**: wait on the primitive's word with a bounded
timeout (a few milliseconds; an implementation constant,
not ABI), then check the worker's pending-instantiation
queue, service it, and wait again. The same loop serves the
worker's own coordination waits (§5.4). Blocked threads pay
a bounded wake latency; running threads are never
interrupted, and §5.4 states the consequence for `dlopen`
completion honestly.

### 5.4 Loading a module mid-run

Layout is a coordinator act; instantiation is per context;
init runs once, in the primary context. A load — from the
host's `loadModule` or from a guest `dlopen` — proceeds:

1. The coordinator resolves the library and its closure
   (retrieving through the transport where needed — the
   coordinator's context does the asynchronous work, as the
   host path does today), computes the layout delta: memory
   and table bases, GOT fills, and, for the new module, one
   stack region and one TLS area per live thread (§3.1,
   §7.2).
2. The coordinator posts an instantiate task to every live
   worker and to its own replica set. Each worker services
   the task at its next wait point (§5.3) — a running worker
   at its next shim call, a blocked worker within its poll
   interval — instantiates its replica with the assigned
   layout, installs its thread's stack top and TLS base for
   the new module, and acknowledges.
3. The primary context — the requesting worker for a guest
   `dlopen`, the coordinator for a host load — instantiates
   as primary and runs the module's init (which applies its
   data, §3.3) once every context has acknowledged: no
   thread can call into the module before its replica exists
   everywhere, and no constructor runs before the module is
   callable process-wide.
4. A guest `dlopen` completes for its caller when step 3
   completes: the requesting worker, blocked since its
   request in the §5.3 wait loop (which is how it serviced
   its own instantiate task, in step 2), wakes to the
   coordinator's completion signal — a word in shared memory
   set and notified by the coordinator — and returns the
   handle. The completion carries the GOT fills for the new
   module's symbols, and every worker applies them to its
   own GOT globals before the signal is acted on, so a
   global of the new module reads its final value in every
   context from the moment the load completes.

A thread created during a load — a constructor calling
`pthread_create` — does not begin executing guest code until
that load completes: its worker is bootstrapped with the
post-load layout and its entry is gated on the coordinator's
load-complete signal. (Native loaders have the same hazard
in sharper form; the rule here removes it.)

The honest consequence of per-context replicas: a `dlopen`
completes only when every live worker has instantiated the
replica, and a thread that neither blocks nor calls a shim
for a long time delays completion for that long. §13 lists
the lazy alternative and why it is not this design.

### 5.5 Exit, join and teardown

A thread's worker runs its TLS destructors (§7.1), returns
its regions — its per-module stacks and TLS areas, its key
array and its proxy block (§5.7) — to the region free lists,
writes the exit value into its thread record — a small block
in shared memory the coordinator allocated at creation,
holding the state word, the exit value and the completion
word — sets the state to exited with `Atomics.notify`,
reports the exit to the coordinator, and terminates. A
joiner waits on the state word through the §5.3 loop, reads
the exit value, and has the coordinator reap the record (a
`thread-reap` message; the record is freed last, once the
value has been taken); a detached thread's record is reaped
by the coordinator on the exit report. The two messages
come from different workers and may arrive in either order,
so the coordinator reaps only once both have been seen. A
second join of an already reaped thread in another context
may report `ESRCH` instead of `EINVAL`; POSIX leaves that
case undefined.

Process end (§2.3) is a coordinator act: on `exit` from any
worker (reported by message) or on the main entry's return,
the coordinator runs finalization in its own replica set
(§5.1) in reverse load order and terminates the remaining
workers. A host `callExport` is delegated to the main
thread's worker: the coordinator posts the call, and the
worker executes it on the main thread's context at that
thread's next wait point, completing before the thread
resumes — a nested call on the main thread's stacks, which
push and pop symmetrically while the thread is parked. The
result returns by message; the host call is a promise
(§2.3).

### 5.6 The allocator and IO over shared memory

The shim allocator's free list already lives in memory; its
two JS-side words do not survive this tier. In every
process (both tiers), `heapTop` and `freeHead` become words
in the reserved header span — `[512, 516)` and `[516, 520)`
— read and written through the memory itself, and every
allocator mutation (including `reserveStatic`) is taken
under a spin lock on the header word at `[520, 524)`,
acquired with `Atomics.compareExchange` and released with
`Atomics.store` + `Atomics.notify`. The lock is never held
across a wait: allocator operations do not block, so the
spin is always brief, and in the simulated tier it is
uncontended by construction. The module-allocator forwarding
rule is unchanged — a module-exported allocator is called
through the calling context's own instance of the exporting
module; whether such an allocator is thread-safe is its own
property, as it is natively for a user-supplied allocator.

Two more header words serve §7 and §6.6: the pthread key
counter at `[524, 528)` and the thread id counter at
`[528, 532)`, both advanced with `Atomics.add` in the real
tier and by plain access in the simulated tier.

Thread regions do not come from the guest heap or from
`reserveStatic`. A **region class table** in shared memory
holds exact-size free lists — one class per (size, align)
pair, at most 64 classes — whose heads and operations live
under the same header spin lock. Allocating takes the head
of the matching class, or carves a fresh block from the
bump frontier when the list is empty (the carve happens
outside the lock, because `reserveStatic` takes the same
lock); freeing pushes the block onto its class's list.
Blocks up to 4096 bytes are zeroed on allocation, so a
reused record or key array starts clean; stacks take their
contents as unspecified. The allocator cannot serve a stack
or a TLS area from the guest heap (`rawMalloc` aligns only
to 8 bytes; stacks need 16, TLS areas their own alignment),
which is why thread regions have their own lists. A
non-threaded process never allocates a region: its
allocation call falls back to `reserveStatic` and its free
call is a no-op, so the 0.4.0 layout is byte-identical.

IO follows the process's existing shape. Output from a
worker is forwarded to the coordinator, which invokes the
process's `io` callbacks; ordering between workers is the
arrival order of those forwards, and interleaving of
concurrent writes is unspecified, as it is for native
threads writing one descriptor. `stdin` remains the fixed
byte string a process is created with: in the real tier it
is placed in shared memory at creation and consumed through
an atomic cursor, so concurrent readers split the stream
rather than duplicating it.

### 5.7 Host functions reach workers through a proxy

A host function exists only in the coordinator's context
(§5.1), and a worker has no way to call it directly. A
worker's binding for one is a **proxy stub**: a call writes
the arguments into the calling thread's proxy block — one
1024-byte request/answer area per thread in shared memory,
taken from the region class table (§5.6) on first use —
posts a wake message carrying only the block's address, and
blocks on the block's futex word. The coordinator never
blocks: on the message it reads the function name, the
argument type codes and the scalar arguments out of the
block, executes the host function, and writes the answer
back — a synchronous result at once, a thenable's when it
settles — then sets the state word and notifies. Pointer
arguments need no marshalling: the memory is shared, so
the coordinator dereferences the same address the guest
passed, and a host function reading or writing through a
pointer sees the guest's memory as it is. Scalar arguments
and results (`i32`, `i64`, `f32`, `f64`) are marshalled by
value; the block holds at most 16 arguments, and a call
outside that, or with an unmarshalable type, fails by name
in the stub. Blocks are per thread, so concurrent proxy
calls from different workers proceed independently and
complete in any order. While a worker waits on its proxy
block it still services loads (§5.4), so a host function
that triggers a module load does not wedge the caller; it
does not service host calls, so a host function that waits
on a guest thread that is itself blocked in a proxy call
deadlocks — the same shape as Emscripten's main-thread
proxying, and the caller's responsibility.

A host function that throws, or whose promise rejects,
reports back through the same block: the error text is
copied into a fixed 512-byte buffer (UTF-8, truncated) and
the stub throws a trap naming the host function and
carrying the text. The coordinator's own replicas bind the
host functions directly and never go through the proxy, and
the package's own shims still execute inside each worker;
only host functions are proxied. The simulated tier and
non-threaded processes call host functions directly, in
the same context, unchanged — including that a promise
returned by a host function is not awaited there.

## 6. The pthread surface

One surface, declared in a new header **`pthread.h`** in
`wasm-backend/include/` — the wasm-only include directory,
beside `dlfcn.h` — and registered in the wasm32 sysroot
manifests by the same route. The functions are `env` imports
supplied by the process, as every guest libc function is;
the header is declarations and the ABI-fixed layouts below,
nothing else. The surface is identical in both tiers; what
differs is only how a wait is realized (§6.3).

### 6.1 Types and object layouts

The layouts are this ABI's own, fixed by the header. A
synchronization object is guest-memory storage the program
allocates (statically or on a stack); the shims operate on
its words in place, so an object's meaning survives being
inspected in a memory dump and is the same in both tiers.

| type | layout |
|---|---|
| `pthread_t` | an `i32`: the thread's id from the process's monotonic counter (§6.6). Ids are never reused in a process; 0 is never a thread |
| `pthread_mutex_t` | `{ i32 state; i32 owner; i32 count; i32 kind; }` — `state`: 0 unlocked, 1 locked, 2 locked with waiters; `owner`: holding thread's id; `count`: recursion count; `kind`: 0 normal, 1 error-checking, 2 recursive; a destroyed mutex has `kind` −1 |
| `pthread_cond_t` | `{ i32 seq; i32 waiters; i32 clock; i32 reserved; }` — `seq`: the generation word waits sleep on; `clock`: 0 = `CLOCK_REALTIME`, 1 = `CLOCK_MONOTONIC` |
| `pthread_key_t` | an `i32` index, 0–127 (`PTHREAD_KEYS_MAX` is 128) |
| `pthread_once_t` | `{ i32 state; }` — 0 never run, 1 running, 2 done; `PTHREAD_ONCE_INIT` is `{ 0 }` |
| `pthread_spinlock_t` | one `i32`: 0 unlocked, 1 locked; `PTHREAD_SPIN_INITIALIZER` is 0 |
| `pthread_attr_t`, `pthread_mutexattr_t`, `pthread_condattr_t` | small fixed structs whose fields are the honored attributes of §6.2; their contents are private to the header |

`PTHREAD_MUTEX_INITIALIZER` is all zeroes, which the layout
reads as an unlocked normal mutex: static initialization
needs no init call, and `pthread_mutex_init` on a zeroed
object produces the same state. `pthread_mutex_destroy`
sets `kind` to −1, so use of a destroyed mutex is detectable
and fails with `EINVAL` rather than corrupting silently.

### 6.2 The function set and its error discipline

Errors are return values, never `errno`, as POSIX specifies
for this API. The set:

- **Threads:** `pthread_create`, `pthread_join`,
  `pthread_detach`, `pthread_self`, `pthread_equal`,
  `pthread_exit`. Detach state is settable through
  `pthread_attr_init` / `pthread_attr_destroy` /
  `pthread_attr_setdetachstate` /
  `pthread_attr_getdetachstate`; other thread attributes
  are accepted and round-trip through their accessors
  without effect (§13, question 4).
- **Mutexes:** `pthread_mutex_init`, `pthread_mutex_destroy`,
  `pthread_mutex_lock`, `pthread_mutex_trylock`,
  `pthread_mutex_timedlock`, `pthread_mutex_unlock`, with
  `pthread_mutexattr_init` / `_destroy` / `_settype` /
  `_gettype` carrying the kind.
- **Condition variables:** `pthread_cond_init`,
  `pthread_cond_destroy`, `pthread_cond_wait`,
  `pthread_cond_timedwait`, `pthread_cond_signal`,
  `pthread_cond_broadcast`, with
  `pthread_condattr_init` / `_destroy` / `_setclock` /
  `_getclock`.
- **Once:** `pthread_once`.
- **Spin locks:** `pthread_spin_init`, `pthread_spin_destroy`,
  `pthread_spin_lock`, `pthread_spin_trylock`,
  `pthread_spin_unlock`.
- **Thread-local storage:** `pthread_key_create`,
  `pthread_key_delete`, `pthread_setspecific`,
  `pthread_getspecific` (§7.1).
- **Scheduling:** `sched_yield` — a rotation yield in the
  simulated tier, a no-op returning 0 in the real tier,
  where the OS scheduler owns the decision.

Error codes, by the cases programs actually test:
`pthread_create` returns `EAGAIN` when the tier cannot
supply another execution context (a worker that fails to
start, in the real tier) and `EINVAL` for a bad attribute;
`pthread_join` returns `EINVAL` for a detached or already
joined thread, `EDEADLK` for joining oneself, `ESRCH` for
an unknown id; mutex lock returns `EDEADLK` when an
error-checking or recursive mutex is relocked past its
rules by its owner (error-checking: any relock; recursive:
count overflow), `EINVAL` for a destroyed mutex; unlock by a
non-owner returns `EPERM` — POSIX mandates that for
error-checking and recursive mutexes and leaves it
undefined for normal mutexes, where this ABI chooses the
detectable behavior over the silent one; `trylock` returns
`EBUSY`; destroying a locked mutex or a waited-on condition
variable returns `EBUSY`; `pthread_cond_timedwait` and
`pthread_mutex_timedlock` return `ETIMEDOUT`;
`pthread_key_create` returns `EAGAIN` when the 128 keys are
exhausted. `pthread_cancel` is not in the surface (§12).

### 6.3 Mutexes

The algorithm is one, with two realizations of "wait":

**Lock.** Fast path: if `state` is 0, claim it (state 1,
owner = caller, count 1) and return. If the caller already
owns it: recursive — increment `count`, return;
error-checking — return `EDEADLK`; normal — the caller
deadlocks against itself, and the wait below simply never
wakes (in the simulated tier the deadlock detector of §4.6
names it). Otherwise set `state` to 2 and wait until woken,
then retry the fast path: ownership is **not** handed off —
a woken waiter contends again, so barging is possible and
fairness is not promised, which POSIX permits.

- *Simulated:* "wait" is the scheduler's queue for this
  mutex's address. The shim enqueues the calling thread and
  suspends (§4.2). Unlock, when waiters exist, marks the
  first waiter runnable; the word stays at 2 until a waiter
  re-claims it.
- *Real:* "wait" is `Atomics.wait` on the `state` word
  while it reads 2, inside the §5.3 polling loop. Unlock
  stores 0 (or 1→0) and `Atomics.notify` on the word when
  the previous state was 2.

**Unlock.** Verify ownership per §6.2; recursive: decrement
`count`, release at 0. Release: store `state` 0; if waiters
were recorded, wake one (simulated: dequeue the first
waiter to runnable; real: notify one on the word).

**Timed lock** is the same loop against a deadline: the
simulated tier races the wait against a timer (§4.1's outer
layer) and returns `ETIMEDOUT` when the timer wins; the
real tier's polling loop checks the deadline between waits.
On timeout the caller does not hold the mutex and any queue
entry it had is withdrawn before returning.

### 6.4 Condition variables

`wait(cond, mutex)` requires the caller to hold `mutex`
(POSIX leaves the contrary undefined; the shim does not
check). The steps, in an order that is atomic with respect
to other threads of the process — trivially so in the
simulated tier, where no other guest thread runs during a
shim call, and by the sequence below in the real tier:

1. record the cond's current `seq`;
2. increment `waiters`; release `mutex` (§6.3);
3. wait until `seq` differs from the recorded value —
   simulated: enqueue on the cond's queue and suspend;
   real: `Atomics.wait` on the `seq` word with the recorded
   value, in the §5.3 loop;
4. decrement `waiters`; re-acquire `mutex` (§6.3, blocking
   as needed); return 0 — or, if the wait timed out,
   `ETIMEDOUT`, still holding `mutex`, as POSIX requires.

`signal` increments `seq` and wakes one waiter (real:
`Atomics.notify` on `seq` after the increment; simulated:
move the cond queue's head to the mutex's queue — the
waiter then contends for the mutex as any locker, and its
`wait` returns only once it holds it). `broadcast`
increments `seq` and wakes all waiters the same way. A
signal with no waiters is lost, as POSIX provides. Spurious
wakeups are permitted by POSIX and this design does not
manufacture them; a woken waiter whose `seq` check fails in
the real tier (a notify it was not the target of) simply
waits again.

`timedwait` adds the deadline machinery of §6.3's timed
lock, against the cond's clock: the abstime is converted
once, at entry, using the process's clock (the existing
shim clock: `Date.now` for `CLOCK_REALTIME`,
`performance.now`-based for `CLOCK_MONOTONIC`).

### 6.5 Once and spin

`pthread_once`: state 0 → the first caller claims it
(state 1) and runs the init routine **without holding any
wait open** — other callers wait (queue / futex on the
state word) until the runner stores 2 and wakes them; if
the init routine itself calls `pthread_once` on the same
control, that is the POSIX-undefined recursive case and it
deadlocks like one. A completed once never runs again;
`pthread_once` returns 0 in all defined cases.

Spin locks are the surface's one busy primitive, and the
two tiers treat them as differently as their models demand.
Simulated: the shim loops on the word with
`Atomics.compareExchange` over the memory view, counting
iterations against the §4.4 budget and yielding on
exhaustion — a spin lock is therefore safe to use in the
simulated tier, which is the entire reason the counting
exists. Real: the shim's loop is a plain `Atomics`
compare-exchange spin with an occasional `Atomics.wait`
pause on the word; the OS schedules the holder meanwhile.
`trylock` is one compare-exchange in both tiers; unlock is
a release store (plus a notify in the real tier, for any
paused spinner).

### 6.6 Thread lifecycle

**Ids and records.** Thread ids come from the process's
monotonic counter (header word, §5.6); the main thread is
id 1 and exists from process creation. Every thread has a
record: simulated — a scheduler entry holding its saved
stack pointers (§4.3), its TLS state (§7), its join state
and its exit value; real — the shared-memory record of
§5.5 plus a coordinator entry. A record lives until the
thread is joined or, if detached, until its exit has been
processed. In the real tier the coordinator and the workers
find a record by id through a **two-level directory** in
shared memory: a table of 4096 block pointers, each block
covering 8192 consecutive ids (32 KiB) and allocated on
demand from the shared allocator, zeroed, and published by
compare-exchange — a reader never sees an unpublished or
half-zeroed block, and a lost publish race discards the
loser's block. The table therefore spans 33,554,432
cumulative thread ids; past that a new thread runs
unregistered and its join reports `ESRCH`, as a
pre-revision table did past 8192. A reaped thread's entry
is cleared to zero; the id itself is never reused.

**Create.** `pthread_create` allocates the id and the
record, reserves the thread's per-module stack regions and
TLS areas (§3.1, §7.2), and makes the thread runnable:
simulated — its entry (the start routine's slot and the
argument) is queued for the rotation; real — a worker is
spawned and bootstrapped (§5.2) and runs the entry once
ready. Creation returns once the thread exists and can be
joined; it does not wait for the thread to have run. A
thread created during a module load starts executing only
after that load completes (§5.4; the simulated tier applies
the same rule at its load boundary).

**Exit.** Returning from the start routine is
`pthread_exit` with the return value. Exit processing, in
order: run the thread's TLS key destructors (§7.1); return
the thread's regions to the free lists (§3.1); store the
exit value in the record; mark the thread exited and wake
every joiner; if detached, reap the record. The main
thread's `pthread_exit` does not end the process by itself
— §2.3 rule 3 governs.

**Join.** Joining a live thread blocks the joiner (queue /
state-word wait) until the target's exit processing stores
its value; the joiner then reads the value, the record is
reaped, and join returns 0. The error cases are §6.2's.
Joining is the only way an exit value is delivered; an
unjoined joinable thread's record persists to process end,
as a zombie does.

## 7. Thread-local storage

Two different mechanisms share the name; the design keeps
them separate, as C does.

### 7.1 `pthread_key` storage

Keys are process-wide: a key is an index handed out from
the header counter word (§5.6), and the process keeps a
table of up to 128 destructor slots (function pointers —
table slots, process-wide by §5.2's layout identity),
written once at `pthread_key_create` and cleared by
`pthread_key_delete`. In the real tier the table lives in a
reserved block of shared memory; in the simulated tier the
scheduler holds it, with the same contents.

Values are per thread and live in memory: at thread
creation the process allocates a **value array** of 128
`i32` words for the thread, zeroed. `pthread_setspecific` /
`pthread_getspecific` are then plain accesses to the
calling thread's array at the key index — after creation
they involve no scheduler or coordinator traffic in either
tier; each execution context simply knows its current
thread's array address (a scheduler field in the simulated
tier, a worker-local in the real tier).

At thread exit, destructors run in the POSIX pattern:
repeat passes over the array; in each pass, for every
non-null value whose key has a destructor, clear the slot
and call the destructor with the value; stop when a pass
calls nothing, after at most `PTHREAD_DESTRUCTOR_ITERATIONS`
(4) passes. In the simulated tier the scheduler runs them
as part of exit processing, on the exiting thread's own
context before its stack is released; in the real tier the
thread's worker runs them before reporting the exit.

### 7.2 `__thread` variables

The backend change is bounded and follows the base-relative
pattern the dylink ABI already uses. A symbol carrying
`VT_TLS` is no longer laid out in the module's data span.
Instead:

- the module's TLS variables occupy a **TLS span** of their
  own, laid out among themselves with the module's existing
  alignment discipline. The metadata gains `tls_size` and
  `tls_align` for the span, and `tls_template`: the span's
  initialized bytes, base64 — the `.tbss` tail is the span's
  remainder and has no bytes. The template lives in the
  metadata, not in a segment, because the loader — not any
  instantiation — is what initializes per-thread areas;
- address materialization for a TLS symbol defined in the
  module emits `global.get` of an imported mutable `i32`
  global, **`env.__tls_base`**, plus the symbol's span
  offset — the same shape as a `GOT.mem` read, one global
  per module that defines or references TLS. A module with
  no TLS symbols imports no such global, keeping the import
  list exact;
- an **extern** TLS variable (defined in another module) is
  materialized through a GOT-style global, `GOT.tls.<name>`:
  the loader keeps one such global per importing instance
  and sets it to (the defining module's TLS area for the
  executing thread) + (the symbol's offset in the definer's
  span). It is refreshed at the same moments as the stack
  pointers: at every simulated context switch (§4.3) and at
  real-tier thread start and module arrival (§5.2, §5.4).

The loader allocates one **TLS area per (thread, module)**:
`tls_size` bytes, reserved when the later of thread
creation and module load occurs, initialized by copying
`tls_template` and zeroing the rest — the `.tdata` / `.tbss`
split, done by the loader because only the loader knows
both events. The area's address is the value installed in
the instance's `__tls_base`. The main thread's areas are
allocated at module load, thread 1 existing from creation.
Areas are returned to the region free lists with the
thread, under §3.1's rule.

A program that never creates a thread still gets correct
`__thread` semantics: its one thread has its areas, and a
TLS variable behaves as an ordinary variable that happens
to live in its span. In a non-threaded process the same
machinery runs with the single thread — which is what makes
the previously silent sharing (§1) simply correct, in every
process form.

## 8. Atomic instructions in the wasm backend

This section is the backend work the real tier stands on —
and, because wasm atomic instructions are equally valid
over a non-shared memory, it improves the simulated tier
and standalone modules at the same time: an atomic
operation over an unshared memory is single-context by
construction and behaves as its plain counterpart, only
correctly ordered.

### 8.1 What is emitted

The emitted set is the integer core of the wasm threads
proposal, at the widths the frontend can produce (1, 2, 4
and 8 bytes — the frontend already restricts `__atomic`
targets to powers of two up to 8):

- **loads:** `i32.atomic.load`, `i64.atomic.load`, and the
  sub-width `load8_u` / `load16_u` (and `load32_u` for i64);
- **stores:** `i32.atomic.store`, `i64.atomic.store`, and
  the sub-width stores;
- **read-modify-write:** `atomic.rmw.add`, `.sub`, `.and`,
  `.or`, `.xor`, `.xchg`, `.cmpxchg`, for i32 and i64 and
  their sub-width forms;
- **fence:** `atomic.fence`.

**`memory.atomic.wait32/64` and `memory.atomic.notify` are
not emitted.** No compiler path generates them — there is
no builtin for a futex wait in the source language's
surface here — and the pthread shims' blocking is JS-side
in both tiers (`Atomics.wait` in the real tier, scheduler
queues in the simulated tier). Emitting instructions
nothing can reach would be surface without a consumer.

Every emitted atomic carries the natural alignment for its
width in its memarg. An unaligned atomic access traps, per
the specification; that is the defined behavior, stated
here because native targets tolerate some unaligned atomic
accesses and this one cannot express them.

Memory orderings are **strengthened, uniformly**: the wasm
atomic instructions are sequentially consistent and carry
no ordering immediate, so every C ordering — relaxed
through seq_cst — compiles to the same instruction. The
result is never weaker than the source asked for. The one
place an ordering changes what is emitted is the fence:
`atomic_thread_fence` (§8.2) emits `atomic.fence` for any
ordering; a fence is never incorrect, merely stronger than
a relaxed one needed.

### 8.2 Where emission hooks in

Two interception points, both in `wasm-backend/src/wasm-gen.c`:

1. **The `__atomic_*` helper calls.** The frontend lowers
   every `__atomic_*` builtin and, through the shipped
   `stdatomic.h`, every C11 `atomic_*` operation to a call
   to `__atomic_<op>_<size>` (§1). In `gfunc_call`'s
   handling of a call to an undefined symbol, a callee of
   that name family is not emitted as an import; the call
   is emitted inline, with the operands the generic call
   path has already staged and the helper's result
   convention:
   - `load` / `store` / `exchange` map to the single
     instructions of §8.1;
   - `compare_exchange` maps to a short sequence: load the
     expected value, `atomic.rmw.cmpxchg`, on failure store
     the actual value back through the expected pointer,
     return the comparison result — the helper's contract,
     in instructions;
   - `fetch_<op>` for add/sub/and/or/xor maps to the single
     `rmw` instruction, which returns the previous value,
     as the helper does; the `<op>_fetch` forms and
     `fetch_nand` — whose results (the new value; the
     complemented conjunction) no single `rmw` produces —
     compile to a `cmpxchg` retry loop: load, compute,
     `cmpxchg`, repeat while the comparison fails;
   - `atomic_thread_fence` — an extern function today
     (§1) — is intercepted by name the same way and emits
     `atomic.fence`; `atomic_signal_fence` emits nothing:
     it is a compiler-only barrier, and this compiler has
     no optimizer for a barrier to constrain, so the empty
     expansion is the complete one;
   - `__atomic_is_lock_free` / `__atomic_always_lock_free`
     fold to the constant 1 for sizes up to 8 (and the
     frontend rejects larger `__atomic` targets before this
     point in any case).
2. **Plain `_Atomic` accesses.** In `load` and `store`
   (wasm-gen.c:1270, :1383), before the opcode is chosen
   from the base type, an lvalue whose type carries
   `VT_ATOMIC` selects the atomic form of the load or store
   for scalar integer and pointer types. `_Atomic`
   floating-point has no atomic form — the instruction set
   contains no floating-point atomics at all — and compiles
   to the ordinary load or store, as it does today; that is
   the instruction set's boundary, recorded here rather
   than discovered by a user.

Emission is unconditional: it does not depend on the output
flavor or on any process property (the backend cannot know
either). A standalone module using `atomic_fetch_add` —
today an unresolved import — simply runs. One engine-side
consequence is stated for completeness: a module containing
atomic instructions validates only where the engine
supports the threads proposal, shared memory or not; the
package's existing capability-probe style (§2.1) is how a
host discovers that, and no fallback emission exists.

## 9. `dlopen` and the suspension point

The dylink design's loader is synchronous; retrieval is
not. In a non-threaded process that seam stays exactly
where 0.4.0 put it — guest `dlopen` requires the library to
be local, and the host's `loadModule` prefetches. A threaded
process has somewhere to put the wait, in both tiers, and
closes the seam:

### 9.1 Simulated tier

The `dlopen` shim is a suspending import (§4.2). On a local
miss it runs the host path's own prefetch — the transport
retrieval of the closure, through `_prefetchClosure`'s
rules and the dylink design's §13 discipline (local first,
per library; write-back best-effort) — with the calling
thread parked on the retrieval: other threads rotate
meanwhile, and when the closure is local the shim completes
the load synchronously, as the loader does today, and
returns the handle. A guest `dlopen` in this tier therefore
fails only for the reasons a completed retrieval still
fails: the library is absent at the origin (the error names
the library and the URL tried), it is not a dylink module,
or a symbol cannot be resolved. Non-locality is no longer
among the failure reasons. The script transport under
`file:` is asynchronous in the same way (a load event), so
the same suspension covers it — the tier `file:` always
gets is the tier where this works.

### 9.2 Real tier

A guest `dlopen` is a coordination operation (§5.4): the
requesting worker blocks in its §5.3 wait loop while the
coordinator retrieves, lays out, and collects every
context's replica instantiation, then signals completion.
The failure reasons after a completed coordination are the
same list as §9.1's. A host `loadModule` follows the same
protocol with the coordinator as primary (§5.4).

### 9.3 What does not change

`dlopen`'s contract otherwise — handles, flags, scoping,
reference counting, `dlerror` — is the dylink design's §8
and §9, unamended. In a real-tier process the `dlerror`
buffer at `[256, 512)` is written by whichever context
performed the failing operation and read by the caller's
own next `dlerror()`; concurrent `dlerror` use from several
threads is the caller's race, as it is natively. Loading a
module into a threaded process additionally allocates its
per-thread state (§3.1, §7.2) and, in the real tier,
follows §5.4; the load algorithm itself — order, identity,
rollback — is the existing one.

## 10. Change surface

**`wasm-backend/src/wasm-gen.c`**

1. *Atomic emission* (§8.2): the `__atomic_*` helper-name
   interception in the undefined-symbol call path; the
   `VT_ATOMIC` check in `load` / `store`.
2. *TLS materialization* (§7.2): a symbol carrying `VT_TLS`
   classifies into the module's TLS span instead of a data
   section; its address materializes as `__tls_base` +
   offset for a defined symbol, or through a `GOT.tls`
   global for an undefined one — a third GOT kind beside
   `func` and `mem`, recorded by the same fixup-registry
   discipline the dylink design's §6 item 2 established.

**`wasm-backend/src/wasm-link.c`**

3. *Passive data segment* (§3.3): in the dylink flavors the
   data segment is emitted passive, and the synthesized
   init applies it (`memory.init`, `data.drop`) as its
   first step. The element segment stays active.
   Standalone output is byte-identical to today.
4. *`__tinyc_dylink_swap_sp`* (§3.1): one more synthesized
   function, appended to the registries like init and fini,
   exported always in the dylink flavors; its body is a
   read of `$__stack_pointer`, a store of the argument, and
   a return of the read value.
5. *Metadata*: `stack_size` always (§3.2); `tls_size`,
   `tls_align`, `tls_template` when the module has TLS
   symbols (§7.2) — the TLS layout pass runs with the
   existing section layout, assigning span offsets instead
   of section bases.

**Headers and manifests**

6. *`wasm-backend/include/pthread.h`* (§6): the surface's
   declarations and layouts; registered in the wasm32
   sysroot manifests by `dlfcn.h`'s route. It stays out of
   `vendor-tinycc/include/` for the reason `dlfcn.h` does:
   that directory feeds the native self-hosting build.

**`packages/browser/`**

7. *`threads.mjs` (new)*: the scheduler (§4), the pthread
   implementation (§6) as process-layer JS functions over
   guest addresses and thread ids — called by the `env`
   shims for direct modules and exposed to bundle runtimes
   as a process service (§4.5) — and the real tier's
   coordinator protocol and worker-side runtime (§5).
8. *`index.mjs`*: `TinycProcess` gains the tier probes and
   the `threads` option (§2), the thread registry, the
   memory-resident allocator wiring (§5.6), the threaded
   forms of the host surface (§2.3), the per-thread region
   allocation hooks in the load path (§3.1, §7.2), and the
   coordinator/replica split of instantiation in the real
   tier (§5.2, §5.4). The single-context load algorithm is
   the simulated and non-threaded path, unchanged.
9. *`runtime-shims.mjs`*: the allocator's state words move
   into the header span under the §5.6 lock, for processes;
   the pthread shim set is registered per process from
   `threads.mjs`, wrapped in `WebAssembly.Suspending` where
   §4.2 requires; worker-side shim sets bind to the shared
   memory and forward IO to the coordinator (§5.6).
10. *`worker.mjs`*: a thread-mode branch in the existing
    worker entry (§5.2): on a thread bootstrap message it
    loads `threads.mjs`'s worker runtime instead of a
    `Tinyc` instance and never returns to compiler duty.

## 11. Verification

The suite is written once against the pthread surface —
guest C programs compiled by the package's own compiler,
run under a process — and executed in **both tiers**, with
the tier forced per run: `createProcess` accepts a tier
override for tests (`'auto'` is the only production value;
forcing `'real'` where the probe fails errors by name,
§2.1). The Chromium consumer setup serves the real-tier
pages over http with COOP/COEP headers so the probe passes;
the `file:` suite asserts `threadTier === 'simulated'` on
its face and runs the simulated suite there — which is also
the standing proof that `file:` cannot reach the real tier.
In the Node suites, where `SharedArrayBuffer` exists, the
override runs each case in the tier under test.

Cases, one per rule:

- **Create/join.** N threads increment disjoint counters
  and return distinct values; the joiner collects every
  exit value exactly; a second join and a join of a detached
  thread return their §6.2 codes.
- **Mutex exclusion.** N threads × M increments of one
  shared counter under one mutex produce the exact total —
  in the real tier this is the case that proves the atomic
  emission and the futex path together; in the simulated
  tier, that the rotation interleaves at block points.
- **Mutex kinds.** Recursive relock counts and releases;
  error-checking relock returns `EDEADLK`; non-owner unlock
  returns `EPERM`; `trylock` on a held mutex returns
  `EBUSY`; destroying a locked mutex returns `EBUSY`.
- **Condition variables.** A bounded-buffer producer /
  consumer pair moves an exact item count; `timedwait` on
  an unsignalled cond returns `ETIMEDOUT` inside a time
  window and returns holding the mutex; broadcast wakes
  every waiter exactly once each.
- **Once.** Concurrent first calls from every thread run
  the initializer exactly once.
- **Spin.** A counter under a guest spin lock reaches the
  exact total; simulated tier only: a thread spinning on a
  spin lock held in short bursts by another thread does not
  starve it — both progress (§4.4's forced yield).
- **Key TLS.** Two threads hold different values under one
  key simultaneously; destructors run the counted number of
  times at exit, including the multi-pass case (a
  destructor that sets another key's value).
- **`__thread`.** Two threads write and read back distinct
  values of one `__thread` variable, initialized and
  uninitialized forms — proving the template copy and the
  per-thread areas; an extern `__thread` variable defined
  in a side module behaves the same through `GOT.tls`.
- **Atomics.** A concurrent `atomic_fetch_add` storm sums
  exactly in both tiers; an `atomic_flag` guest spin lock
  excludes correctly; a standalone (non-process) module
  using `atomic_fetch_add` runs and returns the right
  result — the §8.2 consequence, previously an unresolved
  import.
- **`dlopen` from threads.** A created thread `dlopen`s a
  side module that is deliberately absent locally: the load
  suspends (simulated) / coordinates (real), succeeds, and
  the thread calls into it; the main thread then `dlopen`s
  a second module and an older thread calls into it —
  proving per-thread regions are allocated for modules that
  arrive after threads exist. Under `file:`, the same case
  runs through the script transport.
- **Deadlock (simulated tier).** Two threads take two
  mutexes in opposite orders under a forced interleaving;
  the run fails with the §4.6 error naming both threads,
  both mutex addresses and the owner ids. The real tier
  makes no such promise (§12) and the case is not run
  there.
- **Exit semantics.** A thread calling `exit(7)` ends the
  run with code 7 while another thread is blocked; a main
  thread that calls `pthread_exit` while a worker thread
  finishes the computation ends the run per §2.3 rule 3.
- **No-JSPI environment.** Where `WebAssembly.promising` is
  absent, a direct module's `pthread_create` fails with the
  §4.2 named error and the rest of the process is
  unaffected; the case is gated on the probe, and runs
  wherever an engine without JSPI is available to the
  suite.
- **Host proxy.** A created thread and the main thread call
  host functions through the §5.7 proxy: integer, 64-bit,
  float and double arguments and results arrive exact; a
  pointer argument lets the host read and write the shared
  memory in place; a host function returning a promise
  completes asynchronously. The simulated tier asserts the
  same calls through its direct binding, except the
  asynchronous answer, which the direct binding does not
  await.
- **Host failure.** A host function that throws fails the
  run with an error naming the function and carrying the
  host's text, in both tiers.
- **Directory boundary.** With the process's thread-id
  counter preset just below 8192 (a test-only seam, not a
  public API), the threads created straddle the first block
  boundary and enter the second block; every one joins with
  its exit value and detaches with its §6.2 code. Real tier
  only — the simulated tier keeps no directory.
- **Churn.** Hundreds of create+join and create+detach
  rounds run in bounded memory: the guest reads the
  allocator's heap-top header word after a warmup and again
  at the end, and the growth must stay within a small slack
  (two rounds' regions), proving stacks, TLS areas, key
  arrays and records recycle instead of advancing the
  frontier. Both tiers.
- **Entry return.** `runMainModule` called with an entry
  name returns a promise that resolves with the entry's
  return value (42), not an exit code — the regression for
  the CI run in which the browser consumer compared
  `TinycState.run`'s promise synchronously. Both tiers.
- **Regression.** Every existing suite passes unchanged;
  non-threaded processes behave bit-for-bit as in 0.4.0,
  including the synchronous host surface and the local-only
  guest `dlopen`; the dylink suite runs inside threaded
  processes as well, unmodified in its expectations.

## 12. Non-goals

- **`pthread_cancel`, signals, fork.** There is no signal
  model in this package and no safe point at which a
  worker's guest code could be unwound from outside;
  cancellation is absent from the surface rather than
  simulated.
- **Deadlock detection in the real tier.** Threads there
  are genuinely blocked in the OS sense; a deadlock is
  indistinguishable from a long wait, and no detector is
  promised. Detection is a simulated-tier property (§4.6).
- **Preemption in the simulated tier.** Scheduling is
  cooperative; §4.4 names the boundary (a guest loop over
  raw atomics) instead of pretending otherwise.
- **A shared table object across real-tier workers.**
  Replicated layout instead, for the reason §5.2 gives:
  a slot must hold the executing context's own closures.
- **Worker pooling.** A real-tier worker serves one thread
  and terminates; reuse is a performance refinement that
  changes no rule in this document and is left out.
- **Scheduling policies and priorities.** The rotation is
  FIFO; the real tier leaves scheduling to the OS. No
  `SCHED_*` policy is honored or promised.
- **Read-write locks and barriers.** Not in this release's
  surface; mutexes and condition variables are the
  synchronization core, and the object-layout discipline
  admits the others later without disturbing it.
- **`_Atomic` floating point.** The instruction set has no
  floating-point atomics (§8.2); nothing is simulated in
  their place.

## 13. Open questions

The body follows the recommendation in each case.

1. **Threading by default, and where.** Threaded processes
   everywhere (with `runMainModule` promise-returning in
   all of them), or threaded only where asked?
   *Recommendation: the split in §2.1/§2.3* — the run paths
   always create threaded processes, since running ordinary
   C programs is the feature's purpose and a pthread program
   should simply work there; standalone `createProcess`
   defaults to `threads: false`, keeping the 0.4.0 embedding
   contract — synchronous, creating-context execution —
   bit-for-bit for the API that has one. A uniform default
   in either direction breaks one of those two on purpose
   rather than by accident.
2. **The stack pointer mechanism.** Per-(thread, module)
   regions with a swap export (§3.1), or an ABI revision
   making `$__stack_pointer` an imported global shared by
   all modules of a thread — one native-style stack per
   thread, at the price of changing the dylink module form
   for every process, threaded or not?
   *Recommendation: the swap export* — it keeps the 0.4.0
   module form and the per-module stack model the dylink
   design settled, confines the new requirement to threaded
   processes (§3.3), and its costs (region multiplicity,
   O(modules) switches) are bounded and stated. The
   imported-global form is the prettier semantics and the
   wrong trade against a one-release-old ABI.
3. **JSPI absence.** When the simulated tier has no JSPI,
   fail `pthread_create` by name (§4.2), or attempt an
   Asyncify-style binary-rewriting fallback for direct
   modules? *Recommendation: the named failure* — the
   package has no binary-rewriting machinery, and a
   half-faithful fallback would fail in ways a named gap
   does not. The gap is also narrow: it touches only direct
   modules, only in the simulated tier, only on engines
   without a shipped JSPI.
4. **Attribute scope.** Detach state is honored; stack size
   and the remaining attributes round-trip without effect
   (§6.2). *Recommendation: keep this line for this
   release* — stack sizing is the ABI's per-module span,
   not a per-thread quantity, and inventing a mapping for
   it would promise control the layout cannot deliver.
   Revisit when a real program is shown to need it.
5. **Real-tier `dlopen` completion.** Eager — the load
   completes only when every live worker holds its replica
   (§5.4) — or lazy, with per-worker trap stubs that
   instantiate a pending replica at a thread's first call
   into the module? *Recommendation: eager, as written* —
   when `dlopen` returns, every thread can call, and no
   call path carries a hidden load. The eager protocol's
   stall (a thread that neither blocks nor calls a shim)
   is bounded by the §5.3 service points in every program
   that uses the libc surface at all; the lazy variant is
   the known optimization if a real workload hits the
   stall.
6. **`__thread` in this release, or deferred?** The
   `__tls_base` mechanism (§7.2) rides the same
   per-(thread, module) region machinery as the stacks.
   *Recommendation: include it* — deferring leaves the
   current silent sharing in place for exactly the ordinary
   C code this release serves, and a silently shared
   `__thread` variable is worse than either alternative.
   The marginal cost over §3.1's machinery is one imported
   global kind and the metadata fields.
7. **A cap on threads.** An explicit per-process limit
   (with `pthread_create` returning `EAGAIN` past it), or
   none? *Recommendation: no artificial cap* — the
   simulated tier is bounded by memory, the real tier by
   the browser's worker limits, and both already surface
   as `EAGAIN` at the point of failure (§6.2). A number
   chosen here would be a policy wearing a mechanism's
   clothes.
