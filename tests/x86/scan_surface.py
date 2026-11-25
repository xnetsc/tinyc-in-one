#!/usr/bin/env python3
"""Coverage accounting for the shipped x86 intrinsic headers.

Compares the macro names in vendor-tinycc/include/*intrin.h
against the intrinsic names declared by gcc 13's x86 intrinsic
headers, classifies every residual, checks that each macro
built on a compiler builtin names an opcode the backend tables
contain, and reports which shipped macros no probe exercises.
"""
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
INCLUDE = os.path.join(ROOT, "vendor-tinycc", "include")
GEN = os.path.join(ROOT, "vendor-tinycc", "x86_64-gen.c")
GCC_INC = "/usr/lib/gcc/x86_64-linux-gnu/13/include"
PROBES = os.path.join(ROOT, "tests", "x86", "probes")

GCC_HEADERS = [
    "xmmintrin.h", "emmintrin.h", "tmmintrin.h", "smmintrin.h",
    "nmmintrin.h", "avxintrin.h", "avx2intrin.h", "fmaintrin.h",
    "f16cintrin.h", "avx512fintrin.h", "avx512bwintrin.h",
    "avx512vlintrin.h", "avx512dqintrin.h", "avx512cdintrin.h",
    "avx512vbmiintrin.h", "avx512vnniintrin.h", "immintrin.h",
    "pmmintrin.h", "mmintrin.h",
    "avx512vlbwintrin.h", "avx512vldqintrin.h",
    "avx512vlvnniintrin.h", "avx512vbmivlintrin.h",
    "avx512vnnivlintrin.h",
    # extension headers outside the design's feature model;
    # their names land in the residual classes below
    "avx512bf16intrin.h", "avx512bf16vlintrin.h",
    "avx512bitalgintrin.h", "avx512fp16intrin.h",
    "avx512fp16vlintrin.h", "avx512ifmaintrin.h",
    "avx512ifmavlintrin.h", "avx512vbmi2intrin.h",
    "avx512vbmi2vlintrin.h", "avx512vpopcntdqintrin.h",
    "avx512vpopcntdqvlintrin.h", "avx512erintrin.h",
    "avx512pfintrin.h", "avx5124fmapsintrin.h",
    "avx5124vnniwintrin.h", "avx512vp2intersectintrin.h",
    "avx512vp2intersectvlintrin.h",
]
EXT_HEADERS = {
    "avx512bf16intrin.h", "avx512bf16vlintrin.h",
    "avx512bitalgintrin.h", "avx512fp16intrin.h",
    "avx512fp16vlintrin.h", "avx512ifmaintrin.h",
    "avx512ifmavlintrin.h", "avx512vbmi2intrin.h",
    "avx512vbmi2vlintrin.h", "avx512vpopcntdqintrin.h",
    "avx512vpopcntdqvlintrin.h", "avx512erintrin.h",
    "avx512pfintrin.h", "avx5124fmapsintrin.h",
    "avx5124vnniwintrin.h", "avx512vp2intersectintrin.h",
    "avx512vp2intersectvlintrin.h",
}
OUR_HEADERS = [
    "xmmintrin.h", "emmintrin.h", "tmmintrin.h", "smmintrin.h",
    "nmmintrin.h", "avxintrin.h", "avx2intrin.h", "fmaintrin.h",
    "f16cintrin.h", "avx512fintrin.h", "avx512bwintrin.h",
    "avx512vlintrin.h", "avx512dqintrin.h", "avx512cdintrin.h",
    "avx512vbmiintrin.h", "avx512vnniintrin.h", "immintrin.h",
]

# residual classes by name pattern, matched in order after the
# source-header classes (extension-only, MMX-only, SSE3-only);
# anything unmatched is a genuine gap and fails the scan
CLASSES = [
    ("embedded-rounding forms of two-immediate operations "
     "(fixupimm, range, getmant, reduce, roundscale, cmp and "
     "comi carry a control immediate beside the rounding "
     "selector; the generics carry one immediate)",
     re.compile(r"(fixupimm|range|getmant|reduce|roundscale|"
                r"cmp|comi)_round|_roundscale_round")),
    ("horizontal reductions (composite operations: no single "
     "instruction and no generic in the design)",
     re.compile(r"_reduce_")),
    ("SSE3 operations at any width (horizontal add and "
     "subtract, addsub, the duplicated-move forms and lddqu: "
     "the design's feature word has no SSE3 bit)",
     re.compile(r"_(addsub|hadd|hsub|hadds|hsubs|movsldup|"
                r"movshdup|movddup|movedup|movehdup|moveldup|"
                r"lddqu)")),
    ("subvector forms (cross-width casts, set4/setr4, the "
     "128-bit-source broadcast, insert and element insert / "
     "extract forms at wider widths: the generic layer has "
     "no subvector insert or broadcast)",
     re.compile(r"_cast|_set4|_setr4|_broadcast|_insert|"
                r"_extract_epi|_zext|_setr?_m128")),
    ("two-pointer load and store forms (loadu2/storeu2 have "
     "no generic)",
     re.compile(r"_(loadu2|storeu2)_")),
    ("expand-load memory forms (the masked load generic has "
     "no expand form)",
     re.compile(r"_expandload")),
    ("masked element alignr forms (the element-count "
     "immediate cannot be scaled by a generic form)",
     re.compile(r"_maskz?_alignr_epi(32|64)")),
    ("mask register arithmetic (integer operands only: no "
     "vector operand for a generic to bind)",
     re.compile(r"^_mm[0-9]*_k[a-z]")),
    ("scalar instructions (CRC32, POPCNT and other GPR-only "
     "operations are not vector intrinsics)",
     re.compile(r"^_mm_crc32|^_mm_popcnt|^_mm_tzcnt")),
    ("system operations (fences, MXCSR, prefetch, cache "
     "control: no zero-operand generic form in the design)",
     re.compile(r"^_mm[0-9]*_(sfence|mfence|lfence|pause|"
                r"clflush|clflushopt|clwb|clzero|getcsr|setcsr|"
                r"prefetch|monitor|mwait|rdpid|serialize|tpause|"
                r"umonitor|umwait|zeroupper|zeroall)")),
    ("string instructions with fixed-register operands "
     "(pcmpestri/pcmpestrm and the pcmpistri result forms "
     "read lengths from rax/rdx, maskmovdqu stores through "
     "rdi)",
     re.compile(r"^_mm_cmp(est|ist)|^_mm_maskmove")),
    ("MMX forms declared outside mmintrin.h (the p/i/u "
     "operand spellings in the SSE header family: 64-bit MMX "
     "operands have no register home in the design's "
     "register model)",
     re.compile(r"(^|_)p[iu][0-9]|[tv]p[iu][0-9]|(^|_)pi$|"
                r"2pi|mul_su32|"
                r"^_mm_(set|setr|set1)_epi64$")),
    ("aligned allocation entry points (library functions "
     "with no instruction form)",
     re.compile(r"^_mm_(malloc|free)$")),
    ("non-temporal load and GPR store forms (movntdqa has "
     "no non-temporal load generic in the design; movnti "
     "stores a general register, not a vector)",
     re.compile(r"_stream_(load_si|si32|si64)")),
    ("prefetch gathers and scatters (the prefetch forms of "
     "the AVX-512 PF extension)",
     re.compile(r"(gather|scatter)pf")),
]


def gcc_names():
    """intrinsic name -> set of headers declaring it"""
    by = {}
    for h in GCC_HEADERS:
        p = os.path.join(GCC_INC, h)
        if not os.path.exists(p):
            continue
        text = open(p).read()
        ns = set(re.findall(r"(_mm[0-9]*_[a-z0-9_]+)\s*\(", text))
        ns |= set(re.findall(r"(?m)^#define (_mm[0-9]*_[a-z0-9_]+)",
                             text))
        for n in ns:
            by.setdefault(n, set()).add(h)
    return by


def our_macros():
    out = {}
    for h in OUR_HEADERS:
        p = os.path.join(INCLUDE, h)
        if not os.path.exists(p):
            continue
        text = open(p).read()
        for m in re.finditer(r"(?m)^#define (_mm[0-9a-z_]+)\(([^)]*)\) (.*)$",
                             text):
            out[m.group(1)] = m.group(3)
    return out


def table_opcs():
    text = open(GEN).read()
    tables = {}
    for fam in ("op1", "op1i", "op1m", "op1m2", "op1mi",
                "op1mi2", "op2", "op2i", "op3", "op3i", "op3m",
                "op3m2", "op3mi", "op3mi2", "opm", "opm2", "opmi",
                "opmi2"):
        m = re.search(r"x86_%s_table\[\] = \{(.*?)\n\};"
                      % fam, text, re.S)
        rows = set()
        if m:
            for r in re.finditer(r"\.opc = (-?\d+)", m.group(1)):
                rows.add(int(r.group(1)))
        tables[fam] = rows
    return tables


def main():
    by = gcc_names()
    gcc = set(by)
    ours = our_macros()
    tables = table_opcs()
    covered = sorted(gcc & set(ours))
    residual = sorted(gcc - set(ours))
    extra = sorted(set(ours) - gcc)
    print("gcc x86 intrinsic names:    %d" % len(gcc))
    print("shipped macros:             %d" % len(ours))
    print("covered (in both):          %d" % len(covered))
    print("shipped but not in gcc:     %d" % len(extra))
    for n in extra:
        print("   extra:", n)
    by_class = {}
    gaps = []

    def put(label, name):
        by_class.setdefault(label, []).append(name)

    for name in residual:
        srcs = by[name]
        if srcs & EXT_HEADERS and not (srcs - EXT_HEADERS):
            put("AVX-512 extensions outside the design's "
                "feature model (BF16, FP16, BITALG, IFMA, "
                "VBMI2, VPOPCNTDQ, ER, PF, 4FMAPS, 4VNNIW, "
                "VP2INTERSECT)", name)
            continue
        if srcs <= {"mmintrin.h"}:
            put("MMX surface (64-bit vectors have no register "
                "home in the design's register model)", name)
            continue
        if srcs <= {"pmmintrin.h"}:
            put("SSE3 surface (the design's feature word has "
                "no SSE3 bit)", name)
            continue
        for label, rx in CLASSES:
            if rx.search(name):
                put(label, name)
                break
        else:
            gaps.append(name)
    print("residual (gcc only):        %d" % len(residual))
    for label, names in sorted(by_class.items(),
                               key=lambda kv: -len(kv[1])):
        print("  %-58s %d" % (label[:58], len(names)))
    if gaps:
        print("UNCLASSIFIED GAPS (%d):" % len(gaps))
        for n in gaps:
            print("  ", n)
    # opcode tracing: every macro whose body calls a vector
    # builtin must name an opcode present in the backend tables
    bad = []
    fams = ("op1", "op1i", "op1m", "op1m2", "op1mi", "op1mi2",
            "op2", "op2i", "op3", "op3i", "op3m", "op3m2",
            "op3mi", "op3mi2", "opm", "opm2", "opmi", "opmi2")
    fam_of = {"v" + f: f for f in fams}
    for name, body in sorted(ours.items()):
        m = re.search(r"__builtin_ia32_(vop1m2|vop1mi2|vop3m2|"
                      r"vop3mi2|vop1m|vop1mi|vop3m|vop3mi|vopm2|"
                      r"vopmi2|vop1|vop1i|vop2|vop2i|vop3|vop3i|"
                      r"vopm|vopmi)\((\d+)", body)
        if not m:
            continue
        fam, opc = fam_of[m.group(1)], int(m.group(2))
        if opc not in tables[fam]:
            bad.append((name, fam, opc))
    if bad:
        print("MACROS NAMING MISSING OPCODES (%d):" % len(bad))
        for b in bad:
            print("  ", b)
    # probe exercise: which shipped names no probe mentions
    probe_text = ""
    for f in sorted(os.listdir(PROBES)):
        if f.endswith(".c"):
            probe_text += open(os.path.join(PROBES, f)).read()
    untested = [n for n in sorted(ours) if n not in probe_text]
    print("shipped but untested:       %d" % len(untested))
    for n in untested:
        print("   untested:", n)
    if gaps or bad or extra:
        return 1
    print("scan: no unclassified gaps, no dangling opcodes, "
          "no extra names")
    return 0


if __name__ == "__main__":
    sys.exit(main())
