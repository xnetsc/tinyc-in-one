#!/usr/bin/env python3
"""Generate the x86_64 SIMD builtin surface from one operation
spec (tools/x86_spec.py).

Outputs:
  - the opcode table block inside vendor-tinycc/x86_64-gen.c
    (between the X86_VEC_TABLES markers),
  - the intrinsic headers in vendor-tinycc/include/,
  - differential probes in tests/x86/probes/.

Encodings are not written by hand: for every (operation, lane
shape) pair the generator assembles the instruction with the
system assembler at increasing feature levels and diffs the
encodings of register-allocation variants to learn which bits
of the encoding belong to which operand.  A pair the ISA does
not define simply fails to assemble and is excluded from the
tables, the headers and the probes (and listed on stdout).
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from x86_spec import OPS, parse_types

GEN_C = os.path.join(ROOT, "vendor-tinycc", "x86_64-gen.c")
INCLUDE = os.path.join(ROOT, "vendor-tinycc", "include")
PROBES = os.path.join(ROOT, "tests", "x86", "probes")

# feature levels in dependency order: (TFEAT name, gas extension)
LEVELS = [("SSE", "sse"), ("SSE2", "sse2"), ("SSSE3", "ssse3"),
          ("SSE41", "sse4.1"), ("SSE42", "sse4.2"), ("AVX", "avx"),
          ("AVX2", "avx2"), ("FMA", "fma"), ("F16C", "f16c"),
          ("AVX512F", "avx512f"), ("AVX512CD", "avx512cd"),
          ("AVX512DQ", "avx512dq"), ("AVX512BW", "avx512bw"),
          ("AVX512VL", "avx512vl"), ("AVX512VBMI", "avx512vbmi"),
          ("AVX512VNNI", "avx512_vnni")]
LEVEL_IDX = {name: i for i, (name, _) in enumerate(LEVELS)}

KIND_C = {"SAME": "XVK_SAME", "CVT": "XVK_CVT", "CVT2": "XVK_CVT2",
          "SCALAR": "XVK_SCALAR", "BYTES": "XVK_BYTES",
          "MASK": "XVK_MASK"}
FORM_C = {n: "XVF_" + n for n in
          ["D_B", "D_B_ACC", "D_A_B", "D_B_IMM", "D_A_B_IMM",
           "D_A_B_S4", "DACC_A_B", "DACC_A_BS", "DACC_B_IMM", "COMI",
           "PBLENDVB", "RM_IMM", "DACC3_A_B", "DACC2_A_B",
           "D2_A1", "PTEST"]}
FLAG_C = {"SCALAR2": "XVFL_SCALAR2", "EVRND": "XVFL_EVRND",
          "OFL": "XVFL_OFL", "KSEL": "XVFL_KSEL",
          "COUNT": "XVFL_COUNT", "SCALAR1": "XVFL_SCALAR1",
          "PHSAE": "XVFL_PHSAE", "EVSAE": "XVFL_EVSAE"}

# every table family, in emission order; the masked families
# carry the writemask forms: a trailing 2 is the merging
# (passthrough) variant, a trailing i an immediate operand
FAMS = ("op1", "op1i", "op1m", "op1m2", "op1mi", "op1mi2",
        "op2", "op2i", "op3", "op3i", "op3m", "op3m2",
        "op3mi", "op3mi2", "opm", "opm2", "opmi", "opmi2")
# base family -> (zeroing masked family, merging masked family)
MASKED_OF = {"op1": ("op1m", "op1m2"),
             "op1i": ("op1mi", "op1mi2"),
             "op1m": (None, "op1m2"),
             "op2": ("opm", "opm2"),
             "op2i": ("opmi", "opmi2"),
             "op3": ("op3m", "op3m2"),
             "op3i": ("op3mi", "op3mi2")}

# ------------------------------------------------------------------
# lane shapes, matching the backend: a shape id packs the
# element code in bits [4:2] and the total size index in [1:0]
# ------------------------------------------------------------------
EC = {"B": 0, "H": 1, "S": 2, "D": 3, "F": 4, "DF": 5}

def shape_id(es, fl, tot):
    if fl:
        ec = EC["F"] if es == 4 else EC["DF"]
    else:
        ec = {1: EC["B"], 2: EC["H"], 4: EC["S"], 8: EC["D"]}[es]
    return (ec << 2) | {8: 0, 16: 1, 32: 2, 64: 3}[tot]

def entry_shapes(e):
    """the deduplicated (es, fl, tot) shapes of an entry"""
    out = []
    for kind, es in parse_types(e["types"]):
        fl = kind == "f"
        for tot in e["totals"]:
            if tot < es * 2 and not (tot == es):
                continue
            sh = (es, fl, tot)
            if sh not in out:
                out.append(sh)
    return out

# ------------------------------------------------------------------
# assembler driver
# ------------------------------------------------------------------
def _arch_lines(lvl):
    lines = [".arch generic64"]
    for _, ext in LEVELS[:lvl + 1]:
        lines.append(".arch ." + ext)
    return lines

def _run_as(texts, lvl):
    src = "\n".join(_arch_lines(lvl)) + \
        "\n.intel_syntax noprefix\n.text\n" + "\n".join(texts) + "\n"
    with open("/tmp/x86_gen.s", "w") as f:
        f.write(src)
    r = subprocess.run(["as", "-o", "/tmp/x86_gen.o", "/tmp/x86_gen.s"],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return None
    out = subprocess.run(["objdump", "-d", "/tmp/x86_gen.o"],
                         capture_output=True, text=True).stdout
    res = []
    for line in out.splitlines():
        m = re.match(r"\s*[0-9a-f]+:\t([0-9a-f ]+)\t", line)
        if m:
            res.append(bytes(int(x, 16) for x in m.group(1).split()))
    return res

_asm_cache = {}

def assemble(texts, lvl):
    """assemble a list of instruction texts; a list of bytes
       objects, with None for texts that do not assemble"""
    key = (lvl, tuple(texts))
    if key in _asm_cache:
        return _asm_cache[key]
    res = _run_as(texts, lvl)
    if res is None or len(res) != len(texts):
        # a template is rejected when any render fails; check
        # the base render first so a wholesale rejection costs
        # one extra call, not one per render
        base = _run_as([texts[0]], lvl)
        if not base:
            res = [None] * len(texts)
            _asm_cache[key] = res
            return res
        res = []
        for t in texts:
            one = _run_as([t], lvl)
            res.append(one[0] if one else None)
    _asm_cache[key] = res
    return res

# ------------------------------------------------------------------
# encoding derivation
# ------------------------------------------------------------------
GPR64 = ["rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi"] + \
        ["r%d" % i for i in range(8, 16)]
GPR32 = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"] + \
        ["r%dd" % i for i in range(8, 16)]
GPR16 = ["ax", "cx", "dx", "bx", "sp", "bp", "si", "di"] + \
        ["r%dw" % i for i in range(8, 16)]
GPR8 = ["al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil"] + \
       ["r%db" % i for i in range(8, 16)]
VEC_BASE = {1: 9, 2: 10, 3: 11, 4: 12}   # by role number

def role_map(e, tpl):
    """placeholder -> (role, file); file is 'vec', 'gpr' or 'k'.
       The role numbers match the backend's patch codes and the
       register each backend hook passes in that role."""
    fam = e["fam"]
    m = {}
    def has(p):
        return ("{%s}" % p) in tpl
    if fam in ("op1", "op1i", "op1m", "op1m2", "op1mi", "op1mi2"):
        if has("Kd"):
            m["Kd"] = (1, "k")
        if has("D"):
            m["D"] = (1, "vec")
        for g in ("G", "Gd", "Gw", "Gb"):
            if has(g):
                m[g] = (1, "gpr") if not has("D") else (3, "gpr")
        if has("A"):
            m["A"] = (3, "vec")
        if has("K"):
            m["K"] = (5, "k")
        if has("Kz"):
            m["Kz"] = (5, "k")
    elif fam in ("op2", "op2i"):
        if has("Kd"):
            m["Kd"] = (1, "k")
        if has("D"):
            m["D"] = (1, "vec")
        if has("A"):
            # a mask-valued entry has no {D}: its first
            # operand still takes the A role the backend's
            # mask path passes (x86_emit(e, 2, ra, rb, ...))
            m["A"] = (2, "vec") if has("D") or has("Kd") \
                else (3, "vec")
        if has("B"):
            m["B"] = (3, "vec")
        if has("X"):
            # the shift-count operand: the same register as B,
            # always rendered as an xmm register
            m["X"] = (3, "vec")
        for g in ("G", "Gd", "Gw", "Gb"):
            if has(g):
                m[g] = (3, "gpr")
        if has("K"):
            m["K"] = (5, "k")
        if has("Kz"):
            m["Kz"] = (5, "k")
    else:  # op3, op3i, opm and their masked variants
        if has("Kd"):
            m["Kd"] = (1, "k")
        if has("D"):
            m["D"] = (1, "vec")
        if has("A"):
            m["A"] = (2, "vec")
        if has("B"):
            m["B"] = (3, "vec")
        if has("X"):
            m["X"] = (3, "vec")
        if has("S"):
            m["S"] = (4, "vec")
        for g in ("G", "Gd", "Gw", "Gb"):
            if has(g):
                m[g] = (3, "gpr")
        if has("K"):
            m["K"] = (5, "k")
        if has("Kz"):
            m["Kz"] = (5, "k")
    return m

def _render(tpl, assign, imm, es, tot, dtot=None):
    out = tpl
    out = out.replace("{ss}", {1: "b", 2: "w", 4: "d", 8: "q"}[es])
    out = out.replace("{fs}", "ps" if es == 4 else "pd")
    out = out.replace("{sf}", "ss" if es == 4 else "sd")
    for ph, val in assign.items():
        if ph in ("D", "A", "B", "S"):
            w = (dtot or tot) if ph == "D" else tot
            name = ("xmm" if w <= 16 else
                    "ymm" if w == 32 else "zmm") + str(val)
            out = out.replace("{%s}" % ph, name)
        elif ph == "X":
            out = out.replace("{X}", "xmm" + str(val))
        elif ph == "G":
            out = out.replace("{G}", GPR64[val])
        elif ph == "Gd":
            out = out.replace("{Gd}", GPR32[val])
        elif ph == "Gw":
            out = out.replace("{Gw}", GPR16[val])
        elif ph == "Gb":
            out = out.replace("{Gb}", GPR8[val])
        elif ph == "K":
            out = out.replace("{K}", "{k%d}" % val)
        elif ph == "Kz":
            out = out.replace("{Kz}", "{k%d}{z}" % val)
        elif ph == "Kd":
            out = out.replace("{Kd}", "k%d" % val)
    out = out.replace("{i}", str(imm))
    return out

def derive(e, tpl, es, fl, tot, lvl):
    """derive (template bytes, patch list, imm_pos) for one
       template at one shape and feature level, or None"""
    roles = role_map(e, tpl)
    if not roles and "{i}" not in tpl:
        return None
    base = {}
    for ph, (role, f) in roles.items():
        base[ph] = VEC_BASE[role] if f == "vec" else 13 if f == "gpr" else 7
    imm_lo = e.get("imm", (0, 0))[0]
    imm_hi = e.get("imm", (0, 0))[1]
    has_imm = "{i}" in tpl
    # group placeholders by (role, file): variants flip one
    # role's value at a time
    groups = {}
    for ph, (role, f) in roles.items():
        groups.setdefault((role, f), []).append(ph)
    dtot = e.get("otot") or tot
    texts = [_render(tpl, base, imm_lo, es, tot, dtot)]
    variants = []  # (role, bit, text index)
    for (role, f), phs in sorted(groups.items()):
        bits = [0, 1, 2] if f == "k" else [0, 1, 2, 3]
        for bit in bits:
            a2 = dict(base)
            for ph in phs:
                a2[ph] = base[ph] ^ (1 << bit)
            variants.append((role, bit, len(texts)))
            texts.append(_render(tpl, a2, imm_lo, es, tot,
                                 dtot))
    if has_imm and not e.get("evrnd"):
        texts.append(_render(tpl, base, imm_hi, es, tot, dtot))
    encs = assemble(texts, lvl)
    if any(x is None for x in encs):
        return None
    b0 = encs[0]
    if len(b0) > 12:
        return None
    # A variant may come back in the short 2-byte VEX form
    # (0xc5) when the base render needed the long form (0xc4):
    # that happens exactly when the flipped bit is bit 3 of the
    # r/m-field role, the one field the short form cannot
    # express.  Its patch is then synthesized from the fixed
    # C4 prefix layout (B' is bit 5 of prefix byte 1, inverted).
    synth = []
    for role, bit, ti in variants:
        bx = encs[ti]
        if len(bx) != len(b0):
            if bit == 3 and b0[0] == 0xC4 and bx[0] == 0xC5:
                synth.append(role)
            else:
                return None
    if len(set(synth)) > 1:
        return None
    patches = []
    seen = set()
    for role, bit, ti in variants:
        bx = encs[ti]
        if len(bx) != len(b0):
            continue  # the synthesized C5 case, handled below
        diff = [(i, b0[i] ^ bx[i]) for i in range(len(b0)) if b0[i] != bx[i]]
        bits = [(i, x) for i, x in diff if x]
        if not bits or any(x & (x - 1) for _, x in bits):
            return None
        # a role may own several fields (a destination that is
        # also the vvvv source flips one bit in each of them);
        # every flipped bit becomes its own patch
        ph0 = groups[[k for k in groups if k[0] == role][0]][0]
        base_bit = (base[ph0] >> bit) & 1
        var_bit = ((base[ph0] ^ (1 << bit)) >> bit) & 1
        for pos, x in bits:
            pbit = x.bit_length() - 1
            stored_base = (b0[pos] >> pbit) & 1
            stored_var = (bx[pos] >> pbit) & 1
            if stored_base == base_bit and stored_var == var_bit:
                neg = 0
            elif stored_base == (base_bit ^ 1) and \
                    stored_var == (var_bit ^ 1):
                neg = 1
            else:
                return None
            shift = pbit - bit
            if shift < -8 or shift > 7:
                return None
            code = role | (neg << 3) | ((shift & 0xF) << 4)
            key = (pos, 1 << pbit)
            if key in seen:
                return None
            seen.add(key)
            patches.append((pos, 1 << pbit, code))
    if len(patches) > 19:
        return None
    imm_pos = 0xFF
    if e.get("evrnd") or (not has_imm and e["fam"] in
                          ("op1i", "op2i", "op3i", "op1mi",
                           "op1mi2", "op3mi", "op3mi2", "opmi",
                           "opmi2") and
                          e.get("imm", (0, 0))[1] >
                          e.get("imm", (0, 0))[0]):
        # an immediate the encoding does not carry (the
        # embedded-rounding selector of an exact operation):
        # accepted and range-checked, written nowhere
        imm_pos = 0xFE
    elif has_imm:
        bx = encs[-1]
        diff = [i for i in range(len(b0)) if b0[i] != bx[i]]
        if len(diff) != 1:
            return None
        imm_pos = diff[0]
        if b0[imm_pos] != imm_lo or bx[imm_pos] != imm_hi:
            return None
    if "{S}" in tpl:
        # the is4 byte: the position of the S role's patches;
        # it is the trailing imm8 byte (the blendv forms) --
        # an S role patched anywhere else is an ordinary
        # operand field (the FMA vvvv source, the two-source
        # permutes' r/m source).  The is4 byte is identified
        # exactly: every S patch lies in the last byte's high
        # nibble (the register number lives there alone)
        spos = [p for p, _, c in patches if (c & 7) == 4]
        if not spos:
            return None
        sp = [(p, m) for p, m, c in patches if (c & 7) == 4]
        if all(p == len(b0) - 1 and not (m & 0x0F)
               for p, m in sp):
            imm_pos = spos[0]
    # the modrm byte: the last byte, or the byte before the
    # immediate when the template carries one
    modrm_pos = imm_pos - 1 if 0 <= imm_pos < 0xFE else len(b0) - 1
    for role in set(synth):
        # the role must own the r/m field (the low bits of the
        # modrm byte) for the B' synthesis to apply
        if not any((c & 7) == role and p == modrm_pos and
                   mk in (1, 2, 4) for p, mk, c in patches):
            return None
        if any((c & 7) == role and mk == 0x20 and p == 1
               for p, mk, c in patches):
            return None
        patches.append((1, 0x20, role | 0x08 | (2 << 4)))
    tmpl = bytearray(b0)
    if has_imm and not e.get("evrnd") and "{S}" not in tpl:
        tmpl[imm_pos] = 0
    return bytes(tmpl), patches, imm_pos

def form_of(e, tpl):
    if "form" in e:
        return e["form"]
    fam = e["fam"]
    if fam in ("op1", "op1m", "op1m2"):
        return "D_B"
    if fam in ("op1i", "op1mi", "op1mi2"):
        return "RM_IMM" if "{D}" not in tpl else "D_B_IMM"
    if fam in ("op2", "opm", "opm2"):
        return "D_A_B" if "{A}" in tpl else "D_B_ACC"
    if fam in ("op2i", "opmi", "opmi2"):
        if "{D}" not in tpl:
            return "RM_IMM"
        return "D_A_B_IMM" if "{A}" in tpl else "D_B_ACC"
    if fam in ("op3", "op3m", "op3m2"):
        if "{S}" in tpl:
            return "D_A_B_S4"
        return "D_A_B" if "{A}" in tpl else "PBLENDVB"
    if fam in ("op3i", "op3mi", "op3mi2"):
        return "D_A_B_IMM"
    raise AssertionError(fam)

# ------------------------------------------------------------------
# operation indices: grouped by (family, operation) in spec
# order; the backend's own enums must match the spec prefix
# ------------------------------------------------------------------
def op_indices():
    idx = {}
    order = {}
    for e in OPS:
        fam = e["fam"]
        order.setdefault(fam, [])
        if e["op"] not in order[fam]:
            order[fam].append(e["op"])
        idx[(fam, e["op"])] = order[fam].index(e["op"])
    return idx, order

def check_c_enums(order):
    src = open(GEN_C).read()
    want = {"op1i": "XOP1I_", "op2": "XOP2_", "op2i": "XOP2I_"}
    for fam, prefix in want.items():
        m = re.search(r"enum \{([^}]*)\};\s*\n(?:enum \{[^}]*\};\s*\n)?",
                      src)
        names = []
        for mm in re.finditer(r"enum \{([^}]*)\};", src):
            body = mm.group(1)
            if prefix in body:
                names = [t.strip() for t in body.split(",")
                         if t.strip().startswith(prefix)]
                break
        c_names = [n[len(prefix):] for n in names]
        spec_names = order.get(fam, [])[:len(c_names)]
        if c_names != spec_names:
            raise SystemExit(
                "spec order for %s %s does not match the C enum %s"
                % (fam, spec_names, c_names))

# ------------------------------------------------------------------
# table generation
# ------------------------------------------------------------------
def masked_asm(tpl, variant, kind):
    """the writemask form of a template: the zeroing variant
       carries {k1}{z} on the destination, the merging variant
       {k1}; a mask-valued entry (its destination is {Kd}) only
       has the merging variant, the writemask on the mask
       destination; a template already carrying {Kz} (the op1m
       family) only has the merging variant"""
    if "{Kz}" in tpl:
        return tpl.replace("{Kz}", "{K}") if variant == "m" else None
    if kind == "MASK":
        if variant != "m" or "{Kd}" not in tpl:
            return None
        return tpl.replace("{Kd}", "{Kd} {K}", 1)
    if "{D}" not in tpl:
        # an accumulator template (the two-source permutes):
        # the first operand is the destination, the writemask
        # rides on it
        if "{A}" in tpl:
            return tpl.replace("{A}", "{A} {K}" if variant == "m"
                               else "{A} {Kz}", 1)
        return None
    return tpl.replace("{D}", "{D} {K}" if variant == "m"
                       else "{D} {Kz}", 1)


def gen_tables():
    idx, order = op_indices()
    check_c_enums(order)
    srows = []          # structured rows, formatted at the end
    excluded = []
    shipped = set()     # (OPS index, es, fl, tot) with a table row
    shipped_m = set()   # (OPS index, es, fl, tot, variant)
    midx = {}           # (OPS index, variant) -> masked op index
    # masked families share their table with any native entries
    # of the same family (the opm blend family, the op1m
    # compress family): masked op indices continue after the
    # native op indices so the two never collide
    mcount = {}
    for _f, _op in idx:
        mcount[_f] = mcount.get(_f, 0) + 1

    def width_ok(tmpl, tot):
        # the assembler promotes a 128/256-bit EVEX form to the
        # 512-bit encoding when the arch lacks the VL extension;
        # such a row computes across the full register and is
        # only a fallback for shapes with no true-width form
        if tmpl[0] == 0x62:
            return (tmpl[3] >> 5) & 3 == {8: 0, 16: 0, 32: 1, 64: 2}[tot]
        if tmpl[0] == 0xC4:
            return (tmpl[2] >> 2) & 1 == (1 if tot == 32 else 0)
        if tmpl[0] == 0xC5:
            return (tmpl[1] >> 2) & 1 == (1 if tot == 32 else 0)
        return True

    def derive_row(e, fam, opidx, es, fl, tot):
        sid = shape_id(es, fl, tot)
        opc = (opidx << 5) | sid
        start = LEVEL_IDX[e.get("feat", "SSE")]
        if tot == 32:
            start = max(start, LEVEL_IDX["AVX"])
        elif tot == 64:
            start = max(start, LEVEL_IDX["AVX512F"])
        fallback = None
        for lvl in range(start, len(LEVELS)):
            for tpl in e["asm"]:
                got = derive(e, tpl, es, fl, tot, lvl)
                if got:
                    tmpl, patches, imm_pos = got
                    row = {"fam": fam, "opc": opc, "es": es,
                           "fl": fl, "tot": tot, "tpl": tpl,
                           "lvl": lvl, "tmpl": tmpl,
                           "patches": patches, "imm_pos": imm_pos,
                           "e": e}
                    if width_ok(tmpl, tot):
                        return row
                    if fallback is None:
                        fallback = row
        return fallback

    for ei, e in enumerate(OPS):
        fam = e["fam"]
        if fam not in FAMS:
            continue
        if not e["asm"]:
            continue  # alias entry; handled after the main pass
        opidx = idx[(fam, e["op"])]
        if fam == "opm":
            e = dict(e, asm=[masked_asm(t, "m",
                                         e.get("kind", "SAME"))
                             for t in e["asm"]])
        base_shapes = []
        for es, fl, tot in entry_shapes(e):
            r = derive_row(e, fam, opidx, es, fl, tot)
            if not r:
                excluded.append("%s %s es=%d fl=%d tot=%d"
                                % (fam, e["op"], es, fl, tot))
                continue
            srows.append(r)
            shipped.add((ei, es, fl, tot))
            base_shapes.append((es, fl, tot))
        if not e.get("m") or fam not in MASKED_OF:
            continue
        kind = e.get("kind", "SAME")
        for variant, mfam in zip(("z", "m"), MASKED_OF[fam]):
            if mfam is None:
                continue
            me = dict(e, fam=mfam)
            got_any = False
            for es, fl, tot in base_shapes:
                sid = shape_id(es, fl, tot)
                key = (ei, variant)
                if key not in midx:
                    midx[key] = mcount.get(mfam, 0)
                    mcount[mfam] = midx[key] + 1
                mopc = (midx[key] << 5) | sid
                # derive from the masked templates, in spec order
                start = LEVEL_IDX[e.get("feat", "SSE")]
                if tot == 32:
                    start = max(start, LEVEL_IDX["AVX"])
                elif tot == 64:
                    start = max(start, LEVEL_IDX["AVX512F"])
                start = max(start, LEVEL_IDX["AVX512F"])
                r = None
                rfallback = None
                for lvl in range(start, len(LEVELS)):
                    for tpl in e["asm"]:
                        mtpl = masked_asm(tpl, variant, kind)
                        if mtpl is None:
                            continue
                        got = derive(me, mtpl, es, fl, tot, lvl)
                        if got:
                            tmpl, patches, imm_pos = got
                            cand = {"fam": mfam, "opc": mopc,
                                    "es": es, "fl": fl, "tot": tot,
                                    "tpl": mtpl, "lvl": lvl,
                                    "tmpl": tmpl,
                                    "patches": patches,
                                    "imm_pos": imm_pos, "e": me,
                                    "base_ei": ei}
                            if width_ok(tmpl, tot):
                                r = cand
                                break
                            if rfallback is None:
                                rfallback = cand
                    if r:
                        break
                if r is None:
                    r = rfallback
                if r:
                    srows.append(r)
                    shipped_m.add((ei, es, fl, tot, variant))
                    got_any = True
            # an index with no rows is harmless: no header or
            # probe refers to it

    for ei, e in enumerate(OPS):
        if e["fam"] not in FAMS or e["asm"]:
            continue
        for ej, o in enumerate(OPS):
            if o["fam"] == e["fam"] and o["op"] == e["op"] \
                    and o["asm"]:
                for es, fl, tot in entry_shapes(e):
                    if (ej, es, fl, tot) in shipped:
                        shipped.add((ei, es, fl, tot))

    # VL fixup: a sub-512 row that first assembles at the VL
    # level may also need the wider feature its 512-bit sibling
    # required (a ymm byte op needs AVX512BW as well as VL)
    wide_feats = {}
    for r in srows:
        if r["tot"] == 64:
            key = (r["e"]["fam"], r["opc"] >> 5, r["es"], r["fl"],
                   r.get("base_ei"))
            wide_feats[key] = LEVELS[r["lvl"]][0]
    for r in srows:
        r["feat"] = LEVELS[r["lvl"]][0]
        if r["tot"] < 64 and r["feat"] == "AVX512VL":
            key = (r["e"]["fam"], r["opc"] >> 5, r["es"], r["fl"],
                   r.get("base_ei"))
            sib = wide_feats.get(key)
            if sib in ("AVX512BW", "AVX512DQ", "AVX512CD",
                       "AVX512VBMI", "AVX512VNNI"):
                r["feat"] = sib + " | TFEAT_AVX512VL"

    rows = {fam: [] for fam in FAMS}
    for r in srows:
        e = r["e"]
        kind = KIND_C[e.get("kind", "SAME")]
        form = FORM_C[form_of(e, r["tpl"])]
        flags = list(e.get("flags", ()))
        if e.get("evrnd") and "EVRND" not in flags:
            flags.append("EVRND")
        flag_s = " | ".join(FLAG_C[f] for f in flags) or "0"
        imm_lo, imm_hi = e.get("imm", (0, 0))
        if e.get("imm_mode") == "index":
            imm_hi = min(imm_hi, r["tot"] // r["es"] - 1)
        elif e.get("imm_mode") == "mask":
            imm_hi = min(imm_hi, (1 << (r["tot"] // r["es"])) - 1)
        b_s = ", ".join("0x%02x" % b for b in r["tmpl"])
        p_s = ", ".join("%d, 0x%02x, 0x%02x" % p
                        for p in r["patches"])
        if p_s:
            p_s += ", "
        feat_s = " | ".join("TFEAT_" + f if not f.startswith("TFEAT")
                            else f for f in r["feat"].split(" | "))
        rows[r["fam"]].append(
            "    { .opc = %d, .es = %d, .kind = %s, .form = %s,\n"
            "      .oes = %d, .otot = %d, .feat = %s,\n"
            "      .flags = %s, .imm_lo = %d, .imm_hi = %d,\n"
            "      .imm_pos = 0x%02x, .n = %d,\n"
            "      .b = { %s },\n"
            "      .patch = { %s0xff } },"
            % (r["opc"], r["es"], kind, form, e.get("oes", 0),
               e.get("otot", 0), feat_s, flag_s,
               imm_lo, imm_hi, r["imm_pos"], len(r["tmpl"]), b_s,
               p_s))
    out = ["/* X86_VEC_TABLES_BEGIN (generated by tools/x86_gen.py;"
           " do not edit) */"]
    for fam in FAMS:
        out.append("static const XVop x86_%s_table[] = {" % fam)
        out.extend(rows[fam])
        out.append("    { .opc = -1 }")
        out.append("};")
    out.append("/* X86_VEC_TABLES_END */")
    src = open(GEN_C).read()
    if "/* X86_VEC_TABLES_BEGIN" not in src:
        raise SystemExit("table markers not found in x86_64-gen.c")
    new = re.sub(r"/\* X86_VEC_TABLES_BEGIN.*?\*/.*?/\* X86_VEC_TABLES_END \*/",
                 "\n".join(out), src, flags=re.S)
    if new != src:
        open(GEN_C, "w").write(new)
    return excluded, shipped, shipped_m, midx, idx


# ------------------------------------------------------------------
# intrinsic naming and header generation
# ------------------------------------------------------------------
ELEM_C = {
    ("s", 1): "signed char", ("s", 2): "short", ("s", 4): "int",
    ("s", 8): "long long",
    ("u", 1): "unsigned char", ("u", 2): "unsigned short",
    ("u", 4): "unsigned int", ("u", 8): "unsigned long long",
    ("f", 4): "float", ("f", 8): "double",
    ("x", 8): "long long",
}

def vec_type_name(kind, es, tot):
    if tot == 8:
        return "__m64"
    base = {16: "__m128", 32: "__m256", 64: "__m512"}[tot]
    if kind == "f":
        return base if es == 4 else base + "d"
    return base + "i"

def intrinsic_names(e, es, fl, tot):
    """the (name, kind, es, tot) tuples an entry variant yields
       at one shape; kind is the naming kind ('s', 'u', 'f', 'x')"""
    if "names" in e:
        return [n for n in e["names"] if n[3] == tot and n[2] == es]
    if not e.get("stem"):
        return []
    kind = "f" if fl else None
    if kind is None:
        kinds = {k for k, s in parse_types(e["types"]) if s == es}
        if not kinds:
            return []
        kind = sorted(kinds)[0]
    prefix = {8: "_mm_", 16: "_mm_", 32: "_mm256_",
              64: "_mm512_"}[tot]
    if fl:
        suffix = "ps" if es == 4 else "pd"
    elif e.get("signed_names"):
        suffix = ("epi" if kind == "s" else "epu") + str(es * 8)
    else:
        suffix = "epi" + str(es * 8)
    return [(prefix + e["stem"] + "_" + suffix, kind, es, tot)]

def header_for(e, tot):
    feat = e.get("feat", "SSE")
    if tot == 32:
        return {"FMA": "fmaintrin", "F16C": "f16cintrin",
                "AVX2": "avx2intrin"}.get(feat, "avxintrin")
    if tot == 64:
        return {"AVX512BW": "avx512bwintrin",
                "AVX512DQ": "avx512dqintrin",
                "AVX512CD": "avx512cdintrin",
                "AVX512VL": "avx512vlintrin",
                "AVX512VBMI": "avx512vbmiintrin",
                "AVX512VNNI": "avx512vnniintrin"}.get(
                    feat, "avx512fintrin")
    if feat.startswith("AVX512"):
        return "avx512vlintrin"
    return {"SSE": "xmmintrin", "SSE2": "emmintrin",
            "SSSE3": "tmmintrin", "SSE41": "smmintrin",
            "SSE42": "nmmintrin", "AVX": "avxintrin",
            "AVX2": "avx2intrin", "FMA": "fmaintrin",
            "F16C": "f16cintrin"}.get(feat, "xmmintrin")

HEADER_ORDER = ["xmmintrin", "emmintrin", "tmmintrin", "smmintrin",
                "nmmintrin", "avxintrin", "avx2intrin", "fmaintrin",
                "f16cintrin", "avx512fintrin", "avx512bwintrin",
                "avx512vlintrin", "avx512dqintrin", "avx512cdintrin",
                "avx512vbmiintrin", "avx512vnniintrin"]
HEADER_TYPES = {
    "xmmintrin": [
        "typedef float __m128 __attribute__((__vector_size__(16)));"],
    "emmintrin": [
        "typedef double __m128d __attribute__((__vector_size__(16)));",
        "typedef long long __m128i __attribute__((__vector_size__(16)));"],
    "avxintrin": [
        "typedef float __m256 __attribute__((__vector_size__(32)));",
        "typedef double __m256d __attribute__((__vector_size__(32)));",
        "typedef long long __m256i __attribute__((__vector_size__(32)));"],
    "avx512fintrin": [
        "typedef float __m512 __attribute__((__vector_size__(64)));",
        "typedef double __m512d __attribute__((__vector_size__(64)));",
        "typedef long long __m512i __attribute__((__vector_size__(64)));",
        "typedef unsigned char __mmask8;",
        "typedef unsigned short __mmask16;",
        "typedef unsigned int __mmask32;",
        "typedef unsigned long long __mmask64;"],
}
BUILTIN_OF_STYLE = {
    "op1": "__builtin_ia32_vop1", "op1i": "__builtin_ia32_vop1i",
    "op1m": "__builtin_ia32_vop1m", "op1m2": "__builtin_ia32_vop1m2",
    "op1mi": "__builtin_ia32_vop1mi",
    "op1mi2": "__builtin_ia32_vop1mi2",
    "op2": "__builtin_ia32_vop2",
    "op2i": "__builtin_ia32_vop2i", "op3": "__builtin_ia32_vop3",
    "op3i": "__builtin_ia32_vop3i", "op3m": "__builtin_ia32_vop3m",
    "op3m2": "__builtin_ia32_vop3m2",
    "op3mi": "__builtin_ia32_vop3mi",
    "op3mi2": "__builtin_ia32_vop3mi2",
    "opm": "__builtin_ia32_vopm", "opm2": "__builtin_ia32_vopm2",
    "opmi": "__builtin_ia32_vopmi", "opmi2": "__builtin_ia32_vopmi2",
}
ARGS_OF_STYLE = {
    "op1": "a", "op1i": "a, i", "op1m": "k, a",
    "op1m2": "src, k, a", "op1mi": "k, a, i",
    "op1mi2": "src, k, a, i", "op2": "a, b",
    "op2i": "a, b, i", "op3": "a, b, c", "op3i": "a, b, c, i",
    "op3m": "k, a, b, c", "op3m2": "src, k, a, b, c",
    "op3mi": "k, a, b, c, i", "op3mi2": "src, k, a, b, c, i",
    "opm": "k, a, b", "opm2": "src, k, a, b",
    "opmi": "k, a, b, i", "opmi2": "src, k, a, b, i",
    "splat": "a", "extract": "a, i",
}
# styles whose user argument order is (src, k, ...) while the
# builtin takes (k, src, ...)
MERGING_STYLES = ("op1m2", "op1mi2", "opm2", "opmi2",
                  "op3m2", "op3mi2")


def mask_type(es, tot):
    # the mask typedefs come in widths 8/16/32/64; a shorter
    # lane count uses the next width up (a four-lane mask is
    # an __mmask8)
    n = tot // es
    for w in (8, 16, 32, 64):
        if n <= w:
            return "__mmask%d" % w
    return "__mmask64"


def masked_name(name, variant):
    m = re.match(r"^(_mm\d*_)(.*)$", name)
    return m.group(1) + ("mask_" if variant == "m"
                         else "maskz_") + m.group(2)

def macro_for(e, idx, name, kind, es, tot, ei=None, midx=None,
              shipped_m=None):
    style = e.get("style")
    if style is None:
        return None
    fl = kind == "f"
    sid = shape_id(es, fl, tot)
    if e.get("kind") in ("CVT", "CVT2"):
        # the opcode's shape is the operand's; a convert's
        # name tuple carries the result element instead
        _t0 = e["types"].split()[0]
        sid = shape_id(int(_t0[1:]), _t0[0] == "f", tot)
    rt = vec_type_name(kind, es, tot)
    opidx = idx[(e["fam"], e["op"])]
    opc = (opidx << 5) | sid
    if style == "custom":
        body = e["macro"]
        mg = re.search(r"__builtin_ia32_(vop(?:\d+)?(?:mi?|m)2?)\(",
                       body)
        if mg and midx is not None:
            # a custom macro over a masked generic (the mask2 /
            # mask3 signature permutations) names the masked
            # family's opcode, which belongs to the plain entry
            # of the same op
            gen = mg.group(1)
            variant = "m" if gen.endswith("2") else "z"
            for oi, oe in enumerate(OPS):
                if oe["fam"] == e["fam"] and oe["op"] == e["op"] \
                        and (oi, variant) in midx and \
                        (shipped_m is None or
                         (oi, es, kind == "f", tot, variant)
                         in shipped_m):
                    opc = (midx[(oi, variant)] << 5) | sid
                    break
        body = body.replace("{OPC}", str(opc))
        body = body.replace("{opc}", str(sid))
        body = body.replace("{rt}", rt)
        body = body.replace("{et}", ELEM_C[(kind, es)])
        return "#define %s(%s) %s" % (name, e["args"], body)
    if e.get("kind") == "SCALAR":
        # the result is a scalar: an integer of the entry's
        # result size, or a float when OFL is set; the COMI
        # and PTEST forms return a plain int flag, their oes
        # field is a condition / flag selector, not a size
        if e.get("form") in ("COMI", "PTEST"):
            rt = "int"
            oes = es
        else:
            oes = e.get("oes", es)
        if "OFL" in e.get("flags", ()):
            rt = ELEM_C[("f", oes)]
        elif name.endswith(("_u32", "_u64")):
            # the _u32 / _u64 scalar converts return an
            # unsigned integer; a signed result type would
            # sign-extend the value whenever it is widened
            rt = ELEM_C[("u", oes)]
        else:
            rt = ELEM_C[("s", oes)]
    if e.get("kind") in ("CVT", "CVT2"):
        rt = vec_type_name(kind, e.get("oes", es),
                           e.get("otot") or tot)
    if e.get("kind") == "MASK":
        rt = mask_type(es, tot)
    bn = BUILTIN_OF_STYLE[style]
    args = ARGS_OF_STYLE[style]
    if style in MERGING_STYLES:
        parts = args.split(", ")
        call = ", ".join([parts[1], parts[0]] + parts[2:])
        return "#define %s(%s) %s(%d, %s, %s)" % (
            name, args, bn, opc, call, rt)
    if "imm_fixed" in e:
        args = args.replace(", i", "")
        call_args = args + ", " if args else ""
        return "#define %s(%s) %s(%d, %s%d, %s)" % (
            name, args, bn, opc, call_args, e["imm_fixed"], rt)
    if style in ("op1", "op1m", "splat"):
        return "#define %s(%s) %s(%d, %s, %s)" % (
            name, args, bn, opc, args, rt)
    if style == "extract":
        return "#define %s(%s) %s(%d, %s, %s)" % (
            name, args, bn, opc, args,
            ELEM_C[(kind if kind != "x" else "s", es)])
    return "#define %s(%s) %s(%d, %s, %s)" % (
        name, args, bn, opc, args, rt)

def macro_masked(e, mopidx, variant, name, kind, es, tot):
    """the header macro for a derived writemask variant: the
       zeroing form is (k, operands...), the merging form is
       (src, k, operands...); a mask-valued entry's masked form
       is (k, operands...) with a mask result"""
    fl = kind == "f"
    sid = shape_id(es, fl, tot)
    if e.get("kind") in ("CVT", "CVT2"):
        # the opcode's shape is the operand's; a convert's
        # name tuple carries the result element instead
        _t0 = e["types"].split()[0]
        sid = shape_id(int(_t0[1:]), _t0[0] == "f", tot)
    rt = vec_type_name(kind, es, tot)
    if e.get("kind") in ("CVT", "CVT2"):
        rt = vec_type_name(kind, e.get("oes", es),
                           e.get("otot") or tot)
    mfam = MASKED_OF[e["fam"]][0 if variant == "z" else 1]
    bn = "__builtin_ia32_v" + mfam
    opc = (mopidx << 5) | sid
    vargs = {"op1": "a", "op1m": "a", "op1i": "a, i",
             "op2": "a, b", "op2i": "a, b, i",
             "op3": "a, b, c", "op3i": "a, b, c, i"}[e["fam"]]
    if e.get("kind") == "MASK":
        rt = mask_type(es, tot)
        # the merging masked generics carry a passthrough
        # slot even for mask-valued entries (the backend
        # ignores it there); the mask fills it
        return "#define %s(k, %s) %s(%d, k, k, %s, %s)" % (
            name, vargs, bn, opc, vargs, rt)
    if variant == "z":
        return "#define %s(k, %s) %s(%d, k, %s, %s)" % (
            name, vargs, bn, opc, vargs, rt)
    if e.get("mpass"):
        # the writemask passthrough is the first operand
        # itself (the masked FMA forms): (a, k, rest...)
        first, rest = vargs.split(", ", 1)
        return "#define %s(%s, k, %s) %s(%d, k, %s, %s, %s)" % (
            name, first, rest, bn, opc, first, vargs, rt)
    return "#define %s(src, k, %s) %s(%d, k, src, %s, %s)" % (
        name, vargs, bn, opc, vargs, rt)


def header_for_masked(e, tot):
    if tot == 64:
        return header_for(e, 64)
    if e.get("feat") in ("FMA", "F16C"):
        return header_for(e, tot)
    return "avx512vlintrin"


def gen_headers(shipped, shipped_m, midx, idx):
    per = {h: [] for h in HEADER_ORDER}
    seen = set()
    for ei, e in enumerate(OPS):
        if e["fam"] == "hdr":
            targets = [(n, k, es, tot)
                       for (n, k, es, tot) in e.get("names", [])]
            for name, kind, es, tot in targets:
                if tot == 8 or name in seen:
                    continue
                seen.add(name)
                per[header_for(e, tot)].append(
                    macro_for(e, idx, name, kind, es, tot, ei,
                              midx, shipped_m))
            continue
        if not e.get("style"):
            continue
        for es, fl, tot in entry_shapes(e):
            if (ei, es, fl, tot) not in shipped or tot == 8:
                continue
            for name, kind, kes, ktot in intrinsic_names(e, es, fl, tot):
                if e["fam"] == "op1m":
                    # the family's base form is the zeroing
                    # masked form itself (compress / expand
                    # exist only masked in the ISA's model)
                    name = masked_name(name, "z")
                if name in seen:
                    continue
                seen.add(name)
                per[header_for(e, tot)].append(
                    macro_for(e, idx, name, kind, es, tot, ei,
                              midx, shipped_m))
            if not e.get("m") or e.get("style") == "custom" \
                    or "imm_fixed" in e:
                continue
            for variant in ("z", "m"):
                if (ei, es, fl, tot, variant) not in shipped_m:
                    continue
                for name, kind, kes, ktot in intrinsic_names(
                        e, es, fl, tot):
                    mname = masked_name(name, variant)
                    if mname in seen:
                        continue
                    seen.add(mname)
                    per[header_for_masked(e, tot)].append(
                        macro_masked(e, midx[(ei, variant)], variant,
                                     mname, kind, es, tot))
    written = []
    for hi, h in enumerate(HEADER_ORDER):
        lines = ["/* generated by tools/x86_gen.py; do not edit */",
                 "#ifndef TINYC_%s_H" % h.upper(),
                 "#define TINYC_%s_H" % h.upper(), ""]
        for prev in HEADER_ORDER[:hi]:
            if prev in written:
                lines.append('#include "%s.h"' % prev)
        if hi > 0 and written:
            lines.append("")
        lines.extend(HEADER_TYPES.get(h, []))
        if HEADER_TYPES.get(h):
            lines.append("")
        lines.extend(sorted(set(per[h])))
        lines.append("")
        lines.append("#endif")
        if per[h] or HEADER_TYPES.get(h):
            with open(os.path.join(INCLUDE, h + ".h"), "w") as f:
                f.write("\n".join(lines) + "\n")
            written.append(h)
    lines = ["/* generated by tools/x86_gen.py; do not edit */",
             "#ifndef TINYC_IMMINTRIN_H", "#define TINYC_IMMINTRIN_H",
             ""]
    for h in written:
        lines.append('#include "%s.h"' % h)
    lines += ["", "#endif"]
    with open(os.path.join(INCLUDE, "immintrin.h"), "w") as f:
        f.write("\n".join(lines) + "\n")
    return written

# ------------------------------------------------------------------
# differential probe generation
# ------------------------------------------------------------------
UNSIGNED_C = {"signed char": "unsigned char", "short": "unsigned short",
              "int": "unsigned int", "long long": "unsigned long long",
              "unsigned char": "unsigned char",
              "unsigned short": "unsigned short",
              "unsigned int": "unsigned int",
              "unsigned long long": "unsigned long long",
              "float": "unsigned int", "double": "unsigned long long"}

# scalar reference blocks: C statements over av[], bv[] (the
# operand element arrays), e_[] (the expected result), i, with
# {ES} {TOT} {IMM} substituted by the generator
REFBLOCKS = {
    "bitand":
        "for (i = 0; i < {TOT}; i++)\n"
        "    ((unsigned char *)e_)[i] = ((unsigned char *)av)[i] &\n"
        "                                ((unsigned char *)bv)[i];",
    "bitor":
        "for (i = 0; i < {TOT}; i++)\n"
        "    ((unsigned char *)e_)[i] = ((unsigned char *)av)[i] |\n"
        "                                ((unsigned char *)bv)[i];",
    "bitxor":
        "for (i = 0; i < {TOT}; i++)\n"
        "    ((unsigned char *)e_)[i] = ((unsigned char *)av)[i] ^\n"
        "                                ((unsigned char *)bv)[i];",
    "pshufb":
        "for (i = 0; i < {TOT}; i++) {\n"
        "    int blk_ = i & ~15;\n"
        "    ((unsigned char *)e_)[i] =\n"
        "        (((unsigned char *)bv)[i] & 0x80) ? 0 :\n"
        "        ((unsigned char *)av)[blk_ +\n"
        "                              (((unsigned char *)bv)[i] & 15)];\n"
        "}",
    "unpcklqdq":
        "for (i = 0; i < LANES; i++) {\n"
        "    int blk_ = (i / 2) * 2;\n"
        "    e_[i] = (i % 2 == 0) ? av[blk_] : bv[blk_];\n"
        "}",
    "unpckldq":
        "for (i = 0; i < LANES; i++) {\n"
        "    int blk_ = (i / 4) * 4, j_ = i % 4;\n"
        "    e_[i] = (j_ % 2 == 0) ? av[blk_ + j_ / 2] :\n"
        "                            bv[blk_ + j_ / 2];\n"
        "}",
    "unpcklpd":
        "for (i = 0; i < LANES; i++) {\n"
        "    int blk_ = (i / 2) * 2;\n"
        "    e_[i] = (i % 2 == 0) ? av[blk_] : bv[blk_];\n"
        "}",
    "sllv":
        "for (i = 0; i < LANES; i++) {\n"
        "    unsigned long long c_ = (UELEM)bv[i];\n"
        "    e_[i] = c_ >= {ESB} ? (ELEM)0 :\n"
        "            (ELEM)((UELEM)av[i] << c_);\n"
        "}",
    "srlv":
        "for (i = 0; i < LANES; i++) {\n"
        "    unsigned long long c_ = (UELEM)bv[i];\n"
        "    e_[i] = c_ >= {ESB} ? (ELEM)0 :\n"
        "            (ELEM)((UELEM)av[i] >> c_);\n"
        "}",
    "srav":
        "for (i = 0; i < LANES; i++) {\n"
        "    unsigned long long c_ = (UELEM)bv[i];\n"
        "    e_[i] = c_ >= {ESB} ? (av[i] < 0 ? (ELEM)-1 : (ELEM)0) :\n"
        "            (ELEM)(av[i] >> c_);\n"
        "}",
    "pshufd":
        "for (i = 0; i < LANES; i++) {\n"
        "    int blk_ = (i / 4) * 4, j_ = i % 4;\n"
        "    e_[i] = av[blk_ + (({IMM} >> (2 * j_)) & 3)];\n"
        "}",
    "pshuflw":
        "for (i = 0; i < LANES; i++) {\n"
        "    int blk_ = (i / 8) * 8, j_ = i % 8;\n"
        "    e_[i] = j_ < 4 ? av[blk_ + (({IMM} >> (2 * j_)) & 3)] :\n"
        "                     av[i];\n"
        "}",
    "cmpp":
        "for (i = 0; i < LANES; i++) {\n"
        "    int un_ = isnan(av[i]) || isnan(bv[i]);\n"
        "    int p_ = ({IMM} == 0 || {IMM} == 16) ? (av[i] == bv[i]) :\n"
        "             ({IMM} == 8 || {IMM} == 24) ? (un_ || av[i] == bv[i]) :\n"
        "             ({IMM} == 1 || {IMM} == 17) ? (av[i] < bv[i]) :\n"
        "             ({IMM} == 2 || {IMM} == 18) ? (av[i] <= bv[i]) :\n"
        "             ({IMM} == 3 || {IMM} == 19) ? un_ :\n"
        "             ({IMM} == 4 || {IMM} == 20) ? (av[i] != bv[i]) :\n"
        "             ({IMM} == 12 || {IMM} == 28) ? (!un_ && av[i] != bv[i]) :\n"
        "             ({IMM} == 5 || {IMM} == 21) ? !(av[i] < bv[i]) :\n"
        "             ({IMM} == 6 || {IMM} == 22) ? !(av[i] <= bv[i]) :\n"
        "             ({IMM} == 7 || {IMM} == 23) ? !un_ :\n"
        "             ({IMM} == 9 || {IMM} == 25) ? !(av[i] >= bv[i]) :\n"
        "             ({IMM} == 10 || {IMM} == 26) ? !(av[i] > bv[i]) :\n"
        "             ({IMM} == 13 || {IMM} == 29) ? (av[i] >= bv[i]) :\n"
        "             ({IMM} == 14 || {IMM} == 30) ? (av[i] > bv[i]) :\n"
        "             ({IMM} == 15 || {IMM} == 31) ? 1 : 0;\n"
        "    ((UELEM *)e_)[i] = p_ ? (UELEM)-1 : (UELEM)0;\n"
        "}",
    "fsqrt":
        "for (i = 0; i < LANES; i++) e_[i] = SQRT(av[i]);",
    "fround":
        "{ int mode_ = ({IMM} & 3);\n"
        "  for (i = 0; i < LANES; i++)\n"
        "      e_[i] = (ELEM)(mode_ == 0 ? nearbyint(av[i]) :\n"
        "                     mode_ == 1 ? floor(av[i]) :\n"
        "                     mode_ == 2 ? ceil(av[i]) :\n"
        "                                  trunc(av[i])); }",
    "abs":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = av[i] < 0 ? (ELEM)(0 - (UELEM)av[i]) : av[i];",
    "movmskb":
        "want_ = 0;\n"
        "for (i = 0; i < 16; i++)\n"
        "    want_ |= (((const signed char *)av)[i] < 0) << i;",
    "movmskps":
        "want_ = 0;\n"
        "for (i = 0; i < 4; i++) {\n"
        "    unsigned int u_;\n"
        "    memcpy(&u_, &((const float *)av)[i], sizeof u_);\n"
        "    want_ |= (int)(u_ >> 31) << i;\n"
        "}",
    "movmskpd":
        "want_ = 0;\n"
        "for (i = 0; i < 2; i++) {\n"
        "    unsigned long long u_;\n"
        "    memcpy(&u_, &((const double *)av)[i], sizeof u_);\n"
        "    want_ |= (int)(u_ >> 63) << i;\n"
        "}",
    "sllc":
        "for (i = 0; i < LANES; i++) {\n"
        "    unsigned long long c_ ="
        " ((const unsigned long long *)bv)[0];\n"
        "    e_[i] = c_ >= {ESB} ? (ELEM)0 :\n"
        "            (ELEM)((UELEM)av[i] << c_);\n"
        "}",
    "srlc":
        "for (i = 0; i < LANES; i++) {\n"
        "    unsigned long long c_ ="
        " ((const unsigned long long *)bv)[0];\n"
        "    e_[i] = c_ >= {ESB} ? (ELEM)0 :\n"
        "            (ELEM)((UELEM)av[i] >> c_);\n"
        "}",
    "srac":
        "for (i = 0; i < LANES; i++) {\n"
        "    unsigned long long c_ ="
        " ((const unsigned long long *)bv)[0];\n"
        "    e_[i] = c_ >= {ESB} ? (av[i] < 0 ? (ELEM)-1 :"
        " (ELEM)0) :\n"
        "            (ELEM)(av[i] >> c_);\n"
        "}",
    "adds":
        "for (i = 0; i < LANES; i++) {\n"
        "    long long t_ = (long long)av[i] + bv[i];\n"
        "    long long lo_ = -(1LL << ({ESB} - 1));\n"
        "    long long hi_ = (1LL << ({ESB} - 1)) - 1;\n"
        "    e_[i] = (ELEM)(t_ < lo_ ? lo_ : t_ > hi_ ? hi_ : t_);\n"
        "}",
    "subs":
        "for (i = 0; i < LANES; i++) {\n"
        "    long long t_ = (long long)av[i] - bv[i];\n"
        "    long long lo_ = -(1LL << ({ESB} - 1));\n"
        "    long long hi_ = (1LL << ({ESB} - 1)) - 1;\n"
        "    e_[i] = (ELEM)(t_ < lo_ ? lo_ : t_ > hi_ ? hi_ : t_);\n"
        "}",
    "addus":
        "for (i = 0; i < LANES; i++) {\n"
        "    unsigned long long t_ = (unsigned long long)(UELEM)av[i] + (UELEM)bv[i];\n"
        "    e_[i] = (ELEM)(t_ > (UELEM)-1 ? (UELEM)-1 : (UELEM)t_);\n"
        "}",
    "subus":
        "for (i = 0; i < LANES; i++) {\n"
        "    long long t_ = (long long)(UELEM)av[i] - (UELEM)bv[i];\n"
        "    e_[i] = (ELEM)(t_ < 0 ? 0 : (UELEM)t_);\n"
        "}",
    "muludq":
        "for (i = 0; i < LANES / 2; i++)\n"
        "    ((unsigned long long *)e_)[i] =\n"
        "        (unsigned long long)(UELEM)av[2 * i] * (UELEM)bv[2 * i];",
    "muldq":
        "for (i = 0; i < LANES / 2; i++)\n"
        "    ((long long *)e_)[i] =\n"
        "        (long long)av[2 * i] * bv[2 * i];",
    "pmaddwd":
        "for (i = 0; i < LANES / 2; i++)\n"
        "    ((int *)e_)[i] = (int)((long long)av[2 * i] * bv[2 * i] +\n"
        "                           (long long)av[2 * i + 1] * bv[2 * i + 1]);",
    "pmaddubsw":
        "for (i = 0; i < LANES / 2; i++) {\n"
        "    int t_ = (int)((const unsigned char *)av)[2 * i] * (int)((const signed char *)bv)[2 * i] +\n"
        "             (int)((const unsigned char *)av)[2 * i + 1] * (int)((const signed char *)bv)[2 * i + 1];\n"
        "    e_[i] = (OELEM)(t_ < -32768 ? -32768 : t_ > 32767 ? 32767 : t_);\n"
        "}",
    "packss8":
        "for (i = 0; i < 2 * LANES; i++) {\n"
        "    int K_ = 128 / {ESB}, c_ = i / (2 * K_), j_ = i % (2 * K_);\n"
        "    long long v_ = j_ < K_ ? av[c_ * K_ + j_] : bv[c_ * K_ + j_ - K_];\n"
        "    ((signed char *)e_)[i] = (signed char)(v_ < -128 ? -128 : v_ > 127 ? 127 : v_);\n"
        "}",
    "packss16":
        "for (i = 0; i < 2 * LANES; i++) {\n"
        "    int K_ = 128 / {ESB}, c_ = i / (2 * K_), j_ = i % (2 * K_);\n"
        "    long long v_ = j_ < K_ ? av[c_ * K_ + j_] : bv[c_ * K_ + j_ - K_];\n"
        "    ((short *)e_)[i] = (short)(v_ < -32768 ? -32768 : v_ > 32767 ? 32767 : v_);\n"
        "}",
    "packus8":
        "for (i = 0; i < 2 * LANES; i++) {\n"
        "    int K_ = 128 / {ESB}, c_ = i / (2 * K_), j_ = i % (2 * K_);\n"
        "    long long v_ = j_ < K_ ? av[c_ * K_ + j_] : bv[c_ * K_ + j_ - K_];\n"
        "    ((unsigned char *)e_)[i] = (unsigned char)(v_ < 0 ? 0 : v_ > 255 ? 255 : v_);\n"
        "}",
    "packus16":
        "for (i = 0; i < 2 * LANES; i++) {\n"
        "    int K_ = 128 / {ESB}, c_ = i / (2 * K_), j_ = i % (2 * K_);\n"
        "    long long v_ = j_ < K_ ? av[c_ * K_ + j_] : bv[c_ * K_ + j_ - K_];\n"
        "    ((unsigned short *)e_)[i] = (unsigned short)(v_ < 0 ? 0 : v_ > 65535 ? 65535 : v_);\n"
        "}",
    "unpckl":
        "for (i = 0; i < LANES; i++) {\n"
        "    int K_ = 128 / {ESB}, c_ = i / K_, w_ = i % K_;\n"
        "    e_[i] = (w_ % 2 == 0) ? av[c_ * K_ + w_ / 2] : bv[c_ * K_ + w_ / 2];\n"
        "}",
    "unpckh":
        "for (i = 0; i < LANES; i++) {\n"
        "    int K_ = 128 / {ESB}, c_ = i / K_, w_ = i % K_;\n"
        "    e_[i] = (w_ % 2 == 0) ? av[c_ * K_ + K_ / 2 + w_ / 2] : bv[c_ * K_ + K_ / 2 + w_ / 2];\n"
        "}",
    "shufps":
        "for (i = 0; i < LANES; i++) {\n"
        "    int blk_ = (i / 4) * 4, j_ = i % 4;\n"
        "    e_[i] = j_ < 2 ? av[blk_ + (({IMM} >> (2 * j_)) & 3)] :\n"
        "                     bv[blk_ + (({IMM} >> (2 * (j_ - 2) + 4)) & 3)];\n"
        "}",
    "shufpd":
        "for (i = 0; i < LANES; i++) {\n"
        "    int c_ = i / 2, j_ = i % 2;\n"
        "    e_[i] = j_ == 0 ? av[c_ * 2 + (({IMM} >> (2 * c_)) & 1)] :\n"
        "                      bv[c_ * 2 + (({IMM} >> (2 * c_ + 1)) & 1)];\n"
        "}",
    "blend":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = (({IMM} >> i) & 1) ? bv[i] : av[i];",
    "blendw":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = (({IMM} >> (i % (128 / {ESB}))) & 1) ? bv[i] : av[i];",
    "dpps":
        "{ int c_, j_;\n"
        "  for (c_ = 0; c_ < LANES; c_ += 128 / {ESB}) {\n"
        "      ELEM s_ = 0;\n"
        "      for (j_ = 0; j_ < 128 / {ESB}; j_++)\n"
        "          if (({IMM} >> (4 + j_)) & 1) s_ += av[c_ + j_] * bv[c_ + j_];\n"
        "      for (j_ = 0; j_ < 128 / {ESB}; j_++)\n"
        "          e_[c_ + j_] = (({IMM} >> j_) & 1) ? s_ : (ELEM)0;\n"
        "  } }",
    "insertps":
        "{ int j_;\n"
        "  for (j_ = 0; j_ < LANES; j_++) e_[j_] = av[j_];\n"
        "  e_[({IMM} >> 4) & 3] = bv[({IMM} >> 6) & 3];\n"
        "  for (j_ = 0; j_ < LANES; j_++)\n"
        "      if (({IMM} >> j_) & 1) e_[j_] = 0; }",
    "alignr":
        "for (i = 0; i < {TOT}; i++) {\n"
        "    int chunk_ = i & ~15, j_ = (i & 15) + {IMM};\n"
        "    ((unsigned char *)e_)[i] = j_ < 16 ? ((unsigned char *)bv)[chunk_ + j_] :\n"
        "        j_ < 32 ? ((unsigned char *)av)[chunk_ + j_ - 16] : 0;\n"
        "}",
    "pshufhw":
        "for (i = 0; i < LANES; i++) {\n"
        "    int blk_ = (i / 8) * 8, j_ = i % 8;\n"
        "    e_[i] = j_ < 4 ? av[i] : av[blk_ + 4 + (({IMM} >> (2 * (j_ - 4))) & 3)];\n"
        "}",
    "slli":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = {IMM} >= {ESB} ? (ELEM)0 : (ELEM)((UELEM)av[i] << {IMM});",
    "srli":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = {IMM} >= {ESB} ? (ELEM)0 : (ELEM)((UELEM)av[i] >> {IMM});",
    "srai":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = {IMM} >= {ESB} ? (av[i] < 0 ? (ELEM)-1 : (ELEM)0) : (ELEM)(av[i] >> {IMM});",
    "slliq":
        "for (i = 0; i < {TOT}; i++) {\n"
        "    int chunk_ = i & ~15, j_ = i & 15;\n"
        "    ((unsigned char *)e_)[i] = j_ < {IMM} ? 0 : ((unsigned char *)av)[chunk_ + j_ - {IMM}];\n"
        "}",
    "srliq":
        "for (i = 0; i < {TOT}; i++) {\n"
        "    int chunk_ = i & ~15, j_ = i & 15;\n"
        "    ((unsigned char *)e_)[i] = j_ + {IMM} > 15 ? 0 : ((unsigned char *)av)[chunk_ + j_ + {IMM}];\n"
        "}",
    "movehl":
        "e_[0] = bv[2]; e_[1] = bv[3]; e_[2] = av[2]; e_[3] = av[3];",
    "movelh":
        "e_[0] = av[0]; e_[1] = av[1]; e_[2] = bv[0]; e_[3] = bv[1];",
    "cvt_widen":
        "for (i = 0; i < {RTOT} * 8 / {OESB}; i++)\n"
        "    ((OELEM *)e_)[i] = (OELEM)av[i];",
    "cvt_rint":
        "for (i = 0; i < {TOT} * 8 / {ESB}; i++)\n"
        "    ((OELEM *)e_)[i] = (OELEM)rint(av[i]);",
    "cvt_trunc":
        "for (i = 0; i < {TOT} * 8 / {ESB}; i++)\n"
        "    ((OELEM *)e_)[i] = (OELEM)av[i];",
    "cvt_rintw":
        "for (i = 0; i < {RTOT} * 8 / {OESB}; i++)\n"
        "    ((OELEM *)e_)[i] = (OELEM)rint((double)av[i]);",
    "cvt_truncw":
        "for (i = 0; i < {RTOT} * 8 / {OESB}; i++)\n"
        "    ((OELEM *)e_)[i] = (OELEM)trunc((double)av[i]);",
    "cvt_rintuw":
        "for (i = 0; i < {RTOT} * 8 / {OESB}; i++) {\n"
        "    double d_ = rint((double)av[i]);\n"
        "    ((OUELEM *)e_)[i] = (d_ != d_ || d_ < 0 ||\n"
        "        d_ >= ({OESB} == 64 ?\n"
        "        18446744073709551616.0 : 4294967296.0)) ?\n"
        "        (OUELEM)-1 : (OUELEM)d_;\n"
        "}",
    "cvt_truncuw":
        "for (i = 0; i < {RTOT} * 8 / {OESB}; i++) {\n"
        "    double d_ = trunc((double)av[i]);\n"
        "    ((OUELEM *)e_)[i] = (d_ != d_ || d_ < 0 ||\n"
        "        d_ >= ({OESB} == 64 ?\n"
        "        18446744073709551616.0 : 4294967296.0)) ?\n"
        "        (OUELEM)-1 : (OUELEM)d_;\n"
        "}",
    "cvt_rintu":
        "for (i = 0; i < {TOT} * 8 / {ESB}; i++) {\n"
        "    double d_ = rint((double)av[i]);\n"
        "    ((OUELEM *)e_)[i] = (d_ != d_ || d_ < 0 ||\n"
        "        d_ >= ({OESB} == 64 ?\n"
        "        18446744073709551616.0 : 4294967296.0)) ?\n"
        "        (OUELEM)-1 : (OUELEM)d_;\n"
        "}",
    "cvt_truncu":
        "for (i = 0; i < {TOT} * 8 / {ESB}; i++) {\n"
        "    double d_ = trunc((double)av[i]);\n"
        "    ((OUELEM *)e_)[i] = (d_ != d_ || d_ < 0 ||\n"
        "        d_ >= ({OESB} == 64 ?\n"
        "        18446744073709551616.0 : 4294967296.0)) ?\n"
        "        (OUELEM)-1 : (OUELEM)d_;\n"
        "}",
    "cvt_f2f":
        "for (i = 0; i < {RTOT} * 8 / {OESB}; i++)\n"
        "    ((OFELEM *)e_)[i] = (OFELEM)av[i];",
    "comieq":
        "want_ = (*(const ELEM *)av == *(const ELEM *)bv);",
    "comine":
        "want_ = (*(const ELEM *)av != *(const ELEM *)bv);",
    "comilt":
        "want_ = (*(const ELEM *)av < *(const ELEM *)bv);",
    "comile":
        "want_ = (*(const ELEM *)av <= *(const ELEM *)bv);",
    "comigt":
        "want_ = (*(const ELEM *)av > *(const ELEM *)bv);",
    "comige":
        "want_ = (*(const ELEM *)av >= *(const ELEM *)bv);",
    "ptestz":
        "want_ = 1;\n"
        "for (i = 0; i < {TOT}; i++)\n"
        "    if (((const unsigned char *)av)[i] & ((const unsigned char *)bv)[i]) want_ = 0;",
    "ptestc":
        "want_ = 1;\n"
        "for (i = 0; i < {TOT}; i++)\n"
        "    if (((const unsigned char *)bv)[i] & ~((const unsigned char *)av)[i]) want_ = 0;",
    "ptestnzc":
        "{ int z_ = 1, c_ = 1;\n"
        "  for (i = 0; i < {TOT}; i++) {\n"
        "      if (((const unsigned char *)av)[i] & ((const unsigned char *)bv)[i]) z_ = 0;\n"
        "      if (((const unsigned char *)bv)[i] & ~((const unsigned char *)av)[i]) c_ = 0;\n"
        "  }\n"
        "  want_ = !z_ && !c_; }",
    "ptestzps":
        "want_ = 1;\n"
        "for (i = {ESB} / 8 - 1; i < {TOT}; i += {ESB} / 8)\n"
        "    if (((const unsigned char *)av)[i] & ((const unsigned char *)bv)[i] & 0x80) want_ = 0;",
    "ptestcps":
        "want_ = 1;\n"
        "for (i = {ESB} / 8 - 1; i < {TOT}; i += {ESB} / 8)\n"
        "    if (((const unsigned char *)bv)[i] & ~((const unsigned char *)av)[i] & 0x80) want_ = 0;",
    "ptestnzcps":
        "{ int z_ = 1, c_ = 1;\n"
        "  for (i = {ESB} / 8 - 1; i < {TOT}; i += {ESB} / 8) {\n"
        "      if (((const unsigned char *)av)[i] & ((const unsigned char *)bv)[i] & 0x80) z_ = 0;\n"
        "      if (((const unsigned char *)bv)[i] & ~((const unsigned char *)av)[i] & 0x80) c_ = 0;\n"
        "  }\n"
        "  want_ = !z_ && !c_; }",
    "pextrps":
        "{ unsigned int u_;\n"
        "  memcpy(&u_, &((const float *)av)[{IMM}], sizeof u_);\n"
        "  want_ = (long long)(int)u_; }",
    "bitandnot":
        "for (i = 0; i < {TOT}; i++)\n"
        "    ((unsigned char *)e_)[i] = ~((unsigned char *)av)[i] &\n"
        "                                ((unsigned char *)bv)[i];",
    "movmskb32":
        "want_ = 0;\n"
        "for (i = 0; i < 32; i++)\n"
        "    want_ |= ((long long)(((const signed char *)av)[i] < 0)) << i;\n"
        "want_ = (int)want_;",
    "movmskps8":
        "want_ = 0;\n"
        "for (i = 0; i < 8; i++) {\n"
        "    unsigned int u_;\n"
        "    memcpy(&u_, &((const float *)av)[i], sizeof u_);\n"
        "    want_ |= (long long)(u_ >> 31) << i;\n"
        "}",
    "movmskpd4":
        "want_ = 0;\n"
        "for (i = 0; i < 4; i++) {\n"
        "    unsigned long long u_;\n"
        "    memcpy(&u_, &((const double *)av)[i], sizeof u_);\n"
        "    want_ |= (long long)(u_ >> 63) << i;\n"
        "}",
    "rol":
        "for (i = 0; i < LANES; i++) {\n"
        "    int r_ = {IMM} & ({ESB} - 1);\n"
        "    UELEM u_ = (UELEM)av[i];\n"
        "    e_[i] = (ELEM)((u_ << r_) |\n"
        "        (u_ >> (({ESB} - r_) & ({ESB} - 1)))); }",
    "ror":
        "for (i = 0; i < LANES; i++) {\n"
        "    int r_ = {IMM} & ({ESB} - 1);\n"
        "    UELEM u_ = (UELEM)av[i];\n"
        "    e_[i] = (ELEM)((u_ >> r_) |\n"
        "        (u_ << (({ESB} - r_) & ({ESB} - 1)))); }",
    "rolv":
        "for (i = 0; i < LANES; i++) {\n"
        "    int r_ = (int)((UELEM)bv[i] & ({ESB} - 1));\n"
        "    UELEM u_ = (UELEM)av[i];\n"
        "    e_[i] = (ELEM)((u_ << r_) |\n"
        "        (u_ >> (({ESB} - r_) & ({ESB} - 1)))); }",
    "rorv":
        "for (i = 0; i < LANES; i++) {\n"
        "    int r_ = (int)((UELEM)bv[i] & ({ESB} - 1));\n"
        "    UELEM u_ = (UELEM)av[i];\n"
        "    e_[i] = (ELEM)((u_ >> r_) |\n"
        "        (u_ << (({ESB} - r_) & ({ESB} - 1)))); }",
    "conflict":
        "for (i = 0; i < LANES; i++) {\n"
        "    UELEM m_ = 0;\n"
        "    int j_;\n"
        "    for (j_ = 0; j_ < i; j_++)\n"
        "        if (av[j_] == av[i]) m_ |= (UELEM)1 << j_;\n"
        "    e_[i] = (ELEM)m_; }",
    "lzcnt":
        "for (i = 0; i < LANES; i++) {\n"
        "    UELEM u_ = (UELEM)av[i];\n"
        "    e_[i] = u_ == 0 ? {ESB} :\n"
        "        (ELEM)(__builtin_clzll(u_) - (64 - {ESB})); }",
    "dpbusd":
        "for (i = 0; i < LANES; i++) {\n"
        "    int s_ = 0, k_;\n"
        "    for (k_ = 0; k_ < 4; k_++)\n"
        "        s_ += ((const unsigned char *)bv)[4 * i + k_] *\n"
        "            ((const signed char *)cv)[4 * i + k_];\n"
        "    e_[i] = av[i] + s_; }",
    "dpbusds":
        "for (i = 0; i < LANES; i++) {\n"
        "    long long s_ = av[i];\n"
        "    int k_;\n"
        "    for (k_ = 0; k_ < 4; k_++)\n"
        "        s_ += ((const unsigned char *)bv)[4 * i + k_] *\n"
        "            ((const signed char *)cv)[4 * i + k_];\n"
        "    e_[i] = s_ > 2147483647LL ? (ELEM)2147483647 :\n"
        "        s_ < -2147483647LL - 1 ? (ELEM)(-2147483647 - 1) :\n"
        "        (ELEM)s_; }",
    "dpwssd":
        "for (i = 0; i < LANES; i++) {\n"
        "    int s_ = 0, k_;\n"
        "    for (k_ = 0; k_ < 2; k_++)\n"
        "        s_ += ((const short *)bv)[2 * i + k_] *\n"
        "            ((const short *)cv)[2 * i + k_];\n"
        "    e_[i] = av[i] + s_; }",
    "dpwssds":
        "for (i = 0; i < LANES; i++) {\n"
        "    long long s_ = av[i];\n"
        "    int k_;\n"
        "    for (k_ = 0; k_ < 2; k_++)\n"
        "        s_ += ((const short *)bv)[2 * i + k_] *\n"
        "            ((const short *)cv)[2 * i + k_];\n"
        "    e_[i] = s_ > 2147483647LL ? (ELEM)2147483647 :\n"
        "        s_ < -2147483647LL - 1 ? (ELEM)(-2147483647 - 1) :\n"
        "        (ELEM)s_; }",
    "kblend":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = (k_ >> i) & 1 ? bv[i] : av[i];",
    "scalef":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = isnan(av[i]) || isnan(bv[i]) ? av[i] + bv[i] :\n"
        "        (ELEM)ldexp((double)av[i],\n"
        "            bv[i] > 400 ? 400 : bv[i] < -400 ? -400 :\n"
        "            (int)floor((double)bv[i]));",
    "range":
        "for (i = 0; i < LANES; i++) {\n"
        "    ELEM x_ = av[i], y_ = bv[i], s_;\n"
        "    switch ({IMM} & 3) {\n"
        "    case 0: s_ = x_ < y_ ? x_ : y_; break;\n"
        "    case 1: s_ = x_ > y_ ? x_ : y_; break;\n"
        "    case 2: s_ = fabs(x_) < fabs(y_) ? x_ : y_; break;\n"
        "    default: s_ = fabs(x_) > fabs(y_) ? x_ : y_; break; }\n"
        "    switch (({IMM} >> 2) & 3) {\n"
        "    case 0: e_[i] = (ELEM)copysign((double)s_, (double)x_); break;\n"
        "    case 2: e_[i] = (ELEM)fabs((double)s_); break;\n"
        "    case 3: e_[i] = -(ELEM)fabs((double)s_); break;\n"
        "    default: e_[i] = s_; break; } }",
    "getexp":
        "for (i = 0; i < LANES; i++) {\n"
        "    int ex_;\n"
        "    if (av[i] == 0) e_[i] = (ELEM)(-1.0 / 0.0);\n"
        "    else if (!isfinite(av[i])) e_[i] = av[i];\n"
        "    else { (void)frexp((double)av[i], &ex_);\n"
        "        e_[i] = (ELEM)(ex_ - 1); } }",
    "reduce":
        "for (i = 0; i < LANES; i++) {\n"
        "    int md_ = ({IMM} & 4) ? 0 : ({IMM} & 3);\n"
        "    double v_ = (double)av[i], r_;\n"
        "    r_ = md_ == 0 ? nearbyint(v_) : md_ == 1 ? floor(v_) :\n"
        "        md_ == 2 ? ceil(v_) : trunc(v_);\n"
        "    e_[i] = v_ - r_ == 0 ? (ELEM)(md_ == 1 ?\n"
        "        -0.0 : 0.0) : (ELEM)(v_ - r_); }",
    "shufx4":
        "for (i = 0; i < LANES; i++) {\n"
        "    int c_ = (i * {ESB}) / 128;\n"
        "    int sel_ = (({IMM} >> (2 * c_)) & 3) +\n"
        "        (c_ >= 2 ? {TOT} / 16 : 0);\n"
        "    int w_ = 128 / {ESB};\n"
        "    e_[i] = sel_ < {TOT} / 16 ?\n"
        "        av[sel_ * w_ + i % w_] :\n"
        "        bv[(sel_ - {TOT} / 16) * w_ + i % w_]; }",
    "extractx8":
        "for (i = 0; i < {RTOT} * 8 / {OESB}; i++)\n"
        "    e_[i] = av[i + {IMM} * (256 / {ESB})];",
    "extractx4":
        "for (i = 0; i < {RTOT} * 8 / {OESB}; i++)\n"
        "    e_[i] = av[i + {IMM} * (128 / {ESB})];",
    "insertx4":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = (i / (128 / {ESB})) == {IMM} ?\n"
        "        bv[i % (128 / {ESB})] : av[i];",
    "dbsad":
        "{ int w_, lane_, s_, r_, j_;\n"
        "  for (w_ = 0; w_ < {TOT} * 8 / 16; w_++) {\n"
        "      unsigned int ssum_ = 0;\n"
        "      int ab_, tb_;\n"
        "      lane_ = w_ / 8; s_ = (w_ % 8) / 4; r_ = w_ % 4;\n"
        "      ab_ = lane_ * 16 + s_ * 8 + (r_ >= 2 ? 4 : 0);\n"
        "      tb_ = s_ * 8 + r_;\n"
        "      for (j_ = 0; j_ < 4; j_++) {\n"
        "          int p_ = tb_ + j_;\n"
        "          int sel_ = ({IMM} >> (2 * (p_ / 4))) & 3;\n"
        "          int bv_ = ((const unsigned char *)bv)\n"
        "              [lane_ * 16 + 4 * sel_ + (p_ % 4)];\n"
        "          int d_ = ((const unsigned char *)av)\n"
        "              [ab_ + j_] - bv_;\n"
        "          ssum_ += d_ < 0 ? -d_ : d_; }\n"
        "      ((unsigned short *)e_)[w_] = ssum_; } }",
    "multishift":
        "for (i = 0; i < {TOT}; i++) {\n"
        "    int q_ = i & ~7;\n"
        "    unsigned long long bq_;\n"
        "    int sh_ = (((const unsigned char *)av)[i] & 0x3f);\n"
        "    memcpy(&bq_, (const unsigned char *)bv + q_, 8);\n"
        "    ((unsigned char *)e_)[i] = (unsigned char)(\n"
        "        (bq_ >> sh_) | (bq_ << ((64 - sh_) & 63))); }",
    "permt2":
        "for (i = 0; i < LANES; i++) {\n"
        "    unsigned ix_ = ((const UELEM *)bv)[i] & (2 * LANES - 1);\n"
        "    e_[i] = ix_ < (unsigned)LANES ? av[ix_] :\n"
        "        cv[ix_ - LANES]; }",
    "compress":
        "{ int n_ = 0;\n"
        "  for (i = 0; i < LANES; i++)\n"
        "      if ((k_ >> i) & 1) e_[n_++] = av[i];\n"
        "  for (; n_ < LANES; n_++) e_[n_] = 0; }",
    "expand":
        "{ int n_ = 0;\n"
        "  for (i = 0; i < LANES; i++)\n"
        "      e_[i] = (k_ >> i) & 1 ? av[n_++] : (ELEM)0; }",
    "faddss":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = i == 0 ? av[0] + bv[0] : av[i];",
    "movss":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = i == 0 ? bv[0] : av[i];",
    "minpos":
        "{ int j_ = 0;\n"
        "  for (i = 1; i < LANES; i++)\n"
        "      if (av[i] < av[j_]) j_ = i;\n"
        "  e_[0] = av[j_]; e_[1] = j_;\n"
        "  for (i = 2; i < LANES; i++) e_[i] = 0; }",
    "fsubss":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = i == 0 ? av[0] - bv[0] : av[i];",
    "fmulss":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = i == 0 ? av[0] * bv[0] : av[i];",
    "fdivss":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = i == 0 ? av[0] / bv[0] : av[i];",
    "fminss":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = i == 0 ? (bv[0] < av[0] ? bv[0] : av[0])\n"
        "        : av[i];",
    "fmaxss":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = i == 0 ? (bv[0] > av[0] ? bv[0] : av[0])\n"
        "        : av[i];",
    "fsqrtss":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = i == 0 ? SQRT(bv[0]) : av[i];",
    "pmov":
        "{ int j_;\n"
        "  for (i = 0; i < LANES; i++) e_[i] = (OELEM)av[i];\n"
        "  for (j_ = LANES; j_ < {RTOT} * 8 / {OESB}; j_++)\n"
        "      e_[j_] = 0; }",
    "pmovs":
        "{ int j_;\n"
        "  for (i = 0; i < LANES; i++)\n"
        "      e_[i] = av[i] > (ELEM)((1LL << ({OESB} - 1)) - 1) ?\n"
        "          (OELEM)((1LL << ({OESB} - 1)) - 1) :\n"
        "          av[i] < (ELEM)(-(1LL << ({OESB} - 1))) ?\n"
        "          (OELEM)(-(1LL << ({OESB} - 1))) : (OELEM)av[i];\n"
        "  for (j_ = LANES; j_ < {RTOT} * 8 / {OESB}; j_++)\n"
        "      e_[j_] = 0; }",
    "pmovus":
        "{ int j_;\n"
        "  for (i = 0; i < LANES; i++)\n"
        "      e_[i] = (UELEM)av[i] >\n"
        "          (UELEM)((1ULL << {OESB}) - 1) ?\n"
        "          (OELEM)((1ULL << {OESB}) - 1) :\n"
        "          (OELEM)(UELEM)av[i];\n"
        "  for (j_ = LANES; j_ < {RTOT} * 8 / {OESB}; j_++)\n"
        "      e_[j_] = 0; }",
    "cvtq2f":
        "{ int j_;\n"
        "  for (i = 0; i < LANES; i++) e_[i] = (OFELEM)av[i];\n"
        "  for (j_ = LANES; j_ < {RTOT} * 8 / {OESB}; j_++)\n"
        "      e_[j_] = 0; }",
    "cvtq2fu":
        "{ int j_;\n"
        "  for (i = 0; i < LANES; i++)\n"
        "      e_[i] = (OFELEM)(UELEM)av[i];\n"
        "  for (j_ = LANES; j_ < {RTOT} * 8 / {OESB}; j_++)\n"
        "      e_[j_] = 0; }",
    "cvtuwiden":
        "for (i = 0; i < {RTOT} * 8 / {OESB}; i++)\n"
        "    e_[i] = (OFELEM)(UELEM)av[i];",
    "fsqrtss2":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = i == 0 ? SQRT(av[0]) : av[i];",
    "rangess":
        "for (i = 0; i < LANES; i++) {\n"
        "    ELEM x_ = av[0], y_ = bv[0], s_;\n"
        "    if (i) { e_[i] = av[i]; continue; }\n"
        "    switch ({IMM} & 3) {\n"
        "    case 0: s_ = x_ < y_ ? x_ : y_; break;\n"
        "    case 1: s_ = x_ > y_ ? x_ : y_; break;\n"
        "    case 2: s_ = fabs(x_) < fabs(y_) ? x_ : y_; break;\n"
        "    default: s_ = fabs(x_) > fabs(y_) ? x_ : y_; break; }\n"
        "    switch (({IMM} >> 2) & 3) {\n"
        "    case 0: e_[i] = (ELEM)copysign((double)s_, (double)x_); break;\n"
        "    case 2: e_[i] = (ELEM)fabs((double)s_); break;\n"
        "    case 3: e_[i] = -(ELEM)fabs((double)s_); break;\n"
        "    default: e_[i] = s_; break; } }",
    "roundscaless":
        "{ int mode_ = ({IMM} & 3);\n"
        "  for (i = 0; i < LANES; i++)\n"
        "      e_[i] = i == 0 ? (ELEM)(mode_ == 0 ? nearbyint(bv[0]) :\n"
        "                     mode_ == 1 ? floor(bv[0]) :\n"
        "                     mode_ == 2 ? ceil(bv[0]) :\n"
        "                                  trunc(bv[0])) : av[i]; }",
    "scalefss":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = i == 0 ? (isnan(av[0]) || isnan(bv[0]) ?\n"
        "        av[0] + bv[0] : (ELEM)ldexp((double)av[0],\n"
        "            bv[0] > 400 ? 400 : bv[0] < -400 ? -400 :\n"
        "            (int)floor((double)bv[0]))) : av[i];",
    "getexpss2":
        "for (i = 0; i < LANES; i++) {\n"
        "    int ex_;\n"
        "    if (i == 0) {\n"
        "        if (bv[0] == 0) e_[0] = (ELEM)(-1.0 / 0.0);\n"
        "        else if (!isfinite(bv[0])) e_[0] = bv[0];\n"
        "        else { (void)frexp((double)bv[0], &ex_);\n"
        "            e_[0] = (ELEM)(ex_ - 1); } }\n"
        "    else e_[i] = av[i]; }",
    "getexpss":
        "for (i = 0; i < LANES; i++) {\n"
        "    int ex_;\n"
        "    if (i == 0) {\n"
        "        if (av[0] == 0) e_[0] = (ELEM)(-1.0 / 0.0);\n"
        "        else if (!isfinite(av[0])) e_[0] = av[0];\n"
        "        else { (void)frexp((double)av[0], &ex_);\n"
        "            e_[0] = (ELEM)(ex_ - 1); } }\n"
        "    else e_[i] = av[i]; }",
    "permfullx":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = bv[((const UELEM *)av)[i] & (LANES - 1)];",
    "sad":
        "for (i = 0; i < {TOT} / 8; i++) {\n"
        "    unsigned int s_ = 0;\n"
        "    int j_;\n"
        "    for (j_ = 0; j_ < 8; j_++) {\n"
        "        int d_ = ((const unsigned char *)av)\n"
        "            [i * 8 + j_] -\n"
        "            ((const unsigned char *)bv)[i * 8 + j_];\n"
        "        s_ += d_ < 0 ? -d_ : d_; }\n"
        "    ((unsigned short *)e_)[4 * i] = s_;\n"
        "    ((unsigned short *)e_)[4 * i + 1] = 0;\n"
        "    ((unsigned short *)e_)[4 * i + 2] = 0;\n"
        "    ((unsigned short *)e_)[4 * i + 3] = 0; }",
    "permq512":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = av[(i & ~3) + (({IMM} >> (2 * (i & 3)))"
        " & 3)];",
    "fixupss": 'for (i = 1; i < LANES; i++) e_[i] = bv[i];\n{ int i = 0;\n\n    UELEM cb_ = ((const UELEM *)cv)[i];\n    UELEM ub_ = ((const UELEM *)bv)[i];\n    int tok_;\n    if (isnan(bv[i]))\n        tok_ = (ub_ >> ({ESB} == 32 ? 22 : 51)) & 1 ? 0 : 1;\n    else if (bv[i] == 0) tok_ = 2;\n    else if (bv[i] == 1) tok_ = 3;\n    else if (bv[i] == -1.0 / 0.0) tok_ = 4;\n    else if (bv[i] == 1.0 / 0.0) tok_ = 5;\n    else if (bv[i] < 0) tok_ = 6;\n    else tok_ = 7;\n    switch ((cb_ >> (4 * tok_)) & 15) {\n    case 0: e_[i] = av[i]; break;\n    case 1: e_[i] = bv[i]; break;\n    case 2: { UELEM q_ = ub_ | (UELEM)1 <<\n        ({ESB} == 32 ? 22 : 51);\n        e_[i] = ((ELEM *)&q_)[0]; break; }\n    case 3: { UELEM q_ = {ESB} == 32 ? 0xffc00000u :\n        0xfff8000000000000ULL;\n        e_[i] = ((ELEM *)&q_)[0]; break; }\n    case 4: e_[i] = (ELEM)(-1.0 / 0.0); break;\n    case 5: e_[i] = (ELEM)(1.0 / 0.0); break;\n    case 6: e_[i] = (ELEM)(signbit(bv[i]) ?\n        -1.0 / 0.0 : 1.0 / 0.0); break;\n    case 7: e_[i] = (ELEM)(-0.0); break;\n    case 8: e_[i] = (ELEM)0.0; break;\n    case 9: e_[i] = (ELEM)-1; break;\n    case 10: e_[i] = (ELEM)1; break;\n    case 11: e_[i] = (ELEM)0.5; break;\n    case 12: e_[i] = (ELEM)90; break;\n    case 13: e_[i] = (ELEM)1.57079632679489661923; break;\n    case 14: e_[i] = (ELEM)({ESB} == 32 ?\n        3.4028234663852886e38 : 1.7976931348623157e308); break;\n    default: e_[i] = (ELEM)-({ESB} == 32 ?\n        3.4028234663852886e38 : 1.7976931348623157e308);\n        break; } }',
    "fixup":
        "for (i = 0; i < LANES; i++) {\n"
        "    UELEM cb_ = ((const UELEM *)cv)[i];\n"
        "    UELEM ub_ = ((const UELEM *)bv)[i];\n"
        "    int tok_;\n"
        "    if (isnan(bv[i]))\n"
        "        tok_ = (ub_ >> ({ESB} == 32 ? 22 : 51)) & 1 ? 0 : 1;\n"
        "    else if (bv[i] == 0) tok_ = 2;\n"
        "    else if (bv[i] == 1) tok_ = 3;\n"
        "    else if (bv[i] == -1.0 / 0.0) tok_ = 4;\n"
        "    else if (bv[i] == 1.0 / 0.0) tok_ = 5;\n"
        "    else if (bv[i] < 0) tok_ = 6;\n"
        "    else tok_ = 7;\n"
        "    switch ((cb_ >> (4 * tok_)) & 15) {\n"
        "    case 0: e_[i] = av[i]; break;\n"
        "    case 1: e_[i] = bv[i]; break;\n"
        "    case 2: { UELEM q_ = ub_ | (UELEM)1 <<\n"
        "        ({ESB} == 32 ? 22 : 51);\n"
        "        e_[i] = ((ELEM *)&q_)[0]; break; }\n"
        "    case 3: { UELEM q_ = {ESB} == 32 ? 0xffc00000u :\n"
        "        0xfff8000000000000ULL;\n"
        "        e_[i] = ((ELEM *)&q_)[0]; break; }\n"
        "    case 4: e_[i] = (ELEM)(-1.0 / 0.0); break;\n"
        "    case 5: e_[i] = (ELEM)(1.0 / 0.0); break;\n"
        "    case 6: e_[i] = (ELEM)(signbit(bv[i]) ?\n"
        "        -1.0 / 0.0 : 1.0 / 0.0); break;\n"
        "    case 7: e_[i] = (ELEM)(-0.0); break;\n"
        "    case 8: e_[i] = (ELEM)0.0; break;\n"
        "    case 9: e_[i] = (ELEM)-1; break;\n"
        "    case 10: e_[i] = (ELEM)1; break;\n"
        "    case 11: e_[i] = (ELEM)0.5; break;\n"
        "    case 12: e_[i] = (ELEM)90; break;\n"
        "    case 13: e_[i] = (ELEM)1.57079632679489661923; break;\n"
        "    case 14: e_[i] = (ELEM)({ESB} == 32 ?\n"
        "        3.4028234663852886e38 : 1.7976931348623157e308); break;\n"
        "    default: e_[i] = (ELEM)-({ESB} == 32 ?\n"
        "        3.4028234663852886e38 : 1.7976931348623157e308);\n"
        "        break; } }",
    "mkicmp":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++) {\n"
        "    int c_;\n"
        "    switch ({IMM}) {\n"
        "    case 0: c_ = av[i] == bv[i]; break;\n"
        "    case 1: c_ = av[i] < bv[i]; break;\n"
        "    case 2: c_ = av[i] <= bv[i]; break;\n"
        "    case 3: c_ = 0; break;\n"
        "    case 4: c_ = av[i] != bv[i]; break;\n"
        "    case 5: c_ = av[i] >= bv[i]; break;\n"
        "    case 6: c_ = av[i] > bv[i]; break;\n"
        "    default: c_ = 1; break; }\n"
        "    if (c_) want_ |= 1ULL << i; }",
    "mkscmp":
        "want_ = 0;\n"
        "for (i = 0; i < 1; i++) {\n"
        "    int un_ = isnan(av[i]) || isnan(bv[i]);\n"
        "    int c_;\n"
        "    switch ({IMM}) {\n"
        "    case 0: case 16: case 8: case 24:\n"
        "        c_ = un_ ? ({IMM} == 8 || {IMM} == 24) : av[i] == bv[i]; break;\n"
        "    case 1: case 17: c_ = !un_ && av[i] < bv[i]; break;\n"
        "    case 2: case 18: c_ = !un_ && av[i] <= bv[i]; break;\n"
        "    case 3: case 19: c_ = un_; break;\n"
        "    case 4: case 12: case 20: case 28:\n"
        "        c_ = un_ ? ({IMM} == 4 || {IMM} == 20) : av[i] != bv[i]; break;\n"
        "    case 5: case 21: c_ = un_ || !(av[i] < bv[i]); break;\n"
        "    case 6: case 22: c_ = un_ || !(av[i] <= bv[i]); break;\n"
        "    case 7: case 23: c_ = !un_; break;\n"
        "    case 9: case 25: c_ = un_ || !(av[i] >= bv[i]); break;\n"
        "    case 10: case 26: c_ = un_ || !(av[i] > bv[i]); break;\n"
        "    case 11: case 27: c_ = 0; break;\n"
        "    case 13: case 29: c_ = !un_ && av[i] >= bv[i]; break;\n"
        "    case 14: case 30: c_ = !un_ && av[i] > bv[i]; break;\n"
        "    default: c_ = 1; break; }\n"
        "    if (c_) want_ |= 1ULL << i; }",
    "fpclasss":
        "want_ = 0;\n"
        "for (i = 0; i < 1; i++) {\n"
        "    int eb_ = {ESB} == 32 ? 8 : 11;\n"
        "    int mb_ = {ESB} - eb_ - 1;\n"
        "    UELEM u_ = ((const UELEM *)av)[i];\n"
        "    UELEM ae_ = u_ & (((UELEM)1 << ({ESB} - 1)) - 1);\n"
        "    int sg_ = (int)(u_ >> ({ESB} - 1));\n"
        "    int ex_ = (int)((u_ >> mb_) & ((1 << eb_) - 1));\n"
        "    UELEM mn_ = u_ & (((UELEM)1 << mb_) - 1);\n"
        "    int cl_ = 0;\n"
        "    if (ex_ == (1 << eb_) - 1 && mn_ != 0)\n"
        "        cl_ = ((mn_ >> (mb_ - 1)) & 1) ? 1 : 128;\n"
        "    else if (ex_ == (1 << eb_) - 1)\n"
        "        cl_ = sg_ ? 16 : 8;\n"
        "    else if (ae_ == 0)\n"
        "        cl_ = sg_ ? 4 : 2;\n"
        "    else {\n"
        "        if (ex_ == 0) cl_ |= 32;\n"
        "        if (sg_) cl_ |= 64; }\n"
        "    if ({IMM} & cl_) want_ |= 1ULL << i; }",
    "cmpss":
        "for (i = 0; i < LANES; i++) {\n"
        "    int un_ = isnan(av[0]) || isnan(bv[0]);\n"
        "    int p_ = ({IMM} == 0 || {IMM} == 16) ? (av[0] == bv[0]) :\n"
        "             ({IMM} == 8 || {IMM} == 24) ? (un_ || av[0] == bv[0]) :\n"
        "             ({IMM} == 1 || {IMM} == 17) ? (av[0] < bv[0]) :\n"
        "             ({IMM} == 2 || {IMM} == 18) ? (av[0] <= bv[0]) :\n"
        "             ({IMM} == 3 || {IMM} == 19) ? un_ :\n"
        "             ({IMM} == 4 || {IMM} == 20) ? (av[0] != bv[0]) :\n"
        "             ({IMM} == 12 || {IMM} == 28) ? (!un_ && av[0] != bv[0]) :\n"
        "             ({IMM} == 5 || {IMM} == 21) ? !(av[0] < bv[0]) :\n"
        "             ({IMM} == 6 || {IMM} == 22) ? !(av[0] <= bv[0]) :\n"
        "             ({IMM} == 7 || {IMM} == 23) ? !un_ :\n"
        "             ({IMM} == 9 || {IMM} == 25) ? !(av[0] >= bv[0]) :\n"
        "             ({IMM} == 10 || {IMM} == 26) ? !(av[0] > bv[0]) :\n"
        "             ({IMM} == 13 || {IMM} == 29) ? (av[0] >= bv[0]) :\n"
        "             ({IMM} == 14 || {IMM} == 30) ? (av[0] > bv[0]) :\n"
        "             ({IMM} == 15 || {IMM} == 31) ? 1 : 0;\n"
        "    if (i == 0) ((UELEM *)e_)[0] = p_ ? (UELEM)-1 : (UELEM)0;\n"
        "    else e_[i] = av[i]; }",
    "mkfcmp":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++) {\n"
        "    int un_ = isnan(av[i]) || isnan(bv[i]);\n"
        "    int c_;\n"
        "    switch ({IMM}) {\n"
        "    case 0: case 16: case 8: case 24:\n"
        "        c_ = un_ ? ({IMM} == 8 || {IMM} == 24) : av[i] == bv[i]; break;\n"
        "    case 1: case 17: c_ = !un_ && av[i] < bv[i]; break;\n"
        "    case 2: case 18: c_ = !un_ && av[i] <= bv[i]; break;\n"
        "    case 3: case 19: c_ = un_; break;\n"
        "    case 4: case 12: case 20: case 28:\n"
        "        c_ = un_ ? ({IMM} == 4 || {IMM} == 20) : av[i] != bv[i]; break;\n"
        "    case 5: case 21: c_ = un_ || !(av[i] < bv[i]); break;\n"
        "    case 6: case 22: c_ = un_ || !(av[i] <= bv[i]); break;\n"
        "    case 7: case 23: c_ = !un_; break;\n"
        "    case 9: case 25: c_ = un_ || !(av[i] >= bv[i]); break;\n"
        "    case 10: case 26: c_ = un_ || !(av[i] > bv[i]); break;\n"
        "    case 11: case 27: c_ = 0; break;\n"
        "    case 13: case 29: c_ = !un_ && av[i] >= bv[i]; break;\n"
        "    case 14: case 30: c_ = !un_ && av[i] > bv[i]; break;\n"
        "    default: c_ = 1; break; }\n"
        "    if (c_) want_ |= 1ULL << i; }",
    "mktest":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++)\n"
        "    if ((av[i] & bv[i]) != 0) want_ |= 1ULL << i;",
    "mktestn":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++)\n"
        "    if ((av[i] & bv[i]) == 0) want_ |= 1ULL << i;",
    "mkmov":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++)\n"
        "    if ((((UELEM *)av)[i] >> ({ESB} - 1)) & 1)\n"
        "        want_ |= 1ULL << i;",
    "fpclass":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++) {\n"
        "    int eb_ = {ESB} == 32 ? 8 : 11;\n"
        "    int mb_ = {ESB} - eb_ - 1;\n"
        "    UELEM u_ = ((const UELEM *)av)[i];\n"
        "    UELEM ae_ = u_ & (((UELEM)1 << ({ESB} - 1)) - 1);\n"
        "    int sg_ = (int)(u_ >> ({ESB} - 1));\n"
        "    int ex_ = (int)((u_ >> mb_) & ((1 << eb_) - 1));\n"
        "    UELEM mn_ = u_ & (((UELEM)1 << mb_) - 1);\n"
        "    int cl_ = 0;\n"
        "    if (ex_ == (1 << eb_) - 1 && mn_ != 0)\n"
        "        cl_ = ((mn_ >> (mb_ - 1)) & 1) ? 1 : 128;\n"
        "    else if (ex_ == (1 << eb_) - 1)\n"
        "        cl_ = sg_ ? 16 : 8;\n"
        "    else if (ae_ == 0)\n"
        "        cl_ = sg_ ? 4 : 2;\n"
        "    else {\n"
        "        if (ex_ == 0) cl_ |= 32;\n"
        "        if (sg_) cl_ |= 64; }\n"
        "    if ({IMM} & cl_) want_ |= 1ULL << i; }",
    "blendv":
        "for (i = 0; i < {TOT}; i++)\n"
        "    ((unsigned char *)e_)[i] = (((const unsigned char *)cv)[i] & 0x80) ?\n"
        "        ((unsigned char *)bv)[i] : ((unsigned char *)av)[i];",
    "blendvf":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = (((const UELEM *)cv)[i] >> ({ESB} - 1)) & 1 ? bv[i] : av[i];",
    "permilps":
        "for (i = 0; i < LANES; i++)\n    e_[i] = av[(i & ~3) + (({IMM} >> (2 * (i & 3))) & 3)];",
    "permilpd":
        "for (i = 0; i < LANES; i++)\n    e_[i] = av[(i & ~1) + (({IMM} >> i) & 1)];",
    "permvarps":
        "for (i = 0; i < LANES; i++)\n    e_[i] = av[(i & ~3) + (((const UELEM *)bv)[i] & 3)];",
    "permvarpd":
        "for (i = 0; i < LANES; i++)\n    e_[i] = av[(i & ~1) + (((const UELEM *)bv)[i] & 1)];",
    "perm2x128":
        "{ int s0_ = {IMM} & 3, s1_ = ({IMM} >> 4) & 3,"
        " h_ = LANES / 2, j_;\n"
        "  const ELEM *p0_ = s0_ == 0 ? av : s0_ == 1 ? av + h_"
        " : s0_ == 2 ? bv : bv + h_;\n"
        "  const ELEM *p1_ = s1_ == 0 ? av : s1_ == 1 ? av + h_"
        " : s1_ == 2 ? bv : bv + h_;\n"
        "  for (j_ = 0; j_ < h_; j_++) {\n"
        "      e_[j_] = ({IMM} & 8) ? (ELEM)0 : p0_[j_];\n"
        "      e_[h_ + j_] = ({IMM} & 128) ? (ELEM)0 : p1_[j_];"
        " } }",
    "permq":
        "for (i = 0; i < LANES; i++)\n    e_[i] = av[({IMM} >> (2 * i)) & 3];",
    "permfull":
        "for (i = 0; i < LANES; i++)\n    e_[i] = bv[av[i] & (LANES - 1)];",
    "extract128":
        "for (i = 0; i < {RTOT} * 8 / {OESB}; i++)\n    e_[i] = av[i + ({IMM} & 1) * ({RTOT} * 8 / {OESB})];",
    "bcast128":
        "for (i = 0; i < {RTOT} * 8 / {OESB}; i++)\n    e_[i] = av[i % ({TOT} * 8 / {ESB})];",
    "fma":
        "for (i = 0; i < LANES; i++)\n    e_[i] = MULADD(av[i], bv[i], cv[i]);",
    "fmsub":
        "for (i = 0; i < LANES; i++)\n    e_[i] = MULADD(av[i], bv[i], -cv[i]);",
    "fnmadd":
        "for (i = 0; i < LANES; i++)\n    e_[i] = MULADD(-av[i], bv[i], cv[i]);",
    "fnmsub":
        "for (i = 0; i < LANES; i++)\n    e_[i] = MULADD(-av[i], bv[i], -cv[i]);",
    "fmaddsub":
        "for (i = 0; i < LANES; i++)\n    e_[i] = MULADD(av[i], bv[i], (i & 1) ? cv[i] : -cv[i]);",
    "fmsubadd":
        "for (i = 0; i < LANES; i++)\n    e_[i] = MULADD(av[i], bv[i], (i & 1) ? -cv[i] : cv[i]);",
    "fmass":
        "e_[0] = MULADD(av[0], bv[0], cv[0]);"
        " for (i = 1; i < LANES; i++) e_[i] = av[i];",
    "fmssub":
        "e_[0] = MULADD(av[0], bv[0], -cv[0]);"
        " for (i = 1; i < LANES; i++) e_[i] = av[i];",
    "fmnadd":
        "e_[0] = MULADD(-av[0], bv[0], cv[0]);"
        " for (i = 1; i < LANES; i++) e_[i] = av[i];",
    "fmnsub":
        "e_[0] = MULADD(-av[0], bv[0], -cv[0]);"
        " for (i = 1; i < LANES; i++) e_[i] = av[i];",
    "cvtph":
        "for (i = 0; i < {RTOT} * 8 / {OESB}; i++)\n    e_[i] = half2float(((const unsigned short *)av)[i]);",
    "cvtps2ph":
        "for (i = 0; i < {TOT} * 8 / {ESB}; i++)\n"
        "    ((unsigned short *)e_)[i] = float2half(av[i],"
        " {IMM} & 3);\n"
        "for (i = {TOT} * 8 / {ESB}; i < {RTOT} * 8 / {OESB};"
        " i++)\n"
        "    ((unsigned short *)e_)[i] = 0;",
    "froundss":
        "{ int mode_ = ({IMM} & 3);\n"
        "  for (i = 0; i < LANES; i++) e_[i] = av[i];\n"
        "  e_[0] = (ELEM)(mode_ == 0 ? nearbyint(bv[0]) :\n"
        "                 mode_ == 1 ? floor(bv[0]) :\n"
        "                 mode_ == 2 ? ceil(bv[0]) :\n"
        "                              trunc(bv[0])); }",
    "blendm":
        "for (i = 0; i < LANES; i++)\n"
        "    e_[i] = ((k_ >> i) & 1) ? bv[i] : av[i];",
    "ternlog":
        "for (i = 0; i < {TOT}; i++) {\n"
        "    unsigned char a_ = ((unsigned char *)av)[i];\n"
        "    unsigned char b_ = ((unsigned char *)bv)[i];\n"
        "    unsigned char c_ = ((unsigned char *)cv)[i];\n"
        "    unsigned char r_ = 0;\n"
        "    int j_;\n"
        "    for (j_ = 0; j_ < 8; j_++) {\n"
        "        int idx_ = ((a_ >> j_) & 1) * 4 +\n"
        "                   ((b_ >> j_) & 1) * 2 + ((c_ >> j_) & 1);\n"
        "        r_ |= (unsigned char)((({IMM} >> idx_) & 1)"
        " << j_);\n"
        "    }\n"
        "    ((unsigned char *)e_)[i] = r_;\n"
        "}",
    "pinsr":
        "memcpy(e_, av, sizeof e_);\n"
        "e_[{IMM}] = (ELEM)bs_;",
    "pextr":
        "want_ = (long long)((const ELEM *)av)[{IMM}];\n"
        "if ({ESB} < 32)\n"
        "    want_ &= (long long)((1u << {ESB}) - 1);",
    "mkeq":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++)\n"
        "    if (av[i] == bv[i]) want_ |= 1ULL << i;",
    "mkgt":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++)\n"
        "    if (av[i] > bv[i]) want_ |= 1ULL << i;",
    "mklt":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++)\n"
        "    if (av[i] < bv[i]) want_ |= 1ULL << i;",
    "mkge":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++)\n"
        "    if (av[i] >= bv[i]) want_ |= 1ULL << i;",
    "mkle":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++)\n"
        "    if (av[i] <= bv[i]) want_ |= 1ULL << i;",
    "mkne":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++)\n"
        "    if (av[i] != bv[i]) want_ |= 1ULL << i;",
    "mksign":
        "want_ = 0;\n"
        "for (i = 0; i < LANES; i++)\n"
        "    if ((((UELEM *)av)[i] >> ({ESB} - 1)) & 1)\n"
        "        want_ |= 1ULL << i;",
}
IMM_SETS = {
    "PSHUFD": [0, 27, 85, 170, 255],
    "PSHUFLW": [0, 27, 85, 170, 255],
    "CMPP": [0, 1, 2, 4, 5, 6, 7],
    "CMPSS": [0, 1, 2, 4, 5, 6, 7],
    "CVTRSS2SI": [4, 8, 9, 10, 11], "CVTRSD2SI": [4, 8, 9, 10, 11],
    "CVTSS2USIR": [4, 8, 9, 10, 11], "CVTSD2USIR": [4, 8, 9, 10, 11],
    "CVTTRSS2SI": [4, 8], "CVTTRSD2SI": [4, 8],
    "CVTTSS2USIR": [4, 8], "CVTTSD2USIR": [4, 8],
    "CMPSSK": [0, 1, 2, 4, 5, 6, 7],
    "FPCLASSS": [1, 2, 8, 128, 255],
    "ROUND": [0, 1, 2, 3, 8, 9, 10, 11],
    "PERM": [0, 27, 85, 170, 255],
    "SHUFPS": [0, 27, 85, 170, 255],
    "SHUFX4": [0, 27, 85, 170, 255],
    "ALIGN": [0, 1, 2, 5],
    "TERNLOG": [0, 0x1B, 0x96, 0xC6, 0xFF],
    "RANGE": [0, 1, 2, 3, 5, 10, 15],
    "CVTRSD2SS": [4, 8, 9, 10, 11],
    "ADDR": [4, 8, 9, 10, 11], "SUBR": [4, 8, 9, 10, 11],
    "MULR": [4, 8, 9, 10, 11], "DIVR": [4, 8, 9, 10, 11],
    "MINR": [4, 8], "MAXR": [4, 8],
    "SQRTR": [4, 8, 9, 10, 11], "SCALEFR": [4, 8, 9, 10, 11],
    "GETEXPR": [4, 8],
    "CVTRDQ2PS": [4, 8, 9, 10, 11], "CVTREPI64PD": [4, 8, 9, 10, 11],
    "CVTREPI64PS": [4, 8, 9, 10, 11], "CVTREPU32PS": [4, 8, 9, 10, 11],
    "CVTREPU64PD": [4, 8, 9, 10, 11], "CVTREPU64PS": [4, 8, 9, 10, 11],
    "CVTRPD2DQ": [4, 8, 9, 10, 11], "CVTRPD2QQ": [4, 8, 9, 10, 11],
    "CVTRPD2UDQ": [4, 8, 9, 10, 11], "CVTRPD2UQQ": [4, 8, 9, 10, 11],
    "CVTRPD2PS": [4, 8, 9, 10, 11], "CVTRPS2DQ": [4, 8, 9, 10, 11],
    "CVTRPS2QQ": [4, 8, 9, 10, 11], "CVTRPS2UDQ": [4, 8, 9, 10, 11],
    "CVTRPS2UQQ": [4, 8, 9, 10, 11], "CVTRPS2PD": [4, 8],
    "CVTTRPD2DQ": [4, 8], "CVTTRPD2QQ": [4, 8],
    "CVTTRPS2DQ": [4, 8], "CVTTRPS2UDQ": [4, 8],
    "CVTTRPS2QQ": [4, 8], "CVTTRPS2UQQ": [4, 8],
    "CVTTRPD2UDQ": [4, 8], "CVTTRPD2UQQ": [4, 8],
    "FMADDR": [4, 8, 9, 10, 11], "FMSUBR": [4, 8, 9, 10, 11],
    "FNMADDR": [4, 8, 9, 10, 11], "FNMSUBR": [4, 8, 9, 10, 11],
    "FMADDSUBR": [4, 8, 9, 10, 11],
    "FMSUBADDR": [4, 8, 9, 10, 11],
    "REDUCE": [0, 1, 2, 3, 9],
    "ROL": [0, 1, 7, 31], "ROR": [0, 1, 7, 31],
    "BLEND": [0, 5, 10, 15],
    "FPCLASS": [0, 1, 6, 0x42],
    "ROUNDSCALE": [0, 1, 2, 3, 8, 9, 10, 11],
    "PEXTR": [0, 1, 3],
    "PINSR": [0, 1, 3],
    "SLLI": [0, 1, 7, 15, 31, 63],
    "SRLI": [0, 1, 7, 15, 31, 63],
    "SRAI": [0, 1, 7, 15, 31, 63],
    "ALIGNR": [0, 1, 7, 16, 31],
    "BLEND": [0, 5, 10, 15],
    "DPPS": [0, 0x31, 0xFF],
    "INSERTPS": [0, 0x1E, 0xD2],
    "MPSAD": [0, 1, 4, 5],
    "PERMILPS": [0x00, 0x1B, 0xFF],
    "PERMILPD": [0x00, 0x1B, 0xFF],
    "PERM2F128": [0x00, 0x21, 0x88, 0x31],
    "PERM2I128": [0x00, 0x21, 0x88, 0x31],
    "PERMQ": [0x00, 0x1B, 0xE4],
    "PERMPD": [0x00, 0x1B, 0xE4],
    "CVTPS2PH": [0, 3],
    "SLLDQ": [0, 1, 15, 16],
    "SRLDQ": [0, 1, 15, 16],
}

KPATS = [0xAAAAAAAAAAAAAAAA, 0x5555555555555555,
         0x3C3C3C3C3C3C3C3C, 0x0F0F0F0F0F0F0F0F,
         0xFF00FF00FF00FF00, 0x0000FFFF0000FFFF]


def gen_probes(shipped, shipped_m, midx, idx):
    areas = {}
    counters = {}
    NOPS = {"op1": 1, "op1i": 1, "op2": 2, "op2i": 2,
            "op3": 3, "op3i": 3, "opm": 2}
    HASIMM = ("op1i", "op2i", "op3i")

    def ref_code(e, elem, uelem, es, tot, imm):
        if "refblock" in e:
            code = REFBLOCKS[e["refblock"]]
        else:
            expr = e["ref"].replace("a[", "av[") \
                           .replace("b[", "bv[") \
                           .replace("c[", "cv[")
            code = ("for (i = 0; i < LANES; i++)\n"
                    "    e_[i] = (%s);" % expr)
        if e.get("evrnd"):
            # the reference runs under the MXCSR rounding the
            # immediate selects (the instruction itself uses
            # embedded rounding); the current-direction bit
            # leaves the ambient mode alone
            code = ("{ static const int rm_[4] = { FE_TONEAREST,"
                    " FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO };\n"
                    "  if (!({IMM} & 4)) fesetround(rm_[{IMM} & 3]);\n"
                    "  " + code + "\n"
                    "  fesetround(FE_TONEAREST); }")
        oes = e.get("oes", es)
        return code.replace("{ESB}", str(es * 8)) \
                   .replace("{OESB}", str(oes * 8)) \
                   .replace("{RTOT}", str(e.get("otot") or tot)) \
                   .replace("{IMM}", str(imm)) \
                   .replace("OUELEM", ELEM_C.get(("u", oes),
                                                 uelem)) \
                   .replace("OFELEM", ELEM_C.get(("f", oes),
                                                 elem)) \
                   .replace("OELEM", ELEM_C.get(("s", oes),
                                                elem)) \
                   .replace("UELEM", uelem) \
                   .replace("ELEM", elem) \
                   .replace("SQRT", "sqrtf" if es == 4 else "sqrt") \
                   .replace("FMA", "fmaf" if es == 4 else "fma")

    for ei, e in enumerate(OPS):
        if e["fam"] == "hdr" or not e.get("style"):
            continue
        if "ref" not in e and "refblock" not in e:
            continue
        style = e["style"]
        if style not in NOPS:
            continue
        area = e.get("area", "misc")
        nops = NOPS[style]
        scalar2 = "SCALAR2" in e.get("flags", ())
        for es, fl, tot in entry_shapes(e):
            if (ei, es, fl, tot) not in shipped or tot == 8:
                continue
            for name, kind, kes, ktot in intrinsic_names(
                    e, es, fl, tot):
                variants = [("base", name)]
                if e.get("m") and e.get("kind") not in (
                        "SCALAR",) and "imm_fixed" not in e:
                    for variant in ("z", "m"):
                        if (ei, es, fl, tot, variant) in shipped_m:
                            variants.append(
                                (variant,
                                 masked_name(name, variant)))
                if e.get("kind") in ("CVT", "CVT2"):
                    oes = e.get("oes", es)
                    rtot = e.get("otot") or tot
                    rt = vec_type_name(kind, oes, rtot)
                    okinds = [k for k, s in
                              parse_types(e["types"]) if s == es]
                    okind = "f" if fl else \
                        ("s" if "s" in okinds else "u")
                    ort = vec_type_name(okind, es, tot)
                    elem = ELEM_C[(okind, es)]
                    relem = ELEM_C[(kind, oes)]
                    rlanes = rtot // oes
                    rfl = kind == "f"
                else:
                    rt = vec_type_name(kind, es, tot)
                    ort = rt
                    elem = ELEM_C[(kind, es)]
                    relem = elem
                    rlanes = tot // es
                    rfl = fl
                uelem = UNSIGNED_C[elem]
                lanes = tot // es
                imms = [e["imm_fixed"]] \
                    if "imm_fixed" in e else \
                    IMM_SETS.get(e["op"], [0]) \
                    if style in HASIMM else [0]
                if e.get("imm_mode") == "index":
                    imms = sorted({i % lanes for i in imms})
                elif e.get("imm_mode") == "mask":
                    imms = sorted({i & ((1 << lanes) - 1)
                                   for i in imms})
                for variant, vname in variants:
                    for imm in imms:
                        cn = counters.get(area, 0)
                        counters[area] = cn + 1
                        masked = variant in ("z", "m")
                        merging = variant == "m" and \
                            e.get("kind") != "MASK"
                        L = []
                        L.append("static void case_%d(void) {"
                                 % cn)
                        if e.get("kind") == "SCALAR":
                            L.append(
                                "    union { %s v; %s e[%d]; } ua;"
                                % (rt, elem, lanes))
                            two = style in ("op2", "op2i")
                            if two:
                                L.append(
                                    "    union { %s v; %s e[%d]; }"
                                    " ub;" % (rt, elem, lanes))
                            if fl:
                                fill = "fill_doubles" if es == 8 \
                                    else "fill_floats"
                                L.append("    %s(ua.e, %d, %d);"
                                         % (fill, lanes, cn % 5))
                                if two:
                                    L.append("    %s(ub.e, %d, %d);"
                                             % (fill, lanes,
                                                (cn + 2) % 5))
                            else:
                                L.append(
                                    "    fill_bytes(ua.e, sizeof"
                                    " ua.e, %d);" % (cn % 7))
                                if two:
                                    L.append(
                                        "    fill_bytes(ub.e, sizeof"
                                        " ub.e, %d);"
                                        % ((cn + 3) % 7))
                            call = "%s(ua.v" % vname
                            if two:
                                call += ", ub.v"
                            if style in HASIMM and \
                                    "imm_fixed" not in e:
                                call += ", %d" % imm
                            call += ")"
                            L.append("    long long r_ = %s;"
                                     % call)
                            L.append("    print_hex(%d, &r_,"
                                     " sizeof r_);" % cn)
                            if e.get("refblock") == "none":
                                L.append("}")
                                areas.setdefault(area, []).append(
                                    "\n".join(L))
                                continue
                            code = ref_code(e, elem, uelem, es, tot,
                                            imm)
                            code = code.replace("{TOT}", str(tot))
                            code = code.replace("LANES",
                                                str(lanes))
                            L.append("    { const void *av = ua.e;")
                            if two:
                                L.append("      const void *bv ="
                                         " ub.e;")
                            L.append("      int i; long long want_;")
                            L.append("      " + code.replace(
                                "\n", "\n      "))
                            L.append("      if (r_ != want_) "
                                     "mismatch(\"%s\"); }" % vname)
                            L.append("}")
                            areas.setdefault(area, []).append(
                                "\n".join(L))
                            continue
                        L.append("    int i;")
                        uns = ["ua", "ub", "uc"][:nops]
                        if scalar2:
                            uns = ["ua"]
                        # a {X} count operand is always an
                        # __m128i, whatever the result width
                        xcount = any("{X}" in t
                                     for t in e["asm"])
                        for u in uns:
                            urt = vec_type_name(kind, es, 16) \
                                if xcount and u == "ub" else ort
                            if e.get("atype") and u == "ua":
                                # the first operand's C type
                                # is an integer vector (the
                                # permutexvar index)
                                urt = vec_type_name(
                                    e["atype"][0], e["atype"][1],
                                    tot)
                            if e.get("btype") and u == "ub":
                                # the second operand's C type
                                # is an integer vector (the
                                # permutevar index)
                                urt = vec_type_name(
                                    e["btype"][0], e["btype"][1],
                                    tot)
                            if e.get("ctype") and u == "uc":
                                # the third operand's C type
                                # is an integer vector (the
                                # fixup table)
                                urt = vec_type_name(
                                    e["ctype"][0], e["ctype"][1],
                                    tot)
                            L.append(
                                "    union { %s v; %s e[%d]; } %s;"
                                % (urt, elem,
                                   lanes if urt == ort
                                   or e.get("btype")
                                   or e.get("ctype")
                                   or e.get("atype")
                                   else 16 // es,
                                   u))
                        if merging:
                            L.append(
                                "    union { %s v; %s e[%d]; } us;"
                                % (rt, relem, rlanes))
                        if scalar2:
                            L.append(
                                "    long long bs_ = (long long)"
                                "(0x9E3779B97F4A7C15ULL * %dULL);"
                                % (cn + 1))
                        seeds = {"ua": 0, "ub": 3, "uc": 5,
                                 "us": 1}
                        for u in uns + (["us"] if merging
                                        else []):
                            sd = seeds[u]
                            if u == "us":
                                # the passthrough has the
                                # result's element type; fill
                                # its bytes, the merge is per
                                # result lane either way
                                L.append(
                                    "    fill_bytes(%s.e, sizeof"
                                    " %s.e, %d);"
                                    % (u, u, (cn + sd) % 7))
                            elif fl:
                                fill = "fill_doubles" if es == 8 \
                                    else "fill_floats"
                                L.append("    %s(%s.e, %d, %d);"
                                         % (fill, u,
                                            rlanes if u == "us"
                                            else lanes,
                                            (cn + sd) % 5))
                            else:
                                L.append(
                                    "    fill_bytes(%s.e, sizeof"
                                    " %s.e, %d);"
                                    % (u, u, (cn + sd) % 7))
                        if masked or style == "opm":
                            L.append(
                                "    unsigned long long k_ ="
                                " 0x%016xULL;"
                                % KPATS[cn % len(KPATS)])
                            if e.get("ssmask") and \
                                    not e.get("mpass"):
                                # a scalar form's writemask
                                # consults bit 0 alone; the
                                # lanes above it come from the
                                # first source, so the model
                                # (and the merging move in the
                                # back end) need them selected
                                L.append(
                                    "    k_ |= ~1ULL;")
                        if e.get("kind") == "MASK":
                            call = vname + "("
                            args = []
                            if variant == "m":
                                args.append("k_")
                            args += ["%s.v" % u for u in uns]
                            if style in HASIMM and \
                                    "imm_fixed" not in e:
                                args.append(str(imm))
                            call += ", ".join(args) + ")"
                            L.append(
                                "    unsigned long long r_ = %s;"
                                % call)
                            L.append("    print_hex(%d, &r_,"
                                     " sizeof r_);" % cn)
                            if e.get("refblock") == "none":
                                L.append("}")
                                areas.setdefault(
                                    area, []).append("\n".join(L))
                                continue
                            code = ref_code(e, elem, uelem, es, tot,
                                            imm)
                            code = code.replace("{TOT}", str(tot))
                            code = code.replace("LANES",
                                                str(lanes))
                            L.append("    { %s av[%d]; %s bv[%d];"
                                     " %s cv[%d];"
                                     % (elem, lanes, elem, lanes,
                                        elem, lanes))
                            L.append("      memcpy(av, ua.e,"
                                     " sizeof av);")
                            if nops >= 2:
                                L.append("      memcpy(bv, ub.e,"
                                         " sizeof bv);")
                            if nops >= 3:
                                L.append("      memcpy(cv, uc.e,"
                                         " sizeof cv);")
                            L.append("      unsigned long long"
                                     " want_;")
                            L.append("      " + code.replace(
                                "\n", "\n      "))
                            if variant == "m":
                                L.append("      want_ &= k_;")
                            L.append("      if (r_ != want_) "
                                     "mismatch(\"%s\"); }" % vname)
                            L.append("}")
                            areas.setdefault(area, []).append(
                                "\n".join(L))
                            continue
                        L.append("    union { %s v; %s e[%d]; }"
                                 " ur;" % (rt, relem, rlanes))
                        args = []
                        if style == "opm":
                            args.append("k_")
                        if merging and e.get("mpass"):
                            args += ["%s.v" % uns[0], "k_"]
                            args += ["%s.v" % u
                                     for u in uns[1:]]
                        elif merging:
                            args += ["us.v", "k_"]
                            args += ["%s.v" % u for u in uns]
                        elif masked:
                            args.append("k_")
                            args += ["%s.v" % u for u in uns]
                        else:
                            args += ["%s.v" % u for u in uns]
                        if scalar2:
                            args.append("bs_")
                        if style in HASIMM and \
                                "imm_fixed" not in e:
                            args.append(str(imm))
                        L.append("    ur.v = %s(%s);"
                                 % (vname, ", ".join(args)))
                        L.append("    print_hex(%d, &ur,"
                                 " sizeof ur);" % cn)
                        if e.get("refblock") == "none":
                            L.append("}")
                            areas.setdefault(area, []).append(
                                "\n".join(L))
                            continue
                        code = ref_code(e, elem, uelem, es, tot,
                                        imm)
                        code = code.replace("{TOT}", str(tot))
                        code = code.replace("LANES", str(lanes))
                        L.append("    { %s av[%d]; %s bv[%d];"
                                 " %s cv[%d]; %s e_[%d];"
                                 % (elem, lanes, elem, lanes,
                                    elem, lanes, relem, rlanes))
                        L.append("      memcpy(av, ua.e,"
                                 " sizeof av);")
                        if nops >= 2 and not scalar2:
                            L.append("      memcpy(bv, ub.e,"
                                     " sizeof %s);"
                                     % ("ub.e" if xcount else "bv"))
                        if nops >= 3:
                            L.append("      memcpy(cv, uc.e,"
                                     " sizeof cv);")
                        if merging:
                            L.append("      %s src_[%d];"
                                     % (relem, rlanes))
                            L.append("      memcpy(src_, %s,"
                                     " sizeof src_);"
                                     % ("ua.e" if e.get("mpass")
                                        else "us.e"))
                        L.append("      " + code.replace(
                            "\n", "\n      "))
                        if e.get("refblock") in (
                                "cvt_rint", "cvt_trunc",
                                "cvt_rintu", "cvt_truncu",
                                "cvtq2f", "cvtq2fu",
                                "cvt_f2f", "cvtps2ph",
                                "pmov", "pmovs", "pmovus") and rlanes > lanes:
                            # a narrowing convert leaves the
                            # result lanes past the converted
                            # ones at zero
                            L.append("      for (i = %d; i < %d;"
                                     " i++)" % (lanes, rlanes))
                            L.append("          e_[i] = 0;")
                        if masked:
                            L.append("      { %s base_[%d];"
                                     % (relem, rlanes))
                            L.append("        memcpy(base_, e_,"
                                     " sizeof base_);")
                            other = "src_[i]" if merging \
                                else "(%s)0" % relem
                            if e.get("ssmask"):
                                # a scalar form: only lane 0
                                # is writemasked; the lanes
                                # above it are the first
                                # source's, masked or not
                                other0 = "src_[0]" if merging \
                                    else "(%s)0" % relem
                                L.append(
                                    "        for (i = 0; i < %d;"
                                    " i++)" % rlanes)
                                L.append(
                                    "            e_[i] = i == 0"
                                    " ? ((k_ & 1) ? base_[0]"
                                    " : %s) : base_[i];"
                                    % other0)
                            else:
                                L.append(
                                    "        for (i = 0; i < %d;"
                                    " i++)" % rlanes)
                                L.append(
                                    "            e_[i] = ((k_ >>"
                                    " i) & 1) ? base_[i] : %s;"
                                    % other)
                            L.append("      }")
                        if masked and e.get("refblock") in (
                                "cvt_rint", "cvt_trunc",
                                "cvt_rintu", "cvt_truncu",
                                "cvtq2f", "cvtq2fu",
                                "cvt_f2f", "cvtps2ph",
                                "pmov", "pmovs", "pmovus") and rlanes > lanes:
                            # the lanes past the converted
                            # ones are zero in the result,
                            # masked or not
                            L.append("      for (i = %d; i < %d;"
                                     " i++)" % (lanes, rlanes))
                            L.append("          e_[i] = 0;")
                        if rfl:
                            L.append("      for (i = 0; i < %d;"
                                     " i++)" % rlanes)
                            L.append("          if (memcmp(&e_[i],"
                                     " &ur.e[i], sizeof e_[i]))")
                            L.append("              mismatch("
                                     "\"%s\"); }" % vname)
                        else:
                            L.append("      for (i = 0; i < %d;"
                                     " i++)" % rlanes)
                            L.append("          if (e_[i] != ur.e[i])")
                            L.append("              mismatch("
                                     "\"%s\"); }" % vname)
                        L.append("}")
                        areas.setdefault(area, []).append(
                            "\n".join(L))
    os.makedirs(PROBES, exist_ok=True)
    for area, cases in areas.items():
        lines = ["/* generated by tools/x86_gen.py; do not edit */",
                 '#include "x86_probe.h"', ""]
        lines.extend(cases)
        lines.append("")
        lines.append("int main(void) {")
        for i in range(len(cases)):
            lines.append("    case_%d();" % i)
        lines.append("    return fails != 0;")
        lines.append("}")
        with open(os.path.join(PROBES, "x86_%s.c" % area), "w") as f:
            f.write("\n".join(lines) + "\n")
    return areas


if __name__ == "__main__":
    ex, shipped, shipped_m, midx, idx = gen_tables()
    for line in ex:
        print("excluded:", line)
    hdr = gen_headers(shipped, shipped_m, midx, idx)
    areas = gen_probes(shipped, shipped_m, midx, idx)
    print("tables written (%d exclusions); headers: %s; "
          "probe areas: %s"
          % (len(ex), ", ".join(hdr),
             ", ".join("%s=%d" % (a, len(c))
                       for a, c in sorted(areas.items()))))
