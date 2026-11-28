/*
 * tinyc_jni.c - JNI glue for the Android package (spec 7.4).
 *
 * Compiled into the Android variant of the dual-form file (libtinyc.so)
 * together with the libtcc kernel. It forwards com.tinyc.TinycNative
 * methods to tcc_* / tinyc_main, and bridges the kernel TCCIO callbacks
 * back into the Java TinycIO interface.
 *
 * Spec rules implemented here (6.5):
 *  1. Java exceptions never cross the C boundary: a callback captures the
 *     exception object, clears it, returns -1 to abort the compile; the
 *     JNI entry point rethrows the captured exception after the native
 *     call returns.
 *  2. Callbacks carry raw bytes (byte[]), never NUL-terminated strings.
 *  3. The Java IO / listener objects are held by global references in the
 *     per-state context until the state is deleted.
 *  4. Callbacks run synchronously on the calling thread; the thread is
 *     already attached when it entered JNI, AttachCurrentThread is only
 *     a fallback.
 */

#include <jni.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "libtcc.h"
#include "tinyc_kernel.h"

/* Whether this .so is a native compiler: a mirror of TCC_IS_NATIVE
 * (vendor-tinycc/tcc.h). tccrun.c is compiled only under
 * TCC_IS_NATIVE, so tcc_relocate() and tcc_run() exist only in a
 * native build; a cross-target .so has neither symbol, and a JNI
 * entry point that called them would leave the .so unloadable
 * (UnsatisfiedLinkError at dlopen: "cannot locate symbol").
 *
 * The mirror must evaluate the same way tcc.h's does: the JNI glue
 * is compiled by the same NDK clang invocation, with the same
 * -DTCC_TARGET_* defines, as the kernel in the same .so, so the two
 * agree by construction. If tcc.h's predicate ever changes, this
 * mirror must change with it; a missing guard shows up again as an
 * unloadable .so, an over-broad one as relocate()/run() failing on
 * a target that could run. (tcc.h itself is not included here: it is
 * a kernel-internal header that needs the generated config.h.)
 *
 * A cross-target .so is a compiler for another machine: it can
 * compile and write files, but it can never execute the compiled
 * code in this process, so relocate()/run() have no meaning for it
 * and fail here instead of calling a symbol that does not exist. */
#if defined _WIN32 == defined TCC_TARGET_PE \
    && defined __APPLE__ == defined TCC_TARGET_MACHO
# if defined __i386__ && defined TCC_TARGET_I386
#  define TINYC_JNI_NATIVE 1
# elif defined __x86_64__ && defined TCC_TARGET_X86_64
#  define TINYC_JNI_NATIVE 1
# elif defined __arm__ && defined TCC_TARGET_ARM
#  define TINYC_JNI_NATIVE 1
# elif defined __aarch64__ && defined TCC_TARGET_ARM64
#  define TINYC_JNI_NATIVE 1
# elif defined __riscv && defined __LP64__ && defined TCC_TARGET_RISCV64
#  define TINYC_JNI_NATIVE 1
# endif
#endif

/* ------------------------------------------------------------------ */
/* globals cached in JNI_OnLoad                                        */
/* ------------------------------------------------------------------ */

static JavaVM *g_vm = NULL;
static jclass g_io_class = NULL;        /* com/tinyc/TinycIO */
static jmethodID g_io_write_out = NULL;
static jmethodID g_io_write_err = NULL;
static jmethodID g_io_read_in = NULL;
static jmethodID g_io_flush = NULL;
static jclass g_listener_class = NULL;  /* com/tinyc/Tinyc$ErrorListener */
static jmethodID g_listener_on_error = NULL;
static jclass g_hostfn_class = NULL;    /* com/tinyc/Tinyc$HostFunction */
static jmethodID g_hostfn_invoke = NULL;
static jclass g_object_class = NULL;    /* java/lang/Object */
static jclass g_integer_class = NULL;
static jmethodID g_integer_value_of = NULL;
static jclass g_long_class = NULL;
static jmethodID g_long_value_of = NULL;
static jclass g_double_class = NULL;
static jmethodID g_double_value_of = NULL;
static jclass g_number_class = NULL;    /* java/lang/Number */
static jmethodID g_number_int_value = NULL;
static jmethodID g_number_long_value = NULL;
static jmethodID g_number_double_value = NULL;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    (void)reserved;
    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK || env == NULL) {
        return JNI_ERR;
    }
    g_vm = vm;

    jclass io = (*env)->FindClass(env, "com/tinyc/TinycIO");
    if (io == NULL) return JNI_ERR;
    g_io_class = (*env)->NewGlobalRef(env, io);
    (*env)->DeleteLocalRef(env, io);
    g_io_write_out = (*env)->GetMethodID(env, g_io_class, "writeOut", "([B)I");
    g_io_write_err = (*env)->GetMethodID(env, g_io_class, "writeErr", "([B)I");
    g_io_read_in   = (*env)->GetMethodID(env, g_io_class, "readIn",   "([B)I");
    g_io_flush     = (*env)->GetMethodID(env, g_io_class, "flush",    "()V");
    if (!g_io_write_out || !g_io_write_err || !g_io_read_in || !g_io_flush) {
        return JNI_ERR;
    }

    jclass listener = (*env)->FindClass(env, "com/tinyc/Tinyc$ErrorListener");
    if (listener == NULL) return JNI_ERR;
    g_listener_class = (*env)->NewGlobalRef(env, listener);
    (*env)->DeleteLocalRef(env, listener);
    g_listener_on_error = (*env)->GetMethodID(env, g_listener_class,
                                              "onError", "(Ljava/lang/String;)V");
    if (!g_listener_on_error) return JNI_ERR;

    jclass hostfn = (*env)->FindClass(env, "com/tinyc/Tinyc$HostFunction");
    if (hostfn == NULL) return JNI_ERR;
    g_hostfn_class = (*env)->NewGlobalRef(env, hostfn);
    (*env)->DeleteLocalRef(env, hostfn);
    g_hostfn_invoke = (*env)->GetMethodID(env, g_hostfn_class, "invoke",
                                          "([Ljava/lang/Object;)Ljava/lang/Object;");
    if (!g_hostfn_invoke) return JNI_ERR;

    /* Boxing helpers for the host symbol trampolines. */
    struct { const char *name; jclass *slot; } classes[] = {
        { "java/lang/Object", &g_object_class },
        { "java/lang/Integer", &g_integer_class },
        { "java/lang/Long", &g_long_class },
        { "java/lang/Double", &g_double_class },
        { "java/lang/Number", &g_number_class },
    };
    for (size_t i = 0; i < sizeof(classes) / sizeof(classes[0]); i++) {
        jclass c = (*env)->FindClass(env, classes[i].name);
        if (c == NULL) return JNI_ERR;
        *classes[i].slot = (*env)->NewGlobalRef(env, c);
        (*env)->DeleteLocalRef(env, c);
    }
    g_integer_value_of = (*env)->GetStaticMethodID(env, g_integer_class,
                                                   "valueOf", "(I)Ljava/lang/Integer;");
    g_long_value_of = (*env)->GetStaticMethodID(env, g_long_class,
                                                "valueOf", "(J)Ljava/lang/Long;");
    g_double_value_of = (*env)->GetStaticMethodID(env, g_double_class,
                                                  "valueOf", "(D)Ljava/lang/Double;");
    g_number_int_value = (*env)->GetMethodID(env, g_number_class, "intValue", "()I");
    g_number_long_value = (*env)->GetMethodID(env, g_number_class, "longValue", "()J");
    g_number_double_value = (*env)->GetMethodID(env, g_number_class, "doubleValue", "()D");
    if (!g_integer_value_of || !g_long_value_of || !g_double_value_of
            || !g_number_int_value || !g_number_long_value || !g_number_double_value) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}

/* ------------------------------------------------------------------ */
/* helpers                                                             */
/* ------------------------------------------------------------------ */

/* Get an env for the current thread; *attached is set if we attached
 * and the caller must detach again. */
static JNIEnv *get_env(int *attached) {
    JNIEnv *env = NULL;
    *attached = 0;
    if (g_vm == NULL) return NULL;
    jint rc = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_6);
    if (rc == JNI_EDETACHED) {
        if ((*g_vm)->AttachCurrentThread(g_vm, &env, NULL) == JNI_OK) {
            *attached = 1;
        } else {
            return NULL;
        }
    }
    return env;
}

static void put_env(int attached) {
    if (attached && g_vm != NULL) {
        (*g_vm)->DetachCurrentThread(g_vm);
    }
}

static char *dup_jstring(JNIEnv *env, jstring s) {
    if (s == NULL) return NULL;
    const char *utf = (*env)->GetStringUTFChars(env, s, NULL);
    if (utf == NULL) return NULL;
    char *copy = strdup(utf);
    (*env)->ReleaseStringUTFChars(env, s, utf);
    return copy;
}

/* Capture a pending Java exception into *pending (as a global ref) and
 * clear it, so nothing propagates through C frames (spec 6.5 rule 1). */
static void capture_exception(JNIEnv *env, jthrowable *pending) {
    if (!(*env)->ExceptionCheck(env)) return;
    jthrowable ex = (*env)->ExceptionOccurred(env);
    (*env)->ExceptionClear(env);
    if (ex == NULL) return;
    if (*pending != NULL) {
        (*env)->DeleteGlobalRef(env, *pending);
    }
    *pending = (jthrowable)(*env)->NewGlobalRef(env, ex);
    (*env)->DeleteLocalRef(env, ex);
}

/* Rethrow a previously captured exception, if any. */
static void throw_captured(JNIEnv *env, jthrowable *pending) {
    if (*pending == NULL) return;
    jthrowable ex = *pending;
    *pending = NULL;
    (*env)->Throw(env, ex);
    (*env)->DeleteGlobalRef(env, ex);
}

/* ------------------------------------------------------------------ */
/* IO trampolines: TCCIO function pointers -> Java TinycIO             */
/* ------------------------------------------------------------------ */

typedef struct IoCtx {
    jobject io;         /* global ref for per-state ctx; for the CLI call
                           it may be a local ref valid during that call */
    jthrowable pending; /* captured callback exception (global ref) */
} IoCtx;

static int io_write_common(IoCtx *ctx, const char *buf, size_t len, jmethodID mid) {
    if (ctx->pending != NULL) return -1;
    int attached = 0;
    JNIEnv *env = get_env(&attached);
    if (env == NULL) return -1;

    jbyteArray arr = (*env)->NewByteArray(env, (jsize)len);
    if (arr == NULL) {
        capture_exception(env, &ctx->pending);
        put_env(attached);
        return -1;
    }
    if (len > 0) {
        (*env)->SetByteArrayRegion(env, arr, 0, (jsize)len, (const jbyte *)buf);
    }
    jint n = (*env)->CallIntMethod(env, ctx->io, mid, arr);
    (*env)->DeleteLocalRef(env, arr);
    if ((*env)->ExceptionCheck(env)) {
        capture_exception(env, &ctx->pending);
        put_env(attached);
        return -1;
    }
    put_env(attached);
    return (int)n;
}

static int io_write_out(void *vctx, const char *buf, size_t len) {
    return io_write_common((IoCtx *)vctx, buf, len, g_io_write_out);
}

static int io_write_err(void *vctx, const char *buf, size_t len) {
    return io_write_common((IoCtx *)vctx, buf, len, g_io_write_err);
}

static int io_read_in(void *vctx, char *buf, size_t cap) {
    IoCtx *ctx = (IoCtx *)vctx;
    if (ctx->pending != NULL) return -1;
    int attached = 0;
    JNIEnv *env = get_env(&attached);
    if (env == NULL) return -1;

    jbyteArray arr = (*env)->NewByteArray(env, (jsize)cap);
    if (arr == NULL) {
        capture_exception(env, &ctx->pending);
        put_env(attached);
        return -1;
    }
    jint n = (*env)->CallIntMethod(env, ctx->io, g_io_read_in, arr);
    if ((*env)->ExceptionCheck(env)) {
        capture_exception(env, &ctx->pending);
        (*env)->DeleteLocalRef(env, arr);
        put_env(attached);
        return -1;
    }
    if (n > 0) {
        if ((size_t)n > cap) n = (jint)cap;
        (*env)->GetByteArrayRegion(env, arr, 0, n, (jbyte *)buf);
    }
    (*env)->DeleteLocalRef(env, arr);
    put_env(attached);
    return (int)n; /* 0 = EOF, -1 = error, per TinycIO contract */
}

static void io_flush(void *vctx) {
    IoCtx *ctx = (IoCtx *)vctx;
    if (ctx->pending != NULL) return;
    int attached = 0;
    JNIEnv *env = get_env(&attached);
    if (env == NULL) return;
    (*env)->CallVoidMethod(env, ctx->io, g_io_flush);
    if ((*env)->ExceptionCheck(env)) {
        capture_exception(env, &ctx->pending);
    }
    put_env(attached);
}

static TCCIO *make_tcc_io(IoCtx *ctx) {
    TCCIO *io = (TCCIO *)calloc(1, sizeof(TCCIO));
    if (io == NULL) return NULL;
    io->ctx = ctx;
    io->write_out = io_write_out;
    io->write_err = io_write_err;
    io->read_in = io_read_in;
    io->flush = io_flush;
    return io;
}

/* ------------------------------------------------------------------ */
/* per-state context: IO + error listener + captured exception         */
/* ------------------------------------------------------------------ */

typedef struct StateCtx {
    TCCState *state;
    IoCtx io_ctx;            /* io_ctx.io is a global ref or NULL */
    TCCIO *tcc_io;           /* owned, or NULL */
    jobject error_listener;  /* global ref or NULL */
    struct StateCtx *next;
} StateCtx;

static StateCtx *g_states = NULL;
static pthread_mutex_t g_states_lock = PTHREAD_MUTEX_INITIALIZER;

static StateCtx *ctx_find(TCCState *s, int create) {
    pthread_mutex_lock(&g_states_lock);
    StateCtx *c = g_states;
    while (c != NULL && c->state != s) c = c->next;
    if (c == NULL && create) {
        c = (StateCtx *)calloc(1, sizeof(StateCtx));
        if (c != NULL) {
            c->state = s;
            c->next = g_states;
            g_states = c;
        }
    }
    pthread_mutex_unlock(&g_states_lock);
    return c;
}

/* A state's IO is installed on the state itself (tcc_set_io) and the
 * state does not track ownership: before the object is freed, detach
 * it from its state so the state falls back to stdio. Left installed,
 * the freed object dangles and the state's next IO dispatch (e.g. an
 * error print) calls through freed memory. */
static void detach_io(TCCState *s, TCCIO *io) {
    if (io != NULL && tcc_get_io(s) == io) {
        tcc_set_io(s, NULL);
    }
}

static void ctx_remove(JNIEnv *env, TCCState *s) {
    pthread_mutex_lock(&g_states_lock);
    StateCtx **pp = &g_states;
    while (*pp != NULL && (*pp)->state != s) pp = &(*pp)->next;
    StateCtx *c = *pp;
    if (c != NULL) *pp = c->next;
    pthread_mutex_unlock(&g_states_lock);
    if (c == NULL) return;
    if (c->io_ctx.io != NULL) (*env)->DeleteGlobalRef(env, c->io_ctx.io);
    if (c->io_ctx.pending != NULL) (*env)->DeleteGlobalRef(env, c->io_ctx.pending);
    if (c->error_listener != NULL) (*env)->DeleteGlobalRef(env, c->error_listener);
    detach_io(c->state, c->tcc_io);
    free(c->tcc_io);
    free(c);
}

/* If the state's IO / listener callbacks captured an exception, rethrow
 * it now that the native TCC call has returned (spec 6.5 rule 1). */
static void rethrow_state_pending(JNIEnv *env, TCCState *s) {
    StateCtx *c = ctx_find(s, 0);
    if (c != NULL) {
        throw_captured(env, &c->io_ctx.pending);
    }
}

/* tcc_set_error_func trampoline: opaque is the StateCtx. */
static void error_trampoline(void *opaque, const char *msg) {
    StateCtx *c = (StateCtx *)opaque;
    if (c == NULL || c->error_listener == NULL) return;
    if (c->io_ctx.pending != NULL) return;
    int attached = 0;
    JNIEnv *env = get_env(&attached);
    if (env == NULL) return;
    jstring s = (*env)->NewStringUTF(env, msg != NULL ? msg : "");
    if (s != NULL) {
        (*env)->CallVoidMethod(env, c->error_listener, g_listener_on_error, s);
        (*env)->DeleteLocalRef(env, s);
    }
    if ((*env)->ExceptionCheck(env)) {
        /* the error callback has no error return channel; capture and let
         * the current JNI entry point rethrow after the TCC call */
        capture_exception(env, &c->io_ctx.pending);
    }
    put_env(attached);
}

/* ------------------------------------------------------------------ */
/* host symbol trampolines: Java methods as C function pointers        */
/*                                                                     */
/* tcc_add_symbol registers a raw address in this process, so a Java   */
/* method needs a real C function to point at. For every supported     */
/* signature shape there is one trampoline per slot (16 slots,         */
/* shared by all states); a trampoline packs its scalar arguments      */
/* and calls host_dispatch, which boxes them into an Object[] and      */
/* invokes the registered Tinyc.HostFunction. Supported shapes:        */
/* return void / int / long / double; 0 to 3 parameters, each int /    */
/* long / double. Anything else is rejected at registration time       */
/* (nativeAddSymbol returns -2), never silently misaligned.            */
/*                                                                     */
/* Exceptions follow spec 6.5 rule 1: a throwing HostFunction is       */
/* captured into the owning state's pending slot (the same channel     */
/* the IO callbacks use) and rethrown by the JNI entry point after     */
/* the native call returns; the compiled code sees a zero return.      */
/* ------------------------------------------------------------------ */

#define TINYC_HOST_SLOTS 16   /* matches Tinyc.MAX_HOST_SYMBOLS */

enum { RK_VOID = 0, RK_INT = 1, RK_LONG = 2, RK_DOUBLE = 3 };
enum { PK_INT = 0, PK_LONG = 1, PK_DOUBLE = 2 };

typedef union HostArg {
    int64_t i;
    double d;
} HostArg;

typedef struct HostSlot {
    int used;
    TCCState *state;
    jobject fn;              /* global ref, Tinyc$HostFunction */
    int ret_kind;            /* RK_* */
    int param_kinds[3];      /* PK_* */
    int nparams;
} HostSlot;

static HostSlot g_host_slots[TINYC_HOST_SLOTS];
static pthread_mutex_t g_host_lock = PTHREAD_MUTEX_INITIALIZER;

/* Capture a pending exception into the owning state's pending slot
 * (the IO callbacks' channel), so the current JNI entry point
 * rethrows it after the native call returns. */
static void host_capture_pending(JNIEnv *env, TCCState *s) {
    StateCtx *c = ctx_find(s, 1);
    if (c != NULL) {
        capture_exception(env, &c->io_ctx.pending);
    } else {
        (*env)->ExceptionClear(env);
    }
}

static void host_dispatch(int slot, int nargs, const HostArg *args, HostArg *ret) {
    HostSlot rec;
    ret->i = 0;
    pthread_mutex_lock(&g_host_lock);
    if (slot < 0 || slot >= TINYC_HOST_SLOTS || !g_host_slots[slot].used) {
        pthread_mutex_unlock(&g_host_lock);
        return;
    }
    rec = g_host_slots[slot];
    pthread_mutex_unlock(&g_host_lock);

    int attached = 0;
    JNIEnv *env = get_env(&attached);
    if (env == NULL) return;

    jobjectArray arr = (*env)->NewObjectArray(env, (jsize)nargs, g_object_class, NULL);
    if (arr == NULL) {
        host_capture_pending(env, rec.state);
        put_env(attached);
        return;
    }
    for (int k = 0; k < nargs; k++) {
        jobject box = NULL;
        switch (rec.param_kinds[k]) {
        case PK_INT:
            box = (*env)->CallStaticObjectMethod(env, g_integer_class,
                                                 g_integer_value_of,
                                                 (jint)args[k].i);
            break;
        case PK_LONG:
            box = (*env)->CallStaticObjectMethod(env, g_long_class,
                                                 g_long_value_of,
                                                 (jlong)args[k].i);
            break;
        case PK_DOUBLE:
            box = (*env)->CallStaticObjectMethod(env, g_double_class,
                                                 g_double_value_of,
                                                 args[k].d);
            break;
        }
        if (box != NULL) {
            (*env)->SetObjectArrayElement(env, arr, k, box);
            (*env)->DeleteLocalRef(env, box);
        }
    }
    jobject res = (*env)->CallObjectMethod(env, rec.fn, g_hostfn_invoke, arr);
    (*env)->DeleteLocalRef(env, arr);
    if ((*env)->ExceptionCheck(env)) {
        host_capture_pending(env, rec.state);
        put_env(attached);
        return;
    }
    if (res != NULL) {
        switch (rec.ret_kind) {
        case RK_INT:
            ret->i = (*env)->CallIntMethod(env, res, g_number_int_value);
            break;
        case RK_LONG:
            ret->i = (*env)->CallLongMethod(env, res, g_number_long_value);
            break;
        case RK_DOUBLE:
            ret->d = (*env)->CallDoubleMethod(env, res, g_number_double_value);
            break;
        default:
            break;
        }
        if ((*env)->ExceptionCheck(env)) {
            host_capture_pending(env, rec.state);
            ret->i = 0;
        }
        (*env)->DeleteLocalRef(env, res);
    }
    put_env(attached);
}

/* Per-shape trampoline generators. HT_<k> is the C type of kind k,
 * HF_<k> the HostArg field a parameter of kind k travels in, RF_<k>
 * the field a return value of kind k is read back from. */
#define HT_i int
#define HT_l long
#define HT_d double
#define HF_i i
#define HF_l i
#define HF_d d
#define RF_i i
#define RF_l i
#define RF_d d

#define DEF_TRAMP_0(S, R) \
    static HT_##R tramp_##S##_##R(void) { \
        HostArg r; host_dispatch(S, 0, NULL, &r); \
        return (HT_##R)(r.RF_##R); \
    }
#define DEF_TRAMP_0V(S) \
    static void tramp_##S##_v(void) { \
        HostArg r; host_dispatch(S, 0, NULL, &r); \
    }
#define DEF_TRAMP_1(S, R, P1) \
    static HT_##R tramp_##S##_##R##_##P1(HT_##P1 a0) { \
        HostArg a[1], r; a[0].HF_##P1 = a0; \
        host_dispatch(S, 1, a, &r); \
        return (HT_##R)(r.RF_##R); \
    }
#define DEF_TRAMP_1V(S, P1) \
    static void tramp_##S##_v_##P1(HT_##P1 a0) { \
        HostArg a[1], r; a[0].HF_##P1 = a0; \
        host_dispatch(S, 1, a, &r); \
    }
#define DEF_TRAMP_2(S, R, P1, P2) \
    static HT_##R tramp_##S##_##R##_##P1##_##P2(HT_##P1 a0, HT_##P2 a1) { \
        HostArg a[2], r; a[0].HF_##P1 = a0; a[1].HF_##P2 = a1; \
        host_dispatch(S, 2, a, &r); \
        return (HT_##R)(r.RF_##R); \
    }
#define DEF_TRAMP_2V(S, P1, P2) \
    static void tramp_##S##_v_##P1##_##P2(HT_##P1 a0, HT_##P2 a1) { \
        HostArg a[2], r; a[0].HF_##P1 = a0; a[1].HF_##P2 = a1; \
        host_dispatch(S, 2, a, &r); \
    }
#define DEF_TRAMP_3(S, R, P1, P2, P3) \
    static HT_##R tramp_##S##_##R##_##P1##_##P2##_##P3( \
            HT_##P1 a0, HT_##P2 a1, HT_##P3 a2) { \
        HostArg a[3], r; \
        a[0].HF_##P1 = a0; a[1].HF_##P2 = a1; a[2].HF_##P3 = a2; \
        host_dispatch(S, 3, a, &r); \
        return (HT_##R)(r.RF_##R); \
    }
#define DEF_TRAMP_3V(S, P1, P2, P3) \
    static void tramp_##S##_v_##P1##_##P2##_##P3( \
            HT_##P1 a0, HT_##P2 a1, HT_##P3 a2) { \
        HostArg a[3], r; \
        a[0].HF_##P1 = a0; a[1].HF_##P2 = a1; a[2].HF_##P3 = a2; \
        host_dispatch(S, 3, a, &r); \
    }

/* One shape = the 16 slot trampolines + a solver picking one by slot. */
#define DEF_SHAPE_0(R) \
    DEF_TRAMP_0(0, R) DEF_TRAMP_0(1, R) DEF_TRAMP_0(2, R) DEF_TRAMP_0(3, R) \
    DEF_TRAMP_0(4, R) DEF_TRAMP_0(5, R) DEF_TRAMP_0(6, R) DEF_TRAMP_0(7, R) \
    DEF_TRAMP_0(8, R) DEF_TRAMP_0(9, R) DEF_TRAMP_0(10, R) DEF_TRAMP_0(11, R) \
    DEF_TRAMP_0(12, R) DEF_TRAMP_0(13, R) DEF_TRAMP_0(14, R) DEF_TRAMP_0(15, R) \
    static void *solve_##R(int slot) { \
        static void *const tab[TINYC_HOST_SLOTS] = { \
            tramp_0_##R, tramp_1_##R, tramp_2_##R, tramp_3_##R, \
            tramp_4_##R, tramp_5_##R, tramp_6_##R, tramp_7_##R, \
            tramp_8_##R, tramp_9_##R, tramp_10_##R, tramp_11_##R, \
            tramp_12_##R, tramp_13_##R, tramp_14_##R, tramp_15_##R }; \
        return tab[slot]; \
    }
#define DEF_SHAPE_0V() \
    DEF_TRAMP_0V(0) DEF_TRAMP_0V(1) DEF_TRAMP_0V(2) DEF_TRAMP_0V(3) \
    DEF_TRAMP_0V(4) DEF_TRAMP_0V(5) DEF_TRAMP_0V(6) DEF_TRAMP_0V(7) \
    DEF_TRAMP_0V(8) DEF_TRAMP_0V(9) DEF_TRAMP_0V(10) DEF_TRAMP_0V(11) \
    DEF_TRAMP_0V(12) DEF_TRAMP_0V(13) DEF_TRAMP_0V(14) DEF_TRAMP_0V(15) \
    static void *solve_v(int slot) { \
        static void *const tab[TINYC_HOST_SLOTS] = { \
            tramp_0_v, tramp_1_v, tramp_2_v, tramp_3_v, \
            tramp_4_v, tramp_5_v, tramp_6_v, tramp_7_v, \
            tramp_8_v, tramp_9_v, tramp_10_v, tramp_11_v, \
            tramp_12_v, tramp_13_v, tramp_14_v, tramp_15_v }; \
        return tab[slot]; \
    }
#define DEF_SHAPE_1(R, P1) \
    DEF_TRAMP_1(0, R, P1) DEF_TRAMP_1(1, R, P1) DEF_TRAMP_1(2, R, P1) \
    DEF_TRAMP_1(3, R, P1) DEF_TRAMP_1(4, R, P1) DEF_TRAMP_1(5, R, P1) \
    DEF_TRAMP_1(6, R, P1) DEF_TRAMP_1(7, R, P1) DEF_TRAMP_1(8, R, P1) \
    DEF_TRAMP_1(9, R, P1) DEF_TRAMP_1(10, R, P1) DEF_TRAMP_1(11, R, P1) \
    DEF_TRAMP_1(12, R, P1) DEF_TRAMP_1(13, R, P1) DEF_TRAMP_1(14, R, P1) \
    DEF_TRAMP_1(15, R, P1) \
    static void *solve_##R##_##P1(int slot) { \
        static void *const tab[TINYC_HOST_SLOTS] = { \
            tramp_0_##R##_##P1, tramp_1_##R##_##P1, tramp_2_##R##_##P1, \
            tramp_3_##R##_##P1, tramp_4_##R##_##P1, tramp_5_##R##_##P1, \
            tramp_6_##R##_##P1, tramp_7_##R##_##P1, tramp_8_##R##_##P1, \
            tramp_9_##R##_##P1, tramp_10_##R##_##P1, tramp_11_##R##_##P1, \
            tramp_12_##R##_##P1, tramp_13_##R##_##P1, tramp_14_##R##_##P1, \
            tramp_15_##R##_##P1 }; \
        return tab[slot]; \
    }
#define DEF_SHAPE_1V(P1) \
    DEF_TRAMP_1V(0, P1) DEF_TRAMP_1V(1, P1) DEF_TRAMP_1V(2, P1) \
    DEF_TRAMP_1V(3, P1) DEF_TRAMP_1V(4, P1) DEF_TRAMP_1V(5, P1) \
    DEF_TRAMP_1V(6, P1) DEF_TRAMP_1V(7, P1) DEF_TRAMP_1V(8, P1) \
    DEF_TRAMP_1V(9, P1) DEF_TRAMP_1V(10, P1) DEF_TRAMP_1V(11, P1) \
    DEF_TRAMP_1V(12, P1) DEF_TRAMP_1V(13, P1) DEF_TRAMP_1V(14, P1) \
    DEF_TRAMP_1V(15, P1) \
    static void *solve_v_##P1(int slot) { \
        static void *const tab[TINYC_HOST_SLOTS] = { \
            tramp_0_v_##P1, tramp_1_v_##P1, tramp_2_v_##P1, \
            tramp_3_v_##P1, tramp_4_v_##P1, tramp_5_v_##P1, \
            tramp_6_v_##P1, tramp_7_v_##P1, tramp_8_v_##P1, \
            tramp_9_v_##P1, tramp_10_v_##P1, tramp_11_v_##P1, \
            tramp_12_v_##P1, tramp_13_v_##P1, tramp_14_v_##P1, \
            tramp_15_v_##P1 }; \
        return tab[slot]; \
    }
#define DEF_SHAPE_2(R, P1, P2) \
    DEF_TRAMP_2(0, R, P1, P2) DEF_TRAMP_2(1, R, P1, P2) \
    DEF_TRAMP_2(2, R, P1, P2) DEF_TRAMP_2(3, R, P1, P2) \
    DEF_TRAMP_2(4, R, P1, P2) DEF_TRAMP_2(5, R, P1, P2) \
    DEF_TRAMP_2(6, R, P1, P2) DEF_TRAMP_2(7, R, P1, P2) \
    DEF_TRAMP_2(8, R, P1, P2) DEF_TRAMP_2(9, R, P1, P2) \
    DEF_TRAMP_2(10, R, P1, P2) DEF_TRAMP_2(11, R, P1, P2) \
    DEF_TRAMP_2(12, R, P1, P2) DEF_TRAMP_2(13, R, P1, P2) \
    DEF_TRAMP_2(14, R, P1, P2) DEF_TRAMP_2(15, R, P1, P2) \
    static void *solve_##R##_##P1##_##P2(int slot) { \
        static void *const tab[TINYC_HOST_SLOTS] = { \
            tramp_0_##R##_##P1##_##P2, tramp_1_##R##_##P1##_##P2, \
            tramp_2_##R##_##P1##_##P2, tramp_3_##R##_##P1##_##P2, \
            tramp_4_##R##_##P1##_##P2, tramp_5_##R##_##P1##_##P2, \
            tramp_6_##R##_##P1##_##P2, tramp_7_##R##_##P1##_##P2, \
            tramp_8_##R##_##P1##_##P2, tramp_9_##R##_##P1##_##P2, \
            tramp_10_##R##_##P1##_##P2, tramp_11_##R##_##P1##_##P2, \
            tramp_12_##R##_##P1##_##P2, tramp_13_##R##_##P1##_##P2, \
            tramp_14_##R##_##P1##_##P2, tramp_15_##R##_##P1##_##P2 }; \
        return tab[slot]; \
    }
#define DEF_SHAPE_2V(P1, P2) \
    DEF_TRAMP_2V(0, P1, P2) DEF_TRAMP_2V(1, P1, P2) \
    DEF_TRAMP_2V(2, P1, P2) DEF_TRAMP_2V(3, P1, P2) \
    DEF_TRAMP_2V(4, P1, P2) DEF_TRAMP_2V(5, P1, P2) \
    DEF_TRAMP_2V(6, P1, P2) DEF_TRAMP_2V(7, P1, P2) \
    DEF_TRAMP_2V(8, P1, P2) DEF_TRAMP_2V(9, P1, P2) \
    DEF_TRAMP_2V(10, P1, P2) DEF_TRAMP_2V(11, P1, P2) \
    DEF_TRAMP_2V(12, P1, P2) DEF_TRAMP_2V(13, P1, P2) \
    DEF_TRAMP_2V(14, P1, P2) DEF_TRAMP_2V(15, P1, P2) \
    static void *solve_v_##P1##_##P2(int slot) { \
        static void *const tab[TINYC_HOST_SLOTS] = { \
            tramp_0_v_##P1##_##P2, tramp_1_v_##P1##_##P2, \
            tramp_2_v_##P1##_##P2, tramp_3_v_##P1##_##P2, \
            tramp_4_v_##P1##_##P2, tramp_5_v_##P1##_##P2, \
            tramp_6_v_##P1##_##P2, tramp_7_v_##P1##_##P2, \
            tramp_8_v_##P1##_##P2, tramp_9_v_##P1##_##P2, \
            tramp_10_v_##P1##_##P2, tramp_11_v_##P1##_##P2, \
            tramp_12_v_##P1##_##P2, tramp_13_v_##P1##_##P2, \
            tramp_14_v_##P1##_##P2, tramp_15_v_##P1##_##P2 }; \
        return tab[slot]; \
    }
#define DEF_SHAPE_3(R, P1, P2, P3) \
    DEF_TRAMP_3(0, R, P1, P2, P3) DEF_TRAMP_3(1, R, P1, P2, P3) \
    DEF_TRAMP_3(2, R, P1, P2, P3) DEF_TRAMP_3(3, R, P1, P2, P3) \
    DEF_TRAMP_3(4, R, P1, P2, P3) DEF_TRAMP_3(5, R, P1, P2, P3) \
    DEF_TRAMP_3(6, R, P1, P2, P3) DEF_TRAMP_3(7, R, P1, P2, P3) \
    DEF_TRAMP_3(8, R, P1, P2, P3) DEF_TRAMP_3(9, R, P1, P2, P3) \
    DEF_TRAMP_3(10, R, P1, P2, P3) DEF_TRAMP_3(11, R, P1, P2, P3) \
    DEF_TRAMP_3(12, R, P1, P2, P3) DEF_TRAMP_3(13, R, P1, P2, P3) \
    DEF_TRAMP_3(14, R, P1, P2, P3) DEF_TRAMP_3(15, R, P1, P2, P3) \
    static void *solve_##R##_##P1##_##P2##_##P3(int slot) { \
        static void *const tab[TINYC_HOST_SLOTS] = { \
            tramp_0_##R##_##P1##_##P2##_##P3, \
            tramp_1_##R##_##P1##_##P2##_##P3, \
            tramp_2_##R##_##P1##_##P2##_##P3, \
            tramp_3_##R##_##P1##_##P2##_##P3, \
            tramp_4_##R##_##P1##_##P2##_##P3, \
            tramp_5_##R##_##P1##_##P2##_##P3, \
            tramp_6_##R##_##P1##_##P2##_##P3, \
            tramp_7_##R##_##P1##_##P2##_##P3, \
            tramp_8_##R##_##P1##_##P2##_##P3, \
            tramp_9_##R##_##P1##_##P2##_##P3, \
            tramp_10_##R##_##P1##_##P2##_##P3, \
            tramp_11_##R##_##P1##_##P2##_##P3, \
            tramp_12_##R##_##P1##_##P2##_##P3, \
            tramp_13_##R##_##P1##_##P2##_##P3, \
            tramp_14_##R##_##P1##_##P2##_##P3, \
            tramp_15_##R##_##P1##_##P2##_##P3 }; \
        return tab[slot]; \
    }
#define DEF_SHAPE_3V(P1, P2, P3) \
    DEF_TRAMP_3V(0, P1, P2, P3) DEF_TRAMP_3V(1, P1, P2, P3) \
    DEF_TRAMP_3V(2, P1, P2, P3) DEF_TRAMP_3V(3, P1, P2, P3) \
    DEF_TRAMP_3V(4, P1, P2, P3) DEF_TRAMP_3V(5, P1, P2, P3) \
    DEF_TRAMP_3V(6, P1, P2, P3) DEF_TRAMP_3V(7, P1, P2, P3) \
    DEF_TRAMP_3V(8, P1, P2, P3) DEF_TRAMP_3V(9, P1, P2, P3) \
    DEF_TRAMP_3V(10, P1, P2, P3) DEF_TRAMP_3V(11, P1, P2, P3) \
    DEF_TRAMP_3V(12, P1, P2, P3) DEF_TRAMP_3V(13, P1, P2, P3) \
    DEF_TRAMP_3V(14, P1, P2, P3) DEF_TRAMP_3V(15, P1, P2, P3) \
    static void *solve_v_##P1##_##P2##_##P3(int slot) { \
        static void *const tab[TINYC_HOST_SLOTS] = { \
            tramp_0_v_##P1##_##P2##_##P3, \
            tramp_1_v_##P1##_##P2##_##P3, \
            tramp_2_v_##P1##_##P2##_##P3, \
            tramp_3_v_##P1##_##P2##_##P3, \
            tramp_4_v_##P1##_##P2##_##P3, \
            tramp_5_v_##P1##_##P2##_##P3, \
            tramp_6_v_##P1##_##P2##_##P3, \
            tramp_7_v_##P1##_##P2##_##P3, \
            tramp_8_v_##P1##_##P2##_##P3, \
            tramp_9_v_##P1##_##P2##_##P3, \
            tramp_10_v_##P1##_##P2##_##P3, \
            tramp_11_v_##P1##_##P2##_##P3, \
            tramp_12_v_##P1##_##P2##_##P3, \
            tramp_13_v_##P1##_##P2##_##P3, \
            tramp_14_v_##P1##_##P2##_##P3, \
            tramp_15_v_##P1##_##P2##_##P3 }; \
        return tab[slot]; \
    }

/* The supported signature shapes. X is applied twice below: once to
 * define the trampolines (DEF_SHAPE_*), once to resolve a signature
 * string to its solver (FIND_*). */
#define SHAPES_0(X) X(i) X(l) X(d)
#define SHAPES_0V(X) X()
#define SHAPES_1(X) X(i,i) X(i,l) X(i,d) X(l,i) X(l,l) X(l,d) X(d,i) X(d,l) \
    X(d,d)
#define SHAPES_1V(X) X(i) X(l) X(d)
#define SHAPES_2(X) X(i,i,i) X(i,i,l) X(i,i,d) X(i,l,i) X(i,l,l) X(i,l,d) \
    X(i,d,i) X(i,d,l) X(i,d,d) X(l,i,i) X(l,i,l) X(l,i,d) X(l,l,i) X(l,l,l) \
    X(l,l,d) X(l,d,i) X(l,d,l) X(l,d,d) X(d,i,i) X(d,i,l) X(d,i,d) X(d,l,i) \
    X(d,l,l) X(d,l,d) X(d,d,i) X(d,d,l) X(d,d,d)
#define SHAPES_2V(X) X(i,i) X(i,l) X(i,d) X(l,i) X(l,l) X(l,d) X(d,i) \
    X(d,l) X(d,d)
#define SHAPES_3(X) X(i,i,i,i) X(i,i,i,l) X(i,i,i,d) X(i,i,l,i) X(i,i,l,l) \
    X(i,i,l,d) X(i,i,d,i) X(i,i,d,l) X(i,i,d,d) X(i,l,i,i) X(i,l,i,l) \
    X(i,l,i,d) X(i,l,l,i) X(i,l,l,l) X(i,l,l,d) X(i,l,d,i) X(i,l,d,l) \
    X(i,l,d,d) X(i,d,i,i) X(i,d,i,l) X(i,d,i,d) X(i,d,l,i) X(i,d,l,l) \
    X(i,d,l,d) X(i,d,d,i) X(i,d,d,l) X(i,d,d,d) X(l,i,i,i) X(l,i,i,l) \
    X(l,i,i,d) X(l,i,l,i) X(l,i,l,l) X(l,i,l,d) X(l,i,d,i) X(l,i,d,l) \
    X(l,i,d,d) X(l,l,i,i) X(l,l,i,l) X(l,l,i,d) X(l,l,l,i) X(l,l,l,l) \
    X(l,l,l,d) X(l,l,d,i) X(l,l,d,l) X(l,l,d,d) X(l,d,i,i) X(l,d,i,l) \
    X(l,d,i,d) X(l,d,l,i) X(l,d,l,l) X(l,d,l,d) X(l,d,d,i) X(l,d,d,l) \
    X(l,d,d,d) X(d,i,i,i) X(d,i,i,l) X(d,i,i,d) X(d,i,l,i) X(d,i,l,l) \
    X(d,i,l,d) X(d,i,d,i) X(d,i,d,l) X(d,i,d,d) X(d,l,i,i) X(d,l,i,l) \
    X(d,l,i,d) X(d,l,l,i) X(d,l,l,l) X(d,l,l,d) X(d,l,d,i) X(d,l,d,l) \
    X(d,l,d,d) X(d,d,i,i) X(d,d,i,l) X(d,d,i,d) X(d,d,l,i) X(d,d,l,l) \
    X(d,d,l,d) X(d,d,d,i) X(d,d,d,l) X(d,d,d,d)
#define SHAPES_3V(X) X(i,i,i) X(i,i,l) X(i,i,d) X(i,l,i) X(i,l,l) X(i,l,d) \
    X(i,d,i) X(i,d,l) X(i,d,d) X(l,i,i) X(l,i,l) X(l,i,d) X(l,l,i) X(l,l,l) \
    X(l,l,d) X(l,d,i) X(l,d,l) X(l,d,d) X(d,i,i) X(d,i,l) X(d,i,d) X(d,l,i) \
    X(d,l,l) X(d,l,d) X(d,d,i) X(d,d,l) X(d,d,d)

SHAPES_0(DEF_SHAPE_0)
SHAPES_0V(DEF_SHAPE_0V)
SHAPES_1(DEF_SHAPE_1)
SHAPES_1V(DEF_SHAPE_1V)
SHAPES_2(DEF_SHAPE_2)
SHAPES_2V(DEF_SHAPE_2V)
SHAPES_3(DEF_SHAPE_3)
SHAPES_3V(DEF_SHAPE_3V)

#define FIND_0(R) \
    if (strcmp(sig, #R "()") == 0) return solve_##R(slot);
#define FIND_0V() \
    if (strcmp(sig, "v()") == 0) return solve_v(slot);
#define FIND_1(R, P1) \
    if (strcmp(sig, #R "(" #P1 ")") == 0) return solve_##R##_##P1(slot);
#define FIND_1V(P1) \
    if (strcmp(sig, "v(" #P1 ")") == 0) return solve_v_##P1(slot);
#define FIND_2(R, P1, P2) \
    if (strcmp(sig, #R "(" #P1 #P2 ")") == 0) \
        return solve_##R##_##P1##_##P2(slot);
#define FIND_2V(P1, P2) \
    if (strcmp(sig, "v(" #P1 #P2 ")") == 0) \
        return solve_v_##P1##_##P2(slot);
#define FIND_3(R, P1, P2, P3) \
    if (strcmp(sig, #R "(" #P1 #P2 #P3 ")") == 0) \
        return solve_##R##_##P1##_##P2##_##P3(slot);
#define FIND_3V(P1, P2, P3) \
    if (strcmp(sig, "v(" #P1 #P2 #P3 ")") == 0) \
        return solve_v_##P1##_##P2##_##P3(slot);

static void *host_trampoline_for(const char *sig, int slot) {
    SHAPES_0(FIND_0)
    SHAPES_0V(FIND_0V)
    SHAPES_1(FIND_1)
    SHAPES_1V(FIND_1V)
    SHAPES_2(FIND_2)
    SHAPES_2V(FIND_2V)
    SHAPES_3(FIND_3)
    SHAPES_3V(FIND_3V)
    return NULL;
}

/* Parse "<ret>(<params>)": ret v/i/l/d, params up to 3 of i/l/d.
 * Returns 0 on success. Any accepted string is already in the
 * canonical form host_trampoline_for compares against. */
static int host_parse_signature(const char *sig, int *ret_kind,
                                int *param_kinds, int *nparams) {
    if (sig == NULL) return -1;
    size_t len = strlen(sig);
    if (len < 3 || len > 6 || sig[1] != '(' || sig[len - 1] != ')') {
        return -1;
    }
    switch (sig[0]) {
    case 'v': *ret_kind = RK_VOID; break;
    case 'i': *ret_kind = RK_INT; break;
    case 'l': *ret_kind = RK_LONG; break;
    case 'd': *ret_kind = RK_DOUBLE; break;
    default: return -1;
    }
    int n = 0;
    for (size_t k = 2; k + 1 < len; k++) {
        switch (sig[k]) {
        case 'i': param_kinds[n++] = PK_INT; break;
        case 'l': param_kinds[n++] = PK_LONG; break;
        case 'd': param_kinds[n++] = PK_DOUBLE; break;
        default: return -1;
        }
    }
    *nparams = n;
    return 0;
}

static void host_slot_clear(JNIEnv *env, int slot) {
    jobject fn = NULL;
    pthread_mutex_lock(&g_host_lock);
    if (slot >= 0 && slot < TINYC_HOST_SLOTS && g_host_slots[slot].used) {
        fn = g_host_slots[slot].fn;
        memset(&g_host_slots[slot], 0, sizeof(HostSlot));
    }
    pthread_mutex_unlock(&g_host_lock);
    if (fn != NULL && env != NULL) {
        (*env)->DeleteGlobalRef(env, fn);
    }
}

/* Free every slot owned by a state (called from nativeDelete before
 * the state itself is deleted). */
static void host_slots_clear_state(JNIEnv *env, TCCState *s) {
    jobject refs[TINYC_HOST_SLOTS];
    int n = 0;
    pthread_mutex_lock(&g_host_lock);
    for (int i = 0; i < TINYC_HOST_SLOTS; i++) {
        if (g_host_slots[i].used && g_host_slots[i].state == s) {
            refs[n++] = g_host_slots[i].fn;
            memset(&g_host_slots[i], 0, sizeof(HostSlot));
        }
    }
    pthread_mutex_unlock(&g_host_lock);
    if (env != NULL) {
        for (int i = 0; i < n; i++) {
            if (refs[i] != NULL) (*env)->DeleteGlobalRef(env, refs[i]);
        }
    }
}


/* ------------------------------------------------------------------ */
/* argv marshalling                                                    */
/* ------------------------------------------------------------------ */

static char **build_argv(JNIEnv *env, jobjectArray arr, int *out_argc) {
    jsize n = (arr != NULL) ? (*env)->GetArrayLength(env, arr) : 0;
    char **argv = (char **)calloc((size_t)n + 1, sizeof(char *));
    if (argv == NULL) {
        *out_argc = 0;
        return NULL;
    }
    for (jsize i = 0; i < n; i++) {
        jstring s = (jstring)(*env)->GetObjectArrayElement(env, arr, i);
        argv[i] = dup_jstring(env, s);
        if (s != NULL) (*env)->DeleteLocalRef(env, s);
    }
    argv[n] = NULL;
    *out_argc = (int)n;
    return argv;
}

static void free_argv(char **argv, int argc) {
    if (argv == NULL) return;
    for (int i = 0; i < argc; i++) free(argv[i]);
    free(argv);
}

#define STATE(env, jstate) ((TCCState *)(uintptr_t)(jstate))

/* ------------------------------------------------------------------ */
/* lifecycle                                                           */
/* ------------------------------------------------------------------ */

JNIEXPORT jlong JNICALL
Java_com_tinyc_TinycNative_nativeNew(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    return (jlong)(uintptr_t)tcc_new();
}

JNIEXPORT void JNICALL
Java_com_tinyc_TinycNative_nativeDelete(JNIEnv *env, jclass cls, jlong jstate) {
    (void)cls;
    TCCState *s = STATE(env, jstate);
    if (s == NULL) return;
    host_slots_clear_state(env, s);
    ctx_remove(env, s);
    tcc_delete(s);
}

/* ------------------------------------------------------------------ */
/* configuration                                                       */
/* ------------------------------------------------------------------ */

JNIEXPORT void JNICALL
Java_com_tinyc_TinycNative_nativeSetLibPath(JNIEnv *env, jclass cls,
                                            jlong jstate, jstring path) {
    (void)cls;
    char *p = dup_jstring(env, path);
    tcc_set_lib_path(STATE(env, jstate), p);
    free(p);
}

JNIEXPORT void JNICALL
Java_com_tinyc_TinycNative_nativeSetErrorListener(JNIEnv *env, jclass cls,
                                                  jlong jstate, jobject listener) {
    (void)cls;
    TCCState *s = STATE(env, jstate);
    StateCtx *c = ctx_find(s, 1);
    if (c == NULL) return;
    if (c->error_listener != NULL) {
        (*env)->DeleteGlobalRef(env, c->error_listener);
        c->error_listener = NULL;
    }
    if (listener != NULL) {
        c->error_listener = (*env)->NewGlobalRef(env, listener);
        tcc_set_error_func(s, c, error_trampoline);
    } else {
        tcc_set_error_func(s, NULL, NULL);
    }
}

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeSetOptions(JNIEnv *env, jclass cls,
                                            jlong jstate, jstring options) {
    (void)cls;
    char *o = dup_jstring(env, options);
    int rc = tcc_set_options(STATE(env, jstate), o);
    free(o);
    return rc;
}

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeSetOutputType(JNIEnv *env, jclass cls,
                                               jlong jstate, jint output_type) {
    (void)env; (void)cls;
    return tcc_set_output_type(STATE(env, jstate), (int)output_type);
}

JNIEXPORT void JNICALL
Java_com_tinyc_TinycNative_nativeSetIO(JNIEnv *env, jclass cls,
                                       jlong jstate, jobject io) {
    (void)cls;
    TCCState *s = STATE(env, jstate);
    StateCtx *c = ctx_find(s, 1);
    if (c == NULL) return;
    if (c->io_ctx.io != NULL) {
        (*env)->DeleteGlobalRef(env, c->io_ctx.io);
        c->io_ctx.io = NULL;
    }
    detach_io(s, c->tcc_io);
    free(c->tcc_io);
    c->tcc_io = NULL;
    if (io == NULL) {
        /* back to the default: real stdio (spec 6.4) */
        tcc_set_io(s, NULL);
        return;
    }
    c->io_ctx.io = (*env)->NewGlobalRef(env, io);
    c->tcc_io = make_tcc_io(&c->io_ctx);
    if (c->tcc_io != NULL) {
        tcc_set_io(s, c->tcc_io);
    }
}

/* ------------------------------------------------------------------ */
/* preprocessor                                                        */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeAddIncludePath(JNIEnv *env, jclass cls,
                                                jlong jstate, jstring pathname) {
    (void)cls;
    char *p = dup_jstring(env, pathname);
    int rc = tcc_add_include_path(STATE(env, jstate), p);
    free(p);
    return rc;
}

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeAddSysincludePath(JNIEnv *env, jclass cls,
                                                   jlong jstate, jstring pathname) {
    (void)cls;
    char *p = dup_jstring(env, pathname);
    int rc = tcc_add_sysinclude_path(STATE(env, jstate), p);
    free(p);
    return rc;
}

JNIEXPORT void JNICALL
Java_com_tinyc_TinycNative_nativeDefineSymbol(JNIEnv *env, jclass cls,
                                              jlong jstate, jstring sym, jstring value) {
    (void)cls;
    char *s = dup_jstring(env, sym);
    char *v = dup_jstring(env, value); /* NULL stays NULL */
    tcc_define_symbol(STATE(env, jstate), s, v);
    free(s);
    free(v);
}

JNIEXPORT void JNICALL
Java_com_tinyc_TinycNative_nativeUndefineSymbol(JNIEnv *env, jclass cls,
                                                jlong jstate, jstring sym) {
    (void)cls;
    char *s = dup_jstring(env, sym);
    tcc_undefine_symbol(STATE(env, jstate), s);
    free(s);
}

/* ------------------------------------------------------------------ */
/* compiling                                                           */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeAddFile(JNIEnv *env, jclass cls,
                                         jlong jstate, jstring filename) {
    (void)cls;
    TCCState *s = STATE(env, jstate);
    char *f = dup_jstring(env, filename);
    int rc = tcc_add_file(s, f);
    free(f);
    rethrow_state_pending(env, s);
    return rc;
}

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeCompileString(JNIEnv *env, jclass cls,
                                               jlong jstate, jstring source) {
    (void)cls;
    TCCState *s = STATE(env, jstate);
    const char *src = (source != NULL)
        ? (*env)->GetStringUTFChars(env, source, NULL) : NULL;
    int rc = -1;
    if (src != NULL) {
        rc = tcc_compile_string(s, src);
        (*env)->ReleaseStringUTFChars(env, source, src);
    }
    rethrow_state_pending(env, s);
    return rc;
}

/* ------------------------------------------------------------------ */
/* linking and output                                                  */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeAddLibraryPath(JNIEnv *env, jclass cls,
                                                jlong jstate, jstring pathname) {
    (void)cls;
    char *p = dup_jstring(env, pathname);
    int rc = tcc_add_library_path(STATE(env, jstate), p);
    free(p);
    return rc;
}

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeAddLibrary(JNIEnv *env, jclass cls,
                                            jlong jstate, jstring library_name) {
    (void)cls;
    char *n = dup_jstring(env, library_name);
    int rc = tcc_add_library(STATE(env, jstate), n);
    free(n);
    return rc;
}

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeOutputFile(JNIEnv *env, jclass cls,
                                            jlong jstate, jstring filename) {
    (void)cls;
    TCCState *s = STATE(env, jstate);
    char *f = dup_jstring(env, filename);
    int rc = tcc_output_file(s, f);
    free(f);
    rethrow_state_pending(env, s);
    return rc;
}

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeRelocate(JNIEnv *env, jclass cls, jlong jstate) {
    (void)cls;
    TCCState *s = STATE(env, jstate);
#ifdef TINYC_JNI_NATIVE
    int rc = tcc_relocate(s);
    rethrow_state_pending(env, s);
    return rc;
#else
    /* Cross target: this .so has no tcc_relocate (see
     * TINYC_JNI_NATIVE above). Relocating means running the compiled
     * code in-process, which a cross target can never do on this
     * device. Fail the same way tcc_relocate itself fails (rc -1,
     * which the Java side turns into TinycException). */
    (void)env;
    (void)s;
    return -1;
#endif
}

JNIEXPORT jlong JNICALL
Java_com_tinyc_TinycNative_nativeGetSymbol(JNIEnv *env, jclass cls,
                                           jlong jstate, jstring name) {
    (void)cls;
    char *n = dup_jstring(env, name);
    void *sym = tcc_get_symbol(STATE(env, jstate), n);
    free(n);
    return (jlong)(uintptr_t)sym;
}

/* Register a Java HostFunction as a host symbol (tcc_add_symbol).
 * Returns 0 on success, -2 for an unsupported signature, -3 when the
 * trampoline slot table is full; any other nonzero value is
 * tcc_add_symbol's own failure code. */
JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeAddSymbol(JNIEnv *env, jclass cls,
                                           jlong jstate, jstring name,
                                           jstring signature, jobject fn) {
    (void)cls;
    TCCState *s = STATE(env, jstate);
    char *n = dup_jstring(env, name);
    char *sig = dup_jstring(env, signature);
    int ret_kind = RK_VOID, pk[3] = {0, 0, 0}, np = 0;
    if (n == NULL || sig == NULL
            || host_parse_signature(sig, &ret_kind, pk, &np) != 0) {
        free(n);
        free(sig);
        return -2;
    }
    pthread_mutex_lock(&g_host_lock);
    int slot = -1;
    for (int i = 0; i < TINYC_HOST_SLOTS; i++) {
        if (!g_host_slots[i].used) { slot = i; break; }
    }
    if (slot >= 0) {
        HostSlot *r = &g_host_slots[slot];
        r->used = 1;
        r->state = s;
        r->fn = (*env)->NewGlobalRef(env, fn);
        r->ret_kind = ret_kind;
        r->nparams = np;
        for (int k = 0; k < np; k++) r->param_kinds[k] = pk[k];
    }
    pthread_mutex_unlock(&g_host_lock);
    if (slot < 0) {
        free(n);
        free(sig);
        return -3;
    }
    void *ptr = host_trampoline_for(sig, slot);
    jint rc = -2;
    if (ptr != NULL) {
        rc = (jint)tcc_add_symbol(s, n, ptr);
    }
    if (ptr == NULL || rc != 0) {
        host_slot_clear(env, slot);
        if (ptr == NULL) rc = -2;
    }
    free(n);
    free(sig);
    return rc;
}

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeRun(JNIEnv *env, jclass cls,
                                     jlong jstate, jobjectArray jargv) {
    (void)cls;
    TCCState *s = STATE(env, jstate);
#ifdef TINYC_JNI_NATIVE
    int argc = 0;
    char **argv = build_argv(env, jargv, &argc);
    /* tcc_run runs the compiled program in-process; its exit and crashes
     * belong to the host process (spec 4.3 rule 6) */
    int rc = tcc_run(s, argc, argv);
    free_argv(argv, argc);
    rethrow_state_pending(env, s);
    return rc;
#else
    /* Cross target: this .so has no tcc_run (see TINYC_JNI_NATIVE
     * above). Running the compiled program in-process is meaningless
     * for a target that cannot execute on this device; fail the same
     * way tcc_run itself fails (rc -1). */
    (void)env;
    (void)s;
    (void)jargv;
    return -1;
#endif
}

/* ------------------------------------------------------------------ */
/* main entry (command line semantics)                                 */
/* ------------------------------------------------------------------ */

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeRunCli(JNIEnv *env, jclass cls, jobjectArray jargv) {
    (void)cls;
    int argc = 0;
    char **argv = build_argv(env, jargv, &argc);
    int rc = tinyc_main(argc, argv);
    free_argv(argv, argc);
    return rc;
}

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeRunCliWithIO(JNIEnv *env, jclass cls,
                                              jobjectArray jargv, jobject io) {
    (void)cls;
    int argc = 0;
    char **argv = build_argv(env, jargv, &argc);
    IoCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.io = io; /* local ref, valid for the duration of this call */
    TCCIO *tcc_io = make_tcc_io(&ctx);
    int rc = -1;
    if (tcc_io != NULL) {
        rc = tinyc_main_with_io(argc, argv, tcc_io);
        free(tcc_io);
    }
    free_argv(argv, argc);
    if (ctx.pending != NULL) {
        throw_captured(env, &ctx.pending);
    }
    return rc;
}
