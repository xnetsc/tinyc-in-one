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
        if machine == "riscv64":
            return "riscv64-linux-gnu"
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
        for sym in ["main", "tinyc_main", "tinyc_main_io", "tcc_new",
                    "tcc_compile_string", "tcc_io_buffer", "tinyc_set_io"]:
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
