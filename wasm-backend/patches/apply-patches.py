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
     "ST_FUNC void wasm_registry_state_deleted(TCCState *s1);\n"
     "ST_FUNC void gen_opv(int op);\n"
     "ST_FUNC void gen_vec_gv(void);\n"
     "ST_FUNC void gen_vec_home(void);\n"
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
    # 14. tccgen.c: R_RET / R2_RET know about vector values.
    ("tccgen.c",
     "static int R_RET(int t)\n"
     "{\n"
     "#if defined TCC_TARGET_ARM64 || defined TCC_TARGET_X86_64\n"
     "    /* a vector returns in the first FP/SIMD register whatever its\n"
     "       element type (the AAPCS64 FP/SIMD class on arm64, the SSE\n"
     "       class on x86-64) */\n"
     "    if (t & VT_VECTOR)\n"
     "        return REG_FRET;\n"
     "#endif\n"
     "    if (!is_float(t))",
     "static int R_RET(int t)\n"
     "{\n"
     "#if defined TCC_TARGET_ARM64 || defined TCC_TARGET_X86_64\n"
     "    /* a vector returns in the first FP/SIMD register whatever its\n"
     "       element type (the AAPCS64 FP/SIMD class on arm64, the SSE\n"
     "       class on x86-64) */\n"
     "    if (t & VT_VECTOR)\n"
     "        return REG_FRET;\n"
     "#endif\n"
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
    # 5c. libtcc.c: tcc_delete tells the wasm backend when a state
    #     dies, so a registry cycle owned by that state is drained
    #     even when the state never output (its address may be reused
    #     by the next tcc_new, so the backend cannot rely on pointer
    #     identity alone to notice the takeover).
    ("libtcc.c",
     "LIBTCCAPI void tcc_delete(TCCState *s1)\n"
     "{\n"
     "    /* free sections */",
     "LIBTCCAPI void tcc_delete(TCCState *s1)\n"
     "{\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "    wasm_registry_state_deleted(s1);\n"
     "#endif\n"
     "    /* free sections */",
     1),
    # 7. tcc.h: TCCState fields for the wasm dylink output flavor
    #    (-wasm-dylink=main|side) and its recorded dependency list
    #    (--dylink-need=<path>), wasm builds only.
    ("tcc.h",
     "    /* output type, see TCC_OUTPUT_XXX */\n"
     "    int output_type;",
     "    /* output type, see TCC_OUTPUT_XXX */\n"
     "    int output_type;\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "    /* wasm dylink output flavor: 0 = standalone, 1 = main, 2 = side */\n"
     "    int wasm_dylink_flavor;\n"
     "    /* dependency paths recorded by --dylink-need=, verbatim */\n"
     "    char **wasm_dylink_needs;\n"
     "    int nb_wasm_dylink_needs;\n"
     "#endif",
     1),
    # 8. libtcc.c: option ids, table entries and parser handling for
    #    -wasm-dylink=main|side and --dylink-need=<path> (wasm only).
    ("libtcc.c",
     "    TCC_OPTION_install_name,\n"
     "    TCC_OPTION_compatibility_version ,\n"
     "    TCC_OPTION_current_version,\n"
     "};",
     "    TCC_OPTION_install_name,\n"
     "    TCC_OPTION_compatibility_version ,\n"
     "    TCC_OPTION_current_version,\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "    TCC_OPTION_wasm_dylink,\n"
     "    TCC_OPTION_dylink_need,\n"
     "#endif\n"
     "};",
     1),
    ("libtcc.c",
     "    { \"traditional\", 0, 0 },\n"
     "    { NULL, 0, 0 },",
     "    { \"traditional\", 0, 0 },\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "    { \"wasm-dylink\", TCC_OPTION_wasm_dylink, TCC_OPTION_HAS_ARG },\n"
     "    { \"-dylink-need\", TCC_OPTION_dylink_need, TCC_OPTION_HAS_ARG },\n"
     "#endif\n"
     "    { NULL, 0, 0 },",
     1),
    ("libtcc.c",
     "        case TCC_OPTION_pthread:\n"
     "            s->option_pthread = 1;\n"
     "            break;",
     "        case TCC_OPTION_pthread:\n"
     "            s->option_pthread = 1;\n"
     "            break;\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "        case TCC_OPTION_wasm_dylink:\n"
     "            if (*optarg == '=') ++optarg;\n"
     "            if (!strcmp(optarg, \"main\"))\n"
     "                s->wasm_dylink_flavor = 1;\n"
     "            else if (!strcmp(optarg, \"side\"))\n"
     "                s->wasm_dylink_flavor = 2;\n"
     "            else\n"
     "                return tcc_error_noabort(\"invalid -wasm-dylink value '%s' \"\n"
     "                    \"(expected 'main' or 'side')\", optarg);\n"
     "            break;\n"
     "        case TCC_OPTION_dylink_need:\n"
     "            if (*optarg == '=') ++optarg;\n"
     "            s->wasm_dylink_needs = tcc_realloc(s->wasm_dylink_needs,\n"
     "                (s->nb_wasm_dylink_needs + 1) * sizeof(char*));\n"
     "            s->wasm_dylink_needs[s->nb_wasm_dylink_needs++] =\n"
     "                tcc_strdup(optarg);\n"
     "            break;\n"
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
    # 28. tccgen.c: the wasm backend converts to/from unsigned
    #     long long natively (i64 scratch local), like ARM64/RISCV64;
    #     route gen_cvt_itof1/gen_cvt_ftoi1 straight to the backend
    #     hooks instead of the libtcc1 helper functions
    #     (__floatundidf/__fixunsdfdi do not exist for this target).
    ("tccgen.c",
     "#if defined TCC_TARGET_ARM64 || defined TCC_TARGET_RISCV64 || defined TCC_TARGET_ARM\n"
     "#define gen_cvt_itof1 gen_cvt_itof",
     "#if defined TCC_TARGET_ARM64 || defined TCC_TARGET_RISCV64 || defined TCC_TARGET_ARM || defined TCC_TARGET_WASM32\n"
     "#define gen_cvt_itof1 gen_cvt_itof",
     1),
    ("tccgen.c",
     "#if defined TCC_TARGET_ARM64 || defined TCC_TARGET_RISCV64\n"
     "#define gen_cvt_ftoi1 gen_cvt_ftoi",
     "#if defined TCC_TARGET_ARM64 || defined TCC_TARGET_RISCV64 || defined TCC_TARGET_WASM32\n"
     "#define gen_cvt_ftoi1 gen_cvt_ftoi",
     1),
    # 29. tccgen.c: __builtin_va_start / __builtin_va_arg cases for
    #     the wasm target (the builtin switch has per-target cases
    #     only).  va_start mirrors the riscv64 case (the backend's
    #     gen_va_start pushes the area pointer, vstore assigns it);
    #     va_arg mirrors the arm64 case except the result type is
    #     left to gen_va_arg, which reports the promoted type for
    #     a float argument.
    ("tccgen.c",
     "    case TOK_builtin_va_arg: {\n"
     "\tparse_builtin_params(0, \"et\");\n"
     "\ttype = vtop->type;\n"
     "\tvpop();\n"
     "        //xx check types\n"
     "        gen_va_arg(&type);\n"
     "        vtop->type = type;\n"
     "        break;\n"
     "    }\n"
     "#endif",
     "    case TOK_builtin_va_arg: {\n"
     "\tparse_builtin_params(0, \"et\");\n"
     "\ttype = vtop->type;\n"
     "\tvpop();\n"
     "        //xx check types\n"
     "        gen_va_arg(&type);\n"
     "        vtop->type = type;\n"
     "        break;\n"
     "    }\n"
     "#endif\n"
     "#ifdef TCC_TARGET_WASM32\n"
     "    case TOK_builtin_va_start:\n"
     "        parse_builtin_params(0, \"ee\");\n"
     "        r = vtop->r & VT_VALMASK;\n"
     "        if (r == VT_LLOCAL)\n"
     "            r = VT_LOCAL;\n"
     "        if (r != VT_LOCAL)\n"
     "            tcc_error(\"__builtin_va_start expects a local variable\");\n"
     "        gen_va_start();\n"
     "        vstore();\n"
     "        break;\n"
     "    case TOK_builtin_va_arg: {\n"
     "        parse_builtin_params(0, \"et\");\n"
     "        type = vtop->type;\n"
     "        vpop();\n"
     "        gen_va_arg(&type);\n"
     "        break;\n"
     "    }\n"
     "#endif",
     1),
    # 30. tcctok.h: the va_start/va_arg builtin tokens are defined
    #     per target; add both to the existing wasm branch (which
    #     patch 10 added for the vector builtin tokens).
    ("tcctok.h",
     "#elif defined TCC_TARGET_WASM32\n"
     "     DEF(TOK_wasm_vop1, \"__builtin_wasm_vop1\")",
     "#elif defined TCC_TARGET_WASM32\n"
     "     DEF(TOK_builtin_va_start, \"__builtin_va_start\")\n"
     "     DEF(TOK_builtin_va_arg, \"__builtin_va_arg\")\n"
     "     DEF(TOK_wasm_vop1, \"__builtin_wasm_vop1\")",
     1),
    # 31. include/tccdefs.h: without a wasm branch the va_list
    #     predefs fall through to the i386 macros, which define
    #     __builtin_va_start/__builtin_va_arg as pointer arithmetic
    #     over the named-parameter area - a layout this backend
    #     does not have (parameters live in wasm locals, variadic
    #     arguments in a caller-materialised area).  Give the wasm
    #     target a plain pointer va_list and no macros, so the
    #     builtins reach gen_va_start/gen_va_arg; va_copy is a
    #     plain pointer copy under this layout.
    ("include/tccdefs.h",
     "#else /* __i386__ */\n"
     "    typedef char *__builtin_va_list;",
     "#elif defined __wasm32__\n"
     "    typedef char *__builtin_va_list;\n"
     "    #define __builtin_va_copy(dest, src) ((dest) = (src))\n"
     "#else /* __i386__ */\n"
     "    typedef char *__builtin_va_list;",
     1),
    # 32. conftest.c (the c2str tool): the platform-macro table
    #     has no __wasm32__ entry, so a column-1 `#elif defined
    #     __wasm32__' in tccdefs.h survives conversion unmapped
    #     and the host compiler drops the branch when embedding
    #     the predefs.  Map it to TCC_TARGET_WASM32 like the
    #     other targets.
    ("conftest.c",
     "    \"__riscv\",              \"TCC_TARGET_RISCV64\",\n",
     "    \"__riscv\",              \"TCC_TARGET_RISCV64\",\n"
     "    \"__wasm32__\",           \"TCC_TARGET_WASM32\",\n",
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
