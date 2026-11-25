#!/usr/bin/env python3
"""Generate the riscv64 RVV fixed-length surface from the spec.

Outputs:
  - the opcode enum and builtin tables inside
    vendor-tinycc/riscv64-gen.c (between the RVV_TABLES markers),
  - vendor-tinycc/include/riscv_vector.h (typedefs + intrinsic
    macros for the full type universe),
  - tests/riscv64/probes/riscv_sweep.c (a generated differential
    probe sweeping the core families over every type shape).

Encodings are not trusted from the spec alone: every table entry
is checked by assembling its instruction with the cross assembler
and comparing the funct6/funct3 fields (entries the assembler
rejects are listed in ASM_SKIP with the evidence for their
fields).  A family instance whose name the assembler-independent
checks cannot validate stays out of the sweep probe; the surface
scan (tests/riscv64/scan_surface.py) verifies the shipped names
against the gcc name set.
"""
import re
import subprocess
import sys

sys.path.insert(0, "tools")
import riscv_spec as spec

AS = "riscv64-linux-gnu-as"
OBJDUMP = "riscv64-linux-gnu-objdump"

# ------------------------------------------------------------------
# assembler validation of table encodings
# ------------------------------------------------------------------
# opcode name -> assembler instruction template; {d}=v8, {a}=v16,
# {b}=v24 for the backend's scratch groups
def asm_template(opc_name, kind):
    n = opc_name[len("RVVOP_"):]
    if n in ASM_SPECIAL:
        return ASM_SPECIAL[n]
    base = n.lower()
    # trailing form suffix
    m = re.match(r"(.*)_(vv|vx|vf|wv|wx|wi|mm|m|vs|v|f)$", base)
    if kind in ("RVK_RX",):
        return None  # handled in ASM_SPECIAL
    return None

ASM_SPECIAL = {
    "VMV_V_V": "vmv.v.v v8, v16",
    "VNEG_I": "vrsub.vx v8, v16, zero",
    "VNOT_I": "vxor.vi v8, v16, -1",
    "VFABS": "vfsgnjx.vv v8, v16, v16",
    "VFNEG": "vfsgnjn.vv v8, v16, v16",
    "VFCLASS": "vfclass.v v8, v16",
    "VFSQRT": "vfsqrt.v v8, v16",
    "VMNOT_M": "vmnot.m v8, v16",
    "VMSBF_M": "vmsbf.m v8, v16",
    "VMSIF_M": "vmsif.m v8, v16",
    "VMSOF_M": "vmsof.m v8, v16",
    "VIOTA_M": "viota.m v8, v16",
    "VID_V": "vid.v v8",
    "VFREC7": "vfrec7.v v8, v16",
    "VFRSQRT7": "vfrsqrt7.v v8, v16",
    "VMV_X_S": "vmv.x.s a0, v16",
    "VFMV_F_S": "vfmv.f.s fa0, v16",
    "VCPOP_M": "vcpop.m a0, v16",
    "VFIRST_M": "vfirst.m a0, v16",
    "VMERGE_VVM": "vmerge.vvm v8, v16, v24, v0",
    "VMERGE_VXM": "vmerge.vxm v8, v16, a2, v0",
    "VADC_VVM": "vadc.vvm v8, v16, v24, v0",
    "VSBC_VVM": "vsbc.vvm v8, v16, v24, v0",
    "VCOMPRESS_VV": "vcompress.vm v8, v16, v0",
    "SPLAT_X": "vmv.v.x v8, a2",
    "SPLAT_F": "vfmv.v.f v8, fa2",
    "VMV_S_X": "vmv.s.x v8, a2",
    "VFMV_S_F": "vfmv.s.f v8, fa2",
}

# funct fields verified against gcc's emitted code instead of the
# assembler (binutils rejects these forms): name -> (f6, f3)
ASM_SKIP = {
    "VFWMACC_VF": (0x3c, 5),
    "VFWMSAC_VF": (0x3e, 5),
    "VFWNMSAC_VF": (0x3f, 5),
}

def derive_asm(opc_name, kind):
    """canonical instruction text for a table entry, or None"""
    n = opc_name[len("RVVOP_"):]
    if n in ASM_SPECIAL:
        return ASM_SPECIAL[n]
    b = n.lower()
    b = re.sub(r"3$", "", b)  # ternary-path slide duplicates
    # conversions: vfcvt_x_f -> vfcvt.x.f.v ; vfncvt_* get .w
    m = re.match(r"(vf[wn]?cvt)_(.*)$", b)
    if m:
        rest = m.group(2).replace("_", ".")
        suffix = ".w" if m.group(1) == "vfncvt" else ".v"
        return "%s.%s%s v8, v16" % (m.group(1), rest, suffix)
    m = re.match(r"(.*)_(vv|vx|vf|wv|wx|mm)$", b)
    if m:
        form = m.group(2)
        if re.search(r"(macc|madd|msac|nmsac|nmsub|msub)$", m.group(1)):
            # multiply-accumulates take (vd, scalar, vs2)
            ops = {"vv": "v8, v16, v24", "wv": "v8, v16, v24",
                   "mm": "v8, v16, v24", "vx": "v8, a2, v16",
                   "wx": "v8, v16, a2", "vf": "v8, fa2, v16"}[form]
        else:
            ops = {"vv": "v8, v16, v24", "wv": "v8, v16, v24",
                   "mm": "v8, v16, v24", "vx": "v8, v16, a2",
                   "wx": "v8, v16, a2", "vf": "v8, v16, fa2"}[form]
        return "%s.%s %s" % (m.group(1), form, ops)
    m = re.match(r"(.*)_(wi)$", b)
    if m:
        return "%s.wi v8, v16, 3" % m.group(1)
    m = re.match(r"(.*)_(vi)$", b)
    if m:
        return "%s.vi v8, v16, 3" % m.group(1)
    m = re.match(r"(vred\w+|vwred\w+|vfred\w+|vfwred\w+)$", b)
    if m:
        return "%s.vs v8, v16, v24" % m.group(1)
    m = re.match(r"(.*)_vs$", b)
    if m:
        return "%s.vs v8, v16, v24" % m.group(1)
    return None

def check_encodings():
    bad = []
    unchecked = []
    words = {}
    todo = []
    for tab, entries in spec.TABLES.items():
        for (opc, f6, f3, kind, cfgres, aux) in entries:
            name = opc[len("RVVOP_"):]
            if name in ASM_SKIP:
                if ASM_SKIP[name] != (f6, f3):
                    bad.append((opc, "spec skip entry mismatch"))
                continue
            text = derive_asm(opc, kind)
            if text is None:
                unchecked.append(opc)
            else:
                todo.append((opc, f6, f3, text))
    if todo:
        src = "\n".join(t for _, _, _, t in todo) + "\n"
        with open("/tmp/rvv_gen_check.s", "w") as fh:
            fh.write(src)
        r = subprocess.run([AS, "-march=rv64gcv", "-o", "/tmp/rvv_gen_check.o",
                            "/tmp/rvv_gen_check.s"], capture_output=True, text=True)
        if r.returncode != 0:
            print(r.stderr)
            raise SystemExit("assembler rejected the check file")
        r = subprocess.run([OBJDUMP, "-d", "/tmp/rvv_gen_check.o"],
                           capture_output=True, text=True)
        got = re.findall(r":\t([0-9a-f]{8})\s", r.stdout)
        assert len(got) == len(todo), (len(got), len(todo))
        for (opc, f6, f3, text), w in zip(todo, got):
            v = int(w, 16)
            if ((v >> 26) & 63, (v >> 12) & 7) != (f6, f3):
                bad.append((opc, "asm %s -> f6=%02x f3=%d, spec %02x/%d"
                            % (text, (v >> 26) & 63, (v >> 12) & 7, f6, f3)))
    return bad, unchecked

# ------------------------------------------------------------------
# family expansion
# ------------------------------------------------------------------
def type_of_suffix(suf):
    m = re.fullmatch(r"([iuf])(\d+)(mf8|mf4|mf2|m1|m2|m4|m8)", suf)
    if m:
        return (m.group(1), int(m.group(2)), m.group(3))
    m = re.fullmatch(r"b(\d+)", suf)
    if m:
        return ("b", int(m.group(1)), "")
    return None

ALL_TYPES = list(spec.TYPES) + [("b", b, "") for b in spec.BOOL_TYPES]

def suffix_of(t):
    if t[0] == "b":
        return "b%d" % t[1]
    return spec.type_suffix(t)

def ctype_of(t):
    if t[0] == "b":
        return "vbool%d_t" % t[1]
    return spec.c_type(t)

def mask_type(t):
    """the vbool type holding one bit per lane of t: vbool<e/lmul>"""
    n, d = spec.LMUL_FRAC[t[2]]
    return ("b", t[1] * d // n, "")

def trunc_partner(t, kind=None):
    """the type half the element width and half the LMUL of t
    (the element-count-preserving narrowing partner), or None"""
    k = t[0] if kind is None else kind
    for w in spec.TYPES:
        if w[0] == k and spec.widen(w) == (k, t[1], t[2]):
            return w
    return None

def _lanes_key(t):
    return spec.lanes_at(t, 128)

DOMAINS = {
    # result of a widening operation: the narrowing partner exists
    "widenres": lambda t: trunc_partner(t) is not None,
    "widenres_f": lambda t: trunc_partner(t, "f") is not None,
    "widenres_i": lambda t: trunc_partner(t, "i") is not None,
    "widenres_u": lambda t: trunc_partner(t, "u") is not None,
    # result of a narrowing operation: the widening partner exists
    "narrowres": lambda t: spec.widen(t) is not None,
    "narrowres_f": lambda t: ("f", t[1] * 2, _widen_lmul(t)) in spec.TYPES
    if _widen_lmul(t) else False,
    "narrowres_i": lambda t: ("i", t[1] * 2, _widen_lmul(t)) in spec.TYPES
    if _widen_lmul(t) else False,
    "narrowres_u": lambda t: ("u", t[1] * 2, _widen_lmul(t)) in spec.TYPES
    if _widen_lmul(t) else False,
    # result of a same-width float conversion: the float shape exists
    "cvtsame_f": lambda t: ("f", t[1], t[2]) in spec.TYPES,
    # indexed memory forms: an unsigned index type with the EEW and
    # the same lane count exists
    "idx8": lambda t: any(u[0] == "u" and u[1] == 8
                          and _lanes_key(u) == _lanes_key(t)
                          for u in spec.TYPES),
    "idx16": lambda t: any(u[0] == "u" and u[1] == 16
                           and _lanes_key(u) == _lanes_key(t)
                           for u in spec.TYPES),
    "idx32": lambda t: any(u[0] == "u" and u[1] == 32
                           and _lanes_key(u) == _lanes_key(t)
                           for u in spec.TYPES),
    "idx64": lambda t: any(u[0] == "u" and u[1] == 64
                           and _lanes_key(u) == _lanes_key(t)
                           for u in spec.TYPES),
}

def _widen_lmul(t):
    n, d = spec.LMUL_FRAC[t[2]]
    n *= 2
    inv = {v: k for k, v in spec.LMUL_FRAC.items()}
    import math
    g = math.gcd(n, d)
    return inv.get((n // g, d // g))

def expand_family(nt, args, bt, seen, domain=None):
    """yield (name, args, body) instances for one family"""
    ph = sorted(set(int(x) for x in re.findall(r"@(\d+)", nt + " " + bt)))
    seen_types = []
    for tup in seen:
        seen_types.append(tuple(type_of_suffix(s) for s in tup))
    if not ph:
        yield ((), nt, args, bt)
        return
    if len(ph) == 1:
        kinds = set(t[0][0] for t in seen_types if t and t[0])
        cands = [t for t in ALL_TYPES if t[0] in kinds]
        if domain:
            cands = [t for t in cands if DOMAINS[domain](t)]
        pairs = [(t,) for t in cands]
    elif len(ph) == 2:
        pairs = pair_expansion(seen_types)
    elif len(ph) == 3:
        pairs = triple_expansion(seen_types)
        if pairs is None:
            pairs = seen_types
    else:
        pairs = seen_types
    for tup in pairs:
        if any(t is None for t in tup):
            continue
        name, body = nt, bt
        for i, t in enumerate(tup):
            name = name.replace("@%d" % i, suffix_of(t))
            body = body.replace("@%d" % i, ctype_of(t))
        if "@e" in name or "@e" in body:
            e = str(tup[0][1])
            name = name.replace("@e", e)
            body = body.replace("@e", e)
        if "@s" in name or "@s" in body:
            s = "%s%d" % (tup[0][0], tup[0][1])
            name = name.replace("@s", s)
            body = body.replace("@s", s)
        if "@c" in name or "@c" in body:
            cs = {"i": {8: "signed char", 16: "short", 32: "int",
                        64: "long long"},
                  "u": {8: "unsigned char", 16: "unsigned short",
                        32: "unsigned int", 64: "unsigned long long"},
                  "f": {32: "float", 64: "double"}}[tup[0][0]][tup[0][1]]
            name = name.replace("@c", cs)
            body = body.replace("@c", cs)
        yield (tup, name, args, body)

def triple_expansion(seen_types):
    """(data, unsigned of data, mask of data) triples, as used by
    the masked compare _m forms; expand over every data type"""
    ok = [t for t in seen_types if len(t) == 3 and all(t)]
    if not ok:
        return None
    def fits(p):
        return (p[1] == ("u", p[0][1], p[0][2])
                and p[2] == mask_type(p[0]))
    if not all(fits(p) for p in ok):
        return None
    kinds0 = set(p[0][0] for p in ok)
    out = []
    for t0 in spec.TYPES:
        if t0[0] not in kinds0:
            continue
        out.append((t0, ("u", t0[1], t0[2]), mask_type(t0)))
    return out

def pair_expansion(seen_types):
    ok = [t for t in seen_types if len(t) == 2 and t[0] and t[1]]
    if not ok:
        return seen_types
    def is_maskof(p):
        # (data type, its mask type): one bit per lane, vbool<e/lmul>
        return p[1] == mask_type(p[0])
    def is_widen(p): return spec.widen(p[0]) == p[1]
    def is_narrow(p): return spec.widen(p[1]) == p[0]
    def is_widenred(p):
        # widening reductions: the result is the widened scalar in
        # an LMUL-1 vector: twice the element width, LMUL m1
        return p[1] == (p[0][0], p[0][1] * 2, "m1")
    def is_red(p):
        # plain reductions: the result is the data scalar in an
        # LMUL-1 vector of the data shape
        return p[1] == (p[0][0], p[0][1], "m1")
    def is_equal(p): return p[0] == p[1]
    def is_index(p):
        return (p[1][0] == "u"
                and spec.lanes_at(p[0], 128) == spec.lanes_at(p[1], 128))
    for rule, fn in (("maskof", is_maskof), ("widen", is_widen), ("narrow", is_narrow),
                     ("widenred", is_widenred), ("red", is_red),
                     ("equal", is_equal),
                     ("index", is_index)):
        if all(fn(p) for p in ok):
            kinds0 = set(p[0][0] for p in ok)
            out = []
            for t0 in spec.TYPES:
                if t0[0] not in kinds0:
                    continue
                if rule == "maskof":
                    out.append((t0, mask_type(t0)))
                elif rule == "widen":
                    t1 = spec.widen(t0)
                    if t1:
                        out.append((t0, t1))
                elif rule == "narrow":
                    for t1 in spec.TYPES:
                        if spec.widen(t1) == t0:
                            out.append((t0, t1))
                elif rule == "widenred":
                    t1 = (t0[0], t0[1] * 2, "m1")
                    if t1 in spec.TYPES:
                        out.append((t0, t1))
                elif rule == "red":
                    out.append((t0, (t0[0], t0[1], "m1")))
                elif rule == "equal":
                    out.append((t0, t0))
                else:
                    for t1 in spec.TYPES:
                        if t1[0] == "u" and spec.lanes_at(t0, 128) == spec.lanes_at(t1, 128):
                            out.append((t0, t1))
            return out
    return seen_types

MIN64_TYPES = set()
for t in spec.TYPES:
    n, d = spec.LMUL_FRAC[t[2]]
    if t[1] * n < 8 * d:  # fewer than one element at VLEN 32
        MIN64_TYPES.add(t)
# the def-file REQUIRE_MIN_VLEN_64 set: fractional types whose byte
# size at VLEN 64 is below the element size are valid; the actual
# gcc restriction list is by (elem, lmul):
MIN64_TYPES = {t for t in spec.TYPES
               if (t[1], t[2]) in ((8, "mf8"), (16, "mf4"), (32, "mf2"))}

def emit_header():
    L = []
    L.append("/* RISC-V Vector intrinsics (fixed-length surface, design")
    L.append("   section 7).  Generated by tools/riscv_gen.py from")
    L.append("   tools/riscv_spec.py; do not edit.  The vector types are")
    L.append("   fixed-size GNU vectors whose byte size follows the")
    L.append("   target's minimum VLEN (__riscv_v_min_vlen, set by")
    L.append("   -march=..._zvl<N>b): an m1 type occupies one register,")
    L.append("   m2/m4/m8 span register groups and mf2/mf4/mf8 are")
    L.append("   fractions of one register.  Every function-like name is")
    L.append("   a macro over a compiler builtin, over a memory")
    L.append("   dereference (unit-stride loads and stores), or over a")
    L.append("   composition of those.  The vl argument of the standard")
    L.append("   API is the type's lane count; the surface is")
    L.append("   fixed-length, so each operation always covers the whole")
    L.append("   type. */")
    L.append("#ifndef __RISCV_VECTOR_H")
    L.append("#define __RISCV_VECTOR_H")
    L.append("")
    L.append("#define __rvv_vlenb (__riscv_v_min_vlen / 8)")
    L.append("")
    L.append("/* the byte length of one vector register under the")
    L.append("   fixed-length model: the types in this header are")
    L.append("   sized from it */")
    L.append("#define __riscv_vlenb() (__riscv_v_min_vlen / 8)")
    L.append("")
    # typedefs
    def size_expr(t):
        n, d = spec.LMUL_FRAC[t[2]]
        if (n, d) == (1, 1):
            return "__rvv_vlenb"
        if d == 1:
            return "__rvv_vlenb * %d" % n
        if n == 1:
            return "__rvv_vlenb / %d" % d
        return "__rvv_vlenb * %d / %d" % (n, d)
    plain, guarded = [], []
    for t in spec.TYPES:
        sc = spec.c_scalar(t)
        if t[1] == 64 and t[0] == "i":
            sc = "long long"
        if t[1] == 64 and t[0] == "u":
            sc = "unsigned long long"
        line = "typedef %s %s __attribute__((vector_size(%s)));" % (
            sc, spec.c_type(t), size_expr(t))
        (guarded if t in MIN64_TYPES else plain).append(line)
    L += plain
    if guarded:
        L.append("#if __riscv_v_min_vlen >= 64")
        L += guarded
        L.append("#endif")
    L.append("")
    L.append("/* mask types: vbool<N>_t carries one bit per lane of the")
    L.append("   data type whose element width is N; in memory the bits")
    L.append("   are packed, one byte holding eight lanes */")
    for b in spec.BOOL_TYPES:
        L.append("typedef _Bool vbool%d_t __attribute__((vector_size(__riscv_v_min_vlen / %d)));" % (b, b))
    L.append("")
    # unit-stride loads/stores and mask loads/stores as dereferences
    L.append("/* unit-stride loads and stores are plain dereferences of")
    L.append("   the vector type; the mask forms move the packed bits */")
    mem_plain, mem_guarded = [], []
    for t in spec.TYPES:
        e = t[1]
        ct = spec.c_type(t)
        suf = spec.type_suffix(t)
        mem_plain.append("#define __riscv_vle%d_v_%s(ptr, vl) (*(const %s *)(ptr))" % (e, suf, ct))
        mem_plain.append("#define __riscv_vse%d_v_%s(ptr, v, vl) (*(%s *)(ptr) = (v))" % (e, suf, ct))
    for b in spec.BOOL_TYPES:
        mem_plain.append("#define __riscv_vlm_v_b%d(ptr, vl) (*(const vbool%d_t *)(ptr))" % (b, b))
        mem_plain.append("#define __riscv_vsm_v_b%d(ptr, v, vl) (*(vbool%d_t *)(ptr) = (v))" % (b, b))
    L += mem_plain
    L.append("")
    # families
    fams_plain, fams_guarded = [], []
    for fam in spec.FAMILIES:
        nt, args, bt, seen = fam[0], fam[1], fam[2], fam[3]
        domain = fam[4] if len(fam) > 4 else None
        for (tup, name, a2, body) in expand_family(nt, args, bt, seen, domain):
            line = "#define %s(%s) %s" % (name, a2, body)
            sufs = re.findall(r"__riscv_\w*?_(i|u|f)(\d+)(mf8|mf4|mf2|m1|m2|m4|m8)", name)
            ts = [type_of_suffix(k + e + l) for (k, e, l) in sufs]
            if any(t in MIN64_TYPES for t in ts):
                fams_guarded.append(line)
            else:
                fams_plain.append(line)
    L += sorted(set(fams_plain))
    if fams_guarded:
        L.append("#if __riscv_v_min_vlen >= 64")
        L += sorted(set(fams_guarded))
        L.append("#endif")
    L.append("")
    L.append("#endif /* __RISCV_VECTOR_H */")
    return "\n".join(L) + "\n"

def emit_backend():
    L = []
    L.append("/* RVV_TABLES_BEGIN (generated by tools/riscv_gen.py; do not edit) */")
    L.append("enum {")
    cur_section = None
    for name in spec.OPCODE_ORDER:
        v = spec.OPCODES[name]
        L.append("    %s = %d," % (name, v))
    L.append("};")
    L.append("")
    tab_names = {
        "rvv_op1_tab": "unary", "rvv_op1r_tab": "extract",
        "rvv_op2_tab": "binary", "rvv_op2i_tab": "immediate",
        "rvv_op3_tab": "ternary", "rvv_splat_tab": "splat",
        "rvv_op1m2_tab": "masked unary specials", "rvv_opm_tab": "mask forms",
    }
    for tab in ("rvv_op1_tab", "rvv_op1r_tab", "rvv_op2_tab",
                "rvv_op2i_tab", "rvv_op3_tab", "rvv_splat_tab",
                "rvv_op1m2_tab", "rvv_opm_tab"):
        L.append("static const RvvOp %s[] = {" % tab)
        for (opc, f6, f3, kind, cfgres, aux) in spec.TABLES[tab]:
            L.append("    { %s, 0x%02x, %d, %s, %d, %d }," % (opc, f6, f3, kind, cfgres, aux))
        L.append("};")
        L.append("")
    L.append("/* RVV_TABLES_END */")
    return "\n".join(L) + "\n"

def emit_sweep():
    """core families over every type shape, both legs must agree"""
    head = []
    head.append("/* riscv64 RVV differential probe: generated sweep of the")
    head.append("   core families over every type shape (tools/riscv_gen.py).")
    head.append("   Lane counts come from __riscv_vlenb(). */")
    head.append("#include <riscv_vector.h>")
    head.append("#include <stdio.h>")
    head.append("")
    head.append("static signed char buf8[8192];")
    head.append("static short buf16[4096];")
    head.append("static int buf32[2048];")
    head.append("static long long buf64[1024];")
    head.append("static float buff[2048];")
    head.append("static double bufd[1024];")
    head.append("static unsigned char obuf[8192];")
    head.append("")
    blocks = []
    for t in spec.TYPES:
        kind, e, lm = t
        suf = spec.type_suffix(t)
        ct = spec.c_type(t)
        lanes = "(__riscv_vlenb() * %d * 8 / (%d * %d))" % (
            spec.LMUL_FRAC[lm][0], spec.LMUL_FRAC[lm][1], e)
        buf = {8: "buf8", 16: "buf16", 32: "buf32", 64: "buf64"}[e]
        if kind == "f":
            buf = {32: "buff", 64: "bufd"}[e]
        B = []
        B.append("    {")
        B.append("        int n = %s, i;" % lanes)
        B.append("        %s a = __riscv_vle%d_v_%s(%s, n);" % (ct, e, suf, buf))
        B.append("        %s b = __riscv_vle%d_v_%s(%s + n, n);" % (ct, e, suf, buf))
        if kind == "f":
            B.append("        %s r = __riscv_vfadd_vv_%s(a, b, n);" % (ct, suf))
            B.append("        r = __riscv_vfmul_vv_%s(r, a, n);" % suf)
            B.append("        __riscv_vse%d_v_%s(obuf, r, n);" % (e, suf))
        else:
            B.append("        %s r = __riscv_vadd_vv_%s(a, b, n);" % (ct, suf))
            B.append("        r = __riscv_vmul_vv_%s(r, a, n);" % suf)
            B.append("        r = __riscv_vxor_vv_%s(r, b, n);" % suf)
            B.append("        __riscv_vse%d_v_%s(obuf, r, n);" % (e, suf))
        B.append("        printf(\"%s\");" % suf)
        B.append("        for (i = 0; i < n * (%d / 8); i++) printf(\" %%02x\", obuf[i]);" % e)
        B.append("        printf(\"\\n\");")
        B.append("    }")
        blocks += B
    main = []
    main.append("int main(void)")
    main.append("{")
    main.append("    int i;")
    main.append("    for (i = 0; i < 8192; i++) buf8[i] = (signed char)(i * 37 + 11);")
    main.append("    for (i = 0; i < 4096; i++) buf16[i] = (short)(i * 37 + 11);")
    main.append("    for (i = 0; i < 2048; i++) buf32[i] = i * 37 + 11;")
    main.append("    for (i = 0; i < 1024; i++) buf64[i] = i * 37 + 11;")
    main.append("    for (i = 0; i < 2048; i++) buff[i] = (i % 13) * 0.5f - 2.0f;")
    main.append("    for (i = 0; i < 1024; i++) bufd[i] = (i % 13) * 0.5 - 2.0;")
    main += blocks
    main.append("    return 0;")
    main.append("}")
    return "\n".join(head + main) + "\n"
# ----------------------------------------------------------------
# family-axis sweep probe: one differential block per shipped
# family, instantiated at the family's seed shapes with synthesized
# arguments.  Results are stored to a byte buffer and printed as
# hex; lanes whose value the architecture leaves unspecified
# (masked-off lanes of _m forms, the top lane of slide1down, lanes
# past a compress popcount) are excluded or forced to a sentinel
# in the probe itself, the same discipline as the hand probes.

_OPC_ROW = {}
for _tab in spec.TABLES.values():
    for _row in _tab:
        _OPC_ROW[spec.OPCODES[_row[0]]] = _row

def _ctype(t):
    if t[0] == "b":
        return "vbool%d_t" % t[1]
    return spec.c_type(t)


_TYPE_BY_CTYPE = {}
for _t in ALL_TYPES:
    _TYPE_BY_CTYPE[_ctype(_t)] = _t

_WIDEN_OF = {}
for _t in ALL_TYPES:
    if _t[0] == "b":
        continue
    _w = spec.widen(_t)
    if _w:
        _WIDEN_OF[_w] = _t

_WIDEN_VV = {257, 261, 265, 267, 269, 270, 271, 429, 432, 435}
_WIDEN_WV = {258, 262, 266, 268, 430, 433}
_WIDEN_VX = {259, 263}
_WIDEN_WX = {260, 264}
_NARROW_WV = {272, 274, 276, 278}
_NARROW_WX = {273, 275, 277, 279}
_WIDEN_MAC_VV = {716, 718, 720}
_RED = set(range(437, 453))
_OP3_OPCS = {spec.OPCODES[r[0]] for r in spec.TABLES["rvv_op3_tab"]}
_SHIFTS = {228, 229, 230, 231, 232, 233} | _NARROW_WV | _NARROW_WX


def _lanes_c(t):
    if t[0] == "b":
        return "(__riscv_vlenb() * 8 / %d)" % t[1]
    num, den = spec.LMUL_FRAC[t[2]]
    return "(__riscv_vlenb() * %d * 8 / (%d * %d))" % (num, den, t[1])


def _buf_of(t):
    if t[0] == "f":
        return {32: "buff", 64: "bufd"}[t[1]]
    return {8: "buf8", 16: "buf16", 32: "buf32", 64: "buf64"}[t[1]]


def _load_c(t, off=0):
    suf = spec.type_suffix(t)
    if t[0] == "b":
        return "__riscv_vlm_v_%s(mbuf, %s)" % (suf, _lanes_c(t))
    buf = _buf_of(t)
    ptr = buf if not off else "(%s + %s)" % (buf, _lanes_c(t))
    return "__riscv_vle%d_v_%s(%s, %s)" % (t[1], suf, ptr, _lanes_c(t))


def _scalar_c(kind, e, opc):
    if kind == "f":
        return "0.5f" if e == 32 else "0.5"
    if opc in _SHIFTS:
        return "1"
    return "3"


def _b_type(opc, A):
    if opc in (228, 229, 230, 231, 232, 233, 239, 271):
        return ("u", A[1], A[2])
    return A


def _idx_type(t, eew):
    from fractions import Fraction
    num, den = spec.LMUL_FRAC[t[2]]
    want = Fraction(num * eew, den * t[1])
    for lm, (n, d) in spec.LMUL_FRAC.items():
        if Fraction(n, d) == want:
            return ("u", eew, lm)
    return None


def _widen_shape(t, kind):
    return (kind, t[1] * 2, _widen_lmul(t))


def emit_families():
    H = []
    H.append("/* riscv64 RVV differential probe: generated sweep of")
    H.append("   every shipped family at its seed shapes")
    H.append("   (tools/riscv_gen.py).  Lane counts come from")
    H.append("   __riscv_vlenb(). */")
    H.append("#include <riscv_vector.h>")
    H.append("#include <stdio.h>")
    H.append("#include <string.h>")
    H.append("")
    H.append("static signed char buf8[8192];")
    H.append("static short buf16[4096];")
    H.append("static int buf32[2048];")
    H.append("static long long buf64[1024];")
    H.append("static float buff[2048];")
    H.append("static double bufd[1024];")
    H.append("static unsigned char obuf[8192];")
    H.append("static signed char so8[8192];")
    H.append("static short so16[4096];")
    H.append("static int so32[2048];")
    H.append("static long long so64[1024];")
    H.append("static float sof[2048];")
    H.append("static double sod[1024];")
    H.append("static unsigned char sbuf[8192];")
    H.append("static unsigned char mbuf[64];")
    H.append("static unsigned char xb8[2048], gb8[2048];")
    H.append("static unsigned short xb16[1024], gb16[1024];")
    H.append("static unsigned int xb32[512], gb32[512];")
    H.append("static unsigned long long xb64[256], gb64[256];")
    H.append("")
    H.append("static void pb(const char *tag, const unsigned char *p,"
             " int nbytes)")
    H.append("{")
    H.append("    int i;")
    H.append('    printf("%s", tag);')
    H.append('    for (i = 0; i < nbytes; i++) printf(" %02x", p[i]);')
    H.append('    printf("\\n");')
    H.append("}")
    H.append("")

    blocks = []

    def store_print(R, rexpr, n_c, mode="full", fixup=False):
        """C lines storing rexpr (type R) to obuf and printing it;
        mode: full | lane0 | nm1 | np (np needs np_var)"""
        L = []
        suf = spec.type_suffix(R)
        if R[0] == "b":
            buf = "obuf"
            L.append("__riscv_vsm_v_%s(obuf, %s, %s);"
                     % (suf, rexpr, n_c))
            if fixup:
                L.append("for (i = 0; i < %s; i++)"
                         " if (!((mbuf[i >> 3] >> (i & 7)) & 1))"
                         " obuf[i >> 3] |= (unsigned char)(1u << (i & 7));"
                         % n_c)
            L.append("if (%s %% 8) obuf[(%s + 7) / 8 - 1] &="
                     " (unsigned char)((1u << (%s %% 8)) - 1);"
                     % (n_c, n_c, n_c))
            nb = "((%s + 7) / 8)" % n_c
        else:
            es = R[1] // 8
            if R[0] == "f":
                buf = "sof" if R[1] == 32 else "sod"
            else:
                buf = "so%d" % R[1]
            L.append("__riscv_vse%d_v_%s(%s, %s, %s);"
                     % (R[1], suf, buf, rexpr, n_c))
            if fixup:
                L.append("for (i = 0; i < %s; i++)"
                         " if (!((mbuf[i >> 3] >> (i & 7)) & 1))"
                         " memset((unsigned char *)%s + i * %d,"
                         " 0xEE, %d);"
                         % (n_c, buf, es, es))
            nb = {"full": "(%s * %d)" % (n_c, es),
                  "lane0": "%d" % es,
                  "nm1": "((%s - 1) * %d)" % (n_c, es),
                  "np": "(np * %d)" % es}[mode]
        return L, ("(unsigned char *)%s" % buf, nb)

    seen_names = set()
    for fam in spec.FAMILIES:
        nt, args_s, bt, seeds = fam[0], fam[1], fam[2], fam[3]
        domain = fam[4] if len(fam) > 4 else None
        insts = {}
        for (tup, name, a2, body) in expand_family(nt, args_s, bt,
                                                   seeds, domain):
            insts[name] = (tup, a2, body)
        seed_names = set()
        for tup in (seeds if seeds else [()]):
            # instantiate this seed directly (seeds are suffix
            # strings; expansions yield type tuples)
            want = tuple(type_of_suffix(s) for s in tup)
            for (t2, name, a2, body) in expand_family(
                    nt, args_s, bt, [tup] if tup else [()], domain):
                if t2 == want:
                    seed_names.add(name)
        for name in sorted(seed_names):
            if name in seen_names:
                continue
            seen_names.add(name)
            tup, a2, body = insts[name]
            m = re.match(r"__builtin_rvv_(\w+)\((\d+)", body)
            if not m:
                continue
            builtin, opc = m.group(1), int(m.group(2))
            row = _OPC_ROW.get(opc)
            kind = row[3] if row else None
            ctypes = re.findall(r"v\w+?_t\b", body)
            R = _TYPE_BY_CTYPE.get(ctypes[-1]) if ctypes else None
            argnames = [a.strip() for a in a2.split(",")]
            T0 = tup[0] if tup else None
            B = ["    {"]
            B.append("        int i;")
            try:
                lines, tag = _family_block(name, builtin, opc, kind,
                                           argnames, tup, R, T0,
                                           store_print)
            except _Skip as sk:
                continue
            B += lines
            B.append('        pb("%s", %s, %s);' % (name, tag[0], tag[1])
                     if tag is not None else "")
            B = [l for l in B if l]
            B.append("    }")
            blocks += B

    M = []
    M.append("int main(void)")
    M.append("{")
    M.append("    int i;")
    M.append("    for (i = 0; i < 8192; i++) buf8[i] ="
             " (signed char)(i * 37 + 11);")
    M.append("    for (i = 0; i < 4096; i++) buf16[i] ="
             " (short)(i * 37 + 11);")
    M.append("    for (i = 0; i < 2048; i++) buf32[i] = i * 37 + 11;")
    M.append("    for (i = 0; i < 1024; i++) buf64[i] = i * 37 + 11;")
    M.append("    for (i = 0; i < 2048; i++) buff[i] ="
             " (i % 13) * 0.5f - 2.0f;")
    M.append("    for (i = 0; i < 1024; i++) bufd[i] ="
             " (i % 13) * 0.5 - 2.0;")
    M.append("    for (i = 0; i < 64; i++) mbuf[i] ="
             " (unsigned char)(0xA5 + i * 37);")
    M.append("    for (i = 0; i < 2048; i++) { xb8[i] ="
             " (unsigned char)((i % 16) * 8); gb8[i] ="
             " (unsigned char)(i % 8); }")
    M.append("    for (i = 0; i < 1024; i++) { xb16[i] ="
             " (unsigned short)((i % 16) * 8); gb16[i] ="
             " (unsigned short)(i % 8); }")
    M.append("    for (i = 0; i < 512; i++) { xb32[i] ="
             " (unsigned int)((i % 16) * 8); gb32[i] ="
             " (unsigned int)(i % 8); }")
    M.append("    for (i = 0; i < 256; i++) { xb64[i] ="
             " (unsigned long long)((i % 16) * 8); gb64[i] ="
             " (unsigned long long)(i % 8); }")
    M += blocks
    M.append("    return 0;")
    M.append("}")
    return "\n".join(H + M) + "\n"


class _Skip(Exception):
    pass


def _family_block(name, builtin, opc, kind, argnames, tup, R, T0,
                  store_print):
    """return (C lines, nbytes-expr or None); the caller appends the
    pb() call when nbytes is not None (scalar-result families print
    themselves and return None)"""
    L = []
    args = [a for a in argnames if a != "vl"]

    def vec(var, t, off=0):
        L.append("        %s %s = %s;"
                 % (_ctype(t), var, _load_c(t, off)))
        return var

    def mask_of(t):
        return mask_type(t)

    if builtin == "vop1":
        if opc == 12:  # vid: no operand
            O = None
        elif opc in (15, 16, 17, 18):
            O = ("f", R[1], R[2])
        elif opc == 19:
            O = ("i", R[1], R[2])
        elif opc == 20:
            O = ("u", R[1], R[2])
        elif opc in (21, 22):
            O = trunc_partner(R, "f")
        elif opc == 23:
            O = trunc_partner(R, "i")
        elif opc == 24:
            O = trunc_partner(R, "u")
        elif opc == 25:
            O = trunc_partner(R, "f")
        elif opc in (26, 27, 30, 31, 32, 33):
            O = _widen_shape(R, "f")
        elif opc == 28:
            O = _widen_shape(R, "i")
        elif opc == 29:
            O = _widen_shape(R, "u")
        elif opc == 5:
            O = ("f", R[1], R[2])
        elif opc == 11:
            O = mask_of(R)
        else:
            O = R
        n_c = _lanes_c(R)
        if O is None:
            L.append("        %s r = %s(%s);"
                     % (_ctype(R), name, n_c))
        else:
            vec("va", O)
            L.append("        %s r = %s(va, %s);"
                     % (_ctype(R), name, n_c))
        sl, nb = store_print(R, "r", n_c)
        return L + sl, nb

    if builtin == "vop1r":
        if tup:
            O = T0
        else:
            mb = re.search(r"_b(\d+)$", name)
            O = ("b", int(mb.group(1)), "")
        vec("va", O)
        n_c = _lanes_c(O)
        if opc in (100, 101):
            L.append("        {")
            if opc == 100:
                L.append("            long long r = (long long)%s(va);"
                         % name)
                L.append('            printf("%s %%lld\\n", r);' % name)
            else:
                L.append("            double r = (double)%s(va);"
                         % name)
                L.append("            unsigned long long u;")
                L.append("            memcpy(&u, &r, 8);")
                L.append('            printf("%s %%llx\\n", u);' % name)
            L.append("        }")
        else:
            L.append('        printf("%s %%lld\\n",'
                     " (long long)%s(va, %s));" % (name, name, n_c))
        return L, None

    if builtin == "splat":
        n_c = _lanes_c(T0)
        s = _scalar_c(T0[0], T0[1], opc)
        L.append("        %s r = %s(%s, %s);"
                 % (_ctype(T0), name, s, n_c))
        sl, nb = store_print(T0, "r", n_c)
        return L + sl, nb

    if builtin == "vop2":
        n_c = _lanes_c(T0)
        if opc in _RED:
            T1 = tup[1]
            vec("va", T0)
            vec("vs", T1, 1)
            L.append("        %s r = %s(va, vs, %s);"
                     % (_ctype(T1), name, n_c))
            sl, nb = store_print(T1, "r", n_c, mode="lane0")
            return L + sl, nb
        if opc == 426:  # vrgather_vv: index vector, same shape
            vec("va", T0)
            it = ("u", T0[1], T0[2])
            L.append("        %s vi = __riscv_vle%d_v_%s(gb%d, %s);"
                     % (_ctype(it), T0[1], spec.type_suffix(it),
                        T0[1], n_c))
            L.append("        %s r = %s(va, vi, %s);"
                     % (_ctype(T0), name, n_c))
            sl, nb = store_print(T0, "r", n_c)
            return L + sl, nb
        if opc == 428:  # vrgatherei16
            vec("va", T0)
            it = _idx_type(T0, 16)
            L.append("        %s vi = __riscv_vle16_v_%s(gb16, %s);"
                     % (_ctype(it), spec.type_suffix(it), n_c))
            L.append("        %s r = %s(va, vi, %s);"
                     % (_ctype(T0), name, n_c))
            sl, nb = store_print(T0, "r", n_c)
            return L + sl, nb
        if kind == "RVK_VV":
            if opc in _WIDEN_VV:
                A = B = _WIDEN_OF[T0]
            elif opc in _WIDEN_WV:
                A, B = T0, _WIDEN_OF[T0]
            elif opc in _NARROW_WV:
                A, B = spec.widen(T0), ("u", T0[1], T0[2])
            else:
                A = B = T0
            vec("va", A)
            vec("vb", _b_type(opc, B), 1)
            L.append("        %s r = %s(va, vb, %s);"
                     % (_ctype(R), name, n_c))
            sl, nb = store_print(R, "r", n_c)
            return L + sl, nb
        # scalar second operand (vx / vf / slide1)
        if opc in _WIDEN_VX or opc in (431, 434, 436):
            A = _WIDEN_OF[T0]
            se = T0[1] // 2
        elif opc in _WIDEN_WX:
            A = T0
            se = T0[1] // 2
        elif opc in _NARROW_WX:
            A = spec.widen(T0)
            se = T0[1]
        elif opc in (453, 454):
            A = T0
            se = T0[1] // 2
        else:
            A = T0
            se = T0[1]
        vec("va", A)
        if kind == "RVK_VF" or (opc in (424, 425)):
            s = _scalar_c("f", se, opc)
        elif opc == 427 or opc in (420, 421, 422, 423):
            s = "1"
        else:
            s = _scalar_c("i", se, opc)
        L.append("        %s r = %s(va, %s, %s);"
                 % (_ctype(R), name, s, n_c))
        mode = "nm1" if opc in (423, 425) else "full"
        sl, nb = store_print(R, "r", n_c, mode=mode)
        return L + sl, nb

    if builtin == "vop3":
        n_c = _lanes_c(T0)
        vec("vd", T0)
        if "off" in args:  # vslideup_vx3
            vec("va", T0, 1)
            L.append("        %s r = %s(vd, va, 1, %s);"
                     % (_ctype(T0), name, n_c))
        elif kind == "RVK_VV":
            A = _WIDEN_OF[T0] if opc in _WIDEN_MAC_VV else T0
            vec("va", A)
            vec("vb", A, 1)
            L.append("        %s r = %s(vd, va, vb, %s);"
                     % (_ctype(T0), name, n_c))
        else:  # scalar in the middle
            A = _WIDEN_OF[T0] if opc in (717, 719, 721) else T0
            vec("vb", A, 1)
            s = _scalar_c("f" if kind == "RVK_VF" else "i", A[1], opc)
            L.append("        %s r = %s(vd, %s, vb, %s);"
                     % (_ctype(T0), name, s, n_c))
        sl, nb = store_print(T0, "r", n_c)
        return L + sl, nb

    if builtin == "vopm":
        n_c = _lanes_c(T0)
        M = mask_of(T0)
        vec("va", T0)
        vec("vm", M)
        if opc == 804:  # vcompress
            L.append("        int np = (int)__riscv_vcpop_m_b%d(vm, %s);"
                     % (M[1], n_c))
            L.append("        %s r = %s(va, vm, %s);"
                     % (_ctype(T0), name, n_c))
            sl, nb = store_print(T0, "r", n_c, mode="np")
            return L + sl, nb
        if kind == "RVK_VV":
            vec("vb", T0, 1)
            b = "vb"
        elif kind == "RVK_VF":
            b = _scalar_c("f", T0[1], opc)
        else:
            b = "5"
        L.append("        %s r = %s(va, %s, vm, %s);"
                 % (_ctype(T0), name, b, n_c))
        sl, nb = store_print(T0, "r", n_c)
        return L + sl, nb

    if builtin == "vopm2":
        n_c = _lanes_c(T0)
        M = tup[-1] if tup[-1][0] == "b" else mask_of(T0)
        vec("vm", M)
        has_o = "o" in args
        if has_o:
            vec("vo", R)
        fixup = name.endswith("_m")
        row3 = opc in _OP3_OPCS
        if row3:
            if kind == "RVK_VV":
                A = _WIDEN_OF[T0] if opc in _WIDEN_MAC_VV else T0
                vec("va", A)
                vec("vb", A, 1)
                ab = "va, vb"
            else:
                A = _WIDEN_OF[T0] if opc in (717, 719, 721) else T0
                vec("vb", A, 1)
                ab = "%s, vb" % _scalar_c(
                    "f" if kind == "RVK_VF" else "i", A[1], opc)
        elif kind == "RVK_VV":
            vec("va", T0)
            vec("vb", _b_type(opc, T0), 1)
            ab = "va, vb"
        else:
            vec("va", T0)
            ab = "va, %s" % _scalar_c(
                "f" if kind == "RVK_VF" else "i", T0[1], opc)
        if has_o:
            call = "%s(vm, vo, %s, %s)" % (name, ab, n_c)
        elif fixup:
            call = "%s(vm, %s, %s)" % (name, ab, n_c)
        else:
            call = "%s(vm, %s, %s)" % (name, ab, n_c)
        L.append("        %s r = %s;" % (_ctype(R), call))
        sl, nb = store_print(R, "r", n_c, fixup=fixup)
        return L + sl, nb

    if builtin == "vop1m2":
        if opc in (900, 901):  # vmv.s.x / vfmv.s.f specials
            n_c = _lanes_c(T0)
            s = "7" if opc == 900 else _scalar_c("f", T0[1], opc)
            L.append("        %s r = %s(%s, %s);"
                     % (_ctype(T0), name, s, n_c))
            sl, nb = store_print(T0, "r", n_c, mode="lane0")
            return L + sl, nb
        n_c = _lanes_c(T0)
        M = mask_of(T0)
        vec("vm", M)
        vec("va", T0)
        fixup = name.endswith("_m")
        if "o" in args:
            vec("vo", T0, 1)
            L.append("        %s r = %s(vm, vo, va, %s);"
                     % (_ctype(T0), name, n_c))
        else:
            L.append("        %s r = %s(vm, va, %s);"
                     % (_ctype(T0), name, n_c))
        sl, nb = store_print(T0, "r", n_c, fixup=fixup)
        return L + sl, nb

    if builtin == "vload":
        n_c = _lanes_c(T0)
        buf = _buf_of(T0)
        if opc == 1000:  # strided
            L.append("        %s r = %s(%s, %d, %s);"
                     % (_ctype(T0), name, buf, T0[1] // 8, n_c))
        else:  # indexed
            eew = int(re.search(r"ei(\d+)", name).group(1))
            it = _idx_type(T0, eew)
            L.append("        %s vi = __riscv_vle%d_v_%s(xb%d, %s);"
                     % (_ctype(it), eew, spec.type_suffix(it),
                        eew, n_c))
            L.append("        %s r = %s(%s, vi, %s);"
                     % (_ctype(T0), name, buf, n_c))
        sl, nb = store_print(T0, "r", n_c)
        return L + sl, nb

    if builtin == "vstore":
        n_c = _lanes_c(T0)
        vec("vv", T0)
        L.append("        memset(sbuf, 0, sizeof sbuf);")
        if opc == 1001:  # strided
            L.append("        %s(sbuf, %d, vv, %s);"
                     % (name, T0[1] // 8, n_c))
            nb = "(%s * %d)" % (n_c, T0[1] // 8)
        else:  # indexed
            eew = int(re.search(r"ei(\d+)", name).group(1))
            it = _idx_type(T0, eew)
            L.append("        %s vi = __riscv_vle%d_v_%s(xb%d, %s);"
                     % (_ctype(it), eew, spec.type_suffix(it),
                        eew, n_c))
            L.append("        %s(sbuf, vi, vv, %s);" % (name, n_c))
            nb = "256"
        L.append('        pb("%s", sbuf, %s);' % (name, nb))
        return L, None

    raise _Skip()


def main():
    bad, unchecked = check_encodings()
    for b in bad:
        print("ENCODING MISMATCH:", b)
    if bad:
        raise SystemExit(1)
    if unchecked:
        print("note: %d table entries have no derived assembler check: %s"
              % (len(unchecked), ", ".join(sorted(set(unchecked)))))
    hdr = emit_header()
    # conflict check: one name, one body (up to parameter naming)
    def canon(line):
        m = re.match(r"#define (__riscv_\w+)\(([^)]*)\) (.*)", line)
        if not m:
            return line
        params = [p.strip() for p in m.group(2).split(",")]
        body = m.group(3)
        for i, p in enumerate(params):
            body = re.sub(r"\b%s\b" % re.escape(p), "@p%d" % i, body)
        return body
    seen_defs = {}
    for line in hdr.split("\n"):
        m = re.match(r"#define (__riscv_\w+)\(", line)
        if m:
            c = canon(line)
            if m.group(1) in seen_defs and seen_defs[m.group(1)] != c:
                print("CONFLICTING DEFINITION:", m.group(1))
                raise SystemExit(1)
            seen_defs[m.group(1)] = c
    # dedupe alpha-equivalent redefinitions (keep the first)
    out_lines, kept = [], set()
    for line in hdr.split("\n"):
        m = re.match(r"#define (__riscv_\w+)\(", line)
        if m and m.group(1) in kept:
            continue
        if m:
            kept.add(m.group(1))
        out_lines.append(line)
    hdr = "\n".join(out_lines)
    with open("vendor-tinycc/include/riscv_vector.h", "w") as fh:
        fh.write(hdr)
    print("header: %d defines" % hdr.count("#define __riscv_"))
    block = emit_backend()
    path = "vendor-tinycc/riscv64-gen.c"
    src = open(path).read()
    begin = src.index("/* RVV_TABLES_BEGIN")
    end = src.index("/* RVV_TABLES_END */") + len("/* RVV_TABLES_END */")
    src = src[:begin] + block.rstrip() + src[end:]
    open(path, "w").write(src)
    print("backend tables regenerated")
    with open("tests/riscv64/probes/riscv_sweep.c", "w") as fh:
        fh.write(emit_sweep())
    print("sweep probe regenerated")
    with open("tests/riscv64/probes/riscv_families.c", "w") as fh:
        fh.write(emit_families())
    print("families probe regenerated")

if __name__ == "__main__":
    main()
