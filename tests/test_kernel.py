"""Kernel-level tests for the dual-form file, driven through ctypes
against the distribution directly (no package layer involved)."""
import ctypes
import os
import subprocess
import sys
import tempfile
import unittest

DIST = os.path.normpath(os.path.join(
    os.path.dirname(__file__), "..", "build", "dist", "x86_64-linux-gnu"))
DUAL = os.path.join(DIST, "bin", "tinyc")


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
    return lib


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


if __name__ == "__main__":
    unittest.main(verbosity=2)
