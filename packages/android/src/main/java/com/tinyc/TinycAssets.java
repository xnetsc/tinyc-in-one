package com.tinyc;

import android.content.Context;
import android.content.res.AssetManager;
import android.os.Build;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * AAR 内置数据的初始化：assets 里的 toolchain / sysroot 不是真实文件，
 * TCC 读不了，首次使用时解压到应用 filesDir 下按版本分目录（规格 7.4）。
 *
 * <p>本类是包内唯一依赖 android.* 的类，其余类可在 JVM 上独立编译与测试。</p>
 *
 * <p>典型用法（在 Application 或首个使用点调一次）：</p>
 * <pre>{@code
 * TinycAssets.Paths paths = TinycAssets.init(context);
 * try (Tinyc t = Tinyc.create(paths)) { ... }
 * }</pre>
 */
public final class TinycAssets {

    /** assets 解压版本号。assets 内容变化时改这个值，旧目录不覆盖、并行保留。 */
    public static final String ASSETS_VERSION = "2";

    private static final String ROOT_DIR_NAME = "tinyc";

    private TinycAssets() { }

    /** 解压后的路径集合。 */
    public static final class Paths {
        /** 解压根目录：filesDir/tinyc/&lt;version&gt;/&lt;triple&gt; */
        public final File rootDir;
        /** 目标 triple，如 aarch64-linux-android */
        public final String triple;
        /** toolchain 目录：TCC 自带头与 libtcc1.a，作为 tcc_set_lib_path 的值 */
        public final File toolchainDir;
        /** sysroot 目录：其下 include/ 与 lib/ 是系统头与系统库 */
        public final File sysrootDir;

        Paths(File rootDir, String triple, File toolchainDir, File sysrootDir) {
            this.rootDir = rootDir;
            this.triple = triple;
            this.toolchainDir = toolchainDir;
            this.sysrootDir = sysrootDir;
        }
    }

    /** 当前设备首选 ABI 对应的目标 triple。 */
    public static String tripleForCurrentAbi() {
        String abi = (Build.SUPPORTED_ABIS != null && Build.SUPPORTED_ABIS.length > 0)
                ? Build.SUPPORTED_ABIS[0] : "arm64-v8a";
        return tripleForAbi(abi);
    }

    static String tripleForAbi(String abi) {
        switch (abi) {
            case "arm64-v8a":
                return "aarch64-linux-android";
            case "armeabi-v7a":
                return "arm-linux-androideabi";
            case "x86_64":
                return "x86_64-linux-android";
            case "x86":
                return "i686-linux-android";
            default:
                return "aarch64-linux-android";
        }
    }

    /**
     * 初始化：把当前 ABI 对应 triple 的 toolchain 与 sysroot 从 assets 解压到
     * filesDir（已解压且标记完整时直接复用），记录双形态文件路径供
     * {@link Tinyc#runCli(String...)} 填 argv[0]，返回路径集合。
     */
    public static Paths init(Context context) throws IOException {
        String triple = tripleForCurrentAbi();
        File base = new File(context.getFilesDir(), ROOT_DIR_NAME);
        File root = new File(new File(base, ASSETS_VERSION), triple);
        File toolchainDir = new File(root, "toolchain");
        File sysrootDir = new File(root, "sysroot");
        File marker = new File(root, ".extracted-" + ASSETS_VERSION);

        if (!marker.exists()) {
            AssetManager am = context.getAssets();
            extractTree(am, "toolchain/" + triple, toolchainDir);
            extractTree(am, "sysroot/" + triple, sysrootDir);
            // 标记最后写：中途被杀重启时会重新解压，不会把半成品当完整数据用
            try (OutputStream out = new FileOutputStream(marker)) {
                out.write(ASSETS_VERSION.getBytes("UTF-8"));
            }
        }

        String nativeDir = context.getApplicationInfo().nativeLibraryDir;
        if (nativeDir != null) {
            Tinyc.setKernelPath(new File(nativeDir, "libtinyc.so").getAbsolutePath());
        }
        return new Paths(root, triple, toolchainDir, sysrootDir);
    }

    /** 递归解压 assets 的一棵目录树到目标目录。assets 里只有占位文件时解出的也是占位布局。 */
    private static void extractTree(AssetManager am, String assetPath, File destDir)
            throws IOException {
        String[] children = am.list(assetPath);
        if (children == null || children.length == 0) {
            // 叶子：当作文件拷贝（也可能是空目录，open 会失败，忽略即可）
            copyAssetFile(am, assetPath, destDir);
            return;
        }
        if (!destDir.exists() && !destDir.mkdirs()) {
            throw new IOException("cannot create dir " + destDir);
        }
        for (String child : children) {
            extractTree(am, assetPath + "/" + child, new File(destDir, child));
        }
    }

    private static void copyAssetFile(AssetManager am, String assetPath, File destFile)
            throws IOException {
        File parent = destFile.getParentFile();
        if (parent != null && !parent.exists()) {
            parent.mkdirs();
        }
        try (InputStream in = am.open(assetPath);
             OutputStream out = new FileOutputStream(destFile)) {
            byte[] buf = new byte[8192];
            int n;
            while ((n = in.read(buf)) != -1) {
                out.write(buf, 0, n);
            }
        } catch (IOException e) {
            // 空目录在 assets 里 list 不到子项时可能走到这里：没有可拷贝的文件就跳过
            if (destFile.length() == 0 && !destFile.exists()) {
                return;
            }
            throw e;
        }
    }
}
