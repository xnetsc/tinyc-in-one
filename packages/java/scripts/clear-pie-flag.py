#!/usr/bin/env python3
"""清掉 ELF 文件 DT_FLAGS_1 中的 DF_1_PIE 位。

以 -pie 链接的 ET_DYN 文件带 DF_1_PIE 标志，glibc 的 dlopen 会以
"cannot dynamically load position-independent executable" 拒绝加载。
清掉该位后，同一个文件既可由内核按可执行文件拉起，也可被 dlopen
当作动态库加载（与 libc.so.6 的双形态结构一致）。

用法: clear-pie-flag.py <elf-file>（原地修改）
"""
import struct
import sys

DT_FLAGS_1 = 0x6FFFFFFB
DF_1_PIE = 0x08000000


def main() -> int:
    path = sys.argv[1]
    with open(path, "r+b") as f:
        ident = f.read(64)
        if ident[:4] != b"\x7fELF" or ident[4] != 2:
            print(f"not an ELF64 file: {path}", file=sys.stderr)
            return 1
        e_phoff = struct.unpack_from("<Q", ident, 32)[0]
        e_phentsize = struct.unpack_from("<H", ident, 54)[0]
        e_phnum = struct.unpack_from("<H", ident, 56)[0]

        dyn_off = None
        for i in range(e_phnum):
            f.seek(e_phoff + i * e_phentsize)
            ph = f.read(56)
            p_type, _p_flags, p_offset = struct.unpack_from("<IIQ", ph, 0)
            if p_type == 2:  # PT_DYNAMIC
                dyn_off = p_offset
                break
        if dyn_off is None:
            print(f"no PT_DYNAMIC segment: {path}", file=sys.stderr)
            return 1

        for j in range(512):
            off = dyn_off + j * 16
            f.seek(off)
            tag, val = struct.unpack_from("<qQ", f.read(16))
            if tag == 0:
                break
            if tag == DT_FLAGS_1:
                if val & DF_1_PIE:
                    f.seek(off)
                    f.write(struct.pack("<qQ", tag, val & ~DF_1_PIE))
                    print(f"cleared DF_1_PIE in {path} (FLAGS_1 {val:#x} -> {val & ~DF_1_PIE:#x})")
                else:
                    print(f"DF_1_PIE not set in {path}, nothing to do")
                return 0
        print(f"DT_FLAGS_1 not found in {path}, nothing to do")
        return 0


if __name__ == "__main__":
    sys.exit(main())
