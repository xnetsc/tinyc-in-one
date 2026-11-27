#!/usr/bin/env python3
"""Prune a copied NDK sysroot include tree to the shipped header set.

The shipped set is the bionic surface at the C / POSIX layer: the C
standard library, the full POSIX surface and bionic's own libc-level
extensions, plus the individual kernel UAPI headers that surface's
#include closure pulls in. What does not ship, by layer:

  - kernel UAPI trees (linux/, asm-generic/ and the per-triple asm/
    directories), except the individual headers the closure pulls
    back in;
  - headers of other libraries and subsystems: libc++ (c++/), the
    Khronos graphics stack (EGL/, GLES*/, KHR/, vulkan/), OpenSL ES
    and OpenMAX (SLES/, OMXAL/), ICU (unicode/), the NDK platform
    libraries (aaudio/, amidi/, camera/, media/ and the non-bionic
    part of android/), and the zlib and JNI headers;
  - the POSIX message-queue and AIO headers (mqueue.h, aio.h),
    application-level surfaces; the NDK r26d tree carries neither
    (only the kernel's linux/mqueue.h and linux/aio_abi.h, which
    leave with the UAPI trees and are not pulled back);
  - the same-layer application surfaces the tree does carry:
    System V IPC (sys/ipc.h, sys/shm.h, sys/msg.h, sys/sem.h),
    the DNS resolver surface (resolv.h, arpa/nameser.h,
    arpa/nameser_compat.h) and the arpa application-protocol
    headers (arpa/ftp.h, arpa/telnet.h, arpa/tftp.h). The pthread
    cluster, the socket cluster and dlfcn.h stay as named
    exceptions at that layer;
  - OS interfaces that are neither C nor POSIX: the epoll,
    eventfd, signalfd, timerfd and inotify descriptor
    facilities, ptrace, prctl, mount, the extended-attribute
    and process-control interfaces, the syscall number table
    (syscall.h, sys/syscall.h), the Android system-properties
    interface, and their kin. The socket cluster carries
    netdb.h with it: name lookup ships, the resolver library
    behind it (resolv.h) does not;
    One member of that class stays: sys/user.h, because
    bionic's sys/ucontext.h (named by signal.h) includes it —
    a closure keep, not surface (the position sys/procfs.h
    holds on glibc's aarch64 tree, whose sys/ucontext.h names
    it the same way);
  - convenience facilities and legacy interfaces: getopt.h,
    pty.h, mntent.h, stdio_ext.h, the utmp database (utmp.h,
    utmpx.h, lastlog.h), fts.h, execinfo.h, the BSD error
    printers (err.h, error.h), the link-layer packet and
    address-enumeration interfaces outside the socket cluster,
    and the compat alias spellings of basic headers. Two
    headers of that class stay because bionic's own basic
    headers name them — malloc.h (stdlib.h includes it) and
    xlocale.h (string.h, locale.h and others include it):
    closure keeps, not surface;
  - the asm/ trees of triples this package does not ship.

Internal fragments (bits/) ship only when a kept header's
#include closure reaches them; the final sweep drops the
rest.

The android/ directory mixes bionic's own headers with other
platform libraries'; the bionic-owned subset (ANDROID_BIONIC below)
ships, the rest does not.

The closure is computed per shipped triple with the compiler's own
search order (directory of the including file, then the tree root,
then the triple's arch directory); only references that land in the
kernel UAPI trees are pulled back, transitively. A name the tree
does not contain at all belongs to some other header set (the
compiler's builtin headers, for instance) and is left for the
header-closure audit to account for.

Usage: prune.py [--apply] <include-dir>
Without --apply, prints the sets and counts and changes nothing.
"""

import os
import re
import sys

SHIPPED_TRIPLES = ("aarch64-linux-android", "x86_64-linux-android")

# Directories that never ship as a whole. The kernel UAPI entries
# among them can still contribute individual headers through the
# closure pull-back below.
EXCLUDED_DIRS = {
    # kernel UAPI
    "linux", "asm-generic",
    "aarch64-linux-android", "x86_64-linux-android",
    # asm trees of triples this package does not ship
    "arm-linux-androideabi", "i686-linux-android",
    "riscv64-linux-android",
    # kernel driver UAPI families
    "drm", "mtd", "rdma", "scsi", "sound", "video", "xen", "misc",
    # other libraries and subsystems
    "c++", "EGL", "GLES", "GLES2", "GLES3", "KHR", "vulkan",
    "OMXAL", "SLES", "unicode",
    "aaudio", "amidi", "camera", "media",
}

# The kernel UAPI directories the closure may pull individual
# headers back from (the shipped triples' asm trees included).
UAPI_DIRS = ("linux", "asm-generic") + tuple(
    t + "/asm" for t in SHIPPED_TRIPLES)

# Top-level headers that do not ship: other libraries' surfaces
# (zlib, JNI, ICU's local config), the application-level POSIX
# message-queue and AIO surfaces, the DNS resolver library
# (resolv.h), and the convenience / legacy class. (fstab.h,
# re_comp.h, regexp.h, stab.h and a.out.h belong to the same
# class; the NDK tree carries none of them.) The lookup header
# netdb.h ships with the socket cluster: name resolution is
# part of that surface.
EXCLUDED_FILES = {
    "zlib.h", "zconf.h", "jni.h", "uconfig_local.h",
    "mqueue.h", "aio.h", "resolv.h",
    "getopt.h", "pty.h", "mntent.h", "stdio_ext.h",
    "utmp.h", "utmpx.h", "lastlog.h", "sysexits.h", "paths.h",
    "elf.h", "link.h", "fts.h", "execinfo.h",
    "err.h", "error.h", "ifaddrs.h", "memory.h", "wait.h",
    "termio.h", "byteswap.h",
    "syscall.h", "time64.h", "features.h",
}

# Individual headers below the top level that do not ship:
# System V IPC, the resolver / application-protocol part of
# arpa/, the OS interfaces that are neither C nor POSIX, the
# networking domain outside the socket cluster, and the compat
# alias spellings of basic headers.
EXCLUDED_RELS = {
    "sys/ipc.h", "sys/shm.h", "sys/msg.h", "sys/sem.h",
    "arpa/nameser.h", "arpa/nameser_compat.h",
    "arpa/ftp.h", "arpa/telnet.h", "arpa/tftp.h",
    "sys/epoll.h", "sys/eventfd.h", "sys/signalfd.h",
    "sys/timerfd.h", "sys/inotify.h", "sys/ptrace.h",
    "sys/prctl.h", "sys/mount.h", "sys/xattr.h",
    "sys/sendfile.h", "sys/random.h", "sys/auxv.h",
    "sys/sysinfo.h", "sys/file.h",
    "sys/cachectl.h", "sys/capability.h", "sys/fsuid.h",
    "sys/klog.h", "sys/mtio.h", "sys/personality.h",
    "sys/pidfd.h", "sys/quota.h", "sys/reboot.h", "sys/reg.h",
    "sys/swap.h", "sys/timex.h", "sys/vt.h",
    "sys/statfs.h", "sys/vfs.h", "sys/procfs.h",
    "sys/system_properties.h", "sys/_system_properties.h",
    "sys/thread_properties.h", "sys/syscall.h", "sys/ifunc.h",
    "sys/queue.h", "sys/param.h",
    "sys/errno.h", "sys/fcntl.h", "sys/poll.h", "sys/signal.h",
    "sys/unistd.h", "sys/ttydefaults.h", "sys/sysconf.h",
    "sys/limits.h",
    "net/ethernet.h", "net/if_arp.h", "net/if_packet.h",
    "net/route.h", "netpacket/packet.h",
    "netinet/ether.h", "netinet/icmp6.h", "netinet/if_ether.h",
    "netinet/in_systm.h", "netinet/ip.h", "netinet/ip6.h",
    "netinet/ip_icmp.h", "netinet/udp.h",
}

# The bionic-owned subset of android/; the rest of that directory
# belongs to other platform libraries and does not ship.
ANDROID_BIONIC = {
    "api-level.h", "ndk-version.h", "versioning.h", "dlext.h",
    "fdsan.h", "set_abort_message.h",
    "legacy_stdlib_inlines.h", "legacy_termios_inlines.h",
    "legacy_threads_inlines.h", "legacy_unistd_inlines.h",
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*(?:<([^>]+)>|"([^"]+)")',
                        re.M)


def tree_files(root):
    out = []
    for dirpath, _dirnames, filenames in os.walk(root):
        for name in filenames:
            out.append(os.path.relpath(os.path.join(dirpath, name),
                                       root))
    return sorted(out)


def excluded(rel):
    parts = rel.split(os.sep)
    if rel.replace(os.sep, "/") in EXCLUDED_RELS:
        return True
    if parts[0] in EXCLUDED_DIRS:
        return True
    if parts[0] == "android" and parts[-1] not in ANDROID_BIONIC:
        return True
    if len(parts) == 1 and parts[0] in EXCLUDED_FILES:
        return True
    return False


def resolve(name, includer_dir, files, triple):
    """Resolve an include name the way the compiler's search order
    does over this tree for one shipped triple: directory of the
    including file, tree root, then the triple's arch directory."""
    cands = []
    if includer_dir:
        cands.append(os.path.join(includer_dir, name))
    cands.append(name)
    cands.append(os.path.join(triple, name))
    for cand in cands:
        cand = os.path.normpath(cand)
        if cand in files:
            return cand
    return None


def main():
    apply_changes = "--apply" in sys.argv
    args = [a for a in sys.argv[1:] if a != "--apply"]
    root = args[0]
    all_files = tree_files(root)
    file_set = set(all_files)
    headers = {f for f in all_files if f.endswith(".h")}

    keep = {f for f in headers if not excluded(f)}

    # Closure pull-back: scan the kept set; a reference that only
    # resolves into a kernel UAPI directory pulls that header in,
    # transitively.
    pulled = set()
    work = list(keep)
    while work:
        cur = work.pop()
        try:
            text = open(os.path.join(root, cur),
                        errors="replace").read()
        except OSError:
            continue
        for m in INCLUDE_RE.finditer(text):
            name = m.group(1) or m.group(2)
            # Resolve once per shipped triple: an arch-relative
            # name (asm/...) has one copy per triple, and every
            # shipped triple's copy must be pulled.
            for triple in SHIPPED_TRIPLES:
                hit = resolve(name, os.path.dirname(cur), file_set,
                              triple)
                if hit is None or hit in keep:
                    continue
                if any(hit == d or hit.startswith(d + os.sep)
                       for d in UAPI_DIRS):
                    keep.add(hit)
                    pulled.add(hit)
                    work.append(hit)

    # Fragment sweep: a bits/ fragment ships only when a kept
    # non-fragment header's #include closure reaches it (the
    # rule the glibc collection applies to its fragments).
    # Resolution mirrors the closure audit's: the including
    # file's directory, the tree root, the triple's arch
    # directory, then a basename match in the kept set.
    by_basename = {}
    for f in keep:
        by_basename.setdefault(os.path.basename(f), []).append(f)
    reached = {f for f in keep if not f.startswith("bits/")}
    work = list(reached)
    while work:
        cur = work.pop()
        try:
            text = open(os.path.join(root, cur),
                        errors="replace").read()
        except OSError:
            continue
        for m in INCLUDE_RE.finditer(text):
            name = m.group(1) or m.group(2)
            hits = set()
            for triple in SHIPPED_TRIPLES:
                hit = resolve(name, os.path.dirname(cur), keep,
                              triple)
                if hit is not None:
                    hits.add(hit)
            if not hits:
                hits = set(by_basename.get(os.path.basename(name),
                                           []))
            for hit in hits:
                if hit not in reached:
                    reached.add(hit)
                    work.append(hit)
    swept = sorted(f for f in keep
                   if f.startswith("bits/") and f not in reached)
    keep -= set(swept)

    drop = [f for f in all_files if f not in keep]
    print(f"tree files: {len(all_files)} ({len(headers)} headers)")
    print(f"keep: {len(keep)} headers")
    print(f"drop: {len(drop)} files")
    print(f"kernel UAPI headers pulled back by the closure: "
          f"{len(pulled)}")
    print(f"bits/ fragments dropped as unreachable: {len(swept)}")
    for f in swept:
        print(f"  fragment-dropped: {f}")
    for f in sorted(pulled):
        print(f"  pulled: {f}")
    for name in ("mqueue.h", "aio.h", "linux/mqueue.h",
                 "linux/aio_abi.h"):
        state = "shipped" if name in keep else "not shipped"
        present = "in tree" if name in file_set else "absent from tree"
        print(f"  {name}: {present}, {state}")
    if apply_changes:
        for f in drop:
            os.unlink(os.path.join(root, f))
        for dirpath, dirnames, _f in os.walk(root, topdown=False):
            for d in dirnames:
                p = os.path.join(dirpath, d)
                if not os.listdir(p):
                    os.rmdir(p)
        print("applied: dropped files deleted, empty dirs removed")


if __name__ == "__main__":
    main()
