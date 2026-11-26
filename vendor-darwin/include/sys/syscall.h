/* sys/syscall.h - system call numbers, Darwin.
 *
 * Minimal subset written for this package.  The platform header is
 * a generated table of SYS_* numbers (xnu bsd/sys/syscall.h); this
 * bundled file exists so the libtcc1 build's unconditional
 * #include <sys/syscall.h> in vendor-tinycc/lib/bcheck.c resolves
 * without the macOS SDK.  No SYS_* constant is referenced by the
 * Darwin build: bcheck.c's SYS_thr_self use sits in its FreeBSD
 * branch (on Darwin the bounds-checking thread id falls back to
 * 0), and Darwin defines no SYS_thr_self at all.  The thread
 * identity call on Darwin is thread_selfid, whose number is given
 * below, verified against the machine-generated listing of the
 * platform header (mksysnum_darwin.pl output from
 * /usr/include/sys/syscall.h).
 */

#ifndef _TINYC_DARWIN_SYS_SYSCALL_H
#define _TINYC_DARWIN_SYS_SYSCALL_H

#define SYS_THREAD_SELFID	372

#endif
