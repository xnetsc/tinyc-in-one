#!/usr/bin/env python3
# Expand the GCC 13.3 RVV intrinsic name set from the compiler's own
# registration tables (riscv-vector-builtins*.def/.cc), reimplementing
# the shape get_name rules from riscv-vector-builtins-shapes.cc and the
# derived-type roles semantically. Output: sorted full names, one per line.
import re, sys
from fractions import Fraction

# The six GCC 13.3.0 source files (from the gcc-13.3.0 release,
# gcc/config/riscv/): riscv-vector-builtins.def, -types.def,
# -functions.def, -shapes.cc, -bases.cc and riscv-vector-builtins.cc.
# Point RISCV_GCC13_SRC (or argv[1]) at the directory holding them.
import os
SRC = sys.argv[1] if len(sys.argv) > 1 else os.environ.get(
    'RISCV_GCC13_SRC', '/tmp/rvvref')
if not SRC.endswith('/'):
    SRC += '/'

def read(n):
    with open(SRC + n) as f: return f.read()

def strip_comments(t):
    return re.sub(r'/\*.*?\*/', ' ', t, flags=re.S)

# ---- types -------------------------------------------------------------
bdef = strip_comments(read('riscv-vector-builtins.def'))
types = {}          # name -> dict
by_key = {}         # (kind, e, lnum, lden) -> name ; bool: ('b', N, 1, 1)
for m in re.finditer(r'DEF_RVV_TYPE \(([^)]*)\)', bdef):
    f = [x.strip() for x in m.group(1).split(',')]
    name, vsuf, ssuf, vsetvl = f[0], f[6], f[7], f[8]
    mb = re.fullmatch(r'vbool(\d+)_t', name)
    if mb:
        key = ('b', int(mb.group(1)), 1, 1)
    else:
        mm = re.fullmatch(r'v(int|uint|float)(\d+)m(f)?(\d+)_t', name)
        kind = {'int': 'si', 'uint': 'ui', 'float': 'f'}[mm.group(1)]
        e = int(mm.group(2))
        if mm.group(3): key = (kind, e, 1, int(mm.group(4)))
        else: key = (kind, e, int(mm.group(4)), 1)
    types[name] = dict(name=name, key=key, vsuf=vsuf, ssuf=ssuf, vsetvl=vsetvl)
    by_key[key] = name

def lookup(kind, e, lfrac):
    return by_key.get((kind, e, lfrac.numerator, lfrac.denominator))

def key_parts(t):
    k = types[t]['key']
    return k[0], k[1], Fraction(k[2], k[3])

NONVECTOR = object()

def role(t, r):
    """Resolve RVV_BASE_<r> for type t -> type name, ('tuple', t, n), None
    (structurally absent), or NONVECTOR for scalar-ish roles."""
    kind, e, l = key_parts(t)
    if r == 'vector': return t
    if r == 'mask':
        if kind == 'b': return t
        n = e / l
        if n.denominator != 1: return None
        return by_key.get(('b', int(n), 1, 1))
    if r == 'signed_vector':   return None if kind == 'b' else lookup('si', e, l)
    if r == 'unsigned_vector': return None if kind == 'b' else lookup('ui', e, l)
    if r == 'float_vector':    return lookup('f', e, l) if e in (32, 64) else None
    if r == 'shift_vector':    return None if kind == 'b' else lookup('ui', e, l)
    if r == 'double_trunc_vector':
        return None if kind == 'b' else lookup(kind, e // 2, l / 2)
    if r == 'quad_trunc_vector':
        return None if kind == 'b' else lookup(kind, e // 4, l / 4)
    if r == 'oct_trunc_vector':
        return None if kind == 'b' else lookup(kind, e // 8, l / 8)
    if r == 'double_trunc_signed_vector':   return lookup('si', e // 2, l / 2)
    if r == 'double_trunc_unsigned_vector': return lookup('ui', e // 2, l / 2)
    if r == 'double_trunc_float_vector':
        return lookup('f', e // 2, l / 2) if e // 2 in (32, 64) else None
    if r == 'lmul1_vector':        return lookup(kind, e, Fraction(1))
    if r == 'widen_lmul1_vector':  return None if kind == 'b' else lookup(kind, e * 2, Fraction(1))
    m = re.fullmatch(r'eew(\d+)_index', r)
    if m:
        n = int(m.group(1))
        return lookup('ui', n, l * n / e)
    m = re.fullmatch(r'eew(\d+)_interpret', r)
    if m:
        if kind == 'b': return None
        return lookup(kind, int(m.group(1)), l)
    m = re.fullmatch(r'vlmul_ext_x(\d+)', r)
    if m:
        return lookup(kind, e, l * int(m.group(1)))
    return NONVECTOR

def suffix_of(rt):
    if rt is None or rt is NONVECTOR: return None
    if isinstance(rt, tuple): return types[rt[1]]['vsuf'] + 'x' + str(rt[2])
    return types[rt]['vsuf']

# ---- operand / predication suffixes -------------------------------------
op_suffix = {'none': ''}
for m in re.finditer(r'DEF_RVV_OP_TYPE \((\w+)\)', bdef):
    op_suffix[m.group(1)] = '_' + m.group(1)
pred_suffix = {'none': ''}
for m in re.finditer(r'DEF_RVV_PRED_TYPE \((\w+)\)', bdef):
    pred_suffix[m.group(1)] = '_' + m.group(1)

# ---- type categories (types.def) ----------------------------------------
cats = {}
tdef = strip_comments(read('riscv-vector-builtins-types.def'))
for m in re.finditer(r'DEF_RVV_([A-Z0-9_]+)_OPS \((\w+),', tdef):
    cats.setdefault(m.group(1), []).append(m.group(2))

# ---- builtins.cc: type arrays, pred arrays, arg arrays, op_infos --------
bcc = strip_comments(read('riscv-vector-builtins.cc'))
type_arrays = {}
for m in re.finditer(r'rvv_type_info (\w+)\[\]\s*=\s*\{(.*?)\};', bcc, re.S):
    cs = re.findall(r'DEF_RVV_([A-Z0-9_]+)_OPS', m.group(2))
    lst = []
    for c in cs: lst += cats.get(c, [])
    type_arrays[m.group(1)] = lst
pred_arrays = {}
for m in re.finditer(r'predication_type_index (\w+)\[\]\s*=\s*\{(.*?)\};', bcc, re.S):
    pred_arrays[m.group(1)] = re.findall(r'PRED_TYPE_(\w+)', m.group(2))
arg_arrays = {}
for m in re.finditer(r'rvv_arg_type_info (\w+)\[\]\s*=\s*\{(.*?)\};', bcc, re.S):
    arg_arrays[m.group(1)] = re.findall(r'RVV_BASE_(\w+)', m.group(2))
op_infos = {}
for m in re.finditer(r'rvv_op_info (\w+)\s*=\s*\{(.*?)\};', bcc, re.S):
    body = m.group(2)
    types_name = re.match(r'\s*(\w+)', body).group(1)
    op = re.search(r'OP_TYPE_(\w+)', body).group(1)
    ret = re.search(r'RVV_BASE_(\w+)', body).group(1)
    args_name = None
    for tok in re.findall(r'[A-Za-z_][A-Za-z0-9_]*', body):
        if tok in arg_arrays: args_name = tok
    assert args_name, m.group(1)
    op_infos[m.group(1)] = (types_name, op, ret, args_name)

# ---- functions.def -------------------------------------------------------
fdef = strip_comments(read('riscv-vector-builtins-functions.def'))
functions = []
for m in re.finditer(r'DEF_RVV_FUNCTION \((\w+), (\w+), (\w+), (\w+)\)', fdef):
    functions.append(m.groups())

# ---- name construction ----------------------------------------------------
def build_name(base, shape, op, t, ret_r, arg_roles, pred):
    Tsuf = types[t]['vsuf']
    kind, e, l = key_parts(t)
    ops = op_suffix[op]
    ps = pred_suffix[pred]
    ret = role(t, ret_r)
    retsuf = suffix_of(ret)
    for ar in arg_roles:
        if role(t, ar) is None:
            return None
    if shape in ('alu', 'widen_alu', 'no_mask_policy', 'move'):
        if ret is None: return None
        return base + ops + Tsuf + ps
    if shape == 'loadstore':
        if ret is None: return None
        return base + ('' if kind == 'b' else str(e)) + ops + Tsuf + ps
    if shape == 'indexed_loadstore':
        if ret is None: return None
        if len(arg_roles) < 2 or role(t, arg_roles[1]) is None: return None
        return base + ops + Tsuf + ps
    if shape == 'fault_load':
        if ret is None: return None
        return 'vle' + str(e) + 'ff' + ops + Tsuf + ps
    if shape == 'return_mask':
        if retsuf is None: return None
        return base + ops + Tsuf + retsuf + ps
    if shape == 'narrow_alu':
        if retsuf is None: return None
        rk, re_, rl = key_parts(ret)
        if rl == 8: return None
        return base + ops + retsuf + ps
    if shape == 'mask_alu':
        if ret is None: return None
        return base + (ops if op in ('mm', 'm') else '') + Tsuf + ps
    if shape == 'reduc_alu':
        if retsuf is None: return None
        return base + ops + Tsuf + retsuf + ps
    if shape == 'misc' or shape == 'vget':
        if retsuf is None or not arg_roles: return None
        a0 = suffix_of(role(t, arg_roles[0]))
        if a0 is None: return None
        return base + ops + a0 + retsuf
    if shape == 'vset':
        if retsuf is None or len(arg_roles) < 3: return None
        a2 = suffix_of(role(t, arg_roles[2]))
        if a2 is None: return None
        return base + ops + a2 + retsuf
    if shape == 'scalar_move':
        if ret is None: return None
        return base + ops + Tsuf + types[t]['ssuf']
    if shape == 'vundefined':
        return base + Tsuf
    if shape in ('vsetvl', 'vsetvlmax'):
        return base + types[t]['vsetvl']
    if shape in ('read_vl', 'vlenb'):
        return base
    raise ValueError('unknown shape ' + shape)

names = set()
skipped_shape = set()
for base, shape, preds_name, ops_name in functions:
    types_name, op, ret_r, args_name = op_infos[ops_name]
    arg_roles = arg_arrays[args_name]
    if shape in ('read_vl', 'vlenb'):
        names.add('__riscv_' + base)
        continue
    for t in type_arrays[types_name]:
        for pred in pred_arrays[preds_name]:
            try:
                n = build_name(base, shape, op, t, ret_r, arg_roles, pred)
            except ValueError:
                skipped_shape.add(shape); continue
            if n: names.add('__riscv_' + n)

if skipped_shape:
    print('UNHANDLED SHAPES:', skipped_shape, file=sys.stderr)
for n in sorted(names): print(n)
