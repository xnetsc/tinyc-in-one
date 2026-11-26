"""tinyc_web - TinyC (TCC) for Python running on Pyodide.

The tinyc-web JavaScript build of the compiler, driven from Python
through Pyodide's JS interop. The package is self-contained: it
bundles the JS build and each target's compiler and sysroot asset
envelopes, loads them without any network access or host-page
setup, and presents the same API surface as the native `tinyc`
package (State, run_cli, IO, TinyCError) over the JS TinycState.

Two entry styles over one compiler instance:

- run_cli([...]) drives the compiler's command-line main, exactly
  like running the tcc command. Arguments naming files in the
  Python filesystem are uploaded to the compiler's virtual FS
  first; a -o product is written back afterwards.
- State wraps the fine-grained API for step-by-step control.

Files enter the virtual FS from the Python filesystem: State
methods that name a file (add_file, compile_file) upload it from
the Python filesystem on first use, and output_file writes the
product back.

Initialization tier, decided once at import (see _bootstrap):
on a page main thread the API is asynchronous — import returns at
once and State/run_cli are coroutines whose first call awaits
initialization; in a worker the API is synchronous, with the
initialization waited out through Pyodide's run_sync.

Output of the compiler goes through an IO object, per state:
a State carries its own IO (State.set_io), and run_cli's IO is
installed on the instance for that call.
"""
import os

from pyodide.ffi import create_proxy, to_js
import js

from . import _bootstrap as _boot
from ._bootstrap import TinyCError

__all__ = ["run_cli", "run_cli_capture", "State", "IO", "TinyCError",
           "target_triple", "bundled_targets", "tier", "assets_dir",
           "TCC_OUTPUT_MEMORY", "TCC_OUTPUT_EXE", "TCC_OUTPUT_OBJ",
           "TCC_OUTPUT_DLL", "TCC_OUTPUT_PREPROCESS"]

# libtcc output types (libtcc.h), same values as the native package
TCC_OUTPUT_MEMORY = 1
TCC_OUTPUT_EXE = 2
TCC_OUTPUT_OBJ = 3
TCC_OUTPUT_DLL = 4
TCC_OUTPUT_PREPROCESS = 5


def target_triple():
    """Target the default instance compiles for ($TINYC_TARGET_TRIPLE
    when set, else the bundled default target, wasm32-wasi). Only
    the wasm32 target can run compiled output here; every other
    bundled target compiles to files."""
    return _boot.target_triple()


def bundled_targets():
    """Targets whose compiler and sysroot envelopes this package
    bundles, as a list of target triples."""
    return [t["target"] for t in _boot.BUNDLED]


def tier():
    """The initialization tier in effect: "sync" or "async"."""
    return _boot.TIER


def assets_dir():
    """Directory holding this package's bundled tinyc-web assets."""
    return _boot._ASSETS


def _js_object(d):
    return to_js(d, dict_converter=js.Object.fromEntries)


# --------------------------------------------------------------------- IO

class IO:
    """Compiler IO. Pass to run_cli(io=...) or State.set_io(...).

    Build one with the class methods, or pass any of these directly
    where an io argument is accepted:
      - None            -> the instance IO (console by default)
      - "capture"       -> an IO.buffer(), read .out/.err afterwards
      - a callable      -> called with bytes for both out and err
      - a binary file object (has .write(bytes))

    The channels are byte-oriented like the native package's; the
    JS side speaks decoded text, so bytes are UTF-8 encoded/decoded
    at the boundary (undecodable input is replaced).
    """

    def __init__(self, out_text=None, err_text=None, in_text=None,
                 use_default=False):
        self._out_text = out_text    # callable(str) | None
        self._err_text = err_text    # callable(str) | None
        self._in_text = in_text      # callable() -> str | None, or None
        self._use_default = use_default
        self._proxies = {}
        self._files = []

    # -- JS-facing channel functions ------------------------------------
    def _write_out_text(self, text):
        if self._out_text is not None:
            self._out_text(str(text))

    def _write_err_text(self, text):
        if self._err_text is not None:
            self._err_text(str(text))

    def _read_in_text(self):
        if self._in_text is None:
            return None
        return self._in_text()

    def _proxy(self, key, fn):
        proxy = self._proxies.get(key)
        if proxy is None:
            proxy = create_proxy(fn)
            self._proxies[key] = proxy
        return proxy

    def js_partial(self):
        """The { out, err, in } object handed to the JS side. An IO
        is self-contained: channels it does not implement discard
        output / report EOF. The stdio IO is the empty partial,
        which the JS side fills with its console defaults."""
        if self._use_default:
            return _js_object({})
        return _js_object({
            "out": self._proxy("out", lambda text: self._write_out_text(text)),
            "err": self._proxy("err", lambda text: self._write_err_text(text)),
            "in": self._proxy("in", lambda: self._read_in_text()),
        })

    def close(self):
        for proxy in self._proxies.values():
            proxy.destroy()
        self._proxies = {}
        for f in self._files:
            try:
                f.close()
            except Exception:
                pass
        self._files = []

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    # -- ready-made implementations ------------------------------------
    @staticmethod
    def stdio():
        return IO(use_default=True)

    @staticmethod
    def null():
        return IO(out_text=lambda text: None,
                  err_text=lambda text: None,
                  in_text=lambda: None)

    @staticmethod
    def buffer(merged=False):
        out_chunks, err_chunks, merged_chunks = [], [], []

        def out_text(text):
            out_chunks.append(text)
            merged_chunks.append(text)

        def err_text(text):
            err_chunks.append(text)
            merged_chunks.append(text)

        io = IO(out_text=out_text, err_text=err_text)
        io._chunks = (out_chunks, err_chunks, merged_chunks)
        io._merged = merged
        return io

    @staticmethod
    def file(out_path, err_path=None):
        out_f = open(out_path, "ab")
        err_f = open(err_path, "ab") if err_path else out_f

        def write_to(f):
            def _w(text):
                f.write(str(text).encode("utf-8", "replace"))
                f.flush()
            return _w

        io = IO(out_text=write_to(out_f), err_text=write_to(err_f))
        io._files = [out_f] if err_f is out_f else [out_f, err_f]
        return io

    @staticmethod
    def fd(out_fd=-1, err_fd=-1, in_fd=-1):
        raise TinyCError(
            "tinyc_web: fd IO is not available: the compiler's IO "
            "here is the JS { out, err, in } interface, which has "
            "no file descriptors; use IO.file or IO.callback")

    @staticmethod
    def callback(out=None, err=None, read_in=None):
        """out/err: callable(bytes) -> int|None; read_in: callable(cap) -> bytes."""
        def wrap_write(fn):
            if fn is None:
                return None
            def _w(text):
                fn(str(text).encode("utf-8", "replace"))
            return _w

        def wrap_read(fn):
            if fn is None:
                return None
            def _r():
                data = fn(65536)
                if not data:
                    return None
                return bytes(data).decode("utf-8", "replace")
            return _r

        return IO(out_text=wrap_write(out), err_text=wrap_write(err),
                  in_text=wrap_read(read_in))

    # -- buffer access ---------------------------------------------------
    @property
    def out(self):
        chunks = getattr(self, "_chunks", None)
        if chunks is None:
            return b""
        texts = chunks[2] if getattr(self, "_merged", False) else chunks[0]
        return "".join(texts).encode("utf-8", "replace")

    @property
    def err(self):
        chunks = getattr(self, "_chunks", None)
        if chunks is None:
            return b""
        return "".join(chunks[1]).encode("utf-8", "replace")

    def clear(self):
        chunks = getattr(self, "_chunks", None)
        if chunks is not None:
            for c in chunks:
                c.clear()


def _coerce_io(io):
    """Turn the accepted io shorthands into an IO instance (or None)."""
    if io is None or isinstance(io, IO):
        return io
    if io == "capture":
        return IO.buffer()
    if callable(io):
        return IO.callback(out=io, err=io)
    write = getattr(io, "write", None)
    if write is not None:
        return IO.callback(out=lambda b: write(b), err=lambda b: write(b))
    raise TinyCError(f"unsupported io argument: {io!r}")


# --------------------------------------------------------------------- VFS

def _upload_file(inst, path):
    """Bring a Python-filesystem file into the virtual FS (no-op
    when the virtual FS already holds the path)."""
    p = str(path)
    if inst.hasFile(p):
        return
    if not os.path.exists(p):
        raise TinyCError(
            f"tinyc_web: {p} is not in the virtual FS and does not "
            f"exist in the Python filesystem")
    with open(p, "rb") as f:
        inst.addFileSync(p, f.read())


def _fetch_output(inst, path):
    """Read a compiled product out of the instance as bytes, or
    None when the path is not there. exportFile reads the module
    FS first — that is where the compiler writes — so no hasFile
    gate: the virtual FS only mirrors a path after a read."""
    p = str(path)
    try:
        data = inst.exportFile(p)
    except Exception as exc:
        raise TinyCError(_boot.js_error_message(exc)) from exc
    if data is None:
        return None
    raw = data.to_py() if hasattr(data, "to_py") else data
    if raw is None:
        return None
    return bytes(raw)


def _download_output(inst, path):
    data = _fetch_output(inst, path)
    if data is not None:
        with open(str(path), "wb") as f:
            f.write(data)


# --------------------------------------------------------------------- State

class _StateImpl:
    """The tier-independent State body: synchronous JS calls on one
    JS TinycState. Methods that can produce a JS promise (run, and
    the callables from function) return it raw; the tier facades
    wait it out."""

    def __init__(self, output_type, io):
        self._inst = _boot.instance()
        try:
            self._js = self._inst.createState()
        except Exception as exc:
            raise TinyCError(_boot.js_error_message(exc)) from exc
        self.errors = []
        self._proxies = []
        self._io = None
        if output_type is not None:
            self._js.setOutputType(output_type)
        self._install_io(io)

    # -- IO --------------------------------------------------------------
    def _err_proxy(self):
        def _err(text):
            text = str(text)
            self.errors.append(
                text[:-1] if text.endswith("\n") else text)
            if self._io is not None:
                self._io._write_err_text(text)
        proxy = create_proxy(_err)
        self._proxies.append(proxy)
        return proxy

    def _install_io(self, io):
        io = _coerce_io(io)
        self._io = io
        partial = {"err": self._err_proxy()}
        if io is not None:
            partial["out"] = io._proxy(
                "out", lambda text: io._write_out_text(text))
            partial["in"] = io._proxy("in", lambda: io._read_in_text())
        try:
            self._js.setIO(_js_object(partial))
        except Exception as exc:
            raise TinyCError(_boot.js_error_message(exc)) from exc

    def set_io(self, io):
        """Install an IO on this state only (an IO instance,
        "capture", a callable, or a binary file object; None detaches
        back to the instance IO). Diagnostics always collect into
        .errors as well. The state keeps the IO object alive until
        set_io(None), close(), or replacement."""
        self._install_io(io)

    # -- plumbing ----------------------------------------------------------
    def _invoke(self, what, fn, *args):
        try:
            rc = fn(*args)
        except Exception as exc:
            detail = "; ".join(self.errors) or _boot.js_error_message(exc)
            raise TinyCError(f"{what} failed: {detail}") from exc
        if isinstance(rc, (int, float)) and rc != 0:
            detail = "; ".join(self.errors) or "no diagnostics"
            raise TinyCError(f"{what} failed: {detail}")
        return rc

    def _wrap(self, fn, *args):
        try:
            return fn(*args)
        except Exception as exc:
            raise TinyCError(_boot.js_error_message(exc)) from exc

    def close(self):
        if self._js is not None:
            try:
                self._js.delete()
            except Exception:
                pass
            self._js = None
        for proxy in self._proxies:
            proxy.destroy()
        self._proxies = []
        self._io = None

    # -- dependency management --------------------------------------------
    def add_include_path(self, path):
        return self._invoke("add_include_path",
                            self._js.addIncludePath, str(path))

    def add_sysinclude_path(self, path):
        return self._invoke("add_sysinclude_path",
                            self._js.addSysincludePath, str(path))

    def add_library_path(self, path):
        return self._invoke("add_library_path",
                            self._js.addLibraryPath, str(path))

    def add_library(self, name):
        return self._invoke("add_library", self._js.addLibrary, str(name))

    def add_file(self, path):
        _upload_file(self._inst, path)
        return self._invoke("add_file", self._js.addFile, str(path))

    def define_symbol(self, name, value="1"):
        self._wrap(self._js.defineSymbol, str(name), str(value))

    def undefine_symbol(self, name):
        self._wrap(self._js.undefineSymbol, str(name))

    # -- compilation ---------------------------------------------------------
    def compile_string(self, source):
        if isinstance(source, bytes):
            source = source.decode()
        return self._invoke("compile_string",
                            self._js.compileString, source)

    def compile_file(self, path):
        """Compile a file: it is uploaded from the Python filesystem
        into the virtual FS first when it is not already there."""
        _upload_file(self._inst, path)
        return self._invoke("compile_file",
                            self._js.compileFile, str(path))

    def set_output_type(self, output_type):
        return self._invoke("set_output_type",
                            self._js.setOutputType, output_type)

    def output_file(self, path):
        rc = self._invoke("output_file", self._js.outputFile, str(path))
        _download_output(self._inst, path)
        return rc

    def relocate(self):
        return self._invoke("relocate", self._js.relocate)

    def get_symbol(self, name):
        return int(self._wrap(self._js.getSymbol, str(name)))

    # -- running -------------------------------------------------------------
    def run_raw(self, args=(), entry=None, call_args=None):
        opts = {}
        if entry is not None:
            opts["entry"] = entry
        if call_args is not None:
            opts["callArgs"] = list(call_args)
        argv = to_js([str(a) for a in args])
        try:
            return self._js.run(argv, _js_object(opts))
        except Exception as exc:
            # A standalone wasm product of a C program exports main
            # but no _start; the JS run path without an entry only
            # accepts _start modules and names the missing _start.
            # State.run means "run the program's main" (the native
            # package's tcc_run semantics), so fall back to the main
            # entry — called without arguments in that form. Dylink
            # products (current compilers) run through the process
            # path above and never reach this fallback.
            if entry is None and \
                    "exports no _start" in _boot.js_error_message(exc):
                opts["entry"] = "main"
                try:
                    return self._js.run(argv, _js_object(opts))
                except Exception as exc2:
                    exc = exc2
            detail = "; ".join(self.errors) or _boot.js_error_message(exc)
            raise TinyCError(f"run failed: {detail}") from exc

    def function_raw(self, name, restype, argtypes):
        def call(*args):
            if argtypes and len(args) != len(argtypes):
                raise TypeError(
                    f"{name}() takes {len(argtypes)} arguments "
                    f"({len(args)} given)")
            return self.run_raw((), entry=name, call_args=list(args))
        return call


class _StateBase:
    def __init__(self, output_type=TCC_OUTPUT_MEMORY, io=None):
        self._output_type = output_type
        self._io_arg = io
        self._impl = None

    def _make_impl(self):
        if self._impl is None:
            self._impl = _StateImpl(self._output_type, self._io_arg)
        return self._impl

    @property
    def errors(self):
        """Diagnostics collected so far (list of str). Empty until
        the state has been created, which the async tier defers to
        the first awaited call."""
        return self._impl.errors if self._impl is not None else []

    def close(self):
        if self._impl is not None:
            self._impl.close()
            self._impl = None

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()


class _SyncState(_StateBase):
    """A libtcc state with step-by-step control (sync tier).

    Errors are collected into .errors (list of str) instead of being
    printed. The bundled sysroot is loaded and its include directory
    registered automatically; the output type defaults to
    TCC_OUTPUT_MEMORY. Construction waits out initialization."""

    def __init__(self, output_type=TCC_OUTPUT_MEMORY, io=None):
        super().__init__(output_type, io)
        _boot.ready_sync()
        self._make_impl()

    def _i(self):
        return self._make_impl()

    def set_io(self, io):
        self._i().set_io(io)

    def add_include_path(self, path):
        return self._i().add_include_path(path)

    def add_sysinclude_path(self, path):
        return self._i().add_sysinclude_path(path)

    def add_library_path(self, path):
        return self._i().add_library_path(path)

    def add_library(self, name):
        return self._i().add_library(name)

    def add_file(self, path):
        return self._i().add_file(path)

    def define_symbol(self, name, value="1"):
        return self._i().define_symbol(name, value)

    def undefine_symbol(self, name):
        return self._i().undefine_symbol(name)

    def compile_string(self, source):
        return self._i().compile_string(source)

    def compile_file(self, path):
        return self._i().compile_file(path)

    def set_output_type(self, output_type):
        return self._i().set_output_type(output_type)

    def output_file(self, path):
        return self._i().output_file(path)

    def relocate(self):
        return self._i().relocate()

    def get_symbol(self, name):
        return self._i().get_symbol(name)

    def function(self, name, restype=None, *argtypes):
        """Return a callable for a compiled symbol: it runs the
        symbol as a run entry with the given arguments and returns
        its return value. Arguments and results are numbers, the
        only values the JS run path carries."""
        raw = self._i().function_raw(name, restype, argtypes)
        def call(*args):
            return _boot.wait_value(raw(*args))
        return call

    def run(self, args=()):
        """Run the compiled program's main. args excludes argv[0].
        Returns the program's exit code. Only on the wasm32 target:
        the program runs as a wasm module in the JS host, never in
        this Python process."""
        return _boot.wait_value(self._i().run_raw(args))


class _AsyncState(_StateBase):
    """A libtcc state with step-by-step control (async tier).

    Same surface as the sync tier's State, but every method is a
    coroutine: construction returns at once and the first awaited
    call waits out initialization. Errors are collected into
    .errors (list of str) instead of being printed."""

    async def _ai(self):
        await _boot.ready_async()
        return self._make_impl()

    async def set_io(self, io):
        (await self._ai()).set_io(io)

    async def add_include_path(self, path):
        return (await self._ai()).add_include_path(path)

    async def add_sysinclude_path(self, path):
        return (await self._ai()).add_sysinclude_path(path)

    async def add_library_path(self, path):
        return (await self._ai()).add_library_path(path)

    async def add_library(self, name):
        return (await self._ai()).add_library(name)

    async def add_file(self, path):
        return (await self._ai()).add_file(path)

    async def define_symbol(self, name, value="1"):
        return (await self._ai()).define_symbol(name, value)

    async def undefine_symbol(self, name):
        return (await self._ai()).undefine_symbol(name)

    async def compile_string(self, source):
        return (await self._ai()).compile_string(source)

    async def compile_file(self, path):
        return (await self._ai()).compile_file(path)

    async def set_output_type(self, output_type):
        return (await self._ai()).set_output_type(output_type)

    async def output_file(self, path):
        return (await self._ai()).output_file(path)

    async def relocate(self):
        return (await self._ai()).relocate()

    async def get_symbol(self, name):
        return (await self._ai()).get_symbol(name)

    async def function(self, name, restype=None, *argtypes):
        """Return a callable for a compiled symbol: it runs the
        symbol as a run entry with the given arguments and returns
        its return value. The callable is itself a coroutine
        function. Arguments and results are numbers, the only
        values the JS run path carries."""
        raw = (await self._ai()).function_raw(name, restype, argtypes)
        async def call(*args):
            return await _boot.wait_value_async(raw(*args))
        return call

    async def run(self, args=()):
        """Run the compiled program's main. args excludes argv[0].
        Returns the program's exit code. Only on the wasm32 target:
        the program runs as a wasm module in the JS host, never in
        this Python process."""
        return await _boot.wait_value_async((await self._ai()).run_raw(args))


State = _SyncState if _boot.TIER == "sync" else _AsyncState


# --------------------------------------------------------------------- CLI

def _run_cli_raw(args, io):
    inst = _boot.instance()
    args = [str(a) for a in args]
    # The wasm32 target is freestanding: no crt1.o/crti.o exist for
    # it, and the JS package's own states default to -nostdlib for
    # exactly that reason. A bare "-run" can therefore never link;
    # supply the default for it (an explicit -nostdlib anywhere in
    # the arguments is left alone).
    if "-run" in args and target_triple() == "wasm32-wasi" \
            and "-nostdlib" not in args:
        idx = args.index("-run")
        args = args[:idx] + ["-nostdlib"] + args[idx:]
    for a in args:
        if not a.startswith("-") and os.path.isfile(a):
            _upload_file(inst, a)
    coerced = _coerce_io(io)
    if coerced is not None:
        _boot.push_cli_io(coerced)
    try:
        try:
            result = inst.callMain(to_js(args))
        except Exception as exc:
            # callMain's own "-run" path runs the product through
            # the _start form; a standalone product of a C program
            # exports main but no _start and the run names it. Fall
            # back to composing the run here: compile to a file,
            # then run its main entry through the instance (the
            # dylink path of current compilers never raises this).
            if "-run" not in args or \
                    "exports no _start" not in _boot.js_error_message(exc):
                raise TinyCError(_boot.js_error_message(exc)) from exc
            idx = args.index("-run")
            if idx + 1 >= len(args):
                raise TinyCError(
                    "tinyc_web: run_cli: '-run' must be followed by "
                    "a source file") from exc
            out_path = "/tinyc-run-output.wasm"
            rc = inst.callMain(to_js(
                args[:idx] + ["-o", out_path, args[idx + 1]]))
            if rc != 0:
                return rc
            result = inst.runWasmFile(
                out_path, _js_object({"entry": "main"}))
    finally:
        if coerced is not None:
            _boot.pop_cli_io()
    for i, a in enumerate(args):
        out = None
        if a == "-o" and i + 1 < len(args):
            out = args[i + 1]
        elif a.startswith("-o") and len(a) > 2:
            out = a[2:]
        if out and not out.startswith("-"):
            _download_output(inst, out)
    return result


if _boot.TIER == "sync":
    def run_cli(args, io=None):
        """Run the compiler with command-line arguments, like the
        tcc command. args excludes argv[0]. Returns the exit code;
        nothing is ever exited. A "-run" argument runs the compiled
        program in the JS host (wasm32 target only)."""
        _boot.ready_sync()
        return _boot.wait_value(_run_cli_raw(args, io))

    def run_cli_capture(args):
        """run_cli with a merged-order capture buffer.
        Returns (exit_code, output_bytes)."""
        with IO.buffer(merged=True) as io:
            code = run_cli(args, io=io)
            return code, io.out

else:
    async def run_cli(args, io=None):
        """Run the compiler with command-line arguments, like the
        tcc command. args excludes argv[0]. Returns the exit code;
        nothing is ever exited. A "-run" argument runs the compiled
        program in the JS host (wasm32 target only)."""
        await _boot.ready_async()
        return await _boot.wait_value_async(_run_cli_raw(args, io))

    async def run_cli_capture(args):
        """run_cli with a merged-order capture buffer.
        Returns (exit_code, output_bytes)."""
        with IO.buffer(merged=True) as io:
            code = await run_cli(args, io=io)
            return code, io.out
