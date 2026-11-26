# Bundled glibc headers and crt objects (linux-gnu targets)

Each `<triple>/` directory carries the headers and the crt
objects the native packages ship for that target:

- `include/` — glibc 2.39's header set, cut by level. The
  basic layer ships: the C standard headers and the POSIX
  surface. Three exception clusters ship with it: the
  pthread cluster (`pthread.h`, `sched.h`, `semaphore.h`),
  the socket cluster (`sys/socket.h`, `netinet/in.h`,
  `netinet/tcp.h`, `arpa/inet.h`), and `dlfcn.h`. Everything
  at or above that level is out — application frameworks
  (argp, obstack, gettext), subsystem interfaces, the
  resolver library, System V IPC, the rest of the networking
  domain, OS interfaces that are neither C nor POSIX
  (epoll, eventfd, ptrace, mount), and convenience
  facilities (getopt.h, utmp.h, malloc.h); `collect.sh`
  holds the exclusion list that draws the boundary and
  regenerates a tree from the installed development
  packages. Internal fragments (`bits/`) ship
  only when a shipped header's `#include` closure reaches
  them, and individual kernel UAPI headers enter the same
  way (for example `linux/errno.h`, named by `bits/errno.h`):
  implementation dependencies of shipped headers, not a
  surface of their own. A program that needs more brings
  the headers itself and adds them with an include path at
  compile time. glibc headers are licensed under the GNU
  Lesser General Public License, version 2.1 or later; see
  `COPYING.LIB`. The kernel UAPI headers are licensed under
  the GPL with the syscall exception stated in each file.
  `tgmath.h` is not carried: glibc's version includes
  `complex.h`, whose declarations need `_Complex`, which TCC
  does not support; the compiler's builtin `tgmath.h` serves
  `<tgmath.h>` instead.
- `crt/` — `crt1.o`, `Scrt1.o`, `crti.o`, `crtn.o` from the same
  glibc build (LGPL, see `COPYING.LIB`).

Programs compiled with these headers link the system
`libc.so.6` / `libm.so.6` dynamically at run time; only the
headers and crt objects are bundled.
