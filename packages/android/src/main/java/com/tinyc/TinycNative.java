package com.tinyc;

/**
 * JNI 声明层：全部 native 方法在此集中声明，实现在 jni/tinyc_jni.c，
 * 编入双形态文件的安卓变体 libtinyc-&lt;target&gt;.so（规格 7.4：JNI 层只进安卓变体）。
 *
 * <p>方法与 libtcc 的 {@code tcc_*} 一一对应；{@code nativeRunCli} 对应
 * 双形态文件的 {@code tinyc_main} 入口。TCCState 以 long 句柄（原生指针）传递。</p>
 *
 * <p>本类不含任何 android.* 依赖，可在 JVM 上独立编译校验。</p>
 */
final class TinycNative {

    private static String loadedTarget = null;

    /**
     * 加载当前设备 ABI 对应目标的 .so（见 {@link TinycAssets#tripleForCurrentAbi()}）。
     * 若进程里已加载了别的目标的 .so，直接保持（一个进程只能有一个目标，
     * 见 {@link #ensureLoaded(String)}），不抛错——Tinyc.create() 走这条路，
     * 调用者不该因为此前已选了交叉目标而建不出会话。
     */
    static synchronized void ensureLoaded() {
        if (loadedTarget != null) {
            return;
        }
        ensureLoaded(TinycAssets.tripleForCurrentAbi());
    }

    /**
     * 加载给定目标的 .so（libtinyc-&lt;target&gt;.so，'-' 换成 '_'）。
     * 一个进程只能加载一个目标：JNI 方法按名解析，先生效者胜——第二个
     * .so 的同名 JNI 方法不会重新绑定，调用仍会走到先加载的那个目标的
     * 编译器里。所以首次加载后换目标抛 IllegalStateException，而不是
     * 静默用错编译器。同一目标重复调用直接返回。
     */
    static synchronized void ensureLoaded(String targetTriple) {
        if (loadedTarget != null) {
            if (loadedTarget.equals(targetTriple)) {
                return;
            }
            throw new IllegalStateException(
                    "Tinyc native library already loaded for " + loadedTarget
                    + ", cannot switch to " + targetTriple
                    + " in the same process");
        }
        System.loadLibrary("tinyc-" + targetTriple.replace('-', '_'));
        loadedTarget = targetTriple;
    }

    private TinycNative() { }

    // ---- 生命周期 ----
    static native long nativeNew();
    static native void nativeDelete(long state);

    // ---- 配置 ----
    static native void nativeSetLibPath(long state, String path);
    static native void nativeSetErrorListener(long state, Tinyc.ErrorListener listener);
    static native int nativeSetOptions(long state, String options);
    static native int nativeSetOutputType(long state, int outputType);
    static native void nativeSetIO(long state, TinycIO io);

    // ---- 预处理 ----
    static native int nativeAddIncludePath(long state, String pathname);
    static native int nativeAddSysincludePath(long state, String pathname);
    static native void nativeDefineSymbol(long state, String sym, String value);
    static native void nativeUndefineSymbol(long state, String sym);

    // ---- 编译 ----
    static native int nativeAddFile(long state, String filename);
    static native int nativeCompileString(long state, String source);

    // ---- 链接与输出 ----
    static native int nativeAddLibraryPath(long state, String pathname);
    static native int nativeAddLibrary(long state, String libraryName);
    static native int nativeOutputFile(long state, String filename);
    static native int nativeRelocate(long state);
    static native long nativeGetSymbol(long state, String name);
    static native int nativeRun(long state, String[] argv);

    // ---- main 入口（命令行语义，argv[0] 已由 Java 层填好） ----
    static native int nativeRunCli(String[] argv);
    static native int nativeRunCliWithIO(String[] argv, TinycIO io);
}
