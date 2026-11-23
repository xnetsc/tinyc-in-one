package tinyc;

/**
 * 宿主模拟探针：只触发原生文件定位并打印结果。定位失败时打印完整
 * 异常链后以 3 退出，供 run-tests.sh 的 mode C 断言错误内容。
 * 宿主经环境变量 TINYC_HOST_TRIPLE / TINYC_TARGET_TRIPLE 模拟。
 */
public final class HostProbe {

    public static void main(String[] args) {
        try {
            System.out.println("native file: " + TinyC.nativeFile());
            System.out.println("host: " + Native.HOST_TRIPLE
                    + " target: " + Native.TARGET_TRIPLE);
            System.out.println("outputs: " + TinyC.outputsJson());
        } catch (Throwable t) {
            for (Throwable c = t; c != null; c = c.getCause()) {
                System.out.println("ERR " + c.getClass().getSimpleName()
                        + ": " + c.getMessage());
            }
            System.exit(3);
        }
    }
}
