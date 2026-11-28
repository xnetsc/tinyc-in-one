#!/usr/bin/env python3
"""Dump libcurl's protocol-handler table from a Mach-O binary.

The bundled libcurl keeps its protocol handlers in a 67-entry
perfect-hash table `protocols[]` (vendor-curl/lib/url.c); the
"ws" scheme hashes to slot 60. On macOS the table is emitted as
chained fixups, so a broken link shows up here as a NULL slot
or a target that is not _Curl_handler_ws, while a correct link
shows the handler address with its chain offset.

Usage: check_protocols_table.py <mach-o-binary>
Exit 0 when slot 60 holds _Curl_handler_ws, 1 when it does not,
2 when the symbols or the table cannot be located.
"""
import struct
import sys

SLOTS = {17: "http", 25: "wss", 26: "https", 60: "ws"}
MASK36 = 0xFFFFFFFFF


def main(path):
    try:
        data = open(path, "rb").read()
    except OSError as e:
        print("protocols table: cannot read %s: %s" % (path, e))
        return 2
    if len(data) < 32 or data[:4] not in (b"\xcf\xfa\xed\xfe",
                                         b"\xce\xfa\xed\xfe"):
        print("protocols table: %s is not a Mach-O binary" % path)
        return 2
    is64 = data[0] == 0xCF  # 64-bit magic is CF FA ED FE
    ncmds = struct.unpack("<I", data[16:20])[0]
    off = 32
    symoff = nsyms = stroff = None
    sections = []
    for _ in range(ncmds):
        if off + 8 > len(data):
            break
        cmd, size = struct.unpack("<2I", data[off:off + 8])
        if cmd == 0x2:  # LC_SYMTAB
            symoff, nsyms, stroff, _ = struct.unpack(
                "<4I", data[off + 8:off + 24])
        elif cmd == 0x19:  # LC_SEGMENT_64
            nsects = struct.unpack("<I", data[off + 64:off + 68])[0]
            so = off + 72
            for _ in range(nsects):
                addr, sz, o = struct.unpack("<QQI", data[so + 32:so + 52])
                sections.append((addr, sz, o))
                so += 80
        off += size
    if symoff is None or nsyms is None:
        print("protocols table: no symbol table in %s" % path)
        return 2
    proto = None
    handlers = {}
    entsize = 16 if is64 else 12
    for i in range(nsyms):
        e = symoff + i * entsize
        if e + entsize > len(data):
            break
        if is64:
            n_strx, n_type, n_sect, n_desc, n_value = struct.unpack(
                "<IBBHQ", data[e:e + 16])
        else:
            n_strx, n_type, n_sect, n_desc, n_value = struct.unpack(
                "<IBBHI", data[e:e + 12])
        if not n_strx or stroff + n_strx >= len(data):
            continue
        end = data.index(b"\0", stroff + n_strx)
        name = data[stroff + n_strx:end].decode("utf-8", "replace")
        if name == "_protocols":
            proto = n_value
        elif name.startswith("_Curl_handler_"):
            handlers[name[14:]] = n_value
    if proto is None:
        print("protocols table: _protocols not found in %s" % path)
        return 2
    fo = None
    for addr, sz, o in sections:
        if addr <= proto < addr + sz:
            fo = o + (proto - addr)
            break
    if fo is None:
        print("protocols table: _protocols at %#x not in any section" % proto)
        return 2
    bad = False
    for slot in sorted(SLOTS):
        name = SLOTS[slot]
        v = struct.unpack("<Q", data[fo + slot * 8:fo + slot * 8 + 8])[0]
        target = v & MASK36
        want = handlers.get(name)
        if v == 0:
            print("protocols table: slot %2d (%s): NULL (handler missing)"
                  % (slot, name))
            bad = True
        elif want is not None and target != want:
            print("protocols table: slot %2d (%s): target %#x, want %#x "
                  "(MISMATCH)" % (slot, name, target, want))
            bad = True
        else:
            print("protocols table: slot %2d (%s): target %#x ok"
                  % (slot, name, target))
    if bad:
        print("protocols table: ws handler NOT correctly linked")
        return 1
    print("protocols table: ws handler present at slot 60")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: check_protocols_table.py <mach-o-binary>",
              file=sys.stderr)
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
