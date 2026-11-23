package com.tinyc;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * IO 通道接口，对应内核 TCCIO（规格 6.1 / 6.5）。
 *
 * <p>回调收到的是原始字节块：不保证以 {@code \0} 结尾，不保证一次调用是一整行。
 * 返回值是实际消费 / 读到的字节数；{@code readIn} 返回 0 表示 EOF；返回 -1 表示出错，
 * JNI 跳板会把此处抛出的异常存下、向 C 返回 -1 使当前编译中止，原生调用返回后
 * 再把原异常在 Java 侧重新抛出（规格 6.5 第 1 条：异常不穿过 C 边界）。</p>
 *
 * <p>回调在发起编译的线程上同步执行（规格 6.5 第 4 条）。</p>
 */
public interface TinycIO {

    /** 写标准输出通道（write_out）。返回消费的字节数，-1 为出错。 */
    int writeOut(byte[] buf) throws IOException;

    /** 写标准错误通道（write_err）。返回消费的字节数，-1 为出错。 */
    int writeErr(byte[] buf) throws IOException;

    /** 从标准输入通道读，最多读 buf.length 字节。返回读到的字节数，0 为 EOF，-1 为出错。 */
    int readIn(byte[] buf) throws IOException;

    /** 冲刷输出通道。 */
    void flush() throws IOException;

    /** 指向真实 stdio：System.out / System.err / System.in。这是默认行为（规格 6.4）。 */
    static TinycIO stdio() {
        return of(System.out, System.err, System.in);
    }

    /** 全丢弃：输出不去任何地方，readIn 恒 EOF。对应内核 tcc_io_null。 */
    static TinycIO nullIO() {
        OutputStream sink = new OutputStream() {
            @Override public void write(int b) { }
            @Override public void write(byte[] b, int off, int len) { }
        };
        InputStream empty = new InputStream() {
            @Override public int read() { return -1; }
        };
        return of(sink, sink, empty);
    }

    /**
     * 用流组装 IO。任一参数可为 null，表示该通道回落到对应的真实 stdio 通道
     * （规格 6.3：按通道独立，未给出的通道回落 stdio）。
     * 流是借用：Tinyc 不关闭它们。
     */
    static TinycIO of(OutputStream out, OutputStream err, InputStream in) {
        final OutputStream o = (out != null) ? out : System.out;
        final OutputStream e = (err != null) ? err : System.err;
        final InputStream i = (in != null) ? in : System.in;
        return new TinycIO() {
            @Override public int writeOut(byte[] buf) throws IOException {
                o.write(buf);
                return buf.length;
            }
            @Override public int writeErr(byte[] buf) throws IOException {
                e.write(buf);
                return buf.length;
            }
            @Override public int readIn(byte[] buf) throws IOException {
                return Math.max(i.read(buf), 0);
            }
            @Override public void flush() throws IOException {
                o.flush();
                e.flush();
            }
        };
    }
}
