import ctypes
import os
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
import tinyc


class CliModeTests(unittest.TestCase):
    def test_version_via_main_returns_not_exits(self):
        code, out = tinyc.run_cli_capture(["--version"])
        self.assertEqual(code, 0)
        self.assertIn(b"tcc version", out)

    def test_dumpversion_returns_zero(self):
        code, out = tinyc.run_cli_capture(["-dumpversion"])
        self.assertEqual(code, 0)
        self.assertIn(b"0.9.28", out)

    def test_compile_and_run_executable(self):
        with tempfile.TemporaryDirectory() as d:
            src = os.path.join(d, "hello.c")
            exe = os.path.join(d, "hello")
            with open(src, "w") as f:
                f.write('#include <stdio.h>\n'
                        'int main(){ printf("hi-py\\n"); return 7; }\n')
            code, out = tinyc.run_cli_capture([src, "-o", exe])
            self.assertEqual(code, 0, out)
            r = subprocess.run([exe], capture_output=True)
            self.assertEqual(r.returncode, 7)
            self.assertEqual(r.stdout, b"hi-py\n")

    def test_bad_source_reports_error_and_returns_nonzero(self):
        with tempfile.TemporaryDirectory() as d:
            src = os.path.join(d, "bad.c")
            with open(src, "w") as f:
                f.write("int main( { nope }\n")
            code, out = tinyc.run_cli_capture(["-c", src, "-o",
                                               os.path.join(d, "bad.o")])
            self.assertNotEqual(code, 0)
            self.assertIn(b"error", out)

    def test_preprocess_output_captured_by_io(self):
        with tempfile.TemporaryDirectory() as d:
            src = os.path.join(d, "pp.c")
            with open(src, "w") as f:
                f.write("#define VAL 4242\nint x = VAL;\n")
            code, out = tinyc.run_cli_capture(["-E", src])
            self.assertEqual(code, 0, out)
            self.assertIn(b"int x = 4242;", out)

    def test_dynamic_add_include_dir_and_library_file(self):
        with tempfile.TemporaryDirectory() as d:
            inc = os.path.join(d, "inc"); os.mkdir(inc)
            with open(os.path.join(inc, "dyn.h"), "w") as f:
                f.write("#define DYNGREET 99\n")
            libsrc = os.path.join(d, "dyn.c")
            with open(libsrc, "w") as f:
                f.write("int dynval(void){ return 1234; }\n")
            liba = os.path.join(d, "libdyn.a")
            code, out = tinyc.run_cli_capture(["-c", libsrc, "-o",
                                               os.path.join(d, "dyn.o")])
            self.assertEqual(code, 0, out)
            # The archive is built by TCC's own -ar: it is consumed
            # by TCC, and a system ar/ranlib cannot index TCC's ELF
            # objects on every platform (on macOS its symbol table
            # comes out empty and the link below loses _dynval).
            code, out = tinyc.run_cli_capture(
                ["-ar", "rcs", liba, os.path.join(d, "dyn.o")])
            self.assertEqual(code, 0, out)
            main = os.path.join(d, "main.c")
            with open(main, "w") as f:
                f.write('#include "dyn.h"\n#include <stdio.h>\n'
                        'extern int dynval(void);\n'
                        'int main(){ printf("dyn %d %d\\n", DYNGREET, dynval());'
                        ' return 0; }\n')
            exe = os.path.join(d, "prog")
            code, out = tinyc.run_cli_capture(
                ["-I" + inc, main, liba, "-o", exe])
            self.assertEqual(code, 0, out)
            r = subprocess.run([exe], capture_output=True)
            self.assertEqual(r.stdout, b"dyn 99 1234\n")


class ApiModeTests(unittest.TestCase):
    def test_compile_string_and_call(self):
        with tinyc.State() as s:
            s.compile_string("int add(int a, int b){ return a + b; }")
            add = s.function("add", ctypes.c_int, ctypes.c_int, ctypes.c_int)
            self.assertEqual(add(19, 23), 42)

    def test_compile_error_collected_not_printed(self):
        with tinyc.State() as s:
            with self.assertRaises(tinyc.TinyCError):
                s.compile_string("int broken( {")
            self.assertTrue(any("error" in e for e in s.errors))

    def test_dynamic_add_via_api(self):
        with tempfile.TemporaryDirectory() as d:
            inc = os.path.join(d, "inc"); os.mkdir(inc)
            with open(os.path.join(inc, "cfg.h"), "w") as f:
                f.write("#define CFGVAL 77\n")
            with tinyc.State() as s:
                s.add_include_path(inc)
                s.compile_string('#include "cfg.h"\n'
                                 'int getcfg(void){ return CFGVAL; }')
                getcfg = s.function("getcfg", ctypes.c_int)
                self.assertEqual(getcfg(), 77)

    def test_define_symbol(self):
        with tinyc.State() as s:
            s.define_symbol("MAGIC", "123")
            s.compile_string("int magic(void){ return MAGIC; }")
            magic = s.function("magic", ctypes.c_int)
            self.assertEqual(magic(), 123)

    def test_output_object_file(self):
        with tempfile.TemporaryDirectory() as d:
            obj = os.path.join(d, "x.o")
            with tinyc.State(output_type=tinyc.TCC_OUTPUT_OBJ) as s:
                s.compile_string("int xf(void){ return 1; }")
                s.output_file(obj)
            self.assertTrue(os.path.getsize(obj) > 0)

    def test_sequential_states(self):
        for i in range(3):
            with tinyc.State() as s:
                s.compile_string(f"int v(void){{ return {i}; }}")
                v = s.function("v", ctypes.c_int)
                self.assertEqual(v(), i)


class IoTests(unittest.TestCase):
    def test_io_callback_receives_compiler_output(self):
        chunks = []
        io = tinyc.IO.callback(out=lambda b: chunks.append(("o", b)),
                               err=lambda b: chunks.append(("e", b)))
        code = tinyc.run_cli(["--version"], io=io)
        self.assertEqual(code, 0)
        data = b"".join(b for _, b in chunks)
        self.assertIn(b"tcc version", data)
        io.close()

    def test_io_file(self):
        with tempfile.TemporaryDirectory() as d:
            log = os.path.join(d, "log.txt")
            io = tinyc.IO.file(log)
            code = tinyc.run_cli(["--version"], io=io)
            io.close()
            self.assertEqual(code, 0)
            with open(log, "rb") as f:
                self.assertIn(b"tcc version", f.read())

    def test_run_program_stdio_goes_through_io(self):
        # -run with an IO installed: the program's stdin is fed by
        # read_in, its stdout is delivered to write_out and its
        # stderr to write_err, in separate channels.
        feed = b"echo-me-123\n"
        state = {"pos": 0}

        def read_in(cap):
            if state["pos"] >= len(feed):
                return b""
            chunk = feed[state["pos"]:state["pos"] + cap]
            state["pos"] += len(chunk)
            return chunk

        out_chunks, err_chunks = [], []
        io = tinyc.IO.callback(out=out_chunks.append,
                               err=err_chunks.append, read_in=read_in)
        with tempfile.TemporaryDirectory() as d:
            src = os.path.join(d, "echo.c")
            with open(src, "w") as f:
                f.write('#include <stdio.h>\n'
                        'int main(){ int c;\n'
                        '  while ((c = getchar()) != EOF) putchar(c);\n'
                        '  fprintf(stderr, "run-marker\\n");\n'
                        '  return 3; }\n')
            code = tinyc.run_cli(["-run", src], io=io)
        io.close()
        self.assertEqual(code, 3)
        self.assertEqual(b"".join(out_chunks), feed)
        self.assertEqual(b"".join(err_chunks), b"run-marker\n")

    def test_io_null_swallows(self):
        code, out = tinyc.run_cli_capture(["--version"])
        self.assertIn(b"tcc version", out)  # sanity: capture works
        io = tinyc.IO.null()
        code = tinyc.run_cli(["--version"], io=io)
        self.assertEqual(code, 0)


class HostSimulationTests(unittest.TestCase):
    """Host/target resolution, simulated in child processes through
    TINYC_HOST_TRIPLE / TINYC_TARGET_TRIPLE overrides."""

    PKG_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

    def run_child(self, env_extra, code):
        env = dict(os.environ)
        env.update(env_extra)
        env["PYTHONPATH"] = self.PKG_DIR + os.pathsep + env.get("PYTHONPATH", "")
        return subprocess.run([sys.executable, "-c", code],
                              capture_output=True, env=env)

    def test_host_override_resolves_and_compiles(self):
        # The simulated host's combination is resolved and compiled
        # when this package contains it (the full staging does); a
        # single-host package (e.g. a macOS wheel) does not contain
        # it, and there the override must produce the missing-
        # combination error naming the simulated host instead.
        r = self.run_child(
            {"TINYC_HOST_TRIPLE": "x86_64-linux-gnu"},
            "import json, os\n"
            "import tinyc\n"
            "root = os.path.dirname(os.path.abspath(tinyc.__file__))\n"
            "with open(os.path.join(root, 'packaging-manifest.json')) as f:\n"
            "    combos = {(c['host'], c['target'])\n"
            "              for c in json.load(f)['combinations']}\n"
            "if ('x86_64-linux-gnu', 'x86_64-linux-gnu') in combos:\n"
            "    r = tinyc.resolve_native()\n"
            "    assert 'natives/x86_64-linux-gnu/toolchain/x86_64-linux-gnu' "
            "in r['binary_path'], r['binary_path']\n"
            "    code, out = tinyc.run_cli_capture(['-dumpversion'])\n"
            "    assert code == 0, out\n"
            "    print('VERSION', out.decode().strip())\n"
            "else:\n"
            "    try:\n"
            "        tinyc.resolve_native()\n"
            "        print('NO-THROW')\n"
            "    except Exception as e:\n"
            "        print('ERR', e)\n")
        self.assertEqual(r.returncode, 0, r.stderr)
        if b"VERSION" in r.stdout:
            self.assertIn(b"VERSION 0.9.28", r.stdout)
        else:
            self.assertIn(b"no native build for host x86_64-linux-gnu",
                          r.stdout)
            self.assertIn(b"available combinations:", r.stdout)

    def test_missing_combination_names_host_and_available(self):
        import json
        root = os.path.dirname(os.path.abspath(tinyc.__file__))
        with open(os.path.join(root, "packaging-manifest.json")) as f:
            combos = json.load(f)["combinations"]
        r = self.run_child(
            {"TINYC_HOST_TRIPLE": "x86_64-w64-mingw32"},
            "import tinyc\n"
            "try:\n"
            "    tinyc.resolve_native()\n"
            "    print('NO-THROW')\n"
            "except Exception as e:\n"
            "    print('ERR', e)\n")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn(b"no native build for host x86_64-w64-mingw32", r.stdout)
        # Every combination this package does contain must be named
        # in the error's available list.
        for c in combos:
            self.assertIn(f"{c['host']} x {c['target']}".encode(), r.stdout)

    def test_outputs_come_from_manifest(self):
        import json
        r = tinyc.resolve_native()
        with open(os.path.join(r["root"], "packaging-manifest.json")) as f:
            mf = json.load(f)
        combo = next(c for c in mf["combinations"]
                     if c["host"] == r["host"] and c["target"] == r["target"])
        self.assertEqual(r["outputs"], combo.get("outputs"))

    def test_32bit_host_rejected(self):
        r = self.run_child(
            {"TINYC_HOST_TRIPLE": "i386-linux-gnu"},
            "import tinyc\n"
            "try:\n"
            "    tinyc.resolve_native()\n"
            "    print('NO-THROW')\n"
            "except Exception as e:\n"
            "    print('ERR', e)\n")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn(b"32-bit hosts are not supported", r.stdout)
        self.assertIn(b"i386-linux-gnu", r.stdout)


if __name__ == "__main__":
    unittest.main(verbosity=2)
