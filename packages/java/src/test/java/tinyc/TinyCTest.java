package tinyc;

import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.io.ByteArrayOutputStream;
import java.lang.invoke.MethodHandle;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.concurrent.atomic.AtomicReference;

import static java.lang.foreign.ValueLayout.JAVA_INT;

/**
 * 绑定层测试：四组用例，任一断言失败抛 AssertionError 并以非零码退出。
 * 运行方式见 run-tests.sh。
 */
public final class TinyCTest {

    private static int passed = 0;

    private static void check(boolean cond, String name) {
        if (!cond) {
            throw new AssertionError("FAILED: " + name);
        }
        passed++;
        System.out.println("PASS: " + name);
    }

    public static void main(String[] args) throws Throwable {
        System.out.println("native file: " + TinyC.nativeFile());
        System.out.println("runtime dir: " + TinyC.runtimeDir());
        System.out.println("hasCli: " + TinyC.hasCli());

        testApiCompileAndCall();
        testRunCli();
        testDynamicInclude();
        testIoCaptureAndUpcall();
        testCliWithIo();
        testCliRunStdioThroughIo();

        System.out.println("ALL TESTS PASSED (" + passed + " checks)");
    }

    /** 用例 1：API 编译源码字符串、relocate、取符号调用并断言返回值。 */
    private static void testApiCompileAndCall() throws Throwable {
        try (TccSession s = TinyC.newSession()) {
            check(s.setOutputType(TinyC.TCC_OUTPUT_MEMORY) == 0, "api: setOutputType(MEMORY)");
            int rc = s.compileString("int add(int a, int b) { return a + b; }\n");
            check(rc == 0, "api: compileString rc==0 (got " + rc + ")");
            check(s.relocate() == 0, "api: relocate rc==0");
            MemorySegment fn = s.getSymbol("add");
            check(fn != null, "api: getSymbol(add) found");
            MethodHandle mh = TinyC.functionHandle(fn,
                    FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
            int v = (int) mh.invokeExact(40, 2);
            check(v == 42, "api: add(40, 2) == 42 (got " + v + ")");
        }
    }

    /** 用例 2：runCli 编译一个 .c 到可执行文件，运行并断言退出码与输出。 */
    private static void testRunCli() throws Exception {
        check(TinyC.hasCli(), "cli: native file exports a main entry");
        Path dir = Files.createTempDirectory("tinyc-cli");
        Path src = dir.resolve("hello.c");
        Files.writeString(src, "#include <stdio.h>\n"
                + "int main(void) { printf(\"hello-from-tcc\\n\"); return 7; }\n");
        Path exe = dir.resolve("hello");
        int rc = TinyC.runCli("-o", exe.toString(), src.toString());
        check(rc == 0, "cli: runCli rc==0 (got " + rc + ")");
        check(Files.isExecutable(exe), "cli: output file is executable");

        Process p = new ProcessBuilder(exe.toString()).redirectErrorStream(true).start();
        String out = new String(p.getInputStream().readAllBytes(), StandardCharsets.UTF_8);
        int exit = p.waitFor();
        check(exit == 7, "cli: compiled program exit code == 7 (got " + exit + ")");
        check(out.contains("hello-from-tcc"), "cli: compiled program output contains hello-from-tcc");
    }

    /** 用例 3：动态添加——指定本地目录的头文件并成功 include，全程不拷贝。 */
    private static void testDynamicInclude() throws Throwable {
        Path dir = Files.createTempDirectory("tinyc-inc");
        Files.writeString(dir.resolve("mylib.h"),
                "#pragma once\n#define MAGIC_NUMBER 42\n"
                        + "static int magic(void) { return MAGIC_NUMBER; }\n");
        try (TccSession s = TinyC.newSession()) {
            check(s.addIncludePath(dir.toString()) == 0, "dyn: addIncludePath rc==0");
            check(s.setOutputType(TinyC.TCC_OUTPUT_MEMORY) == 0, "dyn: setOutputType(MEMORY)");
            int rc = s.compileString("#include \"mylib.h\"\nint get_magic(void) { return magic(); }\n");
            check(rc == 0, "dyn: compileString with local header rc==0 (got " + rc + ")");
            check(s.relocate() == 0, "dyn: relocate rc==0");
            MemorySegment fn = s.getSymbol("get_magic");
            check(fn != null, "dyn: getSymbol(get_magic) found");
            MethodHandle mh = TinyC.functionHandle(fn, FunctionDescriptor.of(JAVA_INT));
            int v = (int) mh.invokeExact();
            check(v == 42, "dyn: get_magic() == 42 (got " + v + ")");
        }
    }

    /** 用例 4：IO——错误通道经 upcall 进 Capturing；写/读跳板各做一次往返调用。 */
    private static void testIoCaptureAndUpcall() throws Throwable {
        TccIO.Capturing cap = TccIO.capturing();
        try (TccSession s = TinyC.newSession()) {
            s.setIO(cap);
            s.setOutputType(TinyC.TCC_OUTPUT_MEMORY);
            int rc = s.compileString("int broken( { return 1; }\n");
            check(rc == -1, "io: broken source rc==-1 (got " + rc + ")");
            check(cap.errText().contains("error"),
                    "io: captured stderr contains 'error' (got: " + cap.errText().trim() + ")");
        }

        // 写跳板往返：把 upcall stub 当作 C 函数指针经 downcall 调一次
        TccIO.Capturing cap2 = TccIO.capturing();
        AtomicReference<Throwable> sink = new AtomicReference<>();
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment stub = TccIoUpcalls.writeStub(false, cap2, arena, sink);
            MethodHandle call = TinyC.functionHandle(stub, TccIoUpcalls.WRITE_DESCRIPTOR);
            byte[] payload = "abc".getBytes(StandardCharsets.UTF_8);
            MemorySegment buf = arena.allocateFrom(ValueLayout.JAVA_BYTE, payload);
            int n = (int) call.invokeExact(MemorySegment.NULL, buf, (long) payload.length);
            check(n == 3, "io: write upcall returned 3 (got " + n + ")");
            check(cap2.outText().equals("abc"), "io: write upcall delivered bytes 'abc'");
            check(sink.get() == null, "io: no callback error stored");
        }

        // 读跳板往返：把 upcall stub 当作 C 函数指针经 downcall 调，
        // readIn 备好的字节写进 C 缓冲区，耗尽后返回 0 (EOF)
        byte[] feed = "hello".getBytes(StandardCharsets.UTF_8);
        TccIO feeder = new TccIO() {
            private int pos;

            @Override
            public int writeOut(byte[] data) {
                return data.length;
            }

            @Override
            public int writeErr(byte[] data) {
                return data.length;
            }

            @Override
            public int readIn(byte[] buf) {
                if (pos >= feed.length) {
                    return 0;
                }
                int n = Math.min(buf.length, feed.length - pos);
                System.arraycopy(feed, pos, buf, 0, n);
                pos += n;
                return n;
            }
        };
        AtomicReference<Throwable> sink2 = new AtomicReference<>();
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment stub = TccIoUpcalls.readStub(feeder, arena, sink2);
            MethodHandle call = TinyC.functionHandle(stub, TccIoUpcalls.READ_DESCRIPTOR);
            MemorySegment buf = arena.allocate(16);
            int n = (int) call.invokeExact(MemorySegment.NULL, buf, 16L);
            check(n == 5, "io: read upcall returned 5 (got " + n + ")");
            check(buf.getString(0, StandardCharsets.UTF_8).equals("hello"),
                    "io: read upcall delivered bytes 'hello'");
            int n2 = (int) call.invokeExact(MemorySegment.NULL, buf, 16L);
            check(n2 == 0, "io: read upcall EOF returns 0 (got " + n2 + ")");
            check(sink2.get() == null, "io: no read callback error stored");
        }

        // 读跳板异常：readIn 抛异常时 stub 向 C 返回 -1、异常存进 sink，不穿 C 边界
        TccIO throwingRead = new TccIO() {
            @Override
            public int writeOut(byte[] data) {
                return data.length;
            }

            @Override
            public int writeErr(byte[] data) {
                return data.length;
            }

            @Override
            public int readIn(byte[] buf) {
                throw new IllegalStateException("boom-read");
            }
        };
        AtomicReference<Throwable> sink3 = new AtomicReference<>();
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment stub = TccIoUpcalls.readStub(throwingRead, arena, sink3);
            MethodHandle call = TinyC.functionHandle(stub, TccIoUpcalls.READ_DESCRIPTOR);
            MemorySegment buf = arena.allocate(8);
            int n = (int) call.invokeExact(MemorySegment.NULL, buf, 8L);
            check(n == -1, "io: read upcall exception returns -1 (got " + n + ")");
            check(sink3.get() instanceof IllegalStateException,
                    "io: read exception stored in sink");
        }
    }

    /**
     * 用例 5：runCli 带 IO——三通道经 tcc_io_callback 组成 TCCIO 传给
     * tinyc_main_io：编译器自身输出（--version、-E）经 writeOut 捕获、
     * 诊断经 writeErr 捕获、writeOut 抛异常时原生调用返回后以
     * TccException 重抛。readIn 随 TCCIO 一并传给内核，其跳板往返
     * 见用例 4；-run 目标程序经同一 IO 读 stdin、写 stdout/stderr，
     * 见用例 6。
     */
    private static void testCliWithIo() throws Exception {
        TccIO.Capturing cap = TccIO.capturing();
        int rc = TinyC.runCli(cap, "--version");
        check(rc == 0, "cli-io: --version rc==0 (got " + rc + ")");
        check(cap.outText().contains("version"),
                "cli-io: --version output captured by writeOut (got: " + cap.outText().trim() + ")");

        Path dir = Files.createTempDirectory("tinyc-cli-io");
        Path src = dir.resolve("pp.c");
        Files.writeString(src, "#define MARKER 1234\nint marker_value = MARKER;\n");
        TccIO.Capturing capPp = TccIO.capturing();
        rc = TinyC.runCli(capPp, "-E", src.toString());
        check(rc == 0, "cli-io: -E rc==0 (got " + rc + ")");
        check(capPp.outText().contains("marker_value = 1234"),
                "cli-io: preprocessed output captured by writeOut");

        Path bad = dir.resolve("bad.c");
        Files.writeString(bad, "int broken( { return 1; }\n");
        TccIO.Capturing capErr = TccIO.capturing();
        rc = TinyC.runCli(capErr, "-c", bad.toString(), "-o", dir.resolve("bad.o").toString());
        check(rc != 0, "cli-io: broken source rc!=0 (got " + rc + ")");
        check(capErr.errText().contains("error"),
                "cli-io: diagnostics captured by writeErr (got: " + capErr.errText().trim() + ")");

        TccIO throwingOut = new TccIO() {
            @Override
            public int writeOut(byte[] data) {
                throw new IllegalStateException("boom-out");
            }

            @Override
            public int writeErr(byte[] data) {
                return data.length;
            }

            @Override
            public int readIn(byte[] buf) {
                return 0;
            }
        };
        boolean rethrown = false;
        try {
            TinyC.runCli(throwingOut, "--version");
        } catch (TccException e) {
            rethrown = e.getCause() instanceof IllegalStateException;
        }
        check(rethrown, "cli-io: writeOut exception rethrown as TccException after the native call");
    }

    /**
     * 用例 6：runCli 带 IO 跑 -run——目标程序的 stdin 由 readIn 喂入、
     * stdout 回到 writeOut、stderr 回到 writeErr：程序把 stdin 原样
     * 回显到 stdout，另向 stderr 写一行标记，退出码原样返回。
     */
    private static void testCliRunStdioThroughIo() throws Exception {
        byte[] feed = "java-stdin-echo\n".getBytes(StandardCharsets.UTF_8);
        ByteArrayOutputStream outBuf = new ByteArrayOutputStream();
        ByteArrayOutputStream errBuf = new ByteArrayOutputStream();
        TccIO io = new TccIO() {
            private int pos;

            @Override
            public int writeOut(byte[] data) {
                outBuf.write(data, 0, data.length);
                return data.length;
            }

            @Override
            public int writeErr(byte[] data) {
                errBuf.write(data, 0, data.length);
                return data.length;
            }

            @Override
            public int readIn(byte[] buf) {
                if (pos >= feed.length) {
                    return 0;
                }
                int n = Math.min(buf.length, feed.length - pos);
                System.arraycopy(feed, pos, buf, 0, n);
                pos += n;
                return n;
            }
        };

        Path dir = Files.createTempDirectory("tinyc-cli-run-io");
        Path src = dir.resolve("echo.c");
        Files.writeString(src, "#include <stdio.h>\n"
                + "int main(void) { int c;\n"
                + "  while ((c = getchar()) != EOF) putchar(c);\n"
                + "  fprintf(stderr, \"run-err\\n\");\n"
                + "  return 5; }\n");
        int rc = TinyC.runCli(io, "-run", src.toString());
        check(rc == 5, "cli-run-io: -run rc==5 (got " + rc + ")");
        check(new String(outBuf.toByteArray(), StandardCharsets.UTF_8).equals("java-stdin-echo\n"),
                "cli-run-io: stdin echoed to writeOut (got: "
                        + new String(outBuf.toByteArray(), StandardCharsets.UTF_8).trim() + ")");
        check(new String(errBuf.toByteArray(), StandardCharsets.UTF_8).equals("run-err\n"),
                "cli-run-io: program stderr delivered to writeErr (got: "
                        + new String(errBuf.toByteArray(), StandardCharsets.UTF_8).trim() + ")");
    }
}
