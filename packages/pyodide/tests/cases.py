"""Shared test cases for tinyc_web, run by both harnesses:
run_node.mjs (Node worker, sync tier) and run_page.mjs (Chromium
page main thread, async tier). Cases are written against an aw()
helper that awaits a value only when it is a coroutine, so the
same case list drives the tier whose methods are synchronous and
the tier whose methods are coroutines.
"""
import inspect

import js

import tinyc_web
from tinyc_web import IO, State, TinyCError

RESULTS = []


def check(name, cond):
    RESULTS.append((name, bool(cond)))
    print(("PASS" if cond else "FAIL"), name, flush=True)


async def aw(value):
    if inspect.iscoroutine(value):
        return await value
    return value


async def run_all():
    expected_tier = getattr(js, "__TINYC_WEB_TEST_TIER__", None)
    check("tier is sync or async", tinyc_web.tier() in ("sync", "async"))
    if expected_tier:
        check("tier matches the harness", tinyc_web.tier() == expected_tier)
    check("default target is bundled",
          tinyc_web.target_triple() in tinyc_web.bundled_targets())

    # compileString -> call the compiled function, check its value
    s = State()
    try:
        await aw(s.compile_string(
            "int add(int a, int b) { return a + b; }"))
        add = await aw(s.function("add"))
        check("compileString + add(19, 23) == 42",
              await aw(add(19, 23)) == 42)
    finally:
        s.close()

    # State.run executes main and returns its exit code
    s = State()
    try:
        await aw(s.compile_string("int main(void) { return 7; }"))
        check("State.run main() == 7", await aw(s.run()) == 7)
    finally:
        s.close()

    # a compile error raises TinyCError carrying the diagnostics
    s = State()
    try:
        await aw(s.compile_string("int broken( {"))
        check("bad source raises TinyCError", False)
    except TinyCError as e:
        check("bad source raises TinyCError",
              "error" in str(e) or any("error" in x for x in s.errors))
    finally:
        s.close()

    # run_cli: version through a merged capture buffer
    code, out = await aw(tinyc_web.run_cli_capture(["--version"]))
    check("run_cli_capture --version",
          code == 0 and b"tcc version" in out)

    # run_cli: -E output is captured by the capture IO
    with open("/work_pp.c", "w") as f:
        f.write("#define VAL 4242\nint x = VAL;\n")
    code, out = await aw(tinyc_web.run_cli_capture(["-E", "/work_pp.c"]))
    check("run_cli_capture -E expands macros",
          code == 0 and b"int x = 4242;" in out)

    # run_cli -run: the program runs in the host, its printf reaches
    # the capture IO and its return value is the exit code
    with open("/work_hello.c", "w") as f:
        f.write("int printf(const char *fmt, ...);\n"
                "int main(void) { printf(\"hi-pyodide\\n\"); return 3; }\n")
    code, out = await aw(tinyc_web.run_cli_capture(
        ["-run", "/work_hello.c"]))
    check("run_cli -run returns 3 and prints",
          code == 3 and b"hi-pyodide" in out)

    # IO.callback receives compiler output as bytes
    chunks = []
    io = IO.callback(out=chunks.append, err=chunks.append)
    code = await aw(tinyc_web.run_cli(["--version"], io=io))
    io.close()
    check("IO.callback receives --version output",
          code == 0 and b"tcc version" in b"".join(chunks))

    # per-state IO: the buffer receives the state's own run output
    buf = IO.buffer()
    s = State(io=buf)
    try:
        await aw(s.compile_string(
            "int printf(const char *fmt, ...);\n"
            "int main(void) { printf(\"state-io\\n\"); return 0; }"))
        check("state run with IO returns 0", await aw(s.run()) == 0)
    finally:
        s.close()
    check("state IO buffer captured run output",
          b"state-io" in buf.out)
    buf.close()

    # a file compiled through the virtual FS
    with open("/work_vfs.c", "w") as f:
        f.write("int getv(void) { return 1234; }\n")
    s = State()
    try:
        await aw(s.compile_file("/work_vfs.c"))
        getv = await aw(s.function("getv"))
        check("compile_file via VFS, getv() == 1234",
              await aw(getv()) == 1234)
    finally:
        s.close()

    # define_symbol reaches the compiled code
    s = State()
    try:
        await aw(s.define_symbol("MAGIC", "123"))
        await aw(s.compile_string("int magic(void) { return MAGIC; }"))
        magic = await aw(s.function("magic"))
        check("define_symbol MAGIC == 123", await aw(magic()) == 123)
    finally:
        s.close()

    # output_file writes the product back to the Python filesystem
    s = State(output_type=tinyc_web.TCC_OUTPUT_EXE)
    try:
        await aw(s.compile_string("int xf(void) { return 1; }"))
        await aw(s.output_file("/work_x.wasm"))
    finally:
        s.close()
    with open("/work_x.wasm", "rb") as f:
        magic = f.read(4)
    check("output_file product has wasm magic", magic == b"\x00asm")

    failed = [name for name, ok in RESULTS if not ok]
    print(f"CASES DONE {len(RESULTS) - len(failed)}/{len(RESULTS)}",
          flush=True)
    return {"total": len(RESULTS), "failed": failed}


def entry_sync():
    """Sync-tier entry: the harness calls this through
    callPromising, and run_sync drives the case list."""
    import traceback

    from pyodide.ffi import run_sync

    async def guarded():
        try:
            return await run_all()
        except BaseException:
            traceback.print_exc()
            raise

    return run_sync(guarded())

