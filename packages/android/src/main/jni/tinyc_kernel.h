/*
 * tinyc_kernel.h - declarations of the dual-form kernel additions the
 * Android JNI glue links against:
 *   - TCCIO / tcc_set_io / tcc_get_io : libtcc.h (the kernel's IO is
 *     per-state; tcc_set_io installs a TCCIO on one TCCState and all
 *     of that state's compiler output goes through it)
 *   - tcc_set_io_channel              : spec 6.1 channel form, kept in
 *     the Android variant's adapter (tinyc_kernel_adapter.c); a TCCIO
 *     bundles all channels, so it installs the whole object on the
 *     state like tcc_set_io
 *   - tcc_io_stdio                    : spec 6.3
 *   - tinyc_main / tinyc_main_with_io : spec 4 (main entry; the
 *     _with_io form is the library-mode main taking a host supplied
 *     IO, spec 6.1 "library mode uses the host supplied IO")
 *
 * TCCIO is the kernel's own struct, taken from tinyc_io.h (the kernel
 * tree's tcc.h includes it); it is not redefined here.
 */
#ifndef TINYC_KERNEL_H
#define TINYC_KERNEL_H

#include <stddef.h>
#include "libtcc.h"
#include "tinyc_io.h"

#ifdef __cplusplus
extern "C" {
#endif

/* IO channel ids for tcc_set_io_channel */
#define TCC_IO_OUT 0
#define TCC_IO_ERR 1
#define TCC_IO_IN  2

void tcc_set_io_channel(TCCState *s, int channel, TCCIO *io);

/* main entry of the dual-form file, exported alongside main (spec 4) */
int tinyc_main(int argc, char **argv);
int tinyc_main_with_io(int argc, char **argv, TCCIO *io);

#ifdef __cplusplus
}
#endif

#endif /* TINYC_KERNEL_H */
