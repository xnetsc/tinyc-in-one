/* tcc_io.c - TCCIO implementations and the current-IO registry. */
#include "tinyc_io.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

void tinyc_io_flush_all(void)
{
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

#if defined(__linux__)
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
#else
FILE *tinyc_io_open_out_stream(void) { return NULL; }
#endif

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
