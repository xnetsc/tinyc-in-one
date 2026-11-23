#!/usr/bin/env python3
"""Apply the wasm32 target patches to a *copy* of the tinycc tree.

Each patch is an exact string replacement with an occurrence-count
assertion: if upstream drifts and a pattern no longer matches, the
build stops here instead of silently producing a wrong tree.
Usage: apply-patches.py <copied-tree-dir>
"""
import sys, pathlib

root = pathlib.Path(sys.argv[1])

PATCHES = [
    # 1. tcc.h: host-default target fallback must not fire when
    #    TCC_TARGET_WASM32 is given on the command line (tcc.h:163-165).
    ("tcc.h",
     "#if !defined(TCC_TARGET_I386) && !defined(TCC_TARGET_ARM) && \\\n"
     "    !defined(TCC_TARGET_ARM64) && !defined(TCC_TARGET_C67) && \\\n"
     "    !defined(TCC_TARGET_X86_64) && !defined(TCC_TARGET_RISCV64)",
     "#if !defined(TCC_TARGET_I386) && !defined(TCC_TARGET_ARM) && \\\n"
     "    !defined(TCC_TARGET_ARM64) && !defined(TCC_TARGET_C67) && \\\n"
     "    !defined(TCC_TARGET_X86_64) && !defined(TCC_TARGET_RISCV64) && \\\n"
     "    !defined(TCC_TARGET_WASM32)",
     1),
    # 2. tcc.h: TARGET_DEFS_ONLY include chain (tcc.h:386-390).
    ("tcc.h",
     "#elif defined(TCC_TARGET_RISCV64)\n"
     "# include \"riscv64-gen.c\"\n"
     "# include \"riscv64-link.c\"\n"
     "# include \"riscv64-asm.c\"",
     "#elif defined(TCC_TARGET_RISCV64)\n"
     "# include \"riscv64-gen.c\"\n"
     "# include \"riscv64-link.c\"\n"
     "# include \"riscv64-asm.c\"\n"
     "#elif defined(TCC_TARGET_WASM32)\n"
     "# include \"wasm-gen.c\"\n"
     "# include \"wasm-link.c\"",
     1),
    # 3. tcc.h: wasm backend declarations, inserted before the
    #    tcccoff.c section (mirrors the riscv64-gen.c section at
    #    tcc.h:1729-1737).
    ("tcc.h",
     "/* ------------ tcccoff.c ------------ */",
     "/* ------------ wasm-gen.c ------------ */\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "ST_FUNC void gen_va_start(void);\n"
     "ST_FUNC void gen_va_arg(CType *t);\n"
     "ST_FUNC void gen_clear_cache(void);\n"
     "ST_FUNC int wasm_output_file(TCCState *s1, const char *filename);\n"
     "ST_FUNC void gen_opv(int op);\n"
     "ST_FUNC void gen_vec_gv(void);\n"
     "ST_FUNC void gen_vec_ret(void);\n"
     "ST_FUNC void gen_vec_neg(void);\n"
     "ST_FUNC void gen_vec_not(void);\n"
     "ST_FUNC void gen_vec_op1(int opc, CType *restype);\n"
     "ST_FUNC void gen_vec_op2(int opc, CType *restype);\n"
     "ST_FUNC void gen_vec_splat(int opc, CType *restype);\n"
     "ST_FUNC void gen_vec_extract(int opc, int lane, CType *restype);\n"
     "ST_FUNC void gen_vec_replace(int opc, int lane);\n"
     "ST_FUNC void gen_vec_shuffle(const int *lanes);\n"
     "#endif\n"
     "\n"
     "/* ------------ tcccoff.c ------------ */",
     1),
    # 7. tcc.h: AttributeDef gains the vector_size byte count.
    ("tcc.h",
     "    char attr_mode; /* __attribute__((__mode__(...))) */",
     "    char attr_mode; /* __attribute__((__mode__(...))) */\n"
     "    int vector_size; /* __attribute__((vector_size(N))): total bytes, 0 = none */",
     1),
    # 8. tcc.h: VT_VECTOR flag (bit 19: outside VT_STORAGE,
    #    VT_STRUCT_MASK and VT_TYPE's stripped set).
    ("tcc.h",
     "#define VT_LONG        0x2000  /* long type (also has VT_INT rsp. VT_LLONG) */",
     "#define VT_LONG        0x2000  /* long type (also has VT_INT rsp. VT_LLONG) */\n"
     "#define VT_VECTOR      0x00080000  /* vector type: element type in ref->type,\n"
     "                                    element count in ref->c (array convention) */",
     1),
    # 9. tcctok.h: vector_size attribute tokens.
    ("tcctok.h",
     "     DEF(TOK_ALIGNED2, \"__aligned__\")",
     "     DEF(TOK_ALIGNED2, \"__aligned__\")\n"
     "     DEF(TOK_VECTOR_SIZE1, \"vector_size\")\n"
     "     DEF(TOK_VECTOR_SIZE2, \"__vector_size__\")",
     1),
    # 10. tcctok.h: wasm vector builtin tokens (wasm builds only,
    #     same conditional-DEF pattern as __builtin_va_start).
    ("tcctok.h",
     "#elif defined TCC_TARGET_RISCV64\n"
     "     DEF(TOK_builtin_va_start, \"__builtin_va_start\")\n"
     "#endif",
     "#elif defined TCC_TARGET_RISCV64\n"
     "     DEF(TOK_builtin_va_start, \"__builtin_va_start\")\n"
     "#elif defined TCC_TARGET_WASM32\n"
     "     DEF(TOK_wasm_vop1, \"__builtin_wasm_vop1\")\n"
     "     DEF(TOK_wasm_vop2, \"__builtin_wasm_vop2\")\n"
     "     DEF(TOK_wasm_splat, \"__builtin_wasm_splat\")\n"
     "     DEF(TOK_wasm_extract, \"__builtin_wasm_extract\")\n"
     "     DEF(TOK_wasm_replace, \"__builtin_wasm_replace\")\n"
     "     DEF(TOK_wasm_shuffle, \"__builtin_wasm_shuffle\")\n"
     "#endif",
     1),
    # 11. tccgen.c: parse the vector_size attribute.
    ("tccgen.c",
     "        case TOK_PACKED1:\n"
     "        case TOK_PACKED2:\n"
     "            ad->a.packed = 1;\n"
     "            break;",
     "        case TOK_VECTOR_SIZE1:\n"
     "        case TOK_VECTOR_SIZE2:\n"
     "            skip('(');\n"
     "            n = expr_const();\n"
     "            if (n <= 0)\n"
     "                tcc_error(\"vector_size must be a positive number of bytes\");\n"
     "            ad->vector_size = n;\n"
     "            skip(')');\n"
     "            break;\n"
     "        case TOK_PACKED1:\n"
     "        case TOK_PACKED2:\n"
     "            ad->a.packed = 1;\n"
     "            break;",
     1),
    # 12. tccgen.c: type_size handles vector types.
    ("tccgen.c",
     "ST_FUNC int type_size(CType *type, int *a)\n"
     "{\n"
     "    Sym *s;\n"
     "    int bt;\n"
     "\n"
     "    bt = type->t & VT_BTYPE;",
     "ST_FUNC int type_size(CType *type, int *a)\n"
     "{\n"
     "    Sym *s;\n"
     "    int bt;\n"
     "\n"
     "    if (type->t & VT_VECTOR) {\n"
     "        /* vector type: ref->type is the element type, ref->c the\n"
     "           number of elements (the array convention) */\n"
     "        int ts = type_size(&type->ref->type, a);\n"
     "        int n = ts * type->ref->c;\n"
     "        *a = n < 16 ? n : 16;\n"
     "        return n;\n"
     "    }\n"
     "    bt = type->t & VT_BTYPE;",
     1),
    # 13. tccgen.c: make_vector_type helper, before vpush_type_size.
    ("tccgen.c",
     "/* push type size as known at runtime time on top of value stack. Put\n"
     "   alignment at 'a' */",
     "/* Transform the scalar type in 'type' into a vector type of\n"
     "   'nbytes' total size (GCC __attribute__((vector_size(N)))).\n"
     "   The element type is kept in type->ref->type and the element\n"
     "   count in type->ref->c, the same convention arrays use. */\n"
     "static void make_vector_type(CType *type, int nbytes)\n"
     "{\n"
     "    CType elem;\n"
     "    int esize, align, bt;\n"
     "    bt = type->t & VT_BTYPE;\n"
     "    if (bt == VT_PTR || bt == VT_FUNC || bt == VT_STRUCT ||\n"
     "        bt == VT_VOID || (type->t & VT_ARRAY) || (type->t & VT_VECTOR))\n"
     "        tcc_error(\"vector_size applied to a non-scalar type\");\n"
     "    elem = *type;\n"
     "    elem.t &= ~(VT_STORAGE | VT_ARRAY | VT_VLA);\n"
     "    esize = type_size(&elem, &align);\n"
     "    if (esize <= 0 || nbytes % esize != 0 ||\n"
     "        (nbytes & (nbytes - 1)) != 0)\n"
     "        tcc_error(\"vector_size(%d) is not a power-of-two multiple of the element size\", nbytes);\n"
     "    type->ref = sym_push(SYM_FIELD, &elem, 0, nbytes / esize);\n"
     "    type->t = (type->t & ~VT_STRUCT_MASK) | VT_VECTOR;\n"
     "}\n"
     "\n"
     "/* push type size as known at runtime time on top of value stack. Put\n"
     "   alignment at 'a' */",
     1),
    # 14. tccgen.c: R_RET / R2_RET know about vector values.
    ("tccgen.c",
     "static int R_RET(int t)\n"
     "{\n"
     "    if (!is_float(t))",
     "static int R_RET(int t)\n"
     "{\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "    if (t & VT_VECTOR)\n"
     "        return WREG_VEC; /* vector values return in vector slot 0 */\n"
     "#endif\n"
     "    if (!is_float(t))",
     1),
    ("tccgen.c",
     "static int R2_RET(int t)\n"
     "{\n"
     "    t &= VT_BTYPE;",
     "static int R2_RET(int t)\n"
     "{\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "    if (t & VT_VECTOR)\n"
     "        return VT_CONST; /* a vector is a single v128 value, not a pair */\n"
     "#endif\n"
     "    t &= VT_BTYPE;",
     1),
    # 15. tccgen.c: save_reg_upstack must not try to spill v128 slots
    #     through the scalar store path (wasm locals survive calls;
    #     wasm-gen spills slot residents itself around calls).
    ("tccgen.c",
     "    if ((r &= VT_VALMASK) >= VT_CONST)\n"
     "        return;\n"
     "    if (nocode_wanted)\n"
     "        return;",
     "    if ((r &= VT_VALMASK) >= VT_CONST)\n"
     "        return;\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "    if (r >= WREG_VEC && r < WREG_VEC + 4)\n"
     "        return; /* v128 slots are wasm locals, not TCC registers */\n"
     "#endif\n"
     "    if (nocode_wanted)\n"
     "        return;",
     1),
    # 16. tccgen.c: gen_cast handles vector types (same-size cast is
    #     a bitcast, as in GCC; element conversions are the headers'
    #     job via the wasm vop1 builtin).
    ("tccgen.c",
     "    /* bitfields first get cast to ints */\n"
     "    if (vtop->type.t & VT_BITFIELD)\n"
     "        gv(RC_INT);\n",
     "    /* bitfields first get cast to ints */\n"
     "    if (vtop->type.t & VT_BITFIELD)\n"
     "        gv(RC_INT);\n"
     "\n"
     "    if ((type->t & VT_VECTOR) || (vtop->type.t & VT_VECTOR)) {\n"
     "        int a1, a2;\n"
     "        if ((type->t & VT_BTYPE) == VT_VOID) {\n"
     "            vpop(), vpushi(0);\n"
     "            goto done;\n"
     "        }\n"
     "        if (!(type->t & VT_VECTOR) || !(vtop->type.t & VT_VECTOR) ||\n"
     "            type_size(type, &a1) != type_size(&vtop->type, &a2))\n"
     "            cast_error(&vtop->type, type);\n"
     "        /* a cast between vector types of the same total size is a\n"
     "           bitcast (GCC semantics) */\n"
     "        goto done;\n"
     "    }\n",
     1),
    # 17. tccgen.c: vstore handles vector destinations.
    ("tccgen.c",
     "ST_FUNC void vstore(void)\n"
     "{\n"
     "    int sbt, dbt, ft, r, size, align, bit_size, bit_pos, delayed_cast;\n"
     "\n"
     "    ft = vtop[-1].type.t;",
     "ST_FUNC void vstore(void)\n"
     "{\n"
     "    int sbt, dbt, ft, r, size, align, bit_size, bit_pos, delayed_cast;\n"
     "\n"
     "    if (vtop[-1].type.t & VT_VECTOR) {\n"
     "        /* vector assignment: bitcast-convert the value, load it\n"
     "           into a vector slot and store it */\n"
     "        gen_cast(&vtop[-1].type);\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "        gen_vec_gv();\n"
     "#else\n"
     "        tcc_error(\"vector assignment is not supported for this target\");\n"
     "#endif\n"
     "        if ((vtop[-1].r & VT_VALMASK) == VT_LLOCAL) {\n"
     "            SValue sv;\n"
     "            r = get_reg(RC_INT);\n"
     "            sv.type.t = VT_PTRDIFF_T;\n"
     "            sv.r = VT_LOCAL | VT_LVAL;\n"
     "            sv.c.i = vtop[-1].c.i;\n"
     "            sv.sym = NULL;\n"
     "            load(r, &sv);\n"
     "            vtop[-1].r = r | VT_LVAL;\n"
     "        }\n"
     "        r = vtop->r & VT_VALMASK;\n"
     "        store(r, vtop - 1);\n"
     "        vswap();\n"
     "        vtop--; /* NOT vpop() because on x86 it would flush the fp stack */\n"
     "        return;\n"
     "    }\n"
     "\n"
     "    ft = vtop[-1].type.t;",
     1),
    # 18. tccgen.c: gfunc_return handles vector return values.
    ("tccgen.c",
     "static void gfunc_return(CType *func_type)\n"
     "{\n"
     "    if ((func_type->t & VT_BTYPE) == VT_STRUCT) {",
     "static void gfunc_return(CType *func_type)\n"
     "{\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "    if (func_type->t & VT_VECTOR) {\n"
     "        /* vector return value: move it into vector slot 0, the\n"
     "           wasm return slot (see wasm-gen.c gfunc_epilog) */\n"
     "        gen_vec_ret();\n"
     "        vtop--; /* NOT vpop() because on x86 it would flush the fp stack */\n"
     "        return;\n"
     "    }\n"
     "#endif\n"
     "    if ((func_type->t & VT_BTYPE) == VT_STRUCT) {",
     1),
    # 19. tccgen.c: gen_op_vector + its dispatch in gen_op.
    ("tccgen.c",
     "/* generic gen_op: handles types problems */\n"
     "ST_FUNC void gen_op(int op)",
     "/* check that two vector types have the same layout (total size,\n"
     "   lane count, lane floatness) */\n"
     "static int vec_same_layout(CType *a, CType *b)\n"
     "{\n"
     "    int a1, a2;\n"
     "    if (type_size(a, &a1) != type_size(b, &a2))\n"
     "        return 0;\n"
     "    if (a->ref->c != b->ref->c)\n"
     "        return 0;\n"
     "    return is_float(a->ref->type.t) == is_float(b->ref->type.t);\n"
     "}\n"
     "\n"
     "/* binary operation on vector operands (vtop[-1], vtop[0]).\n"
     "   Shape rules follow GCC: both operands vectors of the same\n"
     "   layout, except shifts, whose right operand may be a scalar\n"
     "   count.  The backend hook gen_opv consumes both operands and\n"
     "   leaves the result in vtop[-1]'s slot. */\n"
     "static void gen_op_vector(int op, int op_class)\n"
     "{\n"
     "    CType *lt = &vtop[-1].type, *rt = &vtop[0].type;\n"
     "    int lvec = (lt->t & VT_VECTOR) != 0;\n"
     "    int rvec = (rt->t & VT_VECTOR) != 0;\n"
     "    CType res;\n"
     "    if (op_class == SHIFT_OP) {\n"
     "        if (!lvec)\n"
     "            tcc_error(\"vector shift needs a vector left operand\");\n"
     "        if (rvec && !vec_same_layout(lt, rt))\n"
     "            tcc_error(\"vector operands have different shapes\");\n"
     "        if (!rvec && !is_integer_btype(rt->t & VT_BTYPE))\n"
     "            tcc_error(\"vector shift count must be an integer\");\n"
     "    } else {\n"
     "        if (!lvec || !rvec)\n"
     "            tcc_error(\"binary operation between vector and scalar is not supported\");\n"
     "        if (!vec_same_layout(lt, rt))\n"
     "            tcc_error(\"vector operands have different shapes\");\n"
     "    }\n"
     "    /* unsigned variants, mirroring gen_op's rewriting */\n"
     "    if ((lt->t | (rvec ? rt->t : 0)) & VT_UNSIGNED) {\n"
     "        if (op == '/') op = TOK_UDIV;\n"
     "        else if (op == '%') op = TOK_UMOD;\n"
     "        else if (op == TOK_SAR) op = TOK_SHR;\n"
     "        else if (op == TOK_LT) op = TOK_ULT;\n"
     "        else if (op == TOK_GT) op = TOK_UGT;\n"
     "        else if (op == TOK_LE) op = TOK_ULE;\n"
     "        else if (op == TOK_GE) op = TOK_UGE;\n"
     "    }\n"
     "    res = *lt;\n"
     "    if (op_class == CMP_OP) {\n"
     "        /* the result is a vector of signed integers with the same\n"
     "           lane size and count as the operands */\n"
     "        CType elem = lt->ref->type;\n"
     "        int esize, align;\n"
     "        esize = type_size(&elem, &align);\n"
     "        elem.t = esize == 8 ? VT_LLONG : esize == 2 ? VT_SHORT :\n"
     "                 esize == 1 ? VT_BYTE : VT_INT;\n"
     "        elem.ref = NULL;\n"
     "        res.ref = sym_push(SYM_FIELD, &elem, 0, lt->ref->c);\n"
     "        res.t = (lt->t & ~(VT_BTYPE | VT_UNSIGNED)) | (elem.t & VT_BTYPE);\n"
     "    }\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "    gen_opv(op);\n"
     "#else\n"
     "    tcc_error(\"vector operations are not supported for this target\");\n"
     "#endif\n"
     "    vtop->type = res;\n"
     "}\n"
     "\n"
     "/* generic gen_op: handles types problems */\n"
     "ST_FUNC void gen_op(int op)",
     1),
    ("tccgen.c",
     "    } else if (!combine_types(&combtype, vtop - 1, vtop, op_class)) {",
     "    } else if ((t1 & VT_VECTOR) || (t2 & VT_VECTOR)) {\n"
     "        gen_op_vector(op, op_class);\n"
     "    } else if (!combine_types(&combtype, vtop - 1, vtop, op_class)) {",
     1),
    # 20. tccgen.c: the wasm vector builtins, parsed in unary_expr
    #     next to the other __builtin cases.
    ("tccgen.c",
     "    case TOK_builtin_expect:",
     "#ifdef TCC_TARGET_WASM32\n"
     "    case TOK_wasm_vop1: {\n"
     "        /* __builtin_wasm_vop1(opc, vec, restype) */\n"
     "        int opc;\n"
     "        CType rt;\n"
     "        next();\n"
     "        skip('(');\n"
     "        opc = expr_const();\n"
     "        skip(',');\n"
     "        expr_eq();\n"
     "        skip(',');\n"
     "        parse_type(&rt);\n"
     "        skip(')');\n"
     "        gen_vec_op1(opc, &rt);\n"
     "        break;\n"
     "    }\n"
     "    case TOK_wasm_vop2: {\n"
     "        /* __builtin_wasm_vop2(opc, vec, vec, restype) */\n"
     "        int opc;\n"
     "        CType rt;\n"
     "        next();\n"
     "        skip('(');\n"
     "        opc = expr_const();\n"
     "        skip(',');\n"
     "        expr_eq();\n"
     "        skip(',');\n"
     "        expr_eq();\n"
     "        skip(',');\n"
     "        parse_type(&rt);\n"
     "        skip(')');\n"
     "        gen_vec_op2(opc, &rt);\n"
     "        break;\n"
     "    }\n"
     "    case TOK_wasm_splat: {\n"
     "        /* __builtin_wasm_splat(opc, scalar, restype) */\n"
     "        int opc;\n"
     "        CType rt;\n"
     "        next();\n"
     "        skip('(');\n"
     "        opc = expr_const();\n"
     "        skip(',');\n"
     "        expr_eq();\n"
     "        skip(',');\n"
     "        parse_type(&rt);\n"
     "        skip(')');\n"
     "        if (!(rt.t & VT_VECTOR))\n"
     "            tcc_error(\"__builtin_wasm_splat result type is not a vector type\");\n"
     "        gen_assign_cast(&rt.ref->type);\n"
     "        gen_vec_splat(opc, &rt);\n"
     "        break;\n"
     "    }\n"
     "    case TOK_wasm_extract: {\n"
     "        /* __builtin_wasm_extract(opc, vec, lane, restype) */\n"
     "        int opc, lane;\n"
     "        CType rt;\n"
     "        next();\n"
     "        skip('(');\n"
     "        opc = expr_const();\n"
     "        skip(',');\n"
     "        expr_eq();\n"
     "        skip(',');\n"
     "        lane = expr_const();\n"
     "        skip(',');\n"
     "        parse_type(&rt);\n"
     "        skip(')');\n"
     "        gen_vec_extract(opc, lane, &rt);\n"
     "        break;\n"
     "    }\n"
     "    case TOK_wasm_replace: {\n"
     "        /* __builtin_wasm_replace(opc, vec, lane, scalar) */\n"
     "        int opc, lane;\n"
     "        CType vt;\n"
     "        next();\n"
     "        skip('(');\n"
     "        opc = expr_const();\n"
     "        skip(',');\n"
     "        expr_eq();\n"
     "        vt = vtop->type;\n"
     "        skip(',');\n"
     "        lane = expr_const();\n"
     "        skip(',');\n"
     "        expr_eq();\n"
     "        skip(')');\n"
     "        if (!(vt.t & VT_VECTOR))\n"
     "            tcc_error(\"__builtin_wasm_replace on a non-vector value\");\n"
     "        gen_assign_cast(&vt.ref->type);\n"
     "        gen_vec_replace(opc, lane);\n"
     "        break;\n"
     "    }\n"
     "    case TOK_wasm_shuffle: {\n"
     "        /* __builtin_wasm_shuffle(vec, vec, l0, ..., l15) */\n"
     "        int lanes[16], i;\n"
     "        next();\n"
     "        skip('(');\n"
     "        expr_eq();\n"
     "        skip(',');\n"
     "        expr_eq();\n"
     "        for (i = 0; i < 16; i++) {\n"
     "            skip(',');\n"
     "            lanes[i] = expr_const();\n"
     "        }\n"
     "        skip(')');\n"
     "        gen_vec_shuffle(lanes);\n"
     "        break;\n"
     "    }\n"
     "#endif\n"
     "    case TOK_builtin_expect:",
     1),
    # 21. tccgen.c: unary operators on vectors.
    ("tccgen.c",
     "    case '-':\n"
     "        next();\n"
     "        unary();\n"
     "\tif (is_float(vtop->type.t)) {",
     "    case '-':\n"
     "        next();\n"
     "        unary();\n"
     "        if (vtop->type.t & VT_VECTOR) {\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "            gen_vec_neg();\n"
     "#else\n"
     "            tcc_error(\"unary '-' on a vector is not supported for this target\");\n"
     "#endif\n"
     "            break;\n"
     "        }\n"
     "\tif (is_float(vtop->type.t)) {",
     1),
    ("tccgen.c",
     "    case '~':\n"
     "        next();\n"
     "        unary();\n"
     "        vpushi(-1);\n"
     "        gen_op('^');\n"
     "        break;",
     "    case '~':\n"
     "        next();\n"
     "        unary();\n"
     "        if (vtop->type.t & VT_VECTOR) {\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "            gen_vec_not();\n"
     "#else\n"
     "            tcc_error(\"unary '~' on a vector is not supported for this target\");\n"
     "#endif\n"
     "        } else {\n"
     "            vpushi(-1);\n"
     "            gen_op('^');\n"
     "        }\n"
     "        break;",
     1),
    ("tccgen.c",
     "    case '!':\n"
     "        next();\n"
     "        unary();\n"
     "        gen_test_zero(TOK_EQ);\n"
     "        break;",
     "    case '!':\n"
     "        next();\n"
     "        unary();\n"
     "        if (vtop->type.t & VT_VECTOR)\n"
     "            tcc_error(\"unary '!' on a vector is not supported\");\n"
     "        gen_test_zero(TOK_EQ);\n"
     "        break;",
     1),
    # 22. tccgen.c: vector subscript in postfix_expr.  The element
    #     lvalue lives in the vector's memory home, so the vector
    #     operand must be an lvalue; the rest is the array path
    #     (address + scaled index + indir) unchanged.
    ("tccgen.c",
     "        } else if (tok == '[') {\n"
     "            next();\n"
     "            gexpr();\n"
     "            gen_op('+');\n"
     "            indir();\n"
     "            skip(']');",
     "        } else if (tok == '[') {\n"
     "            next();\n"
     "            if (vtop->type.t & VT_VECTOR) {\n"
     "                CType etype;\n"
     "                if (!(vtop->r & VT_LVAL))\n"
     "                    tcc_error(\"subscript of a vector rvalue is not supported\");\n"
     "                etype = vtop->type.ref->type;\n"
     "                vtop->type = etype;\n"
     "                mk_pointer(&vtop->type);\n"
     "                gaddrof();\n"
     "            }\n"
     "            gexpr();\n"
     "            gen_op('+');\n"
     "            indir();\n"
     "            skip(']');",
     1),
    # 23. tccgen.c: apply vector_size at the end of decl()'s
    #     declarator processing (covers typedefs, variables and
    #     function declarations; make_vector_type rejects the rest).
    ("tccgen.c",
     "#ifdef TCC_TARGET_PE\n"
     "            pe_check_linkage(&type, &ad);\n"
     "#endif\n"
     "            if (tok == '{') {",
     "#ifdef TCC_TARGET_PE\n"
     "            pe_check_linkage(&type, &ad);\n"
     "#endif\n"
     "            if (ad.vector_size)\n"
     "                make_vector_type(&type, ad.vector_size);\n"
     "            if (tok == '{') {",
     1),
    # 24. tccgen.c: vector_size on structure fields.
    ("tccgen.c",
     "                    type1 = btype;\n"
     "                    if (tok != ':') {\n"
     "\t\t\tif (tok != ';')\n"
     "                            type_decl(&type1, &ad1, &v, TYPE_DIRECT);",
     "                    type1 = btype;\n"
     "                    if (tok != ':') {\n"
     "\t\t\tif (tok != ';')\n"
     "                            type_decl(&type1, &ad1, &v, TYPE_DIRECT);\n"
     "                        if (ad1.vector_size)\n"
     "                            make_vector_type(&type1, ad1.vector_size);",
     1),
    # 25. tccgen.c: brace initializers for vectors take the array
    #     path (same ref convention); a whole-value initializer of
    #     the same vector type goes to one_elem instead.
    ("tccgen.c",
     "    if (type->t & VT_ARRAY) {\n"
     "        no_oblock = 1;",
     "    if ((type->t & VT_ARRAY) || (type->t & VT_VECTOR)) {\n"
     "        if ((type->t & VT_VECTOR) && (flags & DIF_HAVE_ELEM)\n"
     "            && is_compatible_unqualified_types(type, &vtop->type))\n"
     "            goto one_elem;\n"
     "        no_oblock = 1;",
     1),
    ("tccgen.c",
     "\t\tif (type->t & VT_ARRAY) {\n"
     "\t\t    ++indexsym.c;",
     "\t\tif ((type->t & VT_ARRAY) || (type->t & VT_VECTOR)) {\n"
     "\t\t    ++indexsym.c;",
     1),
    # 26. tccgen.c: decl_designator's element addressing must take
    #     the array path for vectors too (otherwise it walks the
    #     index symbol as if it were a struct field list).
    ("tccgen.c",
     "    no_designator:\n"
     "        if (type->t & VT_ARRAY) {",
     "    no_designator:\n"
     "        if ((type->t & VT_ARRAY) || (type->t & VT_VECTOR)) {",
     1),
    # 4. libtcc.c: ONE_SOURCE include chain (libtcc.c:52-55).
    ("libtcc.c",
     "#elif defined(TCC_TARGET_RISCV64)\n"
     "#include \"riscv64-gen.c\"\n"
     "#include \"riscv64-link.c\"\n"
     "#include \"riscv64-asm.c\"",
     "#elif defined(TCC_TARGET_RISCV64)\n"
     "#include \"riscv64-gen.c\"\n"
     "#include \"riscv64-link.c\"\n"
     "#include \"riscv64-asm.c\"\n"
     "#elif defined(TCC_TARGET_WASM32)\n"
     "#include \"wasm-gen.c\"\n"
     "#include \"wasm-link.c\"",
     1),
    # 5. tccelf.c: dispatch tcc_output_file to the wasm writer
    #    (tccelf.c:3223).
    ("tccelf.c",
     "LIBTCCAPI int tcc_output_file(TCCState *s, const char *filename)\n"
     "{\n"
     "    s->nb_errors = 0;",
     "LIBTCCAPI int tcc_output_file(TCCState *s, const char *filename)\n"
     "{\n"
     "    s->nb_errors = 0;\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "    return wasm_output_file(s, filename);\n"
     "#endif",
     1),
    # 5b. libtcc.c: wasm-specific -run error in the cross-compiler
    #     branch of TCC_OPTION_run (upstream text is generic).
    ("libtcc.c",
     "            return tcc_error_noabort(\"-run is not available in a cross compiler\");",
     "#ifdef TCC_TARGET_WASM32\n"
     "            return tcc_error_noabort(\"wasm: -run is not supported for the wasm target; \"\n"
     "                \"instantiate the generated module with a host wasm engine\");\n"
     "#else\n"
     "            return tcc_error_noabort(\"-run is not available in a cross compiler\");\n"
     "#endif",
     1),
    # 6. Makefile: cross-compiler definitions (Makefile:119, 136-137,
    #    171-174, 219).
    ("Makefile",
     "DEF-riscv64        = -DTCC_TARGET_RISCV64",
     "DEF-riscv64        = -DTCC_TARGET_RISCV64\n"
     "DEF-wasm32         = -DTCC_TARGET_WASM32",
     1),
    ("Makefile",
     "TCC_X += riscv64 arm64-osx",
     "TCC_X += riscv64 arm64-osx\n"
     "TCC_X += wasm32",
     1),
    ("Makefile",
     "riscv64_FILES = $(CORE_FILES) riscv64-gen.c riscv64-link.c riscv64-asm.c",
     "riscv64_FILES = $(CORE_FILES) riscv64-gen.c riscv64-link.c riscv64-asm.c\n"
     "wasm32_FILES = $(CORE_FILES) wasm-gen.c wasm-link.c",
     1),
]

for fname, old, new, count in PATCHES:
    p = root / fname
    text = p.read_text()
    n = text.count(old)
    if n != count:
        print(f"PATCH FAILED: {fname}: pattern found {n} times, expected {count}:\n{old[:120]}...")
        sys.exit(1)
    p.write_text(text.replace(old, new))
    print(f"patched {fname}: {old.splitlines()[0][:70]}")

print("all patches applied")
