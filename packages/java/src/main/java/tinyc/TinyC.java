package tinyc;

import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.lang.invoke.MethodHandle;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

/**
 * TinyC 的 Java 绑定入口：双入口封装。
 *
 * <ul>
 *   <li>会话式 API：{@link #newSession()} 拿到 {@link TccSession}，逐步控制编译。</li>
 *   <li>命令行式入口：{@link #runCli(String...)} 调原生文件导出的
 *       {@code tinyc_main}（优先）或 {@code main}，语义与 tcc 命令一致，返回退出码。</li>
 * </ul>
 *
 * <p>原生文件定位见 {@link Native}：系统属性 {@code tinyc.native.path} 优先，
 * 否则从 Jar 的 resources 按版本解压到 {@code ~/.cache/tinyc-java/<version>/}。
 * 新会话在已知 TCC 运行时目录时自动设置 lib 路径（{@code tcc_set_lib_path}），
 * 显式再调 {@link TccSession#setLibPath} 可覆盖。
 *
 * <p>运行时需要 {@code --enable-native-access=ALL-UNNAMED}（JDK 24+ 对受限方法的要求）。
 */
public final class TinyC {

    /** 输出到内存，配合 relocate / run 使用。 */
    public static final int TCC_OUTPUT_MEMORY = 1;
    /** 输出可执行文件。 */
    public static final int TCC_OUTPUT_EXE = 2;
    /** 输出目标文件。 */
    public static final int TCC_OUTPUT_OBJ = 3;
    /** 输出动态库。 */
    public static final int TCC_OUTPUT_DLL = 4;
    /** 只预处理。 */
    public static final int TCC_OUTPUT_PREPROCESS = 5;

    private static final Object CLI_LOCK = new Object();

    private TinyC() {
    }

    /** 实际加载的原生文件路径。 */
    public static Path nativeFile() {
        return Native.FILE;
    }

    /** TCC 运行时目录（含 libtcc1.a 与 include/）；无从得知时为 null。 */
    public static Path runtimeDir() {
        return Native.RUNTIME_DIR;
    }

    /**
     * 当前目标能生成的输出格式：packaging-manifest.json 里已解析组合的
     * {@code outputs} 字段原始 JSON 片段。该字段以 dist manifest 为唯一
     * 权威、各包逐字复制，本类只读不另定义；开发模式或清单缺该字段时
     * 返回 null。
     */
    public static String outputsJson() {
        return Native.OUTPUTS_JSON;
    }

    /** 原生文件是否导出了 main 入口（runCli 可用性）。 */
    public static boolean hasCli() {
        return Native.MAIN != null;
    }

    /** 新建一个会话。已知运行时目录时自动 {@code tcc_set_lib_path}。 */
    public static TccSession newSession() {
        MemorySegment state;
        try {
            state = (MemorySegment) Native.TCC_NEW.invokeExact();
        } catch (Throwable t) {
            throw new TccException("tcc_new failed", t);
        }
        if (state == null || state.address() == 0) {
            throw new TccException("tcc_new returned NULL");
        }
        TccSession session = new TccSession(state);
        if (Native.RUNTIME_DIR != null) {
            session.setLibPath(Native.RUNTIME_DIR.toString());
        }
        return session;
    }

    /**
     * 以命令行方式运行一次编译，参数与 tcc 命令行一致（不含程序名），返回退出码。
     *
     * <p>封装层规则：{@code argv[0]} 由本方法填原生文件的真实路径（TCC 按可执行文件
     * 位置找运行时）；已知运行时目录且参数里没有 {@code -B} 时，自动在最前面补
     * {@code -B<运行时目录>}。main 按「一个进程跑一次」写成，本方法用锁保证串行，
     * 调用返回后冲刷 C stdio。调用的是同一个原生文件导出的真 main，
     * 它返回即返回，不结束宿主进程。
     */
    public static int runCli(String... args) {
        if (Native.MAIN == null) {
            throw new TccException("native file exports neither tinyc_main nor main: " + Native.FILE);
        }
        synchronized (CLI_LOCK) {
            List<String> cli = new ArrayList<>();
            boolean hasB = false;
            for (String a : args) {
                if (a.equals("-B") || a.startsWith("-B")) {
                    hasB = true;
                    break;
                }
            }
            if (Native.RUNTIME_DIR != null && !hasB) {
                cli.add("-B" + Native.RUNTIME_DIR);
            }
            cli.addAll(List.of(args));

            try (Arena arena = Arena.ofConfined()) {
                MemorySegment argv = arena.allocate(ValueLayout.ADDRESS, (long) cli.size() + 2);
                argv.setAtIndex(ValueLayout.ADDRESS, 0, Native.cstr(arena, Native.FILE.toString()));
                for (int i = 0; i < cli.size(); i++) {
                    argv.setAtIndex(ValueLayout.ADDRESS, i + 1, Native.cstr(arena, cli.get(i)));
                }
                int argc = cli.size() + 1;
                int rc = (int) Native.MAIN.invokeExact(argc, argv);
                Native.flushStdio();
                return rc;
            } catch (TccException e) {
                throw e;
            } catch (Throwable t) {
                throw new TccException("calling " + Native.MAIN_NAME + " failed", t);
            }
        }
    }

    /**
     * 把取到的符号地址做成可调用的 MethodHandle，签名由调用方按 C 函数原型给出。
     * 例：{@code functionHandle(sym, FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT))}。
     */
    public static MethodHandle functionHandle(MemorySegment symbol, FunctionDescriptor descriptor) {
        return Linker.nativeLinker().downcallHandle(symbol, descriptor);
    }
}
