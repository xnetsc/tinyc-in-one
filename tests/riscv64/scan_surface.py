#!/usr/bin/env python3
"""Coverage accounting for the shipped riscv_vector.h surface.

Compares the macro names in vendor-tinycc/include/riscv_vector.h
against the GCC 13.3 intrinsic name set in gcc_names.txt (see
gcc_names_gen.py for how that set is produced), classifies every
residual name, checks that each macro built on a compiler builtin
names an opcode the backend tables contain, and reports which
shipped macros no probe exercises.
"""
import importlib.util
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
HEADER = os.path.join(ROOT, "vendor-tinycc", "include", "riscv_vector.h")
GCC_NAMES = os.path.join(ROOT, "tests", "riscv64", "gcc_names.txt")
SPEC = os.path.join(ROOT, "tools", "riscv_spec.py")
PROBES = os.path.join(ROOT, "tests", "riscv64", "probes")

PRED = ("_m", "_mu", "_tu", "_tum", "_tumu")


def _rx(pat):
    return re.compile(pat).match


def _mem_masked(n):
    return (re.match(r"__riscv_(vle\d+|vse\d+|vlse\d+|vsse\d+|"
                     r"vluxei\d+|vloxei\d+|vsuxei\d+|vsoxei\d+)_v_",
                     n) and n.endswith(PRED))


# residual classes, matched in order; anything unmatched is a
# genuine gap and fails the scan
CLASSES = [
    ("VL programming-model entry points (vsetvl, vsetvlmax, "
     "read_vl): the surface is fixed-length and vl is always the "
     "type's lane count",
     _rx(r"__riscv_(vsetvl|vsetvlmax|read_vl)")),
    ("fault-first loads (vle*ff): they return a shortened vl, "
     "which the fixed-length model does not expose",
     _rx(r"__riscv_vle\d+ff_")),
    ("masked load and store forms: the load/store generics carry "
     "one auxiliary operand and cannot express a (mask, "
     "maskedoff, pointer) triple",
     _mem_masked),
    ("register-group insert and extract (vset, vget): whole-group "
     "operations outside the fixed-length value model",
     _rx(r"__riscv_v(set|get)_v_")),
    ("LMUL-changing reinterpretation (vlmul_ext, vlmul_trunc)",
     _rx(r"__riscv_vlmul_(ext|trunc)_v_")),
    ("same-size reinterpretation spellings (vreinterpret): a "
     "plain cast between the fixed-length types",
     _rx(r"__riscv_vreinterpret_v_")),
    ("undefined-value generators (vundefined): no defined value "
     "to ship",
     _rx(r"__riscv_vundefined")),
    ("vncvt: API synonym for a narrowing shift by zero (vnsrl)",
     _rx(r"__riscv_vncvt_")),
    ("carry-out forms (vmadc, vmsbc): the design ships the "
     "carry-in forms vadc and vsbc; the twins that also return "
     "the carry as a mask are not shipped",
     _rx(r"__riscv_v(madc|msbc)_")),
    ("greater-than compares (vmsgt, vmsge, vmsgtu, vmsgeu, "
     "vmfgt, vmfge): operand-swapped spellings of the shipped "
     "lt and le compares",
     _rx(r"__riscv_vm(sgt|sgtu|sge|sgeu|fgt|fge)_")),
    ("mask move and constant mask generators (vmmv, vmclr, "
     "vmset)",
     _rx(r"__riscv_v(mmv_m|mclr_m|mset_m)")),
    ("masked variants of the mask bit and inspection operations "
     "(vmsbf, vmsif, vmsof, vcpop, vfirst): shipped unmasked "
     "only",
     _rx(r"__riscv_(vmsbf|vmsif|vmsof|vcpop|vfirst)_")),
    ("integer widening multiply-accumulate (vwmacc, vwmaccu, "
     "vwmaccsu, vwmaccus): the design ships the widening "
     "multiply without accumulation",
     _rx(r"__riscv_vwmacc")),
    ("widening scalar spellings (vwmul_vx, vwmulu_vx, "
     "vwmulsu_vx, vwsub_vx, vwsub_wx, vwsubu_vx, vwsubu_wx): "
     "gcc aliases these onto the vector widening forms",
     _rx(r"__riscv_vw(mul|mulu|mulsu|sub|subu)_(vx|wx)")),
    ("sign and zero extension forms (vsext, vzext)",
     _rx(r"__riscv_v(sext|zext)_")),
    ("integer widening converts (vwcvt, vwcvtu)",
     _rx(r"__riscv_vwcvt")),
    ("vsmul: fixed-point rounding multiply; no opcode in the "
     "design",
     _rx(r"__riscv_vsmul_")),
    ("rounding shift forms (vssrl, vssra)",
     _rx(r"__riscv_vss(rl|ra)_")),
    ("vfrdiv: gcc implements it as a composite (splat and "
     "divide); there is no instruction",
     _rx(r"__riscv_vfrdiv_")),
    ("widening round-to-zero converts (vfwcvt_rtz_x, "
     "vfwcvt_rtz_xu)",
     _rx(r"__riscv_vfwcvt_rtz_")),
    ("vfwnmacc: no widening negated-accumulate instruction "
     "exists; gcc composes it from the shipped forms",
     _rx(r"__riscv_vfwnmacc_")),
    ("_tu tail-undisturbed variants: the fixed-length model has "
     "no inactive tail elements, so the plain form is the "
     "canonical spelling",
     lambda n: n.endswith("_tu")),
    ("masked and policy variants of families shipped "
     "unmasked-only (the masked core set is the arithmetic, "
     "logical, shift, compare, multiply-accumulate and merge "
     "families)",
     lambda n: n.endswith(PRED)),
]


def main():
    with open(HEADER) as f:
        header = f.read()
    shipped = set(re.findall(r"^#define (__riscv_[a-z0-9_]+)",
                             header, re.M))
    with open(GCC_NAMES) as f:
        gcc = set(l.strip() for l in f
                  if l.strip() and not l.startswith("#"))

    extra = sorted(shipped - gcc)
    residual = sorted(gcc - shipped)

    spec = importlib.util.spec_from_file_location("riscv_spec", SPEC)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    valid_opc = set(mod.OPCODES.values()) | {1000, 1001, 1002,
                                              1003, 1004, 1005}
    used_opc = set()
    for m in re.finditer(r"__builtin_rvv_\w+\((\d+)", header):
        used_opc.add(int(m.group(1)))
    dangling = sorted(used_opc - valid_opc)

    counts = {}
    unclassified = []
    for n in residual:
        for reason, pred in CLASSES:
            if pred(n):
                counts[reason] = counts.get(reason, 0) + 1
                break
        else:
            unclassified.append(n)

    tested = set()
    for fn in os.listdir(PROBES):
        if fn.endswith(".c"):
            with open(os.path.join(PROBES, fn)) as f:
                tested |= set(re.findall(r"__riscv_[a-z0-9_]+",
                                         f.read()))
    untested = sorted(shipped - tested)

    print("shipped macros: %d" % len(shipped))
    print("gcc 13.3 name set: %d" % len(gcc))
    print("shipped outside the gcc set: %d" % len(extra))
    for n in extra[:20]:
        print("  EXTRA %s" % n)
    print("residual (gcc names not shipped): %d" % len(residual))
    for reason, _ in CLASSES:
        if reason in counts:
            print("  %5d  %s" % (counts[reason], reason))
    print("unclassified residual: %d" % len(unclassified))
    for n in unclassified[:20]:
        print("  GAP %s" % n)
    print("dangling opcodes referenced by the header: %d"
          % len(dangling))
    for n in dangling[:20]:
        print("  DANGLING %s" % n)
    print("shipped macros exercised by probes: %d of %d"
          % (len(shipped) - len(untested), len(shipped)))

    if extra or unclassified or dangling:
        print("SCAN FAILED")
        return 1
    print("SCAN OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
