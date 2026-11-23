# tinyc for Python

TinyC (TCC) as an embedded compiler in Python. The package bundles one
dual-form native file — the same file is the `tcc` command and the
libtcc library — together with the headers and libraries it needs.

## Install

```
pip install tinyc
```

The native distribution is inside the package (`tinyc/native/`).
`TINYC_DIST` (distribution directory) and `TINYC_NATIVE` (the dual
file itself) override the bundled location.

## Command-line style

```python
import tinyc

code = tinyc.run_cli(["hello.c", "-o", "hello"])   # returns exit code
code, output = tinyc.run_cli_capture(["--version"])  # captured output
```

`run_cli` fills in argv[0] with the native file's own path, so the
compiler finds its bundled headers and libraries. The host process is
never exited; the return value is the compiler's exit code.

## API style

```python
import ctypes, tinyc

with tinyc.State() as s:
    s.add_include_path("/path/to/headers")     # local paths, used as-is
    s.add_file("/path/to/libfoo.a")
    s.compile_string("int add(int a, int b){ return a + b; }")
    add = s.function("add", ctypes.c_int, ctypes.c_int, ctypes.c_int)
    assert add(19, 23) == 42
```

Compile errors raise `tinyc.TinyCError` and are also collected in
`s.errors`; they are not printed.

## Output (IO)

Compiler output goes through the TCCIO interface. Default is the
process's real stdout/stderr. Alternatives:

```python
tinyc.run_cli(args, io="capture")                      # via run_cli_capture
tinyc.run_cli(args, io=tinyc.IO.file("build.log"))     # to a file
tinyc.run_cli(args, io=tinyc.IO.callback(out=my_fn))   # my_fn(bytes)
tinyc.run_cli(args, io=open("log", "wb"))              # any binary file object
```

## Tests

```
python3 bundle_native.py   # after ../../build.sh
python3 -m unittest discover -s tests
```
