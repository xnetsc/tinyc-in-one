package com.tinyc;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.util.Arrays;

/**
 * JVM 侧纯逻辑测试：只测不依赖原生库、不依赖 android.* 的部分
 * （argv 组装、TinycIO 流适配、常量与 libtcc.h 对齐）。
 *
 * <p>运行：javac 编译 main 源码（除 TinycAssets.java）与本文件后，
 * java com.tinyc.TinycJvmTest。build.sh 的 jvm-test 模式会自动做这件事，
 * 输出留档在 test-output.txt。退出码 0 表示全部通过。</p>
 */
public final class TinycJvmTest {

    private static int passed = 0;
    private static int failed = 0;

    private static void check(String name, boolean cond) {
        if (cond) {
            passed++;
            System.out.println("PASS " + name);
        } else {
            failed++;
            System.out.println("FAIL " + name);
        }
    }

    public static void main(String[] argv) throws Exception {
        testArgsForMain();
        testArgsForRun();
        testRequireNoNulls();
        testIoOfStreams();
        testIoNull();
        testConstants();

        System.out.println("----");
        System.out.println("passed=" + passed + " failed=" + failed);
        if (failed > 0) {
            System.exit(1);
        }
    }

    private static void testArgsForMain() {
        String[] r = TinycArgs.forMain("/data/app/lib/libtinyc.so",
                new String[]{"-o", "a.o", "a.c"});
        check("forMain inserts kernel path as argv[0]",
                r.length == 4 && "/data/app/lib/libtinyc.so".equals(r[0])
                        && "-o".equals(r[1]) && "a.o".equals(r[2]) && "a.c".equals(r[3]));

        String[] empty = TinycArgs.forMain("/k/libtinyc.so", null);
        check("forMain with null user args yields just argv[0]",
                empty.length == 1 && "/k/libtinyc.so".equals(empty[0]));

        String[] fallback = TinycArgs.forMain(null, new String[]{"--version"});
        check("forMain with null kernel path falls back to \"tinyc\"",
                fallback.length == 2 && "tinyc".equals(fallback[0])
                        && "--version".equals(fallback[1]));
    }

    private static void testArgsForRun() {
        String[] r = TinycArgs.forRun("prog", new String[]{"x", "y"});
        check("forRun keeps program name as argv[0]",
                Arrays.equals(r, new String[]{"prog", "x", "y"}));
        String[] dflt = TinycArgs.forRun(null, null);
        check("forRun defaults program name to a.out",
                dflt.length == 1 && "a.out".equals(dflt[0]));
    }

    private static void testRequireNoNulls() {
        boolean threw = false;
        try {
            TinycArgs.requireNoNulls(new String[]{"a", null});
        } catch (IllegalArgumentException e) {
            threw = true;
        }
        check("requireNoNulls rejects null elements", threw);
        TinycArgs.requireNoNulls(new String[]{"a", "b"});
        check("requireNoNulls accepts clean argv", true);
    }

    private static void testIoOfStreams() throws Exception {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        ByteArrayOutputStream err = new ByteArrayOutputStream();
        ByteArrayInputStream in = new ByteArrayInputStream(new byte[]{10, 20, 30});
        TinycIO io = TinycIO.of(out, err, in);

        byte[] hello = "hello".getBytes("UTF-8");
        int n = io.writeOut(hello);
        check("io.writeOut returns byte count", n == hello.length);
        check("io.writeOut lands in out stream",
                "hello".equals(out.toString("UTF-8")));

        byte[] bad = "bad".getBytes("UTF-8");
        io.writeErr(bad);
        check("io.writeErr lands in err stream", "bad".equals(err.toString("UTF-8")));

        byte[] buf = new byte[8];
        int rn = io.readIn(buf);
        check("io.readIn reads available bytes",
                rn == 3 && buf[0] == 10 && buf[1] == 20 && buf[2] == 30);
        int eof = io.readIn(buf);
        check("io.readIn returns 0 at EOF (not -1)", eof == 0);
    }

    private static void testIoNull() throws Exception {
        TinycIO io = TinycIO.nullIO();
        check("nullIO.writeOut consumes", io.writeOut(new byte[]{1, 2}) == 2);
        check("nullIO.writeErr consumes", io.writeErr(new byte[]{1}) == 1);
        check("nullIO.readIn is EOF", io.readIn(new byte[4]) == 0);
        io.flush();
        check("nullIO.flush is a no-op", true);
    }

    private static void testConstants() {
        // 与 vendor libtcc.h 的 TCC_OUTPUT_* 逐个对齐
        check("OUTPUT_MEMORY == 1", Tinyc.OUTPUT_MEMORY == 1);
        check("OUTPUT_EXE == 2", Tinyc.OUTPUT_EXE == 2);
        check("OUTPUT_OBJ == 3", Tinyc.OUTPUT_OBJ == 3);
        check("OUTPUT_DLL == 4", Tinyc.OUTPUT_DLL == 4);
        check("OUTPUT_PREPROCESS == 5", Tinyc.OUTPUT_PREPROCESS == 5);
    }
}
