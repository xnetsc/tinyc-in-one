/*
 * tinyc_kernel_adapter.c - Android-variant adapter between the JNI
 * glue's kernel interface (tinyc_kernel.h) and the dual-form kernel's
 * implemented IO API (core/tinyc_io.h).
 *
 * The kernel routes compiler output through a process-wide current IO
 * and library use is serial by design (one compile at a time per
 * process), so:
 *   - tcc_set_io installs the IO as the current IO; the TCCState
 *     argument is accepted for interface compatibility and not used.
 *   - tcc_set_io_channel does the same: a TCCIO bundles all channels,
 *     so routing is per-object, not per-channel.
 *   - tinyc_main_with_io forwards to the kernel's tinyc_main_io.
 *
 * Compiled only into the Android libtinyc.so, together with the
 * kernel unity build and tinyc_jni.c (see build.sh mode native-libs).
 */
#include "tinyc_kernel.h"

void tcc_set_io(TCCState *s, TCCIO *io)
{
    (void)s;
    tinyc_set_io(io);
}

void tcc_set_io_channel(TCCState *s, int channel, TCCIO *io)
{
    (void)s;
    (void)channel;
    tinyc_set_io(io);
}

int tinyc_main_with_io(int argc, char **argv, TCCIO *io)
{
    return tinyc_main_io(argc, argv, io);
}
