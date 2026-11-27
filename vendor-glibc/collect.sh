#!/bin/bash
# Collect one linux-gnu triple's vendored glibc headers into
# vendor-glibc/<triple>/ from the build host's installed cross
# development packages:
#
#   collect.sh x86_64-linux-gnu   libc6-dev linux-libc-dev
#   collect.sh aarch64-linux-gnu  libc6-dev-arm64-cross linux-libc-dev-arm64-cross
#   collect.sh riscv64-linux-gnu  libc6-dev-riscv64-cross linux-libc-dev-riscv64-cross
#
# The first package named is the glibc development package; the
# second is the kernel header package. What ships is the glibc
# package's header set (the multiarch include subdirectory
# merged into the tree root, the merged layout the compiler
# searches), cut by level:
#
#   * the basic layer — the C standard headers and the POSIX
#     surface — ships;
#   * three exception clusters ship with it: the pthread
#     cluster (pthread.h, sched.h, semaphore.h), the socket
#     cluster (sys/socket.h, netinet/in.h, netinet/tcp.h,
#     arpa/inet.h) together with the name-resolution surface
#     (netdb.h, rpc/netdb.h), and dlfcn.h;
#   * everything at or above that level is out: application
#     frameworks, subsystem interfaces, the rest of the
#     networking domain, OS interfaces that are neither C nor
#     POSIX (epoll, eventfd, ptrace, mount, ...), and
#     convenience facilities (getopt.h, malloc.h, utmp.h,
#     ...); the exclusion list below is the boundary. A
#     program that needs more brings the headers itself and
#     adds them with an include path at compile time.
#
# Internal fragments (bits/) ship only when a shipped header's
# #include closure reaches them, and individual kernel UAPI
# headers enter the same way, drawn from the kernel package
# only where the closure names one (bits/errno.h names
# linux/errno.h, for example): implementation dependencies of
# shipped headers, not a surface of their own. Every shipped
# header's references must resolve inside the shipped set, the
# compiler's builtin set, or the artifact gate's host-provided
# registry; the closure pass below fails the collection
# otherwise. tgmath.h is left out: glibc's tgmath.h pulls in
# complex.h, whose declarations need _Complex, which TCC does
# not support; the compiler's builtin tgmath.h serves
# <tgmath.h>. The crt objects come from the same glibc build
# as the headers.
set -e
cd "$(dirname "$0")"
TRIPLE=$1; GLIBC_PKG=$2; KERNEL_PKG=$3
case $TRIPLE in
    "$(gcc -dumpmachine)")
        INCROOT=/usr/include; SUB=$TRIPLE; LIBDIR=/usr/lib/$TRIPLE ;;
    *)
        INCROOT=/usr/$TRIPLE/include; SUB=""; LIBDIR=/usr/$TRIPLE/lib ;;
esac
DEST=$PWD/$TRIPLE

# The exclusion boundary. Entries are tree-relative file paths,
# or directory prefixes ending in '/'. Absent entries are
# reported, not fatal: the packages' contents vary by target.
EXCLUDED=(
    # application-level POSIX facilities, named out
    mqueue.h aio.h
    sys/ipc.h sys/shm.h sys/msg.h sys/sem.h
    # the resolver library (the socket cluster covers lookup
    # through netdb.h)
    resolv.h arpa/nameser.h arpa/nameser_compat.h
    # GNU application frameworks and convenience facilities
    argp.h obstack.h argz.h envz.h error.h printf.h mcheck.h
    libintl.h err.h fts.h execinfo.h wordexp.h
    # module-author and subsystem interfaces, database
    # facilities, profiling
    nss.h gconv.h thread_db.h proc_service.h aliases.h ttyent.h
    sys/gmon.h sys/gmon_out.h sys/profil.h
    nfs/
    # the networking domain outside the socket cluster
    ifaddrs.h sys/un.h sys/socketvar.h
    net/ netpacket/ protocols/
    netash/ netatalk/ netax25/ neteconet/ netipx/ netiucv/
    netrom/ netrose/
    netinet/ether.h netinet/icmp6.h netinet/if_ether.h
    netinet/if_fddi.h netinet/if_tr.h netinet/igmp.h
    netinet/in_systm.h netinet/ip.h netinet/ip6.h
    netinet/ip_icmp.h netinet/udp.h
    arpa/ftp.h arpa/telnet.h arpa/tftp.h
    # device and hardware subsystems
    scsi/
    sys/mtio.h sys/kd.h sys/vt.h sys/soundcard.h sys/raw.h
    sys/vm86.h sys/io.h sys/perm.h sys/debugreg.h sys/reg.h
    sys/quota.h sys/acct.h sys/rseq.h
    # not a C header at all: the Fortran vector-math interface
    finclude/
    # OS interfaces that are neither C nor POSIX: event and
    # descriptor facilities, tracing, mount and attribute
    # interfaces, process and system control, the ELF and
    # /proc process interfaces, the syscall number table
    sys/epoll.h sys/eventfd.h sys/signalfd.h sys/timerfd.h
    sys/inotify.h sys/fanotify.h sys/ptrace.h sys/prctl.h
    sys/mount.h sys/xattr.h sys/sendfile.h sys/random.h
    sys/auxv.h sys/sysinfo.h sys/file.h
    sys/klog.h sys/personality.h sys/pidfd.h sys/reboot.h
    sys/swap.h sys/fsuid.h sys/pci.h sys/platform/
    sys/timex.h sys/statfs.h sys/vfs.h
    sys/elf.h
    syscall.h sys/syscall.h
    # convenience facilities and legacy interfaces
    getopt.h pty.h mntent.h fstab.h malloc.h stdio_ext.h
    utmp.h utmpx.h lastlog.h re_comp.h regexp.h sysexits.h
    paths.h xlocale.h stab.h a.out.h elf.h link.h
    shadow.h gshadow.h byteswap.h memory.h values.h
    sgtty.h termio.h sys/ttychars.h
    sys/dir.h sys/timeb.h sys/vlimit.h ulimit.h wait.h
    sys/errno.h sys/fcntl.h sys/signal.h sys/termios.h
    sys/unistd.h sys/param.h sys/queue.h
    fpu_control.h ieee754.h sys/bitypes.h
    sys/single_threaded.h
    # glibc-internal data headers: the library name and
    # version tables, named by nothing a program compiles
    # against (the stubs tables stay: features.h names them)
    gnu/lib-names.h gnu/lib-names-64.h gnu/libc-version.h
    gnu-versions.h
)

# One closure-forced exception to the cut above: aarch64's
# sys/ucontext.h (named by signal.h) includes <sys/procfs.h>,
# which includes <sys/user.h>, so both stay on that triple as
# closure keeps. On the other triples nothing names them and
# they are out with the rest of the /proc process interface.
case $TRIPLE in
    aarch64-linux-gnu) ;;
    *) EXCLUDED+=(sys/procfs.h sys/user.h) ;;
esac

rm -rf "$DEST/include" "$DEST/crt" "$DEST/.kernel-pool"
mkdir -p "$DEST/include" "$DEST/crt" "$DEST/.kernel-pool"
# Stage the candidate pools: the glibc package's files into the
# tree, the kernel package's files into a side pool the closure
# pass may draw individual files from.
stage() { # <package> <destdir>
    dpkg -L "$1" | grep "^$INCROOT/" | grep -E '\.(h|inc|def)$' | sort -u \
        | while read -r f; do
            rel=${f#"$INCROOT"/}
            [ -n "$SUB" ] && rel=${rel#"$SUB"/}
            mkdir -p "$2/$(dirname "$rel")"
            cp "$f" "$2/$rel"
          done
}
stage "$GLIBC_PKG" "$DEST/include"
stage "$KERNEL_PKG" "$DEST/.kernel-pool"
rm -f "$DEST/include/tgmath.h"

# Exclusion + closure pass: delete the excluded paths, then
# walk #include references from every remaining non-fragment
# header. Kernel headers the walk names are pulled out of the
# side pool; staged fragments the walk never reaches are
# deleted; a reference that resolves nowhere fails the run.
python3 - "$DEST/include" "$DEST/.kernel-pool" \
        "$PWD/../vendor-tinycc/include" \
        "$PWD/../tests/artifact-gate/header_host_provided.json" \
        "$TRIPLE" "${EXCLUDED[@]}" <<'PYEOF'
import json, os, posixpath, re, sys

tree, kernel_pool, builtin_dir, registry_path, triple = sys.argv[1:6]
excluded = sys.argv[6:]

INCLUDE_RE = re.compile(r'^\s*#\s*include(_next)?\s*[<"]([^>"]+)[>"]', re.M)

def files_under(root):
    out = {}
    for dirpath, _dirs, names in os.walk(root):
        for n in names:
            full = os.path.join(dirpath, n)
            out[os.path.relpath(full, root).replace(os.sep, "/")] = full
    return out

pool = files_under(tree)
kernel = files_under(kernel_pool)
builtin = set(os.listdir(builtin_dir))
with open(registry_path, encoding="utf-8") as f:
    scopes = json.load(f)["scopes"]
registered = {e["header"] for scope in ("native:*", f"native:{triple}")
              for e in scopes.get(scope, [])}

# Exclusions first, so a reference to an excluded header from a
# shipped one surfaces as a problem instead of passing silently.
n_excluded = 0
for entry in excluded:
    if entry.endswith("/"):
        hits = [rel for rel in pool if rel.startswith(entry)]
    else:
        hits = [entry] if entry in pool else []
    if not hits:
        print(f"  note: exclusion '{entry}' matches nothing in this tree")
    for rel in hits:
        os.remove(pool.pop(rel))
        n_excluded += 1

kept = set()          # tree-relative paths that ship
kernel_pulled = []    # kernel-pool files the closure named
problems = []

def resolve(inc, includer_rel):
    """Classify one #include target: a tree path to keep, or one
    of the leaf kinds 'builtin' / 'registered' / None."""
    cand = posixpath.normpath(posixpath.join(posixpath.dirname(includer_rel), inc))
    if cand in pool:
        return cand
    if inc in pool:
        return inc
    if inc in kernel:
        src = kernel.pop(inc)
        dst = os.path.join(tree, *inc.split("/"))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        os.replace(src, dst)
        pool[inc] = dst
        kernel_pulled.append(inc)
        return inc
    if inc in builtin:
        return "builtin"
    if inc in registered:
        return "registered"
    return None

# Roots: every staged header that is not an internal fragment.
# Fragments (bits/) ship only when the walk reaches them.
queue = []
for rel in sorted(pool):
    if not rel.startswith("bits/"):
        kept.add(rel)
        queue.append(rel)

while queue:
    rel = queue.pop()
    with open(pool[rel], encoding="utf-8", errors="replace") as f:
        text = f.read()
    for is_next, inc in INCLUDE_RE.findall(text):
        if is_next and (inc == rel or inc == posixpath.basename(rel)):
            continue  # #include_next chain to the next copy of itself
        hit = resolve(inc, rel)
        if hit in ("builtin", "registered"):
            continue
        if hit is None:
            problems.append(f"{rel} includes '{inc}', which is in none of "
                            f"the shipped set, the kernel pool, the builtin "
                            f"include set, or the host-provided registry")
        elif hit not in kept:
            kept.add(hit)
            queue.append(hit)

if problems:
    for p in problems:
        print(f"collect: {p}", file=sys.stderr)
    sys.exit(1)

dropped = []
for rel, full in sorted(pool.items()):
    if rel not in kept:
        os.remove(full)
        dropped.append(rel)
for dirpath, _dirs, _names in os.walk(tree, topdown=False):
    if dirpath != tree and not os.listdir(dirpath):
        os.rmdir(dirpath)

print(f"{triple}: {len(kept)} headers kept "
      f"({n_excluded} excluded, {len(dropped)} fragments dropped, "
      f"{len(kernel_pulled)} kernel headers pulled by closure)")
for k in sorted(kernel_pulled):
    print(f"  kernel-pulled: {k}")
for d in dropped:
    print(f"  fragment-dropped: {d}")
PYEOF
rm -rf "$DEST/.kernel-pool"
cp "$LIBDIR"/crt1.o "$LIBDIR"/Scrt1.o "$LIBDIR"/crti.o "$LIBDIR"/crtn.o \
   "$DEST/crt/"
cp /usr/share/common-licenses/LGPL-2.1 "$DEST/COPYING.LIB"
