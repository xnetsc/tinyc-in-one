/* tinyc_io.h - IO abstraction for the TinyC dual-form kernel.
 *
 * All compiler-owned input/output goes through a TCCIO object.
 * A NULL TCCIO everywhere in this API means the stdio implementation:
 * output goes directly to the process's real stdout/stderr and input
 * comes from the real stdin.
 */
#ifndef TINYC_IO_H
#define TINYC_IO_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TCCIO TCCIO;
struct TCCIO {
    void *ctx;
    /* Return the number of bytes consumed, or -1 on error. */
    int (*write_out)(void *ctx, const char *buf, size_t len);
    int (*write_err)(void *ctx, const char *buf, size_t len);
    /* Return bytes read (0 = EOF), or -1 on error. */
    int (*read_in)(void *ctx, char *buf, size_t cap);
    void (*flush)(void *ctx);
    /* Internal: release ctx. Set by constructors, used by tcc_io_free. */
    void (*destroy_ctx)(void *ctx);
    int is_singleton; /* stdio/null: tcc_io_free is a no-op */
};

/* ---- ready-made implementations ------------------------------------ */

/* The default: out -> stdout, err -> stderr, in <- stdin. Singleton. */
TCCIO *tcc_io_stdio(void);
/* Discards all output, input is always EOF. Singleton. */
TCCIO *tcc_io_null(void);
/* Accumulates out and err in two separate memory buffers. */
TCCIO *tcc_io_buffer(void);
/* Accumulates out and err in one shared buffer, in write order. */
TCCIO *tcc_io_buffer_merged(void);
/* Writes to files it opens itself (closed by tcc_io_free).
 * err_path == NULL sends err to the same file as out. */
TCCIO *tcc_io_file(const char *out_path, const char *err_path);
/* Writes to borrowed file descriptors (never closed by tcc_io_free).
 * A descriptor of -1 leaves that channel on stdio. */
TCCIO *tcc_io_fd(int out_fd, int err_fd, int in_fd);
/* Writes to borrowed FILE* streams (never closed by tcc_io_free).
 * A NULL stream leaves that channel on stdio. */
TCCIO *tcc_io_FILE(FILE *out, FILE *err, FILE *in);
/* Fully custom callbacks. Any callback may be NULL (channel = stdio). */
TCCIO *tcc_io_callback(int (*write_out)(void *, const char *, size_t),
                       int (*write_err)(void *, const char *, size_t),
                       int (*read_in)(void *, char *, size_t),
                       void *ctx);
/* Sends every channel to both a and b. Takes ownership of neither. */
TCCIO *tcc_io_tee(TCCIO *a, TCCIO *b);

void tcc_io_free(TCCIO *io);

/* Buffer implementations only: channel 0 = out, 1 = err.
 * For the merged buffer both channels return the same data. */
const char *tcc_io_buffer_data(TCCIO *io, int channel, size_t *len);
void tcc_io_buffer_clear(TCCIO *io);

/* ---- current IO (used by the compiler's own output paths) ---------- */

/* Install io as the current IO for subsequent compiler output.
 * NULL restores the stdio default. */
void tinyc_set_io(TCCIO *io);
TCCIO *tinyc_get_io(void);

/* Write helpers honoring the current IO (stdio when none is set). */
int tinyc_io_write_out(const char *buf, size_t len);
int tinyc_io_write_err(const char *buf, size_t len);
void tinyc_io_out_str(const char *s);
void tinyc_io_err_str(const char *s);
void tinyc_io_out_fmt(const char *fmt, ...);
void tinyc_io_err_line(const char *s); /* writes s + "\n" to err */
void tinyc_io_flush_all(void);

/* On Linux: a FILE* whose writes forward to the current IO's out
 * channel, or NULL when the current IO is the stdio default. */
FILE *tinyc_io_open_out_stream(void);

/* ---- -run stdio redirection ----------------------------------------- */

/* Called by tcc_run around the run program's entry point. When the
 * current IO is a non-stdio TCCIO, the process's fd 0/1/2 are
 * redirected through it for the duration of the run: fd 0 is fed the
 * bytes read_in returns (then EOF), and what the program writes to
 * fd 1/2 is delivered to write_out/write_err, per channel, when the
 * run ends. The host's fds are restored by tinyc_run_redirect_end.
 * With no IO installed, or the stdio IO, both functions are no-ops
 * and the run program uses the process's real stdio.
 * keep_stdin != 0 leaves fd 0 alone (tcc_run has already reopened
 * stdin from a file for the program). Nested calls are counted:
 * only the outermost pair redirects. */
void tinyc_run_redirect_begin(int keep_stdin);
void tinyc_run_redirect_end(void);

/* ---- run-path diagnostic markers ------------------------------------ */

/* Append one line to the platform diagnostic markers file
 * (tinyc_init.markers on Windows). The run path calls these at its
 * internal stage boundaries so that a crash inside tcc_run leaves
 * the last completed stage on record. Only the Windows image has
 * a markers file; everywhere else these are no-ops. */
void tinyc_diag_mark(const char *line);
void tinyc_diag_mark_ptr(const char *label, const void *p);

/* ---- dual-form entry points ----------------------------------------- */

/* The compiler's real main, exported from the dual-form file. */
int tinyc_main(int argc, char **argv);
/* tinyc_main with an IO installed for the duration of the call.
 * The previous IO is restored before returning. */
int tinyc_main_io(int argc, char **argv, TCCIO *io);

#ifdef __cplusplus
}
#endif
#endif /* TINYC_IO_H */
