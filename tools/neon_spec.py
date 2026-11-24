# The NEON operation spec: one entry per (family, operation).
# Consumed by neon_gen.py, which derives encodings from the cross
# assembler, emits the backend opcode tables, the arm_neon.h
# header and the differential probes.
#
# Entry fields (see neon_gen.py for the machinery):
#   fam    'op1' | 'op2' | 'op3' | 'op1r'
#   op     operation name (becomes A64OP<fam>_<op>)
#   stem   intrinsic name stem, e.g. 'vadd' (None: no header macro,
#          backend-only operation used by gen_opv or compositions)
#   style  macro shape: 'op1' 'op1i' 'op2' 'op3' 'op3i' 'op1r'
#          'oper' (C operator) 'cmp' (C comparison cast to result)
#          'custom' (the entry carries its own emit function)
#   types  element kinds/sizes the op applies to, as a string of
#          groups: 's1248' = signed with element sizes 1,2,4,8
#   totals totals the op applies at: (8, 16), (8,) or (16,)
#   asm    assembler templates for integer kinds, tried in order;
#          {D} {N} {M} vector registers, {Dd} {Nd} {Md} d
#          registers, {arr} the first operand's arrangement,
#          {arrR} the result's, {arrB} the second operand's,
#          {i} the immediate
#   asmf   templates for float kinds (default: same as asm)
#   kind   shape relation (A64K_*), default 'SAME'
#   form   encoding form (A64F_*), default '2REG'/'3REG' by family
#   oes    result/other-operand element size when it differs
#   feat   required feature beyond NEON (default 0)
#   imm    (lo, hi) immediate range as functions of es or constants
#   naming 'std' (stem + q? + _elem), 'src' (stem + _sourceelem),
#          'cvt' (stem + _resultelem + _sourceelem), 'join'
#   ref    scalar reference expression for the probes, in terms of
#          a[i], b[i], c[i]
#   refblock  C statements computing the expected array e[] from
#          the operand arrays av[], bv[], cv[]
#   area   probe area (file) this op's cases go to

OPS = []

def _add(fam, op, stem, types, asm, **kw):
    d = dict(fam=fam, op=op, stem=stem, types=types, asm=asm)
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
INT_NO64 = "s124 u124"

# ------------------------------------------------------------------
# vop2: plain binary arithmetic and logic (integer kinds; the
# float counterparts are separate stemless ops used by gen_opv and
# the float header macros)
# ------------------------------------------------------------------
_add("op2", "ADD", "vadd", INT,
     ["add {D}.{arr}, {N}.{arr}, {M}.{arr}",
      "add {Dd}, {Nd}, {Md}"],
     style="oper", oper="+", area="arith", ref="a[i] + b[i]")
_add("op2", "ADDPOLY", "vadd", "p12",
     ["eor {D}.{arr8}, {N}.{arr8}, {M}.{arr8}"], kind="BYTES",
     area="arith", ref="a[i] ^ b[i]")
_add("op2", "SUB", "vsub", INT,
     ["sub {D}.{arr}, {N}.{arr}, {M}.{arr}",
      "sub {Dd}, {Nd}, {Md}"],
     style="oper", oper="-", area="arith", ref="a[i] - b[i]")
_add("op2", "MUL", "vmul", "s124 u124",
     ["mul {D}.{arr}, {N}.{arr}, {M}.{arr}"],
     style="oper", oper="*", area="arith", ref="a[i] * b[i]")
_add("op2", "FADD", "vadd", "f48",
     ["fadd {D}.{arr}, {N}.{arr}, {M}.{arr}",
      "fadd {Dd}, {Nd}, {Md}"],
     style="oper", oper="+", area="float", ref="a[i] + b[i]")
_add("op2", "FSUB", "vsub", "f48",
     ["fsub {D}.{arr}, {N}.{arr}, {M}.{arr}",
      "fsub {Dd}, {Nd}, {Md}"],
     style="oper", oper="-", area="float", ref="a[i] - b[i]")
_add("op2", "FMUL", "vmul", "f48",
     ["fmul {D}.{arr}, {N}.{arr}, {M}.{arr}"],
     style="oper", oper="*", area="float", ref="a[i] * b[i]")
_add("op2", "FDIV", "vdiv", "f48",
     ["fdiv {D}.{arr}, {N}.{arr}, {M}.{arr}"],
     style="oper", oper="/", area="float", ref="a[i] / b[i]")
_add("op2", "AND", "vand", INT + " p12",
     ["and {D}.{arr8}, {N}.{arr8}, {M}.{arr8}"],
     style="oper", oper="&", kind="BYTES", area="logic", ref="a[i] & b[i]")
_add("op2", "ORR", "vorr", INT + " p12",
     ["orr {D}.{arr8}, {N}.{arr8}, {M}.{arr8}"],
     style="oper", oper="|", kind="BYTES", area="logic", ref="a[i] | b[i]")
_add("op2", "EOR", "veor", INT + " p12",
     ["eor {D}.{arr8}, {N}.{arr8}, {M}.{arr8}"],
     style="oper", oper="^", kind="BYTES", area="logic", ref="a[i] ^ b[i]")
_add("op2", "BIC", "vbic", INT + " p12",
     ["bic {D}.{arr8}, {N}.{arr8}, {M}.{arr8}"],
     kind="BYTES", area="logic", ref="a[i] & ~b[i]")
_add("op2", "ORN", "vorn", INT + " p12",
     ["orn {D}.{arr8}, {N}.{arr8}, {M}.{arr8}"],
     kind="BYTES", area="logic", ref="a[i] | ~b[i]")
_add("op2", "FABD", "vabd", "f48",
     ["fabd {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="float",
     ref="fabs_ref(a[i] - b[i])")
_add("op2", "FRECPS", "vrecps", "f48",
     ["frecps {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="float")
_add("op2", "FRSQRTS", "vrsqrts", "f48",
     ["frsqrts {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="float")

# saturating and halving arithmetic
_add("op2", "SQADD", "vqadd", "s1248",
     ["sqadd {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="sat_add(a[i], b[i])")
_add("op2", "UQADD", "vqadd", "u1248",
     ["uqadd {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="sat_add(a[i], b[i])")
_add("op2", "SQSUB", "vqsub", "s1248",
     ["sqsub {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="sat_sub(a[i], b[i])")
_add("op2", "UQSUB", "vqsub", "u1248",
     ["uqsub {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="sat_sub(a[i], b[i])")
_add("op2", "SHADD", "vhadd", "s124",
     ["shadd {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="((a[i] + (long long)b[i]) >> 1)")
_add("op2", "UHADD", "vhadd", "u124",
     ["uhadd {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="((a[i] + (long long)b[i]) >> 1)")
_add("op2", "SRHADD", "vrhadd", "s124",
     ["srhadd {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="((a[i] + (long long)b[i] + 1) >> 1)")
_add("op2", "URHADD", "vrhadd", "u124",
     ["urhadd {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="((a[i] + (long long)b[i] + 1) >> 1)")
_add("op2", "SHSUB", "vhsub", "s124",
     ["shsub {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="((a[i] - (long long)b[i]) >> 1)")
_add("op2", "UHSUB", "vhsub", "u124",
     ["uhsub {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="((a[i] - (long long)b[i]) >> 1)")
_add("op2", "SABD", "vabd", "s124",
     ["sabd {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="wide_abs(a[i] - (long long)b[i])")
_add("op2", "UABD", "vabd", "u124",
     ["uabd {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="wide_abs(a[i] - (long long)b[i])")

# min / max, elementwise and pairwise
_add("op2", "SMAX", "vmax", "s124",
     ["smax {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="minmax",
     ref="a[i] > b[i] ? a[i] : b[i]")
_add("op2", "UMAX", "vmax", "u124",
     ["umax {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="minmax",
     ref="a[i] > b[i] ? a[i] : b[i]")
_add("op2", "SMIN", "vmin", "s124",
     ["smin {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="minmax",
     ref="a[i] < b[i] ? a[i] : b[i]")
_add("op2", "UMIN", "vmin", "u124",
     ["umin {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="minmax",
     ref="a[i] < b[i] ? a[i] : b[i]")
_add("op2", "FMAX", "vmax", "f48",
     ["fmax {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="float",
     ref="fmax_ref(a[i], b[i])")
_add("op2", "FMIN", "vmin", "f48",
     ["fmin {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="float",
     ref="fmin_ref(a[i], b[i])")
_add("op2", "FMAXNM", "vmaxnm", "f48",
     ["fmaxnm {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="float")
_add("op2", "FMINNM", "vminnm", "f48",
     ["fminnm {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="float")
_add("op2", "SMAXP", "vpmax", "s124",
     ["smaxp {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="minmax",
     refblock="pair_ref(1, 0)")
_add("op2", "UMAXP", "vpmax", "u124",
     ["umaxp {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="minmax",
     refblock="pair_ref(1, 0)")
_add("op2", "SMINP", "vpmin", "s124",
     ["sminp {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="minmax",
     refblock="pair_ref(0, 0)")
_add("op2", "UMINP", "vpmin", "u124",
     ["uminp {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="minmax",
     refblock="pair_ref(0, 0)")
_add("op2", "FMAXP", "vpmax", "f48",
     ["fmaxp {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="float",
     refblock="pair_ref(1, 1)")
_add("op2", "FMINP", "vpmin", "f48",
     ["fminp {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="float",
     refblock="pair_ref(0, 1)")
_add("op2", "ADDP", "vpadd", "s1248 u1248",
     ["addp {D}.{arr}, {N}.{arr}, {M}.{arr}"],
     asm64=["add {Dd}, {Nd}, {Md}"], area="minmax", bad=[(8, 8)],
     refblock="pair_add_ref()")
_add("op2", "FADDP", "vpadd", "f48",
     ["faddp {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="float",
     refblock="pair_add_ref()")

# widening multiplies (the result arrangement is {arrR})
_add("op2", "SMULL", "vmull", "s124",
     ["smull {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_LO",
     naming="src", area="mul", ref="(long long)a[i] * b[i]")
_add("op2", "UMULL", "vmull", "u124",
     ["umull {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_LO",
     naming="src", area="mul", ref="(long long)a[i] * b[i]")
_add("op2", "SMULL2", "vmull_high", "s124",
     ["smull2 {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_HI",
     naming="src", area="mul", ref="(long long)a[i] * b[i]")
_add("op2", "UMULL2", "vmull_high", "u124",
     ["umull2 {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_HI",
     naming="src", area="mul", ref="(long long)a[i] * b[i]")
_add("op2", "PMULL", "vmull", "p1",
     ["pmull {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_LO",
     naming="src", area="mul")
_add("op2", "PMULL2", "vmull_high", "p1",
     ["pmull2 {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_HI",
     naming="src", area="mul")
_add("op2", "PMUL", "vmul", "p1",
     ["pmul {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="mul",
     ref="pmul_ref(a[i], b[i])")
_add("op2", "SQDMULH", "vqdmulh", "s24",
     ["sqdmulh {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="mul")
_add("op2", "SQRDMULH", "vqrdmulh", "s24",
     ["sqrdmulh {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="mul")

# vector-count shifts: the count operand is an int8 vector of the
# same total size (kind SHIFT)
_add("op2", "SSHL", "vshl", "s1248",
     ["sshl {D}.{arr}, {N}.{arr}, {M}.{arr}"], kind="SHIFT",
     asm64=["sshl {Dd}, {Nd}, {Md}"],
     area="shift", refblock="shift_ref(1, 0)")
_add("op2", "USHL", "vshl", "u1248",
     ["ushl {D}.{arr}, {N}.{arr}, {M}.{arr}"], kind="SHIFT",
     asm64=["ushl {Dd}, {Nd}, {Md}"],
     area="shift", refblock="shift_ref(0, 0)")
_add("op2", "SRSHL", "vrshl", "s1248",
     ["srshl {D}.{arr}, {N}.{arr}, {M}.{arr}"], kind="SHIFT",
     asm64=["srshl {Dd}, {Nd}, {Md}"],
     area="shift", refblock="shift_ref(1, 1)")
_add("op2", "URSHL", "vrshl", "u1248",
     ["urshl {D}.{arr}, {N}.{arr}, {M}.{arr}"], kind="SHIFT",
     asm64=["urshl {Dd}, {Nd}, {Md}"],
     area="shift", refblock="shift_ref(0, 1)")
_add("op2", "SQSHL", "vqshl", "s1248",
     ["sqshl {D}.{arr}, {N}.{arr}, {M}.{arr}"], kind="SHIFT",
     asm64=["sqshl {Dd}, {Nd}, {Md}"],
     area="shift")
_add("op2", "UQSHL", "vqshl", "u1248",
     ["uqshl {D}.{arr}, {N}.{arr}, {M}.{arr}"], kind="SHIFT",
     asm64=["uqshl {Dd}, {Nd}, {Md}"],
     area="shift")

# comparisons as builtins: the header maps the vendor comparison
# names to C operators (gen_opv reaches the same instructions);
# the register forms exist for vtst and for gen_opv itself.
_add("op2", "CMEQ", None, INT,
     ["cmeq {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="cmp")
_add("op2", "CMGT", None, "s1248",
     ["cmgt {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="cmp")
_add("op2", "CMGE", None, "s1248",
     ["cmge {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="cmp")
_add("op2", "CMHI", None, "u1248",
     ["cmhi {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="cmp")
_add("op2", "CMHS", None, "u1248",
     ["cmhs {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="cmp")
_add("op2", "CMTST", "vtst", INT + " p12",
     ["cmtst {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="cmp",
     ressign="u", ref="(a[i] & b[i]) != 0 ? -1 : 0")
_add("op2", "FCMEQ", None, "f48",
     ["fcmeq {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="cmp")
_add("op2", "FCMGT", None, "f48",
     ["fcmgt {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="cmp")
_add("op2", "FCMGE", None, "f48",
     ["fcmge {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="cmp")

# table lookup (single table register; the multi-register forms
# would need consecutive register groups the generics cannot
# promise for their operands)
_add("op2", "TBL", "vtbl1", "s1 u1 p1",
     ["tbl {D}.{arr}, {{N}.16b}, {M}.{arr}"], kind="BYTES",
     totals=(8,), area="permute", refblock="tbl_ref(0)")
_add("op2", "TBLQ", "vqtbl1", "s1 u1 p1",
     ["tbl {D}.{arr}, {{N}.16b}, {M}.{arr}"], kind="TBLQ",
     totals=(8, 16), naming="srcbq", area="permute",
     refblock="tbl_ref(0)")

# two halves joined (vcombine): hand-formed in the backend
_add("op2", "JOIN", "vcombine", "s1248 u1248 p128 f48",
     [], kind="JOIN", totals=(8,), form="JOIN", naming="join",
     area="lane")

# float convert into the high half (vcvt_high_f32_f64)
_add("op2", "FCVTN2", "vcvt_high", "f4",
     ["fcvtn2 {D}.{arrR}, {N}.{arrB}"], kind="NARROW_HI",
     form="2REG_ACC", naming="cvt", area="cvt")

# ------------------------------------------------------------------
# vop3: accumulating three-operand forms (result overwrites a)
# ------------------------------------------------------------------
_add("op3", "MLA", "vmla", "s124 u124",
     ["mla {D}.{arr}, {N}.{arr}, {M}.{arr}"],
     area="mul", ref="a[i] + b[i] * c[i]")
_add("op3", "MLS", "vmls", "s124 u124",
     ["mls {D}.{arr}, {N}.{arr}, {M}.{arr}"],
     area="mul", ref="a[i] - b[i] * c[i]")
_add("op3", "FMLA", "vmla", "f48",
     ["fmla {D}.{arr}, {N}.{arr}, {M}.{arr}"],
     extra_stems=["vfma"], area="mul", ref="a[i] + b[i] * c[i]",
     asm64=["fmadd {Dd}, {Nd}, {Md}, {Dd}"], form64="3REG_FMA64")
_add("op3", "FMLS", "vmls", "f48",
     ["fmls {D}.{arr}, {N}.{arr}, {M}.{arr}"],
     extra_stems=["vfms"], area="mul", ref="a[i] - b[i] * c[i]",
     asm64=["fmsub {Dd}, {Nd}, {Md}, {Dd}"], form64="3REG_FMA64")
_add("op3", "SABA", "vaba", "s124",
     ["saba {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="a[i] + wide_abs(b[i] - (long long)c[i])")
_add("op3", "UABA", "vaba", "u124",
     ["uaba {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="arith",
     ref="a[i] + wide_abs(b[i] - (long long)c[i])")
_add("op3", "TBX", "vtbx1", "s1 u1 p1",
     ["tbx {D}.{arr}, {{N}.16b}, {M}.{arr}"], kind="BYTES",
     totals=(8,), area="permute", refblock="tbl_ref(1)")
_add("op3", "TBXQ", "vqtbx1", "s1 u1 p1",
     ["tbx {D}.{arr}, {{N}.16b}, {M}.{arr}"], kind="TBLQ",
     totals=(8, 16), naming="srcbq", area="permute",
     refblock="tbl_ref(1)")
_add("op3", "BSL", "vbsl", INT + " p12 f48",
     ["bsl {D}.{arr8}, {N}.{arr8}, {M}.{arr8}"], kind="BYTES",
     area="logic", ref="(a[i] & b[i]) | (~a[i] & c[i])")
_add("op3", "BIT", "vbit", INT + " p12",
     ["bit {D}.{arr8}, {N}.{arr8}, {M}.{arr8}"], kind="BYTES",
     area="logic", ref="(a[i] & ~c[i]) | (b[i] & c[i])")
_add("op3", "BIF", "vbif", INT + " p12",
     ["bif {D}.{arr8}, {N}.{arr8}, {M}.{arr8}"], kind="BYTES",
     area="logic", ref="(a[i] & c[i]) | (b[i] & ~c[i])")
# widening multiply-accumulate: b and c are half-width (oes)
_add("op3", "SMLAL", "vmlal", "s1248",
     ["smlal {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_LO",
     naming="srcb", area="mul", ref="a[i] + (long long)b[i] * c[i]")
_add("op3", "UMLAL", "vmlal", "u1248",
     ["umlal {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_LO",
     naming="srcb", area="mul", ref="a[i] + (long long)b[i] * c[i]")
_add("op3", "SMLAL2", "vmlal_high", "s1248",
     ["smlal2 {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_HI",
     naming="srcb", area="mul", ref="a[i] + (long long)b[i] * c[i]")
_add("op3", "UMLAL2", "vmlal_high", "u1248",
     ["umlal2 {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_HI",
     naming="srcb", area="mul", ref="a[i] + (long long)b[i] * c[i]")
_add("op3", "SMLSL", "vmlsl", "s1248",
     ["smlsl {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_LO",
     naming="srcb", area="mul", ref="a[i] - (long long)b[i] * c[i]")
_add("op3", "UMLSL", "vmlsl", "u1248",
     ["umlsl {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_LO",
     naming="srcb", area="mul", ref="a[i] - (long long)b[i] * c[i]")
_add("op3", "SMLSL2", "vmlsl_high", "s1248",
     ["smlsl2 {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_HI",
     naming="srcb", area="mul", ref="a[i] - (long long)b[i] * c[i]")
_add("op3", "UMLSL2", "vmlsl_high", "u1248",
     ["umlsl2 {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_HI",
     naming="srcb", area="mul", ref="a[i] - (long long)b[i] * c[i]")
# dot products (armv8.2-a dot product extension)
_add("op3", "SDOT", "vdot", "s4",
     ["sdot {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="DOT",
     oes=1, feat="DOTPROD", area="dotprod",
     refblock="dot_ref(1)")
_add("op3", "UDOT", "vdot", "u4",
     ["udot {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="DOT",
     oes=1, feat="DOTPROD", area="dotprod",
     refblock="dot_ref(0)")

# ------------------------------------------------------------------
# vop1: unary operations
# ------------------------------------------------------------------
_add("op1", "ABS", "vabs", "s1248",
     ["abs {D}.{arr}, {N}.{arr}"],
     area="arith", ref="abs_ref(a[i])")
_add("op1", "FABS", "vabs", "f48",
     ["fabs {D}.{arr}, {N}.{arr}"],
     area="float", ref="fabs_ref(a[i])")
_add("op1", "SQABS", "vqabs", "s1248",
     ["sqabs {D}.{arr}, {N}.{arr}"], area="arith",
     ref="sat_abs(a[i])")
_add("op1", "NEG", "vneg", "s1248",
     ["neg {D}.{arr}, {N}.{arr}"],
     style="custom", area="arith", ref="-(a[i])")
_add("op1", "FNEG", "vneg", "f48",
     ["fneg {D}.{arr}, {N}.{arr}"],
     style="custom", area="float", ref="-(a[i])")
_add("op1", "SQNEG", "vqneg", "s1248",
     ["sqneg {D}.{arr}, {N}.{arr}"], area="arith",
     ref="sat_neg(a[i])")
_add("op1", "NOT", "vmvn", "s124 u124 p1",
     ["not {D}.{arr8}, {N}.{arr8}"], kind="BYTES", area="logic",
     ref="~(a[i])")
_add("op1", "CLS", "vcls", "s124 u124",
     ["cls {D}.{arr}, {N}.{arr}"], area="logic",
     ressign="s", ref="cls_ref(a[i])")
_add("op1", "CLZ", "vclz", INT_NO64,
     ["clz {D}.{arr}, {N}.{arr}"], area="logic")
_add("op1", "CNT", "vcnt", "s1 u1 p1",
     ["cnt {D}.{arr}, {N}.{arr}"], area="logic",
     ref="popcount_ref(a[i])")
_add("op1", "RBIT", "vrbit", "s1 u1 p1",
     ["rbit {D}.{arr}, {N}.{arr}"], area="logic")
_add("op1", "REV64", "vrev64", "s124 u124 p12 f4",
     ["rev64 {D}.{arr}, {N}.{arr}"], area="permute",
     refblock="rev_ref(8)")
_add("op1", "REV32", "vrev32", "s12 u12 p12",
     ["rev32 {D}.{arr}, {N}.{arr}"], area="permute",
     refblock="rev_ref(4)")
_add("op1", "REV16", "vrev16", "s1 u1 p1",
     ["rev16 {D}.{arr}, {N}.{arr}"], area="permute",
     refblock="rev_ref(2)")
# compare against zero
_add("op1", "CMEQZ", "vceqz", "s1248 u1248 p1",
     ["cmeq {D}.{arr}, {N}.{arr}, #0"], area="cmp",
     ressign="u", ref="a[i] == 0 ? -1 : 0")
_add("op1", "CMGTZ", "vcgtz", "s1248",
     ["cmgt {D}.{arr}, {N}.{arr}, #0"], area="cmp",
     ressign="u", ref="a[i] > 0 ? -1 : 0")
_add("op1", "CMGEZ", "vcgez", "s1248",
     ["cmge {D}.{arr}, {N}.{arr}, #0"], area="cmp",
     ressign="u", ref="a[i] >= 0 ? -1 : 0")
_add("op1", "CMLEZ", "vclez", "s1248",
     ["cmle {D}.{arr}, {N}.{arr}, #0"], area="cmp",
     ressign="u", ref="a[i] <= 0 ? -1 : 0")
_add("op1", "CMLTZ", "vcltz", "s1248",
     ["cmlt {D}.{arr}, {N}.{arr}, #0"], area="cmp",
     ressign="u", ref="a[i] < 0 ? -1 : 0")
_add("op1", "FCMEQZ", "vceqz", "f48",
     ["fcmeq {D}.{arr}, {N}.{arr}, #0"], area="cmp",
     ressign="u", kind="CVTN", ref="a[i] == 0 ? -1 : 0")
_add("op1", "FCMGTZ", "vcgtz", "f48",
     ["fcmgt {D}.{arr}, {N}.{arr}, #0"], area="cmp",
     ressign="u", kind="CVTN", ref="a[i] > 0 ? -1 : 0")
_add("op1", "FCMGEZ", "vcgez", "f48",
     ["fcmge {D}.{arr}, {N}.{arr}, #0"], area="cmp",
     ressign="u", kind="CVTN", ref="a[i] >= 0 ? -1 : 0")
_add("op1", "FCMLEZ", "vclez", "f48",
     ["fcmle {D}.{arr}, {N}.{arr}, #0"], area="cmp",
     ressign="u", kind="CVTN", ref="a[i] <= 0 ? -1 : 0")
_add("op1", "FCMLTZ", "vcltz", "f48",
     ["fcmlt {D}.{arr}, {N}.{arr}, #0"], area="cmp",
     ressign="u", kind="CVTN", ref="a[i] < 0 ? -1 : 0")
# float unary
_add("op1", "FSQRT", "vsqrt", "f48",
     ["fsqrt {D}.{arr}, {N}.{arr}",
      "fsqrt {Dd}, {Nd}"], area="float")
_add("op1", "FRECPE", "vrecpe", "f48",
     ["frecpe {D}.{arr}, {N}.{arr}"], area="float")
_add("op1", "FRSQRTE", "vrsqrte", "f48",
     ["frsqrte {D}.{arr}, {N}.{arr}"], area="float")
for _r, _m in [("FRINTN", "vrndn"), ("FRINTP", "vrndp"),
               ("FRINTM", "vrndm"), ("FRINTZ", "vrnd"),
               ("FRINTA", "vrnda"), ("FRINTX", "vrndx"),
               ("FRINTI", "vrndi")]:
    _add("op1", _r, _m, "f48",
         ["%s {D}.{arr}, {N}.{arr}" % _r.lower()], area="float")
# halves out of a quad vector (vget_low / vget_high)
_add("op1", "MOVLO", "vget_low", "s1248 u1248 p128 f48",
     ["fmov {Dd}, {Nd}"], kind="HALF_LO", totals=(16,),
     naming="src", area="lane")
_add("op1", "MOVHI", "vget_high", "s1248 u1248 p128 f48",
     ["mov {D}.d[0], {N}.d[1]"], kind="HALF_HI", totals=(16,),
     naming="src", area="lane")

# ------------------------------------------------------------------
# conversions (op1, kind CVTN: same lane count, result element
# size/flavour from oes/ofl; names are vcvt_<result>_<source>)
# ------------------------------------------------------------------
def _cvt(op, stem, srctypes, dst, asm, **kw):
    kw.setdefault("kind", "CVTN")
    kw.setdefault("naming", "cvt")
    kw.setdefault("area", "cvt")
    kw["oes"], kw["ofl"] = dst
    _add("op1", op, stem, srctypes, asm, **kw)

# note: ACLE has no size-changing int<->float conversions other
# than the f32<->f64 pair below (no vcvt_f64_s32 / vcvt_s32_f64);
# the 64-bit-element forms at total 8 are scalar instructions
# reached through the CVT64 forms (a GP register in between)
for _sgn, _pre in [("S", "s"), ("U", "u")]:
    _cvt(_sgn + "CVTF", "vcvt", _pre + "48", (0, 1),
         ["%ccvtf {D}.{arr}, {N}.{arr}" % _pre],
         asm64=["%ccvtf {Dd}, {Xn}" % _pre], form64="CVT64I")
    for _rm, _mn in [("Z", "z"), ("N", "n"), ("P", "p"),
                     ("M", "m"), ("A", "a")]:
        _cvt("FCVT" + _rm + _sgn,
             "vcvt" if _rm == "Z" else "vcvt" + _mn.lower(),
             "f48", (0, 0),
             ["fcvt%c%c {D}.{arr}, {N}.{arr}" % (_mn, _sgn.lower())],
             restype_sign=_pre,
             asm64=["fcvt%c%c {Xd}, {Nd}" % (_mn, _sgn.lower())],
             form64="CVT64F")
# float to float width changes
_cvt("FCVTN", "vcvt", "f8", (4, 1),
     ["fcvtn {D}.{arrR}, {N}.{arr}"], kind="NARROW", totals=(16,))
_cvt("FCVTL", "vcvt", "f4", (8, 1),
     ["fcvtl {D}.{arrR}, {N}.{arr}"], kind="WIDEN_LO", totals=(8,))
# fixed-point conversions: the immediate is the number of
# fraction bits, encoded in the opc like a shift count
for _sgn, _pre in [("S", "s"), ("U", "u")]:
    _add("op1", _sgn + "CVTFI", "vcvt_n", _pre + "48",
         ["%ccvtf {D}.{arr}, {N}.{arr}, #{i}" % _pre],
         kind="CVTN", oes=0, ofl=1, naming="cvt", style="op1i",
         imm=(1, "es8"), area="cvt",
         asm64=["%ccvtf {Dd}, {Xn}, #{i}" % _pre], form64="CVT64I")
    _add("op1", "FCVTZI" + _sgn, "vcvt_n", "f48",
         ["fcvtzs {D}.{arr}, {N}.{arr}, #{i}".replace("zs", "z" + _sgn.lower())],
         kind="CVTN", oes=0, ofl=0, naming="cvt", style="op1i",
         imm=(1, "es8"), area="cvt", restype_sign=_pre,
         asm64=["fcvtzs {Xd}, {Nd}, #{i}".replace("zs", "z" + _sgn.lower())],
         form64="CVT64F")

# ------------------------------------------------------------------
# widening and narrowing unary ops
# ------------------------------------------------------------------
_add("op1", "SXTL", "vmovl", "s124",
     ["sshll {D}.{arrR}, {N}.{arr}, #0"], kind="WIDEN_LO",
     totals=(8,), naming="src", area="widen",
     ref="(long long)a[i]")
_add("op1", "UXTL", "vmovl", "u124",
     ["ushll {D}.{arrR}, {N}.{arr}, #0"], kind="WIDEN_LO",
     totals=(8,), naming="src", area="widen",
     ref="(long long)a[i]")
_add("op1", "SXTL2", "vmovl_high", "s124",
     ["sshll2 {D}.{arrR}, {N}.{arr}, #0"], kind="WIDEN_HI",
     totals=(16,), naming="src", area="widen",
     ref="(long long)a[i]")
_add("op1", "UXTL2", "vmovl_high", "u124",
     ["ushll2 {D}.{arrR}, {N}.{arr}, #0"], kind="WIDEN_HI",
     totals=(16,), naming="src", area="widen",
     ref="(long long)a[i]")
_add("op1", "SADDLP", "vpaddl", "s124",
     ["saddlp {D}.{arrR}, {N}.{arr}"], kind="PAIRW",
     naming="srcbq", area="widen",
     refblock="pair_widen_ref()")
_add("op1", "UADDLP", "vpaddl", "u124",
     ["uaddlp {D}.{arrR}, {N}.{arr}"], kind="PAIRW",
     naming="srcbq", area="widen",
     refblock="pair_widen_ref()")
_add("op2", "SADALP", "vpadal", "s248",
     ["sadalp {D}.{arr}, {N}.{arrB}"], kind="ACCW1",
     form="2REG_ACC", naming="srcbq", area="widen")
_add("op2", "UADALP", "vpadal", "u248",
     ["uadalp {D}.{arr}, {N}.{arrB}"], kind="ACCW1",
     form="2REG_ACC", naming="srcbq", area="widen")
_add("op1", "XTN", "vmovn", "s248 u248",
     ["xtn {D}.{arrR}, {N}.{arr}"], kind="NARROW",
     totals=(16,), naming="src", area="narrow",
     ref="(RES_ELEM)a[i]")
_add("op1", "SQXTN", "vqmovn", "s248",
     ["sqxtn {D}.{arrR}, {N}.{arr}"], kind="NARROW",
     totals=(16,), naming="src", area="narrow")
_add("op1", "UQXTN", "vqmovn", "u248",
     ["uqxtn {D}.{arrR}, {N}.{arr}"], kind="NARROW",
     totals=(16,), naming="src", area="narrow")
_add("op1", "SQXTUN", "vqmovun", "s248",
     ["sqxtun {D}.{arrR}, {N}.{arr}"], kind="NARROW",
     totals=(16,), naming="src", area="narrow", ressign="u")
# the _high narrowing forms take the narrow low half as operand a
# (form 2REG_ACC: the instruction writes into a's register)
for _op, _stem, _tp, _bk in [
        ("XTN2", "vmovn_high", "s1248", None),
        ("SQXTN2", "vqmovn_high", "s1248", None),
        ("UQXTN2", "vqmovn_high", "u1248", None),
        ("SQXTUN2", "vqmovun_high", "u1248", "s")]:
    _kw = dict(kind="NARROW_HI", form="2REG_ACC", totals=(8,),
               naming="srcb", area="narrow")
    if _bk:
        _kw["bkind"] = _bk
    _add("op2", _op, _stem, _tp,
         ["%s {D}.{arrR}, {N}.{arrB}" % _op.lower().replace("2", "2")],
         **_kw)
_add("op2", "UXTN2", "vmovn_high", "u1248",
     ["xtn2 {D}.{arrR}, {N}.{arrB}"], kind="NARROW_HI",
     form="2REG_ACC", totals=(8,), naming="srcb", area="narrow")

# ------------------------------------------------------------------
# immediate shifts (op1/op1-accumulating forms; the immediate is
# carried in the opc).  Right shifts take 1..8*es, left shifts and
# fixed-point converts 0..8*es-1 (resp. 0..8*es for converts).
# ------------------------------------------------------------------
def _shift(op, stem, types, asm, **kw):
    kw.setdefault("area", "shiftimm")
    _add("op1", op, stem, types, asm, **kw)

def _shift2(op, stem, types, asm, **kw):
    kw.setdefault("area", "shiftimm")
    kw.setdefault("style", "op2i2")
    _add("op2", op, stem, types, asm, **kw)

_shift("SSHR", "vshr_n", "s1248",
       ["sshr {D}.{arr}, {N}.{arr}, #{i}"], style="op1i",
       imm=(1, "es8"), refblock="shiftimm_ref(1, 0, 0)")
_shift("USHR", "vshr_n", "u1248",
       ["ushr {D}.{arr}, {N}.{arr}, #{i}"], style="op1i",
       imm=(1, "es8"), refblock="shiftimm_ref(0, 0, 0)")
_shift("SRSHR", "vrshr_n", "s1248",
       ["srshr {D}.{arr}, {N}.{arr}, #{i}"], style="op1i",
       imm=(1, "es8"), refblock="shiftimm_ref(1, 0, 1)")
_shift("URSHR", "vrshr_n", "u1248",
       ["urshr {D}.{arr}, {N}.{arr}, #{i}"], style="op1i",
       imm=(1, "es8"), refblock="shiftimm_ref(0, 0, 1)")
_shift2("SSRA", "vsra_n", "s1248",
       ["ssra {D}.{arr}, {N}.{arr}, #{i}"],
       asm64=["ssra {Dd}, {Nd}, #{i}"], style="op2i2",
       form="2REG_IMM_ACC", imm=(1, "es8"))
_shift2("USRA", "vsra_n", "u1248",
       ["usra {D}.{arr}, {N}.{arr}, #{i}"],
       asm64=["usra {Dd}, {Nd}, #{i}"], style="op2i2",
       form="2REG_IMM_ACC", imm=(1, "es8"))
_shift2("SRSRA", "vrsra_n", "s1248",
       ["srsra {D}.{arr}, {N}.{arr}, #{i}"],
       asm64=["srsra {Dd}, {Nd}, #{i}"], style="op2i2",
       form="2REG_IMM_ACC", imm=(1, "es8"))
_shift2("URSRA", "vrsra_n", "u1248",
       ["ursra {D}.{arr}, {N}.{arr}, #{i}"],
       asm64=["ursra {Dd}, {Nd}, #{i}"], style="op2i2",
       form="2REG_IMM_ACC", imm=(1, "es8"))
_shift("SHL", "vshl_n", "s1248 u1248",
       ["shl {D}.{arr}, {N}.{arr}, #{i}"], style="op1i",
       imm=(0, "es8m1"), refblock="shiftimm_ref(1, 1, 0)")
_shift("SQSHL_N", "vqshl_n", "s1248",
       ["sqshl {D}.{arr}, {N}.{arr}, #{i}"], style="op1i",
       imm=(0, "es8m1"))
_shift("UQSHL_N", "vqshl_n", "u1248",
       ["uqshl {D}.{arr}, {N}.{arr}, #{i}"], style="op1i",
       imm=(0, "es8m1"))
_shift("SQSHLU_N", "vqshlu_n", "s1248",
       ["sqshlu {D}.{arr}, {N}.{arr}, #{i}"], style="op1i",
       imm=(0, "es8m1"), ressign="u")
_shift("SSHLL_N", "vshll_n", "s124",
       ["sshll {D}.{arrR}, {N}.{arr}, #{i}"], style="op1i",
       kind="WIDEN_LO", totals=(8,), naming="src",
       imm=(0, "es8m1"), area="widen")
_shift("USHLL_N2", "vshll_n", "u124",
       ["ushll {D}.{arrR}, {N}.{arr}, #{i}"], style="op1i",
       kind="WIDEN_LO", totals=(8,), naming="src",
       imm=(0, "es8m1"), area="widen")
_shift("SSHLL_N2", "vshll_high_n", "s124",
       ["sshll2 {D}.{arrR}, {N}.{arr}, #{i}"], style="op1i",
       kind="WIDEN_HI", totals=(16,), naming="src",
       imm=(0, "es8m1"), area="widen")
_shift("USHLL_N2H", "vshll_high_n", "u124",
       ["ushll2 {D}.{arrR}, {N}.{arr}, #{i}"], style="op1i",
       kind="WIDEN_HI", totals=(16,), naming="src",
       imm=(0, "es8m1"), area="widen")
# narrowing immediate shifts: the entry shape is the wide source
for _op, _stem, _tp, _mn in [
        ("SHRN", "vshrn_n", "s248", "shrn"),
        ("RSHRN", "vrshrn_n", "s248", "rshrn"),
        ("SQSHRN", "vqshrn_n", "s248", "sqshrn"),
        ("UQSHRN", "vqshrn_n", "u248", "uqshrn"),
        ("SQSHRUN", "vqshrun_n", "s248", "sqshrun"),
        ("SQRSHRN", "vqrshrn_n", "s248", "sqrshrn"),
        ("UQRSHRN", "vqrshrn_n", "u248", "uqrshrn"),
        ("SQRSHRUN", "vqrshrun_n", "s248", "sqrshrun"),
        ("USHRN", "vshrn_n", "u248", "shrn"),
        ("URSHRN", "vrshrn_n", "u248", "rshrn")]:
    _kw = dict(style="op1i", kind="NARROW", totals=(16,),
               naming="src", imm=(1, "es4"), area="narrow")
    if _op in ("SQSHRUN", "SQRSHRUN"):
        _kw["ressign"] = "u"
    _shift(_op, _stem, _tp,
           ["%s {D}.{arrR}, {N}.{arr}, #{i}" % _mn], **_kw)

# ------------------------------------------------------------------
# vop1r: horizontal reductions to a scalar
# ------------------------------------------------------------------
for _op, _stem, _tp, _mn in [
        ("ADDV", "vaddv", "s124 u124", "addv"),
        ("SMAXV", "vmaxv", "s124", "smaxv"),
        ("UMAXV", "vmaxv", "u124", "umaxv"),
        ("SMINV", "vminv", "s124", "sminv"),
        ("UMINV", "vminv", "u124", "uminv")]:
    _add("op1r", _op, _stem, _tp,
         ["%s {Ed}, {N}.{arr}" % _mn], kind="SCALAR",
         bad=[(4, 8)], area="reduce")
# float reductions: the full-width forms are direct, the 2-lane
# forms are the scalar pairwise instructions
_add("op1r", "FMAXV", "vmaxv", "f4",
     ["fmaxv {Ed}, {N}.{arr}"], kind="SCALAR", totals=(16,),
     area="reduce")
_add("op1r", "FMINV", "vminv", "f4",
     ["fminv {Ed}, {N}.{arr}"], kind="SCALAR", totals=(16,),
     area="reduce")
for _op, _stem, _mn in [("FADDVD", "vaddv", "faddp"),
                        ("FMAXVD", "vmaxv", "fmaxp"),
                        ("FMINVD", "vminv", "fminp")]:
    _add("op1r", _op, _stem, "f4",
         ["%s {Ed}, {N}.{arr}" % _mn], kind="SCALAR", totals=(8,),
         area="reduce")
# pairwise-single (vpaddd): one 16-byte operand, a scalar result
_add("op1r", "ADDPD", "vpaddd", "s8 u8",
     ["addp {Dd}, {N}.{arr}"], kind="SCALAR", totals=(16,),
     naming="src", area="reduce")
_add("op1r", "FADDPD", "vpaddd", "f8",
     ["faddp {Dd}, {N}.{arr}"], kind="SCALAR", totals=(16,),
     naming="src", area="reduce")

# narrowing two-operand ops (vaddhn and friends) and their
# _high forms (op3, accumulating into the high half)
for _op, _stem, _tp, _mn in [
        ("ADDHN", "vaddhn", "s248 u248", "addhn"),
        ("RADDHN", "vraddhn", "s248 u248", "raddhn"),
        ("SUBHN", "vsubhn", "s248 u248", "subhn"),
        ("RSUBHN", "vrsubhn", "s248 u248", "rsubhn")]:
    _add("op2", _op, _stem, _tp,
         ["%s {D}.{arrR}, {N}.{arr}, {M}.{arr}" % _mn],
         kind="NARROW2", totals=(16,), naming="src", area="narrow")
    _add("op3", _op + "2", _stem + "_high",
         _tp.replace("s248", "s124").replace("u248", "u124"),
         ["%s {D}.{arrR}, {N}.{arrB}, {M}.{arrB}" % (_mn + "2")],
         kind="NARROW2_HI", totals=(8,), naming="srcb", area="narrow")
# widening absolute difference and accumulate-difference
_add("op2", "SABDL", "vabdl", "s124",
     ["sabdl {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_LO",
     naming="src", area="mul", ref="wide_abs(a[i] - (long long)b[i])")
_add("op2", "UABDL", "vabdl", "u124",
     ["uabdl {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_LO",
     naming="src", area="mul", ref="wide_abs(a[i] - (long long)b[i])")
_add("op2", "SABDL2", "vabdl_high", "s124",
     ["sabdl2 {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_HI",
     naming="src", area="mul", ref="wide_abs(a[i] - (long long)b[i])")
_add("op2", "UABDL2", "vabdl_high", "u124",
     ["uabdl2 {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_HI",
     naming="src", area="mul", ref="wide_abs(a[i] - (long long)b[i])")
_add("op3", "SABAL", "vabal", "s1248",
     ["sabal {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_LO",
     naming="srcb", area="mul")
_add("op3", "UABAL", "vabal", "u1248",
     ["uabal {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_LO",
     naming="srcb", area="mul")
_add("op3", "SABAL2", "vabal_high", "s1248",
     ["sabal2 {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_HI",
     naming="srcb", area="mul")
_add("op3", "UABAL2", "vabal_high", "u1248",
     ["uabal2 {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_HI",
     naming="srcb", area="mul")
_add("op2", "SQDMULL", "vqdmull", "s24",
     ["sqdmull {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_LO",
     naming="src", area="mul")
_add("op2", "SQDMULL2", "vqdmull_high", "s24",
     ["sqdmull2 {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_HI",
     naming="src", area="mul")
_add("op1", "URECPE", "vrecpe", "u4",
     ["urecpe {D}.{arr}, {N}.{arr}"], area="float")
_add("op1", "URSQRTE", "vrsqrte", "u4",
     ["ursqrte {D}.{arr}, {N}.{arr}"], area="float")
_add("op2", "FMAXNMP", "vpmaxnm", "f48",
     ["fmaxnmp {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="float",
     refblock="pair_ref(1, 1)")
_add("op2", "FMINNMP", "vpminnm", "f48",
     ["fminnmp {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="float",
     refblock="pair_ref(0, 1)")

# insert/extract-bitfield immediates are op3 with an immediate
_add("op3", "SRI", "vsri_n", "s1248 u1248 p12",
     ["sri {D}.{arr}, {N}.{arr}, #{i}"],
     asm64=["sri {Dd}, {Nd}, #{i}"], style="op3i",
     form="3REG_IMM", imm=(1, "es8"), area="shiftimm")
_add("op3", "SLI", "vsli_n", "s1248 u1248 p12",
     ["sli {D}.{arr}, {N}.{arr}, #{i}"],
     asm64=["sli {Dd}, {Nd}, #{i}"], style="op3i",
     form="3REG_IMM", imm=(0, "es8m1"), area="shiftimm")

# ------------------------------------------------------------------
# lane-indexed forms: one operand is a single lane of a vector.
# op2 indexed entries index operand b; op3 indexed entries index
# operand c.  The header emits both the _lane (64-bit lane
# source) and _laneq (128-bit lane source) spellings.
# ------------------------------------------------------------------
_add("op2", "MUL_L", "vmul_lane", "s124 u124",
     ["mul {D}.{arr}, {N}.{arr}, {M}.{arrB}[{i}]"], form="3REG_IDX",
     area="lane", ref="a[i] * bv")
_add("op2", "FMUL_L", "vmul_lane", "f48",
     ["fmul {D}.{arr}, {N}.{arr}, {M}.{arrB}[{i}]"], form="3REG_IDX",
     area="lane", ref="a[i] * bv")
_add("op2", "FMULX_L", "vmulx_lane", "f48",
     ["fmulx {D}.{arr}, {N}.{arr}, {M}.{arrB}[{i}]"], form="3REG_IDX",
     area="lane")
_add("op2", "SQDMULH_L", "vqdmulh_lane", "s24",
     ["sqdmulh {D}.{arr}, {N}.{arr}, {M}.{arrB}[{i}]"],
     form="3REG_IDX", area="lane")
_add("op2", "SQRDMULH_L", "vqrdmulh_lane", "s24",
     ["sqrdmulh {D}.{arr}, {N}.{arr}, {M}.{arrB}[{i}]"],
     form="3REG_IDX", area="lane")
_add("op2", "SMULL_L", "vmull_lane", "s124",
     ["smull {D}.{arrR}, {N}.{arr}, {M}.{arrB}[{i}]"],
     kind="WIDEN_LO", form="3REG_IDX", naming="src", area="lane")
_add("op2", "UMULL_L", "vmull_lane", "u124",
     ["umull {D}.{arrR}, {N}.{arr}, {M}.{arrB}[{i}]"],
     kind="WIDEN_LO", form="3REG_IDX", naming="src", area="lane")
_add("op3", "MLA_L", "vmla_lane", "s124 u124",
     ["mla {D}.{arr}, {N}.{arr}, {M}.{arrB}[{i}]"], form="3REG_IDX",
     area="lane", ref="a[i] + b[i] * cv")
_add("op3", "MLS_L", "vmls_lane", "s124 u124",
     ["mls {D}.{arr}, {N}.{arr}, {M}.{arrB}[{i}]"], form="3REG_IDX",
     area="lane", ref="a[i] - b[i] * cv")
_add("op3", "FMLA_L", "vmla_lane", "f48",
     ["fmla {D}.{arr}, {N}.{arr}, {M}.{arrB}[{i}]"], form="3REG_IDX",
     extra_stems=["vfmla_lane", "vfma_lane"], area="lane",
     ref="a[i] + b[i] * cv")
_add("op3", "FMLS_L", "vmls_lane", "f48",
     ["fmls {D}.{arr}, {N}.{arr}, {M}.{arrB}[{i}]"], form="3REG_IDX",
     extra_stems=["vfmls_lane", "vfms_lane"], area="lane",
     ref="a[i] - b[i] * cv")
_add("op3", "SMLAL_L", "vmlal_lane", "s1248",
     ["smlal {D}.{arr}, {N}.{arrB}, {M}.{arrB}[{i}]"],
     kind="ACCW_LO", form="3REG_IDX", naming="src", area="lane")
_add("op3", "UMLAL_L", "vmlal_lane", "u1248",
     ["umlal {D}.{arr}, {N}.{arrB}, {M}.{arrB}[{i}]"],
     kind="ACCW_LO", form="3REG_IDX", naming="src", area="lane")
_add("op3", "SMLSL_L", "vmlsl_lane", "s1248",
     ["smlsl {D}.{arr}, {N}.{arrB}, {M}.{arrB}[{i}]"],
     kind="ACCW_LO", form="3REG_IDX", naming="src", area="lane")
_add("op3", "UMLSL_L", "vmlsl_lane", "u1248",
     ["umlsl {D}.{arr}, {N}.{arrB}, {M}.{arrB}[{i}]"],
     kind="ACCW_LO", form="3REG_IDX", naming="src", area="lane")
_add("op3", "SDOT_L", "vdot_lane", "s4",
     ["sdot {D}.{arr}, {N}.{arrB}, {M}.{arrI}[{i}]"], kind="DOT",
     oes=1, form="3REG_IDX", feat="DOTPROD", area="dotprod")
_add("op3", "UDOT_L", "vdot_lane", "u4",
     ["udot {D}.{arr}, {N}.{arrB}, {M}.{arrI}[{i}]"], kind="DOT",
     oes=1, form="3REG_IDX", feat="DOTPROD", area="dotprod")

# ------------------------------------------------------------------
# shuffle family (zip/uzp/trn are register forms; ext carries a
# byte index in the opc immediate field)
# ------------------------------------------------------------------
for _op, _stem, _mn in [("ZIP1", "vzip1", "zip1"), ("ZIP2", "vzip2", "zip2"),
                        ("UZP1", "vuzp1", "uzp1"), ("UZP2", "vuzp2", "uzp2"),
                        ("TRN1", "vtrn1", "trn1"), ("TRN2", "vtrn2", "trn2")]:
    _add("shuf", _op, _stem, "s1248 u1248 p12 f48",
         ["%s {D}.{arr}, {N}.{arr}, {M}.{arr}" % _mn], area="permute")
_add("shuf", "EXT", "vext", "s1248 u1248 p12 f48",
     ["ext {D}.{arr8}, {N}.{arr8}, {M}.{arr8}, #{i}"], style="op2i",
     imm=(0, "totm1"), area="permute")


# ------------------------------------------------------------------
# comparison intrinsics: the header maps them to C operators,
# whose code generation reaches the CMxx instructions through
# gen_opv; the result element is signed (unsigned for unsigned
# operands) of the operand element size.
# ------------------------------------------------------------------
for _op, _stem, _oper in [("CEQ", "vceq", "=="), ("CGT", "vcgt", ">"),
                          ("CGE", "vcge", ">="), ("CLT", "vclt", "<"),
                          ("CLE", "vcle", "<=")]:
    _add("op2", _op, _stem, INT + " f48", [], style="cmp",
         oper=_oper, ressign="u", area="cmp")



# ------------------------------------------------------------------
# widening adds and subtracts, wide adds, count-leading-sign,
# saturating rounding shifts, RDMA accumulating multiplies,
# widening multiply by lane and the high narrowing shifts
_add("op2", "SADDL", "vaddl", "s124",
     ["saddl {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_LO",
     naming="src", area="widen", ref="(long long)a[i] + b[i]")
_add("op2", "UADDL", "vaddl", "u124",
     ["uaddl {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_LO",
     naming="src", area="widen", ref="(long long)a[i] + b[i]")
_add("op2", "SADDL2", "vaddl_high", "s124",
     ["saddl2 {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_HI",
     naming="src", area="widen", ref="(long long)a[i] + b[i]")
_add("op2", "UADDL2", "vaddl_high", "u124",
     ["uaddl2 {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_HI",
     naming="src", area="widen", ref="(long long)a[i] + b[i]")
_add("op2", "SSUBL", "vsubl", "s124",
     ["ssubl {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_LO",
     naming="src", area="widen", ref="(long long)a[i] - b[i]")
_add("op2", "USUBL", "vsubl", "u124",
     ["usubl {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_LO",
     naming="src", area="widen", ref="(long long)a[i] - b[i]")
_add("op2", "SSUBL2", "vsubl_high", "s124",
     ["ssubl2 {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_HI",
     naming="src", area="widen", ref="(long long)a[i] - b[i]")
_add("op2", "USUBL2", "vsubl_high", "u124",
     ["usubl2 {D}.{arrR}, {N}.{arr}, {M}.{arr}"], kind="WIDEN_HI",
     naming="src", area="widen", ref="(long long)a[i] - b[i]")
_add("op2", "SADDW", "vaddw", "s248",
     ["saddw {D}.{arr}, {N}.{arr}, {M}.{arrB}"], kind="ADDW_LO",
     naming="srcb", area="widen", ref="a[i] + (long long)b[i]")
_add("op2", "UADDW", "vaddw", "u248",
     ["uaddw {D}.{arr}, {N}.{arr}, {M}.{arrB}"], kind="ADDW_LO",
     naming="srcb", area="widen", ref="a[i] + (long long)b[i]")
_add("op2", "SADDW2", "vaddw_high", "s248",
     ["saddw2 {D}.{arr}, {N}.{arr}, {M}.{arrB}"], kind="ADDW_HI",
     naming="srcb", area="widen", ref="a[i] + (long long)b[i]")
_add("op2", "UADDW2", "vaddw_high", "u248",
     ["uaddw2 {D}.{arr}, {N}.{arr}, {M}.{arrB}"], kind="ADDW_HI",
     naming="srcb", area="widen", ref="a[i] + (long long)b[i]")
_add("op2", "SSUBW", "vsubw", "s248",
     ["ssubw {D}.{arr}, {N}.{arr}, {M}.{arrB}"], kind="ADDW_LO",
     naming="srcb", area="widen", ref="a[i] - (long long)b[i]")
_add("op2", "USUBW", "vsubw", "u248",
     ["usubw {D}.{arr}, {N}.{arr}, {M}.{arrB}"], kind="ADDW_LO",
     naming="srcb", area="widen", ref="a[i] - (long long)b[i]")
_add("op2", "SSUBW2", "vsubw_high", "s248",
     ["ssubw2 {D}.{arr}, {N}.{arr}, {M}.{arrB}"], kind="ADDW_HI",
     naming="srcb", area="widen", ref="a[i] - (long long)b[i]")
_add("op2", "USUBW2", "vsubw_high", "u248",
     ["usubw2 {D}.{arr}, {N}.{arr}, {M}.{arrB}"], kind="ADDW_HI",
     naming="srcb", area="widen", ref="a[i] - (long long)b[i]")
_add("op2", "SQRSHL", "vqrshl", "s1248",
     ["sqrshl {D}.{arr}, {N}.{arr}, {M}.{arr}"], kind="SHIFT",
     asm64=["sqrshl {Dd}, {Nd}, {Md}"], area="shift")
_add("op2", "UQRSHL", "vqrshl", "u1248",
     ["uqrshl {D}.{arr}, {N}.{arr}, {M}.{arr}"], kind="SHIFT",
     asm64=["uqrshl {Dd}, {Nd}, {Md}"], area="shift")
_add("op3", "SQRDMLAH", "vqrdmlah", "s24",
     ["sqrdmlah {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="mul")
_add("op3", "SQRDMLSH", "vqrdmlsh", "s24",
     ["sqrdmlsh {D}.{arr}, {N}.{arr}, {M}.{arr}"], area="mul")
_add("op2", "SMULL2_L", "vmull_high_lane", "s124",
     ["smull2 {D}.{arrR}, {N}.{arr}, {M}.{arrI}[{i}]"],
     kind="WIDEN_HI", form="3REG_IDX", area="lane")
_add("op2", "UMULL2_L", "vmull_high_lane", "u124",
     ["umull2 {D}.{arrR}, {N}.{arr}, {M}.{arrI}[{i}]"],
     kind="WIDEN_HI", form="3REG_IDX", area="lane")
_add("op3", "SQRDMLAH_L", "vqrdmlah_lane", "s24",
     ["sqrdmlah {D}.{arr}, {N}.{arr}, {M}.{arrI}[{i}]"],
     form="3REG_IDX", area="lane")
_add("op3", "SQRDMLSH_L", "vqrdmlsh_lane", "s24",
     ["sqrdmlsh {D}.{arr}, {N}.{arr}, {M}.{arrI}[{i}]"],
     form="3REG_IDX", area="lane")
for _op, _stem, _mn in [
        ("SHRN2", "vshrn_high_n", "shrn2"),
        ("RSHRN2", "vrshrn_high_n", "rshrn2"),
        ("SQSHRN2", "vqshrn_high_n", "sqshrn2"),
        ("SQRSHRN2", "vqrshrn_high_n", "sqrshrn2"),
        ("SQSHRUN2", "vqshrun_high_n", "sqshrun2"),
        ("SQRSHRUN2", "vqrshrun_high_n", "sqrshrun2")]:
    _kw = dict(kind="NARROW_HI", form="2REG_IMM_ACC",
               style="op2i2", naming="srcb", imm=(1, "es8"),
               area="shiftimm")
    if "RUN" in _op:
        _kw["ressign"] = "u"
    _add("op2", _op, _stem, "s124",
         ["%s {D}.{arrR}, {N}.{arrB}, #{i}" % _mn], **_kw)
_add("op2", "UQSHRN2", "vqshrn_high_n", "u124",
     ["uqshrn2 {D}.{arrR}, {N}.{arrB}, #{i}"],
     kind="NARROW_HI", form="2REG_IMM_ACC", style="op2i2",
     naming="srcb", imm=(1, "es8"), area="shiftimm")
_add("op2", "USHRN2", "vshrn_high_n", "u124",
     ["shrn2 {D}.{arrR}, {N}.{arrB}, #{i}"],
     kind="NARROW_HI", form="2REG_IMM_ACC", style="op2i2",
     naming="srcb", imm=(1, "es8"), area="shiftimm")
_add("op2", "URSHRN2", "vrshrn_high_n", "u124",
     ["rshrn2 {D}.{arrR}, {N}.{arrB}, #{i}"],
     kind="NARROW_HI", form="2REG_IMM_ACC", style="op2i2",
     naming="srcb", imm=(1, "es8"), area="shiftimm")
_add("op2", "UQRSHRN2", "vqrshrn_high_n", "u124",
     ["uqrshrn2 {D}.{arrR}, {N}.{arrB}, #{i}"],
     kind="NARROW_HI", form="2REG_IMM_ACC", style="op2i2",
     naming="srcb", imm=(1, "es8"), area="shiftimm")

# ------------------------------------------------------------------
# mixed-sign saturating adds, the remaining widening RDMA forms,
# high narrowing adds, float multiply-extended, the horizontal
# float max/min-number reductions and the round-to-odd converts
_add("op2", "USQADD", "vsqadd", "u1248",
     ["usqadd {D}.{arr}, {N}.{arr}"], bkind="s", form="2REG_ACC",
     asm64=["usqadd {Dd}, {Nd}"],
     area="arith", ref="usqadd_ref(a[i], b[i])")
_add("op2", "SUQADD", "vuqadd", "s1248",
     ["suqadd {D}.{arr}, {N}.{arr}"], bkind="u", form="2REG_ACC",
     asm64=["suqadd {Dd}, {Nd}"],
     area="arith", ref="uqadd_ref(a[i], b[i])")
_add("op3", "SQDMLAL", "vqdmlal", "s48",
     ["sqdmlal {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_LO",
     naming="srcb", area="mul")
_add("op3", "SQDMLSL", "vqdmlsl", "s48",
     ["sqdmlsl {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_LO",
     naming="srcb", area="mul")
_add("op3", "SQDMLAL2", "vqdmlal_high", "s48",
     ["sqdmlal2 {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_HI",
     naming="srcb", area="mul")
_add("op3", "SQDMLSL2", "vqdmlsl_high", "s48",
     ["sqdmlsl2 {D}.{arr}, {N}.{arrB}, {M}.{arrB}"], kind="ACCW_HI",
     naming="srcb", area="mul")
_add("op3", "SQDMLAL_L", "vqdmlal_lane", "s48",
     ["sqdmlal {D}.{arr}, {N}.{arrB}, {M}.{arrB}[{i}]"],
     kind="ACCW_LO", form="3REG_IDX", naming="src", area="lane")
_add("op3", "SQDMLSL_L", "vqdmlsl_lane", "s48",
     ["sqdmlsl {D}.{arr}, {N}.{arrB}, {M}.{arrB}[{i}]"],
     kind="ACCW_LO", form="3REG_IDX", naming="src", area="lane")
_add("op3", "SQDMLAL2_L", "vqdmlal_high_lane", "s48",
     ["sqdmlal2 {D}.{arr}, {N}.{arrB}, {M}.{arrB}[{i}]"],
     kind="ACCW_HI", form="3REG_IDX", naming="src", area="lane")
_add("op3", "SQDMLSL2_L", "vqdmlsl_high_lane", "s48",
     ["sqdmlsl2 {D}.{arr}, {N}.{arrB}, {M}.{arrB}[{i}]"],
     kind="ACCW_HI", form="3REG_IDX", naming="src", area="lane")
_add("op2", "SQDMULL_L", "vqdmull_lane", "s24",
     ["sqdmull {D}.{arrR}, {N}.{arr}, {M}.{arrI}[{i}]"],
     kind="WIDEN_LO", form="3REG_IDX", area="lane")
_add("op2", "SQDMULL2_L", "vqdmull_high_lane", "s24",
     ["sqdmull2 {D}.{arrR}, {N}.{arr}, {M}.{arrI}[{i}]"],
     kind="WIDEN_HI", form="3REG_IDX", area="lane")
_add("op2", "FMULX", "vmulx", "f48",
     ["fmulx {D}.{arr}, {N}.{arr}, {M}.{arr}"],
     asm64=["fmulx {Dd}, {Nd}, {Md}"], area="float")
_add("op1r", "FMAXNMV", "vmaxnmv", "f48",
     ["fmaxnmv {Sd}, {N}.{arr}", "fmaxnmp {Dd}, {N}.{arr}"],
     kind="SCALAR", area="reduce")
_add("op1r", "FMINNMV", "vminnmv", "f48",
     ["fminnmv {Sd}, {N}.{arr}", "fminnmp {Dd}, {N}.{arr}"],
     kind="SCALAR", area="reduce")
_add("op1", "FCVTL2", "vcvt_high", "f4",
     ["fcvtl2 {D}.{arrR}, {N}.{arr}"], kind="WIDEN_HI",
     naming="cvt", totals=(16,), area="cvt")
_add("op1", "CVTX", "vcvtx", "f8",
     ["fcvtxn {D}.{arrR}, {N}.{arr}"], kind="NARROW",
     naming="cvt", totals=(16,), area="cvt")
_add("op2", "CVTX2", "vcvtx_high", "f4",
     ["fcvtxn2 {D}.{arrR}, {N}.{arrB}"], kind="NARROW_HI",
     form="2REG_ACC", naming="cvt", area="cvt")

# ------------------------------------------------------------------
# scalar (single-lane D register) forms: many operations on a
# one-element vector (int64x1_t / float64x1_t) are scalar SIMD
# instructions.  They are appended last so the vector templates
# win wherever both assemble.
# ------------------------------------------------------------------
_SCALAR2 = ["{Dd}, {Nd}, {Md}"]
_SCALAR1 = ["{Dd}, {Nd}"]
_SCALAR1I = ["{Dd}, {Nd}, #{i}"]
_FALLBACKS = {}
for _op, _mn in [("FMUL", "fmul"), ("FDIV", "fdiv"), ("FABD", "fabd"),
                 ("FRECPS", "frecps"), ("FRSQRTS", "frsqrts"),
                 ("FMAX", "fmax"), ("FMIN", "fmin"),
                 ("FMAXNM", "fmaxnm"), ("FMINNM", "fminnm"),
                 ("CMEQ", "cmeq"), ("CMGT", "cmgt"), ("CMGE", "cmge"),
                 ("CMHI", "cmhi"), ("CMHS", "cmhs"),
                 ("CMTST", "cmtst"), ("FCMEQ", "fcmeq"),
                 ("FCMGT", "fcmgt"), ("FCMGE", "fcmge"),
                 ("SQADD", "sqadd"), ("UQADD", "uqadd"),
                 ("SQSUB", "sqsub"), ("UQSUB", "uqsub"),
                 ("SSHL", "sshl"), ("USHL", "ushl"),
                 ("SRSHL", "srshl"), ("URSHL", "urshl"),
                 ("SQSHL", "sqshl"), ("UQSHL", "uqshl")]:
    _FALLBACKS[_op] = ["%s %s" % (_mn, _SCALAR2[0])]
for _op, _mn in [("ABS", "abs"), ("NEG", "neg"), ("SQABS", "sqabs"),
                 ("SQNEG", "sqneg"), ("FNEG", "fneg"), ("FABS", "fabs"),
                 ("FRECPE", "frecpe"), ("FRSQRTE", "frsqrte"),
                 ("FRINTN", "frintn"), ("FRINTM", "frintm"),
                 ("FRINTP", "frintp"), ("FRINTZ", "frintz"),
                 ("FRINTA", "frinta"), ("FRINTI", "frinti"),
                 ("FRINTX", "frintx")]:
    _FALLBACKS[_op] = ["%s %s" % (_mn, _SCALAR1[0])]
for _op, _mn in [("CMEQZ", "cmeq"), ("CMGTZ", "cmgt"),
                 ("CMGEZ", "cmge"), ("CMLEZ", "cmle"),
                 ("CMLTZ", "cmlt"), ("FCMEQZ", "fcmeq"),
                 ("FCMGTZ", "fcmgt"), ("FCMGEZ", "fcmge"),
                 ("FCMLEZ", "fcmle"), ("FCMLTZ", "fcmlt")]:
    _FALLBACKS[_op] = ["%s %s, #0" % (_mn, _SCALAR1[0])]
for _op, _mn in [("SSHR", "sshr"), ("USHR", "ushr"),
                 ("SRSHR", "srshr"), ("URSHR", "urshr"),
                 ("SSRA", "ssra"), ("USRA", "usra"),
                 ("SRSRA", "srsra"), ("URSRA", "ursra"),
                 ("SHL", "shl"), ("SQSHL_N", "sqshl"),
                 ("UQSHL_N", "uqshl"), ("SQSHLU_N", "sqshlu"),
                 ("SRI", "sri"), ("SLI", "sli")]:
    _FALLBACKS[_op] = ["%s %s" % (_mn, _SCALAR1I[0])]
for _op, _mn in [("FMLA", "fmla"), ("FMLS", "fmls")]:
    _FALLBACKS[_op] = ["%s %s" % (_mn, _SCALAR2[0])]
for _op, _mn in [("FMUL_L", "fmul"), ("FMULX_L", "fmulx"),
                 ("FMLA_L", "fmla"), ("FMLS_L", "fmls")]:
    _FALLBACKS[_op] = ["%s {Dd}, {Nd}, {M}.d[{i}]" % _mn]
for _e in OPS:
    if _e["op"] in _FALLBACKS and _e["asm"]:
        _e["asm"] = _e["asm"] + _FALLBACKS[_e["op"]]
    if _e["op"] in ("FADDP", "FMAXP", "FMINP", "FMAXNMP", "FMINNMP"):
        _e["bad"] = [(8, 8)]
