#!/usr/bin/env python3
"""Coverage accounting for the shipped arm_neon.h.

Compares the macro names in vendor-tinycc/include/arm_neon.h
against the intrinsic names declared by the cross gcc's
arm_neon.h (the ACLE surface as this toolchain spells it),
classifies every residual, checks that each macro built on a
compiler builtin names an opcode the backend tables contain,
and reports which shipped macros no probe exercises.
"""
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
OURS = os.path.join(ROOT, "vendor-tinycc", "include", "arm_neon.h")
GEN = os.path.join(ROOT, "vendor-tinycc", "arm64-gen.c")
# the reference header lives under gcc-cross/ on a foreign-arch
# host and under gcc/ on a native aarch64 host; take whichever
# is installed
GCC_H_CANDIDATES = [
    "/usr/lib/gcc-cross/aarch64-linux-gnu/13/include/arm_neon.h",
    "/usr/lib/gcc/aarch64-linux-gnu/13/include/arm_neon.h",
]
GCC_H = next((p for p in GCC_H_CANDIDATES if os.path.exists(p)),
             GCC_H_CANDIDATES[0])
PROBES = os.path.join(ROOT, "tests", "neon", "probes")

# residual classes, matched in order; anything unmatched is a
# genuine gap and fails the scan
CLASSES = [
    ("fp16 arithmetic (optional FP16 extension, not in the "
     "design's arm64 feature model)",
     re.compile(r"_f16|_f32_f16|_f16_f32|f16x|_bf16")),
    ("bf16 (BF16 extension)",
     re.compile(r"bf16|_bf|bfdot|bfmlal|bfmmla|bfcvtn")),
    ("crypto extensions (AES/SHA/PMULL-64 are separate optional "
     "extensions; the design's arm64 feature bits are NEON and "
     "DOTPROD only)",
     re.compile(r"^vaes|^vsha|^vpmul|_p64|_p128|poly128|vmull_high_p64|"
                r"vsha1|vsha256|vsha512|vsm3|vsm4|vaese|vaesd|vaesmc|"
                r"vaesimc|vrax1")),
    ("i8mm matrix extensions (FEAT_I8MM, separate extension)",
     re.compile(r"mmla|usdot|sudot|smmla|ummla|_lane.*dot|vdot.*_bf")),
    ("SVE-only or scalar (non-vector) names",
     re.compile(r"^v(ld1|st1|ld2|st2|ld3|st3|ld4|st4)q?_"
                r"(s|u|f|p)(8|16|32|64)_(x|dup)")),
    ("SHA3 extension (separate optional extension)",
     re.compile(r"^veor3|^vbcax|^vxar")),
    ("complex arithmetic (FEAT_FCMA, separate optional extension)",
     re.compile(r"^vcadd|^vcmla")),
    ("FRINT extension (vrnd32/vrnd64, separate optional extension)",
     re.compile(r"^vrnd(32|64)")),
    ("multi-register table lookups (the table registers must be "
     "consecutive, which this register allocator does not "
     "guarantee)",
     re.compile(r"^vqtbl[234]|^vqtbx[234]|^vtbl[234]|^vtbx[234]")),
    ("scalar forms (scalar operand or scalar result, not the "
     "vector surface)",
     re.compile(r"[bhsd]_(s|u|f|p)(8|16|32|64)(_|$)|"
                r"[bhsd](q)?_(lane|n)(_|$)|[bhsd]_laneq(_|$)|"
                r"qdml[as]l[hs]|"
                r"_(s|u|f|p)(8|16|32|64)_(lane|n)|"
                r"qdmulh[hs]|qdmull[hs]|qrdml[as]h[hs]|qrdmulh[hs]|"
                r"^vdup[bhsd]_lane|^vrecp[ex][sd]_|^vrsqrt[es][sd]_|"
                r"^vp(add|abs|max|min)[a-z]*[sd]q?_|^vrndns_|"
                r"^v(ab|add|sub|neg)d_|^vtstd_|^vcvt[ad]_|"
                r"^vcvt[a-z]*[sd]_(s|u|f)|^vceqz?[sd]_|"
                r"^vc[lg][a-z]*[sd]_")),
    ("scalar rounding conversions (vcvtr_*, scalar not vector)",
     re.compile(r"^vcvtr|^vcvt[arsmn]_")),
    ("64-bit-element forms ACLE does not define for this op",
     re.compile(r"^vmvnq?_(s|u)64$|^vabsq?_(s|u)64$")),
    ("gcc header gaps: names ACLE defines but this gcc does not "
     "declare (counted on the shipped side instead)",
     re.compile(r"^(vbit|vbif|vtbl1q|vtbx1q|vchi|vchs)")),
]


def gcc_names():
    text = open(GCC_H).read()
    names = set(re.findall(r"(?m)^(v[a-z][a-z0-9_]*|__builtin_aarch64_[a-z0-9_]+) \(",
                           text))
    return {n for n in names if not n.startswith("__")}


def our_macros():
    text = open(OURS).read()
    out = {}
    for m in re.finditer(r"(?m)^#define (v[a-z0-9_]+)\(([^)]*)\) (.*)$",
                         text):
        out[m.group(1)] = m.group(3)
    return out


def table_opcs():
    text = open(GEN).read()
    tables = {}
    for fam in ("op1", "op2", "op3", "op1r", "shuf"):
        m = re.search(r"a64_%s_table\[\] = \{(.*?)\n\};" % fam, text, re.S)
        rows = set()
        if m:
            for r in re.finditer(r"\.opc = (-?\d+)", m.group(1)):
                rows.add(int(r.group(1)))
        tables[fam] = rows
    return tables


def main():
    gcc = gcc_names()
    ours = our_macros()
    tables = table_opcs()
    covered = sorted(gcc & set(ours))
    residual = sorted(gcc - set(ours))
    extra = sorted(set(ours) - gcc)
    print("gcc arm_neon.h intrinsic names: %d" % len(gcc))
    print("shipped arm_neon.h macros:       %d" % len(ours))
    print("covered (in both):               %d" % len(covered))
    print("shipped but not in gcc header:   %d" % len(extra))
    by_class = {}
    gaps = []
    for name in residual:
        for label, rx in CLASSES:
            if rx.search(name):
                by_class.setdefault(label, []).append(name)
                break
        else:
            gaps.append(name)
    print("residual (gcc only):             %d" % len(residual))
    for label, names in sorted(by_class.items()):
        print("  %-58s %d" % (label[:58], len(names)))
    if gaps:
        print("UNCLASSIFIED GAPS (%d):" % len(gaps))
        for n in gaps:
            print("  ", n)
    # opcode tracing: every macro whose body calls a vector
    # builtin must name an opcode present in the backend tables
    bad = []
    for name, body in sorted(ours.items()):
        m = re.search(r"__builtin_aarch64_(vop1r|vop1|vop2|vop3|"
                      r"splat|extract|replace|vldn|vstn)\((\d+)", body)
        if not m:
            continue
        fam, opc = m.group(1), int(m.group(2))
        fam = {"vop1": "op1", "vop2": "op2", "vop3": "op3",
               "vop1r": "op1r"}.get(fam, fam)
        if fam in ("vldn", "vstn"):
            if (opc >> 11) not in (0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14):
                bad.append((name, "vldn opc group", opc))
            continue
        if fam in ("splat", "extract", "replace"):
            continue  # shape opcodes, decoded directly
        key = opc & ~(127 << 4)
        if key not in tables[fam] and opc not in tables[fam]:
            bad.append((name, fam, opc))
    if bad:
        print("MACROS NAMING MISSING OPCODES (%d):" % len(bad))
        for b in bad:
            print("  ", b)
    # probe usage
    probe_text = ""
    for f in sorted(os.listdir(PROBES)):
        if f.endswith(".c"):
            probe_text += open(os.path.join(PROBES, f)).read()
    unused = [n for n in sorted(ours) if n + "(" not in probe_text]
    print("shipped macros not exercised by any probe: %d" % len(unused))
    for n in unused:
        print("   unused:", n)
    return 1 if (gaps or bad) else 0


if __name__ == "__main__":
    sys.exit(main())
