package tinyc;

/** TinyC 绑定层异常：原生调用失败、原生文件缺失、IO 回调异常重抛时使用。 */
public class TccException extends RuntimeException {
    public TccException(String message) {
        super(message);
    }

    public TccException(String message, Throwable cause) {
        super(message, cause);
    }
}
