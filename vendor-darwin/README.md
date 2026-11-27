# vendor-darwin — bundled Darwin headers and link stubs

Self-contained compile-time support for the two Darwin targets
(arm64-apple-darwin, x86_64-apple-darwin): a header set and
libSystem link stubs that ship inside the package, so the
packaged compiler builds hosted C programs without the macOS
SDK installed or consulted.  Programs still link dynamically
against the system libSystem at run time; nothing of the C
library implementation ships here.

## Layout

- `include/` — 42 headers, staged into the dist at
  `lib/tcc/include-libc/` (the compiler's baked-in search path is
  `{B}/include-libc:{B}/include`: this set first, TCC's builtin
  header layer second).
- `lib/` — five `.tbd` link stubs, staged at `lib/tcc/lib/`:
  `libc.tbd` and `libSystem.tbd` carry the full declared symbol
  surface, `libm.tbd` / `libpthread.tbd` / `libdl.tbd` the
  per-header subsets, all with install name
  `/usr/lib/libSystem.B.dylib`, so a linked program records the
  system libSystem as its dependency whichever `-l` spelling the
  link used.  Mach-O has no crt objects; the loader and
  libSystem initialize the process.
- `tools/gen_tbd.py` — regenerates `lib/` from the declarations
  in `include/`; the stubs are derived, never edited by hand.
- `tools/check_darwin.py` — structural checks that run on any
  host: every header compiles standalone and together, the
  include closure stays inside this set plus the builtin layer,
  no name shadows a builtin header, the stubs on disk match the
  headers, and the documented struct layouts match the headers'
  own arithmetic.

Wiring: `mac-test-bundle/build-mac.sh` rewrites the search
paths upstream configure bakes into `config.h` (which otherwise
point at the build machine's SDK) and stages this tree into the
bundle dist, via `mac-test-bundle/darwin-dist.py`.

## Provenance

Apple's SDK headers are not redistributable, so every header
here is written for this project from the C and POSIX standards
and from Darwin's published ABI facts (manual pages, open-source
releases of the platform interfaces, and the observable behavior
of the system library).  Declaration text is original to this
repository.

## ABI notes

Both Darwin targets are LP64 with the same C library layout for
everything declared here, except `jmp_buf` internals, which are
opaque.  Points where Darwin differs from Linux and the headers
encode the Darwin value: `errno` codes (EAGAIN is 35, EDEADLK
11, ENOTSUP 45), signal numbers (SIGBUS 10, SIGSYS 12, SIGUSR1
30), `O_*` flags (O_CREAT 0x0200, O_APPEND 0x0008, O_CLOEXEC
0x1000000), `MAP_ANON` 0x1000, `CLOCK_MONOTONIC` 6, `errno`
itself is `(*__error())`, the stdio streams are `__stdinp` /
`__stdoutp` / `__stderrp`, `qsort_r` takes the thunk first,
`strerror_r` is the XSI form, `mcontext_t` is a pointer to the
machine state (unlike Linux, where it is the struct itself),
siginfo_t is the 104-byte LP64 form with the BSD si_code
values, fenv_t is 16 bytes on both architectures (x87
control/status/tag plus MXCSR on x86_64, FPSR/FPCR plus
reserved words on arm64), FE_DENORMAL exists only on arm64
(0x80, part of FE_ALL_EXCEPT 0x9f), and the arm64 rounding
constants are the FPCR rounding-mode field (bits 22-23),
and `struct stat` carries a birth-time field (144 bytes,
offsets in sys/stat.h).

Opaque or fixed-size types follow the platform's public layout:
`FILE` is fully opaque; `pthread_t` is a pointer; the pthread
attribute/object types are the platform's fixed opaque sizes
with the platform's static-initializer signatures; `mbstate_t`
is the platform's 128-byte union; `fd_set` is 1024 bits.

`jmp_buf` / `sigjmp_buf` are deliberately oversized opaque
arrays (512 / 1024 bytes): code that only passes them to
setjmp/longjmp is always safe; code that embeds them in other
structs or compares `sizeof` against the platform's exact size
is outside what these headers guarantee.

## Coverage

Declared: the C standard library surface (stdio, stdlib,
string, strings, math in all three floating forms, fenv,
ctype, time, wide characters, errno, limits, stdint,
inttypes, locale, setjmp, signal, assert), the common
POSIX surface (unistd, fcntl, dirent, pthread core, sched, and sys/types, sys/stat,
sys/time, sys/wait, sys/mman, sys/uio, sys/resource,
sys/ucontext), and the networking surface (sys/socket.h,
netinet/in.h, netinet/tcp.h, arpa/inet.h, netdb.h: the socket
endpoint API and name/address lookup, with the Darwin
layouts — the length byte in the address structures,
SOL_SOCKET 0xffff, the positive BSD getaddrinfo error
codes).  Combined
with TCC's builtin layer (stddef, stdarg, stdbool, stdalign,
stdnoreturn, stdatomic, float, tgmath, varargs) the set is the
complete compile-time surface the Darwin targets ship.  Four
minimal additions exist only because the compiler's own build
needs them: dispatch/dispatch.h (the semaphore surface tcc.h
and bcheck.c use), sys/syscall.h (exists so bcheck.c's
unconditional include resolves; it carries only the
thread_selfid number, since Darwin defines no SYS_thr_self),
sys/ucontext.h (the machine state tccrun.c's backtrace reads
out of the signal handler's context), and runetype.h (the
_DefaultRuneLocale pointer bcheck.c takes the address of).

Not declared: mach and other Darwin-specific subsystems
past the dispatch semaphore surface above, pthread cancellation
and spin locks, and anything past the surface above.  Programs
using those still need the platform SDK headers.

## Verification status

Structure is verified locally by tools/check_darwin.py.
Platform agreement of every declared size, offset and constant
is verified on macOS by tests/macos/run_macos_sdkfree.sh, which
compiles a functional battery and an ABI probe with the packaged
compiler while the SDK is hidden, and diffs the probe output
against the same probe built with the platform SDK headers.
Until that has run on a macOS machine, the individual ABI values
are as documented in the per-header comments, not platform-
confirmed; the probe diff is the mechanism that settles each
one, and the first run's diff (if any) names the exact values to
correct.
