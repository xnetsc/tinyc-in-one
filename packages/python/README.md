# tinyc for Python

TinyC (TCC) as an embedded compiler in Python. The package bundles one
dual-form native file per target — the same file is the `tcc` command
and the libtcc library — together with the headers and libraries it
needs. The binding is `ctypes` only, with no extra native components.

## Install

```
pip install tinyc
```

Wheels are published per host platform tag (`manylinux` x86_64 /
aarch64, `win_amd64`, `macosx` arm64 / x86_64); each wheel carries
that host's target variants. Inside the wheel, the `tinyc` package
uses the unified layout shared by all the native packages:

```
tinyc/
├─ packaging-manifest.json     the (host, target) combinations inside
├─ natives/<host-triple>/toolchain/<target-triple>/tinyc[.exe]
└─ sysroot/<target-triple>/    one sysroot per target, shared by all hosts
```

The host triple is detected from the platform (`$TINYC_HOST_TRIPLE`
overrides detection, `$TINYC_TRIPLE` is an alias); the target
defaults to the host (`$TINYC_TARGET_TRIPLE` selects a cross
target). `$TINYC_DIST` overrides the package root used for the
lookup, and `$TINYC_NATIVE` points at the dual-form file itself,
bypassing the manifest lookup. A combination the wheel does not
contain fails with an error naming the host, the target, and the
combinations available. Hosts and targets are 64-bit only.

The host/target matrix and the run rule (a compiled program runs
in-process only when the target is the host's own platform; cross
combinations compile to files) are in the
[repository README](../../README.md).

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
