/* tcc_io.c - TCCIO implementations and the current-IO registry. */
#if defined(__ANDROID__) && !defined(_BSD_SOURCE)
/* bionic declares funopen (used by tinyc_io_open_out_stream below)
 * only under __USE_BSD, which _BSD_SOURCE enables. */
# define _BSD_SOURCE 1
#endif
#include "tinyc_io.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if defined(__GLIBC__)
#include <stdio_ext.h> /* __fpurge */
#endif

/* ------------------------------------------------------------------ */
/* registry                                                            */

static TCCIO *g_current_io;

void tinyc_set_io(TCCIO *io) { g_current_io = io; }
TCCIO *tinyc_get_io(void) { return g_current_io; }

static TCCIO *eff_io(void)
{
    return g_current_io ? g_current_io : tcc_io_stdio();
}

int tinyc_io_write_out(const char *buf, size_t len)
{
    TCCIO *io = eff_io();
    return io->write_out ? io->write_out(io->ctx, buf, len) : (int)len;
}

int tinyc_io_write_err(const char *buf, size_t len)
{
    TCCIO *io = eff_io();
    return io->write_err ? io->write_err(io->ctx, buf, len) : (int)len;
}

void tinyc_io_out_str(const char *s)
{
    if (s) tinyc_io_write_out(s, strlen(s));
}

void tinyc_io_err_str(const char *s)
{
    if (s) tinyc_io_write_err(s, strlen(s));
}

void tinyc_io_err_line(const char *s)
{
    if (s) tinyc_io_write_err(s, strlen(s));
    tinyc_io_write_err("\n", 1);
}

void tinyc_io_out_fmt(const char *fmt, ...)
{
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if ((size_t)n < sizeof buf) {
        tinyc_io_write_out(buf, (size_t)n);
        return;
    }
    char *big = malloc((size_t)n + 1);
    if (!big) return;
    va_start(ap, fmt);
    vsnprintf(big, (size_t)n + 1, fmt, ap);
    va_end(ap);
    tinyc_io_write_out(big, (size_t)n);
    free(big);
}

#ifdef _WIN32
/* Defined with the Windows tinyc_io_open_out_stream below: delivers
 * any captured -E output that is still sitting in its temp file. */
static void tinyc_io_win_drain_out_streams(void);
#endif

void tinyc_io_flush_all(void)
{
#ifdef _WIN32
    tinyc_io_win_drain_out_streams();
#endif
    TCCIO *io = eff_io();
    if (io->flush) io->flush(io->ctx);
    else { fflush(stdout); fflush(stderr); }
}

/* ------------------------------------------------------------------ */
/* helpers                                                             */

static TCCIO *io_alloc(void *ctx,
                       int (*wo)(void *, const char *, size_t),
                       int (*we)(void *, const char *, size_t),
                       int (*ri)(void *, char *, size_t),
                       void (*fl)(void *),
                       void (*destroy)(void *))
{
    TCCIO *io = calloc(1, sizeof *io);
    if (!io) return NULL;
    io->ctx = ctx;
    io->write_out = wo;
    io->write_err = we;
    io->read_in = ri;
    io->flush = fl;
    io->destroy_ctx = destroy;
    return io;
}

void tcc_io_free(TCCIO *io)
{
    if (!io || io->is_singleton) return;
    if (io->flush) io->flush(io->ctx);
    if (io->destroy_ctx) io->destroy_ctx(io->ctx);
    free(io);
}

static int stdio_write(FILE *f, const char *buf, size_t len)
{
    return (int)fwrite(buf, 1, len, f);
}

/* ------------------------------------------------------------------ */
/* stdio                                                               */

static int stdio_wo(void *ctx, const char *b, size_t n) { (void)ctx; return stdio_write(stdout, b, n); }
static int stdio_we(void *ctx, const char *b, size_t n) { (void)ctx; return stdio_write(stderr, b, n); }
static int stdio_ri(void *ctx, char *b, size_t n) { (void)ctx; size_t r = fread(b, 1, n, stdin); return (int)r; }
static void stdio_fl(void *ctx) { (void)ctx; fflush(stdout); fflush(stderr); }

static TCCIO g_stdio_io = { NULL, stdio_wo, stdio_we, stdio_ri, stdio_fl, NULL, 1 };
TCCIO *tcc_io_stdio(void) { return &g_stdio_io; }

/* ------------------------------------------------------------------ */
/* null                                                                */

static int null_w(void *ctx, const char *b, size_t n) { (void)ctx; (void)b; return (int)n; }
static int null_r(void *ctx, char *b, size_t n) { (void)ctx; (void)b; (void)n; return 0; }

static TCCIO g_null_io = { NULL, null_w, null_w, null_r, NULL, NULL, 1 };
TCCIO *tcc_io_null(void) { return &g_null_io; }

/* ------------------------------------------------------------------ */
/* buffer                                                              */

typedef struct {
    char *out; size_t out_len, out_cap;
    char *err; size_t err_len, err_cap;
    int merged;
} BufCtx;

static int buf_append(char **p, size_t *len, size_t *cap, const char *b, size_t n)
{
    if (*len + n + 1 > *cap) {
        size_t nc = (*cap ? *cap : 256);
        while (*len + n + 1 > nc) nc *= 2;
        char *np = realloc(*p, nc);
        if (!np) return -1;
        *p = np; *cap = nc;
    }
    memcpy(*p + *len, b, n);
    *len += n;
    (*p)[*len] = '\0';
    return (int)n;
}

static int buf_wo(void *c, const char *b, size_t n)
{
    BufCtx *x = c; return buf_append(&x->out, &x->out_len, &x->out_cap, b, n);
}
static int buf_we(void *c, const char *b, size_t n)
{
    BufCtx *x = c;
    if (x->merged) return buf_append(&x->out, &x->out_len, &x->out_cap, b, n);
    return buf_append(&x->err, &x->err_len, &x->err_cap, b, n);
}
static void buf_destroy(void *c)
{
    BufCtx *x = c; free(x->out); free(x->err); free(x);
}

static TCCIO *buffer_impl(int merged)
{
    BufCtx *x = calloc(1, sizeof *x);
    if (!x) return NULL;
    x->merged = merged;
    return io_alloc(x, buf_wo, buf_we, null_r, NULL, buf_destroy);
}

TCCIO *tcc_io_buffer(void) { return buffer_impl(0); }
TCCIO *tcc_io_buffer_merged(void) { return buffer_impl(1); }

const char *tcc_io_buffer_data(TCCIO *io, int channel, size_t *len)
{
    if (!io || io->write_out != buf_wo) { if (len) *len = 0; return ""; }
    BufCtx *x = io->ctx;
    if (channel == 0 || x->merged) {
        if (len) *len = x->out_len;
        return x->out ? x->out : "";
    }
    if (len) *len = x->err_len;
    return x->err ? x->err : "";
}

void tcc_io_buffer_clear(TCCIO *io)
{
    if (!io || io->write_out != buf_wo) return;
    BufCtx *x = io->ctx;
    x->out_len = 0; if (x->out) x->out[0] = '\0';
    x->err_len = 0; if (x->err) x->err[0] = '\0';
}

/* ------------------------------------------------------------------ */
/* fd (borrowed descriptors)                                           */

typedef struct { int out_fd, err_fd, in_fd; } FdCtx;

static int fd_write(int fd, FILE *fallback, const char *b, size_t n)
{
    if (fd < 0) return stdio_write(fallback, b, n);
    size_t done = 0;
    while (done < n) {
        ssize_t w = write(fd, b + done, n - done);
        if (w < 0) return -1;
        done += (size_t)w;
    }
    return (int)n;
}
static int fd_wo(void *c, const char *b, size_t n) { return fd_write(((FdCtx *)c)->out_fd, stdout, b, n); }
static int fd_we(void *c, const char *b, size_t n) { return fd_write(((FdCtx *)c)->err_fd, stderr, b, n); }
static int fd_ri(void *c, char *b, size_t n)
{
    FdCtx *x = c;
    if (x->in_fd < 0) return stdio_ri(NULL, b, n);
    ssize_t r = read(x->in_fd, b, n);
    return r < 0 ? -1 : (int)r;
}

TCCIO *tcc_io_fd(int out_fd, int err_fd, int in_fd)
{
    FdCtx *x = malloc(sizeof *x);
    if (!x) return NULL;
    x->out_fd = out_fd; x->err_fd = err_fd; x->in_fd = in_fd;
    return io_alloc(x, fd_wo, fd_we, fd_ri, NULL, free);
}

/* ------------------------------------------------------------------ */
/* FILE* (borrowed streams) and file paths (owned streams)             */

typedef struct { FILE *out, *err, *in; int owned; } FileCtx;

static int file_wo(void *c, const char *b, size_t n)
{
    FileCtx *x = c; return stdio_write(x->out ? x->out : stdout, b, n);
}
static int file_we(void *c, const char *b, size_t n)
{
    FileCtx *x = c; return stdio_write(x->err ? x->err : stderr, b, n);
}
static int file_ri(void *c, char *b, size_t n)
{
    FileCtx *x = c; return (int)fread(b, 1, n, x->in ? x->in : stdin);
}
static void file_fl(void *c)
{
    FileCtx *x = c;
    if (x->out) fflush(x->out);
    if (x->err && x->err != x->out) fflush(x->err);
}
static void file_destroy(void *c)
{
    FileCtx *x = c;
    if (x->owned) {
        if (x->out) fclose(x->out);
        if (x->err && x->err != x->out) fclose(x->err);
        if (x->in) fclose(x->in);
    }
    free(x);
}

TCCIO *tcc_io_FILE(FILE *out, FILE *err, FILE *in)
{
    FileCtx *x = calloc(1, sizeof *x);
    if (!x) return NULL;
    x->out = out; x->err = err; x->in = in; x->owned = 0;
    return io_alloc(x, file_wo, file_we, file_ri, file_fl, file_destroy);
}

TCCIO *tcc_io_file(const char *out_path, const char *err_path)
{
    FileCtx *x = calloc(1, sizeof *x);
    if (!x) return NULL;
    x->owned = 1;
    x->out = out_path ? fopen(out_path, "wb") : NULL;
    if (out_path && !x->out) { free(x); return NULL; }
    if (err_path) {
        x->err = fopen(err_path, "wb");
        if (!x->err) { if (x->out) fclose(x->out); free(x); return NULL; }
    } else {
        x->err = x->out;
    }
    return io_alloc(x, file_wo, file_we, file_ri, file_fl, file_destroy);
}

/* ------------------------------------------------------------------ */
/* callback                                                            */

typedef struct {
    int (*wo)(void *, const char *, size_t);
    int (*we)(void *, const char *, size_t);
    int (*ri)(void *, char *, size_t);
    void *uctx;
} CbCtx;

static int cb_wo(void *c, const char *b, size_t n)
{
    CbCtx *x = c; return x->wo ? x->wo(x->uctx, b, n) : stdio_wo(NULL, b, n);
}
static int cb_we(void *c, const char *b, size_t n)
{
    CbCtx *x = c; return x->we ? x->we(x->uctx, b, n) : stdio_we(NULL, b, n);
}
static int cb_ri(void *c, char *b, size_t n)
{
    CbCtx *x = c; return x->ri ? x->ri(x->uctx, b, n) : 0;
}

TCCIO *tcc_io_callback(int (*wo)(void *, const char *, size_t),
                       int (*we)(void *, const char *, size_t),
                       int (*ri)(void *, char *, size_t),
                       void *ctx)
{
    CbCtx *x = malloc(sizeof *x);
    if (!x) return NULL;
    x->wo = wo; x->we = we; x->ri = ri; x->uctx = ctx;
    return io_alloc(x, cb_wo, cb_we, cb_ri, NULL, free);
}

/* ------------------------------------------------------------------ */
/* tee                                                                 */

typedef struct { TCCIO *a, *b; } TeeCtx;

static int tee_call(TCCIO *io, int which, const char *b, size_t n)
{
    if (!io) return (int)n;
    int (*f)(void *, const char *, size_t) =
        which == 0 ? io->write_out : which == 1 ? io->write_err : NULL;
    if (!f) return (int)n;
    return f(io->ctx, b, n);
}
static int tee_wo(void *c, const char *b, size_t n)
{
    TeeCtx *x = c; tee_call(x->b, 0, b, n); return tee_call(x->a, 0, b, n);
}
static int tee_we(void *c, const char *b, size_t n)
{
    TeeCtx *x = c; tee_call(x->b, 1, b, n); return tee_call(x->a, 1, b, n);
}
static int tee_ri(void *c, char *b, size_t n)
{
    TeeCtx *x = c;
    if (x->a && x->a->read_in) return x->a->read_in(x->a->ctx, b, n);
    return 0;
}
static void tee_fl(void *c)
{
    TeeCtx *x = c;
    if (x->a && x->a->flush) x->a->flush(x->a->ctx);
    if (x->b && x->b->flush) x->b->flush(x->b->ctx);
}

TCCIO *tcc_io_tee(TCCIO *a, TCCIO *b)
{
    TeeCtx *x = malloc(sizeof *x);
    if (!x) return NULL;
    x->a = a; x->b = b;
    return io_alloc(x, tee_wo, tee_we, tee_ri, tee_fl, free);
}

/* ------------------------------------------------------------------ */
/* out stream as FILE* (for the preprocessor output path)              */

#if defined(__GLIBC__)
static ssize_t cookie_write(void *c, const char *b, size_t n)
{
    (void)c;
    int r = tinyc_io_write_out(b, n);
    return r < 0 ? -1 : r;
}

FILE *tinyc_io_open_out_stream(void)
{
    if (!g_current_io || g_current_io == &g_stdio_io) return NULL;
    cookie_io_functions_t fns = { .read = NULL, .write = cookie_write,
                                  .seek = NULL, .close = NULL };
    return fopencookie(NULL, "w", fns);
}
#elif defined(__APPLE__) || defined(__ANDROID__)
/* macOS and Android (bionic, API 24+) both provide BSD funopen with
 * this callback shape; bionic has no fopencookie. */
static int funopen_write(void *c, const char *b, int n)
{
    (void)c;
    int r = tinyc_io_write_out(b, (size_t)n);
    return r < 0 ? -1 : r;
}

FILE *tinyc_io_open_out_stream(void)
{
    if (!g_current_io || g_current_io == &g_stdio_io) return NULL;
    return funopen(NULL, NULL, funopen_write, NULL, NULL);
}
#elif defined(_WIN32)
/* MSVCRT/MinGW has neither fopencookie nor funopen, and a plain FILE*
 * offers no write or close hook, so this branch cannot forward on
 * write the way the branches above do. The returned FILE* instead
 * writes to a temporary file: the preprocessor's writes reach the
 * file as its stdio buffer flushes, and in full at the caller's
 * fclose (the only close the caller performs, in tinyc_main after
 * tcc_delete). tinyc_io_flush_all drains the file to the current
 * IO's out channel, and tinyc_main_io calls tinyc_io_flush_all after
 * tinyc_main returns - after that fclose - so by the time a -E call
 * returns, the captured output has been delivered in full, the same
 * observable state the synchronous branches leave behind.
 * The writer handle shares read access, so a drain while the stream
 * is still open (tinyc_io_flush_all is also called from the error
 * path mid-compile) delivers the bytes flushed so far, in order, and
 * keeps the entry. A drain finds the stream finished when an
 * exclusive open succeeds, which the writer's share mode refuses
 * while its FILE* is still open; that drain delivers the rest,
 * deletes the file and drops the entry. Delivery is plain file IO
 * in the caller's thread - no reader thread, no pipe - so a drain
 * never waits on the writer at any output size. */
#include <fcntl.h>
#include <io.h>
#include <stdint.h>
#include <windows.h>

typedef struct WinOutStream {
    struct WinOutStream *next;
    char path[MAX_PATH];
    long long delivered; /* bytes already passed to tinyc_io_write_out */
} WinOutStream;

static WinOutStream *g_win_out_streams;

FILE *tinyc_io_open_out_stream(void)
{
    if (!g_current_io || g_current_io == &g_stdio_io) return NULL;

    char dir[MAX_PATH], path[MAX_PATH];
    DWORD n = GetTempPathA(MAX_PATH, dir);
    if (n == 0 || n > MAX_PATH) return NULL;
    if (GetTempFileNameA(dir, "tio", 0, path) == 0) return NULL;

    HANDLE hw = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hw == INVALID_HANDLE_VALUE) { DeleteFileA(path); return NULL; }
    int fd = _open_osfhandle((intptr_t)hw, _O_BINARY);
    if (fd < 0) { CloseHandle(hw); DeleteFileA(path); return NULL; }
    FILE *fp = _fdopen(fd, "wb");
    if (!fp) { _close(fd); DeleteFileA(path); return NULL; }

    WinOutStream *e = malloc(sizeof *e);
    if (!e) { fclose(fp); DeleteFileA(path); return NULL; }
    strcpy(e->path, path);
    e->delivered = 0;
    e->next = g_win_out_streams;
    g_win_out_streams = e;
    return fp;
}

static void tinyc_io_win_drain_out_streams(void)
{
    WinOutStream **link = &g_win_out_streams;
    while (*link) {
        WinOutStream *e = *link;
        int closed = 0;
        HANDLE probe = CreateFileA(e->path, GENERIC_READ, 0, NULL,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                   NULL);
        if (probe != INVALID_HANDLE_VALUE) {
            closed = 1;
            CloseHandle(probe);
        }
        HANDLE hr = CreateFileA(e->path, GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                NULL);
        if (hr != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER pos;
            pos.QuadPart = e->delivered;
            if (SetFilePointerEx(hr, pos, NULL, FILE_BEGIN)) {
                char buf[8192];
                for (;;) {
                    DWORD got = 0;
                    if (!ReadFile(hr, buf, sizeof buf, &got, NULL) || got == 0)
                        break;
                    int w = tinyc_io_write_out(buf, got);
                    if (w < 0)
                        break;
                    e->delivered += w;
                    if (w < (int)got || got < sizeof buf)
                        break;
                }
            }
            CloseHandle(hr);
        }
        if (closed) {
            DeleteFileA(e->path);
            *link = e->next;
            free(e);
        } else {
            link = &e->next;
        }
    }
}
#else
FILE *tinyc_io_open_out_stream(void) { return NULL; }
#endif

/* ------------------------------------------------------------------ */
/* Run-path diagnostic markers: only the Windows image has a        */
/* markers file; everywhere else these are no-ops. The Windows      */
/* implementation lives here because tcc_io.o is the one project    */
/* object present in every link that contains the run path - the    */
/* upstream Makefile's object lists (LIBTCC_OBJ / TCC_FILES) and    */
/* the dual-form link in build.sh alike - while tinyc_win.c is      */
/* only in the dual-form link. The writer appends one line to       */
/* tinyc_init.markers per call with plain Win32 file calls and      */
/* stays silent on failure. Every call site (tcc_run, the PE_RUN    */
/* import builder) is reached only after the image's imports are    */
/* bound - by the loader in a process image, by tinyc_init in the   */
/* library form - so imported calls are safe here; the init-stage   */
/* writer in tinyc_win.c runs before the binding and resolves its   */
/* file pointers by hand instead.                                   */

#ifdef _WIN32
#include <stdint.h>
#include <windows.h>

static void tinyc_diag_write_line(const char *line)
{
    HANDLE h;
    DWORD written;
    h = CreateFileA("tinyc_init.markers", FILE_APPEND_DATA,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;
    WriteFile(h, line, (DWORD)strlen(line), &written, NULL);
    WriteFile(h, "\n", 1, &written, NULL);
    CloseHandle(h);
}

void tinyc_diag_mark(const char *line)
{
    tinyc_diag_write_line(line);
}

void tinyc_diag_mark_ptr(const char *label, const void *p)
{
    char buf[96];
    snprintf(buf, sizeof buf, "%s=0x%016llx", label,
             (unsigned long long)(uintptr_t)p);
    tinyc_diag_write_line(buf);
}
#else
void tinyc_diag_mark(const char *line) { (void)line; }
void tinyc_diag_mark_ptr(const char *label, const void *p)
{
    (void)label; (void)p;
}
#endif

/* ------------------------------------------------------------------ */
/* -run stdio redirection                                              */
/*                                                                     */
/* The host's global stdio is never replaced; for the duration of one  */
/* run the process's fd 0/1/2 are pointed at temporary files. fd 0's   */
/* file is pre-filled from the current IO's read_in, fd 1/2's files    */
/* are drained into write_out/write_err after the run, and the real    */
/* fds are restored. Temporary files (not pipes) are what make this    */
/* deadlock-free at any output size: the program can write without a   */
/* concurrent reader, and its stdin is fully materialized before it    */
/* starts. One file per channel, so out and err never interleave.      */
/* On Windows both functions are no-ops.                               */

#ifdef _WIN32

void tinyc_run_redirect_begin(int keep_stdin) { (void)keep_stdin; }
void tinyc_run_redirect_end(void) {}

#else

typedef struct {
    int depth;          /* nesting count; the redirect lives at depth 1 */
    TCCIO *io;          /* IO captured at the outermost begin */
    int saved[3];       /* dup of the original fd; -1 = channel untouched,
                           -2 = the fd was closed at begin */
    FILE *tmp[3];       /* per-channel temporary file, NULL if unused */
} RunRedir;

static RunRedir g_run_redir;

/* Point fd at tmp's file, remembering how to restore it. */
static void redir_install(RunRedir *r, int fd, FILE *tmp)
{
    int saved = dup(fd);
    r->saved[fd] = saved >= 0 ? saved : -2;
    if (dup2(fileno(tmp), fd) < 0) {
        if (saved >= 0) close(saved);
        r->saved[fd] = -1;
    }
}

void tinyc_run_redirect_begin(int keep_stdin)
{
    RunRedir *r = &g_run_redir;
    if (r->depth > 0) { r->depth++; return; }
    TCCIO *io = g_current_io;
    if (!io || io == &g_stdio_io) return;

    /* Output buffered so far belongs to the real fds / the IO itself,
     * not to the run program's channels. */
    tinyc_io_flush_all();
    fflush(stdout);
    fflush(stderr);

    FILE *out_tmp = tmpfile();
    FILE *err_tmp = tmpfile();
    if (!out_tmp || !err_tmp) {
        if (out_tmp) fclose(out_tmp);
        if (err_tmp) fclose(err_tmp);
        return;
    }
    r->io = io;
    r->tmp[0] = NULL;
    r->tmp[1] = out_tmp;
    r->tmp[2] = err_tmp;
    r->saved[0] = r->saved[1] = r->saved[2] = -1;
    redir_install(r, 1, out_tmp);
    redir_install(r, 2, err_tmp);

    if (!keep_stdin) {
        FILE *in_tmp = tmpfile();
        if (in_tmp) {
            /* Drain read_in up front; the program then reads the
             * file to EOF at its own pace. A read error ends the
             * input the same way EOF does. A NULL read_in is EOF. */
            char buf[8192];
            for (;;) {
                int n = io->read_in ? io->read_in(io->ctx, buf, sizeof buf) : 0;
                if (n <= 0) break;
                if (fwrite(buf, 1, (size_t)n, in_tmp) != (size_t)n) break;
            }
            rewind(in_tmp);
            r->tmp[0] = in_tmp;
            /* The stdin FILE still carries the previous fd 0's state:
             * a sticky EOF flag and possibly bytes buffered from it.
             * Both belong to the old stream, not to read_in's bytes;
             * reset the FILE before the program reads through it. */
            clearerr(stdin);
#if defined(__GLIBC__)
            __fpurge(stdin);
#endif
            redir_install(r, 0, in_tmp);
        }
    }
    r->depth = 1;
}

void tinyc_run_redirect_end(void)
{
    RunRedir *r = &g_run_redir;
    if (r->depth == 0) return;
    if (--r->depth > 0) return;

    /* The run program's stdio buffers empty into the capture files
     * while the fds still point at them. */
    fflush(stdout);
    fflush(stderr);

    int fd;
    for (fd = 0; fd < 3; fd++) {
        if (r->saved[fd] >= 0) {
            dup2(r->saved[fd], fd);
            close(r->saved[fd]);
        } else if (r->saved[fd] == -2) {
            close(fd);
        }
        r->saved[fd] = -1;
    }
    if (r->tmp[0]) {
        /* Leave the host a clean stdin FILE: drop the run program's
         * EOF flag and any input it left buffered. */
        clearerr(stdin);
#if defined(__GLIBC__)
        __fpurge(stdin);
#endif
    }

    /* Deliver each captured channel through the IO, now that the real
     * fds are back (an IO channel that falls back to stdio writes to
     * the real stream). A NULL channel function discards, matching
     * tinyc_io_write_out/err. */
    TCCIO *io = r->io;
    char buf[8192];
    size_t n;
    if (r->tmp[1] && io->write_out) {
        rewind(r->tmp[1]);
        while ((n = fread(buf, 1, sizeof buf, r->tmp[1])) > 0)
            io->write_out(io->ctx, buf, n);
    }
    if (r->tmp[2] && io->write_err) {
        rewind(r->tmp[2]);
        while ((n = fread(buf, 1, sizeof buf, r->tmp[2])) > 0)
            io->write_err(io->ctx, buf, n);
    }
    for (fd = 0; fd < 3; fd++) {
        if (r->tmp[fd]) fclose(r->tmp[fd]);
        r->tmp[fd] = NULL;
    }
    r->io = NULL;
    if (io && io->flush) io->flush(io->ctx);
}

#endif /* _WIN32 */

/* ------------------------------------------------------------------ */
/* main entry with IO                                                  */

int tinyc_main_io(int argc, char **argv, TCCIO *io)
{
    TCCIO *prev = g_current_io;
    g_current_io = io;
    int ret = tinyc_main(argc, argv);
    tinyc_io_flush_all();
    g_current_io = prev;
    return ret;
}
