"""JS interop bootstrap for tinyc_web.

Loads the bundled tinyc-web JS build and the bundled per-target
asset envelopes into the JavaScript realm this Pyodide runtime
lives in, and drives tinyc-web's loadTinyc() with them:

- the JS build (assets/tinyc-web.js) is imported from a blob URL;
  the classic-script bundle assigns the TinycWeb global inside its
  IIFE, and a one-line export trailer appended to the blob turns
  that global into the module's default export;
- each target's sysroot envelope payload is registered in the
  package's own asset registry (globalThis.__TINYC_ASSET_REGISTRY__),
  the same registry the script transport fills, so loadSysroot
  finds it there and no fetch happens;
- the compiler envelope is handed to loadTinyc through its
  loadModule hook, read from this package's files.

Initialization tier, decided once at import:

- "async" (page main thread): importing this package returns at
  once; the public API's methods are coroutines and the first call
  awaits initialization.
- "sync" (worker): the public API is synchronous; initialization
  promises are waited out with pyodide.ffi.run_sync, Pyodide's
  stack-switching synchronous wait. run_sync only works when the
  host entered Python through a promising call (a Python callable
  invoked with callPromising, per Pyodide's contract for run_sync);
  the first wait checks can_run_sync() and raises a TinyCError
  naming that requirement otherwise. A host can also force the
  tier by setting the JS global __TINYC_WEB_TIER__ to "sync" or
  "async" before importing this package.
"""
import asyncio
import json
import os
import sys

try:
    import js
    from pyodide.ffi import JsPromise, create_proxy, run_sync, can_run_sync, to_js
except ImportError as exc:  # plain CPython: this package cannot run there
    raise ImportError(
        "tinyc_web requires a Pyodide runtime (it drives the "
        "tinyc-web JavaScript build through Pyodide's JS interop)"
    ) from exc


class TinyCError(Exception):
    """A compile or link step failed; message carries TCC's diagnostics."""


_HERE = os.path.dirname(os.path.abspath(__file__))
_ASSETS = os.path.join(_HERE, "assets")


def _read_asset_text(name):
    with open(os.path.join(_ASSETS, name), "r", encoding="utf-8") as f:
        return f.read()


def _bundled_targets():
    with open(os.path.join(_ASSETS, "targets.json"), encoding="utf-8") as f:
        return json.load(f)["targets"]


BUNDLED = _bundled_targets()
_BY_TARGET = {t["target"]: t for t in BUNDLED}

DEFAULT_TARGET = "wasm32-wasi"


def target_triple():
    """Target the default instance compiles for: $TINYC_TARGET_TRIPLE
    when set, else the bundled default target (wasm32-wasi)."""
    env = os.environ.get("TINYC_TARGET_TRIPLE")
    if env:
        return env
    if DEFAULT_TARGET in _BY_TARGET:
        return DEFAULT_TARGET
    return BUNDLED[0]["target"] if BUNDLED else DEFAULT_TARGET


def _detect_tier():
    try:
        override = js.__TINYC_WEB_TIER__
    except Exception:
        override = None
    if override in ("sync", "async"):
        return str(override)
    try:
        if isinstance(js.self, js.WorkerGlobalScope):
            return "sync"
    except Exception:
        pass
    return "async"


TIER = _detect_tier()

# ---------------------------------------------------------------------
# The init sequence, on the JS side: one async function that takes
# the bundle text, the sysroot registry payloads and the loadModule
# hook, and resolves to { api, instance }.
# ---------------------------------------------------------------------

# ---------------------------------------------------------------------
# The instance IO installed at loadTinyc time. The compiler's own
# stdout/stderr (callMain output, -E output) is bound to the io
# object loadTinyc receives — a later setIO on the instance does
# not reroute it — so the instance gets forwarding functions that
# dispatch to the currently active sink: the IO a run_cli call
# installed, or Python's own stdout/stderr when none is active.
# ---------------------------------------------------------------------

_cli_io_stack = []


def push_cli_io(io):
    _cli_io_stack.append(io)


def pop_cli_io():
    _cli_io_stack.pop()


def _active_sink():
    return _cli_io_stack[-1] if _cli_io_stack else None


def _fwd_out(text):
    sink = _active_sink()
    if sink is not None:
        sink._write_out_text(str(text))
    else:
        sys.stdout.write(str(text))
        sys.stdout.flush()


def _fwd_err(text):
    sink = _active_sink()
    if sink is not None:
        sink._write_err_text(str(text))
    else:
        sys.stderr.write(str(text))
        sys.stderr.flush()


def _fwd_in():
    sink = _active_sink()
    if sink is not None:
        return sink._read_in_text()
    return None


_instance_io = None


def _instance_io_object():
    global _instance_io
    if _instance_io is None:
        from pyodide.ffi import to_js as _to_js
        _instance_io = _to_js(
            {"out": create_proxy(_fwd_out),
             "err": create_proxy(_fwd_err),
             "in": create_proxy(_fwd_in)},
            dict_converter=js.Object.fromEntries)
    return _instance_io


_JS_INIT = js.eval(
    """(async (bundleText, sysrootPayloads, loadModule, target, io) => {
  const registry = (globalThis.__TINYC_ASSET_REGISTRY__ ??= Object.create(null));
  for (const [key, payload] of Object.entries(sysrootPayloads)) {
    registry[key] = payload;
  }
  const source = bundleText + "\\nexport default TinycWeb;\\n";
  const blobUrl = URL.createObjectURL(new Blob(
    [source], { type: "text/javascript" }));
  let mod;
  try {
    mod = await import(blobUrl);
  } catch {
    /* Some module loaders (Node's) refuse blob: URLs; a data: URL
     * carries the same source there — the same fallback tinyc-web's
     * own module building uses. */
    const bytes = new TextEncoder().encode(source);
    let binary = "";
    for (const byte of bytes) binary += String.fromCharCode(byte);
    mod = await import("data:text/javascript;base64," + btoa(binary));
  } finally {
    URL.revokeObjectURL(blobUrl);
  }
  const api = mod.default;
  if (!api || typeof api.loadTinyc !== "function") {
    throw new Error("tinyc_web: the bundled tinyc-web build does not export loadTinyc");
  }
  const instance = await api.loadTinyc({
    target, loadModule, transport: "fetch", io,
    sysroot: { baseUrl: "https://tinyc-web.invalid/sysroot/" },
    /* Only used to form the URLs the loadModule hook and the
     * sysroot registry lookup key off; both are served from this
     * package, so nothing is ever fetched from it. A real absolute
     * URL is needed because the bundle's own module URL (a blob:
     * or data: URL after this import) cannot resolve relatives. */
    baseUrl: "https://tinyc-web.invalid/",
  });
  return { api, instance };
})"""
)


def _load_module(target, _files):
    """The loadModule hook: return the target's compiler envelope
    content from this package's files."""
    entry = _BY_TARGET.get(str(target))
    if entry is None:
        raise TinyCError(
            f"tinyc_web: no bundled compiler module for target "
            f"{target!r}; bundled targets: "
            + ", ".join(sorted(_BY_TARGET)))
    return _read_asset_text(entry["compiler"])


_load_module_proxy = create_proxy(_load_module)

_init_promise = None
_api = None
_instance = None
_init_error = None


def _parse_envelope(text):
    """The envelope payload: the JSON between the first { and the
    last } of the envelope file (the same rule tinyc-web's own
    parseAssetEnvelopeText applies)."""
    start = text.index("{")
    end = text.rindex("}")
    if start < 0 or end <= start:
        raise TinyCError("tinyc_web: a bundled asset envelope is not "
                         "in the envelope form")
    return json.loads(text[start:end + 1])


def _start_init():
    global _init_promise
    if _init_promise is not None:
        return _init_promise
    sysroots = {}
    for entry in BUNDLED:
        payload = _parse_envelope(_read_asset_text(entry["sysroot"]))
        sysroots[f"sysroot-{entry['target']}"] = payload
    _init_promise = _JS_INIT(
        _read_asset_text("tinyc-web.js"),
        to_js(sysroots, dict_converter=js.Object.fromEntries),
        _load_module_proxy,
        target_triple(),
        _instance_io_object(),
    )
    return _init_promise


async def _await_promise(promise):
    return await promise


def _adopt(result):
    global _api, _instance
    _api = result.api
    _instance = result.instance


def _wrap_init_error(exc):
    message = getattr(exc, "message", None) or str(exc)
    return TinyCError(f"tinyc_web: initialization failed: {message}")


async def ready_async():
    """Await initialization (async tier). Raises TinyCError when the
    init sequence failed."""
    global _init_error
    if _instance is not None:
        return _instance
    if _init_error is not None:
        raise _init_error
    try:
        result = await _start_init()
    except Exception as exc:
        _init_error = _wrap_init_error(exc)
        raise _init_error from exc
    _adopt(result)
    return _instance


def ready_sync():
    """Wait out initialization (sync tier) and return the instance."""
    global _init_error
    if _instance is not None:
        return _instance
    if _init_error is not None:
        raise _init_error
    if not can_run_sync():
        raise TinyCError(
            "tinyc_web: the sync tier cannot wait for initialization "
            "here: pyodide.ffi.run_sync needs a promising entrypoint "
            "(the host must call into Python with callPromising) and "
            "an engine with JSPI stack switching")
    try:
        result = run_sync(_await_promise(_start_init()))
    except Exception as exc:
        _init_error = _wrap_init_error(exc)
        raise _init_error from exc
    _adopt(result)
    return _instance


def _awaitable(value):
    """A JS call that crosses back into Python as a pending result:
    a JS promise (JsPromise), or the PyodideFuture Pyodide's event
    loop integration substitutes for one when the call was made
    from Python (an asyncio.Future)."""
    return isinstance(value, JsPromise) or \
        isinstance(value, asyncio.Future)


def wait_value(value):
    """Sync tier: a JS call that returned a promise (a dylink run)
    is waited out; any other value passes through."""
    if _awaitable(value):
        if not can_run_sync():
            raise TinyCError(
                "tinyc_web: the sync tier cannot wait for this run "
                "to finish: pyodide.ffi.run_sync needs a promising "
                "entrypoint (callPromising) and JSPI stack switching")
        return run_sync(_await_promise(value))
    return value


async def wait_value_async(value):
    """Async tier counterpart of wait_value."""
    if _awaitable(value):
        return await value
    return value


def instance():
    if _instance is None:
        raise TinyCError("tinyc_web: not initialized yet")
    return _instance


def api():
    if _api is None:
        raise TinyCError("tinyc_web: not initialized yet")
    return _api


def js_error_message(exc):
    return getattr(exc, "message", None) or str(exc)
