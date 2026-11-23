#!/usr/bin/env python3
"""Clear the DF_1_PIE bit in DT_FLAGS_1 of an ELF PIE executable.

glibc's dlopen refuses files carrying DF_1_PIE ("cannot dynamically
load position-independent executable"). The bit only records how the
file was linked; execution through the kernel and the dynamic linker
does not depend on it. Clearing it turns a PIE into a file that both
executes normally and dlopens as a shared library.
"""
import struct, sys

DF_1_PIE = 0x08000000
DT_FLAGS_1 = 0x6FFFFFFB
PT_DYNAMIC = 2

def main(path):
    data = bytearray(open(path, "rb").read())
    if data[:4] != b"\x7fELF" or data[4] != 2:
        sys.exit("not an ELF64 file: " + path)
    phoff, = struct.unpack_from("<Q", data, 0x20)
    phentsize, phnum = struct.unpack_from("<HH", data, 0x36)
    for i in range(phnum):
        off = phoff + i * phentsize
        p_type, = struct.unpack_from("<I", data, off)
        if p_type != PT_DYNAMIC:
            continue
        p_offset, = struct.unpack_from("<Q", data, off + 8)
        for j in range(1024):
            doff = p_offset + j * 16
            tag, val = struct.unpack_from("<qQ", data, doff)
            if tag == DT_FLAGS_1:
                if val & DF_1_PIE:
                    struct.pack_into("<Q", data, doff + 8, val & ~DF_1_PIE)
                    open(path, "wb").write(data)
                    print(f"{path}: DF_1_PIE cleared (FLAGS_1 {val:#x} -> {val & ~DF_1_PIE:#x})")
                else:
                    print(f"{path}: DF_1_PIE already clear")
                return
            if tag == 0:
                break
        sys.exit("DT_FLAGS_1 not found in " + path)
    sys.exit("PT_DYNAMIC not found in " + path)

if __name__ == "__main__":
    main(sys.argv[1])
