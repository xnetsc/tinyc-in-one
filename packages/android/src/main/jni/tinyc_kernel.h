/*
 * tinyc_kernel.h - declarations of the dual-form kernel additions that the
 * Android JNI glue links against. The signatures mirror the spec:
 *   - TCCIO / tcc_set_io / tcc_set_io_channel : spec 6.1
 *   - tcc_io_stdio                            : spec 6.3
 *   - tinyc_main / tinyc_main_with_io         : spec 4 (main entry; the
 *     _with_io form is the library-mode main taking a host supplied IO,
 *     spec 6.1 "library mode uses the host supplied IO")
 *
 * These are provided by the dual-form kernel build (the Android variant of
 * libtinyc.so). When the patched libtcc.h in the kernel tree already carries
 * the same declarations, drop this header and include libtcc.h only.
 */
#ifndef TINYC_KERNEL_H
#define TINYC_KERNEL_H

#include <stddef.h>
#include "libtcc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* IO channel ids for tcc_set_io_channel */
#define TCC_IO_OUT 0
#define TCC_IO_ERR 1
#define TCC_IO_IN  2

typedef struct TCCIO {
    void *ctx;
    int  (*write_out)(void *ctx, const char *buf, size_t len);
    int  (*write_err)(void *ctx, const char *buf, size_t len);
    int  (*read_in )(void *ctx, char *buf, size_t cap);
    void (*flush)(void *ctx);
} TCCIO;

void tcc_set_io(TCCState *s, TCCIO *io);
void tcc_set_io_channel(TCCState *s, int channel, TCCIO *io);

/* built-in stdio implementation (default when no IO is set, spec 6.4) */
TCCIO *tcc_io_stdio(void);

/* main entry of the dual-form file, exported alongside main (spec 4) */
int tinyc_main(int argc, char **argv);
int tinyc_main_with_io(int argc, char **argv, TCCIO *io);

#ifdef __cplusplus
}
#endif

#endif /* TINYC_KERNEL_H */
