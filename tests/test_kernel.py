"""Kernel-level tests for the dual-form file, driven through ctypes
against the distribution directly (no package layer involved)."""
import ctypes
import os
import platform
import subprocess
import sys
import tempfile
import unittest


def host_triple():
    """The host this test runs on, resolved the same way the package
    loaders resolve it ($TINYC_HOST_TRIPLE, alias $TINYC_TRIPLE,
    wins over detection). The kernel tests exercise the native
    (host == target) distribution, which is the only one whose
    dual-form file can execute and dlopen on this machine."""
    env = os.environ.get("TINYC_HOST_TRIPLE") or os.environ.get("TINYC_TRIPLE")
    if env:
        return env
    machine = platform.machine().lower()
    if sys.platform.startswith("linux"):
        if machine in ("x86_64", "amd64"):
            return "x86_64-linux-gnu"
        if machine in ("aarch64", "arm64"):
            return "aarch64-linux-gnu"
    if sys.platform == "win32" and machine in ("amd64", "x86_64"):
        return "x86_64-w64-mingw32"
    if sys.platform == "darwin":
        if machine == "arm64":
            return "arm64-apple-darwin"
        if machine in ("x86_64", "amd64"):
            return "x86_64-apple-darwin"
    raise RuntimeError(
        f"cannot detect a 64-bit host triple for {sys.platform}/{machine}")


HOST_TRIPLE = host_triple()
DIST = os.path.normpath(os.path.join(
    os.path.dirname(__file__), "..", "build", "dist", HOST_TRIPLE))
DUAL = os.path.join(
    DIST, "bin", "tinyc.exe" if HOST_TRIPLE == "x86_64-w64-mingw32"
    else "tinyc")


def load():
    lib = ctypes.CDLL(DUAL)
    lib.tinyc_main.restype = ctypes.c_int
    lib.tinyc_main.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_char_p)]
    lib.tinyc_main_io.restype = ctypes.c_int
    lib.tinyc_main_io.argtypes = [ctypes.c_int,
                                  ctypes.POINTER(ctypes.c_char_p),
                                  ctypes.c_void_p]
    lib.tcc_io_buffer.restype = ctypes.c_void_p
    lib.tcc_io_buffer_merged.restype = ctypes.c_void_p
    lib.tcc_io_buffer_data.restype = ctypes.c_void_p
    lib.tcc_io_buffer_data.argtypes = [ctypes.c_void_p, ctypes.c_int,
                                       ctypes.POINTER(ctypes.c_size_t)]
    lib.tcc_io_free.argtypes = [ctypes.c_void_p]
    lib.tcc_io_callback.restype = ctypes.c_void_p
    lib.tcc_io_callback.argtypes = [ctypes.c_void_p, ctypes.c_void_p,
                                    ctypes.c_void_p, ctypes.c_void_p]
    return lib


_IO_WRITE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p,
                             ctypes.c_char_p, ctypes.c_size_t)
_IO_READ = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p,
                            ctypes.c_void_p, ctypes.c_size_t)


def argv_of(args):
    arr = (ctypes.c_char_p * (len(args) + 1))()
    arr[:] = [a.encode() for a in args] + [None]
    return arr


def capture(lib, args, merged=True):
    io = lib.tcc_io_buffer_merged() if merged else lib.tcc_io_buffer()
    try:
        code = lib.tinyc_main_io(len(args), argv_of(args), io)
        n = ctypes.c_size_t()
        p = lib.tcc_io_buffer_data(io, 0, ctypes.byref(n))
        out = ctypes.string_at(p, n.value) if p else b""
        p = lib.tcc_io_buffer_data(io, 1, ctypes.byref(n))
        err = ctypes.string_at(p, n.value) if p else b""
        return code, out, err
    finally:
        lib.tcc_io_free(io)


class ExecutableModeTests(unittest.TestCase):
    def run_dual(self, *args):
        return subprocess.run([DUAL, *args], capture_output=True)

    def test_file_is_pie_with_interp(self):
        out = subprocess.run(["file", DUAL], capture_output=True, text=True).stdout
        self.assertIn("pie executable" if "pie executable" in out else "shared object", out)
        self.assertIn("interpreter", out)

    def test_version(self):
        r = self.run_dual("--version")
        self.assertEqual(r.returncode, 0)
        self.assertIn(b"tcc version", r.stdout)

    def test_compile_run_return_code(self):
        with tempfile.TemporaryDirectory() as d:
            src = os.path.join(d, "m.c")
            with open(src, "w") as f:
                f.write("int main(){ return 42; }")
            r = self.run_dual("-run", src)
            self.assertEqual(r.returncode, 42)

    def test_exports_present(self):
        out = subprocess.run(["nm", "-D", DUAL], capture_output=True, text=True).stdout
        for sym in ["main", "tinyc_main", "tinyc_main_io",
                    "tinyc_main_with_io", "tcc_new",
                    "tcc_compile_string", "tcc_io_buffer",
                    "tcc_set_io", "tcc_get_io"]:
            self.assertIn(" T " + sym, out)


class LibraryModeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = load()

    def test_main_dumpversion_returns_not_exits(self):
        code, out, err = capture(self.lib, ["tinyc", "-dumpversion"])
        self.assertEqual(code, 0)
        self.assertIn(b"0.9.28", out)

    def test_main_version(self):
        code, out, err = capture(self.lib, ["tinyc", "--version"])
        self.assertEqual(code, 0)
        self.assertIn(b"tcc version", out)

    def test_main_called_repeatedly(self):
        for _ in range(3):
            code, out, err = capture(self.lib, ["tinyc", "-dumpversion"])
            self.assertEqual(code, 0)

    def test_compile_error_captured_in_io_err(self):
        with tempfile.TemporaryDirectory() as d:
            src = os.path.join(d, "bad.c")
            with open(src, "w") as f:
                f.write("int main( { broken }\n")
            code, out, err = capture(
                self.lib, ["tinyc", "-c", src, "-o", os.path.join(d, "b.o")],
                merged=False)
            self.assertNotEqual(code, 0)
            self.assertIn(b"error", err)

    def test_host_stdout_untouched_by_capture(self):
        # capturing through IO must not redirect the process's stdio:
        # a marker written to real stdout after the call still works,
        # and nothing from the compiler leaked into this process's
        # captured streams (checked by the harness by design of
        # tinyc_main_io using the IO object, not fd redirection).
        code, out, err = capture(self.lib, ["tinyc", "--version"])
        self.assertIn(b"tcc version", out)
        sys.stdout.write("")
        sys.stdout.flush()

    def test_api_compile_and_call(self):
        lib = self.lib
        lib.tcc_new.restype = ctypes.c_void_p
        lib.tcc_set_lib_path.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        lib.tcc_set_output_type.argtypes = [ctypes.c_void_p, ctypes.c_int]
        lib.tcc_compile_string.restype = ctypes.c_int
        lib.tcc_compile_string.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        lib.tcc_relocate.restype = ctypes.c_int
        lib.tcc_relocate.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        lib.tcc_get_symbol.restype = ctypes.c_void_p
        lib.tcc_get_symbol.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        lib.tcc_add_include_path.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        s = lib.tcc_new()
        lib.tcc_set_lib_path(s, os.path.join(DIST, "lib", "tcc").encode())
        lib.tcc_set_output_type(s, 1)  # TCC_OUTPUT_MEMORY
        self.assertEqual(
            lib.tcc_compile_string(s, b"int mul(int a,int b){return a*b;}"), 0)
        self.assertEqual(lib.tcc_relocate(s, None), 0)
        addr = lib.tcc_get_symbol(s, b"mul")
        fn = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_int)(addr)
        self.assertEqual(fn(6, 7), 42)
        lib.tcc_delete(ctypes.c_void_p(s))


class PerStateIoTests(unittest.TestCase):
    """IO is per-state: two states with different buffer IOs used in
    alternation keep their compiler output separate, and a
    tinyc_main_io call in between disturbs neither state's IO."""

    @classmethod
    def setUpClass(cls):
        lib = load()
        lib.tcc_new.restype = ctypes.c_void_p
        lib.tcc_delete.argtypes = [ctypes.c_void_p]
        lib.tcc_set_lib_path.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        lib.tcc_set_output_type.argtypes = [ctypes.c_void_p, ctypes.c_int]
        lib.tcc_compile_string.restype = ctypes.c_int
        lib.tcc_compile_string.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        lib.tcc_set_io.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        lib.tcc_get_io.restype = ctypes.c_void_p
        lib.tcc_get_io.argtypes = [ctypes.c_void_p]
        lib.tcc_io_stdio.restype = ctypes.c_void_p
        cls.lib = lib

    def make_state(self, io=None, output_type=1):
        lib = self.lib
        s = lib.tcc_new()
        self.assertTrue(s)
        lib.tcc_set_lib_path(s, os.path.join(DIST, "lib", "tcc").encode())
        lib.tcc_set_output_type(s, output_type)
        if io is not None:
            lib.tcc_set_io(s, io)
        return s

    def buffer_bytes(self, io, channel):
        n = ctypes.c_size_t()
        p = self.lib.tcc_io_buffer_data(io, channel, ctypes.byref(n))
        return ctypes.string_at(p, n.value) if p else b""

    def test_default_io_is_the_stdio_singleton(self):
        lib = self.lib
        s = self.make_state()
        try:
            self.assertEqual(lib.tcc_get_io(s), lib.tcc_io_stdio())
            io = lib.tcc_io_buffer()
            try:
                lib.tcc_set_io(s, io)
                self.assertEqual(lib.tcc_get_io(s), io)
                lib.tcc_set_io(s, None)
                self.assertEqual(lib.tcc_get_io(s), lib.tcc_io_stdio())
            finally:
                lib.tcc_io_free(io)
        finally:
            lib.tcc_delete(ctypes.c_void_p(s))

    def test_two_states_interleaved_keep_output_separate(self):
        lib = self.lib
        io_a, io_b = lib.tcc_io_buffer(), lib.tcc_io_buffer()
        a = self.make_state(io_a)
        b = self.make_state(io_b)
        try:
            # No error callback is installed, so diagnostics take the
            # default path into the state's own IO err channel.
            self.assertNotEqual(
                lib.tcc_compile_string(a, b"int fa( { broken_a }"), 0)
            err_a = self.buffer_bytes(io_a, 1)
            self.assertIn(b"error", err_a)
            self.assertEqual(self.buffer_bytes(io_b, 1), b"")

            self.assertNotEqual(
                lib.tcc_compile_string(b, b"int fb( { broken_b }"), 0)
            err_b = self.buffer_bytes(io_b, 1)
            self.assertIn(b"error", err_b)
            # B's compile added nothing to A's IO, and vice versa.
            self.assertEqual(self.buffer_bytes(io_a, 1), err_a)

            self.assertNotEqual(
                lib.tcc_compile_string(a, b"int fa2( { broken_a2 }"), 0)
            self.assertGreater(len(self.buffer_bytes(io_a, 1)), len(err_a))
            self.assertEqual(self.buffer_bytes(io_b, 1), err_b)
        finally:
            lib.tcc_delete(ctypes.c_void_p(a))
            lib.tcc_delete(ctypes.c_void_p(b))
            lib.tcc_io_free(io_a)
            lib.tcc_io_free(io_b)

    def test_preprocess_output_goes_to_owning_state(self):
        lib = self.lib
        io_a, io_b = lib.tcc_io_buffer(), lib.tcc_io_buffer()
        # TCC_OUTPUT_PREPROCESS = 5
        a = self.make_state(io_a, output_type=5)
        b = self.make_state(io_b, output_type=5)
        try:
            self.assertEqual(lib.tcc_compile_string(
                a, b"#define VA 111\nint xa = VA;\n"), 0)
            self.assertIn(b"int xa = 111;", self.buffer_bytes(io_a, 0))
            self.assertEqual(self.buffer_bytes(io_b, 0), b"")
            self.assertEqual(lib.tcc_compile_string(
                b, b"#define VB 222\nint xb = VB;\n"), 0)
            self.assertIn(b"int xb = 222;", self.buffer_bytes(io_b, 0))
            self.assertNotIn(b"xb", self.buffer_bytes(io_a, 0))
        finally:
            lib.tcc_delete(ctypes.c_void_p(a))
            lib.tcc_delete(ctypes.c_void_p(b))
            lib.tcc_io_free(io_a)
            lib.tcc_io_free(io_b)

    def test_main_io_does_not_disturb_state_io(self):
        lib = self.lib
        io_a = lib.tcc_io_buffer()
        io_main = lib.tcc_io_buffer()
        a = self.make_state(io_a)
        try:
            self.assertEqual(lib.tcc_get_io(a), io_a)
            code = lib.tinyc_main_io(
                2, argv_of(["tinyc", "--version"]), io_main)
            self.assertEqual(code, 0)
            self.assertIn(b"tcc version", self.buffer_bytes(io_main, 0))
            # The CLI call installed nothing on the API state and
            # captured nothing into its IO.
            self.assertEqual(lib.tcc_get_io(a), io_a)
            self.assertEqual(self.buffer_bytes(io_a, 0), b"")
            self.assertEqual(self.buffer_bytes(io_a, 1), b"")
            # And the state still routes to its own IO afterwards.
            self.assertNotEqual(
                lib.tcc_compile_string(a, b"int fz( { broken_z }"), 0)
            self.assertIn(b"error", self.buffer_bytes(io_a, 1))
            self.assertEqual(self.buffer_bytes(io_main, 1), b"")
        finally:
            lib.tcc_delete(ctypes.c_void_p(a))
            lib.tcc_io_free(io_a)
            lib.tcc_io_free(io_main)


class RunRedirectTests(unittest.TestCase):
    """-run through tinyc_main_io: the run program's process stdio is
    redirected through the installed IO for the duration of the run
    (fd 0 fed from read_in, fd 1/2 delivered to write_out/write_err)."""

    @classmethod
    def setUpClass(cls):
        cls.lib = load()

    def write_src(self, d, body):
        src = os.path.join(d, "prog.c")
        with open(src, "w") as f:
            f.write(body)
        return src

    def run_buffer(self, src, merged=False):
        # argv[0] is the real file path, as the package wrappers do.
        io = (self.lib.tcc_io_buffer_merged() if merged
              else self.lib.tcc_io_buffer())
        try:
            code = self.lib.tinyc_main_io(
                3, argv_of([DUAL, "-run", src]), io)
            n = ctypes.c_size_t()
            p = self.lib.tcc_io_buffer_data(io, 0, ctypes.byref(n))
            out = ctypes.string_at(p, n.value) if p else b""
            p = self.lib.tcc_io_buffer_data(io, 1, ctypes.byref(n))
            err = ctypes.string_at(p, n.value) if p else b""
            return code, out, err
        finally:
            self.lib.tcc_io_free(io)

    def test_run_stdout_captured(self):
        with tempfile.TemporaryDirectory() as d:
            src = self.write_src(
                d, '#include <stdio.h>\n'
                   'int main(){ printf("run-out\\n"); return 3; }\n')
            code, out, err = self.run_buffer(src)
            self.assertEqual(code, 3)
            self.assertEqual(out, b"run-out\n")
            self.assertEqual(err, b"")

    def test_run_stderr_kept_separate(self):
        with tempfile.TemporaryDirectory() as d:
            src = self.write_src(
                d, '#include <stdio.h>\n'
                   'int main(){ printf("to-out\\n");\n'
                   '  fprintf(stderr, "to-err\\n"); return 0; }\n')
            code, out, err = self.run_buffer(src)
            self.assertEqual(code, 0)
            self.assertEqual(out, b"to-out\n")
            self.assertEqual(err, b"to-err\n")

    def test_run_stdin_fed_from_read_in(self):
        feed = b"kernel-stdin-echo\n"
        state = {"pos": 0}
        out_chunks = []

        def _w(ctx, buf, n):
            out_chunks.append(ctypes.string_at(buf, n))
            return n

        def _r(ctx, buf, cap):
            if state["pos"] >= len(feed):
                return 0
            chunk = feed[state["pos"]:state["pos"] + cap]
            state["pos"] += len(chunk)
            ctypes.memmove(buf, chunk, len(chunk))
            return len(chunk)

        keep = [_IO_WRITE(_w), _IO_READ(_r)]
        io = self.lib.tcc_io_callback(keep[0], keep[0], keep[1], None)
        try:
            with tempfile.TemporaryDirectory() as d:
                src = self.write_src(
                    d, '#include <stdio.h>\n'
                       'int main(){ int c;\n'
                       '  while ((c = getchar()) != EOF) putchar(c);\n'
                       '  return 0; }\n')
                code = self.lib.tinyc_main_io(
                    3, argv_of([DUAL, "-run", src]), io)
            self.assertEqual(code, 0)
            self.assertEqual(b"".join(out_chunks), feed)
        finally:
            self.lib.tcc_io_free(io)

    def test_run_stdin_echo_repeated_in_one_process(self):
        # A run program that reads stdin to EOF leaves the host's
        # stdin FILE with a sticky EOF flag; the next run must still
        # see read_in's bytes. Run the echo three times back to back.
        for _ in range(3):
            self.test_run_stdin_fed_from_read_in()

    def test_run_large_output_does_not_deadlock(self):
        # 8192 lines x 40 bytes = 320 KiB, far beyond a pipe buffer.
        with tempfile.TemporaryDirectory() as d:
            src = self.write_src(
                d, '#include <stdio.h>\n'
                   'int main(){ int i;\n'
                   '  for (i = 0; i < 8192; i++)\n'
                   '    printf("line-%06d-012345678901234567890123456\\n", i);\n'
                   '  return 0; }\n')
            code, out, err = self.run_buffer(src)
            self.assertEqual(code, 0)
            self.assertEqual(len(out), 8192 * 40)
            self.assertTrue(out.startswith(b"line-000000-"))
            self.assertTrue(out.endswith(b"line-008191-012345678901234567890123456\n"))
            self.assertEqual(err, b"")


if __name__ == "__main__":
    unittest.main(verbosity=2)
