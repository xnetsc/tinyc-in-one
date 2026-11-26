#!/usr/bin/env python3
"""Structural checks for the bundled Darwin headers and stubs.

Runs on any host with a C compiler (it is run on Linux during
development; the macOS CI probe in tests/macos/ is the runtime
authority).  Checks:

  1. Every bundled header compiles standalone, and all of them
     compile together in one translation unit (cc -fsyntax-only,
     bundled include dir first, TCC's builtin include dir second,
     mirroring the search order the packaged compiler is built
     with).
  2. Every #include <...> in a bundled header resolves inside the
     bundled set or TCC's builtin set (the include closure is
     closed without the platform SDK).
  3. No bundled header name collides with a builtin header name
     (the bundled set must extend the builtin layer, not shadow
     it).
  4. The .tbd files on disk carry exactly the symbol surfaces
     gen_tbd.py derives from the current headers (no stale stub).
  5. The struct layouts the headers document are arithmetically
     what the headers produce: a generated C file static-asserts
     the sizes and field offsets for struct stat, struct dirent,
     the pthread types and the other ABI-visible layouts.  The
     typedefs come from the bundled sys/types.h, so the arithmetic
     is the Darwin LP64 arithmetic on any LP64 host compiler.
     Whether the platform agrees is the macOS CI probe's job.

Exit status is the number of failed checks.
"""
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
VENDOR = os.path.dirname(HERE)
REPO = os.path.abspath(os.path.join(VENDOR, ".."))
INCLUDE = os.path.join(VENDOR, "include")
BUILTIN = os.path.join(REPO, "vendor-tinycc", "include")
LIB = os.path.join(VENDOR, "lib")
CC = os.environ.get("CC", "cc")

sys.path.insert(0, HERE)
import gen_tbd

fails = 0


def check(ok, label, detail=""):
    global fails
    if ok:
        print(f"PASS: {label}")
    else:
        fails += 1
        print(f"FAIL: {label}" + (f" ({detail})" if detail else ""))


def header_names():
    names = []
    for root, _dirs, files in os.walk(INCLUDE):
        for name in files:
            if name.endswith(".h"):
                names.append(os.path.relpath(os.path.join(root, name), INCLUDE))
    return sorted(names)


def syntax_check():
    names = header_names()
    with tempfile.TemporaryDirectory() as td:
        for rel in names:
            src = os.path.join(td, "one.c")
            with open(src, "w") as f:
                f.write(f"#include <{rel}>\nint main(void) {{ return 0; }}\n")
            r = subprocess.run(
                [CC, "-std=gnu11", "-fsyntax-only",
                 "-I", INCLUDE, "-I", BUILTIN, src],
                capture_output=True, text=True)
            check(r.returncode == 0, f"header compiles standalone: {rel}",
                  r.stderr.strip().splitlines()[0] if r.stderr else "")
        src = os.path.join(td, "all.c")
        with open(src, "w") as f:
            for rel in names:
                f.write(f"#include <{rel}>\n")
            f.write("int main(void) { return 0; }\n")
        r = subprocess.run(
            [CC, "-std=gnu11", "-fsyntax-only",
             "-I", INCLUDE, "-I", BUILTIN, src],
            capture_output=True, text=True)
        check(r.returncode == 0, "all headers compile together",
              r.stderr.strip().splitlines()[0] if r.stderr else "")


def closure_check():
    names = set(header_names())
    builtin = {f for f in os.listdir(BUILTIN) if f.endswith(".h")}
    missing = []
    for rel in names:
        with open(os.path.join(INCLUDE, rel)) as f:
            for inc in re.findall(r"#include\s*<([^>]+)>", f.read()):
                if inc not in names and inc not in builtin:
                    missing.append(f"{rel} -> <{inc}>")
    check(not missing, "include closure is closed",
          "; ".join(missing))


def collision_check():
    names = {os.path.basename(n) for n in header_names()}
    builtin = {f for f in os.listdir(BUILTIN) if f.endswith(".h")}
    both = sorted(names & builtin)
    check(not both, "no bundled header shadows a builtin header",
          ", ".join(both))


def tbd_symbols(path):
    syms = set()
    with open(path) as f:
        for line in f:
            m = re.search(r"symbols: \[([^\]]*)\]", line)
            if m:
                syms |= {s.strip() for s in m.group(1).split(",") if s.strip()}
    return syms


def tbd_check():
    per_header, all_funcs, all_vars = gen_tbd.extract_all()
    surface = {"_" + s for s in all_funcs | all_vars | set(gen_tbd.EXTRAS)}
    expectations = {
        "libc.tbd": surface,
        "libSystem.tbd": surface,
        "libm.tbd": {"_" + s for s in per_header["math.h"][0]}
                    | {"_" + s for s in gen_tbd.EXTRAS},
        "libpthread.tbd": {"_" + s for s in per_header["pthread.h"][0]}
                          | {"_pthread_sigmask"},
        "libdl.tbd": {"_" + s for s in per_header["dlfcn.h"][0]},
    }
    for name, want in expectations.items():
        path = os.path.join(LIB, name)
        if not os.path.exists(path):
            check(False, f"{name} is fresh", "file missing")
            continue
        got = tbd_symbols(path)
        check(got == want, f"{name} is fresh",
              f"missing {sorted(want - got)[:4]} extra {sorted(got - want)[:4]}")
    with open(os.path.join(LIB, "libc.tbd")) as f:
        head = f.read(400)
    check("install-name: /usr/lib/libSystem.B.dylib" in head,
          "libc.tbd install name is the system libSystem")


LAYOUT_ASSERTS = """
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ucontext.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <fenv.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <locale.h>
#include <wchar.h>
#include <utime.h>
#include <sched.h>

#define SA_SIZE(T, n) _Static_assert(sizeof(T) == n, #T " size")
#define SA_OFF(T, f, n) _Static_assert(offsetof(T, f) == n, #T "." #f)

SA_SIZE(struct timespec, 16);
SA_OFF(struct timespec, tv_nsec, 8);
SA_SIZE(struct timeval, 16);
SA_OFF(struct timeval, tv_usec, 8);
SA_SIZE(struct timezone, 8);
SA_SIZE(struct itimerval, 32);

SA_SIZE(struct stat, 144);
SA_OFF(struct stat, st_mode, 4);
SA_OFF(struct stat, st_ino, 8);
SA_OFF(struct stat, st_uid, 16);
SA_OFF(struct stat, st_rdev, 24);
SA_OFF(struct stat, st_atimespec, 32);
SA_OFF(struct stat, st_mtimespec, 48);
SA_OFF(struct stat, st_ctimespec, 64);
SA_OFF(struct stat, st_birthtimespec, 80);
SA_OFF(struct stat, st_size, 96);
SA_OFF(struct stat, st_blocks, 104);
SA_OFF(struct stat, st_blksize, 112);
SA_OFF(struct stat, st_qspare, 128);

SA_SIZE(struct dirent, 1048);
SA_OFF(struct dirent, d_seekoff, 8);
SA_OFF(struct dirent, d_reclen, 16);
SA_OFF(struct dirent, d_type, 20);
SA_OFF(struct dirent, d_name, 21);

SA_SIZE(struct flock, 24);
SA_OFF(struct flock, l_pid, 16);
SA_OFF(struct flock, l_type, 20);

SA_SIZE(struct sigaction, 16);
SA_OFF(struct sigaction, sa_mask, 8);
SA_OFF(struct sigaction, sa_flags, 12);
SA_SIZE(stack_t, 24);
SA_OFF(stack_t, ss_size, 8);

SA_SIZE(struct __siginfo, 104);
SA_OFF(struct __siginfo, si_code, 8);
SA_OFF(struct __siginfo, si_addr, 24);

SA_SIZE(ucontext_t, 56);
SA_OFF(ucontext_t, uc_sigmask, 4);
SA_OFF(ucontext_t, uc_stack, 8);
SA_OFF(ucontext_t, uc_link, 32);
SA_OFF(ucontext_t, uc_mcsize, 40);
SA_OFF(ucontext_t, uc_mcontext, 48);

#if defined(__x86_64__)
SA_SIZE(struct x86_float_state64, 524);
SA_SIZE(struct __darwin_mcontext64, 712);
SA_OFF(struct __darwin_mcontext64, __ss, 16);
SA_OFF(struct __darwin_mcontext64, __fs, 184);
SA_OFF(struct x86_thread_state64, __rip, 128);
SA_OFF(struct x86_thread_state64, __rbp, 48);
#elif defined(__aarch64__)
SA_SIZE(struct arm_neon_state64, 528);
SA_SIZE(struct __darwin_mcontext64, 816);
SA_OFF(struct __darwin_mcontext64, __ss, 16);
SA_OFF(struct __darwin_mcontext64, __ns, 288);
SA_OFF(struct arm_thread_state64, __pc, 256);
SA_OFF(struct arm_thread_state64, __fp, 232);
#endif

#if defined(__x86_64__)
SA_SIZE(fenv_t, 16);
#elif defined(__aarch64__)
SA_SIZE(fenv_t, 16);
#endif
SA_SIZE(fexcept_t, 2);

SA_SIZE(struct tm, 56);
SA_OFF(struct tm, tm_gmtoff, 40);
SA_OFF(struct tm, tm_zone, 48);

SA_SIZE(struct rusage, 144);
SA_OFF(struct rusage, ru_maxrss, 32);
SA_OFF(struct rusage, ru_nivcsw, 136);
SA_SIZE(struct rlimit, 16);

SA_SIZE(pthread_attr_t, 64);
SA_SIZE(pthread_mutex_t, 64);
SA_SIZE(pthread_mutexattr_t, 16);
SA_SIZE(pthread_cond_t, 48);
SA_SIZE(pthread_condattr_t, 16);
SA_SIZE(pthread_rwlock_t, 200);
SA_SIZE(pthread_rwlockattr_t, 24);
SA_SIZE(pthread_once_t, 16);
SA_SIZE(pthread_t, 8);
SA_SIZE(pthread_key_t, 8);

SA_SIZE(fd_set, 128);
SA_SIZE(mbstate_t, 128);
SA_SIZE(struct sched_param, 8);
SA_SIZE(struct utimbuf, 16);
SA_SIZE(struct lconv, 96);

int main(void) { return 0; }
"""


def layout_check():
    with tempfile.TemporaryDirectory() as td:
        src = os.path.join(td, "layout.c")
        with open(src, "w") as f:
            f.write(LAYOUT_ASSERTS)
        r = subprocess.run(
            [CC, "-std=gnu11", "-fsyntax-only",
             "-I", INCLUDE, "-I", BUILTIN, src],
            capture_output=True, text=True)
        check(r.returncode == 0,
              "documented struct layouts match the headers' arithmetic",
              "; ".join(r.stderr.strip().splitlines()[:3]))


def main():
    syntax_check()
    closure_check()
    collision_check()
    tbd_check()
    layout_check()
    print(f"check_darwin: {fails} failure(s)")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
