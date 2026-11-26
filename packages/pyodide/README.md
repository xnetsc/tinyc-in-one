# tinyc for Pyodide

**English** | [简体中文](README.zh-CN.md)

TinyC (TCC) as an embedded compiler for Pyodide. The package
bundles the tinyc-web JavaScript build, one compiler envelope per
target and each target's sysroot, and loads all of it by itself:
`import tinyc_web` is the whole setup — no network access, no
host-page preload, no JavaScript glue in user code.

The API mirrors the native `tinyc` package (`State`, `run_cli`,
`run_cli_capture`, `IO`, `TinyCError`, the `TCC_OUTPUT_*`
constants). The compiler underneath is the JavaScript
`TinycState` from tinyc-web, held through Pyodide proxies.

## Install

Build a wheel from this directory and install it into the
Pyodide environment (for example with `micropip`):

```
python -m pip wheel . -w dist
```

## Quick start

```python
import tinyc_web

with tinyc_web.State() as s:
    s.compile_string("int add(int a, int b) { return a + b; }")
    add = s.function("add")
    assert add(19, 23) == 42

code, out = tinyc_web.run_cli_capture(["--version"])
```

On the page main thread the package runs in its asynchronous
tier: the same calls return awaitables, so `await` them
(`await s.compile_string(...)`, `add = await s.function("add")`,
`await add(19, 23)`). In a worker it runs in its synchronous
tier and the calls above work as written. The tier is chosen
once, at import.

The synchronous tier waits out initialization with Pyodide's
`run_sync`, so the Python entry point that first touches the
package must be a promising one — in JavaScript terms, call it
through `callPromising` (or run a JSPI-enabled Pyodide build).
The asynchronous tier has no such requirement.

## Targets

The default target is `wasm32-wasi`: code compiles to WebAssembly
and runs in place. Other bundled targets cross-compile (for
example to `x86_64-linux-gnu` ELF objects). `$TINYC_TARGET_TRIPLE`
selects the target before import; `tinyc_web.bundled_targets()`
lists what the package carries and `tinyc_web.target_triple()`
reports the active one.

## Files and IO

The compiler works in tinyc-web's virtual file system. Files
named in `compile_file`, as `run_cli` inputs or as the `-o`
product cross between the Python filesystem and the virtual one
automatically: inputs are uploaded when the virtual copy is
missing, products are written back after the call.

`IO` is the same surface as in the native package — `stdio`,
`null`, `buffer`, `file`, `callback` — carrying bytes at the
Python surface. A state's IO receives that state's diagnostics
and the output of programs it runs; `run_cli`'s IO additionally
receives the compiler's own output (`--version`, `-E`, …).
Differences from the native package:

- `IO.fd(...)` has no counterpart here and raises `TinyCError`.
- `State.function(...)` carries numeric arguments and returns;
  richer signatures go through `State.run` or `run_cli`.
- There are no host binaries, so `host_triple`, `native_path`,
  `dist_dir` and `resolve_native` do not exist here;
  `assets_dir()` points at the bundled files instead.

## Package layout

```
tinyc_web/
├─ __init__.py       the API surface
├─ _bootstrap.py     tier detection and the self-loading init
└─ assets/           the tinyc-web JS build, the compiler
                     envelopes and the sysroots, per target
bundle_assets.py     rebuilds assets/ from ../browser
tests/               the shared case list and its two runners
```

`bundle_assets.py` regenerates `assets/` from the sibling
tinyc-web package's current build (its JavaScript sources are
bundled fresh; the compiler and sysroot envelopes are taken from
its asset files). Targets whose compiler module is not built in
`../browser` are skipped.

## Tests

`tests/cases.py` is one case list run by both tiers:

```
cd tests && npm install        # provides pyodide
node run_node.mjs               # synchronous tier, Node worker
node run_page.mjs               # asynchronous tier, Chromium page
```
