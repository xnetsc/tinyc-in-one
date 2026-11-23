/* Unit tests for the TCCIO implementations (core/tcc_io.c).
 * Build: gcc -I core tests/test_io.c core/tcc_io.c -o test_io
 * tcc_io.c's tinyc_main_io references tinyc_main; a stub stands in. */
#include "tinyc_io.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int tinyc_main(int argc, char **argv) { (void)argc; (void)argv; return 5; }

static char acc[128];
static size_t acc_len;
static int acc_write(void *ctx, const char *buf, size_t len)
{
    (void)ctx;
    memcpy(acc + acc_len, buf, len);
    acc_len += len;
    acc[acc_len] = 0;
    return (int)len;
}

static int failures;
#define CHECK(cond) do { \
    if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } \
} while (0)

static void w(TCCIO *io, int err, const char *s)
{
    if (err) io->write_err(io->ctx, s, strlen(s));
    else io->write_out(io->ctx, s, strlen(s));
}

static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    static char buf[8192];
    size_t n = fread(buf, 1, sizeof buf - 1, f);
    buf[n] = 0;
    fclose(f);
    return buf;
}

int main(void)
{
    size_t n;
    const char *d;

    /* buffer: separate channels */
    TCCIO *b = tcc_io_buffer();
    w(b, 0, "out1"); w(b, 1, "err1"); w(b, 0, "out2");
    d = tcc_io_buffer_data(b, 0, &n); CHECK(n == 8 && memcmp(d, "out1out2", 8) == 0);
    d = tcc_io_buffer_data(b, 1, &n); CHECK(n == 4 && memcmp(d, "err1", 4) == 0);
    tcc_io_buffer_clear(b);
    d = tcc_io_buffer_data(b, 0, &n); CHECK(n == 0);
    tcc_io_free(b);

    /* buffer merged: write order preserved across channels */
    TCCIO *m = tcc_io_buffer_merged();
    w(m, 0, "a"); w(m, 1, "b"); w(m, 0, "c");
    d = tcc_io_buffer_data(m, 0, &n); CHECK(n == 3 && memcmp(d, "abc", 3) == 0);
    d = tcc_io_buffer_data(m, 1, &n); CHECK(n == 3 && memcmp(d, "abc", 3) == 0);
    tcc_io_free(m);

    /* null: accepts everything, reads EOF */
    TCCIO *nl = tcc_io_null();
    CHECK(nl->write_out(nl->ctx, "x", 1) == 1);
    char rb[4]; CHECK(nl->read_in(nl->ctx, rb, sizeof rb) == 0);
    tcc_io_free(nl); /* singleton: free is a no-op, must not crash */

    /* file: owned streams, err merged when err_path is NULL */
    TCCIO *f = tcc_io_file("/tmp/tcc_io_test.log", NULL);
    CHECK(f != NULL);
    w(f, 0, "to-file-out"); w(f, 1, "to-file-err");
    tcc_io_free(f);
    char *content = slurp("/tmp/tcc_io_test.log");
    CHECK(strstr(content, "to-file-out") && strstr(content, "to-file-err"));

    /* fd: borrowed descriptor, writes go to that fd only */
    int pfd[2]; pipe(pfd);
    TCCIO *fdio = tcc_io_fd(pfd[1], pfd[1], -1);
    w(fdio, 0, "via-fd");
    tcc_io_free(fdio); /* must not close pfd[1] */
    close(pfd[1]);
    char pbuf[32] = {0};
    ssize_t rn = read(pfd[0], pbuf, sizeof pbuf - 1);
    CHECK(rn == 6 && memcmp(pbuf, "via-fd", 6) == 0);
    close(pfd[0]);

    /* FILE*: borrowed stream */
    FILE *tf = tmpfile();
    TCCIO *fio = tcc_io_FILE(tf, tf, NULL);
    w(fio, 0, "via-file-star");
    tcc_io_free(fio); /* must not close tf */
    fflush(tf); rewind(tf);
    char fb[32] = {0}; size_t fn = fread(fb, 1, sizeof fb - 1, tf);
    CHECK(fn == 13 && memcmp(fb, "via-file-star", 13) == 0);
    fclose(tf);

    /* callback: custom sink receives bytes */
    acc_len = 0; acc[0] = 0;
    TCCIO *cio = tcc_io_callback(acc_write, acc_write, NULL, NULL);
    CHECK(cio != NULL);
    w(cio, 0, "cb-out"); w(cio, 1, "cb-err");
    CHECK(acc_len == 12 && memcmp(acc, "cb-outcb-err", 12) == 0);
    tcc_io_free(cio);

    /* tee: both sinks receive */
    TCCIO *b1 = tcc_io_buffer(), *b2 = tcc_io_buffer();
    TCCIO *tee = tcc_io_tee(b1, b2);
    w(tee, 0, "tee-data");
    d = tcc_io_buffer_data(b1, 0, &n); CHECK(n == 8 && memcmp(d, "tee-data", 8) == 0);
    d = tcc_io_buffer_data(b2, 0, &n); CHECK(n == 8 && memcmp(d, "tee-data", 8) == 0);
    tcc_io_free(tee); tcc_io_free(b1); tcc_io_free(b2);

    /* current IO registry + helpers + tinyc_main_io */
    TCCIO *cap = tcc_io_buffer();
    tinyc_set_io(cap);
    tinyc_io_out_str("registry-out");
    tinyc_io_err_line("registry-err");
    tinyc_set_io(NULL);
    d = tcc_io_buffer_data(cap, 0, &n); CHECK(n == 12 && memcmp(d, "registry-out", 12) == 0);
    d = tcc_io_buffer_data(cap, 1, &n); CHECK(n == 13 && memcmp(d, "registry-err\n", 13) == 0);
    int rc = tinyc_main_io(1, (char *[]){"tinyc", NULL}, cap);
    CHECK(rc == 5); /* stub main ran under the installed IO */
    tcc_io_free(cap);

    if (failures == 0) printf("test_io: ALL PASS\n");
    return failures ? 1 : 0;
}
