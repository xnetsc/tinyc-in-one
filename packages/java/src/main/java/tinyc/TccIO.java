package tinyc;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UncheckedIOException;
import java.nio.charset.StandardCharsets;

/**
 * TinyC 的 IO 通道接口，对应内核 TCCIO 的三个通道：write_out、write_err、read_in。
 *
 * <p>数据一律以字节传递，不保证一次调用是一整行，也不保证以 {@code \0} 结尾。
 * 返回值是实际消费的字节数；返回 -1 表示出错。实现里抛出的异常不会穿过 C 边界：
 * 跳板层会抓住异常、让当前原生调用中止，并在原生调用返回后以 {@link TccException}
 * 重新抛给调用方。
 *
 * <p>接法有二：会话式 API 的诊断经 {@code tcc_set_error_func} 交给 writeErr，
 * 见 {@link TccSession#setIO(TccIO)}；命令行式入口
 * {@link TinyC#runCli(TccIO, String...)} 把三个通道的 upcall 跳板
 * （{@link TccIoUpcalls}）经内核 {@code tcc_io_callback} 组成回调型 TCCIO，
 * 传给 {@code tinyc_main_io}，在该次调用期间作为编译器的当前 IO：
 * 编译器自身的输出走 writeOut、诊断走 writeErr、readIn 随 TCCIO 一并
 * 传给内核。不带 IO 的 {@code runCli} 走进程真实 stdio，与命令行一致。
 * 带 IO 跑 {@code -run} 时，目标程序在运行期间的 stdin 由 readIn
 * 喂入，stdout 与 stderr 分别经 writeOut 与 writeErr 交付。
 */
public interface TccIO {

    /** 写标准输出通道。返回消费的字节数，-1 表示出错。 */
    int writeOut(byte[] data);

    /** 写错误通道。返回消费的字节数，-1 表示出错。 */
    int writeErr(byte[] data);

    /**
     * 从标准输入通道读数据，写入 {@code buf}，返回读到的字节数；
     * 0 表示 EOF，-1 表示出错。
     */
    int readIn(byte[] buf);

    /** 冲刷输出通道。默认无操作。 */
    default void flush() {
    }

    /**
     * Java 侧 stdio 实现：out 写 {@code System.out}，err 写 {@code System.err}，
     * in 读 {@code System.in}。
     */
    static TccIO stdio() {
        return of(System.out, System.err, System.in);
    }

    /**
     * 用流适配：任一参数为 null 时，该通道回落到对应的 {@code System} 流。
     * 流由调用方持有，适配层不关闭它们。
     */
    static TccIO of(OutputStream out, OutputStream err, InputStream in) {
        OutputStream o = out != null ? out : System.out;
        OutputStream e = err != null ? err : System.err;
        InputStream i = in != null ? in : System.in;
        return new TccIO() {
            @Override
            public int writeOut(byte[] data) {
                return write(o, data);
            }

            @Override
            public int writeErr(byte[] data) {
                return write(e, data);
            }

            @Override
            public int readIn(byte[] buf) {
                try {
                    int n = i.read(buf);
                    return Math.max(n, 0);
                } catch (IOException ex) {
                    throw new UncheckedIOException(ex);
                }
            }

            @Override
            public void flush() {
                try {
                    o.flush();
                    e.flush();
                } catch (IOException ex) {
                    throw new UncheckedIOException(ex);
                }
            }

            private static int write(OutputStream os, byte[] data) {
                try {
                    os.write(data);
                    return data.length;
                } catch (IOException ex) {
                    throw new UncheckedIOException(ex);
                }
            }
        };
    }

    /** 内存攒积实现：out、err 分开存，readIn 恒 EOF。 */
    static Capturing capturing() {
        return new Capturing();
    }

    /** {@link #capturing()} 的实现类，调用后用 getter 取走数据。 */
    final class Capturing implements TccIO {
        private final ByteArrayOutputStream outBuf = new ByteArrayOutputStream();
        private final ByteArrayOutputStream errBuf = new ByteArrayOutputStream();

        @Override
        public synchronized int writeOut(byte[] data) {
            outBuf.write(data, 0, data.length);
            return data.length;
        }

        @Override
        public synchronized int writeErr(byte[] data) {
            errBuf.write(data, 0, data.length);
            return data.length;
        }

        @Override
        public int readIn(byte[] buf) {
            return 0;
        }

        public synchronized byte[] outBytes() {
            return outBuf.toByteArray();
        }

        public synchronized byte[] errBytes() {
            return errBuf.toByteArray();
        }

        public String outText() {
            return new String(outBytes(), StandardCharsets.UTF_8);
        }

        public String errText() {
            return new String(errBytes(), StandardCharsets.UTF_8);
        }

        public synchronized void clear() {
            outBuf.reset();
            errBuf.reset();
        }
    }
}
