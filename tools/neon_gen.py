#!/usr/bin/env python3
"""Generate the aarch64 NEON builtin surface from one operation spec.

Outputs:
  - the opcode table block inside vendor-tinycc/arm64-gen.c
    (between the NEON_TABLES_BEGIN/END markers),
  - vendor-tinycc/include/arm_neon.h (typedefs + intrinsic macros),
  - differential probes in tests/neon/probes/ (one C file per area).

Encodings are not written by hand: for every (operation, lane
shape) pair the generator assembles the instruction with the cross
assembler and extracts the base encoding, so a pair the ISA does
not define simply fails to assemble and is excluded from the
tables, the header and the probes (and listed on stdout).
"""
import re
import subprocess
import sys

AS = "aarch64-linux-gnu-as"
OBJDUMP = "aarch64-linux-gnu-objdump"
MARCH_AS = "armv8.2-a+dotprod+crypto+fp16"

# ------------------------------------------------------------------
# lane shapes: (es, n, fl) -> shape id, matching A64SH_* in
# arm64-gen.c.  Element kinds: s (signed), u (unsigned), f (float),
# p (poly, an unsigned alias).
# ------------------------------------------------------------------
def shape_id(es, n, fl):
    if fl:
        if es == 4:
            return 8 if n == 2 else 9
        return 10 if n == 1 else 11
    if es == 1:
        return 0 if n == 8 else 1
    if es == 2:
        return 2 if n == 4 else 3
    if es == 4:
        return 4 if n == 2 else 5
    return 6 if n == 1 else 7

SHAPE_NAMES = ["I8X8", "I8X16", "I16X4", "I16X8", "I32X2", "I32X4",
               "I64X1", "I64X2", "F32X2", "F32X4", "F64X1", "F64X2"]

ELEM_C = {
    ("s", 1): "signed char", ("s", 2): "short", ("s", 4): "int",
    ("s", 8): "long long",
    ("u", 1): "unsigned char", ("u", 2): "unsigned short",
    ("u", 4): "unsigned int", ("u", 8): "unsigned long long",
    ("p", 1): "unsigned char", ("p", 2): "unsigned short",
    ("p", 8): "unsigned long long",
    ("f", 4): "float", ("f", 8): "double",
}
KIND_PREFIX = {"s": "int", "u": "uint", "f": "float", "p": "poly"}

def vec_type(kind, es, n):
    return "%s%dx%d_t" % (KIND_PREFIX[kind], es * 8, n)

def arr_of(es, total):
    return {1: "8b" if total == 8 else "16b",
            2: "4h" if total == 8 else "8h",
            4: "2s" if total == 8 else "4s",
            8: "1d" if total == 8 else "2d"}[es]

def suffix(kind, es, total):
    return "%s_%s%d" % ("q" if total == 16 else "", kind, es * 8)

# ------------------------------------------------------------------
# assembler driver: assemble one instruction, return its word.
# ------------------------------------------------------------------
_asm_cache = {}

def assemble_one_slow(text):
    """Assemble one instruction; return its word or None."""
    with open("/tmp/neon_gen.s", "w") as f:
        f.write("\t.text\n\t" + text + "\n")
    r = subprocess.run(["aarch64-linux-gnu-as",
                        "-march=armv8.2-a+dotprod+crypto+fp16",
                        "-o", "/tmp/neon_gen.o", "/tmp/neon_gen.s"],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return None
    out = subprocess.run(["aarch64-linux-gnu-objdump", "-d",
                          "/tmp/neon_gen.o"],
                         capture_output=True, text=True).stdout
    m = re.search(r":\s+([0-9a-f]{8})\s", out)
    return int(m.group(1), 16) if m else None

def assemble_one(text):
    return _asm_cache.get(text)

def assemble_group(texts):
    """Assemble a list of instruction texts in one file; fall
    back to one-by-one assembly when any line is rejected."""
    if not texts:
        return []
    with open("/tmp/neon_gen.s", "w") as f:
        f.write("\t.text\n")
        for t in texts:
            f.write("\t" + t + "\n")
    r = subprocess.run(["aarch64-linux-gnu-as",
                        "-march=armv8.2-a+dotprod+crypto+fp16",
                        "-o", "/tmp/neon_gen.o", "/tmp/neon_gen.s"],
                       capture_output=True, text=True)
    if r.returncode == 0:
        out = subprocess.run(["aarch64-linux-gnu-objdump", "-d",
                              "/tmp/neon_gen.o"],
                             capture_output=True, text=True).stdout
        words = [int(m, 16)
                 for m in re.findall(r":\s+([0-9a-f]{8})\s", out)]
        if len(words) == len(texts):
            return words
    return [assemble_one_slow(t) for t in texts]

def sample_texts(tpl, q, extra, lo, hi):
    """All rendered sample texts derive_enc will ask for."""
    texts = [render(tpl, (0, 0, 0), q, lo, extra),
             render(tpl, (3, 5, 7), q, lo, extra)]
    if "{i}" in tpl:
        cand = {lo, hi}
        for v in (lo + 1, lo + 2, lo + 3, lo + 4,
                  1, 2, 4, 8, 16, 32, 63):
            if lo <= v <= hi:
                cand.add(v)
        for v in sorted(cand):
            texts.append(render(tpl, (0, 0, 0), q, v, extra))
    return texts

def render(tpl, regs, q, imm, extra):
    t = tpl
    for k, v in sorted(extra.items(), key=lambda kv: -len(kv[0])):
        t = t.replace(k, v)
    t = t.replace("{D}", "v%d" % regs[0])
    t = t.replace("{Dd}", "d%d" % regs[0])
    t = t.replace("{Sd}", "s%d" % regs[0])
    t = t.replace("{N}", "v%d" % regs[1])
    t = t.replace("{Nd}", "d%d" % regs[1])
    t = t.replace("{Sn}", "s%d" % regs[1])
    t = t.replace("{M}", "v%d" % regs[2])
    t = t.replace("{Md}", "d%d" % regs[2])
    t = t.replace("{Sm}", "s%d" % regs[2])
    t = t.replace("{Xd}", "x%d" % regs[0])
    t = t.replace("{Xn}", "x%d" % regs[1])
    t = t.replace("{Xm}", "x%d" % regs[2])
    t = t.replace("{i}", str(imm))
    return t

def derive_enc(tpl, q, extra, lo=0, hi=0):
    """Derive an encoding template from assembler samples.

    Returns (enc, imm_pos, imm_base, imm_sign, imm_spread) or
    None.  enc has the register fields and the immediate field
    cleared.  The immediate model is either affine (the field is
    imm_base + imm_sign*imm at imm_pos) or a bit spread
    (imm_sign == 2: bit i of imm goes to the position packed in
    imm_spread, 5 bits per position, biased by 1)."""
    def word(imm, regs=(0, 0, 0)):
        return assemble_one(render(tpl, regs, q, imm, extra))
    w000 = word(lo)
    w375 = word(lo, (3, 5, 7))
    if w000 is None or w375 is None:
        return None
    regmask = w000 ^ w375
    base0 = w000 & ~regmask & 0xFFFFFFFF
    if "{i}" not in tpl:
        return (base0, 0, 0, 0, 0)
    cand = {lo, hi}
    for v in (lo + 1, lo + 2, lo + 3, lo + 4, 1, 2, 4, 8, 16, 32, 63):
        if lo <= v <= hi:
            cand.add(v)
    samples = {}
    for v in sorted(cand):
        w = word(v)
        if w is None:
            return None
        samples[v] = w & ~regmask & 0xFFFFFFFF
    varying = 0
    for w in samples.values():
        varying |= w ^ samples[lo]
    enc0 = samples[lo] & ~varying & 0xFFFFFFFF
    if varying:
        pos = (varying & -varying).bit_length() - 1
        m = varying >> pos
        if m & (m + 1) == 0:  # contiguous field: try affine
            f = {v: (w >> pos) & m for v, w in samples.items()}
            d = f[lo + 1] - f[lo] if lo + 1 in f else 0
            if d in (1, -1) and all(f[v] == f[lo] + d * (v - lo)
                                    for v in f):
                return (enc0, pos, f[lo] - d * lo, d, 0)
        # bit-spread model (immediates starting at 0): each imm
        # bit lands on one position of its own
        if lo == 0:
            positions = []
            ok = True
            for i in range(7):
                v = 1 << i
                if v > hi:
                    positions.append(0)
                    continue
                delta = samples[v] ^ samples[lo]
                if delta == 0 or delta & (delta - 1):
                    ok = False
                    break
                positions.append(delta.bit_length() - 1)
            if ok:
                def spread(v):
                    b = 0
                    for i, pp in enumerate(positions):
                        if pp and (v >> i) & 1:
                            b |= 1 << pp
                    return b
                if all(samples[v] == enc0 | spread(v) for v in samples):
                    pack = 0
                    for i, pp in enumerate(positions):
                        if pp:
                            pack |= (pp + 1) << (5 * i)
                    return (enc0, 0, 0, 2, pack)
    return (enc0, 0, 0, 0, 0) if not varying else None

import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import neon_spec

FAILS = []   # (op, kind, es, total, template) that the assembler rejected
ROWS = {"op1": [], "op2": [], "op3": [], "op1r": [], "shuf": []}
MACROS = []  # (name, body) for the header, in emission order
PROBES = []  # dicts describing one differential case each

def res_kind_letter(entry, kind_letter):
    if "ressign" in entry:
        return entry["ressign"]
    return kind_letter

def result_kind_letter(entry, kind_letter):
    """The element kind letter of an operation's result: the
    float flag wins for float-producing conversions, then the
    declared result sign for int-producing conversions."""
    if entry.get("ofl"):
        return "f"
    if entry.get("restype_sign"):
        return entry["restype_sign"]
    return res_kind_letter(entry, kind_letter)

def shapes_for(entry, kind_letter, es, total):
    """Return (res, b, c) shape tuples (es, total) or None when the
    combination is not a real NEON shape.  The entry shape is
    operand a's (es, total); c is None for op1/op2."""
    fam = entry["fam"]
    kind = entry.get("kind", "SAME")
    if (es, total) in entry.get("bad", ()):
        return None
    n = total // es
    def ok(t):
        return t[0] in (1, 2, 4, 8) and t[1] in (8, 16)
    if kind == "SAME" or kind == "SHIFT":
        res = (es, total); b = (es, total); c = (es, total)
    elif kind == "BYTES":
        res = (es, total); b = (es, total); c = (es, total)
    elif kind == "CVTN":
        oes = entry.get("oes", es) or es
        res = (oes, n * oes); b = None; c = None
        if not ok(res):
            return None
    elif kind == "NARROW":
        if total != 16 or es < 2:
            return None
        res = (es // 2, 8); b = None; c = None
    elif kind == "NARROW_HI":
        if total != 8:
            return None
        res = (es, 16); b = (es * 2, 16)
        c = (es * 2, 16) if fam == "op3" else None
        if not ok(b):
            return None
    elif kind == "NARROW2":
        if total != 16 or es < 2:
            return None
        res = (es // 2, 8); b = (es, total); c = None
    elif kind == "NARROW2_HI":
        if total != 8:
            return None
        res = (es, 16); b = (es * 2, 16); c = (es * 2, 16)
        if not ok(b):
            return None
    elif kind == "WIDEN_LO":
        if total != 8:
            return None
        res = (es * 2, 16); b = (es, total); c = (es, total)
        if not ok(res):
            return None
    elif kind == "WIDEN_HI":
        if total != 16:
            return None
        res = (es * 2, 16); b = (es, total); c = (es, total)
        if not ok(res):
            return None
    elif kind == "ACCW_LO":
        if total != 16:
            return None
        res = (es, total); b = (es // 2, 8); c = (es // 2, 8)
        if es < 2:
            return None
    elif kind == "ACCW_HI":
        if total != 16:
            return None
        res = (es, total); b = (es // 2, 16); c = (es // 2, 16)
        if es < 2:
            return None
    elif kind == "PAIRW":
        if es > 4:
            return None
        res = (es * 2, total); b = (es, total); c = (es, total)
    elif kind == "TBLQ":
        res = (es, total)
        b = (es, 16) if fam == "op3" else (es, total)
        c = (es, total) if fam == "op3" else None
    elif kind == "ADDW_LO":
        if total != 16 or es < 2:
            return None
        res = (es, total); b = (es // 2, 8); c = None
    elif kind == "ADDW_HI":
        if total != 16 or es < 2:
            return None
        res = (es, total); b = (es // 2, 16); c = None
    elif kind == "ACCW1":
        res = (es, total); b = (es // 2, total); c = None
        if es < 2:
            return None
    elif kind == "DOT":
        res = (es, total); b = (entry.get("oes", 1), total)
        c = (entry.get("oes", 1), total)
    elif kind == "JOIN":
        if total != 8:
            return None
        res = (es, 16); b = (es, total); c = None
    elif kind in ("HALF_LO", "HALF_HI"):
        if total != 16:
            return None
        res = (es, 8); b = None; c = None
    elif kind == "SCALAR":
        res = (entry.get("oes", es) or es, 0); b = None; c = None
    else:
        raise AssertionError(kind)
    return res, b, c

def arr_of_shape(shape):
    es, total = shape
    return arr_of(es, total)

def iter_combos():
    """Yield (entry, kind, es, total, shapes, extra, ilo, ihi,
    templates) for every spec entry and shape."""
    for entry in neon_spec.OPS:
        totals = entry.get("totals", (8, 16))
        for (kl, es) in neon_spec.parse_types(entry["types"]):
            fl = 1 if kl == "f" else 0
            for total in totals:
                sh = shapes_for(entry, kl, es, total)
                if sh is None:
                    continue
                res, bsh, csh = sh
                templates = []
                if entry["asm"]:
                    templates = entry.get("asmf", entry["asm"]) \
                        if fl else entry["asm"]
                extra = {
                    "{arr}": arr_of(es, total),
                    "{arrR}": arr_of_shape(res) if res[1] else "",
                    "{arrB}": arr_of_shape(bsh) if bsh else "",
                    "{Ed}": {1: "b", 2: "h", 4: "s", 8: "d"}[res[0]]
                            + "0",
                    "{arr8}": arr_of(1, total),
                    "{arrI}": (("%d%s" % (es // entry.get("oes", 1),
                                          {1: "b", 2: "h", 4: "s",
                                           8: "d"}[entry.get("oes", 1)]))
                               if entry.get("kind") == "DOT"
                               else arr_of(csh[0], 16)
                               if entry["fam"] == "op3" and csh
                               else arr_of(bsh[0], 16) if bsh else ""),
                }
                ilo = entry["imm"][0] if "imm" in entry else 0
                ihi = ilo
                if entry.get("form") == "3REG_IDX":
                    # derive over the widest lane source: the index
                    # field layout does not depend on source width
                    if entry["fam"] == "op3":
                        ihi = (16 // 4 - 1 if entry.get("kind") == "DOT"
                               else 16 // csh[0] - 1)
                    else:
                        ihi = 16 // bsh[0] - 1
                if "imm" in entry:
                    h = entry["imm"][1]
                    ihi = (8 * es if h == "es8" else
                           8 * es - 1 if h == "es8m1" else
                           4 * es if h == "es4" else
                           total - 1 if h == "totm1" else h)
                yield (entry, kl, es, total, sh, extra, ilo, ihi,
                       templates)

def prime_samples():
    jobs = {}
    for (entry, kl, es, total, sh, extra, ilo, ihi,
         templates) in iter_combos():
        for ti, tpl in enumerate(templates):
            texts = sample_texts(tpl, total == 16, extra, ilo, ihi)
            jobs.setdefault((id(entry), ti), []).extend(texts)
    for (entry, kl, es, total, sh, extra, ilo, ihi,
         templates) in iter_combos():
        if entry.get("asm64") and es == 8 and total == 8:
            for ti, tpl in enumerate(entry["asm64"]):
                texts = sample_texts(tpl, False, extra, ilo, ihi)
                jobs.setdefault((id(entry), "x%d" % ti),
                                []).extend(texts)
    for texts in jobs.values():
        words = assemble_group(texts)
        for t, w in zip(texts, words):
            _asm_cache[t] = w

def build():
    prime_samples()
    op_index = {}
    for (entry, kl, es, total, sh, extra, ilo, ihi,
         templates) in iter_combos():
        fam = entry["fam"]
        res, bsh, csh = sh
        fl = 1 if kl == "f" else 0
        enc = None
        ipos = ibase = isign = ispread = 0
        row_form = None
        if entry.get("asm64") and es == 8 and total == 8:
            templates = entry["asm64"]
            row_form = entry.get("form64", entry.get("form"))
        if entry["asm"]:
            got = None
            for tpl in templates:
                got = derive_enc(tpl, total == 16, extra, ilo, ihi)
                if got is not None:
                    break
            if got is None:
                FAILS.append((entry["op"], kl, es, total,
                              templates[0]))
                continue
            enc, ipos, ibase, isign, ispread = got
        elif entry.get("form") != "JOIN":
            enc = 0  # header-only entry (comparisons)
        if fam not in op_index:
            op_index[fam] = {}
        if entry["op"] not in op_index[fam]:
            op_index[fam][entry["op"]] = len(op_index[fam])
        row = dict(entry=entry, kl=kl, es=es, total=total,
                   fl=fl, res=res, bsh=bsh, csh=csh, enc=enc,
                   form=row_form,
                   imm_pos=ipos, imm_base=ibase,
                   imm_sign=isign, imm_spread=ispread,
                   opidx=op_index[fam][entry["op"]])
        if entry["asm"] or entry.get("form") == "JOIN":
            ROWS[fam].append(row)
        emit_macro(row)
        emit_probe(row)
    return op_index

def suffix_of(kl, es, total):
    return ("q" if total == 16 else "") + "_" + kl + str(es * 8)

def name_for(row):
    entry = row["entry"]
    stem = entry["stem"]
    if stem is None:
        return None
    naming = entry.get("naming", "std")
    kl, es, total = row["kl"], row["es"], row["total"]
    res = row["res"]
    rkl = result_kind_letter(entry, kl)
    if naming == "std":
        if stem.endswith("_n"):
            return (stem[:-2] + ("q" if total == 16 else "")
                    + "_n_" + kl + str(es * 8))
        return stem + suffix_of(kl, es, total)
    if naming == "src":
        return stem + "_" + kl + str(es * 8)
    if naming == "srcb":
        bkl = entry.get("bkind", kl)
        return stem + "_" + bkl + str(row["bsh"][0] * 8)
    if naming == "srcbq":
        bkl = entry.get("bkind", kl)
        return (stem + ("q" if res[1] == 16 else "") + "_" + bkl
                + str(row["bsh"][0] * 8))
    if naming == "cvt":
        if entry["fam"] == "op1":
            skl, ses = kl, es
        else:
            skl = entry.get("bkind", kl)
            ses = row["bsh"][0]
        stot = total if entry["fam"] == "op1" else row["bsh"][1]
        q = "q" if res[1] == 16 and stot == 16 else ""
        if "_high" in stem:
            q = ""  # the _high convert names never carry q
        if stem.endswith("_n"):
            base = stem[:-2] + q + "_n"
        else:
            base = stem + q
        return (base + "_" + rkl + str(res[0] * 8)
                + "_" + skl + str(ses * 8))
    if naming == "join":
        return stem + "_" + rkl + str(res[0] * 8)
    raise AssertionError(naming)

def type_of(kl, es, total):
    if total == 0:
        return ELEM_C[(kl, es)]
    return vec_type(kl, es, total // es)

def opc_of(row):
    return (row["opidx"] << 11) | shape_id(row["es"], row["total"] // row["es"],
                                            row["fl"])

def imm_expr(row, arg):
    return "((%s) << 4)" % arg

def emit_macro(row):
    entry = row["entry"]
    if entry["stem"] is None:
        return
    style = entry.get("style", {"op1": "op1", "op2": "op2",
                                "op3": "op3", "op1r": "op1r",
                                "shuf": "op2"}[entry["fam"]])
    name = name_for(row)
    kl, es, total = row["kl"], row["es"], row["total"]
    res = row["res"]
    rkl = result_kind_letter(entry, kl)
    at = type_of(kl, es, total)
    rt = type_of(rkl, res[0], res[1])
    opc = opc_of(row)
    if entry.get("form") == "3REG_IDX":
        for st in [entry["stem"]] + entry.get("extra_stems", []):
            emit_idx_macro(row, st, rt)
        return
    stems = [entry["stem"]] + entry.get("extra_stems", [])
    names = []
    for st in stems:
        save = entry["stem"]
        entry["stem"] = st
        names.append(name_for(row))
        entry["stem"] = save
    for name in names:
        if style == "oper":
            body = "((a) %s (b))" % entry["oper"]
            MACROS.append((name, "#define %s(a, b) %s" % (name, body)))
        elif style == "cmp":
            crkl = "u" if kl == "u" else "s"
            crt = type_of(crkl, es, total)
            body = "((%s)((a) %s (b)))" % (crt, entry["oper"])
            MACROS.append((name, "#define %s(a, b) %s" % (name, body)))
        elif style == "custom":
            MACROS.append((name, "#define %s(a) (-(a))" % name))
        elif entry["fam"] == "op1" and style == "op1":
            MACROS.append((name,
                "#define %s(a) __builtin_aarch64_vop1(%d, a, %s)"
                % (name, opc, rt)))
        elif entry["fam"] == "op1" and style == "op1i":
            MACROS.append((name,
                "#define %s(a, n) __builtin_aarch64_vop1(%d + ((n) << 4), a, %s)"
                % (name, opc, rt)))
        elif entry["fam"] == "op1r":
            MACROS.append((name,
                "#define %s(a) __builtin_aarch64_vop1r(%d, a, %s)"
                % (name, opc, rt)))
        elif entry["fam"] == "op2" and style == "op2":
            bt = type_of(entry.get("bkind", kl),
                         row["bsh"][0], row["bsh"][1])
            MACROS.append((name,
                "#define %s(a, b) __builtin_aarch64_vop2(%d, a, b, %s)"
                % (name, opc, rt)))
        elif entry["fam"] == "op3" and style == "op3":
            MACROS.append((name,
                "#define %s(a, b, c) __builtin_aarch64_vop3(%d, a, b, c, %s)"
                % (name, opc, rt)))
        elif entry["fam"] == "op3" and style == "op3i":
            MACROS.append((name,
                "#define %s(a, b, n) __builtin_aarch64_vop3(%d + ((n) << 4), a, b, 0, %s)"
                % (name, opc, rt)))
        elif entry["fam"] == "op2" and style == "op2i2":
            MACROS.append((name,
                "#define %s(a, b, n) __builtin_aarch64_vop2(%d + ((n) << 4), a, b, %s)"
                % (name, opc, rt)))
        elif entry["fam"] == "shuf" and style == "op2":
            MACROS.append((name,
                "#define %s(a, b) __builtin_aarch64_shuffle(%d, a, b, 0, %s)"
                % (name, opc, rt)))
        elif entry["fam"] == "shuf" and style == "op2i":
            MACROS.append((name,
                "#define %s(a, b, n) __builtin_aarch64_shuffle(%d, a, b, ((n) * %d), %s)"
                % (name, opc, row["es"], rt)))
        else:
            raise AssertionError("style %s fam %s" % (style, entry["fam"]))

def idx_name(row, stem, src_total):
    """Lane-intrinsic name: the q before _lane marks a 16-byte
    result (same-shape ops only), the q after _lane marks a
    16-byte lane source."""
    entry = row["entry"]
    kind = entry.get("kind", "SAME")
    if kind in ("ACCW_LO", "ACCW_HI", "WIDEN_LO"):
        ekl, ees = entry.get("bkind", row["kl"]), row["bsh"][0]
    else:
        ekl, ees = row["kl"], row["es"]
    qpre = "q" if kind in ("SAME", "DOT") and row["total"] == 16 else ""
    qpost = "q" if src_total == 16 else ""
    return "%s%s_lane%s_%s%d" % (stem[:-5], qpre, qpost, ekl,
                                 ees * 8)

def emit_idx_macro(row, stem, rt):
    entry = row["entry"]
    opc = opc_of(row)
    if entry["fam"] == "op2":
        for src_total in (8, 16):
            nm = idx_name(row, stem, src_total)
            MACROS.append((nm,
                "#define %s(a, b, n) __builtin_aarch64_vop2(%d + ((n) << 4), a, b, %s)"
                % (nm, opc, rt)))
    else:
        for src_total in (8, 16):
            nm = idx_name(row, stem, src_total)
            MACROS.append((nm,
                "#define %s(a, b, c, n) __builtin_aarch64_vop3(%d + ((n) << 4), a, b, c, %s)"
                % (nm, opc, rt)))

def emit_probe(row):
    entry = row["entry"]
    if entry["stem"] is None:
        return
    name = name_for(row)
    PROBES.append(dict(row=row, name=name))

# ------------------------------------------------------------------
# probe generation: one C case per (entry, shape); the same source
# is compiled by TCC with this header and by the cross gcc with
# its own header, run under qemu, and the outputs diffed.
# ------------------------------------------------------------------
CASES = []   # (area, code)
CASES_TCC = []   # (area, code): cases the reference gcc header
# lacks (ACLE names it does not declare); they still run under
# TCC with their scalar references
GCC_MISSING_ROWS = {("FMLA_L", "f", 8, 8), ("FMLS_L", "f", 8, 8),
                    ("FMLA_L", "f", 8, 16), ("FMLS_L", "f", 8, 16)}
GCC_MISSING = {("BIT", "s"), ("BIT", "u"), ("BIT", "p"),
               ("BIF", "s"), ("BIF", "u"), ("BIF", "p"),
               ("AND", "p"), ("ORR", "p"), ("EOR", "p"),
               ("BIC", "p"), ("ORN", "p")}
CASE_SEQ = [0]

def next_idx():
    i = CASE_SEQ[0]
    CASE_SEQ[0] += 1
    return i

def add_case(row, code):
    entry = row["entry"]
    if (entry["op"], row["kl"]) in GCC_MISSING or \
            (entry["op"], row["kl"], row["es"], row["total"]) \
            in GCC_MISSING_ROWS:
        CASES_TCC.append((entry.get("area", "misc"), code))
    else:
        CASES.append((entry.get("area", "misc"), code))

def imm_values(row):
    entry = row["entry"]
    lo, hi = entry["imm"]
    if hi == "es8":
        hi = 8 * row["es"]
    elif hi == "es8m1":
        hi = 8 * row["es"] - 1
    elif hi == "totm1":
        hi = row["total"] - 1
    elif hi == "es4":
        hi = 4 * row["es"]
    vals = []
    for v in (lo, (lo + hi) // 2, hi):
        if v not in vals:
            vals.append(v)
    return vals

def fill_code(var, etype, n, seed, fl):
    if fl:
        tab = "ftab" if etype == "float" else "dtab"
        return "for (i = 0; i < %d; i++) %s.e[i] = %s[(i + %d) & 7];" \
            % (n, var, tab, seed)
    return "fill_bytes(%s.e, sizeof %s.e, %d);" % (var, var, seed)

def ref_code(row, rt_elem, rn):
    entry = row["entry"]
    ref = entry.get("ref")
    if not ref:
        return ""
    kl, es = row["kl"], row["es"]
    if kl == "f" and not any(h in ref for h in
                             ("fabs_ref", "fmax_ref", "fmin_ref")):
        return ""
    r = ref.replace("RES_ELEM", rt_elem)
    aoff = " + %d" % (row["total"] // row["es"] // 2) \
        if entry.get("kind") == "WIDEN_HI" else ""
    boff = " + %d" % (row["bsh"][1] // row["bsh"][0] // 2) \
        if entry.get("kind") in ("ACCW_HI", "WIDEN_HI", "ADDW_HI") else ""
    coff = " + %d" % (row["csh"][1] // row["csh"][0] // 2) \
        if entry.get("kind") in ("ACCW_HI",) and row["csh"] else ""
    r = r.replace("a[i]", "ua.e[i%s]" % aoff)
    r = r.replace("b[i]", "ub.e[i%s]" % boff)
    r = r.replace("c[i]", "uc.e[i%s]" % coff)
    r = r.replace("fabs_ref", "\x01")
    for h in ("sat_add", "sat_sub", "sat_abs", "sat_neg", "abs_ref",
              "cls_ref", "usqadd_ref", "uqadd_ref"):
        r = r.replace(h + "(", "%s_%s%d(" % (h, kl, es * 8))
    r = r.replace("\x01", "fabs_ref")
    return ("for (i = 0; i < %d; i++) { %s e_ = (%s); "
            "if (e_ != ur.e[i]) mismatch(\"%s\"); }"
            % (rn, rt_elem, r, row["name"] if "name" in row else ""))

def gen_case(row, name, call, extra_decls="", refname=""):
    entry = row["entry"]
    kl, es, total = row["kl"], row["es"], row["total"]
    idx = next_idx()
    at = type_of(kl, es, total)
    aet = ELEM_C[(kl, es)]
    an = total // es
    res = row["res"]
    if entry.get("style") == "op2i2" and "ressign" in entry:
        # the accumulator operand has the result's signedness
        at = type_of(entry["ressign"], es, total)
        aet = ELEM_C[(entry["ressign"], es)]
    if entry.get("kind") == "TBLQ" and entry["fam"] == "op2":
        at, aet, an = type_of(kl, es, 16), ELEM_C[(kl, es)], 16 // es
    if entry["op"] == "BSL":
        # the bit-select mask is an unsigned vector of the value shape
        at, aet, an = type_of("u", es, total), ELEM_C[("u", es)], total // es
    lines = ["static void case_%d(void) {" % idx,
             "    int i;",
             "    union { %s v; %s e[%d]; } ua;" % (at, aet, an)]
    if entry["fam"] in ("op2", "op3", "shuf"):
        bkl = entry.get("bkind", kl)
        if entry.get("kind") == "SHIFT":
            bkl = "s"  # ACLE shift counts are always signed
        if entry["op"] in ("TBL", "TBLQ"):
            bkl = "u" if kl == "p" else kl
        if entry.get("kind") == "TBLQ" and entry["fam"] == "op2":
            bkl = "u"  # the Q-table index operand is unsigned
        bes, btot = row["bsh"]
        bt = type_of(bkl, bes, btot)
        bet = ELEM_C[(bkl, bes)]
        lines.append("    union { %s v; %s e[%d]; } ub;"
                     % (bt, bet, btot // bes))
    if entry["fam"] == "op3":
        ckl = entry.get("bkind", kl)
        ces, ctot = row["csh"]
        if entry["op"] in ("TBX", "TBXQ") and kl == "p":
            ckl = "u"  # the table index operand is unsigned
        if entry["op"] in ("BIT", "BIF"):
            ckl = "u"  # the mask is an unsigned vector of the value shape
        if entry.get("kind") == "TBLQ":
            ckl = "u"  # the Q-table index operand is unsigned
        ct = type_of(ckl, ces, ctot)
        cet = ELEM_C[(ckl, ces)]
        lines.append("    union { %s v; %s e[%d]; } uc;"
                     % (ct, cet, ctot // ces))
    if extra_decls:
        lines.append(extra_decls)
    lines.append("    " + fill_code("ua", aet, an, idx, kl == "f"))
    if entry["fam"] in ("op2", "op3", "shuf"):
        lines.append("    " + fill_code("ub", ELEM_C[(entry.get("bkind", kl),
                                        row["bsh"][0])],
                                        row["bsh"][1] // row["bsh"][0],
                                        idx + 31, kl == "f"))
    if entry["fam"] == "op3":
        lines.append("    " + fill_code("uc", ELEM_C[(entry.get("bkind", kl),
                                        row["csh"][0])],
                                        row["csh"][1] // row["csh"][0],
                                        idx + 57, kl == "f"))
    if entry["fam"] == "op1r":
        rt = type_of(res_kind_letter(entry, kl), res[0], 0)
        lines.append("    { %s r_ = %s; print_hex(%d, &r_, sizeof r_); }"
                     % (rt, call, idx))
    else:
        rkl = result_kind_letter(entry, kl)
        rt = type_of(rkl, res[0], res[1])
        ret = ELEM_C[(rkl, res[0])]
        rn = res[1] // res[0]
        lines.append("    union { %s v; %s e[%d]; } ur;" % (rt, ret, rn))
        lines.append("    ur.v = %s;" % call)
        lines.append("    print_hex(%d, &ur, sizeof ur);" % idx)
        row["name"] = refname or name
        rc = ref_code(row, ret, rn)
        if rc:
            lines.append("    " + rc)
        rb = entry.get("refblock", "")
        if rb.startswith("tbl_ref"):
            bound = 16 if entry.get("kind") == "TBLQ" else total
            if entry["fam"] == "op2":
                lines.append(
                    '    tbl_check(0, (const unsigned char *)&ua, '
                    '(const unsigned char *)&ub, '
                    '(const unsigned char *)&ur, %d, %d, 0, "%s");'
                    % (total, bound, name))
            else:
                lines.append(
                    '    tbl_check((const unsigned char *)&ua, '
                    '(const unsigned char *)&ub, '
                    '(const unsigned char *)&uc, '
                    '(const unsigned char *)&ur, %d, %d, 1, "%s");'
                    % (total, bound, name))
    lines.append("}")
    add_case(row, "\n".join(lines))

def build_probes():
    for pr in PROBES:
        row = pr["row"]
        entry = row["entry"]
        name = pr["name"]
        fam = entry["fam"]
        style = entry.get("style", "")
        if entry.get("form") == "3REG_IDX":
            build_idx_probes(row, name)
        elif style in ("op1i",):
            for v in imm_values(row):
                gen_case(row, name, "%s(ua.v, %d)" % (name, v))
        elif style == "op2i2":
            for v in imm_values(row):
                gen_case(row, name, "%s(ua.v, ub.v, %d)" % (name, v))
        elif style == "op3i":
            for v in imm_values(row):
                gen_case(row, name, "%s(ua.v, ub.v, %d)" % (name, v))
        elif entry["fam"] == "shuf" and style == "op2i":
            seen = []
            for v in imm_values(row):
                v //= row["es"]
                if v not in seen:
                    seen.append(v)
                    gen_case(row, name,
                             "%s(ua.v, ub.v, %d)" % (name, v))
        elif fam == "op1" or fam == "op1r":
            gen_case(row, name, "%s(ua.v)" % name)
        elif fam in ("op2", "shuf"):
            gen_case(row, name, "%s(ua.v, ub.v)" % name)
        elif fam == "op3":
            gen_case(row, name, "%s(ua.v, ub.v, uc.v)" % name)

def build_idx_probes(row, name):
    entry = row["entry"]
    kl, es = row["kl"], row["es"]
    grp = es if entry.get("kind") == "DOT" else None
    if entry["fam"] == "op2":
        bes = row["bsh"][0]
        bkl = entry.get("bkind", kl)
        for btot in (8, 16):
            nm = idx_name(row, entry["stem"], btot)
            for lane in (0, btot // bes - 1):
                bt = type_of(bkl, bes, btot)
                decl = "    union { %s v; %s e[%d]; } ub2;" \
                    % (bt, ELEM_C[(bkl, bes)], btot // bes)
                r2 = dict(row)
                r2["bsh"] = (bes, btot)
                gen_case_idx(r2, nm, lane, decl, "ub2")
    else:
        ces = row["csh"][0]
        ckl = entry.get("bkind", kl)
        for ctot in (8, 16):
            nm = idx_name(row, entry["stem"], ctot)
            for lane in (0, ctot // (grp or ces) - 1):
                ct = type_of(ckl, ces, ctot)
                decl = "    union { %s v; %s e[%d]; } uc2;" \
                    % (ct, ELEM_C[(ckl, ces)], ctot // ces)
                r2 = dict(row)
                r2["csh"] = (ces, ctot)
                gen_case_idx3(r2, nm, lane, decl, "uc2")

def gen_case_idx(row, name, lane, decl, var):
    # op2 indexed: call NAME(ua.v, VAR.v, lane); the generic
    # gen_case declares ub from row["bsh"], so reuse it by
    # generating with the adjusted row and a renamed call.
    entry = row["entry"]
    kl, es, total = row["kl"], row["es"], row["total"]
    idx = next_idx()
    at = type_of(kl, es, total)
    aet = ELEM_C[(kl, es)]
    an = total // es
    bkl = entry.get("bkind", kl)
    bes, btot = row["bsh"]
    bt = type_of(bkl, bes, btot)
    bet = ELEM_C[(bkl, bes)]
    res = row["res"]
    rkl = result_kind_letter(entry, kl)
    rt = type_of(rkl, res[0], res[1])
    ret = ELEM_C[(rkl, res[0])]
    rn = res[1] // res[0]
    lines = ["static void case_%d(void) {" % idx,
             "    int i;",
             "    union { %s v; %s e[%d]; } ua;" % (at, aet, an),
             "    union { %s v; %s e[%d]; } ub;" % (bt, bet, btot // bes),
             "    union { %s v; %s e[%d]; } ur;" % (rt, ret, rn),
             "    " + fill_code("ua", aet, an, idx, kl == "f"),
             "    " + fill_code("ub", bet, btot // bes, idx + 31,
                                kl == "f"),
             "    ur.v = %s(ua.v, ub.v, %d);" % (name, lane),
             "    print_hex(%d, &ur, sizeof ur);" % idx,
             "}"]
    add_case(row, "\n".join(lines))

def gen_case_idx3(row, name, lane, decl, var):
    entry = row["entry"]
    kl, es, total = row["kl"], row["es"], row["total"]
    idx = next_idx()
    at = type_of(kl, es, total)
    aet = ELEM_C[(kl, es)]
    an = total // es
    bkl = entry.get("bkind", kl)
    bes, btot = row["bsh"]
    bt = type_of(bkl, bes, btot)
    bet = ELEM_C[(bkl, bes)]
    ces, ctot = row["csh"]
    ct = type_of(entry.get("bkind", kl), ces, ctot)
    cet = ELEM_C[(entry.get("bkind", kl), ces)]
    res = row["res"]
    rkl = result_kind_letter(entry, kl)
    rt = type_of(rkl, res[0], res[1])
    ret = ELEM_C[(rkl, res[0])]
    rn = res[1] // res[0]
    lines = ["static void case_%d(void) {" % idx,
             "    int i;",
             "    union { %s v; %s e[%d]; } ua;" % (at, aet, an),
             "    union { %s v; %s e[%d]; } ub;" % (bt, bet, btot // bes),
             "    union { %s v; %s e[%d]; } uc;" % (ct, cet, ctot // ces),
             "    union { %s v; %s e[%d]; } ur;" % (rt, ret, rn),
             "    " + fill_code("ua", aet, an, idx, kl == "f"),
             "    " + fill_code("ub", bet, btot // bes, idx + 31,
                                kl == "f"),
             "    " + fill_code("uc", cet, ctot // ces, idx + 57,
                                kl == "f"),
             "    ur.v = %s(ua.v, ub.v, uc.v, %d);" % (name, lane),
             "    print_hex(%d, &ur, sizeof ur);" % idx,
             "}"]
    add_case(row, "\n".join(lines))

# ------------------------------------------------------------------
# C table emission (the block between NEON_TABLES_BEGIN/END in
# arm64-gen.c)
# ------------------------------------------------------------------
def imm_hi_of(row):
    entry = row["entry"]
    if "imm" not in entry:
        return 0
    lo, hi = entry["imm"]
    if hi == "es8":
        return 8 * row["es"]
    if hi == "es8m1":
        return 8 * row["es"] - 1
    if hi == "es4":
        return 4 * row["es"]
    if hi == "totm1":
        return row["total"] - 1
    return hi

def emit_tables(op_index):
    out = []
    famnames = {"op1": ("A64OP1", "AVop1", "a64_op1_table"),
                "op2": ("A64OP2", "AVop2", "a64_op2_table"),
                "op3": ("A64OP3", "AVop3", "a64_op3_table"),
                "op1r": ("A64OP1R", "AVop1", "a64_op1r_table"),
                "shuf": ("A64SHUF", "AVop2", "a64_shuf_table")}
    for fam in ("op1", "op2", "op3", "op1r", "shuf"):
        pre, sty, tname = famnames[fam]
        ops = sorted(op_index.get(fam, {}).items(), key=lambda kv: kv[1])
        out.append("enum { %s };" % ", ".join(
            "%s_%s" % (pre, op) for op, _ in ops))
        out.append("static const %s %s[] = {" % (sty, tname))
        for row in ROWS[fam]:
            entry = row["entry"]
            fields = [".opc = %d" % opc_of(row),
                      ".es = %d" % row["es"]]
            if row["fl"]:
                fields.append(".fl = 1")
            oes = row["res"][0] if entry.get("kind") not in (
                "SAME", "BYTES", "SHIFT") else row["es"]
            if entry.get("kind") in ("ACCW_LO", "ACCW_HI", "DOT"):
                oes = row["bsh"][0]
            if oes != row["es"]:
                fields.append(".oes = %d" % oes)
            if entry.get("ofl"):
                fields.append(".ofl = 1")
            kind = entry.get("kind", "SAME")
            if kind != "SAME":
                fields.append(".kind = A64K_%s" % kind)
            form = row.get("form") or entry.get("form")
            if not form:
                if fam in ("op1", "op1r"):
                    form = "2REG_IMM" if "imm" in entry else "2REG"
                elif fam == "op3":
                    form = "3REG_IMM" if "imm" in entry else "3REG_ACC"
                else:
                    form = "3REG_IMM" if "imm" in entry else "3REG"
            fields.append(".form = A64F_%s" % form)
            if "imm" in entry:
                fields.append(".imm_lo = %d" % entry["imm"][0])
                fields.append(".imm_hi = %d" % imm_hi_of(row))
            if row["imm_pos"]:
                fields.append(".imm_pos = %d" % row["imm_pos"])
            if row["imm_base"]:
                fields.append(".imm_base = %d" % row["imm_base"])
            if row["imm_sign"]:
                fields.append(".imm_sign = %d" % row["imm_sign"])
            if row["imm_spread"]:
                fields.append(".imm_spread = 0x%x" % row["imm_spread"])
            if entry.get("feat"):
                fields.append(".feat = TFEAT_%s" % entry["feat"])
            if row["enc"]:
                fields.append(".enc = 0x%08x" % row["enc"])
            out.append("    { %s }," % ", ".join(fields))
        out.append("    { .opc = -1 }")
        out.append("};")
    return "\n".join(out)

# ------------------------------------------------------------------
# header emission
# ------------------------------------------------------------------
BASE_TYPES = []
for _kl in ("s", "u", "p", "f"):
    for _es, _ns in ((1, (8, 16)), (2, (4, 8)), (4, (2, 4)),
                     (8, (1, 2))):
        if _kl == "f" and _es < 4:
            continue
        if _kl == "p" and _es > 2 and not (_es == 8):
            continue
        for _n in _ns:
            BASE_TYPES.append((_kl, _es, _n * _es))

def emit_header():
    out = []
    out.append("/* NEON intrinsics for AArch64 (ACLE-compatible")
    out.append("   surface).  Every function-like name here is a")
    out.append("   macro: over a compiler builtin that emits the")
    out.append("   real instruction, over a C operator with the")
    out.append("   same semantics, or over a composition of those. */")
    out.append("#ifndef __ARM_NEON_H")
    out.append("#define __ARM_NEON_H")
    out.append("")
    out.append("typedef unsigned char poly8_t;")
    out.append("typedef unsigned short poly16_t;")
    out.append("typedef unsigned long long poly64_t;")
    out.append("")
    for (kl, es, total) in BASE_TYPES:
        n = total // es
        out.append("typedef %s %s __attribute((vector_size(%d)));"
                   % (ELEM_C[(kl, es)], vec_type(kl, es, n), total))
    out.append("")
    for (kl, es, total) in BASE_TYPES:
        t = vec_type(kl, es, total // es)
        for cnt in (2, 3, 4):
            out.append("typedef struct { %s val[%d]; } %sx%d_t;"
                       % (t, cnt, t[:-2], cnt))
    out.append("")
    out.append("/* generated operation macros "
               "(tools/neon_gen.py) */")
    seen = set()
    for name, line in MACROS:
        if name in seen:
            raise AssertionError("duplicate macro " + name)
        seen.add(name)
        out.append(line)
    out += emit_splat_extract()
    out += emit_compositions()
    out.append("")
    out.append("#endif")
    return "\n".join(out) + "\n", seen

def shape_opc(es, total, fl=0):
    return shape_id(es, total // es, fl)

def emit_splat_extract():
    out = ["", "/* splat, lane extract and lane replace */"]
    for (kl, es, total) in BASE_TYPES:
        t = vec_type(kl, es, total // es)
        et = ELEM_C[(kl, es)]
        suf = suffix_of(kl, es, total)
        qpre = "q" if total == 16 else ""
        out.append("#define vdup%s_n_%s(a) __builtin_aarch64_splat(%d, a, %s)"
                   % (qpre, kl + str(es * 8), shape_opc(es, total, kl == "f"), t))
        out.append("#define vmov%s_n_%s(a) __builtin_aarch64_splat(%d, a, %s)"
                   % (qpre, kl + str(es * 8), shape_opc(es, total, kl == "f"), t))
        lname = "vget_lane" if total == 8 else "vgetq_lane"
        out.append("#define %s%s(a, n) __builtin_aarch64_extract(%d, a, n, %s)"
                   % (lname, "_" + kl + str(es * 8), shape_opc(es, total, kl == "f"), et))
        sname = "vset_lane" if total == 8 else "vsetq_lane"
        out.append("#define %s_%s(a, b, n) __builtin_aarch64_replace(%d, a, n, b)"
                   % (sname, kl + str(es * 8), shape_opc(es, total, kl == "f")))
    return out

def emit_compositions():
    out = ["", "/* loads, stores and compositions */"]
    # horizontal reductions the ISA only has in pairwise or
    # full-width form: compose them from real instructions
    for _sgn in ("s", "u"):
        out.append("#define vaddv_%s32(a) vget_lane_%s32(vpadd_%s32(a, a), 0)"
                   % (_sgn, _sgn, _sgn))
        out.append("#define vmaxv_%s32(a) vget_lane_%s32(vpmax_%s32(a, a), 0)"
                   % (_sgn, _sgn, _sgn))
        out.append("#define vminv_%s32(a) vget_lane_%s32(vpmin_%s32(a, a), 0)"
                   % (_sgn, _sgn, _sgn))
        out.append("#define vaddvq_%s64(a) vpaddd_%s64(a)"
                   % (_sgn, _sgn))
    out.append("#define vmaxnmv_f32(a) vmaxnmvq_f32(vcombine_f32(a, a))")
    out.append("#define vminnmv_f32(a) vminnmvq_f32(vcombine_f32(a, a))")
    out.append("#define vaddvq_f32(a) vaddv_f32(vget_low_f32(vpaddq_f32(a, a)))")
    out.append("#define vaddvq_f64(a) vpaddd_f64(a)")
    out.append("#define vmaxvq_f64(a) vget_lane_f64(vmax_f64(vget_low_f64(a), vget_high_f64(a)), 0)")
    out.append("#define vminvq_f64(a) vget_lane_f64(vmin_f64(vget_low_f64(a), vget_high_f64(a)), 0)")
    for (kl, es, total) in BASE_TYPES:
        t = vec_type(kl, es, total // es)
        et = ELEM_C[(kl, es)]
        suf = suffix_of(kl, es, total)
        elem = kl + str(es * 8)
        # vld1 / vst1 are plain dereferences
        out.append("#define vld1%s(p) (*(const %s *)(p))" % (suf, t))
        out.append("#define vst1%s(p, a) do { *(%s *)(p) = (a); } while (0)"
                   % (suf, t))
        qpre = "q" if total == 16 else ""
        setn = "vsetq_lane" if total == 16 else "vset_lane"
        getn = "vgetq_lane" if total == 16 else "vget_lane"
        out.append("#define vld1%s_lane_%s(p, a, n) %s_%s(*(p), a, n)"
                   % (qpre, elem, setn, elem))
        out.append("#define vst1%s_lane_%s(p, a, n) do { *(p) = %s_%s(a, n); } while (0)"
                   % (qpre, elem, getn, elem))
        out.append("#define vld1%s_dup_%s(p) vdup%s_n_%s(*(p))"
                   % (qpre, elem, qpre, elem))
        # vdup_lane family: result total x source total
        for rt_total, rq in ((8, ""), (16, "q")):
            rt = vec_type(kl, es, rt_total // es)
            for st_total, sq in ((8, ""), (16, "q")):
                nm = "vdup%s_lane%s_%s" % (rq, sq, elem)
                gl = "vget_lane" if st_total == 8 else "vgetq_lane"
                out.append("#define %s(a, n) __builtin_aarch64_splat(%d, %s_%s(a, n), %s)"
                           % (nm, shape_opc(es, rt_total, kl == "f"), gl, elem, rt))
        # vcopy_lane family
        for rt_total, rq in ((8, ""), (16, "q")):
            rt = vec_type(kl, es, rt_total // es)
            rsuf = suffix_of(kl, es, rt_total)
            for st_total, sq in ((8, ""), (16, "q")):
                nm = "vcopy%s_lane%s_%s" % (rq, sq, elem)
                gl = "vget_lane" if st_total == 8 else "vgetq_lane"
                setn = "vset_lane" if rt_total == 8 else "vsetq_lane"
                out.append("#define %s(a, na, b, nb) %s_%s(%s_%s(b, nb), a, na)"
                           % (nm, setn, elem, gl, elem))
        # structured loads and stores
        for cnt in (2, 3, 4):
            st = "%sx%d_t" % (t[:-2], cnt)
            op = cnt - 2
            opc0 = (op << 11) | shape_opc(es, total, kl == "f")
            opcl = ((4 + op) << 11) | shape_opc(es, total, kl == "f")
            opcx = ((8 + op) << 11) | shape_opc(es, total, kl == "f")
            out.append("#define vld%d%s(p) (__extension__({ %s __t[1]; __builtin_aarch64_vldn(%d, p, __t); __t[0]; }))"
                       % (cnt, suf, st, opc0))
            out.append("#define vst%d%s(p, a) __builtin_aarch64_vstn(%d, p, (%s[]){ (a) })"
                       % (cnt, suf, opc0, st))
            out.append("#define vld%d%s_lane_%s(p, a, n) (__extension__({ %s __t[1] = { (a) }; __builtin_aarch64_vldn(%d + ((n) << 4), p, __t); __t[0]; }))"
                       % (cnt, "q" if total == 16 else "", elem, st, opcl))
            out.append("#define vst%d%s_lane_%s(p, a, n) __builtin_aarch64_vstn(%d + ((n) << 4), p, (%s[]){ (a) })"
                       % (cnt, "q" if total == 16 else "", elem, opcl, st))
            out.append("#define vld1%s_x%d(p) (__extension__({ %s __t[1]; __builtin_aarch64_vldn(%d, p, __t); __t[0]; }))"
                       % (suf, cnt, st, opcx))
            out.append("#define vst1%s_x%d(p, a) __builtin_aarch64_vstn(%d, p, (%s[]){ (a) })"
                       % (suf, cnt, opcx, st))
            opcd = ((12 + op) << 11) | shape_opc(es, total, kl == "f")
            out.append("#define vld%d%s_dup_%s(p) (__extension__({ %s __t[1]; __builtin_aarch64_vldn(%d, p, __t); __t[0]; }))"
                       % (cnt, "q" if total == 16 else "", elem, st,
                          opcd))
    # scalar-operand forms: X_n(a, ..., s) is X with the scalar
    # broadcast, a composition over the register form
    have = {name for name, _ in MACROS}
    for (kl, es, total) in BASE_TYPES:
        if es == 8 and kl != "f":
            continue
        elem = kl + str(es * 8)
        q = "q" if total == 16 else ""
        dup = "vdup%s_n_%s" % (q, elem)
        for stem in ("vmul",):
            base = "%s%s_%s" % (stem, q, elem)
            if base in have:
                out.append("#define %s%s_n_%s(a, s) %s(a, %s(s))"
                           % (stem, q, elem, base, dup))
        if kl == "p":
            continue
        for stem in ("vmla", "vmls", "vfma", "vfms"):
            if stem.startswith("vf") and kl != "f":
                continue
            base = "%s%s_%s" % (stem, q, elem)
            if base in have:
                out.append("#define %s%s_n_%s(a, b, s) %s(a, b, %s(s))"
                           % (stem, q, elem, base, dup))
        for stem in ("vqdmulh", "vqrdmulh"):
            base = "%s%s_%s" % (stem, q, elem)
            if base in have:
                out.append("#define %s%s_n_%s(a, s) %s(a, %s(s))"
                           % (stem, q, elem, base, dup))
        if total == 8:
            for stem in ("vmull", "vqdmull"):
                base = "%s_%s" % (stem, elem)
                if base in have:
                    out.append("#define %s_n_%s(a, s) %s(a, vdup_n_%s(s))"
                               % (stem, elem, base, elem))
            for stem in ("vmlal", "vmlsl", "vqdmlal", "vqdmlsl"):
                base = "%s_%s" % (stem, elem)
                if base in have:
                    out.append("#define %s_n_%s(a, b, s) %s(a, b, vdup_n_%s(s))"
                               % (stem, elem, base, elem))
        if total == 16:
            for stem in ("vmull", "vqdmull"):
                base = "%s_high_%s" % (stem, elem)
                if base in have:
                    out.append("#define %s_high_n_%s(a, s) %s(a, vdupq_n_%s(s))"
                               % (stem, elem, base, elem))
            for stem in ("vmlal", "vmlsl", "vqdmlal", "vqdmlsl"):
                base = "%s_high_%s" % (stem, elem)
                if base in have:
                    out.append("#define %s_high_n_%s(a, b, s) %s(a, b, vdupq_n_%s(s))"
                               % (stem, elem, base, elem))
    have = {name for name, _ in MACROS}
    for (kl, es, total) in BASE_TYPES:
        if kl == "p" and es > 2:
            continue
        elem = kl + str(es * 8)
        suf = suffix_of(kl, es, total)
        t2 = "%sx2_t" % vec_type(kl, es, total // es)[:-2]
        for stem, one, two in (("vtrn", "vtrn1", "vtrn2"),
                               ("vzip", "vzip1", "vzip2"),
                               ("vuzp", "vuzp1", "vuzp2")):
            if "%s%s" % (one, suf) in have:
                out.append("#define %s%s(a, b) ((%s){ { %s%s(a, b), "
                           "%s%s(a, b) } })"
                           % (stem, suf, t2, one, suf, two, suf))
    for sgn in ("s", "u"):
        out.append("#define vaddlv_%s8(a) vaddv_%s16(vpaddl_%s8(a))"
                   % (sgn, sgn, sgn))
        out.append("#define vaddlvq_%s8(a) vaddvq_%s16(vpaddlq_%s8(a))"
                   % (sgn, sgn, sgn))
        out.append("#define vaddlv_%s16(a) vaddv_%s32(vpaddl_%s16(a))"
                   % (sgn, sgn, sgn))
        out.append("#define vaddlvq_%s16(a) vaddvq_%s32(vpaddlq_%s16(a))"
                   % (sgn, sgn, sgn))
        out.append("#define vaddlv_%s32(a) vget_lane_%s64(vpaddl_%s32(a), 0)"
                   % (sgn, sgn, sgn))
        out.append("#define vaddlvq_%s32(a) vpaddd_%s64(vpaddlq_%s32(a))"
                   % (sgn, sgn, sgn))
    for f in (32, 64):
        for q in ("", "q"):
            out.append("#define vcagt%s_f%d(a, b) vcgt%s_f%d(vabs%s_f%d(a), vabs%s_f%d(b))"
                       % (q, f, q, f, q, f, q, f))
            out.append("#define vcalt%s_f%d(a, b) vclt%s_f%d(vabs%s_f%d(a), vabs%s_f%d(b))"
                       % (q, f, q, f, q, f, q, f))
    out.append("#define vceq_p8(a, b) vceq_u8(vreinterpret_u8_p8(a), vreinterpret_u8_p8(b))")
    # the D-register shift forms also carry a d in the stem
    have2 = {name for name, _ in MACROS}
    for plain, dstem in (("vrsra_n", "vrsrad_n"), ("vsra_n", "vsrad_n"),
                         ("vsli_n", "vslid_n"), ("vsri_n", "vsrid_n")):
        for sgn in ("s64", "u64"):
            if "%s_%s" % (plain, sgn) in have2:
                out.append("#define %s_%s(a, n) %s_%s(a, n)"
                           % (dstem, sgn, plain, sgn))
    for plain, dstem in (("vqrshl", "vqrshld"), ("vqshl", "vqshld"),
                         ("vrshl", "vrshld"), ("vshl", "vshld")):
        for sgn in ("s64", "u64"):
            if "%s_%s" % (plain, sgn) in have2:
                out.append("#define %s_%s(a, b) %s_%s(a, b)"
                           % (dstem, sgn, plain, sgn))
    out.append("#define vceqq_p8(a, b) vceqq_u8(vreinterpretq_u8_p8(a), vreinterpretq_u8_p8(b))")
    # float absolute comparisons: vcage/vcale (and the gt/lt
    # forms above) compare absolute values
    for es in (32, 64):
        out.append("#define vcage_f%d(a, b) vcge_f%d(vabs_f%d(a), vabs_f%d(b))"
                   % (es, es, es, es))
        out.append("#define vcale_f%d(a, b) vcle_f%d(vabs_f%d(a), vabs_f%d(b))"
                   % (es, es, es, es))
        out.append("#define vcageq_f%d(a, b) vcgeq_f%d(vabsq_f%d(a), vabsq_f%d(b))"
                   % (es, es, es, es))
        out.append("#define vcaleq_f%d(a, b) vcleq_f%d(vabsq_f%d(a), vabsq_f%d(b))"
                   % (es, es, es, es))
    # vreinterpret: a plain cast between any two vector types of
    # the same total size
    for total in (8, 16):
        q = "q" if total == 16 else ""
        types = [(kl, es) for (kl, es, t) in BASE_TYPES if t == total]
        for (k1, e1) in types:
            for (k2, e2) in types:
                if (k1, e1) == (k2, e2):
                    continue
                out.append("#define vreinterpret%s_%s%d_%s%d(a) ((%s)(a))"
                           % (q, k2, e2 * 8, k1, e1 * 8,
                              vec_type(k2, e2, total // e2)))
    # vcreate: build a 64-bit vector from a bit pattern
    for (kl, es, total) in BASE_TYPES:
        if total != 8:
            continue
        out.append("#define vcreate_%s%d(x) (__extension__({ union { unsigned long long __u; %s __v; } __z; __z.__u = (x); __z.__v; }))"
                   % (kl, es * 8, vec_type(kl, es, 8 // es)))
    return out


# ------------------------------------------------------------------
# sweep probe: mechanical per-type coverage of the load/store,
# lane, splat and reinterpret families (differential against gcc;
# names the gcc header lacks go to the TCC-only file)
# ------------------------------------------------------------------

def _gcc_neon_names():
    import glob as _glob
    names = set()
    for path in _glob.glob("/usr/lib/gcc-cross/aarch64-linux-gnu/"
                           "*/include/arm_neon.h"):
        import re as _re
        for m in _re.finditer(r"\b(v[a-z][a-z0-9_]*)\s*\(",
                              open(path).read()):
            names.add(m.group(1))
    return names


def emit_sweep(header_text):
    import re as _re
    shipped = set(_re.findall(r"#define (v\w+)\(", header_text))
    gcc_names = _gcc_neon_names()
    both, tconly = [], []
    idx = [0]

    def case(code, names):
        idx[0] += 1
        if not all(n in shipped for n in names):
            return
        body = "    { %s }" % code
        if not gcc_names or all(n in gcc_names for n in names):
            both.append(body)
        else:
            tconly.append(body)

    for (kl, es, total) in BASE_TYPES:
        n = total // es
        T = vec_type(kl, es, n)
        E = ELEM_C[(kl, es)]
        q = "q" if total == 16 else ""
        suf = "%s%d" % (kl, es * 8)
        P = "(const %s *)sb.b" % E
        # plain load/store
        case("%s v = vld1%s_%s(%s); vst1%s_%s((%s *)ob.b, v);"
             " print_hex(idx++, ob.b, %d);"
             % (T, q, suf, P, q, suf, E, total),
             ["vld1%s_%s" % (q, suf), "vst1%s_%s" % (q, suf)])
        # multi-register load/store x2/x3/x4
        for cnt in (2, 3, 4):
            TT = "%sx%d_t" % (T[:-2], cnt)
            stmts = ["%s r = vld1%s_%s_x%d(%s);" % (TT, q, suf, cnt, P)]
            for i in range(cnt):
                stmts.append("vst1%s_%s((%s *)ob.b + %d, r.val[%d]);"
                             " print_hex(idx++, ob.b + %d, %d);"
                             % (q, suf, E, i * n, i, i * total, total))
            case(" ".join(stmts),
                 ["vld1%s_%s_x%d" % (q, suf, cnt),
                  "vst1%s_%s" % (q, suf)])
            stmts = ["%s r = vld1%s_%s_x%d(%s);" % (TT, q, suf, cnt, P),
                     "vst1%s_%s_x%d((%s *)ob.b, r);" % (q, suf, cnt, E)]
            case(" ".join(stmts), ["vst1%s_%s_x%d" % (q, suf, cnt)])
        # structured (interleaved) load/store vld2/3/4 + lane + dup
        for cnt in (2, 3, 4):
            TT = "%sx%d_t" % (T[:-2], cnt)
            nm = "vld%d%s_%s" % (cnt, q, suf)
            case("%s r = %s(%s); vst%d%s_%s((%s *)ob.b, r);"
                 " print_hex(idx++, ob.b, %d);"
                 % (TT, nm, P, cnt, q, suf, E, cnt * total),
                 [nm, "vst%d%s_%s" % (cnt, q, suf)])
            nmd = "vld%d%s_dup_%s" % (cnt, q, suf)
            case("%s r = %s(%s); vst%d%s_%s((%s *)ob.b, r);"
                 " print_hex(idx++, ob.b, %d);"
                 % (TT, nmd, P, cnt, q, suf, E, cnt * total),
                 [nmd, "vst%d%s_%s" % (cnt, q, suf)])
            if n == 1:
                nml = "vld%d%s_lane_%s" % (cnt, q, suf)
                case("memset(ob.b, 0, 64);"
                     " %s a = %s(%s);"
                     " %s r = %s(%s + 2, a, 0);"
                     " vst%d%s_lane_%s((%s *)ob.b, r, 0);"
                     " print_hex(idx++, ob.b, 64);"
                     % (TT, nm, P, TT, nml, P, cnt, q, suf, E),
                     [nml, "vst%d%s_lane_%s" % (cnt, q, suf)])
            if n > 1:
                nml = "vld%d%s_lane_%s" % (cnt, q, suf)
                case("memset(ob.b, 0, 64);"
                     " %s a = %s(%s);"
                     " %s r = %s(%s + 2, a, %d);"
                     " vst%d%s_lane_%s((%s *)ob.b, r, %d);"
                     " print_hex(idx++, ob.b, 64);"
                     % (TT, nm, P, TT, nml, P, n - 1, cnt, q, suf, E,
                        n - 1),
                     [nml, "vst%d%s_lane_%s" % (cnt, q, suf)])
        if n > 1:
            # lane load/store
            case("memset(ob.b, 0, 64);"
                 " %s v = vld1%s_%s(%s);"
                 " %s w = vld1%s_lane_%s(%s + 3, v, %d);"
                 " vst1%s_lane_%s((%s *)ob.b, w, %d);"
                 " print_hex(idx++, ob.b, %d);"
                 % (T, q, suf, P, T, q, suf, P, n - 1, q, suf, E, n - 1,
                    total),
                 ["vld1%s_lane_%s" % (q, suf), "vst1%s_lane_%s" % (q, suf)])
        # dup load
        case("%s v = vld1%s_dup_%s(%s + 1); print_hex(idx++, &v, %d);"
             % (T, q, suf, P, total), ["vld1%s_dup_%s" % (q, suf)])
        # get/set lane
        case("%s v = vld1%s_%s(%s); %s e = vget%s_lane_%s(v, %d);"
             " print_hex(idx++, &e, %d);"
             " %s w = vset%s_lane_%s(((const %s *)sb.b)[5], v, %d);"
             " print_hex(idx++, &w, %d);"
             % (T, q, suf, P, E, q, suf, n - 1, es, T, q, suf, E,
                n - 1, total),
             ["vget%s_lane_%s" % (q, suf), "vset%s_lane_%s" % (q, suf)])
        # splat / mov_n / create
        case("%s v = vdup%s_n_%s(((const %s *)sb.b)[3]);"
             " print_hex(idx++, &v, %d);"
             % (T, q, suf, E, total), ["vdup%s_n_%s" % (q, suf)])
        case("%s v = vmov%s_n_%s(((const %s *)sb.b)[4]);"
             " print_hex(idx++, &v, %d);"
             % (T, q, suf, E, total), ["vmov%s_n_%s" % (q, suf)])
        if total == 8:
            case("%s v = vcreate_%s(0x0102030405060708ULL);"
                 " print_hex(idx++, &v, %d);" % (T, suf, total),
                 ["vcreate_%s" % suf])
    # dup-lane / copy-lane four combinations, per (kind, element size)
    for kl in ("s", "u", "p", "f"):
        for es in (1, 2, 4, 8):
            if kl == "f" and es < 4:
                continue
            if kl == "p" and es == 4:
                continue
            suf = "%s%d" % (kl, es * 8)
            E = ELEM_C[(kl, es)]
            TD = vec_type(kl, es, 8 // es)
            TQ = vec_type(kl, es, 16 // es)
            ld = min(1, 8 // es - 1)
            lq = min(1, 16 // es - 1)
            P = "(const %s *)sb.b" % E
            case("%s a = vld1_%s(%s); %s r = vdup_lane_%s(a, %d);"
                 " print_hex(idx++, &r, 8);"
                 % (TD, suf, P, TD, suf, ld), ["vdup_lane_%s" % suf])
            case("%s a = vld1_%s(%s); %s r = vdupq_lane_%s(a, %d);"
                 " print_hex(idx++, &r, 16);"
                 % (TD, suf, P, TQ, suf, ld), ["vdupq_lane_%s" % suf])
            case("%s a = vld1q_%s(%s); %s r = vdup_laneq_%s(a, %d);"
                 " print_hex(idx++, &r, 8);"
                 % (TQ, suf, P, TD, suf, lq), ["vdup_laneq_%s" % suf])
            case("%s a = vld1q_%s(%s); %s r = vdupq_laneq_%s(a, %d);"
                 " print_hex(idx++, &r, 16);"
                 % (TQ, suf, P, TQ, suf, lq), ["vdupq_laneq_%s" % suf])
            case("%s a = vld1_%s(%s); %s b = vld1_%s(%s + 8);"
                 " %s r = vcopy_lane_%s(a, %d, b, %d);"
                 " print_hex(idx++, &r, 8);"
                 % (TD, suf, P, TD, suf, P, TD, suf, ld, ld),
                 ["vcopy_lane_%s" % suf])
            case("%s a = vld1q_%s(%s); %s b = vld1_%s(%s + 8);"
                 " %s r = vcopyq_lane_%s(a, %d, b, %d);"
                 " print_hex(idx++, &r, 16);"
                 % (TQ, suf, P, TD, suf, P, TQ, suf, lq, ld),
                 ["vcopyq_lane_%s" % suf])
            case("%s a = vld1_%s(%s); %s b = vld1q_%s(%s + 8);"
                 " %s r = vcopy_laneq_%s(a, %d, b, %d);"
                 " print_hex(idx++, &r, 8);"
                 % (TD, suf, P, TQ, suf, P, TD, suf, ld, lq),
                 ["vcopy_laneq_%s" % suf])
            case("%s a = vld1q_%s(%s); %s b = vld1q_%s(%s + 8);"
                 " %s r = vcopyq_laneq_%s(a, %d, b, %d);"
                 " print_hex(idx++, &r, 16);"
                 % (TQ, suf, P, TQ, suf, P, TQ, suf, lq, lq),
                 ["vcopyq_laneq_%s" % suf])
            if es == 8:
                for stem in ("vtrn", "vzip", "vuzp"):
                    case("%s a = vld1q_%s(%s); %s b = vld1q_%s(%s + 8);"
                         " %sx2_t r = %sq_%s(a, b);"
                         " print_hex(idx++, &r.val[0], 16);"
                         " print_hex(idx++, &r.val[1], 16);"
                         % (TQ, suf, P, TQ, suf, P, TQ[:-2], stem, suf),
                         ["%sq_%s" % (stem, suf)])
            if es < 8:
                # transpose / zip / unzip struct forms
                for stem in ("vtrn", "vzip", "vuzp"):
                    case("%s a = vld1_%s(%s); %s b = vld1_%s(%s + 8);"
                         " %sx2_t r = %s_%s(a, b);"
                         " print_hex(idx++, &r.val[0], 8);"
                         " print_hex(idx++, &r.val[1], 8);"
                         % (TD, suf, P, TD, suf, P, TD[:-2], stem, suf),
                         ["%s_%s" % (stem, suf)])
                    case("%s a = vld1q_%s(%s); %s b = vld1q_%s(%s + 8);"
                         " %sx2_t r = %sq_%s(a, b);"
                         " print_hex(idx++, &r.val[0], 16);"
                         " print_hex(idx++, &r.val[1], 16);"
                         % (TQ, suf, P, TQ, suf, P, TQ[:-2], stem, suf),
                         ["%sq_%s" % (stem, suf)])
    # get_low / get_high / combine for 16-byte types
    for (kl, es, total) in BASE_TYPES:
        if total != 16:
            continue
        n = total // es
        T = vec_type(kl, es, n)
        TD = vec_type(kl, es, n // 2)
        suf = "%s%d" % (kl, es * 8)
        E = ELEM_C[(kl, es)]
        P = "(const %s *)sb.b" % E
        case("%s a = vld1q_%s(%s); %s lo = vget_low_%s(a);"
             " %s hi = vget_high_%s(a); %s r = vcombine_%s(hi, lo);"
             " print_hex(idx++, &lo, 8); print_hex(idx++, &hi, 8);"
             " print_hex(idx++, &r, 16);"
             % (T, suf, P, TD, suf, TD, suf, T, suf),
             ["vget_low_%s" % suf, "vget_high_%s" % suf,
              "vcombine_%s" % suf])
    # reinterpret: every ordered pair of distinct types of one total
    for total in (8, 16):
        types = [(kl, es) for (kl, es, t) in BASE_TYPES if t == total]
        q = "q" if total == 16 else ""
        for (k1, e1) in types:
            for (k2, e2) in types:
                if (k1, e1) == (k2, e2):
                    continue
                T1 = vec_type(k1, e1, total // e1)
                T2 = vec_type(k2, e2, total // e2)
                nm = "vreinterpret%s_%s%d_%s%d" % (q, k2, e2 * 8, k1,
                                                  e1 * 8)
                case("%s a = vld1%s_%s%d((const %s *)sb.b);"
                     " %s r = %s(a); print_hex(idx++, &r, %d);"
                     % (T1, q, k1, e1 * 8, ELEM_C[(k1, e1)], T2, nm,
                        total),
                     [nm, "vld1%s_%s%d" % (q, k1, e1 * 8)])
    # scalar-operand (_n) compositions across types
    for (kl, es, total) in BASE_TYPES:
        if kl == "p":
            continue
        n = total // es
        T = vec_type(kl, es, n)
        E = ELEM_C[(kl, es)]
        q = "q" if total == 16 else ""
        suf = "%s%d" % (kl, es * 8)
        P = "(const %s *)sb.b" % E
        sc = "((const %s *)sb.b)[6]" % E
        for stem in ("vmul", "vmla", "vmls"):
            nn = 2 if stem == "vmul" else 3
            args = ", ".join(["vld1%s_%s(%s + %d)" % (q, suf, P, i * n)
                              for i in range(nn - 1)] + [sc])
            case("%s r = %s%s_n_%s(%s); print_hex(idx++, &r, %d);"
                 % (T, stem, q, suf, args, total),
                 ["%s%s_n_%s" % (stem, q, suf)])
        if total == 8 and es < 8:
            TQ = vec_type(kl, es * 2, n)
            case("%s r = vmull_n_%s(vld1_%s(%s), %s);"
                 " print_hex(idx++, &r, 16);"
                 % (TQ, suf, suf, P, sc), ["vmull_n_%s" % suf])
    # lane forms of one-lane types, float fma _n compositions,
    # long pairwise adds and the widening _n compositions
    for (kl, es, total) in BASE_TYPES:
        n = total // es
        T = vec_type(kl, es, n)
        E = ELEM_C[(kl, es)]
        q = "q" if total == 16 else ""
        suf = "%s%d" % (kl, es * 8)
        P = "(const %s *)sb.b" % E
        sc = "((const %s *)sb.b)[6]" % E
        if n == 1:
            case("memset(ob.b, 0, 64);"
                 " %s v = vld1%s_%s(%s);"
                 " %s w = vld1%s_lane_%s(%s + 1, v, 0);"
                 " vst1%s_lane_%s((%s *)ob.b, w, 0);"
                 " print_hex(idx++, ob.b, %d);"
                 % (T, q, suf, P, T, q, suf, P, q, suf, E, total),
                 ["vld1%s_lane_%s" % (q, suf),
                  "vst1%s_lane_%s" % (q, suf)])
        if kl == "f":
            for stem in ("vfma", "vfms"):
                case("%s r = %s%s_n_%s(vld1%s_%s(%s), vld1%s_%s(%s + %d),"
                     " %s); print_hex(idx++, &r, %d);"
                     % (T, stem, q, suf, q, suf, P, q, suf, P, n, sc,
                        total),
                     ["%s%s_n_%s" % (stem, q, suf)])
        if kl in ("s", "u"):
            ET = ELEM_C[(kl, es * 2 if es < 8 else 8)]
            case("%s e = vaddlv%s_%s(vld1%s_%s(%s));"
                 " print_hex(idx++, &e, (int)sizeof e);"
                 % (ET, q, suf, q, suf, P),
                 ["vaddlv%s_%s" % (q, suf)])
        if kl in ("s", "u") and total == 8 and es < 8:
            TQ = vec_type(kl, es * 2, n)
            asuf = "%s%d" % (kl, es * 16)
            AE = ELEM_C[(kl, es * 2)]
            for stem in ("vmlal", "vmlsl"):
                case("%s r = %s_n_%s(vld1q_%s((const %s *)sb.b),"
                     " vld1_%s(%s), %s); print_hex(idx++, &r, 16);"
                     % (TQ, stem, suf, asuf, AE, suf, P, sc),
                     ["%s_n_%s" % (stem, suf)])
            case("%s r = vmull_high_n_%s(vld1q_%s(%s), %s);"
                 " print_hex(idx++, &r, 16);"
                 % (TQ, suf, suf, P, sc), ["vmull_high_n_%s" % suf])
        if kl in ("s", "u") and total == 16 and es < 8:
            TQ = vec_type(kl, es * 2, n // 2)
            asuf = "%s%d" % (kl, es * 16)
            AE = ELEM_C[(kl, es * 2)]
            for stem in ("vmlal", "vmlsl"):
                case("%s r = %s_high_n_%s(vld1q_%s((const %s *)sb.b),"
                     " vld1q_%s(%s + %d), %s);"
                     " print_hex(idx++, &r, 16);"
                     % (TQ, stem, suf, asuf, AE, suf, P, n, sc),
                     ["%s_high_n_%s" % (stem, suf)])
    for kl in ("f",):
        for es in (4, 8):
            suf = "%s%d" % (kl, es * 8)
            E = ELEM_C[(kl, es)]
            TD = vec_type(kl, es, 8 // es)
            TQ = vec_type(kl, es, 16 // es)
            P = "(const %s *)sb.b" % E
            nd = 8 // es
            nq = 16 // es
            for stem in ("vfma", "vfms"):
                case("%s r = %s_%s(vld1_%s(%s), vld1_%s(%s + %d),"
                     " vld1_%s(%s + %d)); print_hex(idx++, &r, 8);"
                     % (TD, stem, suf, suf, P, suf, P, nd, suf, P,
                        2 * nd),
                     ["%s_%s" % (stem, suf)])
                case("%s r = %sq_%s(vld1q_%s(%s), vld1q_%s(%s + %d),"
                     " vld1q_%s(%s + %d)); print_hex(idx++, &r, 16);"
                     % (TQ, stem, suf, suf, P, suf, P, nq, suf, P,
                        2 * nq),
                     ["%sq_%s" % (stem, suf)])
                case("%s r = %s_lane_%s(vld1_%s(%s), vld1_%s(%s + %d),"
                     " vld1_%s(%s + %d), %d);"
                     " print_hex(idx++, &r, 8);"
                     % (TD, stem, suf, suf, P, suf, P, nd, suf, P,
                        2 * nd, nd - 1),
                     ["%s_lane_%s" % (stem, suf)])
                case("%s r = %s_laneq_%s(vld1_%s(%s), vld1_%s(%s + %d),"
                     " vld1q_%s(%s + %d), %d);"
                     " print_hex(idx++, &r, 8);"
                     % (TD, stem, suf, suf, P, suf, P, nd, suf, P,
                        2 * nd, nq - 1),
                     ["%s_laneq_%s" % (stem, suf)])
                case("%s r = %sq_lane_%s(vld1q_%s(%s), vld1q_%s(%s + %d),"
                     " vld1_%s(%s + %d), %d);"
                     " print_hex(idx++, &r, 16);"
                     % (TQ, stem, suf, suf, P, suf, P, nq, suf, P,
                        2 * nq, nd - 1),
                     ["%sq_lane_%s" % (stem, suf)])
                case("%s r = %sq_laneq_%s(vld1q_%s(%s),"
                     " vld1q_%s(%s + %d), vld1q_%s(%s + %d), %d);"
                     " print_hex(idx++, &r, 16);"
                     % (TQ, stem, suf, suf, P, suf, P, nq, suf, P,
                        2 * nq, nq - 1),
                     ["%sq_laneq_%s" % (stem, suf)])
    pre = ["/* generated by tools/neon_gen.py; do not edit */",
           '#include "neon_probe.h"', "",
           "static union { unsigned char b[1024]; long long a; } sb, ob;",
           "static int idx;", ""]
    def wrap(cases):
        return pre + ["int main(void) {",
                      "    fill_bytes(sb.b, (int)sizeof sb.b, 7);"] + \
            cases + ["    return fails != 0;", "}"]
    return ("\n".join(wrap(both)) + "\n",
            "\n".join(wrap(tconly)) + "\n")


def main():
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
    op_index = build()
    build_probes()
    tables = emit_tables(op_index)
    gen_path = os.path.join(root, "vendor-tinycc", "arm64-gen.c")
    src = open(gen_path).read()
    begin = src.index("/* NEON_TABLES_BEGIN")
    end = src.index("/* NEON_TABLES_END */")
    end = src.index("\n", end) + 1
    block = ("/* NEON_TABLES_BEGIN (generated by tools/neon_gen.py; "
             "do not edit) */\n" + tables + "\n/* NEON_TABLES_END */\n")
    open(gen_path, "w").write(src[:begin] + block + src[end:])
    header, names = emit_header()
    open(os.path.join(root, "vendor-tinycc", "include", "arm_neon.h"),
         "w").write(header)
    # probes grouped by area
    areas = {}
    for area, code in CASES:
        areas.setdefault(area, []).append(code)
    for area, code in CASES_TCC:
        areas.setdefault(area + "_tcconly", []).append(code)
    pdir = os.path.join(root, "tests", "neon", "probes")
    # drop stale generated probes (areas that lost all their cases
    # would otherwise linger); hand-written probes are kept
    keep = {"neon_ldst.c", "neon_calls.c", "neon_operators.c",
            "neon_compositions.c", "neon_sweep.c",
            "neon_sweep_tcconly.c"}
    sw_both, sw_tcc = emit_sweep(header)
    open(os.path.join(pdir, "neon_sweep.c"), "w").write(sw_both)
    open(os.path.join(pdir, "neon_sweep_tcconly.c"), "w").write(sw_tcc)
    for fn in os.listdir(pdir):
        if fn.startswith("neon_") and fn.endswith(".c") \
                and fn not in keep:
            os.unlink(os.path.join(pdir, fn))
    import re as _re
    for area, codes in areas.items():
        idxs = [int(_re.search(r"case_(\d+)", c).group(1))
                for c in codes]
        body = ["/* generated by tools/neon_gen.py; do not edit */",
                '#include "neon_probe.h"', ""] + codes + \
            ["int main(void) {"] + \
            ["    case_%d();" % i for i in idxs] + \
            ["    return fails != 0;", "}"]
        open(os.path.join(pdir, "neon_%s.c" % area), "w").write(
            "\n".join(body) + "\n")
    print("ops per family:", {k: len(v) for k, v in op_index.items()})
    print("table rows:", {k: len(v) for k, v in ROWS.items()})
    print("macros:", len(MACROS), "cases:", len(CASES))
    print("assembler-rejected shapes:", len(FAILS))
    for f in FAILS:
        print("  FAIL", f)

if __name__ == "__main__":
    main()
