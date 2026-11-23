package tinyc;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Consumer;

/**
 * 会话式 API：持有一个 TCCState，逐步控制编译过程，对应 libtcc 的 {@code tcc_*} 函数。
 *
 * <p>默认不设错误回调，libtcc 的报错走进程真实 stderr，与命令行一致。
 * 调 {@link #setErrorHandler} 或 {@link #setIO} 后，错误文本改走回调。
 * 回调里抛出的异常被跳板抓住存下，原生调用返回后由本类以 {@link TccException} 重抛。
 *
 * <p>用完必须 {@link #close()}（{@code tcc_delete}）。回调 stub 所在的 Arena
 * 由本类钉住到 close 为止。
 */
public final class TccSession implements AutoCloseable {

    private final MemorySegment state;
    private final AtomicReference<Throwable> callbackError = new AtomicReference<>();
    private Arena callbackArena;
    private TccIO io;
    private boolean closed;

    TccSession(MemorySegment state) {
        this.state = state;
    }

    private void ensureOpen() {
        if (closed) {
            throw new IllegalStateException("session already closed");
        }
    }

    private void checkCallbackError() {
        Throwable t = callbackError.getAndSet(null);
        if (t != null) {
            throw new TccException("IO/error callback threw", t);
        }
    }

    private int invokeInt(NativeCall call, String what) {
        ensureOpen();
        try {
            int rc = call.run();
            checkCallbackError();
            return rc;
        } catch (TccException e) {
            throw e;
        } catch (Throwable t) {
            throw new TccException("native call failed: " + what, t);
        }
    }

    @FunctionalInterface
    private interface NativeCall {
        int run() throws Throwable;
    }

    // ---- 配置 ----

    /** 设置 TCC 运行时目录（含 libtcc1.a 与 include/）。对应 {@code tcc_set_lib_path}。 */
    public void setLibPath(String path) {
        ensureOpen();
        try (Arena a = Arena.ofConfined()) {
            Native.TCC_SET_LIB_PATH.invokeExact(state, Native.cstr(a, path));
        } catch (Throwable t) {
            throw new TccException("tcc_set_lib_path failed", t);
        }
        checkCallbackError();
    }

    /** 按命令行方式设置选项，如 {@code "-g -Wall"}。对应 {@code tcc_set_options}。 */
    public int setOptions(String options) {
        return invokeInt(() -> {
            try (Arena a = Arena.ofConfined()) {
                return (int) Native.TCC_SET_OPTIONS.invokeExact(state, Native.cstr(a, options));
            }
        }, "tcc_set_options");
    }

    /** 加头文件目录，先于系统路径搜索。对应 {@code tcc_add_include_path}。 */
    public int addIncludePath(String path) {
        return invokeInt(() -> {
            try (Arena a = Arena.ofConfined()) {
                return (int) Native.TCC_ADD_INCLUDE_PATH.invokeExact(state, Native.cstr(a, path));
            }
        }, "tcc_add_include_path");
    }

    /** 加系统头文件目录。对应 {@code tcc_add_sysinclude_path}。 */
    public int addSysincludePath(String path) {
        return invokeInt(() -> {
            try (Arena a = Arena.ofConfined()) {
                return (int) Native.TCC_ADD_SYSINCLUDE_PATH.invokeExact(state, Native.cstr(a, path));
            }
        }, "tcc_add_sysinclude_path");
    }

    /** 加库搜索目录，对应 {@code -L}。 */
    public int addLibraryPath(String path) {
        return invokeInt(() -> {
            try (Arena a = Arena.ofConfined()) {
                return (int) Native.TCC_ADD_LIBRARY_PATH.invokeExact(state, Native.cstr(a, path));
            }
        }, "tcc_add_library_path");
    }

    /** 加库名，对应 {@code -l}。 */
    public int addLibrary(String libraryName) {
        return invokeInt(() -> {
            try (Arena a = Arena.ofConfined()) {
                return (int) Native.TCC_ADD_LIBRARY.invokeExact(state, Native.cstr(a, libraryName));
            }
        }, "tcc_add_library");
    }

    /** 直接加一个文件（.a / .o / .so / .dll / .lib / .def），不走 -l 搜索。对应 {@code tcc_add_file} 的加库用法。 */
    public int addFile(String filename) {
        return invokeInt(() -> {
            try (Arena a = Arena.ofConfined()) {
                return (int) Native.TCC_ADD_FILE.invokeExact(state, Native.cstr(a, filename));
            }
        }, "tcc_add_file");
    }

    /** 定义预处理符号，value 可为 null。对应 {@code tcc_define_symbol}。 */
    public void defineSymbol(String sym, String value) {
        ensureOpen();
        try (Arena a = Arena.ofConfined()) {
            Native.TCC_DEFINE_SYMBOL.invokeExact(state, Native.cstr(a, sym),
                    value == null ? MemorySegment.NULL : Native.cstr(a, value));
        } catch (Throwable t) {
            throw new TccException("tcc_define_symbol failed", t);
        }
        checkCallbackError();
    }

    /** 取消定义预处理符号。对应 {@code tcc_undefine_symbol}。 */
    public void undefineSymbol(String sym) {
        ensureOpen();
        try (Arena a = Arena.ofConfined()) {
            Native.TCC_UNDEFINE_SYMBOL.invokeExact(state, Native.cstr(a, sym));
        } catch (Throwable t) {
            throw new TccException("tcc_undefine_symbol failed", t);
        }
        checkCallbackError();
    }

    /** 给编译产物加一个宿主符号。对应 {@code tcc_add_symbol}。 */
    public int addSymbol(String name, MemorySegment value) {
        return invokeInt(() -> {
            try (Arena a = Arena.ofConfined()) {
                return (int) Native.TCC_ADD_SYMBOL.invokeExact(state, Native.cstr(a, name), value);
            }
        }, "tcc_add_symbol");
    }

    // ---- 编译与输出 ----

    /** 设置输出类型，必须在任何编译之前调用。对应 {@code tcc_set_output_type}，取值见 {@link TinyC} 的 TCC_OUTPUT_* 常量。 */
    public int setOutputType(int outputType) {
        return invokeInt(() -> (int) Native.TCC_SET_OUTPUT_TYPE.invokeExact(state, outputType),
                "tcc_set_output_type");
    }

    /** 编译一段 C 源码字符串。成功返回 0，失败返回 -1。对应 {@code tcc_compile_string}。 */
    public int compileString(String source) {
        return invokeInt(() -> {
            try (Arena a = Arena.ofConfined()) {
                return (int) Native.TCC_COMPILE_STRING.invokeExact(state, Native.cstr(a, source));
            }
        }, "tcc_compile_string");
    }

    /**
     * 编译一个 C 源文件。libtcc.h 没有 {@code tcc_compile_file}，
     * 本方法由 {@code tcc_add_file} 实现，与其语义一致。成功返回 0，失败返回 -1。
     */
    public int compileFile(String filename) {
        return addFile(filename);
    }

    /** 输出可执行文件、库或目标文件。调用前不要 relocate。对应 {@code tcc_output_file}。 */
    public int outputFile(String filename) {
        return invokeInt(() -> {
            try (Arena a = Arena.ofConfined()) {
                return (int) Native.TCC_OUTPUT_FILE.invokeExact(state, Native.cstr(a, filename));
            }
        }, "tcc_output_file");
    }

    /** 完成全部重定位，之后可用 {@link #getSymbol} 取符号。成功返回 0。对应 {@code tcc_relocate}。 */
    public int relocate() {
        if (Native.TCC_RELOCATE == null) {
            throw new TccException("tcc_relocate is not available for target "
                    + Native.TARGET_TRIPLE + " on host " + Native.HOST_TRIPLE
                    + ": in-process relocation/run requires a native (host == target)"
                    + " combination; cross combinations compile to files"
                    + " (outputFile / runCli)");
        }
        return invokeInt(() -> (int) Native.TCC_RELOCATE.invokeExact(state), "tcc_relocate");
    }

    /** 取编译产物中的符号地址；不存在时返回 null。对应 {@code tcc_get_symbol}。 */
    public MemorySegment getSymbol(String name) {
        ensureOpen();
        try (Arena a = Arena.ofConfined()) {
            MemorySegment seg = (MemorySegment) Native.TCC_GET_SYMBOL.invokeExact(state, Native.cstr(a, name));
            checkCallbackError();
            return seg == null || seg.address() == 0 ? null : seg;
        } catch (Throwable t) {
            throw new TccException("tcc_get_symbol failed", t);
        }
    }

    /**
     * 链接并在当前进程内运行编译产物的 main，返回其返回值。对应 {@code tcc_run}。
     * 目标程序在宿主进程内运行，它的 exit 与崩溃归宿主，语义与命令行 {@code -run} 相同。
     * {@code args[0]} 是目标程序的 argv[0]。
     */
    public int run(String... args) {
        if (Native.TCC_RUN == null) {
            throw new TccException("tcc_run is not available for target "
                    + Native.TARGET_TRIPLE + " on host " + Native.HOST_TRIPLE
                    + ": in-process relocation/run requires a native (host == target)"
                    + " combination; cross combinations compile to files"
                    + " (outputFile / runCli)");
        }
        return invokeInt(() -> {
            try (Arena a = Arena.ofConfined()) {
                MemorySegment argv = a.allocate(ValueLayout.ADDRESS, (long) args.length + 1);
                for (int i = 0; i < args.length; i++) {
                    argv.setAtIndex(ValueLayout.ADDRESS, i, Native.cstr(a, args[i]));
                }
                return (int) Native.TCC_RUN.invokeExact(state, args.length, argv);
            }
        }, "tcc_run");
    }

    // ---- 错误回调与 IO ----

    private void installErrorCallback(TccIO callbackIo) {
        ensureOpen();
        if (callbackArena == null) {
            callbackArena = Arena.ofConfined();
        }
        MemorySegment stub = TccIoUpcalls.errorFuncStub(callbackIo, callbackArena, callbackError);
        try {
            Native.TCC_SET_ERROR_FUNC.invokeExact(state, MemorySegment.NULL, stub);
        } catch (Throwable t) {
            throw new TccException("tcc_set_error_func failed", t);
        }
        this.io = callbackIo;
    }

    /** 设置错误处理器：libtcc 的每条错误/警告文本交给 handler。对应 {@code tcc_set_error_func}。 */
    public void setErrorHandler(Consumer<String> handler) {
        installErrorCallback(new TccIO() {
            @Override
            public int writeOut(byte[] data) {
                return data.length;
            }

            @Override
            public int writeErr(byte[] data) {
                String text = new String(data, StandardCharsets.UTF_8);
                if (text.endsWith("\n")) {
                    text = text.substring(0, text.length() - 1);
                }
                handler.accept(text);
                return data.length;
            }

            @Override
            public int readIn(byte[] buf) {
                return 0;
            }
        });
    }

    /**
     * 设置 IO：当前接入的是错误通道，libtcc 错误文本经 upcall 跳板以字节
     * 交给 {@code io.writeErr}。writeOut / readIn 的跳板见 {@link TccIoUpcalls}，
     * 待内核提供 {@code tcc_set_io} 后接入；在此之前正常输出走进程真实 stdio。
     */
    public void setIO(TccIO io) {
        installErrorCallback(io);
    }

    /** 当前设置的 IO，未设置时为 null。 */
    public TccIO io() {
        return io;
    }

    // ---- 生命周期 ----

    @Override
    public void close() {
        if (closed) {
            return;
        }
        closed = true;
        try {
            Native.TCC_DELETE.invokeExact(state);
        } catch (Throwable t) {
            throw new TccException("tcc_delete failed", t);
        } finally {
            if (callbackArena != null) {
                callbackArena.close();
                callbackArena = null;
            }
        }
    }
}
