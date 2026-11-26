/* setjmp.h - nonlocal jumps, Darwin.
 *
 * Written for this package from the C99 <setjmp.h> requirements.
 * The jump buffers are deliberately oversized opaque arrays: the
 * library writes a fixed, architecture-specific register set into
 * the buffer (callee-saved registers, stack and program counter,
 * plus the signal mask for sigjmp_buf), and a larger buffer is
 * always safe for the library's own accesses.  The exact platform
 * sizes are recorded by the macOS CI probe; what must not happen
 * is a buffer smaller than the register set, which the generous
 * sizes here rule out on both architectures.  Code that embeds a
 * jmp_buf inside another structure, or compares sizeof(jmp_buf)
 * with the platform headers, is outside what this header
 * guarantees.
 */

#ifndef _TINYC_DARWIN_SETJMP_H
#define _TINYC_DARWIN_SETJMP_H

typedef union {
	char __bytes[512];
	long long __align;
	void *__ptr;
} jmp_buf[1];

typedef union {
	char __bytes[1024];
	long long __align;
	void *__ptr;
} sigjmp_buf[1];

int setjmp(jmp_buf env);
int _setjmp(jmp_buf env);
int sigsetjmp(sigjmp_buf env, int savemask);

void longjmp(jmp_buf env, int val);
void _longjmp(jmp_buf env, int val);
void siglongjmp(sigjmp_buf env, int val);

#endif
