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

/* The adapter installs a state's IO as the kernel's process-wide
 * current IO, and the kernel registry does not track ownership: if
 * the IO being freed here is the installed one, uninstall it first.
 * Left installed, the freed object dangles and the next current-IO
 * dispatch (e.g. an error print on another state) calls through
 * freed memory. */
static void uninstall_if_current(TCCIO *io) {
    if (io != NULL && tinyc_get_io() == io) {
        tinyc_set_io(tcc_io_stdio());
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
    uninstall_if_current(c->tcc_io);
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
    uninstall_if_current(c->tcc_io);
    free(c->tcc_io);
    c->tcc_io = NULL;
    if (io == NULL) {
        /* back to the default: real stdio (spec 6.4) */
        tcc_set_io(s, tcc_io_stdio());
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
    int rc = tcc_relocate(s);
    rethrow_state_pending(env, s);
    return rc;
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

JNIEXPORT jint JNICALL
Java_com_tinyc_TinycNative_nativeRun(JNIEnv *env, jclass cls,
                                     jlong jstate, jobjectArray jargv) {
    (void)cls;
    TCCState *s = STATE(env, jstate);
    int argc = 0;
    char **argv = build_argv(env, jargv, &argc);
    /* tcc_run runs the compiled program in-process; its exit and crashes
     * belong to the host process (spec 4.3 rule 6) */
    int rc = tcc_run(s, argc, argv);
    free_argv(argv, argc);
    rethrow_state_pending(env, s);
    return rc;
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
