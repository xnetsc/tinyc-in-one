package tinyc;

import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
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

    /** 用例 4：IO——错误通道经 upcall 进 Capturing；写跳板做一次往返调用。 */
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
    }
}
