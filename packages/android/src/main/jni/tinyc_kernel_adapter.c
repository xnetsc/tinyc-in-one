/*
 * tinyc_kernel_adapter.c - Android-variant adapter between the JNI
 * glue's kernel interface (tinyc_kernel.h) and the dual-form kernel's
 * IO API (core/tinyc_io.h).
 *
 * The kernel's IO is per-state: tcc_set_io / tcc_get_io are libtcc's
 * own (libtcc.h), and tinyc_main_with_io is the kernel's entry in
 * tcc.c, so neither needs an adapter here. What remains is
 * tcc_set_io_channel: a TCCIO bundles all channels, so routing is
 * per-object, not per-channel, and the channel form installs the
 * whole object on the state exactly like tcc_set_io.
 *
 * Compiled only into the Android libtinyc.so, together with the
 * kernel unity build and tinyc_jni.c (see build.sh mode native-libs).
 */
#include "tinyc_kernel.h"

void tcc_set_io_channel(TCCState *s, int channel, TCCIO *io)
{
    (void)channel;
    tcc_set_io(s, io);
}
