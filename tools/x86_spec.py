# The x86 operation spec: one entry per (family, operation)
# variant.  Consumed by x86_gen.py, which derives encodings from
# the system assembler, emits the backend opcode tables, the
# intrinsic headers and the differential probes.
#
# Several entries may share one operation name: they are the
# variants (integer / float, or per-element-size template sets)
# of a single operation index.  The op2 and op2i families start
# with the operations the backend itself uses, in the order of
# the XOP2_* / XOP2I_* enums in x86_64-gen.c; x86_gen.py checks
# the two agree.
#
# Entry fields:
#   fam    'op1' | 'op1i' | 'op1m' | 'op2' | 'op2i' | 'op3' |
#          'op3i' | 'opm' and the masked variants of each (a
#          trailing 2 is the merging form, a trailing i an
#          immediate operand): 'op1m2' 'op1mi' 'op1mi2' 'opm2'
#          'opmi' 'opmi2' 'op3m' 'op3m2' 'op3mi' 'op3mi2'
#   op     operation name (becomes XOP<fam>_<op>)
#   stem   intrinsic name stem, e.g. 'add' (None: no header
#          macro from the naming rule)
#   names  explicit intrinsic list [(name, kind, es, total)];
#          overrides the stem naming rule
#   types  element kinds/sizes as groups: 's1248' = signed with
#          element sizes 1,2,4,8; kinds s u f
#   totals totals the op applies at: (16,), (8, 16, 32, 64), ...
#   asm    assembler templates, tried in order at each feature
#          level; placeholders {D} {A} {B} {S} vector registers,
#          {G} general register, {K} / {Kz} writemask, {Kd}
#          mask destination, {i} immediate, {ss} element suffix
#          b/w/d/q, {fs} float suffix ps/pd
#   kind   shape relation (XVK_*), default 'SAME'
#   form   encoding form (XVF_*); derived from the template
#          when not given
#   feat   lowest feature level to try (a TFEAT name without
#          the prefix), default 'SSE'
#   imm    (lo, hi) immediate range
#   evrnd  the immediate is an EVEX embedded-rounding selector
#   flags  XVFL_* names, e.g. ('SCALAR2',)
#   oes    result element size / COMI condition, when it
#          differs from the operand's
#   header header file key the macros go to ('xmmintrin', ...)
#   style  header macro shape: 'op1' 'op1i' 'op2' 'op2i' 'op3'
#          'op3i' 'splat' 'extract' 'custom'
#   macro  custom header macro text (style 'custom' or a plain
#          composition); {a} {b} {c} {d} are the arguments
#   m      derive the writemask variants (maskz and merging)
#          of this entry into the masked families
#   ref    scalar reference expression for the probes, in terms
#          of a[i], b[i], c[i]
#   refblock  C statements computing the expected array e[]
#   area   probe area (file) this op's cases go to

OPS = []

def _add(fam, op, stem, types, totals, asm, **kw):
    d = dict(fam=fam, op=op, stem=stem, types=types,
             totals=totals, asm=asm)
    d.update(kw)
    OPS.append(d)
    return d

def parse_types(spec):
    out = []
    for tok in spec.split():
        kind = tok[0]
        for ch in tok[1:]:
            out.append((kind, int(ch)))
    return out

INT = "s1248 u1248"
INT_NO8 = "s124 u124"

# ------------------------------------------------------------------
# op2: the backend's own operations come first, in XOP2_* order
# (ADD SUB MUL AND OR XOR CMPEQ CMPGT PSHUFB PUNPCKLQDQ
#  PUNPCKLDQ UNPCKLPD SLLV SRLV SRAV SLL SRL SRA DIV)
# ------------------------------------------------------------------
_add("op2", "ADD", "add", INT, (8, 16, 32, 64),
     ["padd{ss} {D}, {B}", "vpadd{ss} {D}, {A}, {B}"],
     feat="SSE2", style="op2", area="arith", ref="a[i] + b[i]", m=True)
_add("op2", "ADD", "add", "f48", (16, 32, 64),
     ["add{fs} {D}, {B}", "vadd{fs} {D}, {A}, {B}"],
     feat="SSE", style="op2", area="float", ref="a[i] + b[i]", m=True)
_add("op2", "SUB", "sub", INT, (8, 16, 32, 64),
     ["psub{ss} {D}, {B}", "vpsub{ss} {D}, {A}, {B}"],
     feat="SSE2", style="op2", area="arith", ref="a[i] - b[i]", m=True)
_add("op2", "SUB", "sub", "f48", (16, 32, 64),
     ["sub{fs} {D}, {B}", "vsub{fs} {D}, {A}, {B}"],
     feat="SSE", style="op2", area="float", ref="a[i] - b[i]", m=True)
# lane-wise low multiply exists only for 16 and 32 bit elements
_add("op2", "MUL", "mullo", "s24 u24", (8, 16, 32, 64),
     ["pmull{ss} {D}, {B}", "vpmull{ss} {D}, {A}, {B}"],
     feat="SSE2", style="op2", area="arith", ref="a[i] * b[i]", m=True)
_add("op2", "MUL", "mul", "f48", (16, 32, 64),
     ["mul{fs} {D}, {B}", "vmul{fs} {D}, {A}, {B}"],
     feat="SSE", style="op2", area="float", ref="a[i] * b[i]", m=True)
_add("op2", "AND", None, INT, (8, 16, 32, 64),
     ["pand {D}, {B}", "vpand {D}, {A}, {B}",
     "vpandd {D}, {A}, {B}"],
     feat="SSE2", area="logic",
     names=[("_mm_and_si128", "x", 8, 16),
            ("_mm256_and_si256", "x", 8, 32),
            ("_mm512_and_si512", "x", 8, 64),
            ],
     style="op2", ref="a[i] & b[i]")
_add("op2", "AND", "and", "f48", (16, 32, 64),
     ["and{fs} {D}, {B}", "vand{fs} {D}, {A}, {B}"],
     feat="SSE", style="op2", area="float", refblock="bitand", m=True)
_add("op2", "OR", None, INT, (8, 16, 32, 64),
     ["por {D}, {B}", "vpor {D}, {A}, {B}",
     "vpord {D}, {A}, {B}"],
     feat="SSE2", area="logic",
     names=[("_mm_or_si128", "x", 8, 16),
            ("_mm256_or_si256", "x", 8, 32),
            ("_mm512_or_si512", "x", 8, 64),
            ],
     style="op2", ref="a[i] | b[i]")
_add("op2", "OR", "or", "f48", (16, 32, 64),
     ["or{fs} {D}, {B}", "vor{fs} {D}, {A}, {B}"],
     feat="SSE", style="op2", area="float", refblock="bitor", m=True)
_add("op2", "XOR", None, INT, (8, 16, 32, 64),
     ["pxor {D}, {B}", "vpxor {D}, {A}, {B}",
     "vpxord {D}, {A}, {B}"],
     feat="SSE2", area="logic",
     names=[("_mm_xor_si128", "x", 8, 16),
            ("_mm256_xor_si256", "x", 8, 32),
            ("_mm512_xor_si512", "x", 8, 64),
            ],
     style="op2", ref="a[i] ^ b[i]")
_add("op2", "XOR", "xor", "f48", (16, 32, 64),
     ["xor{fs} {D}, {B}", "vxor{fs} {D}, {A}, {B}"],
     feat="SSE", style="op2", area="float", refblock="bitxor", m=True)
_add("op2", "CMPEQ", "cmpeq", "s1248 u1248", (8, 16, 32, 64),
     ["pcmpeq{ss} {D}, {B}", "vpcmpeq{ss} {D}, {A}, {B}"],
     feat="SSE2", style="op2", area="cmp",
     ref="(a[i] == b[i]) ? -1 : 0")
_add("op2", "CMPGT", "cmpgt", "s1248", (8, 16, 32, 64),
     ["pcmpgt{ss} {D}, {B}", "vpcmpgt{ss} {D}, {A}, {B}"],
     feat="SSE2", style="op2", area="cmp",
     ref="(a[i] > b[i]) ? -1 : 0")
_add("op2", "PSHUFB", "shuffle", "s1 u1", (16, 32, 64),
     ["pshufb {D}, {B}", "vpshufb {D}, {A}, {B}"],
     feat="SSSE3", style="op2", area="permute", refblock="pshufb", m=True)
_add("op2", "PUNPCKLQDQ", None, "s8", (16, 32, 64),
     ["punpcklqdq {D}, {B}", "vpunpcklqdq {D}, {A}, {B}"],
     feat="SSE2", kind="BYTES", area="permute", refblock="unpcklqdq")
_add("op2", "PUNPCKLDQ", None, "s4", (16, 32, 64),
     ["punpckldq {D}, {B}", "vpunpckldq {D}, {A}, {B}"],
     feat="SSE2", kind="BYTES", area="permute", refblock="unpckldq")
_add("op2", "UNPCKLPD", None, "f8", (16, 32, 64),
     ["unpcklpd {D}, {B}", "vunpcklpd {D}, {A}, {B}"],
     feat="SSE2", kind="BYTES", area="permute", refblock="unpcklpd")
_add("op2", "SLLV", "sllv", "s248 u248", (16, 32, 64),
     ["vpsllv{ss} {D}, {A}, {B}"],
     feat="AVX2", style="op2", area="shift", refblock="sllv", m=True)
_add("op2", "SRLV", "srlv", "s248 u248", (16, 32, 64),
     ["vpsrlv{ss} {D}, {A}, {B}"],
     feat="AVX2", style="op2", area="shift", refblock="srlv", m=True)
_add("op2", "SRAV", "srav", "s2", (16, 32, 64),
     ["vpsrav{ss} {D}, {A}, {B}"],
     feat="AVX512BW", style="op2", area="shift", refblock="srav", m=True)
_add("op2", "SRAV", "srav", "s8", (16, 32, 64),
     ["vpsrav{ss} {D}, {A}, {B}"],
     feat="AVX512F", style="op2", area="shift", refblock="srav", m=True)
_add("op2", "SRAV", "srav", "s4", (16, 32, 64),
     ["vpsrav{ss} {D}, {A}, {B}"],
     feat="AVX2", style="op2", area="shift", refblock="srav", m=True)
_add("op2", "SLL", "sll", "s248 u248", (8, 16, 32, 64),
     ["psll{ss} {D}, {B}", "vpsll{ss} {D}, {A}, {X}"],
     feat="SSE2", style="op2", area="shift", refblock="sllc",
     flags=("COUNT",), m=True)
_add("op2", "SRL", "srl", "s248 u248", (8, 16, 32, 64),
     ["psrl{ss} {D}, {B}", "vpsrl{ss} {D}, {A}, {X}"],
     feat="SSE2", style="op2", area="shift", refblock="srlc",
     flags=("COUNT",), m=True)
_add("op2", "SRA", "sra", "s24", (8, 16, 32, 64),
     ["psra{ss} {D}, {B}", "vpsra{ss} {D}, {A}, {X}"],
     feat="SSE2", style="op2", area="shift", refblock="srac",
     flags=("COUNT",), m=True)
_add("op2", "SRA", "sra", "s8", (16, 32, 64),
     ["vpsra{ss} {D}, {A}, {X}"],
     feat="AVX512F", style="op2", area="shift", refblock="srac",
     flags=("COUNT",), m=True)
_add("op2", "DIV", "div", "f48", (16, 32, 64),
     ["div{fs} {D}, {B}", "vdiv{fs} {D}, {A}, {B}"],
     feat="SSE", style="op2", area="float", ref="a[i] / b[i]", m=True)

# ------------------------------------------------------------------
# op1i: PSHUFD, PSHUFLW head the family (XOP1I_* order); the
# backend's splat paths look these up in the op1i table
# ------------------------------------------------------------------
_add("op1i", "PSHUFD", "shuffle", "s4 u4", (16, 32, 64),
     ["pshufd {D}, {A}, {i}", "vpshufd {D}, {A}, {i}"],
     feat="SSE2", style="op1i", imm=(0, 255), area="permute",
     refblock="pshufd", m=True)
_add("op1i", "PSHUFLW", "shufflelo", "s2 u2", (16, 32, 64),
     ["pshuflw {D}, {A}, {i}", "vpshuflw {D}, {A}, {i}"],
     feat="SSE2", style="op1i", imm=(0, 255), area="permute",
     refblock="pshuflw", m=True)

# ------------------------------------------------------------------
# op2i: CMPP heads the family (XOP2I_* order)
# ------------------------------------------------------------------
_add("op2i", "CMPP", "cmp", "f48", (16, 32, 64),
     ["cmp{fs} {D}, {B}, {i}", "vcmp{fs} {D}, {A}, {B}, {i}"],
     feat="SSE", style="op2i", imm=(0, 31), area="cmp",
     refblock="cmpp")

# ------------------------------------------------------------------
# op1 starter set
# ------------------------------------------------------------------
_add("op1", "SQRT", "sqrt", "f48", (16, 32, 64),
     ["sqrt{fs} {D}, {A}", "vsqrt{fs} {D}, {A}"],
     feat="SSE", style="op1", area="float", refblock="fsqrt", m=True)
_add("op1", "RSQRT", "rsqrt", "f4", (16, 32, 64),
     ["rsqrtps {D}, {A}", "vrsqrtps {D}, {A}"],
     feat="SSE", style="op1", area="float", refblock="none")
_add("op1", "RCP", "rcp", "f4", (16, 32, 64),
     ["rcpps {D}, {A}", "vrcpps {D}, {A}"],
     feat="SSE", style="op1", area="float", refblock="none")
_add("op1", "ABS", "abs", "s124", (16, 32, 64),
     ["pabs{ss} {D}, {A}", "vpabs{ss} {D}, {A}"],
     feat="SSSE3", style="op1", area="arith", refblock="abs", m=True)
_add("op1", "MOVMSK", "movemask", "s1", (16,),
     ["pmovmskb {G}, {A}"],
     feat="SSE2", kind="SCALAR", oes=4, style="op1", area="misc",
     refblock="movmskb")
_add("op1", "MOVMSK", "movemask", "f4", (16,),
     ["movmskps {G}, {A}"],
     feat="SSE", kind="SCALAR", oes=4, style="op1", area="misc",
     refblock="movmskps")
_add("op1", "MOVMSK", "movemask", "f8", (16,),
     ["movmskpd {G}, {A}"],
     feat="SSE2", kind="SCALAR", oes=4, style="op1", area="misc",
     refblock="movmskpd")

# ------------------------------------------------------------------
# op1i starter set
# ------------------------------------------------------------------
_add("op1i", "ROUND", "round", "f48", (16, 32),
     ["round{fs} {D}, {A}, {i}", "vround{fs} {D}, {A}, {i}"],
     feat="SSE41", style="op1i", imm=(0, 15), area="float",
     refblock="fround")
_add("op1i", "ROUNDSCALE", "roundscale", "f48", (16, 32, 64),
     ["vrndscale{fs} {D}, {A}, {i}"],
     feat="AVX512F", style="op1i", imm=(0, 255), area="float",
     refblock="fround", m=True)
_add("op1", "RCP14", "rcp14", "f4", (16, 32, 64),
     ["vrcp14ps {D}, {A}"],
     feat="AVX512F", style="op1", area="float", refblock="none",
     m=True)
_add("op1", "RSQRT14", "rsqrt14", "f4", (16, 32, 64),
     ["vrsqrt14ps {D}, {A}"],
     feat="AVX512F", style="op1", area="float", refblock="none",
     m=True)

# ------------------------------------------------------------------
# SSE / SSE2 / SSSE3 / SSE4 completion
# ------------------------------------------------------------------
# saturating and averaging arithmetic
for _op, _stem, _blk in (("ADDS", "adds", "adds"),
                         ("SUBS", "subs", "subs")):
    _add("op2", _op, _stem, "s12", (16, 32, 64),
         ["p%s{ss} {D}, {B}" % _stem, "vp%s{ss} {D}, {A}, {B}" % _stem],
         feat="SSE2", style="op2", area="arith", refblock=_blk,
         m=True)
    _add("op2", {"ADDS": "ADDUS", "SUBS": "SUBUS"}[_op],
         _stem, "u12", (16, 32, 64),
         ["p%s{ss} {D}, {B}" % {"adds": "addus",
                               "subs": "subus"}[_stem],
          "vp%s{ss} {D}, {A}, {B}" % {"adds": "addus",
                                     "subs": "subus"}[_stem]],
         feat="SSE2", style="op2", area="arith",
         refblock=_blk.replace("adds", "addus").replace(
             "subs", "subus"),
         signed_names=True, m=True)
_add("op2", "AVG", "avg", "u12", (16, 32, 64),
     ["pavg{ss} {D}, {B}", "vpavg{ss} {D}, {A}, {B}"],
     feat="SSE2", style="op2", area="arith",
     ref="((a[i] + b[i] + 1) >> 1)", signed_names=True, m=True)
_add("op2", "MULHI", "mulhi", "s2", (16, 32, 64),
     ["pmulh{ss} {D}, {B}", "vpmulh{ss} {D}, {A}, {B}"],
     feat="SSE2", style="op2", area="arith",
     ref="((a[i] * b[i]) >> 16)", m=True)
_add("op2", "MULHU", "mulhi", "u2", (16, 32, 64),
     ["pmulhu{ss} {D}, {B}", "vpmulhu{ss} {D}, {A}, {B}"],
     feat="SSE2", style="op2", area="arith",
     ref="((a[i] * b[i]) >> 16)", signed_names=True, m=True)
_add("op2", "MULUDQ", None, "u4", (16, 32, 64),
     ["pmuludq {D}, {B}", "vpmuludq {D}, {A}, {B}"],
     feat="SSE2", kind="CVT", oes=8, style="op2", area="arith",
     names=[("_mm_mul_epu32", "u", 4, 16),
            ("_mm256_mul_epu32", "u", 4, 32),
            ("_mm512_mul_epu32", "u", 4, 64)],
     refblock="muludq", m=True)
_add("op2", "MULDQ", None, "s4", (16, 32, 64),
     ["pmuldq {D}, {B}", "vpmuldq {D}, {A}, {B}"],
     feat="SSE41", kind="CVT", oes=8, style="op2", area="arith",
     names=[("_mm_mul_epi32", "s", 4, 16),
            ("_mm256_mul_epi32", "s", 4, 32),
            ("_mm512_mul_epi32", "s", 4, 64)],
     refblock="muldq", m=True)
_add("op2", "PMADDWD", "madd", "s2", (16, 32, 64),
     ["pmaddwd {D}, {B}", "vpmaddwd {D}, {A}, {B}"],
     feat="SSE2", kind="CVT", oes=4, style="op2", area="arith",
     refblock="pmaddwd", m=True)
_add("op2", "PMADDUBSW", None, "s1", (16, 32, 64),
     ["pmaddubsw {D}, {B}", "vpmaddubsw {D}, {A}, {B}"],
     feat="SSSE3", kind="CVT", oes=2, style="op2", area="arith",
     names=[("_mm_maddubs_epi16", "s", 1, 16),
            ("_mm256_maddubs_epi16", "s", 1, 32),
            ("_mm512_maddubs_epi16", "s", 1, 64)],
     refblock="pmaddubsw", m=True)
_add("op2", "MULHRSW", "mulhrs", "s2", (16, 32, 64),
     ["pmulhrsw {D}, {B}", "vpmulhrsw {D}, {A}, {B}"],
     feat="SSSE3", style="op2", area="arith",
     ref="(((a[i] * b[i]) >> 14) + 1) >> 1", m=True)
_add("op2", "PSIGN", "sign", "s124", (16, 32),
     ["psign{ss} {D}, {B}", "vpsign{ss} {D}, {A}, {B}"],
     feat="SSSE3", style="op2", area="arith",
     ref="(b[i] < 0) ? (ELEM)(0 - (UELEM)a[i]) : "
         "(b[i] == 0) ? (ELEM)0 : a[i]",
     m=True)
# packs (the result element is half the operand's)
_add("op2", "PACKSS", "packs", "s2", (16, 32, 64),
     ["packsswb {D}, {B}", "vpacksswb {D}, {A}, {B}"],
     feat="SSE2", kind="CVT", oes=1, style="op2", area="permute",
     refblock="packss8", m=True)
_add("op2", "PACKSS", "packs", "s4", (16, 32, 64),
     ["packssdw {D}, {B}", "vpackssdw {D}, {A}, {B}"],
     feat="SSE2", kind="CVT", oes=2, style="op2", area="permute",
     refblock="packss16", m=True)
_add("op2", "PACKUS", "packus", "s2", (16, 32, 64),
     ["packuswb {D}, {B}", "vpackuswb {D}, {A}, {B}"],
     feat="SSE2", kind="CVT", oes=1, style="op2", area="permute",
     refblock="packus8", m=True)
_add("op2", "PACKUS", "packus", "s4", (16, 32, 64),
     ["packusdw {D}, {B}", "vpackusdw {D}, {A}, {B}"],
     feat="SSE41", kind="CVT", oes=2, style="op2", area="permute",
     refblock="packus16", m=True)
# unpacks: the starter's three entries gain their header names;
# the rest of the family follows
for _e in OPS:
    if _e["op"] == "PUNPCKLQDQ":
        _e.update(style="op2", refblock="unpckl", m=True,
                  names=[("_mm_unpacklo_epi64", "s", 8, 16),
                         ("_mm256_unpacklo_epi64", "s", 8, 32),
                         ("_mm512_unpacklo_epi64", "s", 8, 64)])
    elif _e["op"] == "PUNPCKLDQ":
        _e.update(style="op2", refblock="unpckl", m=True,
                  names=[("_mm_unpacklo_epi32", "s", 4, 16),
                         ("_mm256_unpacklo_epi32", "s", 4, 32),
                         ("_mm512_unpacklo_epi32", "s", 4, 64)])
    elif _e["op"] == "UNPCKLPD":
        _e.update(style="op2", refblock="unpckl", m=True,
                  names=[("_mm_unpacklo_pd", "f", 8, 16),
                         ("_mm256_unpacklo_pd", "f", 8, 32),
                         ("_mm512_unpacklo_pd", "f", 8, 64)])
for _op, _insn, _es, _nm in (
        ("PUNPCKLBW", "punpcklbw", 1, "unpacklo_epi8"),
        ("PUNPCKHBW", "punpckhbw", 1, "unpackhi_epi8"),
        ("PUNPCKLWD", "punpcklwd", 2, "unpacklo_epi16"),
        ("PUNPCKHWD", "punpckhwd", 2, "unpackhi_epi16"),
        ("PUNPCKHDQ", "punpckhdq", 4, "unpackhi_epi32"),
        ("PUNPCKHQDQ", "punpckhqdq", 8, "unpackhi_epi64")):
    _add("op2", _op, None, "s%d" % _es, (16, 32, 64),
         ["%s {D}, {B}" % _insn, "v%s {D}, {A}, {B}" % _insn],
         feat="SSE2", kind="BYTES", style="op2", area="permute",
         names=[("_mm_" + _nm, "s", _es, 16),
                ("_mm256_" + _nm, "s", _es, 32),
                ("_mm512_" + _nm, "s", _es, 64)],
         refblock="unpckl" if "lo" in _nm else "unpckh", m=True)
for _op, _insn, _es, _nm, _blk in (
        ("UNPCKLPS", "unpcklps", 4, "unpacklo_ps", "unpckl"),
        ("UNPCKHPS", "unpckhps", 4, "unpackhi_ps", "unpckh"),
        ("UNPCKHPD", "unpckhpd", 8, "unpackhi_pd", "unpckh")):
    _add("op2", _op, None, "f%d" % _es, (16, 32, 64),
         ["%s {D}, {B}" % _insn, "v%s {D}, {A}, {B}" % _insn],
         feat="SSE", kind="BYTES", style="op2", area="permute",
         names=[("_mm_" + _nm, "f", _es, 16),
                ("_mm256_" + _nm, "f", _es, 32),
                ("_mm512_" + _nm, "f", _es, 64)],
         refblock=_blk, m=True)
# andnot, min, max
_add("op2", "ANDNOT", None, INT, (8, 16, 32, 64),
     ["pandn {D}, {B}", "vpandn {D}, {A}, {B}",
      "vpandnd {D}, {A}, {B}"],
     feat="SSE2", area="logic",
     names=[("_mm_andnot_si128", "x", 8, 16),
            ("_mm256_andnot_si256", "x", 8, 32),
            ("_mm512_andnot_si512", "x", 8, 64),
            ],
     style="op2", refblock="bitandnot")
_add("op2", "ANDNOT", "andnot", "f48", (16, 32, 64),
     ["andn{fs} {D}, {B}", "vandn{fs} {D}, {A}, {B}"],
     feat="SSE", style="op2", area="float", refblock="bitandnot",
     m=True)
for _op, _cmp in (("MAX", ">"), ("MIN", "<")):
    _insn = _op.lower()
    _add("op2", _op, _insn, "s124", (16, 32, 64),
         ["p%ss{ss} {D}, {B}" % _insn,
          "vp%ss{ss} {D}, {A}, {B}" % _insn],
         feat="SSE2", style="op2", area="arith",
         ref="(a[i] %s b[i]) ? a[i] : b[i]" % _cmp, m=True)
    _add("op2", _op + "U", _insn, "u124", (16, 32, 64),
         ["p%su{ss} {D}, {B}" % _insn,
          "vp%su{ss} {D}, {A}, {B}" % _insn],
         feat="SSE2", style="op2", area="arith",
         ref="(a[i] %s b[i]) ? a[i] : b[i]" % _cmp,
         signed_names=True, m=True)
    _add("op2", _op, _insn, "f48", (16, 32, 64),
         ["%s{fs} {D}, {B}" % _insn,
          "v%s{fs} {D}, {A}, {B}" % _insn],
         feat="SSE", style="op2", area="float",
         ref="(a[i] %s b[i]) ? a[i] : b[i]" % _cmp, m=True)
# half-vector moves
_add("op2", "MOVEHL", None, "f4", (16,),
     ["movhlps {D}, {B}", "vmovhlps {D}, {A}, {B}"],
     feat="SSE", kind="BYTES", style="op2", area="permute",
     names=[("_mm_movehl_ps", "f", 4, 16)], refblock="movehl")
_add("op2", "MOVELH", None, "f4", (16,),
     ["movlhps {D}, {B}", "vmovlhps {D}, {A}, {B}"],
     feat="SSE", kind="BYTES", style="op2", area="permute",
     names=[("_mm_movelh_ps", "f", 4, 16)], refblock="movelh")

# ------------------------------------------------------------------
# immediate forms
# ------------------------------------------------------------------
_add("op2i", "SHUFPS", "shuffle", "f4", (16, 32, 64),
     ["shufps {D}, {B}, {i}", "vshufps {D}, {A}, {B}, {i}"],
     feat="SSE", style="op2i", imm=(0, 255), area="permute",
     refblock="shufps", m=True)
_add("op2i", "SHUFPD", "shuffle", "f8", (16, 32, 64),
     ["shufpd {D}, {B}, {i}", "vshufpd {D}, {A}, {B}, {i}"],
     feat="SSE2", style="op2i", imm=(0, 255), area="permute",
     refblock="shufpd", m=True)
_add("op2i", "BLEND", "blend", "f4", (16, 32),
     ["blendps {D}, {B}, {i}", "vblendps {D}, {A}, {B}, {i}"],
     feat="SSE41", style="op2i", imm=(0, 255), area="permute",
     imm_mode="mask", refblock="blend")
_add("op2i", "BLEND", "blend", "f8", (16, 32),
     ["blendpd {D}, {B}, {i}", "vblendpd {D}, {A}, {B}, {i}"],
     feat="SSE41", style="op2i", imm=(0, 255), area="permute",
     imm_mode="mask", refblock="blend")
_add("op2i", "BLENDW", "blend", "s2", (16, 32),
     ["pblendw {D}, {B}, {i}", "vpblendw {D}, {A}, {B}, {i}"],
     feat="SSE41", style="op2i", imm=(0, 255), area="permute",
     imm_mode="mask", refblock="blendw")
_add("op2i", "DPPS", "dp", "f48", (16, 32),
     ["dp{fs} {D}, {B}, {i}", "vdp{fs} {D}, {A}, {B}, {i}"],
     feat="SSE41", style="op2i", imm=(0, 255), area="float",
     refblock="dpps")
_add("op2i", "INSERTPS", "insert", "f4", (16,),
     ["insertps {D}, {B}, {i}", "vinsertps {D}, {A}, {B}, {i}"],
     feat="SSE41", style="op2i", imm=(0, 255), area="permute",
     refblock="insertps")
_add("op2i", "MPSAD", None, "u1", (16, 32),
     ["mpsadbw {D}, {B}, {i}", "vmpsadbw {D}, {A}, {B}, {i}"],
     feat="SSE41", kind="CVT", oes=2, style="op2i", imm=(0, 255),
     area="arith",
     names=[("_mm_mpsadbw_epu8", "u", 1, 16),
            ("_mm256_mpsadbw_epu8", "u", 1, 32)],
     refblock="none")
_add("op2i", "ALIGNR", "alignr", "s1", (16, 32, 64),
     ["palignr {D}, {B}, {i}", "vpalignr {D}, {A}, {B}, {i}"],
     feat="SSSE3", style="op2i", imm=(0, 255), area="permute",
     refblock="alignr", m=True)
_add("op1i", "SHUFHW", "shufflehi", "s2", (16, 32, 64),
     ["pshufhw {D}, {A}, {i}", "vpshufhw {D}, {A}, {i}"],
     feat="SSE2", style="op1i", imm=(0, 255), area="permute",
     refblock="pshufhw", m=True)
for _op, _stem, _blk in (("SLLI", "slli", "slli"),
                         ("SRLI", "srli", "srli")):
    _add("op1i", _op, _stem, "s248", (16, 32, 64),
         ["psll{ss} {A}, {i}" if _op == "SLLI" else
          "psrl{ss} {A}, {i}",
          "vpsll{ss} {D}, {A}, {i}" if _op == "SLLI" else
          "vpsrl{ss} {D}, {A}, {i}"],
         feat="SSE2", style="op1i", imm=(0, 255), area="shift",
         refblock=_blk, m=True)
_add("op1i", "SRAI", "srai", "s24", (16, 32, 64),
     ["psra{ss} {A}, {i}", "vpsra{ss} {D}, {A}, {i}"],
     feat="SSE2", style="op1i", imm=(0, 255), area="shift",
     refblock="srai", m=True)
_add("op1i", "SRAI", "srai", "s8", (16, 32, 64),
     ["vpsra{ss} {D}, {A}, {i}"],
     feat="AVX512F", style="op1i", imm=(0, 255), area="shift",
     refblock="srai", m=True)
for _op, _insn, _nstem, _blk in (
        ("SLLDQ", "pslldq", "slli", "slliq"),
        ("SRLDQ", "psrldq", "srli", "srliq")):
    _add("op1i", _op, None, "x8", (16, 32),
         ["%s {A}, {i}" % _insn, "v%s {D}, {A}, {i}" % _insn],
         feat="SSE2", kind="BYTES", style="op1i", imm=(0, 255),
         area="shift",
         names=[("_mm_%s_si128" % _nstem, "x", 8, 16),
                ("_mm256_%s_si256" % _nstem, "x", 8, 32),
                ("_mm256_b%s_epi128" % _nstem, "x", 8, 32)],
         refblock=_blk, m=True)
    _add("op1i", _op, None, "x8", (64,),
         ["v%s {D}, {A}, {i}" % _insn],
         feat="AVX512BW", kind="BYTES", style="op1i", imm=(0, 255),
         area="shift",
         names=[("_mm512_b%s_epi128" % _nstem, "x", 8, 64)],
         refblock=_blk)
# variable-mask blends (the legacy form's mask is implicit)
_add("op3", "BLENDVB", None, "s1", (16, 32),
     ["pblendvb {D}, {B}", "vpblendvb {D}, {A}, {B}, {S}"],
     feat="SSE41", style="op3", area="permute",
     names=[("_mm_blendv_epi8", "s", 1, 16),
            ("_mm256_blendv_epi8", "s", 1, 32)],
     refblock="blendv")
_add("op3", "BLENDV", None, "f4", (16, 32),
     ["blendvps {D}, {B}", "vblendvps {D}, {A}, {B}, {S}"],
     feat="SSE41", style="op3", area="permute",
     names=[("_mm_blendv_ps", "f", 4, 16),
            ("_mm256_blendv_ps", "f", 4, 32)],
     refblock="blendvf")
_add("op3", "BLENDV", None, "f8", (16, 32),
     ["blendvpd {D}, {B}", "vblendvpd {D}, {A}, {B}, {S}"],
     feat="SSE41", style="op3", area="permute",
     names=[("_mm_blendv_pd", "f", 8, 16),
            ("_mm256_blendv_pd", "f", 8, 32)],
     refblock="blendvf")
# lane extract / insert
_add("op1i", "PEXTR", None, "s124", (16,),
     ["pextr{ss} {Gd}, {A}, {i}"],
     feat="SSE2", kind="SCALAR", style="op1i", imm=(0, 255),
     imm_mode="index", area="misc",
     names=[("_mm_extract_epi8", "s", 1, 16),
            ("_mm_extract_epi16", "s", 2, 16),
            ("_mm_extract_epi32", "s", 4, 16)],
     refblock="pextr")
_add("op1i", "PEXTR", None, "s8", (16,),
     ["pextrq {G}, {A}, {i}"],
     feat="SSE41", kind="SCALAR", style="op1i", imm=(0, 255),
     imm_mode="index", area="misc",
     names=[("_mm_extract_epi64", "s", 8, 16)],
     refblock="pextr")
_add("op1i", "PEXTRPS", None, "f4", (16,),
     ["pextrd {Gd}, {A}, {i}"],
     feat="SSE41", kind="SCALAR", style="op1i", imm=(0, 255),
     imm_mode="index", area="misc",
     names=[("_mm_extract_ps", "f", 4, 16)],
     refblock="pextrps")
_add("op2i", "PINSR", None, "s124", (16,),
     ["pinsr{ss} {D}, {Gd}, {i}"],
     feat="SSE2", style="op2i", imm=(0, 255), area="misc",
     imm_mode="index", form="D_B_ACC", flags=("SCALAR2",),
     names=[("_mm_insert_epi8", "s", 1, 16),
            ("_mm_insert_epi16", "s", 2, 16),
            ("_mm_insert_epi32", "s", 4, 16)],
     refblock="pinsr")
_add("op2i", "PINSR", None, "s8", (16,),
     ["pinsrq {D}, {G}, {i}"],
     feat="SSE41", style="op2i", imm=(0, 255), area="misc",
     imm_mode="index", form="D_B_ACC", flags=("SCALAR2",),
     names=[("_mm_insert_epi64", "s", 8, 16)],
     refblock="pinsr")
# wide movemask forms
_add("op1", "MOVMSK", None, "s1", (32,),
     ["vpmovmskb {G}, {A}"],
     feat="AVX2", kind="SCALAR", oes=4, style="op1", area="misc",
     names=[("_mm256_movemask_epi8", "s", 1, 32)],
     refblock="movmskb32")
_add("op1", "MOVMSK", None, "f4", (32,),
     ["vmovmskps {G}, {A}"],
     feat="AVX", kind="SCALAR", oes=4, style="op1", area="misc",
     names=[("_mm256_movemask_ps", "f", 4, 32)],
     refblock="movmskps8")
_add("op1", "MOVMSK", None, "f8", (32,),
     ["vmovmskpd {G}, {A}"],
     feat="AVX", kind="SCALAR", oes=4, style="op1", area="misc",
     names=[("_mm256_movemask_pd", "f", 8, 32)],
     refblock="movmskpd4")
# scalar round forms and the ceil / floor fixed forms
_add("op2i", "ROUNDSS", None, "f4", (16,),
     ["roundss {D}, {B}, {i}", "vroundss {D}, {A}, {B}, {i}"],
     feat="SSE41", style="op2i", imm=(0, 15), area="float",
     names=[("_mm_round_ss", "f", 4, 16)], refblock="froundss")
_add("op2i", "ROUNDSD", None, "f8", (16,),
     ["roundsd {D}, {B}, {i}", "vroundsd {D}, {A}, {B}, {i}"],
     feat="SSE41", style="op2i", imm=(0, 15), area="float",
     names=[("_mm_round_sd", "f", 8, 16)], refblock="froundss")
for _op, _insn, _es, _tot, _pre in (
        ("ROUND", "ps", 4, 16, "_mm"), ("ROUND", "pd", 8, 16, "_mm"),
        ("ROUND", "ps", 4, 32, "_mm256"),
        ("ROUND", "pd", 8, 32, "_mm256")):
    for _fx, _iv in (("ceil", 2), ("floor", 1)):
        _add("op1i", _op, None, "f%d" % _es, (_tot,),
             [], feat="SSE41", style="op1i", imm=(0, 15),
             imm_fixed=_iv, area="float",
             names=[("%s_%s_%s" % (_pre, _fx, _insn),
                     "f", _es, _tot)])
for _op, _sfx, _es in (("ROUNDSS", "ss", 4), ("ROUNDSD", "sd", 8)):
    for _fx, _iv in (("ceil", 2), ("floor", 1)):
        _add("op2i", _op, None, "f%d" % _es, (16,),
             [], feat="SSE41", style="custom", area="float",
             args="a",
             macro="__builtin_ia32_vop2i({OPC}, (a), (a), %d,"
                   " {rt})" % _iv,
             names=[("_mm_%s_%s" % (_fx, _sfx), "f", _es, 16)])

# ------------------------------------------------------------------
# converts
# ------------------------------------------------------------------
# sign / zero extension (pmovsx / pmovzx)
for _sgn, _mid in (("sx", "cvtepi"), ("zx", "cvtepu")):
    for _ss, _ds, _sb, _db in (("b", "w", 1, 2), ("b", "d", 1, 4),
                               ("b", "q", 1, 8), ("w", "d", 2, 4),
                               ("w", "q", 2, 8), ("d", "q", 4, 8)):
        for _otot, _pre in ((16, "_mm"), (32, "_mm256"),
                            (64, "_mm512")):
            # the source holds result-lane-count elements of
            # the narrower size, in at least one 128-bit lane
            _tot = max(16, _otot * _sb // _db)
            _add("op1", "PMOV%s%s%s" % (_sgn.upper(), _ss.upper(),
                                        _ds.upper()),
                 None, "%s%d" % ("s" if _sgn == "sx" else "u", _sb),
                 (_tot,),
                 ["pmov%s%s%s {D}, {A}" % (_sgn, _ss, _ds),
                  "vpmov%s%s%s {D}, {A}" % (_sgn, _ss, _ds)],
                 feat="SSE41", kind="CVT", oes=_db, otot=_otot,
                 style="op1", area="cvt", refblock="cvt_widen",
                 names=[("%s_%s%d_epi%d" % (_pre, _mid, _sb * 8,
                                            _db * 8),
                         "s" if _sgn == "sx" else "u", _sb, _tot)],
                 m=True)
# packed converts between float and integer
_add("op1", "CVTPS2DQ", None, "f4", (16, 32, 64),
     ["cvtps2dq {D}, {A}", "vcvtps2dq {D}, {A}"],
     feat="SSE2", kind="CVT", oes=4, style="op1", area="cvt",
     names=[("_mm_cvtps_epi32", "s", 4, 16),
            ("_mm256_cvtps_epi32", "s", 4, 32),
            ("_mm512_cvtps_epi32", "s", 4, 64)],
     refblock="cvt_rint", m=True)
_add("op1", "CVTTPS2DQ", None, "f4", (16, 32, 64),
     ["cvttps2dq {D}, {A}", "vcvttps2dq {D}, {A}"],
     feat="SSE2", kind="CVT", oes=4, style="op1", area="cvt",
     names=[("_mm_cvttps_epi32", "s", 4, 16),
            ("_mm256_cvttps_epi32", "s", 4, 32),
            ("_mm512_cvttps_epi32", "s", 4, 64)],
     refblock="cvt_trunc", m=True)
_add("op1", "CVTPD2DQ", None, "f8", (16,),
     ["cvtpd2dq {D}, {A}", "vcvtpd2dq {D}, {A}"],
     feat="SSE2", kind="CVT", oes=4, style="op1", area="cvt",
     names=[("_mm_cvtpd_epi32", "s", 8, 16)],
     refblock="cvt_rint", m=True)
_add("op1", "CVTPD2DQ", None, "f8", (32,),
     ["vcvtpd2dq {D}, {A}"],
     feat="AVX", kind="CVT", oes=4, otot=16, style="op1",
     area="cvt",
     names=[("_mm256_cvtpd_epi32", "s", 8, 32)],
     refblock="cvt_rint", m=True)
_add("op1", "CVTPD2DQ", None, "f8", (64,),
     ["vcvtpd2dq {D}, {A}"],
     feat="AVX512F", kind="CVT", oes=4, otot=32, style="op1",
     area="cvt",
     names=[("_mm512_cvtpd_epi32", "s", 8, 64)],
     refblock="cvt_rint", m=True)
_add("op1", "CVTTPD2DQ", None, "f8", (16,),
     ["cvttpd2dq {D}, {A}", "vcvttpd2dq {D}, {A}"],
     feat="SSE2", kind="CVT", oes=4, style="op1", area="cvt",
     names=[("_mm_cvttpd_epi32", "s", 8, 16)],
     refblock="cvt_trunc", m=True)
_add("op1", "CVTTPD2DQ", None, "f8", (32,),
     ["vcvttpd2dq {D}, {A}"],
     feat="AVX", kind="CVT", oes=4, otot=16, style="op1",
     area="cvt",
     names=[("_mm256_cvttpd_epi32", "s", 8, 32)],
     refblock="cvt_trunc", m=True)
_add("op1", "CVTTPD2DQ", None, "f8", (64,),
     ["vcvttpd2dq {D}, {A}"],
     feat="AVX512F", kind="CVT", oes=4, otot=32, style="op1",
     area="cvt",
     names=[("_mm512_cvttpd_epi32", "s", 8, 64)],
     refblock="cvt_trunc", m=True)
_add("op1", "CVTPD2PS", None, "f8", (16,),
     ["cvtpd2ps {D}, {A}", "vcvtpd2ps {D}, {A}"],
     feat="SSE2", kind="CVT", oes=4, flags=("OFL",), style="op1", area="cvt",
     names=[("_mm_cvtpd_ps", "f", 8, 16)],
     refblock="cvt_f2f", m=True)
_add("op1", "CVTPD2PS", None, "f8", (32,),
     ["vcvtpd2ps {D}, {A}"],
     feat="AVX", kind="CVT", oes=4, flags=("OFL",), otot=16, style="op1",
     area="cvt",
     names=[("_mm256_cvtpd_ps", "f", 8, 32)],
     refblock="cvt_f2f", m=True)
_add("op1", "CVTPD2PS", None, "f8", (64,),
     ["vcvtpd2ps {D}, {A}"],
     feat="AVX512F", kind="CVT", oes=4, flags=("OFL",), otot=32, style="op1",
     area="cvt",
     names=[("_mm512_cvtpd_ps", "f", 8, 64)],
     refblock="cvt_f2f", m=True)
_add("op1", "CVTPS2PD", None, "f4", (16,),
     ["cvtps2pd {D}, {A}", "vcvtps2pd {D}, {A}"],
     feat="SSE2", kind="CVT", oes=8, flags=("OFL",), style="op1", area="cvt",
     names=[("_mm_cvtps_pd", "f", 4, 16)],
     refblock="cvt_f2f", m=True)
_add("op1", "CVTPS2PD", None, "f4", (16,),
     ["vcvtps2pd {D}, {A}"],
     feat="AVX", kind="CVT", oes=8, flags=("OFL",), otot=32, style="op1",
     area="cvt",
     names=[("_mm256_cvtps_pd", "f", 4, 16)],
     refblock="cvt_f2f", m=True)
_add("op1", "CVTPS2PD", None, "f4", (32,),
     ["vcvtps2pd {D}, {A}"],
     feat="AVX512F", kind="CVT", oes=8, flags=("OFL",), otot=64, style="op1",
     area="cvt",
     names=[("_mm512_cvtps_pd", "f", 4, 32)],
     refblock="cvt_f2f", m=True)
_add("op1", "CVTDQ2PS", None, "s4", (16, 32, 64),
     ["cvtdq2ps {D}, {A}", "vcvtdq2ps {D}, {A}"],
     feat="SSE2", kind="CVT", oes=4, flags=("OFL",), style="op1",
     area="cvt",
     names=[("_mm_cvtepi32_ps", "f", 4, 16),
            ("_mm256_cvtepi32_ps", "f", 4, 32),
            ("_mm512_cvtepi32_ps", "f", 4, 64)],
     refblock="cvt_f2f", m=True)
_add("op1", "CVTDQ2PD", None, "s4", (16,),
     ["cvtdq2pd {D}, {A}", "vcvtdq2pd {D}, {A}"],
     feat="SSE2", kind="CVT", oes=8, flags=("OFL",), style="op1",
     area="cvt",
     names=[("_mm_cvtepi32_pd", "f", 4, 16)],
     refblock="cvt_f2f", m=True)
_add("op1", "CVTDQ2PD", None, "s4", (16,),
     ["vcvtdq2pd {D}, {A}"],
     feat="AVX", kind="CVT", oes=8, otot=32, flags=("OFL",),
     style="op1", area="cvt",
     names=[("_mm256_cvtepi32_pd", "f", 4, 16)],
     refblock="cvt_f2f", m=True)
_add("op1", "CVTDQ2PD", None, "s4", (32,),
     ["vcvtdq2pd {D}, {A}"],
     feat="AVX512F", kind="CVT", oes=8, otot=64, flags=("OFL",),
     style="op1", area="cvt",
     names=[("_mm512_cvtepi32_pd", "f", 4, 32)],
     refblock="cvt_f2f", m=True)
for _op, _insn, _tp, _nm16, _blk in (
        ("CVTPS2UDQ", "cvtps2udq", "f4", "_mm_cvtps_epu32",
         "cvt_rintu"),
        ("CVTTPS2UDQ", "cvttps2udq", "f4", "_mm_cvttps_epu32",
         "cvt_truncu")):
    _add("op1", _op, None, _tp, (16, 32, 64),
         ["v%s {D}, {A}" % _insn],
         feat="AVX512F", kind="CVT", oes=4, style="op1",
         area="cvt",
         names=[(_nm16, "u", int(_tp[1]), 16),
                (_nm16.replace("_mm_", "_mm256_"), "u",
                 int(_tp[1]), 32),
                (_nm16.replace("_mm_", "_mm512_"), "u",
                 int(_tp[1]), 64)],
         refblock=_blk, m=True)
for _op, _insn, _nm16, _blk in (
        ("CVTPD2UDQ", "cvtpd2udq", "_mm_cvtpd_epu32",
         "cvt_rintu"),
        ("CVTTPD2UDQ", "cvttpd2udq", "_mm_cvttpd_epu32",
         "cvt_truncu")):
    _add("op1", _op, None, "f8", (16,),
         ["v%s {D}, {A}" % _insn],
         feat="AVX512F", kind="CVT", oes=4, style="op1",
         area="cvt",
         names=[(_nm16, "u", 8, 16)],
         refblock=_blk, m=True)
    _add("op1", _op, None, "f8", (32,),
         ["v%s {D}, {A}" % _insn],
         feat="AVX512F", kind="CVT", oes=4, otot=16, style="op1",
         area="cvt",
         names=[(_nm16.replace("_mm_", "_mm256_"), "u", 8, 32)],
         refblock=_blk, m=True)
    _add("op1", _op, None, "f8", (64,),
         ["v%s {D}, {A}" % _insn],
         feat="AVX512F", kind="CVT", oes=4, otot=32, style="op1",
         area="cvt",
         names=[(_nm16.replace("_mm_", "_mm512_"), "u", 8, 64)],
         refblock=_blk, m=True)
# scalar converts
for _op, _insn, _tp, _oes, _names in (
        ("CVTSS2SI", "cvtss2si", "f4", 4, ["_mm_cvtss_si32", "_mm_cvtss_i32"]),
        ("CVTSS2SI", "cvtss2si", "f4", 8,
         ["_mm_cvtss_si64", "_mm_cvtss_si64x", "_mm_cvtss_i64"]),
        ("CVTTSS2SI", "cvttss2si", "f4", 4, ["_mm_cvttss_si32", "_mm_cvttss_i32"]),
        ("CVTTSS2SI", "cvttss2si", "f4", 8,
         ["_mm_cvttss_si64", "_mm_cvttss_si64x", "_mm_cvttss_i64"]),
        ("CVTSD2SI", "cvtsd2si", "f8", 4, ["_mm_cvtsd_si32", "_mm_cvtsd_i32"]),
        ("CVTSD2SI", "cvtsd2si", "f8", 8,
         ["_mm_cvtsd_si64", "_mm_cvtsd_si64x", "_mm_cvtsd_i64"]),
        ("CVTTSD2SI", "cvttsd2si", "f8", 4, ["_mm_cvttsd_si32", "_mm_cvttsd_i32"]),
        ("CVTTSD2SI", "cvttsd2si", "f8", 8,
         ["_mm_cvttsd_si64", "_mm_cvttsd_si64x", "_mm_cvttsd_i64"])):
    _add("op1", _op, None, _tp, (16,),
         ["%s {%s}, {A}" % (_insn, "Gd" if _oes == 4 else "G")],
         feat="SSE2" if _tp == "f8" else "SSE", kind="SCALAR",
         oes=_oes, style="op1", area="cvt",
         names=[(n, "f", int(_tp[1]), 16) for n in _names],
         refblock="none")
# the int-to-float converts take their GPR operand in the
# backend's scalar-first order, so the public macros are
# customs over a names-free derivation entry
_add("op2", "CVTSI2SS", None, "f4", (16,),
     ["cvtsi2ss {D}, {Gd}"],
     feat="SSE", kind="BYTES", form="D2_A1", flags=("SCALAR1",),
     style="op2", area="cvt", names=[])
_add("op2", "CVTSI2SS", None, "f4", (16,),
     ["cvtsi2ss {D}, {Gd}"],
     style="custom", feat="SSE",
     args="a, b",
     macro="__builtin_ia32_vop2({OPC}, (b), (a), {rt})",
     names=[("_mm_cvtsi32_ss", "f", 4, 16),
            ("_mm_cvti32_ss", "f", 4, 16),
            ("_mm_cvtsi64_ss", "f", 4, 16),
            ("_mm_cvti64_ss", "f", 4, 16),
            ("_mm_cvtsi64x_ss", "f", 4, 16)])
_add("op2", "CVTSI2SD", None, "f8", (16,),
     ["cvtsi2sd {D}, {Gd}"],
     feat="SSE2", kind="BYTES", form="D2_A1", flags=("SCALAR1",),
     style="op2", area="cvt", names=[])
_add("op2", "CVTSI2SD", None, "f8", (16,),
     ["cvtsi2sd {D}, {Gd}"],
     style="custom", feat="SSE2",
     args="a, b",
     macro="__builtin_ia32_vop2({OPC}, (b), (a), {rt})",
     names=[("_mm_cvtsi32_sd", "f", 8, 16),
            ("_mm_cvti32_sd", "f", 8, 16),
            ("_mm_cvtsi64_sd", "f", 8, 16),
            ("_mm_cvti64_sd", "f", 8, 16),
            ("_mm_cvtsi64x_sd", "f", 8, 16)])

# comi / ucomi scalar compares: oes is the condition index of
# x86_comi_result (0 eq, 1 ne, 2 lt, 3 le, 4 gt, 5 ge)
for _pred, _oes in (("eq", 0), ("neq", 1), ("lt", 2),
                    ("le", 3), ("gt", 4), ("ge", 5)):
    for _sfx, _es, _insn in (("ss", 4, "comiss"),
                             ("sd", 8, "comisd")):
        _add("op2", "COMI" + _pred.upper(), None, "f%d" % _es,
             (16,),
             ["%s {D}, {B}" % _insn],
             feat="SSE", form="COMI", oes=_oes, kind="SCALAR",
             style="op2", area="cmp",
             names=[("_mm_comi%s_%s" % (_pred, _sfx),
                     "f", _es, 16),
                    ("_mm_ucomi%s_%s" % (_pred, _sfx),
                     "f", _es, 16)],
             refblock="comi" + _pred.replace("neq", "ne"))
# ptest family: oes picks the flag readback
for _op, _oes, _names, _blk in (
        ("PTESTZ", 0, ["_mm_testz_si128", "_mm_test_all_zeros"],
         "ptestz"),
        ("PTESTC", 1, ["_mm_testc_si128"], "ptestc"),
        ("PTESTNZC", 2, ["_mm_testnzc_si128",
                         "_mm_test_mix_ones_zeros"], "ptestnzc")):
    _add("op2", _op, None, "s8", (16,),
         ["ptest {D}, {B}"],
         feat="SSE41", form="PTEST", oes=_oes, kind="SCALAR",
         style="op2", area="cmp",
         names=[(n, "x", 8, 16) for n in _names],
         refblock=_blk)
    _add("op2", _op, None, "s8", (32,),
         ["vptest {D}, {B}"],
         feat="AVX", form="PTEST", oes=_oes, kind="SCALAR",
         style="op2", area="cmp",
         names=[(_names[0].replace("_mm_", "_mm256_").replace(
             "si128", "si256"), "x", 8, 32)],
         refblock=_blk)
    for _ty, _es2, _sx in (("f4", 4, "ps"), ("f8", 8, "pd")):
        _add("op2", _op, None, _ty, (16, 32),
             ["vtest{fs} {D}, {B}"],
             feat="AVX", form="PTEST", oes=_oes, kind="SCALAR",
             style="op2", area="cmp",
             names=[("%s_test%s_%s" % (_pre,
                     {"PTESTZ": "z", "PTESTC": "c",
                      "PTESTNZC": "nzc"}[_op], _sx),
                     "f", _es2, _tot)
                    for _pre, _tot in (("_mm", 16),
                                       ("_mm256", 32))],
             refblock=_blk + "ps")


# ------------------------------------------------------------------
# ------------------------------------------------------------------
# permute and broadcast forms
_add("op1i", "PERMILPS", "permute", "f4", (16, 32, 64),
     ["vpermilps {D}, {A}, {i}"],
     feat="AVX", style="op1i", imm=(0, 255), area="permute",
     refblock="permilps", m=True)
_add("op1i", "PERMILPD", "permute", "f8", (16, 32, 64),
     ["vpermilpd {D}, {A}, {i}"],
     feat="AVX", style="op1i", imm=(0, 255), area="permute",
     refblock="permilpd", m=True)
_add("op2", "PERMILPSV", "permutevar", "f4", (16, 32, 64),
     ["vpermilps {D}, {A}, {B}"],
     feat="AVX", style="op2", area="permute", refblock="permvarps",
     btype=("s", 4), m=True)
_add("op2", "PERMILPDV", "permutevar", "f8", (16, 32, 64),
     ["vpermilpd {D}, {A}, {B}"],
     feat="AVX", style="op2", area="permute", refblock="permvarpd",
     btype=("s", 8), m=True)
_add("op2i", "PERM2F128", None, "f4", (32,),
     ["vperm2f128 {D}, {A}, {B}, {i}"],
     feat="AVX", style="op2i", imm=(0, 255), area="permute",
     names=[("_mm256_permute2f128_ps", "f", 4, 32)],
     refblock="perm2x128")
_add("op2i", "PERM2F128", None, "f8", (32,),
     ["vperm2f128 {D}, {A}, {B}, {i}"],
     feat="AVX", style="op2i", imm=(0, 255), area="permute",
     names=[("_mm256_permute2f128_pd", "f", 8, 32)],
     refblock="perm2x128")
_add("op2i", "PERM2I128", None, "s8", (32,),
     ["vperm2i128 {D}, {A}, {B}, {i}"],
     feat="AVX2", style="op2i", imm=(0, 255), area="permute",
     names=[("_mm256_permute2f128_si256", "s", 8, 32),
            ("_mm256_permute2x128_si256", "s", 8, 32)],
     refblock="perm2x128")
_add("op1i", "PERMQ", None, "s8", (32,),
     ["vpermq {D}, {A}, {i}"],
     feat="AVX2", style="op1i", imm=(0, 255), area="permute",
     names=[("_mm256_permute4x64_epi64", "s", 8, 32)],
     refblock="permq")
_add("op1i", "PERMPD", None, "f8", (32,),
     ["vpermpd {D}, {A}, {i}"],
     feat="AVX2", style="op1i", imm=(0, 255), area="permute",
     names=[("_mm256_permute4x64_pd", "f", 8, 32)],
     refblock="permq")
_add("op1i", "VEXTRACTF128", None, "f4", (32,),
     ["vextractf128 {D}, {A}, {i}"],
     feat="AVX", kind="CVT", oes=4, otot=16, style="op1i",
     flags=("OFL",), imm=(0, 1), area="permute",
     names=[("_mm256_extractf128_ps", "f", 4, 32)],
     refblock="extract128")
_add("op1i", "VEXTRACTF128", None, "f8", (32,),
     ["vextractf128 {D}, {A}, {i}"],
     feat="AVX", kind="CVT", oes=8, otot=16, style="op1i",
     flags=("OFL",), imm=(0, 1), area="permute",
     names=[("_mm256_extractf128_pd", "f", 8, 32)],
     refblock="extract128")
_add("op1i", "VEXTRACTI128", None, "s8", (32,),
     ["vextracti128 {D}, {A}, {i}"],
     feat="AVX2", kind="CVT", oes=8, otot=16, style="op1i",
     imm=(0, 1), area="permute",
     names=[("_mm256_extracti128_si256", "s", 8, 32),
            ("_mm256_extractf128_si256", "s", 8, 32)],
     refblock="extract128")
_add("op2", "PERMD", None, "s4", (32,),
     ["vpermd {D}, {A}, {B}"],
     feat="AVX2", style="custom", args="a, b",
     macro="__builtin_ia32_vop2({OPC}, (b), (a), {rt})",
     names=[("_mm256_permutevar8x32_epi32", "s", 4, 32)],
     refblock="permfull")
_add("op2", "PERMPS", None, "f4", (32,),
     ["vpermps {D}, {A}, {B}"],
     feat="AVX2", style="custom", args="a, b",
     macro="__builtin_ia32_vop2({OPC}, (b), (a), {rt})",
     names=[("_mm256_permutevar8x32_ps", "f", 4, 32)],
     refblock="permfull")

# ------------------------------------------------------------------
# FMA: the 132 encoding serves every form; the addend is the
# S role, the multiplier the A role
for _op, _mn, _rb in (("FMADD", "vfmadd", "fma"),
                      ("FMSUB", "vfmsub", "fmsub"),
                      ("FNMADD", "vfnmadd", "fnmadd"),
                      ("FNMSUB", "vfnmsub", "fnmsub"),
                      ("FMADDSUB", "vfmaddsub", "fmaddsub"),
                      ("FMSUBADD", "vfmsubadd", "fmsubadd")):
    for _es, _sfx in ((4, "ps"), (8, "pd")):
        _add("op3", _op, _op.lower(), "f%d" % _es, (16, 32, 64),
             ["%s132{fs} {D}, {S}, {A}" % _mn],
             feat="FMA", style="op3", form="DACC_A_BS",
             area="float", refblock=_rb, m=True, mpass=True,
             names=[("_mm_%s_%s" % (_op.lower(), _sfx),
                     "f", _es, 16),
                    ("_mm256_%s_%s" % (_op.lower(), _sfx),
                     "f", _es, 32),
                    ("_mm512_%s_%s" % (_op.lower(), _sfx),
                     "f", _es, 64)])
    if _op in ("FMADD", "FMSUB", "FNMADD", "FNMSUB"):
        for _es, _sfx in ((4, "ss"), (8, "sd")):
            _add("op3", _op + "SS", _op.lower(), "f%d" % _es,
                 (16,),
                 ["%s132{sf} {D}, {S}, {A}" % _mn],
                 feat="FMA", style="op3", form="DACC_A_BS",
                 area="float",
                 refblock={"fma": "fmass", "fmsub": "fmssub",
                           "fnmadd": "fmnadd",
                           "fnmsub": "fmnsub"}[_rb],
                 m=True, mpass=True, ssmask=True,
                 names=[("_mm_%s_%s" % (_op.lower(), _sfx),
                         "f", _es, 16)])

# ------------------------------------------------------------------
# F16C converts
_add("op1", "CVTPH2PS", None, "s2", (16,),
     ["vcvtph2ps {D}, {A}"],
     feat="F16C", kind="CVT", oes=4, otot=16, flags=("OFL",),
     style="op1", area="cvt",
     names=[("_mm_cvtph_ps", "f", 2, 16)], refblock="cvtph",
     m=True)
_add("op1", "CVTPH2PS", None, "s2", (16,),
     ["vcvtph2ps {D}, {A}"],
     feat="F16C", kind="CVT", oes=4, otot=32, flags=("OFL",),
     style="op1", area="cvt",
     names=[("_mm256_cvtph_ps", "f", 2, 16)], refblock="cvtph",
     m=True)
_add("op1", "CVTPH2PS", None, "s2", (32,),
     ["vcvtph2ps {D}, {A}"],
     feat="F16C", kind="CVT", oes=4, otot=64, flags=("OFL",),
     style="op1", area="cvt",
     names=[("_mm512_cvtph_ps", "f", 2, 32)], refblock="cvtph",
     m=True)
_add("op1i", "CVTPS2PH", None, "f4", (16,),
     ["vcvtps2ph {D}, {A}, {i}"],
     feat="F16C", kind="CVT", oes=2, otot=16, style="op1i",
     imm=(0, 255), area="cvt",
     names=[("_mm_cvtps_ph", "s", 4, 16)], refblock="cvtps2ph",
     m=True, flags=("PHSAE",))
_add("op1i", "CVTPS2PH", None, "f4", (32,),
     ["vcvtps2ph {D}, {A}, {i}"],
     feat="F16C", kind="CVT", oes=2, otot=16, style="op1i",
     imm=(0, 255), area="cvt",
     names=[("_mm256_cvtps_ph", "s", 4, 32)], refblock="cvtps2ph",
     m=True, flags=("PHSAE",))
_add("op1i", "CVTPS2PH", None, "f4", (64,),
     ["vcvtps2ph {D}, {A}, {i}"],
     feat="F16C", kind="CVT", oes=2, otot=32, style="op1i",
     imm=(0, 255), area="cvt",
     names=[("_mm512_cvtps_ph", "s", 4, 64)], refblock="cvtps2ph",
     m=True, flags=("PHSAE",))

# header-only compositions (design section 4.1): loads and
# stores are plain dereferences, splats and lane inserts go
# through the splat / replace generics.  fam 'hdr' entries emit
# no opcode table rows.
# ------------------------------------------------------------------
def _hdr(names, args, macro, **kw):
    d = dict(fam="hdr", op="HDR", stem=None, types="", totals=(),
             asm=[], names=names, args=args, macro=macro,
             style="custom")
    d.update(kw)
    OPS.append(d)
    return d

_hdr([("_mm_cvtss_sd", "f", 8, 16)], "a, b",
     "_mm_move_sd((a), _mm_cvtps_pd((b)))",
     feat="SSE2", area="cvt")
_hdr([("_mm_mask_cvtss_sd", "f", 8, 16)], "src, k, a, b",
     "_mm_move_sd((a), _mm_mask_mov_pd((src), (k),"
     " _mm_cvtss_sd((a), (b))))",
     feat="AVX512VL", area="cvt")
_hdr([("_mm_maskz_cvtss_sd", "f", 8, 16)], "k, a, b",
     "_mm_move_sd((a), _mm_maskz_mov_pd((k),"
     " _mm_cvtss_sd((a), (b))))",
     feat="AVX512VL", area="cvt")
_hdr([("_mm_cvtsd_ss", "f", 4, 16)], "a, b",
     "_mm_move_ss((a), _mm_cvtpd_ps((b)))",
     feat="SSE2", area="cvt")
_hdr([("_mm_mask_cvtsd_ss", "f", 4, 16)], "src, k, a, b",
     "_mm_move_ss((a), _mm_mask_mov_ps((src), (k),"
     " _mm_cvtsd_ss((a), (b))))",
     feat="AVX512VL", area="cvt")
_hdr([("_mm_maskz_cvtsd_ss", "f", 4, 16)], "k, a, b",
     "_mm_move_ss((a), _mm_maskz_mov_ps((k),"
     " _mm_cvtsd_ss((a), (b))))",
     feat="AVX512VL", area="cvt")

for _sfx, _k, _es in (("ps", "f", 4), ("pd", "f", 8),
                      ("si128", "x", 8)):
    _kw = dict(feat="SSE2") if _k == "x" else {}
    _hdr([("_mm_load_" + _sfx, _k, _es, 16)],
         "a", "(*(const {rt} *)(a))", **_kw)
    _hdr([("_mm_loadu_" + _sfx, _k, _es, 16)],
         "a", "(*(const {rt} *)(a))", **_kw)
    _hdr([("_mm_store_" + _sfx, _k, _es, 16)],
         "a, b", "(*({rt} *)(a) = (b))", **_kw)
    _hdr([("_mm_storeu_" + _sfx, _k, _es, 16)],
         "a, b", "(*({rt} *)(a) = (b))", **_kw)
for _pre, _tot in (("_mm256", 32), ("_mm512", 64)):
    for _sfx, _k, _es in (("ps", "f", 4), ("pd", "f", 8),
                          ("si%d" % (_tot * 8), "x", 8)):
        _hdr([("%s_load_%s" % (_pre, _sfx), _k, _es, _tot)],
             "a", "(*(const {rt} *)(a))")
        _hdr([("%s_loadu_%s" % (_pre, _sfx), _k, _es, _tot)],
             "a", "(*(const {rt} *)(a))")
        _hdr([("%s_store_%s" % (_pre, _sfx), _k, _es, _tot)],
             "a, b", "(*({rt} *)(a) = (b))")
        _hdr([("%s_storeu_%s" % (_pre, _sfx), _k, _es, _tot)],
             "a, b", "(*({rt} *)(a) = (b))")

_SET1 = [("ps", "f", 4), ("pd", "f", 8), ("epi8", "s", 1),
         ("epi16", "s", 2), ("epi32", "s", 4), ("epi64", "s", 8)]
for _pre, _tot in (("_mm", 16), ("_mm256", 32), ("_mm512", 64)):
    for _sfx, _k, _es in _SET1:
        _nm = "%s_set1_%s" % (_pre, _sfx)
        if _pre in ("_mm", "_mm256") and _sfx == "epi64":
            _nm = "%s_set1_epi64x" % _pre
        _kw = dict(feat="SSE2") if _tot == 16 and _k != "f" else {}
        _hdr([(_nm, _k, _es, _tot)],
             "a", "__builtin_ia32_splat({opc}, (a), {rt})", **_kw)
        _hdr([("%s_setzero_%s" % (_pre, {"ps": "ps", "pd": "pd"}.get(
                 _sfx, "si%d" % (_tot * 8) if _tot > 16 else "si128")),
               _k, _es, _tot)],
             "", "__builtin_ia32_splat({opc}, 0, {rt})")
_hdr([("_mm_setzero_ps", "f", 4, 16)], "",
     "__builtin_ia32_splat({opc}, 0, {rt})")
_hdr([("_mm_setzero_pd", "f", 8, 16)], "",
     "__builtin_ia32_splat({opc}, 0, {rt})")
_hdr([("_mm_setzero_si128", "x", 8, 16)], "",
     "__builtin_ia32_splat({opc}, 0, {rt})", feat="SSE2")

_SET_ARGS = [chr(97 + i) for i in range(26)] + \
    ["a%d" % i for i in range(1, 40)]

def _set_macro(lanes):
    body = "__builtin_ia32_splat({opc}, 0, {rt})"
    args = _SET_ARGS
    for lane in range(lanes):
        body = "__builtin_ia32_replace({opc}, %s, %d, ({et})(%s))" % (
            body, lane, args[lanes - 1 - lane])
    return ", ".join(args[:lanes]), body

for _name, _k, _es, _tot, _lanes in (
        ("_mm_set_ps", "f", 4, 16, 4), ("_mm_set_pd", "f", 8, 16, 2),
        ("_mm_set_epi32", "s", 4, 16, 4),
        ("_mm_set_epi16", "s", 2, 16, 8),
        ("_mm_set_epi8", "s", 1, 16, 16),
        ("_mm_set_epi64x", "s", 8, 16, 2),
        ("_mm256_set_ps", "f", 4, 32, 8),
        ("_mm256_set_pd", "f", 8, 32, 4),
        ("_mm256_set_epi32", "s", 4, 32, 8),
        ("_mm256_set_epi16", "s", 2, 32, 16),
        ("_mm256_set_epi8", "s", 1, 32, 32),
        ("_mm256_set_epi64x", "s", 8, 32, 4),
        ("_mm512_set_ps", "f", 4, 64, 16),
        ("_mm512_set_pd", "f", 8, 64, 8),
        ("_mm512_set_epi32", "s", 4, 64, 16),
        ("_mm512_set_epi16", "s", 2, 64, 32),
        ("_mm512_set_epi8", "s", 1, 64, 64),
        ("_mm512_set_epi64", "s", 8, 64, 8)):
    _a, _b = _set_macro(_lanes)
    _kw = dict(feat="SSE2") if _tot == 16 and _k == "s" else {}
    _hdr([(_name, _k, _es, _tot)], _a, _b, **_kw)
    if _name in ("_mm512_set_epi8", "_mm512_set_epi16"):
        continue  # gcc declares no 512-bit setr for epi8/epi16
    if _name == "_mm_set_epi64x":
        continue  # gcc declares no _mm_setr_epi64x, and the
        # _mm_set_epi64 / _mm_setr_epi64 names are the MMX
        # forms over __m64, not this __m128i constructor
    _rname = _name.replace("_set_", "_setr_")
    _a2, _b2 = _set_macro(_lanes)
    # setr takes the lanes in memory order: lane i is argument i
    _body = "__builtin_ia32_splat({opc}, 0, {rt})"
    _args = _SET_ARGS
    for lane in range(_lanes):
        _body = "__builtin_ia32_replace({opc}, %s, %d, ({et})(%s))" % (
            _body, lane, _args[lane])
    _hdr([(_rname, _k, _es, _tot)], _a2, _body, **_kw)

# more header-only compositions: scalar and partial loads and
# stores, casts, undefined values, streaming stores
_hdr([("_mm_test_all_ones", "x", 8, 16)], "a",
     "_mm_testc_si128((a), _mm_cmpeq_epi32((a), (a)))",
     feat="SSE41")
_hdr([("_mm_loadl_epi64", "x", 8, 16)], "a",
     "__builtin_ia32_replace(13, __builtin_ia32_splat(13, 0,"
     " __m128i), 0, *(const long long *)(a), __m128i)",
     feat="SSE2")
_hdr([("_mm_storel_epi64", "x", 8, 16)], "a, b",
     "(*(long long *)(a) = __builtin_ia32_extract(13, (b), 0,"
     " long long), (void)0)", feat="SSE2")
_hdr([("_mm_load_ss", "f", 4, 16)], "a",
     "__builtin_ia32_replace(17, __builtin_ia32_splat(17, 0,"
     " __m128), 0, *(const float *)(a), __m128)")
_hdr([("_mm_store_ss", "f", 4, 16)], "a, b",
     "(*(float *)(a) = __builtin_ia32_extract(17, (b), 0, float),"
     " (void)0)")
_hdr([("_mm_set_ss", "f", 4, 16)], "a",
     "__builtin_ia32_replace(17, __builtin_ia32_splat(17, 0,"
     " __m128), 0, (a), __m128)")
_hdr([("_mm_load_sd", "f", 8, 16)], "a",
     "__builtin_ia32_replace(21, __builtin_ia32_splat(21, 0,"
     " __m128d), 0, *(const double *)(a), __m128d)", feat="SSE2")
_hdr([("_mm_store_sd", "f", 8, 16)], "a, b",
     "(*(double *)(a) = __builtin_ia32_extract(21, (b), 0,"
     " double), (void)0)", feat="SSE2")
_hdr([("_mm_set_sd", "f", 8, 16)], "a",
     "__builtin_ia32_replace(21, __builtin_ia32_splat(21, 0,"
     " __m128d), 0, (a), __m128d)", feat="SSE2")
# broadcast loads and stores of one lane value
_hdr([("_mm_load1_ps", "f", 4, 16), ("_mm_load_ps1", "f", 4, 16)],
     "a", "__builtin_ia32_splat(17, *(const float *)(a), __m128)")
_hdr([("_mm_set_ps1", "f", 4, 16)], "a",
     "__builtin_ia32_splat(17, (a), __m128)")
_hdr([("_mm_load1_pd", "f", 8, 16), ("_mm_load_pd1", "f", 8, 16)],
     "a", "__builtin_ia32_splat(21, *(const double *)(a),"
     " __m128d)", feat="SSE2")
_hdr([("_mm_set_pd1", "f", 8, 16)], "a",
     "__builtin_ia32_splat(21, (a), __m128d)", feat="SSE2")
_hdr([("_mm_loadr_ps", "f", 4, 16)], "a",
     "__builtin_ia32_replace(17, __builtin_ia32_replace(17,"
     " __builtin_ia32_replace(17, __builtin_ia32_replace(17,"
     " __builtin_ia32_splat(17, 0, __m128), 0,"
     " ((const float *)(a))[3]), 1, ((const float *)(a))[2]),"
     " 2, ((const float *)(a))[1]), 3, ((const float *)(a))[0],"
     " __m128)")
_hdr([("_mm_loadr_pd", "f", 8, 16)], "a",
     "__builtin_ia32_replace(21, __builtin_ia32_replace(21,"
     " __builtin_ia32_splat(21, 0, __m128d), 0,"
     " ((const double *)(a))[1]), 1, ((const double *)(a))[0],"
     " __m128d)", feat="SSE2")
_hdr([("_mm_store1_ps", "f", 4, 16), ("_mm_store_ps1", "f", 4, 16)],
     "a, b",
     "(((float *)(a))[0] = __builtin_ia32_extract(17, (b), 0,"
     " float), ((float *)(a))[1] = __builtin_ia32_extract(17,"
     " (b), 0, float), ((float *)(a))[2] ="
     " __builtin_ia32_extract(17, (b), 0, float),"
     " ((float *)(a))[3] = __builtin_ia32_extract(17, (b), 0,"
     " float), (void)0)")
_hdr([("_mm_store1_pd", "f", 8, 16), ("_mm_store_pd1", "f", 8, 16)],
     "a, b",
     "(((double *)(a))[0] = __builtin_ia32_extract(21, (b), 0,"
     " double), ((double *)(a))[1] = __builtin_ia32_extract(21,"
     " (b), 0, double), (void)0)", feat="SSE2")
_hdr([("_mm_storer_ps", "f", 4, 16)], "a, b",
     "(((float *)(a))[0] = __builtin_ia32_extract(17, (b), 3,"
     " float), ((float *)(a))[1] = __builtin_ia32_extract(17,"
     " (b), 2, float), ((float *)(a))[2] ="
     " __builtin_ia32_extract(17, (b), 1, float),"
     " ((float *)(a))[3] = __builtin_ia32_extract(17, (b), 0,"
     " float), (void)0)")
_hdr([("_mm_storer_pd", "f", 8, 16)], "a, b",
     "(((double *)(a))[0] = __builtin_ia32_extract(21, (b), 1,"
     " double), ((double *)(a))[1] = __builtin_ia32_extract(21,"
     " (b), 0, double), (void)0)", feat="SSE2")
# half-vector loads and stores
_hdr([("_mm_loadh_pd", "f", 8, 16)], "a, b",
     "__builtin_ia32_replace(21, (a), 1, *(const double *)(b),"
     " __m128d)", feat="SSE2")
_hdr([("_mm_loadl_pd", "f", 8, 16)], "a, b",
     "__builtin_ia32_replace(21, (a), 0, *(const double *)(b),"
     " __m128d)", feat="SSE2")
_hdr([("_mm_storeh_pd", "f", 8, 16)], "a, b",
     "(*(double *)(a) = __builtin_ia32_extract(21, (b), 1,"
     " double), (void)0)", feat="SSE2")
_hdr([("_mm_storel_pd", "f", 8, 16)], "a, b",
     "(*(double *)(a) = __builtin_ia32_extract(21, (b), 0,"
     " double), (void)0)", feat="SSE2")
_hdr([("_mm_loadh_pi", "f", 4, 16)], "a, b",
     "__builtin_ia32_replace(17, __builtin_ia32_replace(17, (a),"
     " 2, ((const float *)(b))[0]), 3, ((const float *)(b))[1],"
     " __m128)")
_hdr([("_mm_loadl_pi", "f", 4, 16)], "a, b",
     "__builtin_ia32_replace(17, __builtin_ia32_replace(17, (a),"
     " 0, ((const float *)(b))[0]), 1, ((const float *)(b))[1],"
     " __m128)")
_hdr([("_mm_storeh_pi", "f", 4, 16)], "a, b",
     "(((float *)(a))[0] = __builtin_ia32_extract(17, (b), 2,"
     " float), ((float *)(a))[1] = __builtin_ia32_extract(17,"
     " (b), 3, float), (void)0)")
_hdr([("_mm_storel_pi", "f", 4, 16)], "a, b",
     "(((float *)(a))[0] = __builtin_ia32_extract(17, (b), 0,"
     " float), ((float *)(a))[1] = __builtin_ia32_extract(17,"
     " (b), 1, float), (void)0)")
# partial-width scalar loads and stores
for _w, _ty, _sid in ((16, "short", 5), (32, "int", 9),
                      (64, "long long", 13)):
    _hdr([("_mm_loadu_si%d" % _w, "x", 8, 16)], "a",
         "__builtin_ia32_replace(13, __builtin_ia32_splat(13, 0,"
         " __m128i), 0, (long long)*(const %s *)(a), __m128i)"
         % _ty, feat="SSE2")
    _hdr([("_mm_storeu_si%d" % _w, "x", 8, 16)], "a, b",
         "(*(%s *)(a) = (%s)__builtin_ia32_extract(%d, (b), 0,"
         " long long), (void)0)" % (_ty, _ty, _sid), feat="SSE2")
_hdr([("_mm_move_epi64", "x", 8, 16)], "a",
     "__builtin_ia32_replace(13, __builtin_ia32_splat(13, 0,"
     " __m128i), 0, __builtin_ia32_extract(13, (a), 0,"
     " long long), __m128i)", feat="SSE2")
_hdr([("_mm_cvtsi128_si32", "x", 8, 16)], "a",
     "__builtin_ia32_extract(9, (a), 0, int)", feat="SSE2")
_hdr([("_mm_cvtsi32_si128", "x", 8, 16)], "a",
     "__builtin_ia32_replace(13, __builtin_ia32_splat(13, 0,"
     " __m128i), 0, (long long)(a), __m128i)", feat="SSE2")
_hdr([("_mm_cvtsi128_si64", "x", 8, 16)], "a",
     "__builtin_ia32_extract(13, (a), 0, long long)", feat="SSE2")
_hdr([("_mm_undefined_ps", "f", 4, 16)], "",
     "__builtin_ia32_splat(17, 0, __m128)")
_hdr([("_mm_undefined_pd", "f", 8, 16)], "",
     "__builtin_ia32_splat(21, 0, __m128d)", feat="SSE2")
_hdr([("_mm_undefined_si128", "x", 8, 16)], "",
     "__builtin_ia32_splat(13, 0, __m128i)", feat="SSE2")
for _nm, _k, _es, _tot, _rt in (
        ("_mm_castps_pd", "f", 8, 16, "__m128d"),
        ("_mm_castpd_ps", "f", 4, 16, "__m128"),
        ("_mm_castps_si128", "x", 8, 16, "__m128i"),
        ("_mm_castpd_si128", "x", 8, 16, "__m128i"),
        ("_mm_castsi128_ps", "f", 4, 16, "__m128"),
        ("_mm_castsi128_pd", "f", 8, 16, "__m128d")):
    _hdr([(_nm, _k, _es, _tot)], "a", "((%s)(a))" % _rt,
         feat="SSE2")
# streaming stores through the non-temporal generic; the
# opcode index (ps 0, pd 1, dq 2) is baked into the constant
_hdr([("_mm_stream_ps", "f", 4, 16)], "a, b",
     "__builtin_ia32_vntstore(17, (a), (b))")
_hdr([("_mm256_stream_ps", "f", 4, 32)], "a, b",
     "__builtin_ia32_vntstore(18, (a), (b))", feat="AVX")
_hdr([("_mm512_stream_ps", "f", 4, 64)], "a, b",
     "__builtin_ia32_vntstore(19, (a), (b))", feat="AVX512F")
_hdr([("_mm_stream_pd", "f", 8, 16)], "a, b",
     "__builtin_ia32_vntstore(53, (a), (b))", feat="SSE2")
_hdr([("_mm256_stream_pd", "f", 8, 32)], "a, b",
     "__builtin_ia32_vntstore(54, (a), (b))", feat="AVX")
_hdr([("_mm512_stream_pd", "f", 8, 64)], "a, b",
     "__builtin_ia32_vntstore(55, (a), (b))", feat="AVX512F")
_hdr([("_mm_stream_si128", "x", 8, 16)], "a, b",
     "__builtin_ia32_vntstore(77, (a), (b))", feat="SSE2")
_hdr([("_mm256_stream_si256", "x", 8, 32)], "a, b",
     "__builtin_ia32_vntstore(78, (a), (b))", feat="AVX2")
_hdr([("_mm512_stream_si512", "x", 8, 64)], "a, b",
     "__builtin_ia32_vntstore(79, (a), (b))", feat="AVX512F")

# ------------------------------------------------------------------
# gathers and scatters (header macros over the memory generics;
# the opcode is the index family in the high bits and the
# result (or value) shape id in the low five, baked per name
# like the streaming stores above).  The AVX2 masked gathers
# take the mask as a vector in gcc's order (src, base, index,
# mask, scale); the AVX-512 forms take an integer writemask
# (src, k, base, index, scale).
_GSID = {("x", 4): (9, 10, 11), ("x", 8): (13, 14, 15),
         ("f", 4): (17, 18, 19), ("f", 8): (21, 22, 23)}
_GPRE = {16: "_mm", 32: "_mm256", 64: "_mm512"}
for _ik, _fam in (("i32", 0), ("i64", 1)):
    for _sh, _rtk in ((("x", 4), "epi32"), (("x", 8), "epi64"),
                      (("f", 4), "ps"), (("f", 8), "pd")):
        for _tot in (16, 32):
            _sid = _GSID[_sh][0 if _tot == 16 else 1]
            _opc = (_fam << 5) | _sid
            _nm = "%s_%sgather_%s" % (_GPRE[_tot], _ik, _rtk)
            _tuple = (_nm, "s" if _sh[0] == "x" and _sh[1] == 4
                      else _sh[0], _sh[1], _tot)
            _hdr([_tuple], "a, b, s",
                 "__builtin_ia32_gather(%d, (a), (b), (s), {rt})"
                 % _opc, feat="AVX2")
            _hdr([("%s_mask_%sgather_%s"
                   % (_GPRE[_tot], _ik, _rtk),) + _tuple[1:]],
                 "a, b, c, d, s",
                 "__builtin_ia32_gatherm(%d, (d), (a), (b), (c),"
                 " (s), {rt})" % _opc, feat="AVX2")
for _ik, _fam in (("i32", 2), ("i64", 3)):
    for _sh, _rtk in ((("x", 4), "epi32"), (("x", 8), "epi64"),
                      (("f", 4), "ps"), (("f", 8), "pd")):
        for _tot, _rsh in ((16, 0), (32, 1), (64, 2)):
            # the i64-index forms with a narrower result share
            # the opcode of the matching-width form
            _rsid = _GSID[_sh][_rsh]
            if _ik == "i64" and _sh[1] == 4 and _tot == 64:
                _rsid = _GSID[_sh][1]
            if _ik == "i64" and _sh == ("f", 4) and _tot == 32:
                _rsid = _GSID[_sh][0]
            if _ik == "i64" and _sh == ("f", 4) and _tot == 64:
                _rsid = _GSID[_sh][1]
            _opc = (_fam << 5) | _rsid
            _rtot = _tot if not (_ik == "i64" and _tot == 64
                                 and _sh[1] == 4) else 32
            if _ik == "i64" and _sh == ("f", 4) and _tot == 32:
                _rtot = 16
            _nm = "%s_%sgather_%s" % (_GPRE[_tot], _ik, _rtk)
            _tuple = (_nm, "s" if _sh[0] == "x" and _sh[1] == 4
                      else _sh[0], _sh[1], _rtot)
            # the AVX-512 gather order is (index, base[, ...]),
            # reversed against the AVX2 forms above
            _hdr([_tuple], "a, b, s",
                 "__builtin_ia32_gather(%d, (b), (a), (s), {rt})"
                 % _opc, feat="AVX512F")
            _hdr([("%s_mask_%sgather_%s"
                   % (_GPRE[_tot], _ik, _rtk),) + _tuple[1:]],
                 "a, k, b, c, s",
                 "__builtin_ia32_gatherm(%d, (k), (a), (c), (b),"
                 " (s), {rt})" % _opc, feat="AVX512F")
            if _tot in (16, 32):
                # the VL mmask forms: same gathers with an
                # integer writemask and the index operand
                # before the base in the intrinsic order
                _hdr([("%s_mmask_%sgather_%s"
                       % (_GPRE[_tot], _ik, _rtk),) + _tuple[1:]],
                     "a, k, b, c, s",
                     "__builtin_ia32_gatherm(%d, (k), (a), (c),"
                     " (b), (s), {rt})" % _opc, feat="AVX512VL")
            _snm = "%s_%sscatter_%s" % (_GPRE[_tot], _ik, _rtk)
            _stuple = (_snm, "s" if _sh[0] == "x" and _sh[1] == 4
                       else _sh[0], _sh[1], _rtot)
            _hdr([_stuple], "a, b, c, s",
                 "__builtin_ia32_scatter(%d, (a), (b), (c), (s))"
                 % _opc, feat="AVX512F")
            _hdr([("%s_mask_%sscatter_%s"
                   % (_GPRE[_tot], _ik, _rtk),) + _stuple[1:]],
                 "a, k, b, c, s",
                 "__builtin_ia32_scatterm(%d, (k), (a), (b), (c),"
                 " (s))" % _opc, feat="AVX512F")

# broadcast of lane 0 of a register through the extract and
# splat generics (the memory-only x4 forms and the 128-bit
# half-width inserts are not in the generic model)
for _sfx, _es, _et, _ssid, _d0 in (
        ("b_epi8", 1, "signed char", 1, 1),
        ("w_epi16", 2, "short", 5, 5),
        ("d_epi32", 4, "int", 9, 9),
        ("q_epi64", 8, "long long", 13, 13)):
    for _tot, _dsid, _feat in ((16, _d0, "AVX2"),
                               (32, _d0 + 1, "AVX2"),
                               (64, _d0 + 2, "AVX512F")):
        _hdr([("%s_broadcast%s" % (_GPRE[_tot], _sfx),
               "x", 8, _tot)], "a",
             "__builtin_ia32_splat(%d, __builtin_ia32_extract"
             "(%d, (a), 0, %s), {rt})" % (_dsid, _ssid, _et),
             feat=_feat)
for _sfx, _k, _es, _et, _ssid in (("ss_ps", "f", 4, "float", 17),
                                  ("sd_pd", "f", 8, "double", 21)):
    for _tot, _dsid, _feat in ((16, _ssid, "AVX2"),
                               (32, _ssid + 1, "AVX2"),
                               (64, _ssid + 2, "AVX512F")):
        _hdr([("%s_broadcast%s" % (_GPRE[_tot], _sfx),
               _k, _es, _tot)], "a",
             "__builtin_ia32_splat(%d, __builtin_ia32_extract"
             "(%d, (a), 0, %s), {rt})" % (_dsid, _ssid, _et),
             feat=_feat)

# ------------------------------------------------------------------
# AVX-512 mask-valued operations: compares, tests, sign masks
# and fpclass produce a writemask in a GPR (kind MASK); the
# named-predicate forms are header aliases over the immediate
# forms with gcc's predicate numbers.
for _su, _op, _tpl in (("s", "CMPK", "vpcmp{ss} {Kd}, {A}, {B}, {i}"),
                       ("u", "CMPKU",
                        "vpcmpu{ss} {Kd}, {A}, {B}, {i}")):
    for _es in (1, 2, 4, 8):
        _add("op2i", _op, None, "%s%d" % (_su, _es), (16, 32, 64),
             [_tpl], kind="MASK", style="op2i", imm=(0, 7),
             feat="AVX512BW" if _es <= 2 else "AVX512F",
             area="cmp", refblock="mkicmp", m=True,
             names=[("%s_cmp_ep%s%d_mask"
                     % (_pre, "i" if _su == "s" else "u", _es * 8),
                     _su, _es, _tot)
                    for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                       ("_mm512", 64))])
_add("op2i", "CMPPK", None, "f4 f8", (16, 32, 64),
     ["vcmp{fs} {Kd}, {A}, {B}, {i}"],
     kind="MASK", style="op2i", imm=(0, 31), feat="AVX512F",
     area="cmp", refblock="mkfcmp", m=True,
     names=[("%s_cmp_%s_mask" % (_pre, _sfx), "f", _es, _tot)
            for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                               ("_mm512", 64))
            for _sfx, _es in (("ps", 4), ("pd", 8))])
for _op, _tpl, _rb in (("TESTK", "vptestm{ss} {Kd}, {A}, {B}",
                        "mktest"),
                       ("TESTNK", "vptestnm{ss} {Kd}, {A}, {B}",
                        "mktestn")):
    for _es in (1, 2, 4, 8):
        # vptestm/vptestnm are sign-agnostic and gcc declares
        # only the epi spellings
        _add("op2", _op, None, "s%d" % _es, (16, 32, 64),
             [_tpl], kind="MASK", style="op2",
             feat="AVX512BW" if _es <= 2 else "AVX512F",
             area="cmp", refblock=_rb, m=True,
             names=[("%s_%s_epi%d_mask"
                     % (_pre, "test" if _op == "TESTK" else "testn",
                        _es * 8), "s", _es, _tot)
                    for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                       ("_mm512", 64))])
for _es in (1, 2, 4, 8):
    _add("op1", "MOV2MK", None, "s%d" % _es, (16, 32, 64),
         ["vpmov{ss}2m {Kd}, {A}"], kind="MASK", style="op1",
         feat="AVX512BW" if _es <= 2 else "AVX512F",
         area="misc", refblock="mkmov",
         names=[("%s_movepi%d_mask" % (_pre, _es * 8),
                 "s", _es, _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))])
_add("op1i", "FPCLASS", None, "f4 f8", (16, 32, 64),
     ["vfpclass{fs} {Kd}, {A}, {i}"],
     kind="MASK", style="op1i", imm=(0, 255), feat="AVX512DQ",
     area="cmp", refblock="fpclass", m=True,
     names=[("%s_fpclass_%s_mask" % (_pre, _sfx), "f", _es, _tot)
            for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                               ("_mm512", 64))
            for _sfx, _es in (("ps", 4), ("pd", 8))])

_IPRED = (("eq", 0), ("lt", 1), ("le", 2), ("neq", 4),
          ("ge", 5), ("gt", 6))
for _pre, _tot in (("_mm", 16), ("_mm256", 32), ("_mm512", 64)):
    for _su in ("s", "u"):
        for _es in (1, 2, 4, 8):
            _feat = "AVX512BW" if _es <= 2 else "AVX512F"
            for _pn, _pi in _IPRED:
                _ep = "epi" if _su == "s" else "epu"
                _hdr([("%s_cmp%s_%s%d_mask"
                       % (_pre, _pn, _ep, _es * 8), _su, _es, _tot)],
                     "a, b",
                     "%s_cmp_%s%d_mask((a), (b), %d)"
                     % (_pre, _ep, _es * 8, _pi), feat=_feat)
                _hdr([("%s_mask_cmp%s_%s%d_mask"
                       % (_pre, _pn, _ep, _es * 8), _su, _es, _tot)],
                     "k, a, b",
                     "%s_mask_cmp_%s%d_mask((k), (a), (b), %d)"
                     % (_pre, _ep, _es * 8, _pi), feat=_feat)
_FPRED = (("eq", 0), ("lt", 1), ("le", 2), ("unord", 3),
          ("neq", 4), ("nlt", 5), ("nle", 6), ("ord", 7))
# gcc declares the named floating-point predicate forms
# only at 512 bits (the VL widths have only the immediate form)
for _pre, _tot in (("_mm512", 64),):
    for _sfx, _es in (("ps", 4), ("pd", 8)):
        for _pn, _pi in _FPRED:
            _hdr([("%s_cmp%s_%s_mask" % (_pre, _pn, _sfx),
                   "f", _es, _tot)], "a, b",
                 "%s_cmp_%s_mask((a), (b), %d)" % (_pre, _sfx, _pi),
                 feat="AVX512F")
            _hdr([("%s_mask_cmp%s_%s_mask" % (_pre, _pn, _sfx),
                   "f", _es, _tot)], "k, a, b",
                 "%s_mask_cmp_%s_mask((k), (a), (b), %d)"
                 % (_pre, _sfx, _pi), feat="AVX512F")

# ------------------------------------------------------------------
# AVX-512 operation families: ternary logic, conflict / lzcnt
# (CD), the VNNI dot products, rotates, mask-selected blends
# and the epi32 / epi64 spellings of the logic operations.
_add("op3i", "TERNLOG", None, "s4 s8", (16, 32, 64),
     ["vpternlog{ss} {D}, {A}, {B}, {i}"],
     style="op3i", imm=(0, 255), feat="AVX512F", area="logic",
     form="DACC_A_B", refblock="ternlog", m=True, mpass=True,
     names=[("%s_ternarylogic_epi%d" % (_pre, _es * 8),
             "s", _es, _tot)
            for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                               ("_mm512", 64))
            for _es in (4, 8)])
for _op, _tpl, _rb, _sfx in (
        ("CONFLICT", "vpconflict{ss} {D}, {A}", "conflict",
         "conflict"),
        ("LZCNT", "vplzcnt{ss} {D}, {A}", "lzcnt", "lzcnt")):
    _add("op1", _op, None, "s4 s8", (16, 32, 64), [_tpl],
         style="op1", feat="AVX512CD", area="arith",
         refblock=_rb, m=True,
         names=[("%s_%s_epi%d" % (_pre, _sfx, _es * 8),
                 "s", _es, _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))
                for _es in (4, 8)])
for _op, _tpl, _rb, _sfx in (
        ("DPBUSD", "vpdpbusd {D}, {A}, {B}", "dpbusd", "dpbusd"),
        ("DPBUSDS", "vpdpbusds {D}, {A}, {B}", "dpbusds",
         "dpbusds"),
        ("DPWSSD", "vpdpwssd {D}, {A}, {B}", "dpwssd", "dpwssd"),
        ("DPWSSDS", "vpdpwssds {D}, {A}, {B}", "dpwssds",
         "dpwssds")):
    _add("op3", _op, None, "s4", (16, 32, 64), [_tpl],
         style="op3", feat="AVX512VNNI", area="arith",
         form="DACC_A_B", refblock=_rb, m=True, mpass=True,
         names=[("%s_%s_epi32" % (_pre, _sfx), "s", 4, _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))])
for _op, _tpl, _rb in (("ROL", "vprol{ss} {D}, {A}, {i}", "rol"),
                       ("ROR", "vpror{ss} {D}, {A}, {i}", "ror")):
    _add("op1i", _op, None, "s4 s8", (16, 32, 64), [_tpl],
         style="op1i", imm=(0, 255), feat="AVX512F",
         area="shift", refblock=_rb, m=True,
         names=[("%s_%s_epi%d" % (_pre, _op.lower(), _es * 8),
                 "s", _es, _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))
                for _es in (4, 8)])
for _op, _tpl, _rb in (("ROLV", "vprolv{ss} {D}, {A}, {B}",
                        "rolv"),
                       ("RORV", "vprorv{ss} {D}, {A}, {B}",
                        "rorv")):
    _add("op2", _op, None, "s4 s8", (16, 32, 64), [_tpl],
         style="op2", feat="AVX512F", area="shift",
         refblock=_rb, m=True,
         names=[("%s_%s_epi%d" % (_pre, _op.lower(), _es * 8),
                 "s", _es, _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))
                for _es in (4, 8)])
for _es in (1, 2, 4, 8):
    _add("opm", "BLENDM", None, "s%d" % _es, (16, 32, 64),
         ["vpblendm{ss} {D}, {A}, {B}"],
         flags=("KSEL",), style="opm",
         feat="AVX512BW" if _es <= 2 else "AVX512F",
         area="logic", refblock="kblend",
         names=[("%s_mask_blend_epi%d" % (_pre, _es * 8),
                 "s", _es, _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))])
_add("opm", "BLENDM", None, "f4 f8", (16, 32, 64),
     ["vblendm{fs} {D}, {A}, {B}"],
     flags=("KSEL",), style="opm", feat="AVX512F",
     area="logic", refblock="kblend",
     names=[("%s_mask_blend_%s" % (_pre, _sfx), "f", _es, _tot)
            for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                               ("_mm512", 64))
            for _sfx, _es in (("ps", 4), ("pd", 8))])

# ------------------------------------------------------------------
# two-source and variable permutes, element align, the AVX-512
# floating point assistants (scalef, range, getexp, getmant,
# reduce, fixupimm) and the compress / expand family
def _permt2(op, i2op, types, kind, es, sfx, feat, area):
    """one two-source permute group: the plain vpermt2 form, a
       nameless derivation entry carrying the writemask rows
       (the generic masked macros cannot express the masked
       forms' argument order: the merging form's passthrough
       is the index operand), the vpermi2 derivation entry the
       mask2 form resolves through, and the three masked forms
       as signature-exact custom macros"""
    _s = "{fs}" if kind == "f" else "{ss}"
    _tpl = "vpermt2%s {A}, {B}, {S}" % _s
    _tpl2 = "vpermi2%s {D}, {A}, {B}" % _s
    _kw = dict(btype=("s", es)) if kind == "f" else {}
    _add("op3", op, None, types, (16, 32, 64), [_tpl],
         style="op3", form="DACC2_A_B", feat=feat, area=area,
         refblock="permt2",
         names=[("%s_permutex2var_%s" % (_pre, sfx), kind, es,
                 _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))], **_kw)
    _add("op3", op, None, types, (16, 32, 64), [_tpl],
         form="DACC2_A_B", feat=feat, m=True, names=[], **_kw)
    _add("op3", i2op, None, types, (16, 32, 64), [_tpl2],
         form="D_A_B", feat=feat, m=True, names=[], **_kw)
    for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                       ("_mm512", 64)):
        _add("op3", op, None, types, (_tot,), [],
             style="custom", form="DACC2_A_B", feat=feat,
             args="a, k, idx, b",
             macro="__builtin_ia32_vop3m2({OPC}, (k), (idx),"
                   " (idx), (a), (b), {rt})",
             names=[("%s_mask_permutex2var_%s" % (_pre, sfx),
                     kind, es, _tot)])
        _add("op3", op, None, types, (_tot,), [],
             style="custom", form="DACC2_A_B", feat=feat,
             args="k, a, idx, b",
             macro="__builtin_ia32_vop3m({OPC}, (k), (idx),"
                   " (a), (b), {rt})",
             names=[("%s_maskz_permutex2var_%s" % (_pre, sfx),
                     kind, es, _tot)])
        _add("op3", i2op, None, types, (_tot,), [],
             style="custom", form="D_A_B", feat=feat,
             args="a, idx, k, b",
             macro="__builtin_ia32_vop3m2({OPC}, (k), (a),"
                   " (idx), (b), (b), {rt})",
             names=[("%s_mask2_permutex2var_%s" % (_pre, sfx),
                     kind, es, _tot)])

_permt2("PERMT2B", "PERMI2B", "s1", "s", 1, "epi8",
        "AVX512VBMI", "vbmi")
_permt2("PERMT2W", "PERMI2W", "s2", "s", 2, "epi16",
        "AVX512BW", "permute")
_permt2("PERMT2", "PERMI2", "s4", "s", 4, "epi32",
        "AVX512F", "permute")
_permt2("PERMT2", "PERMI2", "s8", "s", 8, "epi64",
        "AVX512F", "permute")
_permt2("PERMT2F", "PERMI2F", "f4", "f", 4, "ps",
        "AVX512F", "permute")
_permt2("PERMT2F", "PERMI2F", "f8", "f", 8, "pd",
        "AVX512F", "permute")
for _es, _feat, _area in ((1, "AVX512VBMI", "vbmi"),
                          (2, "AVX512BW", "permute")):
    _add("op2", "PERMUTEXVAR", None, "s%d" % _es, (16, 32, 64),
         ["vperm{ss} {D}, {A}, {B}"],
         style="op2", feat=_feat, area=_area, refblock="permfull",
         m=True,
         names=[("%s_permutexvar_epi%d" % (_pre, _es * 8),
                 "s", _es, _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))])
_add("op2", "PERMUTEXVAR", "permutexvar", "s4 s8", (16, 32, 64),
     ["vperm{ss} {D}, {A}, {B}"],
     style="op2", feat="AVX512F", area="permute",
     refblock="permfull", m=True)
for _es in (4, 8):
    _add("op2", "PERMUTEXVAR", "permutexvar", "f%d" % _es, (16, 32, 64),
         ["vperm{ss} {D}, {A}, {B}"],
         style="op2", feat="AVX512F", area="permute", kind="BYTES",
         refblock="permfullx", m=True, atype=("s", _es))

_add("op1i", "PERMUTEXQ", None, "s8", (64,),
     ["vpermq {D}, {A}, {i}"],
     style="op1i", imm=(0, 255), feat="AVX512F", area="permute",
     refblock="permq512", m=True,
     names=[("_mm512_permutex_epi64", "s", 8, 64)])
_add("op1i", "PERMUTEXPD", None, "f8", (64,),
     ["vpermpd {D}, {A}, {i}"],
     style="op1i", imm=(0, 255), feat="AVX512F", area="permute",
     refblock="permq512", m=True,
     names=[("_mm512_permutex_pd", "f", 8, 64)])
_add("op2", "SCALEF", "scalef", "f4 f8", (16, 32, 64),
     ["vscalef{fs} {D}, {A}, {B}"],
     style="op2", feat="AVX512F", area="float",
     refblock="scalef", m=True)
_add("op2i", "RANGE", "range", "f4 f8", (16, 32, 64),
     ["vrange{fs} {D}, {A}, {B}, {i}"],
     style="op2i", imm=(0, 15), feat="AVX512DQ", area="float",
     refblock="range", m=True)
_add("op1", "GETEXP", "getexp", "f4 f8", (16, 32, 64),
     ["vgetexp{fs} {D}, {A}"],
     style="op1", feat="AVX512F", area="float",
     refblock="getexp", m=True)
_add("op1i", "REDUCE", "reduce", "f4 f8", (16, 32, 64),
     ["vreduce{fs} {D}, {A}, {i}"],
     style="op1i", imm=(0, 255), feat="AVX512F", area="float",
     refblock="reduce", m=True)
_add("op1i", "GETMANT", None, "f4 f8", (16, 32, 64),
     ["vgetmant{fs} {D}, {A}, {i}"],
     style="custom", feat="AVX512F", imm=(0, 15), m=True,
     args="a, n, s",
     macro="__builtin_ia32_vop1i({OPC}, (a),"
           " ((n) + ((s) << 2)), {rt})",
     names=[("%s_getmant_%s" % (_pre, _sfx), "f", _es, _tot)
            for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                               ("_mm512", 64))
            for _sfx, _es in (("ps", 4), ("pd", 8))],
     refblock="getexp")
_add("op1i", "GETMANT", None, "f4 f8", (16, 32, 64),
     ["vgetmant{fs} {D}, {A}, {i}"],
     style="custom", feat="AVX512F", imm=(0, 15),
     args="src, k, a, n, s",
     macro="__builtin_ia32_vop1mi2({OPC}, (k), (src), (a),"
           " ((n) + ((s) << 2)), {rt})",
     names=[("%s_mask_getmant_%s" % (_pre, _sfx), "f", _es, _tot)
            for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                               ("_mm512", 64))
            for _sfx, _es in (("ps", 4), ("pd", 8))])
_add("op1i", "GETMANT", None, "f4 f8", (16, 32, 64),
     ["vgetmant{fs} {D}, {A}, {i}"],
     style="custom", feat="AVX512F", imm=(0, 15),
     args="k, a, n, s",
     macro="__builtin_ia32_vop1mi({OPC}, (k), (a),"
           " ((n) + ((s) << 2)), {rt})",
     names=[("%s_maskz_getmant_%s" % (_pre, _sfx), "f", _es, _tot)
            for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                               ("_mm512", 64))
            for _sfx, _es in (("ps", 4), ("pd", 8))])
_add("op3i", "FIXUPIMM", "fixupimm", "f4 f8", (16, 32, 64),
     ["vfixupimm{fs} {D}, {A}, {B}, {i}"],
     style="op3i", form="DACC_A_B", imm=(0, 255),
     feat="AVX512DQ", area="float", refblock="fixup",
     ctype=("x", 8), m=True, mpass=True)
_add("op3i", "FIXUPSS", None, "f4 f8", (16,),
     ["vfixupimm{sf} {D}, {A}, {B}, {i}"],
     style="op3i", form="DACC_A_B", imm=(0, 255),
     feat="AVX512DQ", area="float", refblock="fixupss",
     ctype=("x", 8), m=True, mpass=True, ssmask=True,
     names=[("_mm_fixupimm_ss", "f", 4, 16),
            ("_mm_fixupimm_sd", "f", 8, 16)])
for _op, _tpl, _rb in (("COMPRESS", "vpcompress{ss} {D} {Kz}, {A}",
                        "compress"),
                       ("EXPAND", "vpexpand{ss} {D} {Kz}, {A}",
                        "expand")):
    _add("op1m", _op, _op.lower(), "s4 s8", (16, 32, 64), [_tpl],
         style="op1m", feat="AVX512F", area="permute",
         refblock=_rb, m=True)
    _add("op1m", _op, _op.lower(), "f4 f8", (16, 32, 64),
         ["vcompress{fs} {D} {Kz}, {A}" if _op == "COMPRESS"
          else "vexpand{fs} {D} {Kz}, {A}"],
         style="op1m", feat="AVX512F", area="permute",
         refblock=_rb, m=True)

# ------------------------------------------------------------------
# scalar floating point binary operations (the result is the
# first operand with its low element replaced) and the
# embedded-rounding (_round_) surface: 512-bit vector forms,
# scalar forms and the FMA forms, all through the imm-carrying
# generics with evrnd entries
for _op, _mn, _rb in (("ADD", "add", "faddss"),
                      ("SUB", "sub", "fsubss"),
                      ("MUL", "mul", "fmulss"),
                      ("DIV", "div", "fdivss"),
                      ("MIN", "min", "fminss"),
                      ("MAX", "max", "fmaxss")):
    for _es, _sfx in ((4, "ss"), (8, "sd")):
        _add("op2", _op + "SS", None, "f%d" % _es, (16,),
             ["%s{sf} {D}, {B}" % _mn,
              "v%s{sf} {D}, {A}, {B}" % _mn],
             feat="SSE" if _es == 4 else "SSE2", style="op2",
             area="float", refblock=_rb, m=True, ssmask=True,
             names=[("_mm_%s_%s" % (_mn, _sfx), "f", _es, 16)])
_add("op1", "SQRTSS", None, "f4", (16,),
     ["vsqrtss {D}, {D}, {A}"],
     feat="SSE", style="op1", area="float", refblock="fsqrtss2",
     form="D_B_ACC",
     names=[("_mm_sqrt_ss", "f", 4, 16)])
# the masked sqrt_ss forms take a separate source operand,
# unlike the one-operand plain form: they ride a two-operand
# derivation entry through customs
_add("op2", "SQRTSS2", None, "f4", (16,),
     ["vsqrtss {D}, {A}, {B}"],
     feat="AVX512F", style="op2", area="float", m=True,
     names=[])
_add("op2", "SQRTSS2", None, "f4", (16,),
     ["vsqrtss {D}, {A}, {B}"],
     style="custom", feat="AVX512F",
     args="a, k, b",
     macro="__builtin_ia32_vop2m2({OPC}, (k), (a), (a), (b),"
           " {rt})",
     names=[("_mm_mask_sqrt_ss", "f", 4, 16)])
_add("op2", "SQRTSS2", None, "f4", (16,),
     ["vsqrtss {D}, {A}, {B}"],
     style="custom", feat="AVX512F",
     args="k, a, b",
     macro="__builtin_ia32_vop2m({OPC}, (k), (a), (b), {rt})",
     names=[("_mm_maskz_sqrt_ss", "f", 4, 16)])
_add("op2", "SQRTSD", None, "f8", (16,),
     ["sqrtsd {D}, {B}", "vsqrtsd {D}, {A}, {B}"],
     feat="SSE2", style="op2", area="float", refblock="fsqrtss",
     m=True, ssmask=True,
     names=[("_mm_sqrt_sd", "f", 8, 16)])
for _op, _mn, _ref in (("ADDR", "add", "a[i] + b[i]"),
                       ("SUBR", "sub", "a[i] - b[i]"),
                       ("MULR", "mul", "a[i] * b[i]"),
                       ("DIVR", "div", "a[i] / b[i]")):
    _add("op2i", _op, None, "f4 f8", (64,),
         ["v%s{fs} {D}, {A}, {B}" % _mn],
         feat="AVX512F", style="op2i", imm=(0, 15), evrnd=True,
         area="float", ref=_ref, m=True,
         names=[("_mm512_%s_round_%s" % (_mn, _sfx), "f", _es, 64)
                for _sfx, _es in (("ps", 4), ("pd", 8))])
for _op, _mn, _cmp in (("MINR", "min", "<"), ("MAXR", "max",
                                                ">")):
    _add("op2i", _op, None, "f4 f8", (64,),
         ["v%s{fs} {D}, {A}, {B}" % _mn],
         feat="AVX512F", style="op2i", imm=(0, 15),
         area="float",
         ref="(a[i] %s b[i]) ? a[i] : b[i]" % _cmp, m=True,
         names=[("_mm512_%s_round_%s" % (_mn, _sfx), "f", _es, 64)
                for _sfx, _es in (("ps", 4), ("pd", 8))])
_add("op1i", "SQRTR", None, "f4 f8", (64,),
     ["vsqrt{fs} {D}, {A}"],
     feat="AVX512F", style="op1i", imm=(0, 15), evrnd=True,
     area="float", refblock="fsqrt", m=True,
     names=[("_mm512_sqrt_round_%s" % _sfx, "f", _es, 64)
            for _sfx, _es in (("ps", 4), ("pd", 8))])
_add("op2i", "SCALEFR", None, "f4 f8", (64,),
     ["vscalef{fs} {D}, {A}, {B}"],
     feat="AVX512F", style="op2i", imm=(0, 15), evrnd=True,
     area="float", refblock="scalef", m=True,
     names=[("_mm512_scalef_round_%s" % _sfx, "f", _es, 64)
            for _sfx, _es in (("ps", 4), ("pd", 8))])
_add("op1i", "GETEXPR", None, "f4 f8", (64,),
     ["vgetexp{fs} {D}, {A}"],
     feat="AVX512F", style="op1i", imm=(0, 15),
     area="float", refblock="getexp", m=True,
     names=[("_mm512_getexp_round_%s" % _sfx, "f", _es, 64)
            for _sfx, _es in (("ps", 4), ("pd", 8))])
for _op, _mn, _rb in (("ADD", "add", "faddss"),
                      ("SUB", "sub", "fsubss"),
                      ("MUL", "mul", "fmulss"),
                      ("DIV", "div", "fdivss"),
                      ):
    for _es, _sfx in ((4, "ss"), (8, "sd")):
        _add("op2i", _op + "R", None, "f%d" % _es, (16,),
             ["v%s{sf} {D}, {A}, {B} {rn-sae}" % _mn],
             feat="AVX512F", style="op2i", imm=(0, 15),
             evrnd=True, area="float", refblock=_rb, m=True,
             ssmask=True,
             names=[("_mm_%s_round_%s" % (_mn, _sfx),
                     "f", _es, 16)])
for _op, _mn, _rb in (("MIN", "min", "fminss"),
                      ("MAX", "max", "fmaxss")):
    for _es, _sfx in ((4, "ss"), (8, "sd")):
        _add("op2i", _op + "R", None, "f%d" % _es, (16,),
             ["v%s{sf} {D}, {A}, {B} {sae}" % _mn],
             feat="AVX512F", style="op2i", imm=(0, 15),
             flags=("EVSAE",),
             area="float", refblock=_rb, m=True,
             ssmask=True,
             names=[("_mm_%s_round_%s" % (_mn, _sfx),
                     "f", _es, 16)])
for _es, _sfx in ((4, "ss"), (8, "sd")):
    _add("op2i", "SQRTR", None, "f%d" % _es, (16,),
         ["vsqrt{sf} {D}, {A}, {B} {rn-sae}"],
         feat="AVX512F", style="op2i", imm=(0, 15), evrnd=True,
         area="float", refblock="fsqrtss", m=True,
         ssmask=True,
         names=[("_mm_sqrt_round_%s" % _sfx, "f", _es, 16)])
    _add("op2i", "SCALEFR", None, "f%d" % _es, (16,),
         ["vscalef{sf} {D}, {A}, {B} {rn-sae}"],
         feat="AVX512F", style="op2i", imm=(0, 15), evrnd=True,
         area="float", refblock="scalefss", m=True,
         ssmask=True,
         names=[("_mm_scalef_round_%s" % _sfx, "f", _es, 16)])
    _add("op2i", "GETEXPR", None, "f%d" % _es, (16,),
         ["vgetexp{sf} {D}, {A}, {B}"],
         feat="AVX512F", style="op2i", imm=(0, 15),
         area="float", refblock="getexpss2", m=True,
         ssmask=True,
         names=[("_mm_getexp_round_%s" % _sfx, "f", _es, 16)])
for _op, _mn, _rb in (("FMADDR", "vfmadd", "fma"),
                      ("FMSUBR", "vfmsub", "fmsub"),
                      ("FNMADDR", "vfnmadd", "fnmadd"),
                      ("FNMSUBR", "vfnmsub", "fnmsub"),
                      ("FMADDSUBR", "vfmaddsub", "fmaddsub"),
                      ("FMSUBADDR", "vfmsubadd", "fmsubadd")):
    _add("op3i", _op, None, "f4 f8", (64,),
         ["%s132{fs} {D}, {S}, {A}" % _mn],
         feat="AVX512F", style="op3i", form="DACC_A_BS",
         imm=(0, 15), evrnd=True, area="float", refblock=_rb,
         m=True, mpass=True,
         names=[("_mm512_%s_round_%s"
                 % (_mn[1:], _sfx), "f", _es, 64)
                for _sfx, _es in (("ps", 4), ("pd", 8))])
    if _op in ("FMADDR", "FMSUBR", "FNMADDR", "FNMSUBR"):
        for _es, _sfx in ((4, "ss"), (8, "sd")):
            _add("op3i", _op, None, "f%d" % _es, (16,),
                 ["%s132{sf} {D}, {S}, {A} {rn-sae}" % _mn],
                 feat="AVX512F", style="op3i", form="DACC_A_BS",
                 imm=(0, 15), evrnd=True, area="float",
                 refblock={"fma": "fmass", "fmsub": "fmssub",
                           "fnmadd": "fmnadd",
                           "fnmsub": "fmnsub"}[_rb],
                 m=True, mpass=True, ssmask=True,
                 names=[("_mm_%s_round_%s" % (_mn[1:], _sfx),
                         "f", _es, 16)])

for _op, _mn in (("FMADDR", "vfmadd"), ("FMSUBR", "vfmsub"),
                 ("FNMADDR", "vfnmadd"), ("FNMSUBR", "vfnmsub"),
                 ("FMADDSUBR", "vfmaddsub"),
                 ("FMSUBADDR", "vfmsubadd")):
    _add("op3i", _op, None, "f4 f8", (64,),
         ["%s132{fs} {D}, {S}, {A}" % _mn],
         style="custom", form="DACC_A_BS", feat="AVX512F",
         imm=(0, 15), evrnd=True,
         args="a, b, c, k, r",
         macro="__builtin_ia32_vop3mi2({OPC}, (k), (c), (a),"
               " (b), (c), (r), {rt})",
         names=[("_mm512_mask3_%s_round_%s" % (_mn[1:], _sfx),
                 "f", _es, 64)
                for _sfx, _es in (("ps", 4), ("pd", 8))])
    if _op in ("FMADDR", "FMSUBR", "FNMADDR", "FNMSUBR"):
        _add("op3i", _op, None, "f4 f8", (16,),
             ["%s132{sf} {D}, {S}, {A} {rn-sae}" % _mn],
             style="custom", form="DACC_A_BS", feat="AVX512F",
             imm=(0, 15), evrnd=True,
             args="a, b, c, k, r",
             macro="__builtin_ia32_vop3mi2({OPC}, (k), (c), (a),"
                   " (b), (c), (r), {rt})",
             names=[("_mm_mask3_%s_round_%s" % (_mn[1:], _sfx),
                     "f", _es, 16)
                    for _sfx, _es in (("ss", 4), ("sd", 8))])

# ------------------------------------------------------------------
# narrowing converts (vpmov / vpmovs / vpmovus, spelled cvt in
# the intrinsic names), their store forms, the 64-bit float
# converts and the cvt_round embedded-rounding converts
_EC = {1: 0, 2: 1, 4: 2, 8: 3}
_TI = {8: 0, 16: 1, 32: 2, 64: 3}
for _ses, _des in ((2, 1), (4, 2), (4, 1), (8, 4), (8, 2),
                   (8, 1)):
    for _var, _mid, _rb in (("", "cvt", "pmov"),
                            ("S", "cvts", "pmovs"),
                            ("US", "cvtus", "pmovus")):
        _op = "PMOV%s%d_%d" % (_var, _ses, _des)
        _feat = "AVX512BW" if (_ses, _des) == (2, 1) \
            else "AVX512F"
        for _tot in (16, 32, 64):
            _otot = max(16, _tot * _des // _ses)
            _pre = {16: "_mm", 32: "_mm256", 64: "_mm512"}[_tot]
            _nm = "%s_%sepi%d_epi%d" % (_pre, _mid, _ses * 8,
                                        _des * 8)
            _add("op1", _op, None, "s%d" % _ses, (_tot,),
                 ["vpmov%s%s%s {D}, {A}" % (_var.lower(),
                  {1: "b", 2: "w", 4: "d", 8: "q"}[_ses],
                  {1: "b", 2: "w", 4: "d", 8: "q"}[_des])],
                 feat=_feat, kind="CVT", oes=_des, otot=_otot,
                 style="op1", area="cvt", refblock=_rb, m=True,
                 names=[(_nm, "s", _ses, _tot)])
            _add("op1", _op, None, "s%d" % _ses, (_tot,),
                 ["vpmov%s%s%s {D}, {A}" % (_var.lower(),
                  {1: "b", 2: "w", 4: "d", 8: "q"}[_ses],
                  {1: "b", 2: "w", 4: "d", 8: "q"}[_des])],
                 feat=_feat, kind="CVT", oes=_des, otot=_otot,
                 style="custom", area="cvt", args="p, k, a",
                 macro="__builtin_ia32_vstorem(%d, (k), (p),"
                       " __builtin_ia32_vop1({OPC}, (a), {rt}))"
                       % ((1 << 5) | _rsid),
                 names=[("%s_mask_%sepi%d_storeu_epi%d"
                         % (_pre, _mid, _ses * 8, _des * 8),
                         "s", _ses, _tot)])
for _op, _tpl, _rb, _oes, _otot, _nm in (
        ("CVTEPI64PD", "vcvtqq2pd {D}, {A}", "cvt_f2f", 8,
         None, "cvtepi64_pd"),
        ("CVTEPI64PS", "vcvtqq2ps {D}, {A}", "cvtq2f", 4,
         "half", "cvtepi64_ps"),
        ("CVTEPU64PD", "vcvtuqq2pd {D}, {A}", "cvtuwiden", 8,
         None, "cvtepu64_pd"),
        ("CVTEPU64PS", "vcvtuqq2ps {D}, {A}", "cvtq2fu", 4,
         "half", "cvtepu64_ps")):
    for _tot in (16, 32, 64):
        _pre = {16: "_mm", 32: "_mm256", 64: "_mm512"}[_tot]
        _ot = _tot if _otot is None else max(16, _tot // 2)
        _add("op1", _op, None, "s8", (_tot,), [_tpl],
             feat="AVX512DQ", kind="CVT", oes=_oes, otot=_ot,
             flags=("OFL",), style="op1", area="cvt",
             refblock=_rb, m=True,
             names=[("%s_%s" % (_pre, _nm), "f", 8, _tot)])
for _op, _tpl, _rb, _nm in (
        ("CVTEPU32PD", "vcvtudq2pd {D}, {A}", "cvtuwiden",
         "cvtepu32_pd"),
        ("CVTEPU32PS", "vcvtudq2ps {D}, {A}", "cvtuwiden",
         "cvtepu32_ps")):
    for _tot in ((16,) if _nm.endswith("_pd") else (32, 64)):
        _pre = {16: "_mm256", 32: "_mm256", 64: "_mm512"}[_tot]
        _ot = 32 if _nm.endswith("_pd") else _tot
        _add("op1", _op, None, "s4", (_tot,), [_tpl],
             feat="AVX512DQ", kind="CVT", oes=8 if _nm.endswith(
                 "_pd") else 4, otot=_ot, flags=("OFL",),
             style="op1", area="cvt", refblock=_rb, m=True,
             names=[("%s_%s" % (_pre, _nm), "f", 4, _tot)])
_add("op1", "CVTEPU32PD", None, "s4", (16,),
     ["vcvtudq2pd {D}, {A}"],
     feat="AVX512DQ", kind="CVT", oes=8, otot=16, flags=("OFL",),
     style="op1", area="cvt", refblock="cvtuwiden", m=True,
     names=[("_mm_cvtepu32_pd", "f", 4, 16)])
_add("op1", "CVTEPU32PS", None, "s4", (16,),
     ["vcvtudq2ps {D}, {A}"],
     feat="AVX512DQ", kind="CVT", oes=4, flags=("OFL",),
     style="op1", area="cvt", refblock="cvtuwiden", m=True,
     names=[("_mm_cvtepu32_ps", "f", 4, 16)])
for _var, _mid, _rb in (("", "cvt", "cvt_rint"),
                        ("TT", "cvtt", "cvt_trunc")):
    for _dst, _sfx, _kind in (("QQ", "epi64", "s"),
                              ("UQQ", "epu64", "u")):
        _op = "CVT%sPD2%s" % (_var, _dst)
        _add("op1", _op, None, "f8", (16, 32, 64),
             ["vcvt%spd2%s {D}, {A}" % ("t" if _var else "",
                                        _dst.lower())],
             feat="AVX512DQ", kind="CVT", oes=8, style="op1",
             area="cvt", refblock=_rb if _kind == "s"
             else ("cvt_rintu" if _var == "" else "cvt_truncu"),
             m=True,
             names=[("%s_%spd_%s" % (_pre, _mid, _sfx),
                     _kind, 8, _tot)
                    for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                       ("_mm512", 64))])
        _op = "CVT%sPS2%s" % (_var, _dst)
        for _tot, _otot, _pre in ((16, 16, "_mm"),
                                  (16, 32, "_mm256"),
                                  (32, 64, "_mm512")):
            _add("op1", _op, None, "f4", (_tot,),
                 ["vcvt%sps2%s {D}, {A}" % ("t" if _var else "",
                                            _dst.lower())],
                 feat="AVX512DQ", kind="CVT", oes=8,
                 otot=_otot, style="op1", area="cvt",
                 refblock=(_rb + "w") if _kind == "s"
                 else ("cvt_rintuw" if _var == ""
                       else "cvt_truncuw"),
                 m=True,
                 names=[("%s_%sps_%s" % (_pre, _mid, _sfx),
                         _kind, 4, _tot)])
for _op, _tpl, _types, _tot, _oes, _otot, _feat, _rb, _nm, \
        _rk, _res, _rtot in (
        ("CVTRDQ2PS", "vcvtdq2ps {D}, {A}", "s4", 64, 4, None,
         "AVX512F", "cvt_f2f", "_mm512_cvt_roundepi32_ps",
         "f", 4, 64),
        ("CVTREPI64PD", "vcvtqq2pd {D}, {A}", "s8", 64, 8, None,
         "AVX512DQ", "cvt_f2f", "_mm512_cvt_roundepi64_pd",
         "f", 8, 64),
        ("CVTREPI64PS", "vcvtqq2ps {D}, {A}", "s8", 64, 4, 32,
         "AVX512DQ", "cvtq2f", "_mm512_cvt_roundepi64_ps",
         "f", 8, 64),
        ("CVTREPU32PS", "vcvtudq2ps {D}, {A}", "s4", 64, 4, None,
         "AVX512F", "cvtuwiden", "_mm512_cvt_roundepu32_ps",
         "f", 4, 64),
        ("CVTREPU64PD", "vcvtuqq2pd {D}, {A}", "s8", 64, 8, None,
         "AVX512DQ", "cvtuwiden", "_mm512_cvt_roundepu64_pd",
         "f", 8, 64),
        ("CVTREPU64PS", "vcvtuqq2ps {D}, {A}", "s8", 64, 4, 32,
         "AVX512DQ", "cvtq2fu", "_mm512_cvt_roundepu64_ps",
         "f", 8, 64),
        ("CVTRPD2DQ", "vcvtpd2dq {D}, {A}", "f8", 64, 4, 32,
         "AVX512F", "cvt_rint", "_mm512_cvt_roundpd_epi32",
         "s", 8, 64),
        ("CVTRPD2QQ", "vcvtpd2qq {D}, {A}", "f8", 64, 8, None,
         "AVX512DQ", "cvt_rint", "_mm512_cvt_roundpd_epi64",
         "s", 8, 64),
        ("CVTRPD2UDQ", "vcvtpd2udq {D}, {A}", "f8", 64, 4, 32,
         "AVX512F", "cvt_rintu", "_mm512_cvt_roundpd_epu32",
         "u", 8, 64),
        ("CVTRPD2UQQ", "vcvtpd2uqq {D}, {A}", "f8", 64, 8, None,
         "AVX512DQ", "cvt_rintu", "_mm512_cvt_roundpd_epu64",
         "u", 8, 64),
        ("CVTRPD2PS", "vcvtpd2ps {D}, {A}", "f8", 64, 4, 32,
         "AVX512F", "cvt_f2f", "_mm512_cvt_roundpd_ps",
         "f", 8, 64),
        ("CVTRPS2DQ", "vcvtps2dq {D}, {A}", "f4", 64, 4, None,
         "AVX512F", "cvt_rint", "_mm512_cvt_roundps_epi32",
         "s", 4, 64),
        ("CVTRPS2QQ", "vcvtps2qq {D}, {A}", "f4", 32, 8, 64,
         "AVX512DQ", "cvt_rint", "_mm512_cvt_roundps_epi64",
         "s", 4, 32),
        ("CVTRPS2UDQ", "vcvtps2udq {D}, {A}", "f4", 64, 4, None,
         "AVX512F", "cvt_rintu", "_mm512_cvt_roundps_epu32",
         "u", 4, 64),
        ("CVTRPS2UQQ", "vcvtps2uqq {D}, {A}", "f4", 32, 8, 64,
         "AVX512DQ", "cvt_rintu", "_mm512_cvt_roundps_epu64",
         "u", 4, 32),
        ("CVTRPS2PD", "vcvtps2pd {D}, {A}", "f4", 32, 8, 64,
         "AVX512F", "cvt_f2f", "_mm512_cvt_roundps_pd",
         "f", 4, 32)):
    _kw = dict(otot=_otot) if _otot is not None else {}
    if _rk == "f":
        _kw["flags"] = ("OFL",)
    _add("op1i", _op, None, _types, (_tot,), [_tpl],
         feat=_feat, kind="CVT", oes=_oes,
         style="op1i", imm=(0, 15), evrnd=True, area="cvt",
         refblock=_rb, m=True, names=[(_nm, _rk, _res, _rtot)],
         **_kw)

# ------------------------------------------------------------------
# masked moves, masked set1, masked loads and stores, the AVX
# vector-mask loads and stores, mask compress-stores and the
# mask identity: header macros over the blendm generic, the
# loadm / storem generics and the shipped set1 / compress macros
_PRE = {16: "_mm", 32: "_mm256", 64: "_mm512"}
_SHAPES = (("epi8", "s", 1), ("epi16", "s", 2), ("epi32", "s", 4),
           ("epi64", "s", 8), ("ps", "f", 4), ("pd", "f", 8))
for _tot in (16, 32, 64):
    _pre = _PRE[_tot]
    for _sfx, _k, _es in _SHAPES:
        _feat = "AVX512BW" if _es <= 2 and _k == "s" \
            else "AVX512F"
        _tup = [("%s_mask_mov_%s" % (_pre, _sfx), _k, _es, _tot)]
        _hdr(_tup, "a, k, b",
             "__builtin_ia32_vopm({opc}, (k), (a), (b), {rt})",
             feat=_feat)
        _zero = "%s_setzero_%s" % (
            _pre, _sfx if _k == "f" else
            {16: "si128", 32: "si256", 64: "si512"}[_tot])
        _hdr([("%s_maskz_mov_%s" % (_pre, _sfx), _k, _es, _tot)],
             "k, a",
             "__builtin_ia32_vopm({opc}, (k), (%s()), (a), {rt})"
             % _zero, feat=_feat)
        _s1 = "%s_set1_%s" % (_pre, "epi64x" if _es == 8
                              and _tot < 64 else _sfx)
        if _k == "s":
            _hdr([("%s_mask_set1_%s" % (_pre, _sfx),
                   _k, _es, _tot)],
                 "a, k, b",
                 "%s_mask_mov_%s((a), (k), %s((b)))"
                 % (_pre, _sfx, _s1), feat=_feat)
            _hdr([("%s_maskz_set1_%s" % (_pre, _sfx),
                   _k, _es, _tot)],
                 "k, b",
                 "%s_maskz_mov_%s((k), %s((b)))"
                 % (_pre, _sfx, _s1), feat=_feat)
        _hdr([("%s_maskz_loadu_%s" % (_pre, _sfx), _k, _es, _tot)],
             "k, p",
             "__builtin_ia32_vloadm({opc} + 96, (k), (p), {rt})",
             feat=_feat)
        if _es > 2 or _k == "f":
            _hdr([("%s_maskz_load_%s" % (_pre, _sfx),
                   _k, _es, _tot)],
                 "k, p",
                 "__builtin_ia32_vloadm({opc} + 64, (k), (p),"
                 " {rt})", feat=_feat)
        _hdr([("%s_mask_loadu_%s" % (_pre, _sfx), _k, _es, _tot)],
             "a, k, p",
             "__builtin_ia32_vopm({opc}, (k), (a),"
             " __builtin_ia32_vloadm({opc} + 96, (k), (p), {rt}),"
             " {rt})", feat=_feat)
        if _es > 2 or _k == "f":
            _hdr([("%s_mask_load_%s" % (_pre, _sfx),
                   _k, _es, _tot)],
                 "a, k, p",
                 "__builtin_ia32_vopm({opc}, (k), (a),"
                 " __builtin_ia32_vloadm({opc} + 64, (k), (p),"
                 " {rt}), {rt})", feat=_feat)
            _hdr([("%s_mask_store_%s" % (_pre, _sfx),
                   _k, _es, _tot)],
                 "p, k, a",
                 "__builtin_ia32_vstorem({opc}, (k), (p), (a))",
                 feat=_feat)
        _hdr([("%s_mask_storeu_%s" % (_pre, _sfx), _k, _es, _tot)],
             "p, k, a",
             "__builtin_ia32_vstorem({opc} + 32, (k), (p), (a))",
             feat=_feat)
for _sfx, _es in (("ss", 4), ("sd", 8)):
    _hdr([("_mm_maskz_load_%s" % _sfx, "f", _es, 16)],
         "k, p",
         "__builtin_ia32_vloadm({opc} + 128, (k), (p), {rt})",
         feat="AVX512F")
    _hdr([("_mm_mask_load_%s" % _sfx, "f", _es, 16)],
         "a, k, p",
         "_mm_mask_move_%s((a), (k), (a), _mm_maskz_load_%s((k),"
         " (p)))" % (_sfx, _sfx), feat="AVX512F")
    _hdr([("_mm_mask_store_%s" % _sfx, "f", _es, 16)],
         "p, k, a",
         "__builtin_ia32_vstorem({opc} + 64, (k), (p), (a))",
         feat="AVX512F")
for _tot in (16, 32):
    _pre = _PRE[_tot]
    for _sfx, _k, _es in (("ps", "f", 4), ("pd", "f", 8),
                          ("epi32", "s", 4), ("epi64", "s", 8)):
        _feat = "AVX" if _k == "f" else "AVX2"
        _hdr([("%s_maskload_%s" % (_pre, _sfx), _k, _es, _tot)],
             "p, m",
             "__builtin_ia32_vloadm({opc}, (m), (p), {rt})",
             feat=_feat)
        _hdr([("%s_maskstore_%s" % (_pre, _sfx), _k, _es, _tot)],
             "p, m, a",
             "__builtin_ia32_vstorem({opc}, (m), (p), (a))",
             feat=_feat)
for _tot in (16, 32, 64):
    _pre = _PRE[_tot]
    for _sfx, _k, _es in (("epi32", "s", 4), ("epi64", "s", 8),
                          ("ps", "f", 4), ("pd", "f", 8)):
        _ec = _EC[_es] if _k == "s" else (4 if _es == 4 else 5)
        _sid = (_ec << 2) | _TI[_tot]
        _hdr([("%s_mask_compressstoreu_%s" % (_pre, _sfx),
               _k, _es, _tot)],
             "p, k, a",
             "({ unsigned long long n_ = (unsigned long long)"
             "__builtin_popcountll((unsigned long long)(k));"
             " __builtin_ia32_vstorem(%d, n_ ?"
             " (~0ULL >> (64 - n_)) : 0, (p),"
             " %s_maskz_compress_%s((k), (a))); })"
             % ((1 << 5) | _sid, _pre, _sfx),
             feat="AVX512F")
_hdr([("_mm512_kmov", "s", 2, 16)], "a", "(a)", feat="AVX512F")

# ------------------------------------------------------------------
# remaining in-model forms: 64-bit absolute value, the 512-bit
# ceil / floor forms, the wider casts, the mask identity
# conversions, the element alignr forms and the mask3 FMA
# signature permutation
_add("op1", "ABSQ", "abs", "s8", (16, 32, 64),
     ["vpabsq {D}, {A}"],
     feat="AVX512DQ", style="op1", area="arith", refblock="abs",
     m=True)
for _sfx, _es, _const in (("ps", 4, "2147483647"),
                          ("pd", 8, "9223372036854775807LL")):
    _hdr([("_mm512_abs_%s" % _sfx, "f", _es, 64)], "a",
         "_mm512_and_%s((a), _mm512_castsi512_%s("
         "_mm512_set1_epi%d(%s)))"
         % (_sfx, _sfx, _es * 8, _const), feat="AVX512F")
for _op, _tot, _pre in (("ROUNDSCALE", 64, "_mm512"),):
    for _es, _insn in ((4, "ps"), (8, "pd")):
        for _fx, _iv in (("ceil", 2), ("floor", 1)):
            _add("op1i", _op, None, "f%d" % _es, (_tot,),
                 [], feat="AVX512F", style="op1i", imm=(0, 15),
                 imm_fixed=_iv, area="float",
                 names=[("%s_%s_%s" % (_pre, _fx, _insn),
                         "f", _es, _tot)])
for _tot, _si, _pre, _vs in ((32, "si256", "_mm256", "__m256"),
                             (64, "si512", "_mm512", "__m512")):
    for _nm, _k, _es, _rt in (
            ("%s_castps_pd" % _pre, "f", 8, _vs + "d"),
            ("%s_castpd_ps" % _pre, "f", 4, _vs),
            ("%s_castps_%s" % (_pre, _si), "x", 8, _vs + "i"),
            ("%s_castpd_%s" % (_pre, _si), "x", 8, _vs + "i"),
            ("%s_cast%s_ps" % (_pre, _si), "f", 4, _vs),
            ("%s_cast%s_pd" % (_pre, _si), "f", 8, _vs + "d")):
        _hdr([(_nm, _k, _es, _tot)], "a", "((%s)(a))" % _rt,
             feat="AVX" if _tot == 32 else "AVX512F")
_hdr([("_mm512_int2mask", "s", 2, 16)], "a", "((__mmask16)(a))",
     feat="AVX512F")
_hdr([("_mm512_mask2int", "s", 4, 16)], "a", "((int)(a))",
     feat="AVX512F")
for _es in (4, 8):
    _add("op2i", "ALIGNR%d" % _es, None, "s%d" % _es, (16, 32, 64),
         ["vpalignr {D}, {A}, {B}, {i}"],
         style="custom", feat="AVX512F", imm=(0, 255),
         args="a, b, i",
         macro="__builtin_ia32_vop2i({OPC}, (a), (b),"
               " ((i) * %d), {rt})" % _es,
         names=[("%s_alignr_epi%d" % (_pre, _es * 8),
                 "s", _es, _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))],
         refblock="alignr")
for _op in ("FMADD", "FMSUB", "FNMADD", "FNMSUB", "FMADDSUB",
            "FMSUBADD"):
    _add("op3", _op, None, "f4 f8", (16, 32, 64),
         ["%s132{fs} {D}, {S}, {A}" %
          {"FMADD": "vfmadd", "FMSUB": "vfmsub",
           "FNMADD": "vfnmadd", "FNMSUB": "vfnmsub",
           "FMADDSUB": "vfmaddsub",
           "FMSUBADD": "vfmsubadd"}[_op]],
         style="custom", form="DACC_A_BS", feat="FMA",
         args="a, b, c, k",
         macro="__builtin_ia32_vop3m2({OPC}, (k), (c), (a),"
               " (b), (c), {rt})",
         names=[("%s_mask3_%s_%s" % (_pre, _op.lower(), _sfx),
                 "f", _es, _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))
                for _sfx, _es in (("ps", 4), ("pd", 8))])
    if _op in ("FMADD", "FMSUB", "FNMADD", "FNMSUB"):
        _add("op3", _op + "SS", None, "f4 f8", (16,),
             ["%s132{sf} {D}, {S}, {A}" %
              {"FMADD": "vfmadd", "FMSUB": "vfmsub",
               "FNMADD": "vfnmadd",
               "FNMSUB": "vfnmsub"}[_op]],
             style="custom", form="DACC_A_BS", feat="FMA",
             args="a, b, c, k",
             macro="__builtin_ia32_vop3m2({OPC}, (k), (c), (a),"
                   " (b), (c), {rt})",
             names=[("_mm_mask3_%s_%s" % (_op.lower(), _sfx),
                     "f", _es, 16)
                    for _sfx, _es in (("ss", 4), ("sd", 8))])
for _tot, _pre in ((32, "_mm256"), (64, "_mm512")):
    _hdr([("%s_undefined_ps" % _pre, "f", 4, _tot)], "",
         "__builtin_ia32_splat(%d, 0, __m%d)"
         % (17 + _TI[_tot] - 1, _tot * 8))
    _hdr([("%s_undefined_pd" % _pre, "f", 8, _tot)], "",
         "__builtin_ia32_splat(%d, 0, __m%dd)"
         % (21 + _TI[_tot] - 1, _tot * 8), feat="AVX512F"
         if _tot == 64 else "AVX")
    _hdr([("%s_undefined_si%d" % (_pre, _tot * 8), "x", 8, _tot)],
         "", "__builtin_ia32_splat(%d, 0, __m%di)"
         % (13 + _TI[_tot] - 1, _tot * 8),
         feat="AVX512F" if _tot == 64 else "AVX2")
_hdr([("_mm512_setzero", "f", 4, 64)], "",
     "__builtin_ia32_splat({opc}, 0, {rt})", feat="AVX512F")
_hdr([("_mm512_setzero_epi32", "s", 4, 64)], "",
     "__builtin_ia32_splat({opc}, 0, {rt})", feat="AVX512F")
_hdr([("_mm512_undefined", "f", 4, 64)], "",
     "__builtin_ia32_splat(19, 0, __m512)", feat="AVX512F")
_hdr([("_mm512_undefined_epi32", "s", 4, 64)], "",
     "__builtin_ia32_splat(15, 0, __m512i)", feat="AVX512F")
for _sfx, _es in (("ps", 4), ("pd", 8)):
    _hdr([("_mm512_mask_abs_%s" % _sfx, "f", _es, 64)],
         "a, k, b",
         "_mm512_mask_mov_%s((a), (k), _mm512_abs_%s((b)))"
         % (_sfx, _sfx), feat="AVX512F")
    for _fx in ("ceil", "floor"):
        _hdr([("_mm512_mask_%s_%s" % (_fx, _sfx), "f", _es, 64)],
             "a, k, b",
             "_mm512_mask_mov_%s((a), (k), _mm512_%s_%s((b)))"
             % (_sfx, _fx, _sfx), feat="AVX512F")
_hdr([("_mm512_mullox_epi64", "s", 8, 64)], "a, b",
     "_mm512_mullo_epi64((a), (b))", feat="AVX512DQ")
_hdr([("_mm512_mask_mullox_epi64", "s", 8, 64)], "a, k, b",
     "_mm512_mask_mov_epi64((a), (k),"
     " _mm512_mullox_epi64((a), (b)))", feat="AVX512DQ")

# ------------------------------------------------------------------
# the immediate dword blend, the byte-sum-of-differences, the
# 64-bit maximum / minimum and multiply-low forms, the VBMI
# multishift and the 512-bit subvector extract / insert forms
_add("op2i", "BLENDD", "blend", "s4", (16, 32),
     ["vpblendd {D}, {A}, {B}, {i}"],
     feat="AVX512F", style="op2i", imm=(0, 255),
     imm_mode="mask", area="logic", refblock="blend", m=True)
_add("op2i", "DBSAD", None, "s1", (16, 32, 64),
     ["vdbpsadbw {D}, {A}, {B}, {i}"],
     feat="AVX512BW", style="op2i",
     kind="CVT", oes=2, imm=(0, 255), area="arith",
     refblock="dbsad", m=True,
     names=[("%s_dbsad_epu8" % _pre, "s", 1, _tot)
            for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                               ("_mm512", 64))])
for _op, _tpl, _cmp in (("MAXQ", "vpmaxsq {D}, {A}, {B}", ">"),
                        ("MINQ", "vpminsq {D}, {A}, {B}", "<")):
    _add("op2", _op, None, "s8", (16, 32, 64), [_tpl],
         feat="AVX512DQ", style="op2", area="arith",
         ref="(a[i] %s b[i]) ? a[i] : b[i]" % _cmp, m=True,
         names=[("%s_%s_epi64" % (_pre, _op[:3].lower()),
                 "s", 8, _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))])
for _op, _tpl, _cmp in (("MAXUQ", "vpmaxuq {D}, {A}, {B}", ">"),
                        ("MINUQ", "vpminuq {D}, {A}, {B}", "<")):
    _add("op2", _op, None, "u8", (16, 32, 64), [_tpl],
         feat="AVX512DQ", style="op2", area="arith",
         ref="(a[i] %s b[i]) ? a[i] : b[i]" % _cmp, m=True,
         signed_names=True,
         names=[("%s_%s_epu64" % (_pre, _op[:3].lower()),
                 "u", 8, _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))])
_add("op2", "MULLQ", "mullo", "s8", (16, 32, 64),
     ["vpmullq {D}, {A}, {B}"],
     feat="AVX512DQ", style="op2", area="arith",
     ref="a[i] * b[i]", m=True)
_add("op2", "MULTISHIFT", None, "s1", (16, 32, 64),
     ["vpmultishiftqb {D}, {A}, {B}"],
     feat="AVX512VBMI", style="op2", area="vbmi",
     refblock="multishift", m=True,
     names=[("%s_multishift_epi64_epi8" % _pre, "s", 1, _tot)
            for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                               ("_mm512", 64))])
for _kind, _es, _tag in (("f", 4, "f32x4"), ("f", 8, "f64x2"),
                         ("s", 4, "i32x4"), ("s", 8, "i64x2")):
    _add("op1i", "VEXTRACTX4", None, "%s%d" % (_kind, _es), (64,),
         ["vextract%s {D}, {A}, {i}" % _tag],
         feat="AVX512F", kind="CVT", oes=_es, otot=16,
         flags=("OFL",) if _kind == "f" else (),
         style="op1i", imm=(0, 3), area="permute", m=True,
         names=[("_mm512_extract%s_%s" % (_tag,
                 "ps" if _es == 4 and _kind == "f" else
                 "pd" if _kind == "f" else "epi%d" % (_es * 8)),
                 _kind, _es, 64)],
         refblock="extractx4")
    _add("op1i", "VEXTRACTX4", None, "%s%d" % (_kind, _es), (32,),
         ["vextract%s {D}, {A}, {i}" % _tag],
         feat="AVX512F", kind="CVT", oes=_es, otot=16,
         flags=("OFL",) if _kind == "f" else (),
         style="op1i", imm=(0, 1), area="permute", m=True,
         names=[("_mm256_extract%s_%s" % (_tag,
                 "ps" if _es == 4 and _kind == "f" else
                 "pd" if _kind == "f" else "epi%d" % (_es * 8)),
                 _kind, _es, 32)],
         refblock="extractx4")
for _kind, _es, _tag in (("f", 4, "f32x4"), ("f", 8, "f64x2"),
                         ("s", 4, "i32x4"), ("s", 8, "i64x2")):
    _add("op2i", "SHUFX4", None, "%s%d" % (_kind, _es), (64,),
         ["vshuf%s {D}, {A}, {B}, {i}" % _tag],
         feat="AVX512F", style="op2i", imm=(0, 255),
         area="permute", refblock="shufx4", m=True,
         names=[("%s_shuffle_%s" % (_pre, _tag),
                 _kind, _es, _tot)
                for _pre, _tot in (("_mm512", 64),)])
    _add("op2i", "SHUFX4", None, "%s%d" % (_kind, _es), (32,),
         ["vshuf%s {D}, {A}, {B}, {i}" % _tag],
         feat="AVX512F", style="op2i", imm=(0, 255),
         area="permute", refblock="shufx4", m=True,
         names=[])
    _add("op2i", "SHUFX4", None, "%s%d" % (_kind, _es), (32,),
         ["vshuf%s {D}, {A}, {B}, {i}" % _tag],
         style="custom", feat="AVX512F", imm=(0, 255),
         args="src, k, a, b, i",
         macro="__builtin_ia32_vop2mi2({OPC}, (k), (src), (a),"
               " (b), (i), {rt})",
         names=[("_mm256_mask_shuffle_%s" % _tag,
                 _kind, _es, 32)])
    _add("op2i", "SHUFX4", None, "%s%d" % (_kind, _es), (32,),
         ["vshuf%s {D}, {A}, {B}, {i}" % _tag],
         style="custom", feat="AVX512F", imm=(0, 255),
         args="k, a, b, i",
         macro="__builtin_ia32_vop2mi({OPC}, (k), (a), (b),"
               " (i), {rt})",
         names=[("_mm256_maskz_shuffle_%s" % _tag,
                 _kind, _es, 32)])
for _op, _tpls in (("AND", ["pand {D}, {B}", "vpand {D}, {A}, {B}",
                            "vpandd {D}, {A}, {B}"]),
                   ("OR", ["por {D}, {B}", "vpor {D}, {A}, {B}",
                           "vpord {D}, {A}, {B}"]),
                   ("XOR", ["pxor {D}, {B}", "vpxor {D}, {A}, {B}",
                            "vpxord {D}, {A}, {B}"]),
                   ("ANDNOT", ["pandn {D}, {B}",
                               "vpandn {D}, {A}, {B}",
                               "vpandnd {D}, {A}, {B}"])):
    for _es in (4, 8):
        for _pre, _tot in (("_mm", 16), ("_mm256", 32)):
            _nm = "%s_%s_epi%d" % (_pre, _op.lower(), _es * 8)
            _add("op2", _op, None, "s%d" % _es, (_tot,), _tpls,
                 style="custom", feat="AVX512F", area="logic",
                 args="src, k, a, b",
                 macro="__builtin_ia32_vopm2({OPC}, (k), (src),"
                       " (a), (b), {rt})",
                 names=[("%s_mask_%s_epi%d"
                         % (_pre, _op.lower(), _es * 8),
                         "s", _es, _tot)])
            _add("op2", _op, None, "s%d" % _es, (_tot,), _tpls,
                 style="custom", feat="AVX512F", area="logic",
                 args="k, a, b",
                 macro="__builtin_ia32_vopm({OPC}, (k), (a),"
                       " (b), {rt})",
                 names=[("%s_maskz_%s_epi%d"
                         % (_pre, _op.lower(), _es * 8),
                         "s", _es, _tot)])
for _op, _tpls, _rb in (
        ("AND", ["vpand{ss} {D}, {A}, {B}"], "a[i] & b[i]"),
        ("OR", ["vpor{ss} {D}, {A}, {B}"], "a[i] | b[i]"),
        ("XOR", ["vpxor{ss} {D}, {A}, {B}"], "a[i] ^ b[i]"),
        ("ANDNOT", ["vpandn{ss} {D}, {A}, {B}"], None)):
    _kw = dict(refblock="bitandnot") if _rb is None else \
        dict(ref=_rb)
    # AND / ANDNOT also derive at the VL widths: the masked
    # VL customs resolve their opcodes through this entry
    _tots = (16, 32, 64) if _op in ("AND", "ANDNOT") else (64,)
    _add("op2", _op, None, "s4 s8", _tots, _tpls,
         feat="AVX512F", style="op2", area="logic", m=True,
         names=[("_mm512_%s_epi32" % _op.lower(), "s", 4, 64),
                ("_mm512_%s_epi64" % _op.lower(), "s", 8, 64)],
         **_kw)
for _op, _kind, _es, _tag in (("PERM2F128", "f", 4, "f32x4"),
                              ("PERM2F128", "f", 8, "f64x2"),
                              ("PERM2I128", "s", 4, "i32x4"),
                              ("PERM2I128", "s", 8, "i64x2")):
    _add("op2i", _op, None, "%s%d" % (_kind, _es), (32,),
         ["vperm2f128 {D}, {A}, {B}, {i}"] if _kind == "f"
         else ["vperm2i128 {D}, {A}, {B}, {i}"],
         style="custom", feat="AVX", imm=(0, 3), args="a, b, i",
         macro="__builtin_ia32_vop2i({OPC}, (a), (b),"
               " ((i) & 1) | (((i) & 2) << 3), {rt})",
         names=[("_mm256_shuffle_%s" % _tag, _kind, _es, 32)],
         refblock="perm2f128")
_add("op2i", "PERM2I128", None, "s4", (32,),
     ["vperm2i128 {D}, {A}, {B}, {i}"],
     feat="AVX2", style="op2i", imm=(0, 255), area="permute",
     names=[], refblock="perm2f128")

# ------------------------------------------------------------------
# spelling aliases: the integer load / store names, the mask
# to vector forms and the wide element extracts (composed from
# the 128-bit extract and the chunk extract)
for _tot, _pre in ((16, "_mm"), (32, "_mm256"), (64, "_mm512")):
    _lf = "SSE2" if _tot == 16 else "AVX" if _tot == 32 \
        else "AVX512F"
    for _sfx in ("epi32", "epi64"):
        _hdr([("%s_load_%s" % (_pre, _sfx), "x", 8, _tot)],
             "a", "(*({rt} const *)(a))", feat=_lf)
        _hdr([("%s_store_%s" % (_pre, _sfx), "x", 8, _tot)],
             "a, b", "(*({rt} *)(a) = (b))", feat=_lf)
    for _sfx in ("epi8", "epi16", "epi32", "epi64"):
        _hdr([("%s_loadu_%s" % (_pre, _sfx), "x", 8, _tot)],
             "a", "(*({rt} const *)(a))", feat=_lf)
        _hdr([("%s_storeu_%s" % (_pre, _sfx), "x", 8, _tot)],
             "a, b", "(*({rt} *)(a) = (b))", feat=_lf)
    for _sfx, _es in (("epi8", 1), ("epi16", 2), ("epi32", 4),
                      ("epi64", 8)):
        _sid = (_EC[_es] << 2) | _TI[_tot]
        _hdr([("%s_movm_%s" % (_pre, _sfx), "s", _es, _tot)],
             "a",
             "__builtin_ia32_vopm(%d, (a),"
             " __builtin_ia32_splat(%d, 0, {rt}),"
             " __builtin_ia32_splat(%d, -1, {rt}), {rt})"
             % (_sid, _sid, _sid),
             feat="AVX512BW" if _es <= 2 else "AVX512F")
for _pre, _tot in (("_mm256", 32), ("_mm512", 64)):
    for _sfx, _es in (("epi8", 1), ("epi16", 2), ("epi32", 4),
                      ("epi64", 8)):
        _cast = "%s_castsi%d_si128" % (_pre, _tot * 8)
        _ext = "%s_extractf128_si256" % _pre if _tot == 32 \
            else None
        if _tot == 32:
            _hdr([("%s_extract_%s" % (_pre, _sfx),
                   "s", _es, _tot)],
                 "a, i",
                 "_mm_extract_%s(%s_extractf128_si256((a),"
                 " ((i) * %d / 128)), ((i) %% %d))"
                 % (_sfx, _pre, _es * 8, 128 // (_es * 8)),
                 feat="AVX2")

for _op, _tpl, _rb in (("SADBW", "vpsadbw {D}, {A}, {B}", "sad"),):
    _add("op2", _op, None, "s1", (16, 32, 64), [_tpl],
         feat="AVX512BW", style="op2", area="arith",
         refblock=_rb, m=True,
         names=[("%s_sad_epu8" % _pre, "s", 1, _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32),
                                   ("_mm512", 64))])
_add("op1", "CVTEPU32PD512", None, "s4", (32,),
     ["vcvtudq2pd {D}, {A}"],
     feat="AVX512DQ", kind="CVT", oes=8, otot=64,
     flags=("OFL",), style="op1", area="cvt",
     refblock="cvtuwiden", m=True,
     names=[("_mm512_cvtepu32_pd", "f", 4, 32)])
for _op, _tpl, _rb in (("RCP14", "vrcp14{fs} {D}, {A}", "rcp14"),
                       ("RSQRT14", "vrsqrt14{fs} {D}, {A}",
                        "rsqrt14")):
    _add("op1", _op, _op.lower(), "f8", (16, 32, 64), [_tpl],
         feat="AVX512DQ", style="op1", area="float",
         refblock="none", m=True)
for _tot, _pre in ((32, "_mm256"), (64, "_mm512")):
    _add("op1i", "PERMUTEXQ", None, "s8", (_tot,),
         ["vpermq {D}, {A}, {i}"],
         style="op1i", imm=(0, 255), feat="AVX512F",
         area="permute", refblock="permq512", m=True,
         names=[("%s_permutex_epi64" % _pre, "s", 8, _tot)])
    _add("op1i", "PERMUTEXPD", None, "f8", (_tot,),
         ["vpermpd {D}, {A}, {i}"],
         style="op1i", imm=(0, 255), feat="AVX512F",
         area="permute", refblock="permq512", m=True,
         names=[("%s_permutex_pd" % _pre, "f", 8, _tot)])
for _op, _tpl, _rb in (
        ("CVTTRPD2DQ", "vcvttpd2dq {D}, {A}", "cvt_trunc"),
        ("CVTTRPD2QQ", "vcvttpd2qq {D}, {A}", "cvt_trunc"),
        ("CVTTRPD2UDQ", "vcvttpd2udq {D}, {A}", "cvt_truncu"),
        ("CVTTRPD2UQQ", "vcvttpd2uqq {D}, {A}", "cvt_truncu")):
    _oes = 4 if _op.endswith("DQ") else 8
    _kw = dict(otot=32) if _op.endswith("DQ") else {}
    _add("op1i", _op, None, "f8", (64,), [_tpl],
         feat="AVX512DQ" if _op.endswith("QQ") else "AVX512F",
         kind="CVT", oes=_oes, style="op1i",
         imm=(0, 15), flags=("EVSAE",), area="cvt", refblock=_rb,
         m=True,
         names=[("_mm512_cvtt_roundpd_%s"
                 % ("epi64" if _op.endswith("QQ") and
                    "U" not in _op else
                    "epu64" if _op.endswith("QQ") else
                    "epi32" if "U" not in _op else "epu32"),
                 "s" if "U" not in _op else "u", 8, 64)],
         **_kw)
for _op, _mn in (("OR", "vpor"), ("XOR", "vpxor")):
    _add("op2", _op, None, "s4 s8", (16, 32),
         ["%s{ss} {D}, {A}, {B}" % _mn],
         feat="AVX512F", style="op2", area="logic", m=True,
         names=[("%s_%s_epi32" % (_pre, _op.lower()), "s", 4,
                 _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32))] +
               [("%s_%s_epi64" % (_pre, _op.lower()), "s", 8,
                 _tot)
                for _pre, _tot in (("_mm", 16), ("_mm256", 32))],
         ref="a[i] | b[i]" if _op == "OR" else "a[i] ^ b[i]")
for _op, _tpl, _rb, _oes, _otot in (
        ("CVTTRPS2DQ", "vcvttps2dq {D}, {A}", "cvt_trunc", 4,
         None),
        ("CVTTRPS2UDQ", "vcvttps2udq {D}, {A}", "cvt_truncu", 4,
         None),
        ("CVTTRPS2QQ", "vcvttps2qq {D}, {A}", "cvt_truncw", 8,
         64),
        ("CVTTRPS2UQQ", "vcvttps2uqq {D}, {A}", "cvt_truncuw", 8,
         64)):
    _kw = dict(otot=_otot) if _otot is not None else {}
    _add("op1i", _op, None, "f4", (32 if _otot else 64,),
         [_tpl], feat="AVX512DQ" if _otot else "AVX512F",
         kind="CVT", oes=_oes, style="op1i", imm=(0, 15),
         flags=("EVSAE",), area="cvt", refblock=_rb, m=True,
         names=[("_mm512_cvtt_roundps_%s"
                 % (("epu64" if "U" in _op else "epi64") if _otot
                    else ("epu32" if "U" in _op else "epi32")),
                 "s" if "U" not in _op else "u", 4,
                 32 if _otot else 64)], **_kw)
for _kind, _es, _tag in (("f", 4, "f32x8"), ("f", 8, "f64x4"),
                         ("s", 4, "i32x8"), ("s", 8, "i64x4")):
    _add("op1i", "VEXTRACTX8", None, "%s%d" % (_kind, _es), (64,),
         ["vextract%s {D}, {A}, {i}" % _tag],
         feat="AVX512DQ", kind="CVT", oes=_es, otot=32,
         flags=("OFL",) if _kind == "f" else (),
         style="op1i", imm=(0, 1), area="permute", m=True,
         names=[("_mm512_extract%s_%s" % (_tag,
                 "ps" if _es == 4 and _kind == "f" else
                 "pd" if _kind == "f" else "epi%d" % (_es * 8)),
                 _kind, _es, 64)],
         refblock="extractx8")

# ------------------------------------------------------------------
# scalar moves and scalar reciprocal forms (appended: new op2
# operations go after the backend's own XOP2 enum block)
# ------------------------------------------------------------------
for _op, _es, _sfx in (("MOVSS", 4, "ss"), ("MOVSD", 8, "sd")):
    _add("op2", _op, None, "f%d" % _es, (16,),
         ["mov{sf} {D}, {B}", "vmov{sf} {D}, {D}, {B}"],
         feat="SSE" if _es == 4 else "SSE2", style="op2",
         form="D_B_ACC", area="float", refblock="movss",
         names=[("_mm_move_%s" % _sfx, "f", _es, 16)])
    # the masked moves merge into a separate passthrough for
    # the low element only, while the upper lanes always come
    # from the first operand: a masked scalar move into the
    # passthrough followed by a plain move over it composes
    # exactly that
    _pfx = "pd" if _es == 8 else "ps"
    _hdr([("_mm_mask_move_%s" % _sfx, "f", _es, 16)],
         "src, k, a, b",
         "_mm_move_%s((a), _mm_mask_mov_%s((src), (k), (b)))"
         % (_sfx, _pfx),
         feat="AVX512VL", area="float")
    _hdr([("_mm_maskz_move_%s" % _sfx, "f", _es, 16)],
         "k, a, b",
         "_mm_move_%s((a), _mm_maskz_mov_%s((k), (b)))"
         % (_sfx, _pfx),
         feat="AVX512VL", area="float")
_add("op1", "RCPSS", None, "f4", (16,), ["vrcpss {D}, {D}, {A}"],
     feat="SSE", style="op1", form="D_B_ACC", area="float",
     refblock="none",
     names=[("_mm_rcp_ss", "f", 4, 16)])
_add("op1", "RSQRTSS", None, "f4", (16,), ["vrsqrtss {D}, {D}, {A}"],
     feat="SSE", style="op1", form="D_B_ACC", area="float",
     refblock="none",
     names=[("_mm_rsqrt_ss", "f", 4, 16)])

# ------------------------------------------------------------------
# AVX-512 scalar float forms (appended: new op2 / op2i
# operations go after the backend's own enum blocks)
# ------------------------------------------------------------------
_add("op2", "GETEXPSS", None, "f4 f8", (16,),
     ["vgetexp{sf} {D}, {A}, {B}"],
     feat="AVX512F", style="op2", area="float",
     refblock="getexpss2", m=True, ssmask=True,
     names=[("_mm_getexp_ss", "f", 4, 16),
            ("_mm_getexp_sd", "f", 8, 16)])
_add("op2", "SCALEFSS", None, "f4 f8", (16,),
     ["vscalef{sf} {D}, {A}, {B}"],
     feat="AVX512F", style="op2", area="float",
     refblock="scalefss", m=True, ssmask=True,
     names=[("_mm_scalef_ss", "f", 4, 16),
            ("_mm_scalef_sd", "f", 8, 16)])
_add("op2i", "RANGESS", None, "f4 f8", (16,),
     ["vrange{sf} {D}, {A}, {B}, {i}"],
     feat="AVX512DQ", style="op2i", imm=(0, 15), area="float",
     refblock="rangess", m=True, ssmask=True,
     names=[("_mm_range_ss", "f", 4, 16),
            ("_mm_range_sd", "f", 8, 16)])
_add("op2i", "ROUNDSCALESS", None, "f4 f8", (16,),
     ["vrndscale{sf} {D}, {A}, {B}, {i}"],
     feat="AVX512F", style="op2i", imm=(0, 255), area="float",
     refblock="roundscaless", m=True, ssmask=True,
     names=[("_mm_roundscale_ss", "f", 4, 16),
            ("_mm_roundscale_sd", "f", 8, 16)])
for _op, _mn in (("RCP14SS", "vrcp14"), ("RSQRT14SS", "vrsqrt14")):
    _add("op2", _op, None, "f4 f8", (16,),
         ["%s{sf} {D}, {A}, {B}" % _mn],
         feat="AVX512F", style="op2", area="float",
         refblock="none", m=True, ssmask=True,
         names=[("_mm_%s_ss" % _mn[1:], "f", 4, 16),
                ("_mm_%s_sd" % _mn[1:], "f", 8, 16)])
_add("op2i", "GETMANTSS", None, "f4 f8", (16,),
     ["vgetmant{sf} {D}, {A}, {B}, {i}"],
     feat="AVX512F", style="op2i", imm=(0, 15), area="float",
     m=True, names=[])
_add("op2i", "GETMANTSS", None, "f4 f8", (16,),
     ["vgetmant{sf} {D}, {A}, {B}, {i}"],
     style="custom", feat="AVX512F", imm=(0, 15),
     args="a, b, n, s",
     macro="__builtin_ia32_vop2i({OPC}, (a), (b),"
           " ((n) + ((s) << 2)), {rt})",
     names=[("_mm_getmant_ss", "f", 4, 16),
            ("_mm_getmant_sd", "f", 8, 16)])
_add("op2i", "GETMANTSS", None, "f4 f8", (16,),
     ["vgetmant{sf} {D}, {A}, {B}, {i}"],
     style="custom", feat="AVX512F", imm=(0, 15),
     args="src, k, a, b, n, s",
     macro="__builtin_ia32_vop2mi2({OPC}, (k), (src), (a), (b),"
           " ((n) + ((s) << 2)), {rt})",
     names=[("_mm_mask_getmant_ss", "f", 4, 16),
            ("_mm_mask_getmant_sd", "f", 8, 16)])
_add("op2i", "GETMANTSS", None, "f4 f8", (16,),
     ["vgetmant{sf} {D}, {A}, {B}, {i}"],
     style="custom", feat="AVX512F", imm=(0, 15),
     args="k, a, b, n, s",
     macro="__builtin_ia32_vop2mi({OPC}, (k), (a), (b),"
           " ((n) + ((s) << 2)), {rt})",
     names=[("_mm_maskz_getmant_ss", "f", 4, 16),
            ("_mm_maskz_getmant_sd", "f", 8, 16)])
_add("op2i", "CVTRSD2SS", None, "f8", (16,),
     ["vcvtsd2ss {D}, {A}, {B} {rn-sae}"],
     feat="AVX512F", kind="CVT", oes=4, otot=16, flags=("OFL",),
     style="op2i", imm=(0, 15), evrnd=True, area="cvt",
     refblock="none", m=True, ssmask=True, atype=("f", 4),
     names=[("_mm_cvt_roundsd_ss", "f", 8, 16)])
# float-to-double conversion is exact, so the ss_sd rounding
# forms have no rounding to apply: they are the plain convert
_hdr([("_mm_cvt_roundss_sd", "f", 8, 16)], "a, b, r",
     "_mm_cvtss_sd((a), (b))",
     feat="AVX512F", area="cvt")
_hdr([("_mm_mask_cvt_roundss_sd", "f", 8, 16)], "src, k, a, b, r",
     "_mm_mask_cvtss_sd((src), (k), (a), (b))",
     feat="AVX512F", area="cvt")
_hdr([("_mm_maskz_cvt_roundss_sd", "f", 8, 16)], "k, a, b, r",
     "_mm_maskz_cvtss_sd((k), (a), (b))",
     feat="AVX512F", area="cvt")

# ------------------------------------------------------------------
# scalar compares and class tests (appended op1i / op2i
# operations), and the fixed-predicate spellings
# ------------------------------------------------------------------
_add("op2i", "CMPSS", None, "f4 f8", (16,),
     ["cmp{sf} {D}, {B}, {i}", "vcmp{sf} {D}, {A}, {B}, {i}"],
     feat="SSE", style="op2i", imm=(0, 31), area="cmp",
     refblock="cmpss", m=True, ssmask=True,
     names=[("_mm_cmp_ss", "f", 4, 16),
            ("_mm_cmp_sd", "f", 8, 16)])
_add("op2i", "CMPSSK", None, "f4 f8", (16,),
     ["vcmp{sf} {Kd}, {A}, {B}, {i}"],
     kind="MASK", style="op2i", imm=(0, 31), feat="AVX512F",
     area="cmp", refblock="mkscmp", m=True,
     names=[("_mm_cmp_ss_mask", "f", 4, 16),
            ("_mm_cmp_sd_mask", "f", 8, 16)])
_add("op1i", "FPCLASSS", None, "f4 f8", (16,),
     ["vfpclass{sf} {Kd}, {A}, {i}"],
     kind="MASK", style="op1i", imm=(0, 255), feat="AVX512DQ",
     area="cmp", refblock="fpclasss", m=True,
     names=[("_mm_fpclass_ss_mask", "f", 4, 16),
            ("_mm_fpclass_sd_mask", "f", 8, 16)])
_FPRED = (("eq", 0), ("lt", 1), ("le", 2),
          ("neq", 4), ("nlt", 5), ("nle", 6),
          ("ord", 7), ("unord", 3))
for _pn, _pi in _FPRED:
    _add("op2i", "CMPSS", None, "f4 f8", (16,),
         [], style="op2i", imm=(0, 31), imm_fixed=_pi,
         area="cmp",
         names=[("_mm_cmp%s_ss" % _pn, "f", 4, 16),
                ("_mm_cmp%s_sd" % _pn, "f", 8, 16)])
    for _sfx, _es in (("ps", 4), ("pd", 8)):
        _hdr([("_mm_cmp%s_%s" % (_pn, _sfx), "f", _es, 16)],
             "a, b", "_mm_cmp_%s((a), (b), %d)" % (_sfx, _pi),
             feat="SSE" if _es == 4 else "SSE2")
# the gt / ge / ngt / nge predicates need immediate bits above
# 7, which the legacy compare encoding drops: they ride
# VEX-only operations instead
_add("op2i", "CMPPV", None, "f48", (16,),
     ["vcmp{fs} {D}, {A}, {B}, {i}"],
     feat="AVX", style="op2i", imm=(0, 31), area="cmp",
     names=[])
_add("op2i", "CMPSSV", None, "f4 f8", (16,),
     ["vcmp{sf} {D}, {A}, {B}, {i}"],
     feat="AVX", style="op2i", imm=(0, 31), area="cmp",
     names=[])
for _pn, _pi in (("gt", 14), ("ge", 13), ("ngt", 10),
                 ("nge", 9)):
    _add("op2i", "CMPPV", None, "f48", (16,),
         [], style="op2i", imm=(0, 31), imm_fixed=_pi,
         area="cmp", refblock="cmpp",
         names=[("_mm_cmp%s_ps" % _pn, "f", 4, 16),
                ("_mm_cmp%s_pd" % _pn, "f", 8, 16)])
    _add("op2i", "CMPSSV", None, "f4 f8", (16,),
         [], style="op2i", imm=(0, 31), imm_fixed=_pi,
         area="cmp", refblock="cmpss",
         names=[("_mm_cmp%s_ss" % _pn, "f", 4, 16),
                ("_mm_cmp%s_sd" % _pn, "f", 8, 16)])


# ------------------------------------------------------------------
# the convert tail: unsigned and embedded-rounding scalar
# converts, the horizontal minimum, and the alias spellings
# ------------------------------------------------------------------
_add("op1", "MINPOS", None, "u2", (16,),
     ["phminposuw {D}, {A}", "vphminposuw {D}, {A}"],
     feat="SSE41", style="op1", area="arith", refblock="minpos",
     names=[("_mm_minpos_epu16", "u", 2, 16)])
# _mm_cvt{,tt}{ss,sd}_u32/u64 and the _round_ float-to-integer
# forms: _nm is the intrinsic stem between _mm_ and the result
# suffix, e.g. ("cvtss", 4) -> _mm_cvtss_u32
for _op, _insn, _tp, _nm in (
        ("CVTSS2USI", "vcvtss2usi", "f4", "cvtss"),
        ("CVTTSS2USI", "vcvttss2usi", "f4", "cvttss"),
        ("CVTSD2USI", "vcvtsd2usi", "f8", "cvtsd"),
        ("CVTTSD2USI", "vcvttsd2usi", "f8", "cvttsd")):
    for _oes in (4, 8):
        _add("op1", _op, None, _tp, (16,),
             ["%s {%s}, {A}" % (_insn, "Gd" if _oes == 4 else "G")],
             feat="AVX512F", kind="SCALAR", oes=_oes,
             style="op1", area="cvt", refblock="none",
             names=[("_mm_%s_u%d" % (_nm, _oes * 8),
                     "f", int(_tp[1]), 16)])
        _add("op1i", _op + "R", None, _tp, (16,),
             ["%s {%s}, {A} {sae}"
              % (_insn, "Gd" if _oes == 4 else "G")]
             if _insn.startswith("vcvtt") else
             ["%s {%s}, {A} {rn-sae}"
              % (_insn, "Gd" if _oes == 4 else "G")],
             feat="AVX512F", kind="SCALAR", oes=_oes,
             flags=("EVSAE",) if _insn.startswith("vcvtt") else (),
             style="op1i", imm=(0, 15),
             evrnd=not _insn.startswith("vcvtt"), area="cvt",
             refblock="none",
             names=[("_mm_%s_u%d"
                     % (_nm.replace("cvt", "cvt_round", 1)
                        if not _nm.startswith("cvtt") else
                        _nm.replace("cvtt", "cvtt_round", 1),
                        _oes * 8),
                     "f", int(_tp[1]), 16)])
for _op, _insn, _tp, _nm in (
        ("CVTRSS2SI", "vcvtss2si", "f4", "cvt_roundss"),
        ("CVTTRSS2SI", "vcvttss2si", "f4", "cvtt_roundss"),
        ("CVTRSD2SI", "vcvtsd2si", "f8", "cvt_roundsd"),
        ("CVTTRSD2SI", "vcvttsd2si", "f8", "cvtt_roundsd")):
    _tt = "tt" in _insn[1:4] or "tt" in _insn[3:5]
    _tt = _insn.startswith("vcvtt")
    for _oes in (4, 8):
        _add("op1i", _op, None, _tp, (16,),
             ["%s {%s}, {A} {sae}"
              % (_insn, "Gd" if _oes == 4 else "G")] if _tt else
             ["%s {%s}, {A} {rn-sae}"
              % (_insn, "Gd" if _oes == 4 else "G")],
             feat="AVX512F", kind="SCALAR", oes=_oes,
             flags=("EVSAE",) if _tt else (),
             style="op1i", imm=(0, 15),
             evrnd=not _tt, area="cvt",
             refblock="none",
             names=[("_mm_%s_i%d" % (_nm, _oes * 8),
                     "f", int(_tp[1]), 16),
                    ("_mm_%s_si%d" % (_nm, _oes * 8),
                     "f", int(_tp[1]), 16)])
for _op, _insn, _tp, _nm in (
        ("CVTSI2SSR", "vcvtsi2ss", "f4", "ss"),
        ("CVTSI2SDR", "vcvtsi2sd", "f8", "sd")):
    _add("op2i", _op, None, _tp, (16,),
         ["%s {D}, {A}, {%s} {rn-sae}"
          % (_insn, "Gd" if _tp == "f4" else "G")],
         feat="AVX512F", kind="BYTES",
         flags=("SCALAR2",), style="op2i", imm=(0, 15),
         evrnd=True, area="cvt", names=[(n, "f", int(_tp[1]), 16) for n in
                (["_mm_cvt_roundi32_%s" % _nm,
                  "_mm_cvt_roundsi32_%s" % _nm,
                  "_mm_cvt_roundi64_%s" % _nm,
                  "_mm_cvt_roundsi64_%s" % _nm]
                 if _tp == "f4" else
                 ["_mm_cvt_roundi64_%s" % _nm,
                  "_mm_cvt_roundsi64_%s" % _nm])])
for _op, _insn, _tp, _nm in (
        ("CVTUSI2SS", "vcvtusi2ss", "f4", "ss"),
        ("CVTUSI2SD", "vcvtusi2sd", "f8", "sd")):
    _add("op2", _op, None, _tp, (16,),
         ["%s {D}, {A}, {Gd}" % _insn],
         feat="AVX512F", kind="BYTES", form="D2_A1",
         flags=("SCALAR1",), style="op2", area="cvt",
         names=[])
    _add("op2", _op, None, _tp, (16,),
         ["%s {D}, {A}, {Gd}" % _insn],
         style="custom", feat="AVX512F",
         args="a, b",
         macro="__builtin_ia32_vop2({OPC}, (b), (a), {rt})",
         names=[("_mm_cvtu32_%s" % _nm, "f", int(_tp[1]), 16),
                ("_mm_cvtu64_%s" % _nm, "f", int(_tp[1]), 16)])
    _add("op2i", _op + "R", None, _tp, (16,),
         ["%s {D}, {A}, {%s} {rn-sae}"
          % (_insn, "Gd" if _tp == "f4" else "G")],
         feat="AVX512F", kind="BYTES",
         flags=("SCALAR2",), style="op2i", imm=(0, 15),
         evrnd=True, area="cvt", names=[(n, "f", int(_tp[1]), 16) for n in
                (["_mm_cvt_roundu32_%s" % _nm,
                  "_mm_cvt_roundu64_%s" % _nm]
                 if _tp == "f4" else
                 ["_mm_cvt_roundu64_%s" % _nm])])
_hdr([("_mm_cvt_ss2si", "s", 4, 16)], "a",
     "_mm_cvtss_si32((a))", feat="SSE")
_hdr([("_mm_cvtt_ss2si", "s", 4, 16)], "a",
     "_mm_cvttss_si32((a))", feat="SSE")
_hdr([("_mm_cvt_si2ss", "f", 4, 16)], "a, b",
     "_mm_cvtsi32_ss((a), (b))", feat="SSE")
_hdr([("_mm_cvtsi64_si128", "s", 8, 16)], "a",
     "_mm_set_epi64x(0, (a))", feat="SSE2")
_hdr([("_mm_cvtsi64x_si128", "s", 8, 16)], "a",
     "_mm_set_epi64x(0, (a))", feat="SSE2")
_hdr([("_mm_cvtsi128_si64x", "s", 8, 16)], "a",
     "_mm_extract_epi64((a), 0)", feat="SSE2")
for _es in (1, 2, 4):
    _hdr([("_mm_cmplt_epi%d" % (_es * 8), "s", _es, 16)],
         "a, b", "_mm_cmpgt_epi%d((b), (a))" % (_es * 8),
         feat="SSE2")
_hdr([("_mm_bslli_si128", "x", 8, 16)], "a, n",
     "_mm_slli_si128((a), (n))", feat="SSE2")
_hdr([("_mm_bsrli_si128", "x", 8, 16)], "a, n",
     "_mm_srli_si128((a), (n))", feat="SSE2")
for _nm, _sid, _k, _es, _tot, _rt in (
        ("_mm_cvtss_f32", 17, "f", 4, 16, "float"),
        ("_mm_cvtsd_f64", 21, "f", 8, 16, "double"),
        ("_mm256_cvtss_f32", 18, "f", 4, 32, "float"),
        ("_mm256_cvtsd_f64", 22, "f", 8, 32, "double"),
        ("_mm512_cvtss_f32", 19, "f", 4, 64, "float"),
        ("_mm512_cvtsd_f64", 23, "f", 8, 64, "double"),
        ("_mm256_cvtsi256_si32", 10, "s", 4, 32, "int"),
        ("_mm512_cvtsi512_si32", 11, "s", 4, 64, "int")):
    _hdr([(_nm, _k, _es, _tot)], "a",
         "__builtin_ia32_extract(%d, (a), 0, %s)" % (_sid, _rt),
         feat="AVX512F" if _tot == 64 else "AVX" if _tot == 32
         else "SSE")

# ------------------------------------------------------------------
# F16C embedded-rounding forms: vcvtph2ps with {rn-sae} (the
# conversion is exact; the selector only steers exceptions)
# and the vcvtps2ph rounding forms, whose immediate packs the
# mode in bits 1:0 and suppress-exceptions in bit 2
# ------------------------------------------------------------------
_hdr([("_mm512_cvt_roundph_ps", "s", 2, 32)], "a, r",
     "_mm512_cvtph_ps((a))", feat="AVX512F", area="cvt")
_hdr([("_mm512_mask_cvt_roundph_ps", "s", 2, 32)], "a, k, b, r",
     "_mm512_mask_cvtph_ps((a), (k), (b))",
     feat="AVX512F", area="cvt")
_hdr([("_mm512_maskz_cvt_roundph_ps", "s", 2, 32)], "k, b, r",
     "_mm512_maskz_cvtph_ps((k), (b))",
     feat="AVX512F", area="cvt")
_add("op1i", "CVTPS2PH", None, "f4", (64,),
     ["vcvtps2ph {D}, {A}, {i}"],
     style="custom", feat="AVX512F", imm=(0, 255),
     kind="CVT", oes=2, otot=32,
     args="a, r",
     macro="__builtin_ia32_vop1i({OPC}, (a),"
           " (r), __m256i)",
     names=[("_mm512_cvt_roundps_ph", "s", 4, 64)])
for _pre, _tot in (("_mm", 16), ("_mm256", 32), ("_mm512", 64)):
    _phrt = "__m256i" if _tot == 64 else "__m128i"
    _add("op1i", "CVTPS2PH", None, "f4", (_tot,),
         ["vcvtps2ph {D}, {A}, {i}"],
         style="custom", feat="AVX512F", imm=(0, 255),
         kind="CVT", oes=2, otot=16 if _tot < 64 else 32,
         args="src, k, a, r",
         macro="__builtin_ia32_vop1mi2({OPC}, (k), (src), (a),"
               " (r), " + _phrt + ")",
         names=[("%s_mask_cvt_roundps_ph" % _pre, "f", 4, _tot)])
    _add("op1i", "CVTPS2PH", None, "f4", (_tot,),
         ["vcvtps2ph {D}, {A}, {i}"],
         style="custom", feat="AVX512F", imm=(0, 255),
         kind="CVT", oes=2, otot=16 if _tot < 64 else 32,
         args="k, a, r",
         macro="__builtin_ia32_vop1mi({OPC}, (k), (a),"
               " (r), " + _phrt + ")",
         names=[("%s_maskz_cvt_roundps_ph" % _pre, "f", 4, _tot)])
