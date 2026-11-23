package com.tinyc;

/**
 * argv 组装的纯逻辑部分，不依赖 Android、不依赖原生库，JVM 上可直接测试。
 *
 * <p>规格 4.3 第 2 条：库模式调 main 入口时，{@code argv[0]} 由封装层填双形态
 * 文件自身的真实路径，用户只给 {@code argv[1]} 起的参数。TCC 按可执行文件
 * 位置查找 libtcc1.a 与自带头文件，argv[0] 填错会去错目录找 sysroot。</p>
 */
final class TinycArgs {

    private TinycArgs() { }

    /**
     * 为 main 入口组装 argv：在用户参数前插入内核路径作为 argv[0]。
     * kernelPath 为 null 或空时用 "tinyc" 占位（未初始化时的降级行为，
     * 此时 TCC 能否找到 sysroot 取决于调用方是否另行设置了 lib 路径）。
     */
    static String[] forMain(String kernelPath, String[] userArgs) {
        String[] args = (userArgs != null) ? userArgs : new String[0];
        String[] argv = new String[args.length + 1];
        argv[0] = (kernelPath != null && !kernelPath.isEmpty()) ? kernelPath : "tinyc";
        System.arraycopy(args, 0, argv, 1, args.length);
        return argv;
    }

    /**
     * 为 tcc_run 组装 argv：argv[0] 是被运行程序自己的名字（programName），
     * 与 main 入口的 argv[0] 语义不同，不做内核路径替换。
     */
    static String[] forRun(String programName, String[] progArgs) {
        String[] args = (progArgs != null) ? progArgs : new String[0];
        String[] argv = new String[args.length + 1];
        argv[0] = (programName != null && !programName.isEmpty()) ? programName : "a.out";
        System.arraycopy(args, 0, argv, 1, args.length);
        return argv;
    }

    /** 校验参数数组里没有 null 元素，原生层逐个取 UTF-8 字符串时 null 会直接崩。 */
    static void requireNoNulls(String[] argv) {
        for (int i = 0; i < argv.length; i++) {
            if (argv[i] == null) {
                throw new IllegalArgumentException("argv[" + i + "] is null");
            }
        }
    }
}
