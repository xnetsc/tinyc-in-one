/* sys/ucontext.h - user context, Darwin.
 *
 * Written for this package from Darwin's published ABI facts
 * (xnu's sys/ucontext.h and machine/_mcontext.h, whose content
 * this file mirrors in one place: the platform splits the
 * machine state across sys/ucontext.h and machine/_mcontext.h,
 * but this package ships no machine/ directory, so the arch
 * state structs live here).  Only the machine state the
 * compiler's own build needs is declared: tccrun.c reads the
 * saved program counter and frame pointer out of the signal
 * handler's ucontext on each target, and the signal handler
 * itself takes the context as a void pointer.
 *
 * Darwin facts: mcontext_t is a POINTER to the machine state
 * (unlike Linux, where it is the struct itself); ucontext_t is
 * 56 bytes LP64 (onstack, sigmask, stack, link, mcsize,
 * mcontext); the 64-bit x86 mcontext is exception + thread +
 * float state (712 bytes), the 64-bit arm mcontext is
 * exception + thread + neon state (816 bytes: the neon state's
 * 128-bit registers lift its alignment to 16, so the 808
 * bytes of fields pad to 816).  Only the two 64-bit targets
 * this package ships are covered.
 */

#ifndef _TINYC_DARWIN_SYS_UCONTEXT_H
#define _TINYC_DARWIN_SYS_UCONTEXT_H

#include <signal.h>
#include <stdint.h>
#include <sys/types.h>

#if defined(__x86_64__)

struct x86_exception_state64 {
	uint32_t __trapno;
	uint32_t __err;
	uint64_t __faultvaddr;
};
typedef struct x86_exception_state64 __darwin_x86_exception_state64_t;

struct x86_thread_state64 {
	uint64_t __rax;
	uint64_t __rbx;
	uint64_t __rcx;
	uint64_t __rdx;
	uint64_t __rdi;
	uint64_t __rsi;
	uint64_t __rbp;
	uint64_t __rsp;
	uint64_t __r8;
	uint64_t __r9;
	uint64_t __r10;
	uint64_t __r11;
	uint64_t __r12;
	uint64_t __r13;
	uint64_t __r14;
	uint64_t __r15;
	uint64_t __rip;
	uint64_t __rflags;
	uint64_t __cs;
	uint64_t __fs;
	uint64_t __gs;
};
typedef struct x86_thread_state64 __darwin_x86_thread_state64_t;

struct __mmst_reg {
	char __mmst_reg[10];
	char __mmst_rsrv[6];
};
struct __xmm_reg {
	char __xmm_reg[16];
};

struct x86_float_state64 {
	int __fpu_reserved[2];
	uint16_t __fpu_fcw;
	uint16_t __fpu_fsw;
	uint8_t __fpu_ftw;
	uint8_t __fpu_rsrv1;
	uint16_t __fpu_fop;
	uint32_t __fpu_ip;
	uint16_t __fpu_cs;
	uint16_t __fpu_rsrv2;
	uint32_t __fpu_dp;
	uint16_t __fpu_ds;
	uint16_t __fpu_rsrv3;
	uint32_t __fpu_mxcsr;
	uint32_t __fpu_mxcsrmask;
	struct __mmst_reg __fpu_stmm0;
	struct __mmst_reg __fpu_stmm1;
	struct __mmst_reg __fpu_stmm2;
	struct __mmst_reg __fpu_stmm3;
	struct __mmst_reg __fpu_stmm4;
	struct __mmst_reg __fpu_stmm5;
	struct __mmst_reg __fpu_stmm6;
	struct __mmst_reg __fpu_stmm7;
	struct __xmm_reg __fpu_xmm0;
	struct __xmm_reg __fpu_xmm1;
	struct __xmm_reg __fpu_xmm2;
	struct __xmm_reg __fpu_xmm3;
	struct __xmm_reg __fpu_xmm4;
	struct __xmm_reg __fpu_xmm5;
	struct __xmm_reg __fpu_xmm6;
	struct __xmm_reg __fpu_xmm7;
	struct __xmm_reg __fpu_xmm8;
	struct __xmm_reg __fpu_xmm9;
	struct __xmm_reg __fpu_xmm10;
	struct __xmm_reg __fpu_xmm11;
	struct __xmm_reg __fpu_xmm12;
	struct __xmm_reg __fpu_xmm13;
	struct __xmm_reg __fpu_xmm14;
	struct __xmm_reg __fpu_xmm15;
	char __fpu_rsrv4[6 * 16];
	uint32_t __fpu_reserved1;
};
typedef struct x86_float_state64 __darwin_x86_float_state64_t;

struct __darwin_mcontext64 {
	__darwin_x86_exception_state64_t __es;
	__darwin_x86_thread_state64_t __ss;
	__darwin_x86_float_state64_t __fs;
};

#elif defined(__aarch64__)

struct arm_exception_state64 {
	uint64_t __far;
	uint32_t __esr;
	uint32_t __exception;
};
typedef struct arm_exception_state64 __darwin_arm_exception_state64_t;

struct arm_thread_state64 {
	uint64_t __x[29];
	uint64_t __fp;
	uint64_t __lr;
	uint64_t __sp;
	uint64_t __pc;
	uint32_t __cpsr;
	uint32_t __pad;
};
typedef struct arm_thread_state64 __darwin_arm_thread_state64_t;

struct arm_neon_state64 {
	__uint128_t __v[32];
	uint32_t __fpsr;
	uint32_t __fpcr;
};
typedef struct arm_neon_state64 __darwin_arm_neon_state64_t;

struct __darwin_mcontext64 {
	__darwin_arm_exception_state64_t __es;
	__darwin_arm_thread_state64_t __ss;
	__darwin_arm_neon_state64_t __ns;
};

#else
#error "sys/ucontext.h: only the x86_64 and arm64 targets are supported"
#endif

struct __darwin_ucontext {
	int uc_onstack;
	sigset_t uc_sigmask;
	stack_t uc_stack;
	struct __darwin_ucontext *uc_link;
	size_t uc_mcsize;
	struct __darwin_mcontext64 *uc_mcontext;
};
typedef struct __darwin_ucontext ucontext_t;

typedef struct __darwin_mcontext64 *mcontext_t;

#endif
