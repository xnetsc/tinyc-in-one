"""tinyc - TinyC (TCC) embedded compiler for Python.

Two entry styles over one dual-form native file (executable + library):

- run_cli([...]) drives the exported tinyc_main with command-line
  arguments, exactly like running the tcc command.
- State wraps the libtcc tcc_* API for step-by-step control.

Output of the compiler itself goes through the TCCIO interface.
With no IO installed it writes directly to the process's real
stdout/stderr, exactly like the command line.
"""
import ctypes
import json
import os
import platform
import sys

__all__ = ["run_cli", "run_cli_capture", "State", "IO", "TinyCError",
           "native_path", "dist_dir", "host_triple", "target_triple",
           "resolve_native"]

# libtcc output types (libtcc.h)
TCC_OUTPUT_MEMORY = 1
TCC_OUTPUT_EXE = 2
TCC_OUTPUT_OBJ = 3
TCC_OUTPUT_DLL = 4
TCC_OUTPUT_PREPROCESS = 5


class TinyCError(Exception):
    """A compile or link step failed; message carries TCC's diagnostics."""


_MACHINE_32BIT = ("i386", "i486", "i586", "i686", "x86", "ia32",
                  "armv6l", "armv7l", "arm")


def _check_64bit_triple(triple, what="host"):
    arch = triple.split("-", 1)[0].lower()
    if arch in _MACHINE_32BIT or arch.startswith("armv"):
        raise TinyCError(
            f"tinyc: 32-bit {what}s are not supported ({what} triple: "
            f"{triple}); tinyc ships 64-bit binaries only")
    return triple


def host_triple():
    """Triple of the host this process runs on (OS x 64-bit CPU).

    Hosts are 64-bit only: a 32-bit host raises instead of falling
    back to some other platform's files. Override the detection with
    $TINYC_HOST_TRIPLE ($TINYC_TRIPLE accepted as an alias)."""
    env = os.environ.get("TINYC_HOST_TRIPLE") or os.environ.get("TINYC_TRIPLE")
    if env:
        return _check_64bit_triple(env)
    machine = platform.machine().lower()
    if machine in _MACHINE_32BIT:
        raise TinyCError(
            f"tinyc: 32-bit hosts are not supported (machine: {machine})")
    if sys.platform.startswith("linux"):
        if machine in ("x86_64", "amd64"):
            return "x86_64-linux-gnu"
        if machine in ("aarch64", "arm64"):
            return "aarch64-linux-gnu"
        if machine == "riscv64":
            return "riscv64-linux-gnu"
    elif sys.platform == "win32":
        if machine in ("amd64", "x86_64"):
            return "x86_64-w64-mingw32"
    elif sys.platform == "darwin":
        if machine == "arm64":
            return "arm64-apple-darwin"
        if machine in ("x86_64", "amd64"):
            return "x86_64-apple-darwin"
    raise TinyCError(
        f"tinyc: unsupported host platform {sys.platform}/{machine}")


def target_triple():
    """Triple the compiler generates code for. One TCC binary has one
    target; cross targets are parallel variants in the same package.
    Defaults to the host triple; override with $TINYC_TARGET_TRIPLE."""
    env = os.environ.get("TINYC_TARGET_TRIPLE")
    if env:
        return _check_64bit_triple(env, "target")
    return host_triple()


def _package_root():
    """Directory holding packaging-manifest.json for this package.
    Located with importlib.resources; $TINYC_DIST overrides it."""
    env = os.environ.get("TINYC_DIST")
    if env:
        return env
    try:
        from importlib.resources import files
        return str(files("tinyc"))
    except Exception:
        return os.path.dirname(os.path.abspath(__file__))


_resolved = None


def resolve_native():
    """Resolve the (host, target) combination for this process.

    Distribution model: package type x host x target. The host selects
    the dual-form binary under natives/<host>/toolchain/<target>/;
    the target's sysroot (sysroot/<target>/, one copy per target,
    shared across hosts) provides libtcc1.a and the TCC headers.
    Raises TinyCError naming the host triple and the combinations the
    package does contain when the combination is absent."""
    global _resolved
    if _resolved is not None:
        return _resolved
    host, target = host_triple(), target_triple()
    root = _package_root()
    mf_path = os.path.join(root, "packaging-manifest.json")
    if not os.path.exists(mf_path):
        raise TinyCError(
            "tinyc: packaging-manifest.json not found in this package; "
            "the native distribution is missing (run bundle_native.py)")
    with open(mf_path) as f:
        manifest = json.load(f)
    combos = manifest.get("combinations", [])
    for c in combos:
        if c.get("host") == host and c.get("target") == target:
            _resolved = {
                "host": host,
                "target": target,
                "root": root,
                "binary_path": os.path.join(root, c["binary"]),
                "sysroot_dir": os.path.join(root, c["sysroot"]),
                # The runtime directory ({B} base for tcc_set_lib_path)
                # comes from the manifest, staged from the target's
                # dist manifest: lib/tcc on ELF/Mach-O targets, one
                # level up (lib/tcc/win32) on PE targets, whose search
                # paths are {B}/lib and {B}/include.
                "lib_path": (os.path.join(root, c["libPath"])
                             if c.get("libPath") else
                             os.path.join(root, c["sysroot"], "lib", "tcc")),
                # Output formats come from the manifest (copied
                # verbatim from the target's dist manifest); never
                # hardcoded here.
                "outputs": c.get("outputs"),
            }
            return _resolved
    available = ", ".join(f"{c.get('host')} x {c.get('target')}"
                          for c in combos) or "none"
    raise TinyCError(
        f"tinyc: no native build for host {host} / target {target} in "
        f"this package; available combinations: {available}")


def dist_dir():
    """Package root that provided the resolved native build (the
    directory holding packaging-manifest.json)."""
    return resolve_native()["root"]


def native_path():
    env = os.environ.get("TINYC_NATIVE")
    if env:
        return env
    return resolve_native()["binary_path"]


def lib_path():
    """TCC runtime directory of the resolved target sysroot: the
    directory holding TCC's support files and headers in the shape
    libtcc's search paths expect (libtcc1.a beside include/ on
    ELF/Mach-O targets, under lib/ and include/ on PE targets)."""
    return resolve_native()["lib_path"]


_lib = None


def _load():
    global _lib
    if _lib is not None:
        return _lib
    lib = ctypes.CDLL(native_path())
    # On Windows the dual-form file is an EXE loaded as a library:
    # the loader never runs its process entry, so the image's import
    # table is bound by tinyc_init, which must run before any other
    # symbol is prototyped or called. It is called bare on purpose:
    # ctypes' defaults (int restype, no argtypes) already match its
    # int () signature. Other hosts' images are fully initialized by
    # the loader and export no tinyc_init, so nothing changes there.
    if sys.platform == "win32" and hasattr(lib, "tinyc_init"):
        rc = lib.tinyc_init()
        if rc != 0:
            raise TinyCError(
                f"tinyc: tinyc_init failed with code {rc} in "
                f"{native_path()}")
    lib.tcc_new.restype = ctypes.c_void_p
    lib.tcc_compile_string.restype = ctypes.c_int
    lib.tcc_compile_string.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    # Cross builds (host != target) export no tcc_relocate/tcc_run:
    # relocating target code into this process is native-only.
    if hasattr(lib, "tcc_relocate"):
        lib.tcc_relocate.restype = ctypes.c_int
        lib.tcc_relocate.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    lib.tcc_get_symbol.restype = ctypes.c_void_p
    lib.tcc_get_symbol.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    lib.tcc_add_include_path.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    lib.tcc_add_sysinclude_path.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    lib.tcc_add_library_path.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    lib.tcc_add_library.restype = ctypes.c_int
    lib.tcc_add_library.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    lib.tcc_add_file.restype = ctypes.c_int
    lib.tcc_add_file.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    lib.tcc_define_symbol.argtypes = [ctypes.c_void_p, ctypes.c_char_p,
                                      ctypes.c_char_p]
    lib.tcc_undefine_symbol.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    lib.tcc_set_lib_path.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    lib.tcc_set_output_type.argtypes = [ctypes.c_void_p, ctypes.c_int]
    lib.tcc_output_file.restype = ctypes.c_int
    lib.tcc_output_file.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    if hasattr(lib, "tcc_run"):
        lib.tcc_run.restype = ctypes.c_int
        lib.tcc_run.argtypes = [ctypes.c_void_p, ctypes.c_int,
                                ctypes.POINTER(ctypes.c_char_p)]
    lib.tcc_set_error_func.argtypes = [ctypes.c_void_p, ctypes.c_void_p,
                                       ctypes.c_void_p]
    lib.tcc_delete.argtypes = [ctypes.c_void_p]
    lib.tinyc_main.restype = ctypes.c_int
    lib.tinyc_main.argtypes = [ctypes.c_int,
                               ctypes.POINTER(ctypes.c_char_p)]
    lib.tinyc_main_io.restype = ctypes.c_int
    lib.tinyc_main_io.argtypes = [ctypes.c_int,
                                  ctypes.POINTER(ctypes.c_char_p),
                                  ctypes.c_void_p]
    lib.tcc_io_buffer.restype = ctypes.c_void_p
    lib.tcc_io_buffer_merged.restype = ctypes.c_void_p
    lib.tcc_io_null.restype = ctypes.c_void_p
    lib.tcc_io_stdio.restype = ctypes.c_void_p
    lib.tcc_io_file.restype = ctypes.c_void_p
    lib.tcc_io_file.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.tcc_io_fd.restype = ctypes.c_void_p
    lib.tcc_io_fd.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
    lib.tcc_io_callback.restype = ctypes.c_void_p
    lib.tcc_io_callback.argtypes = [ctypes.c_void_p, ctypes.c_void_p,
                                    ctypes.c_void_p, ctypes.c_void_p]
    lib.tcc_io_buffer_data.restype = ctypes.c_void_p
    lib.tcc_io_buffer_data.argtypes = [ctypes.c_void_p, ctypes.c_int,
                                       ctypes.POINTER(ctypes.c_size_t)]
    lib.tcc_io_buffer_clear.argtypes = [ctypes.c_void_p]
    lib.tcc_io_free.argtypes = [ctypes.c_void_p]
    _lib = lib
    return lib


def _argv(args):
    arr = (ctypes.c_char_p * (len(args) + 1))()
    arr[:] = [a.encode() if isinstance(a, str) else a for a in args] + [None]
    return arr


# --------------------------------------------------------------------- IO

# The buffer argument is c_void_p, not c_char_p: a c_char_p argument
# would hand the callback a NUL-truncated bytes copy of the buffer,
# and writes through it would never reach the C buffer.
_IO_WRITE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p,
                             ctypes.c_void_p, ctypes.c_size_t)
_IO_READ = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p,
                            ctypes.c_void_p, ctypes.c_size_t)


class IO:
    """A TCCIO object. Pass to run_cli(io=...) or State.set_io_via_cli.

    Build one with the class methods, or pass any of these directly
    where an io argument is accepted:
      - None            -> real stdio (the default)
      - "capture"       -> an IO.buffer(), read .out/.err afterwards
      - a callable      -> called with bytes for both out and err
      - a binary file object (has .write(bytes))
    """

    def __init__(self, ptr, owned=True):
        self._ptr = ptr
        self._owned = owned
        self._keepalive = []

    @property
    def ptr(self):
        return self._ptr

    def close(self):
        if self._ptr and self._owned:
            _load().tcc_io_free(ctypes.c_void_p(self._ptr))
        self._ptr = None
        self._keepalive = []

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    # -- ready-made implementations ------------------------------------
    @staticmethod
    def stdio():
        return IO(_load().tcc_io_stdio(), owned=False)

    @staticmethod
    def null():
        return IO(_load().tcc_io_null(), owned=False)

    @staticmethod
    def buffer(merged=False):
        lib = _load()
        ptr = lib.tcc_io_buffer_merged() if merged else lib.tcc_io_buffer()
        return IO(ptr)

    @staticmethod
    def file(out_path, err_path=None):
        ptr = _load().tcc_io_file(
            out_path.encode() if out_path else None,
            err_path.encode() if err_path else None)
        if not ptr:
            raise TinyCError(f"cannot open IO file: {out_path}")
        return IO(ptr)

    @staticmethod
    def fd(out_fd=-1, err_fd=-1, in_fd=-1):
        return IO(_load().tcc_io_fd(out_fd, err_fd, in_fd))

    @staticmethod
    def callback(out=None, err=None, read_in=None):
        """out/err: callable(bytes) -> int|None; read_in: callable(cap) -> bytes."""
        lib = _load()
        keep = []

        def wrap_write(fn):
            if fn is None:
                return None
            def _w(ctx, buf, n):
                try:
                    data = ctypes.string_at(buf, n)
                    r = fn(data)
                    return n if r is None else int(r)
                except Exception:
                    return -1
            cb = _IO_WRITE(_w)
            keep.append(cb)
            return cb

        def wrap_read(fn):
            if fn is None:
                return None
            def _r(ctx, buf, cap):
                try:
                    data = fn(cap)
                    if not data:
                        return 0
                    ctypes.memmove(buf, data, min(len(data), cap))
                    return min(len(data), cap)
                except Exception:
                    return -1
            cb = _IO_READ(_r)
            keep.append(cb)
            return cb

        ptr = lib.tcc_io_callback(wrap_write(out), wrap_write(err),
                                  wrap_read(read_in), None)
        io = IO(ptr)
        io._keepalive = keep
        return io

    # -- buffer access ---------------------------------------------------
    def _buffer_bytes(self, channel):
        lib = _load()
        n = ctypes.c_size_t()
        p = lib.tcc_io_buffer_data(ctypes.c_void_p(self._ptr), channel,
                                   ctypes.byref(n))
        if not p:
            return b""
        return ctypes.string_at(p, n.value)

    @property
    def out(self):
        return self._buffer_bytes(0)

    @property
    def err(self):
        return self._buffer_bytes(1)

    def clear(self):
        _load().tcc_io_buffer_clear(ctypes.c_void_p(self._ptr))


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


# --------------------------------------------------------------------- CLI

def run_cli(args, io=None):
    """Run the compiler with command-line arguments, like the tcc
    command. args excludes argv[0] (the native path is filled in, so
    the compiler finds its bundled headers/libraries). Returns the
    exit code; the host process is never exited."""
    lib = _load()
    args = list(args)
    # The binary does not sit next to its runtime in the unified
    # layout, so point TCC at the target sysroot with -B unless the
    # caller already passed one.
    if not any(a == "-B" or a.startswith("-B") for a in args):
        args = ["-B" + lib_path()] + args
    full = [native_path()] + args
    argv = _argv(full)
    io = _coerce_io(io)
    if io is None:
        return lib.tinyc_main(len(full), argv)
    return lib.tinyc_main_io(len(full), argv, ctypes.c_void_p(io.ptr))


def run_cli_capture(args):
    """run_cli with a merged-order capture buffer.
    Returns (exit_code, output_bytes)."""
    with IO.buffer(merged=True) as io:
        code = run_cli(args, io=io)
        return code, io.out


# --------------------------------------------------------------------- API

_ERROR_FUNC = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_char_p)


class State:
    """A libtcc TCCState with step-by-step control.

    Errors are collected into .errors (list of str) instead of being
    printed. The bundled distribution's lib path is set automatically
    and the output type defaults to TCC_OUTPUT_MEMORY.
    """

    def __init__(self, output_type=TCC_OUTPUT_MEMORY):
        lib = _load()
        self._lib = lib
        self._s = lib.tcc_new()
        if not self._s:
            raise TinyCError("tcc_new failed")
        self.errors = []
        self._err_cb = _ERROR_FUNC(
            lambda opaque, msg: self.errors.append(msg.decode("utf-8", "replace")))
        lib.tcc_set_error_func(self._s, None, self._err_cb)
        lib.tcc_set_lib_path(self._s, lib_path().encode())
        if output_type is not None:
            lib.tcc_set_output_type(self._s, output_type)
        self._relocated = False

    def close(self):
        if self._s:
            self._lib.tcc_delete(ctypes.c_void_p(self._s))
            self._s = None

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def _check(self, ret, what):
        if ret != 0:
            detail = "; ".join(self.errors) or "no diagnostics"
            raise TinyCError(f"{what} failed: {detail}")
        return ret

    # dependency management: local real filesystem paths, used as-is
    def add_include_path(self, path):
        self._lib.tcc_add_include_path(self._s, str(path).encode())

    def add_sysinclude_path(self, path):
        self._lib.tcc_add_sysinclude_path(self._s, str(path).encode())

    def add_library_path(self, path):
        self._lib.tcc_add_library_path(self._s, str(path).encode())

    def add_library(self, name):
        return self._check(self._lib.tcc_add_library(
            self._s, str(name).encode()), f"add_library({name})")

    def add_file(self, path):
        return self._check(self._lib.tcc_add_file(
            self._s, str(path).encode()), f"add_file({path})")

    def define_symbol(self, name, value="1"):
        self._lib.tcc_define_symbol(self._s, str(name).encode(),
                                    str(value).encode())

    def undefine_symbol(self, name):
        self._lib.tcc_undefine_symbol(self._s, str(name).encode())

    # compilation
    def compile_string(self, source):
        if isinstance(source, str):
            source = source.encode()
        return self._check(self._lib.tcc_compile_string(self._s, source),
                           "compile_string")

    def compile_file(self, path):
        """Queue a source file for compilation (libtcc compiles queued
        files at relocate()/output_file() time)."""
        return self.add_file(path)

    def set_output_type(self, output_type):
        self._lib.tcc_set_output_type(self._s, output_type)

    def output_file(self, path):
        return self._check(self._lib.tcc_output_file(
            self._s, str(path).encode()), f"output_file({path})")

    def _require_native_only(self, symbol):
        if not hasattr(self._lib, symbol):
            r = resolve_native()
            raise TinyCError(
                f"{symbol} is not available for target {r['target']} on "
                f"host {r['host']}: in-process relocation/run requires a "
                "native (host == target) combination; cross combinations "
                "compile to files (output_file / run_cli)")

    def relocate(self):
        self._require_native_only("tcc_relocate")
        ret = self._lib.tcc_relocate(ctypes.c_void_p(self._s), None)
        self._check(ret, "relocate")
        self._relocated = True

    def get_symbol(self, name):
        if not self._relocated:
            self.relocate()
        addr = self._lib.tcc_get_symbol(self._s, str(name).encode())
        if not addr:
            raise TinyCError(f"symbol not found: {name}")
        return addr

    def function(self, name, restype, *argtypes):
        """Return a callable ctypes function for a compiled symbol."""
        addr = self.get_symbol(name)
        return ctypes.CFUNCTYPE(restype, *argtypes)(addr)

    def run(self, args=()):
        """Run the compiled program's main in this process (tcc_run).
        args excludes argv[0]. Returns the program's exit code.
        The program runs in-process: its exit() ends this process."""
        self._require_native_only("tcc_run")
        argv = _argv(["tinyc-run"] + list(args))
        return self._lib.tcc_run(ctypes.c_void_p(self._s), len(args) + 1, argv)
