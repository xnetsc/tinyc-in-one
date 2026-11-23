package com.tinyc;

/** TinyC 操作失败时抛出：编译、链接、输出文件等原生调用返回错误时使用。 */
public class TinycException extends RuntimeException {
    public TinycException(String message) {
        super(message);
    }

    public TinycException(String message, Throwable cause) {
        super(message, cause);
    }
}
