package tinyc;

import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicReference;

/**
 * 把 {@link TccIO} 的 Java 方法做成 C 函数指针的 Panama upcall 跳板。
 *
 * <p>统一规矩：
 * <ul>
 *   <li>异常不穿 C 边界：回调里抛出的异常被抓住存进 {@code errSink}，
 *       写/读跳板向 C 返回 -1，错误回调（签名无返回值）只记录；
 *       原生调用返回后由 {@link TccSession} 检查并重抛。</li>
 *   <li>字节传递：写通道按 {@code (buf, len)} 拷出字节交给 Java 方法，
 *       不假设 {@code \0} 结尾。</li>
 *   <li>生命周期：stub 所在的 {@link Arena} 由调用方（会话）持有到
 *       state 释放后才关闭，本类不关闭它。</li>
 *   <li>同步执行：upcall 在发起原生调用的线程上被 C 同步唤起，
 *       Panama 已处理好线程附着。</li>
 * </ul>
 *
 * <p>当前实际接入的是错误回调跳板（{@link #errorFuncStub}），
 * 经 {@code tcc_set_error_func} 注册到 libtcc。写/读跳板
 * （{@link #writeStub}、{@link #readStub}）是 TCCIO 的
 * {@code int (void*, const char*, size_t)} 与 {@code int (void*, char*, size_t)}
 * 签名的现成映射，待内核提供 {@code tcc_set_io} 符号后接入。
 */
public final class TccIoUpcalls {

    /** C 侧 write 函数签名：int (void *ctx, const char *buf, size_t len)。 */
    public static final FunctionDescriptor WRITE_DESCRIPTOR =
            FunctionDescriptor.of(ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.JAVA_LONG);

    /** C 侧 read 函数签名：int (void *ctx, char *buf, size_t cap)。 */
    public static final FunctionDescriptor READ_DESCRIPTOR = WRITE_DESCRIPTOR;

    /** libtcc 错误回调签名：void (void *opaque, const char *msg)。 */
    public static final FunctionDescriptor ERROR_FUNC_DESCRIPTOR =
            FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS);

    private TccIoUpcalls() {
    }

    // ---- 回调目标（被 upcall 调用的 Java 方法） ----

    private static int onWrite(TccIO io, AtomicReference<Throwable> errSink,
                               boolean errChannel,
                               MemorySegment ctx, MemorySegment buf, long len) {
        try {
            byte[] data = buf.reinterpret(len).toArray(ValueLayout.JAVA_BYTE);
            return errChannel ? io.writeErr(data) : io.writeOut(data);
        } catch (Throwable t) {
            errSink.compareAndSet(null, t);
            return -1;
        }
    }

    private static int onRead(TccIO io, AtomicReference<Throwable> errSink,
                              MemorySegment ctx, MemorySegment buf, long cap) {
        try {
            byte[] tmp = new byte[Math.toIntExact(cap)];
            int n = io.readIn(tmp);
            if (n > 0) {
                MemorySegment.copy(tmp, 0, buf.reinterpret(cap), ValueLayout.JAVA_BYTE, 0, n);
            }
            return n;
        } catch (Throwable t) {
            errSink.compareAndSet(null, t);
            return -1;
        }
    }

    private static void onError(TccIO io, AtomicReference<Throwable> errSink,
                                MemorySegment opaque, MemorySegment msg) {
        try {
            // 错误消息是短文本，按 64 KiB 上界重解释后按 C 字符串读
            String text = msg.reinterpret(1 << 16).getString(0, StandardCharsets.UTF_8);
            io.writeErr((text + "\n").getBytes(StandardCharsets.UTF_8));
        } catch (Throwable t) {
            errSink.compareAndSet(null, t);
        }
    }

    // ---- stub 工厂 ----

    private static MethodHandle handle(String name, MethodType type) {
        try {
            return MethodHandles.lookup().findStatic(TccIoUpcalls.class, name, type);
        } catch (ReflectiveOperationException e) {
            throw new TccException("cannot create upcall handle " + name, e);
        }
    }

    /**
     * 写通道跳板。{@code errChannel} 为 true 时交给 {@code io.writeErr}，
     * 否则交给 {@code io.writeOut}。stub 钉在 {@code arena} 上，由调用方管理生命周期。
     */
    public static MemorySegment writeStub(boolean errChannel, TccIO io, Arena arena,
                                          AtomicReference<Throwable> errSink) {
        MethodHandle mh = handle("onWrite",
                MethodType.methodType(int.class, TccIO.class, AtomicReference.class,
                        boolean.class, MemorySegment.class, MemorySegment.class, long.class));
        mh = MethodHandles.insertArguments(mh, 0, io, errSink, errChannel);
        return Linker.nativeLinker().upcallStub(mh, WRITE_DESCRIPTOR, arena);
    }

    /** 读通道跳板，交给 {@code io.readIn}。生命周期同 {@link #writeStub}。 */
    public static MemorySegment readStub(TccIO io, Arena arena,
                                         AtomicReference<Throwable> errSink) {
        MethodHandle mh = handle("onRead",
                MethodType.methodType(int.class, TccIO.class, AtomicReference.class,
                        MemorySegment.class, MemorySegment.class, long.class));
        mh = MethodHandles.insertArguments(mh, 0, io, errSink);
        return Linker.nativeLinker().upcallStub(mh, READ_DESCRIPTOR, arena);
    }

    /**
     * libtcc 错误回调跳板：把错误文本（UTF-8）以字节交给 {@code io.writeErr}，
     * 每条消息补一个换行，与 libtcc 默认打印一致。
     */
    public static MemorySegment errorFuncStub(TccIO io, Arena arena,
                                              AtomicReference<Throwable> errSink) {
        MethodHandle mh = handle("onError",
                MethodType.methodType(void.class, TccIO.class, AtomicReference.class,
                        MemorySegment.class, MemorySegment.class));
        mh = MethodHandles.insertArguments(mh, 0, io, errSink);
        return Linker.nativeLinker().upcallStub(mh, ERROR_FUNC_DESCRIPTOR, arena);
    }
}
