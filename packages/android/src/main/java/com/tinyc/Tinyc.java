package com.tinyc;

import java.io.File;

/**
 * TinyC 会话式 API，语义与 libtcc 的 {@code tcc_*} 一一对应（规格第 4 节）。
 *
 * <p>典型用法：</p>
 * <pre>{@code
 * try (Tinyc t = Tinyc.create()) {
 *     t.setOutputType(Tinyc.OUTPUT_OBJ);
 *     t.addIncludePath("/data/data/com.example/files/mylib/include");
 *     t.compileString("int answer(void) { return 42; }");
 *     t.outputFile(new File(dir, "answer.o").getAbsolutePath());
 * }
 * }</pre>
 *
 * <p>命令行语义的一次性用法走静态方法 {@link #runCli(String...)}，对应
 * 双形态文件的 {@code tinyc_main} 入口：参数解析、报错、输出与命令行逐字
 * 一致，返回退出码。runCli 内部串行（规格 4.3 第 3 条），不可多线程同时调用；
 * 会话式 API 的单个实例同样不是线程安全的，不要跨线程共用一个实例。</p>
 *
 * <p>注意 {@code -run} 语义：{@link #run(String, String...)} 是把编译出的
 * 代码在本进程内直接运行，目标程序的 exit 与崩溃都落到宿主进程头上。</p>
 */
public final class Tinyc implements AutoCloseable {

    // 与 libtcc.h 的 TCC_OUTPUT_* 一致
    public static final int OUTPUT_MEMORY = 1;
    public static final int OUTPUT_EXE = 2;
    public static final int OUTPUT_OBJ = 3;
    public static final int OUTPUT_DLL = 4;
    public static final int OUTPUT_PREPROCESS = 5;

    /** 编译错误 / 警告回调，对应 tcc_set_error_func。msg 已包含 "file:line: error: ..." 格式。 */
    public interface ErrorListener {
        void onError(String msg);
    }

    private static final Object CLI_LOCK = new Object();
    private static volatile String kernelPath; // 双形态文件真实路径，runCli 用作 argv[0]

    private long state;
    private TinycIO io;              // 强引用钉住回调，直到 close（规格 6.5 第 3 条）
    private ErrorListener errorListener; // 同上
    private TinycAssets.Paths bundledPaths;
    private boolean sysrootApplied;

    private Tinyc(long state) {
        this.state = state;
    }

    /** 创建一个编译会话，对应 tcc_new。 */
    public static Tinyc create() {
        TinycNative.ensureLoaded();
        long s = TinycNative.nativeNew();
        if (s == 0) {
            throw new TinycException("tcc_new returned null");
        }
        return new Tinyc(s);
    }

    /**
     * 创建会话并绑定 AAR 内置的 toolchain / sysroot（见 TinycAssets）。
     * sysroot 的系统头与库路径在第一次编译动作前才注册到原生层，
     * 保证此前 addIncludePath 等用户路径排在 sysroot 之前（规格 5.2）。
     */
    public static Tinyc create(TinycAssets.Paths paths) {
        Tinyc t = create();
        t.useBundledPaths(paths);
        return t;
    }

    /** 记录内置路径：lib 路径（toolchain 目录）立即设置，其余延迟到首次编译前注册。 */
    public void useBundledPaths(TinycAssets.Paths paths) {
        this.bundledPaths = paths;
        if (paths != null) {
            TinycNative.nativeSetLibPath(state, paths.toolchainDir.getAbsolutePath());
        }
    }

    private void applySysrootIfNeeded() {
        if (bundledPaths != null && !sysrootApplied) {
            sysrootApplied = true;
            TinycNative.nativeAddSysincludePath(state,
                    new File(bundledPaths.sysrootDir, "include").getAbsolutePath());
            TinycNative.nativeAddLibraryPath(state,
                    new File(bundledPaths.sysrootDir, "lib").getAbsolutePath());
        }
    }

    private void checkOpen() {
        if (state == 0) {
            throw new IllegalStateException("Tinyc already closed");
        }
    }

    private static void checkResult(String op, int rc) {
        if (rc == -1) {
            throw new TinycException(op + " failed");
        }
    }

    // ---- 配置 ----

    /** 对应 tcc_set_lib_path：设置 TCC 自带头与 libtcc1.a 所在目录（toolchain 目录）。 */
    public void setLibPath(String path) {
        checkOpen();
        TinycNative.nativeSetLibPath(state, path);
    }

    /** 对应 tcc_set_error_func。listener 为 null 时回到默认去向（IO 的 err 通道）。 */
    public void setErrorListener(ErrorListener listener) {
        checkOpen();
        this.errorListener = listener;
        TinycNative.nativeSetErrorListener(state, listener);
    }

    /**
     * 设置 IO 通道（规格第 6 节）。io 为 null 时恢复默认 stdio 行为。
     * 封装层持有 io 的强引用直到 close，回调不会被 GC 提前回收。
     */
    public void setIO(TinycIO io) {
        checkOpen();
        this.io = io;
        TinycNative.nativeSetIO(state, io);
    }

    /** 对应 tcc_set_options：一次传多个命令行风格选项，如 "-nostdlib -Wall"。 */
    public void setOptions(String options) {
        checkOpen();
        checkResult("tcc_set_options", TinycNative.nativeSetOptions(state, options));
    }

    /** 对应 tcc_set_output_type，必须在任何编译动作之前调用。 */
    public void setOutputType(int outputType) {
        checkOpen();
        checkResult("tcc_set_output_type", TinycNative.nativeSetOutputType(state, outputType));
    }

    // ---- 动态添加：本地真实路径（规格 5.3） ----

    /** 对应 tcc_add_include_path。用户头目录走这个接口，排在系统路径之前，可遮蔽 sysroot 同名头。 */
    public int addIncludePath(String pathname) {
        checkOpen();
        return TinycNative.nativeAddIncludePath(state, pathname);
    }

    /** 对应 tcc_add_sysinclude_path。系统头语义，排在 include 路径之后。 */
    public int addSysincludePath(String pathname) {
        checkOpen();
        return TinycNative.nativeAddSysincludePath(state, pathname);
    }

    /** 对应 tcc_add_library_path，等价命令行 -L。 */
    public int addLibraryPath(String pathname) {
        checkOpen();
        return TinycNative.nativeAddLibraryPath(state, pathname);
    }

    /** 对应 tcc_add_library，等价命令行 -l，参数是不带 lib 前缀与扩展名的库名。 */
    public int addLibrary(String libraryName) {
        checkOpen();
        return TinycNative.nativeAddLibrary(state, libraryName);
    }

    /**
     * 对应 tcc_add_file：直接给文件路径，不走 -l 搜索。
     * .c 源文件、.o、.a、.so 均可。编译源文件时也走这个接口。
     */
    public int addFile(String filename) {
        checkOpen();
        applySysrootIfNeeded();
        return TinycNative.nativeAddFile(state, filename);
    }

    /** 对应 tcc_define_symbol。value 可为 null。sym 也可写成 "NAME=value" 形式。 */
    public void defineSymbol(String sym, String value) {
        checkOpen();
        TinycNative.nativeDefineSymbol(state, sym, value);
    }

    /** 对应 tcc_undefine_symbol。 */
    public void undefineSymbol(String sym) {
        checkOpen();
        TinycNative.nativeUndefineSymbol(state, sym);
    }

    // ---- 编译 ----

    /** 对应 tcc_compile_string：直接编译内存中的源码。失败抛 TinycException。 */
    public void compileString(String source) {
        checkOpen();
        applySysrootIfNeeded();
        checkResult("tcc_compile_string", TinycNative.nativeCompileString(state, source));
    }

    /** 编译一个源文件，等价于对该文件调 tcc_add_file。失败抛 TinycException。 */
    public void compileFile(String filename) {
        checkOpen();
        applySysrootIfNeeded();
        checkResult("tcc_add_file(" + filename + ")", TinycNative.nativeAddFile(state, filename));
    }

    // ---- 链接与输出 ----

    /** 对应 tcc_output_file：把产物（exe / dll / obj）落成文件。调用前不要 relocate。 */
    public void outputFile(String filename) {
        checkOpen();
        applySysrootIfNeeded();
        checkResult("tcc_output_file", TinycNative.nativeOutputFile(state, filename));
    }

    /** 对应 tcc_relocate：完成重定位，之后可用 getSymbol 取符号。 */
    public void relocate() {
        checkOpen();
        applySysrootIfNeeded();
        checkResult("tcc_relocate", TinycNative.nativeRelocate(state));
    }

    /** 对应 tcc_get_symbol：返回符号地址，未找到返回 0。需先 relocate。 */
    public long getSymbol(String name) {
        checkOpen();
        return TinycNative.nativeGetSymbol(state, name);
    }

    /**
     * 对应 tcc_run：在本进程内运行已编译程序的 main，返回它的返回值。
     * 目标程序的 exit 与崩溃归宿主进程，见类注释。
     */
    public int run(String programName, String... progArgs) {
        checkOpen();
        applySysrootIfNeeded();
        return TinycNative.nativeRun(state, TinycArgs.forRun(programName, progArgs));
    }

    // ---- main 入口（命令行语义） ----

    /**
     * 以命令行语义运行一次完整编译，对应 tinyc_main(argc, argv)。
     * args 是 argv[1] 起的用户参数，argv[0] 由本层填双形态文件真实路径
     * （规格 4.3 第 2 条，经 TinycAssets.init 时记录；未初始化时退化为 "tinyc"）。
     * 返回退出码：0 成功，非 0 失败——本方法不抛异常表达编译失败。
     * 全进程串行：多线程同时调用会互相等待。
     */
    public static int runCli(String... args) {
        TinycNative.ensureLoaded();
        String[] argv = TinycArgs.forMain(kernelPath, args);
        TinycArgs.requireNoNulls(argv);
        synchronized (CLI_LOCK) {
            return TinycNative.nativeRunCli(argv);
        }
    }

    /** 带 IO 的 runCli：输出走给定 IO 而不是默认 stdio（规格第 6 节）。io 为 null 时同 runCli。 */
    public static int runCli(TinycIO io, String... args) {
        if (io == null) {
            return runCli(args);
        }
        TinycNative.ensureLoaded();
        String[] argv = TinycArgs.forMain(kernelPath, args);
        TinycArgs.requireNoNulls(argv);
        synchronized (CLI_LOCK) {
            return TinycNative.nativeRunCliWithIO(argv, io);
        }
    }

    /** 由 TinycAssets.init 调用，记录双形态文件路径供 runCli 填 argv[0]。 */
    static void setKernelPath(String path) {
        kernelPath = path;
    }

    /** 当前记录的双形态文件路径，未初始化时为 null。 */
    public static String getKernelPath() {
        return kernelPath;
    }

    // ---- 生命周期 ----

    /** 对应 tcc_delete。重复调用无副作用。 */
    @Override
    public void close() {
        if (state != 0) {
            long s = state;
            state = 0;
            TinycNative.nativeDelete(s);
            io = null;
            errorListener = null;
        }
    }
}
