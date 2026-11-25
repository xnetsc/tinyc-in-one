#ifdef TARGET_DEFS_ONLY

// Number of registers available to allocator:
#define NB_REGS 24 // x10-x17 aka a0-a7, f10-f17 aka fa0-fa7, xxx, ra, sp, s2-s6 (regcache only)
#define CONFIG_TCC_ASM

#define TREG_R(x) (x) // x = 0..7
#define TREG_F(x) (x + 8) // x = 0..7

// Register classes sorted from more general to more precise:
#define RC_INT (1 << 0)
#define RC_FLOAT (1 << 1)
#define RC_R(x) (1 << (2 + (x))) // x = 0..7
#define RC_F(x) (1 << (10 + (x))) // x = 0..7

#define RC_IRET (RC_R(0)) // int return register class
#define RC_IRE2 (RC_R(1)) // int 2nd return register class
#define RC_FRET (RC_F(0)) // float return register class

#define REG_IRET (TREG_R(0)) // int return register number
#define REG_IRE2 (TREG_R(1)) // int 2nd return register number
#define REG_FRET (TREG_F(0)) // float return register number

#define PTR_SIZE 8

#define LDOUBLE_SIZE 16
#define LDOUBLE_ALIGN 16

#define MAX_ALIGN 16

#define CHAR_IS_UNSIGNED

/* define if return values need to be extended explicitely
   at caller side (for interfacing with non-TCC compilers) */
#define PROMOTE_RET

/* Hardware s2-s6 have indices past the sp register in the
   allocator's numbering; ireg() maps the indices back to the
   hardware numbers.  They carry class 0 in reg_classes, so the
   expression allocator never hands them out. */
#define TREG_CS2 19
#define TREG_CS3 20
#define TREG_CS4 21
#define TREG_CS5 22
#define TREG_CS6 23

/* Dedicated registers for the tccgen.c local-variable register cache:
   s2-s6, callee-saved under the RISC-V ABI.  (s0 is this backend's
   frame base register and is not available.) */
#define REGCACHE_NREGS 5
#define REGCACHE_REGS regcache_regs
ST_DATA const int regcache_regs[REGCACHE_NREGS];
ST_FUNC void regcache_flush_all(void);
ST_FUNC void regcache_disable(void);
ST_FUNC int regcache_used_mask(void);

/* The generic vector layer (tcc.h) is implemented for this target
   by the RVV fixed-length hooks in the implementation section. */
#define TCC_VEC_HOOKS

#else
#define USING_GLOBALS
#include "tcc.h"
#include <assert.h>

#define UPPER(x)	(((unsigned)(x) + 0x800u) & 0xfffff000)
#define LOW_OVERFLOW(x)	UPPER(x)
#define SIGN7(x)	((((x) & 0xff) ^ 0x80) - 0x80)
#define SIGN11(x)	((((x) & 0xfff) ^ 0x800) - 0x800)

ST_DATA const char * const target_machine_defs =
    "__riscv\0"
    "__riscv_xlen 64\0"
    "__riscv_flen 64\0"
    "__riscv_div\0"
    "__riscv_mul\0"
    "__riscv_fdiv\0"
    "__riscv_fsqrt\0"
    "__riscv_float_abi_double\0"
    ;

#define XLEN 8

#define TREG_RA 17
#define TREG_SP 18

ST_DATA const int reg_classes[NB_REGS] = {
  RC_INT | RC_R(0),
  RC_INT | RC_R(1),
  RC_INT | RC_R(2),
  RC_INT | RC_R(3),
  RC_INT | RC_R(4),
  RC_INT | RC_R(5),
  RC_INT | RC_R(6),
  RC_INT | RC_R(7),
  RC_FLOAT | RC_F(0),
  RC_FLOAT | RC_F(1),
  RC_FLOAT | RC_F(2),
  RC_FLOAT | RC_F(3),
  RC_FLOAT | RC_F(4),
  RC_FLOAT | RC_F(5),
  RC_FLOAT | RC_F(6),
  RC_FLOAT | RC_F(7),
  0,
  1 << TREG_RA,
  1 << TREG_SP,
  0, /* s2: regcache only, never allocated */
  0, /* s3: regcache only, never allocated */
  0, /* s4: regcache only, never allocated */
  0, /* s5: regcache only, never allocated */
  0  /* s6: regcache only, never allocated */
};

#if defined(CONFIG_TCC_BCHECK)
static addr_t func_bound_offset;
static unsigned long func_bound_ind;
ST_DATA int func_bound_add_epilog;

/* RVV fixed-length vector support (bottom of this file) */
static void rvv_cfg_invalidate(void);
static void rvv_store_vec(SValue *sv);
#endif

static int ireg(int r)
{
    if (r == TREG_RA)
      return 1; // ra
    if (r == TREG_SP)
      return 2; // sp
    if (r >= TREG_CS2 && r <= TREG_CS6)
      return r - TREG_CS2 + 18; /* regcache index -> hardware s2-s6 */
    assert(r >= 0 && r < 8);
    return r + 10;  // tccrX --> aX == x(10+X)
}

static int is_ireg(int r)
{
    return (unsigned)r < 8 || r == TREG_RA || r == TREG_SP
        || (r >= TREG_CS2 && r <= TREG_CS6);
}

static int freg(int r)
{
    assert(r >= 8 && r < 16);
    return r - 8 + 10;  // tccfX --> faX == f(10+X)
}

static int is_freg(int r)
{
    return r >= 8 && r < 16;
}

ST_FUNC void o(unsigned int c)
{
    int ind1 = ind + 4;
    if (nocode_wanted)
        return;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(cur_text_section, ind1);
    write32le(cur_text_section->data + ind, c);
    ind = ind1;
}

static void EIu(uint32_t opcode, uint32_t func3,
               uint32_t rd, uint32_t rs1, uint32_t imm)
{
    o(opcode | (func3 << 12) | (rd << 7) | (rs1 << 15) | (imm << 20));
}

static void ER(uint32_t opcode, uint32_t func3,
               uint32_t rd, uint32_t rs1, uint32_t rs2, uint32_t func7)
{
    o(opcode | func3 << 12 | rd << 7 | rs1 << 15 | rs2 << 20 | func7 << 25);
}

static void EI(uint32_t opcode, uint32_t func3,
               uint32_t rd, uint32_t rs1, uint32_t imm)
{
    assert(! LOW_OVERFLOW(imm));
    EIu(opcode, func3, rd, rs1, imm);
}

static void ES(uint32_t opcode, uint32_t func3,
               uint32_t rs1, uint32_t rs2, uint32_t imm)
{
    assert(! LOW_OVERFLOW(imm));
    o(opcode | (func3 << 12) | ((imm & 0x1f) << 7) | (rs1 << 15)
      | (rs2 << 20) | ((imm >> 5) << 25));
}

// Patch all branches in list pointed to by t to branch to a:
ST_FUNC void gsym_addr(int t_, int a_)
{
    uint32_t t = t_;
    uint32_t a = a_;
    rvv_cfg_invalidate(); /* a label: control arrives from elsewhere */
    while (t) {
        unsigned char *ptr = cur_text_section->data + t;
        uint32_t next = read32le(ptr);
        uint32_t r = a - t, imm;
        if ((r + (1 << 21)) & ~((1U << 22) - 2))
          tcc_error("out-of-range branch chain");
        imm = (((r >> 12) &  0xff) << 12)
            | (((r >> 11) &     1) << 20)
            | (((r >>  1) & 0x3ff) << 21)
            | (((r >> 20) &     1) << 31);
        write32le(ptr, r == 4 ? 0x33 : 0x6f | imm); // nop || j imm
        t = next;
    }
}

static int load_symofs(int r, SValue *sv, int forstore, int *new_fc)
{
    int rr, doload = 0, large_addend = 0;
    int fc = sv->c.i, v = sv->r & VT_VALMASK;
    if (sv->r & VT_SYM) {
        Sym label = {0};
        assert(v == VT_CONST);
        if (sv->sym->type.t & VT_TLS) {
            /* TLS Local Exec model: lui + addi + add tp */
            rr = is_ireg(r) ? ireg(r) : 5;
            greloca(cur_text_section, sv->sym, ind,
                    R_RISCV_TPREL_HI20, sv->c.i);
            o(0x37 | (rr << 7));  // lui RR, 0 %tprel_hi(sym)
            greloca(cur_text_section, sv->sym, ind,
                    R_RISCV_TPREL_LO12_I, sv->c.i);
            EI(0x13, 0, rr, rr, 0); // addi RR, RR, 0 %tprel_lo(sym)
            ER(0x33, 0, rr, rr, 4, 0); // add RR, RR, tp
            *new_fc = 0;
            return rr;
        }
        if (sv->sym->type.t & VT_STATIC) { // XXX do this per linker relax
            greloca(cur_text_section, sv->sym, ind,
                    R_RISCV_PCREL_HI20, sv->c.i);
            *new_fc = 0;
        } else {
            if (LOW_OVERFLOW(fc)){
              large_addend = 1;
            }
            greloca(cur_text_section, sv->sym, ind,
                    R_RISCV_GOT_HI20, 0);
            doload = 1;
        }
        label.type.t = VT_VOID | VT_STATIC;
	if (!nocode_wanted)
            put_extern_sym(&label, cur_text_section, ind, 0);
        rr = is_ireg(r) ? ireg(r) : 5;
        o(0x17 | (rr << 7));   // auipc RR, 0 %pcrel_hi(sym)+addend
        greloca(cur_text_section, &label, ind,
                doload || !forstore
                  ? R_RISCV_PCREL_LO12_I : R_RISCV_PCREL_LO12_S, 0);
        if (doload) {
            EI(0x03, 3, rr, rr, 0); // ld RR, 0(RR)
            if (large_addend) {
                o(0x37 | (6 << 7) | UPPER(fc)); //lui t1, high(fc)
                ER(0x33, 0, rr, rr, 6, 0); // add RR, RR, t1
                *new_fc = SIGN11(fc);
            }
        }
    } else if (v == VT_LOCAL || v == VT_LLOCAL) {
        rr = 8; // s0
        if (fc != sv->c.i)
          tcc_error("unimp: store(giant local off) (0x%lx)", (long)sv->c.i);
        if (LOW_OVERFLOW(fc)) {
            rr = is_ireg(r) ? ireg(r) : 5; // t0
            o(0x37 | (rr << 7) | UPPER(fc)); //lui RR, upper(fc)
            ER(0x33, 0, rr, rr, 8, 0); // add RR, RR, s0
            *new_fc = SIGN11(fc);
        }
    } else
      tcc_error("uhh");
    return rr;
}

static void load_large_constant(int rr, int fc, uint32_t pi)
{
    if (fc < 0)
	pi++;
    o(0x37 | (rr << 7) | UPPER(pi)); // lui RR, up(up(fc))
    EI(0x13, 0, rr, rr, SIGN11(pi));   // addi RR, RR, lo(up(fc))
    EI(0x13, 1, rr, rr, 12); // slli RR, RR, 12
    EI(0x13, 0, rr, rr, SIGN11(((unsigned)fc + (1 << 19)) >> 20)); // addi RR, RR, up(lo(fc))
    EI(0x13, 1, rr, rr, 12); // slli RR, RR, 12
    EI(0x13, 0, rr, rr, SIGN11(((unsigned)fc + (1 << 7)) >> 8));  // addi RR, RR, lo1(lo(fc))
    EI(0x13, 1, rr, rr, 8); // slli RR, RR, 8
}

ST_FUNC void load(int r, SValue *sv)
{
    int fr = sv->r;
    int v = fr & VT_VALMASK;
    int rr = is_ireg(r) ? ireg(r) : freg(r);
    int fc = sv->c.i;
    int bt = sv->type.t & VT_BTYPE;
    int align, size;
    if (sv->type.t & VT_VECTOR)
        tcc_error("a vector value has no scalar register form on riscv64");
    if (fr & VT_LVAL) {
        int func3, opcode = is_freg(r) ? 0x07 : 0x03, br;
        size = type_size(&sv->type, &align);
        assert (!is_freg(r) || bt == VT_FLOAT || bt == VT_DOUBLE);
        if (bt == VT_PTR || bt == VT_FUNC) /* XXX should be done in generic code */
          size = PTR_SIZE;
        func3 = size == 1 ? 0 : size == 2 ? 1 : size == 4 ? 2 : 3;
        if (size < 4 && !is_float(sv->type.t) && (sv->type.t & VT_UNSIGNED))
          func3 |= 4;
        if (v == VT_LOCAL || (fr & VT_SYM)) {
            br = load_symofs(r, sv, 0, &fc);
        } else if (v < VT_CONST) {
            br = ireg(v);
            /*if (LOW_OVERFLOW(fc))
              tcc_error("unimp: load(large addend) (0x%x)", fc);*/
            fc = 0; // XXX store ofs in LVAL(reg)
        } else if (v == VT_LLOCAL) {
            br = load_symofs(r, sv, 0, &fc);
            EI(0x03, 3, rr, br, fc); // ld RR, fc(BR)
            br = rr;
            fc = 0;
        } else if (v == VT_CONST) {
            int64_t si = sv->c.i;
            si >>= 32;
            if (si != 0) {
		load_large_constant(rr, fc, si);
                fc = SIGN7(fc);
            } else {
                o(0x37 | (rr << 7) | UPPER(fc)); //lui RR, upper(fc)
                fc = SIGN11(fc);
	    }
            br = rr;
	} else {
            tcc_error("unimp: load(non-local lval)");
        }
        EI(opcode, func3, rr, br, fc); // l[bhwd][u] / fl[wd] RR, fc(BR)
    } else if (v == VT_CONST) {
        int rb = 0, do32bit = 8, zext = 0;
        if (is_float(sv->type.t) && bt != VT_LDOUBLE) {
            /* load float/double constant: move bit pattern from int reg */
            uint64_t val = sv->c.i;
            int is_dbl = bt == VT_DOUBLE;
            if (val == 0) {
                o(0x53 | (rr << 7) | ((unsigned)(0x78 | is_dbl) << 25));
                return;
            }
            if (is_dbl) {
                load_large_constant(6, (int)val, (int)(val >> 32));
            } else {
                if (LOW_OVERFLOW(fc))
                    o(0x37 | (6 << 7) | UPPER(fc)); // lui t1, upper
                EI(0x13 | 8, 0, 6, LOW_OVERFLOW(fc) ? 6 : 0, SIGN11(fc)); // addiw t1,...
            }
            o(0x53 | (rr << 7) | (6 << 15) | ((unsigned)(0x78 | is_dbl) << 25));
            return;
        }
        assert(is_ireg(r) || bt == VT_LDOUBLE);
        if (fr & VT_SYM) {
            rb = load_symofs(r, sv, 0, &fc);
            do32bit = 0;
        }
        if (do32bit && fc != sv->c.i) {
            int64_t si = sv->c.i;
            si >>= 32;
            if (si != 0) {
		load_large_constant(rr, fc, si);
                fc = SIGN7(fc);
                rb = rr;
                do32bit = 0;
            } else if (bt == VT_LLONG) {
                /* A 32bit unsigned constant for a 64bit type.
                   lui always sign extends, so we need to do an explicit zext.*/
                zext = 1;
            }
        }
        if (LOW_OVERFLOW(fc))
            o(0x37 | (rr << 7) | UPPER(fc)), rb = rr; //lui RR, upper(fc)
        if (fc || (rr != rb) || do32bit || (fr & VT_SYM))
          EI(0x13 | do32bit, 0, rr, rb, SIGN11(fc)); // addi[w] R, x0|R, FC
        if (zext) {
            EI(0x13, 1, rr, rr, 32); // slli RR, RR, 32
            EI(0x13, 5, rr, rr, 32); // srli RR, RR, 32
        }
    } else if (v == VT_LOCAL) {
        int br = load_symofs(r, sv, 0, &fc);
        assert(is_ireg(r));
        EI(0x13, 0, rr, br, fc); // addi R, s0, FC
    } else if (v < VT_CONST) { /* reg-reg */
        //assert(!fc); XXX support offseted regs
        if (is_freg(r) && is_freg(v))
          ER(0x53, 0, rr, freg(v), freg(v), bt == VT_DOUBLE ? 0x11 : 0x10); //fsgnj.[sd] RR, V, V == fmv.[sd] RR, V
        else if (is_ireg(r) && is_ireg(v)) {
          /* A 32-bit integer is held sign-extended to full width in
             this backend (loads use lw, arithmetic the w forms), so
             moving an int-typed value between registers must extend
             it again: a plain mv would leak the upper bits of a wider
             source value into int-typed consumers.  With this, a
             register move of an int is the exact equivalent of a
             store/load round trip through its frame slot. */
          if (bt == VT_INT)
            EI(0x13 | 8, 0, rr, ireg(v), 0); // addiw RR, V, 0 == sext.w RR, V
          else
            EI(0x13, 0, rr, ireg(v), 0); // addi RR, V, 0 == mv RR, V
        }
        else {
            int func7 = is_ireg(r) ? 0x70 : 0x78;
            size = type_size(&sv->type, &align);
            if (size == 8)
              func7 |= 1;
            assert(size == 4 || size == 8);
            o(0x53 | (rr << 7) | ((is_freg(v) ? freg(v) : ireg(v)) << 15)
              | ((unsigned)func7 << 25)); // fmv.{w.x, x.w, d.x, x.d} RR, VR
        }
    } else if (v == VT_CMP) {
        int op = vtop->cmp_op;
        int a = vtop->cmp_r & 0xff;
        int b = (vtop->cmp_r >> 8) & 0xff;
        int inv = 0;
        switch (op) {
            case TOK_ULT:
            case TOK_UGE:
            case TOK_ULE:
            case TOK_UGT:
            case TOK_LT:
            case TOK_GE:
            case TOK_LE:
            case TOK_GT:
                if (op & 1) { // remove [U]GE,GT
                    inv = 1;
                    op--;
                }
                if ((op & 7) == 6) { // [U]LE
                    int t = a; a = b; b = t;
                    inv ^= 1;
                }
                ER(0x33, (op > TOK_UGT) ? 2 : 3, rr, a, b, 0); // slt[u] d, a, b
                if (inv)
                  EI(0x13, 4, rr, rr, 1); // xori d, d, 1
                break;
            case TOK_NE:
            case TOK_EQ:
                if (rr != a || b)
                  ER(0x33, 0, rr, a, b, 0x20); // sub d, a, b
                if (op == TOK_NE)
                  ER(0x33, 3, rr, 0, rr, 0); // sltu d, x0, d == snez d,d
                else
                  EI(0x13, 3, rr, rr, 1); // sltiu d, d, 1 == seqz d,d
                break;
        }
    } else if ((v & ~1) == VT_JMP) {
        int t = v & 1;
        assert(is_ireg(r));
        EI(0x13, 0, rr, 0, t);      // addi RR, x0, t
        gjmp_addr(ind + 8);
        gsym(fc);
        EI(0x13, 0, rr, 0, t ^ 1);  // addi RR, x0, !t
    } else
      tcc_error("unimp: load(non-const)");
}

ST_FUNC void store(int r, SValue *sv)
{
    int fr = sv->r & VT_VALMASK;
    int rr, ptrreg;
    int fc = sv->c.i;
    int bt = sv->type.t & VT_BTYPE;
    int align, size;
    if (sv->type.t & VT_VECTOR) {
        /* vector values are memory-resident; the "register"
           argument names no register for them */
        rvv_store_vec(sv);
        return;
    }
    rr = is_ireg(r) ? ireg(r) : freg(r);
    size = type_size(&sv->type, &align);
    assert(!is_float(bt) || is_freg(r) || bt == VT_LDOUBLE);
    /* long doubles are in two integer registers, but the load/store
       primitives only deal with one, so do as if it's one reg.  */
    if (bt == VT_LDOUBLE)
      size = align = 8;
    if (bt == VT_STRUCT)
      tcc_error("unimp: store(struct)");
    if (size > 8)
      tcc_error("unimp: large sized store");
    assert(sv->r & VT_LVAL);
    if (fr == VT_LOCAL || (sv->r & VT_SYM)) {
        ptrreg = load_symofs(-1, sv, 1, &fc);
    } else if (fr < VT_CONST) {
        ptrreg = ireg(fr);
        /*if (LOW_OVERFLOW(fc))
          tcc_error("unimp: store(large addend) (0x%x)", fc);*/
        fc = 0; // XXX support offsets regs
    } else if (fr == VT_CONST) {
        int64_t si = sv->c.i;
        ptrreg = 8; // s0
        si >>= 32;
        if (si != 0) {
	    load_large_constant(ptrreg, fc, si);
            fc = SIGN7(fc);
        } else {
            o(0x37 | (ptrreg << 7) | UPPER(fc)); //lui RR, upper(fc)
            fc = SIGN11(fc);
	}
    } else
      tcc_error("implement me: %s(!local)", __FUNCTION__);
    ES(is_freg(r) ? 0x27 : 0x23,                          // fs... | s...
       size == 1 ? 0 : size == 2 ? 1 : size == 4 ? 2 : 3, // ... [wd] | [bhwd]
       ptrreg, rr, fc);                                   // RR, fc(base)
}

static void gcall_or_jmp(int docall)
{
    int tr = docall ? 1 : 5; // ra or t0
    if ((vtop->r & (VT_VALMASK | VT_LVAL)) == VT_CONST &&
        ((vtop->r & VT_SYM) && vtop->c.i == (int)vtop->c.i)) {
        /* constant symbolic case -> simple relocation */
        greloca(cur_text_section, vtop->sym, ind,
                R_RISCV_CALL_PLT, (int)vtop->c.i);
        o(0x17 | (tr << 7));   // auipc TR, 0 %call(func)
        EI(0x67, 0, tr, tr, 0);// jalr  TR, r(TR)
    } else if (vtop->r < VT_CONST) {
        int r = ireg(vtop->r);
        EI(0x67, 0, tr, r, 0);      // jalr TR, 0(R)
    } else {
        int r = TREG_RA;
        load(r, vtop);
        r = ireg(r);
        EI(0x67, 0, tr, r, 0);      // jalr TR, 0(R)
    }
}

#if defined(CONFIG_TCC_BCHECK)

static void gen_bounds_call(int v)
{
    Sym *sym = external_helper_sym(v);

    greloca(cur_text_section, sym, ind, R_RISCV_CALL_PLT, 0);
    o(0x17 | (1 << 7));   // auipc TR, 0 %call(func)
    EI(0x67, 0, 1, 1, 0); // jalr  TR, r(TR)
}

static void gen_bounds_prolog(void)
{
    /* leave some room for bound checking code */
    func_bound_offset = lbounds_section->data_offset;
    func_bound_ind = ind;
    func_bound_add_epilog = 0;
    o(0x00000013);  /* ld a0,#lbound section pointer */
    o(0x00000013);
    o(0x00000013);  /* nop -> call __bound_local_new */
    o(0x00000013);
}

static void gen_bounds_epilog(void)
{
    addr_t saved_ind;
    addr_t *bounds_ptr;
    Sym *sym_data;
    Sym label = {0};

    int offset_modified = func_bound_offset != lbounds_section->data_offset;

    if (!offset_modified && !func_bound_add_epilog)
        return;

    /* add end of table info */
    bounds_ptr = section_ptr_add(lbounds_section, sizeof(addr_t));
    *bounds_ptr = 0;

    sym_data = get_sym_ref(&char_pointer_type, lbounds_section,
                           func_bound_offset, PTR_SIZE);

    label.type.t = VT_VOID | VT_STATIC;
    /* generate bound local allocation */
    if (offset_modified) {
        saved_ind = ind;
        ind = func_bound_ind;
        put_extern_sym(&label, cur_text_section, ind, 0);
        greloca(cur_text_section, sym_data, ind, R_RISCV_GOT_HI20, 0);
        o(0x17 | (10 << 7));    // auipc a0, 0 %pcrel_hi(sym)+addend
        greloca(cur_text_section, &label, ind, R_RISCV_PCREL_LO12_I, 0);
        EI(0x03, 3, 10, 10, 0); // ld a0, 0(a0)
        gen_bounds_call(TOK___bound_local_new);
        ind = saved_ind;
        label.c = 0; /* force new local ELF symbol */
    }

    /* generate bound check local freeing */
    o(0xe02a1101); /* addi sp,sp,-32  sd   a0,0(sp)   */
    o(0xa82ae42e); /* sd   a1,8(sp)   fsd  fa0,16(sp) */
    put_extern_sym(&label, cur_text_section, ind, 0);
    greloca(cur_text_section, sym_data, ind, R_RISCV_GOT_HI20, 0);
    o(0x17 | (10 << 7));    // auipc a0, 0 %pcrel_hi(sym)+addend
    greloca(cur_text_section, &label, ind, R_RISCV_PCREL_LO12_I, 0);
    EI(0x03, 3, 10, 10, 0); // ld a0, 0(a0)
    gen_bounds_call(TOK___bound_local_delete);
    o(0x65a26502); /* ld   a0,0(sp)   ld   a1,8(sp)   */
    o(0x61052542); /* fld  fa0,16(sp) addi sp,sp,32   */
}
#endif

static void reg_pass_rec(CType *type, int *rc, int *fieldofs, int ofs)
{
    if ((type->t & VT_BTYPE) == VT_STRUCT) {
        Sym *f;
        if (type->ref->type.t == VT_UNION)
          rc[0] = -1;
        else for (f = type->ref->next; f; f = f->next)
          reg_pass_rec(&f->type, rc, fieldofs, ofs + f->c);
    } else if (type->t & VT_ARRAY) {
        if (type->ref->c < 0 || type->ref->c > 2)
          rc[0] = -1;
        else {
            int a, sz = type_size(&type->ref->type, &a);
            reg_pass_rec(&type->ref->type, rc, fieldofs, ofs);
            if (rc[0] > 2 || (rc[0] == 2 && type->ref->c > 1))
              rc[0] = -1;
            else if (type->ref->c == 2 && rc[0] && rc[1] == RC_FLOAT) {
              rc[++rc[0]] = RC_FLOAT;
              fieldofs[rc[0]] = ((ofs + sz) << 4)
                                | (type->ref->type.t & VT_BTYPE);
            } else if (type->ref->c == 2)
              rc[0] = -1;
        }
    } else if (rc[0] == 2 || rc[0] < 0 || (type->t & VT_BTYPE) == VT_LDOUBLE)
      rc[0] = -1;
    else if (!rc[0] || rc[1] == RC_FLOAT || is_float(type->t)) {
      rc[++rc[0]] = is_float(type->t) ? RC_FLOAT : RC_INT;
      fieldofs[rc[0]] = (ofs << 4) | ((type->t & VT_BTYPE) == VT_PTR ? VT_LLONG : type->t & VT_BTYPE);
    } else
      rc[0] = -1;
}

static void reg_pass(CType *type, int *prc, int *fieldofs, int named)
{
    prc[0] = 0;
    reg_pass_rec(type, prc, fieldofs, 0);
    if (prc[0] <= 0 || !named) {
        int align, size = type_size(type, &align);
        prc[0] = (size + 7) >> 3;
        prc[1] = prc[2] = RC_INT;
        fieldofs[1] = (0 << 4) | (size <= 1 ? VT_BYTE : size <= 2 ? VT_SHORT : size <= 4 ? VT_INT : VT_LLONG);
        fieldofs[2] = (8 << 4) | (size <= 9 ? VT_BYTE : size <= 10 ? VT_SHORT : size <= 12 ? VT_INT : VT_LLONG);
    }
}

ST_DATA const int regcache_regs[REGCACHE_NREGS] = {
    TREG_CS2, TREG_CS3, TREG_CS4, TREG_CS5, TREG_CS6
};

/* The prolog reserves REGCACHE_FRAME bytes of frame below the
   parameter area (register i of REGCACHE_REGS gets the slot at
   regcache_slot_base - 8*(i+1)) and REGCACHE_NREGS instruction slots
   at its end, which the epilog backpatches with the saves of the
   registers actually used (a save is a single sd; unused slots stay
   nop padding).  The reservation is made at the end of gfunc_prolog
   so the parameter home slots keep their stock offsets.  The slots
   sit just below the parameter area, so their offsets always fit
   the 12-bit load/store immediate. */
#define REGCACHE_FRAME (REGCACHE_NREGS * 8)
static unsigned long riscv_regcache_save_offset;
static int regcache_slot_base;

ST_FUNC void gfunc_call(int nb_args)
{
    int i, align, size, areg[2];
    int *info = tcc_malloc((nb_args + 1) * sizeof (int));
    int stack_adj = 0, tempspace = 0, stack_add, ofs, splitofs = 0;
    int old = (vtop[-nb_args].type.ref->f.func_type == FUNC_OLD);
    SValue *sv;
    Sym *sa;

#ifdef CONFIG_TCC_BCHECK
    int bc_save = tcc_state->do_bounds_check;
    if (tcc_state->do_bounds_check)
        gbound_args(nb_args);
#endif

    regcache_flush_all(); /* F3: no cached value crosses a call */
    areg[0] = 0; /* int arg regs */
    areg[1] = 8; /* float arg regs */
    sa = vtop[-nb_args].type.ref->next;
    for (i = 0; i < nb_args; i++) {
        int nregs, byref = 0, tempofs;
        int prc[3], fieldofs[3];
        sv = &vtop[1 + i - nb_args];
        sv->type.t &= ~VT_ARRAY; // XXX this should be done in tccgen.c
        size = type_size(&sv->type, &align);
        if (size > 16 || (sv->type.t & VT_VECTOR)) {
            if (align < XLEN)
              align = XLEN;
            tempspace = (tempspace + align - 1) & -align;
            tempofs = tempspace;
            tempspace += size;
            size = align = 8;
            byref = 64 | (tempofs << 7);
        }
        /* a byref argument (a vector, or anything over 16 bytes)
           travels as a plain pointer: classify it as one, or a
           float-element vector would be mistaken for a float
           scalar and its pointer loaded into an FP register */
        reg_pass(byref ? &char_pointer_type : &sv->type,
                 prc, fieldofs, old || sa != 0);
        if (!old && !sa && align == 2*XLEN && size <= 2*XLEN)
          areg[0] = (areg[0] + 1) & ~1;
        nregs = prc[0];
        if (size == 0)
            info[i] = 0;
        else if ((prc[1] == RC_INT && areg[0] >= 8)
            || (prc[1] == RC_FLOAT && areg[1] >= 16)
            || (nregs == 2 && prc[1] == RC_FLOAT && prc[2] == RC_FLOAT
                && areg[1] >= 15)
            || (nregs == 2 && prc[1] != prc[2]
                && (areg[1] >= 16 || areg[0] >= 8))) {
            info[i] = 32;
            if (align < XLEN)
              align = XLEN;
            stack_adj += (size + align - 1) & -align;
            if (!old && !sa) /* one vararg on stack forces the rest on stack */
              areg[0] = 8, areg[1] = 16;
        } else {
            info[i] = areg[prc[1] - 1]++;
            if (!byref)
              info[i] |= (fieldofs[1] & VT_BTYPE) << 12;
            assert(!(fieldofs[1] >> 4));
            if (nregs == 2) {
                if (prc[2] == RC_FLOAT || areg[0] < 8)
                  info[i] |= (1 + areg[prc[2] - 1]++) << 7;
                else {
                    info[i] |= 16;
                    stack_adj += 8;
                }
                if (!byref) {
                    assert((fieldofs[2] >> 4) < 2048);
                    info[i] |= fieldofs[2] << (12 + 4); // includes offset
                }
            }
        }
        info[i] |= byref;
        if (sa)
          sa = sa->next;
    }
    stack_adj = (stack_adj + 15) & -16;
    tempspace = (tempspace + 15) & -16;
    stack_add = stack_adj + tempspace;

    if (stack_add) {
        if (stack_add >= 0x800) {
            o(0x37 | (5 << 7) | UPPER(-stack_add)); //lui t0, upper(v)
            EI(0x13, 0, 5, 5, SIGN11(-stack_add)); // addi t0, t0, lo(v)
            ER(0x33, 0, 2, 2, 5, 0); // add sp, sp, t0
        }
        else
            EI(0x13, 0, 2, 2, -stack_add);   // addi sp, sp, -adj
        for (i = ofs = 0; i < nb_args; i++) {
            if (info[i] & (64 | 32)) {
                vrotb(nb_args - i);
                size = type_size(&vtop->type, &align);
                if (info[i] & 64) {
                    vset(&char_pointer_type, TREG_SP, 0);
                    vpushi(stack_adj + (info[i] >> 7));
                    gen_op('+');
                    vpushv(vtop); // this replaces the old argument
                    vrott(3);
                    indir();
                    vtop->type = vtop[-1].type;
                    vswap();
                    vstore();
                    vpop();
                    size = align = 8;
                }
                if (info[i] & 32) {
                    if (align < XLEN)
                      align = XLEN;
                    /* Once we support offseted regs we can do this:
                       vset(&vtop->type, TREG_SP | VT_LVAL, ofs);
                       to construct the lvalue for the outgoing stack slot,
                       until then we have to jump through hoops.  */
                    vset(&char_pointer_type, TREG_SP, 0);
                    ofs = (ofs + align - 1) & -align;
                    vpushi(ofs);
                    gen_op('+');
                    indir();
                    vtop->type = vtop[-1].type;
                    vswap();
                    vstore();
                    vtop->r = vtop->r2 = VT_CONST; // this arg is done
                    ofs += size;
                }
                vrott(nb_args - i);
            } else if (info[i] & 16) {
                assert(!splitofs);
                splitofs = ofs;
                ofs += 8;
            }
        }
    }
    for (i = 0; i < nb_args; i++) {
        int ii = info[nb_args - 1 - i], r = ii, r2 = r;
        if (!(r & 32)) {
            CType origtype;
            int loadt;
            r &= 15;
            r2 = r2 & 64 ? 0 : (r2 >> 7) & 31;
            assert(r2 <= 16);
            vrotb(i+1);
            origtype = vtop->type;
            size = type_size(&vtop->type, &align);
            if (size == 0)
                goto done;
            loadt = vtop->type.t & VT_BTYPE;
            if (loadt == VT_STRUCT) {
                loadt = (ii >> 12) & VT_BTYPE;
            }
            if (info[nb_args - 1 - i] & 16) {
                assert(!r2);
                r2 = 1 + TREG_RA;
            }
            if (loadt == VT_LDOUBLE) {
                assert(r2);
                r2--;
            } else if (r2) {
                test_lvalue();
                vpushv(vtop);
            }
            vtop->type.t = loadt | (vtop->type.t & VT_UNSIGNED);
            gv(r < 8 ? RC_R(r) : RC_F(r - 8));
            vtop->type = origtype;

            if (r2 && loadt != VT_LDOUBLE) {
                r2--;
                assert(r2 < 16 || r2 == TREG_RA);
                vswap();
                gaddrof();
                vtop->type = char_pointer_type;
                vpushi(ii >> 20);
#ifdef CONFIG_TCC_BCHECK
		if ((origtype.t & VT_BTYPE) == VT_STRUCT)
                    tcc_state->do_bounds_check = 0;
#endif
                gen_op('+');
#ifdef CONFIG_TCC_BCHECK
		tcc_state->do_bounds_check = bc_save;
#endif
                indir();
                vtop->type = origtype;
                loadt = vtop->type.t & VT_BTYPE;
                if (loadt == VT_STRUCT) {
                    loadt = (ii >> 16) & VT_BTYPE;
                }
                save_reg_upstack(r2, 1);
                vtop->type.t = loadt | (vtop->type.t & VT_UNSIGNED);
                load(r2, vtop);
                assert(r2 < VT_CONST);
                vtop--;
                vtop->r2 = r2;
            }
            if (info[nb_args - 1 - i] & 16) {
                ES(0x23, 3, 2, ireg(vtop->r2), splitofs); // sd t0, ofs(sp)
                vtop->r2 = VT_CONST;
            } else if (loadt == VT_LDOUBLE && vtop->r2 != r2) {
                assert(vtop->r2 <= 7 && r2 <= 7);
                /* XXX we'd like to have 'gv' move directly into
                   the right class instead of us fixing it up.  */
                EI(0x13, 0, ireg(r2), ireg(vtop->r2), 0); // mv Ra+1, RR2
                vtop->r2 = r2;
            }
done:
            vrott(i+1);
        }
    }
    vrotb(nb_args + 1);
    save_regs(nb_args + 1);
    gcall_or_jmp(1);
    rvv_cfg_invalidate(); /* the callee owns the vector state now */
    vtop -= nb_args + 1;
    if (stack_add) {
        if (stack_add >= 0x800) {
            o(0x37 | (5 << 7) | UPPER(stack_add)); //lui t0, upper(v)
            EI(0x13, 0, 5, 5, SIGN11(stack_add)); // addi t0, t0, lo(v)
            ER(0x33, 0, 2, 2, 5, 0); // add sp, sp, t0
        }
        else
            EI(0x13, 0, 2, 2, stack_add);      // addi sp, sp, adj
   }
   tcc_free(info);
}

static int func_sub_sp_offset, num_va_regs, func_va_list_ofs;

ST_FUNC void gfunc_prolog(Sym *func_sym)
{
    CType *func_type = &func_sym->type;
    int i, addr, align, size;
    int param_addr = 0;
    int areg[2];
    Sym *sym;
    CType *type;

    sym = func_type->ref;
    loc = -16; // for ra and s0
    func_sub_sp_offset = ind;
    ind += 5 * 4;

    areg[0] = 0, areg[1] = 0;
    addr = 0;
    rvv_cfg_invalidate(); /* the vector state at entry is not known */
    /* if the function returns by reference, then add an
       implicit pointer parameter */
    size = type_size(&func_vt, &align);
    if (size > 2 * XLEN || (func_vt.t & VT_VECTOR)) {
        loc -= 8;
        func_vc = loc;
        ES(0x23, 3, 8, 10 + areg[0]++, loc); // sd a0, loc(s0)
    }
    /* define parameters */
    while ((sym = sym->next) != NULL) {
        int byref = 0;
        int regcount;
        int prc[3], fieldofs[3];
        type = &sym->type;
        size = type_size(type, &align);
        if (size > 2 * XLEN || (type->t & VT_VECTOR)) {
            type = &char_pointer_type;
            size = align = byref = 8;
        }
        reg_pass(type, prc, fieldofs, 1);
        regcount = prc[0];
        if (areg[prc[1] - 1] >= 8
            || (regcount == 2
                && ((prc[1] == RC_FLOAT && prc[2] == RC_FLOAT && areg[1] >= 7)
                    || (prc[1] != prc[2] && (areg[1] >= 8 || areg[0] >= 8))))) {
            if (align < XLEN)
              align = XLEN;
            addr = (addr + align - 1) & -align;
            param_addr = addr;
            addr += size;
        } else {
            loc -= regcount * 8; // XXX could reserve only 'size' bytes
            param_addr = loc;
            for (i = 0; i < regcount; i++) {
                if (areg[prc[1+i] - 1] >= 8) {
                    assert(i == 1 && regcount == 2 && !(addr & 7));
                    EI(0x03, 3, 5, 8, addr); // ld t0, addr(s0)
                    addr += 8;
                    ES(0x23, 3, 8, 5, loc + i*8); // sd t0, loc(s0)
                } else if (prc[1+i] == RC_FLOAT) {
                    ES(0x27, (size / regcount) == 4 ? 2 : 3, 8, 10 + areg[1]++, loc + (fieldofs[i+1] >> 4)); // fs[wd] FAi, loc(s0)
                } else {
                    ES(0x23, 3, 8, 10 + areg[0]++, loc + i*8); // sd aX, loc(s0) // XXX
                }
            }
        }
        gfunc_set_param(sym, param_addr, byref);
    }
    func_va_list_ofs = addr;
    num_va_regs = 0;
    if (func_var) {
        for (; areg[0] < 8; areg[0]++) {
            num_va_regs++;
            ES(0x23, 3, 8, 10 + areg[0], -8 + num_va_regs * 8); // sd aX, loc(s0)
        }
    }
    riscv_regcache_save_offset = ind;
    /* In gfunc_epilog these will be replaced with the saves of the
       regcache registers the function actually used. */
    for (i = 0; i < REGCACHE_NREGS; ++i)
        EI(0x13, 0, 0, 0, 0); // nop
    /* reserve the regcache save slots below everything the prolog
       proper allocated */
    regcache_slot_base = loc;
    loc -= REGCACHE_FRAME;
#ifdef CONFIG_TCC_BCHECK
    if (tcc_state->do_bounds_check)
        gen_bounds_prolog();
#endif
}

ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret,
                       int *ret_align, int *regsize)
{
    int align, size = type_size(vt, &align), nregs;
    int prc[3], fieldofs[3];
    *ret_align = 1;
    *regsize = 8;
    if (vt->t & VT_VECTOR)
      return 0; /* vector results go through the sret buffer */
    if (size > 16)
      return 0;
    reg_pass(vt, prc, fieldofs, 1);
    nregs = prc[0];
    if (nregs == 2 && prc[1] != prc[2])
      return -1;  /* generic code can't deal with this case */
    if (prc[1] == RC_FLOAT) {
        *regsize = size / nregs;
    }
    ret->t = fieldofs[1] & VT_BTYPE;
    ret->ref = NULL;
    return nregs;
}

ST_FUNC void arch_transfer_ret_regs(int aftercall)
{
    int prc[3], fieldofs[3];
    reg_pass(&vtop->type, prc, fieldofs, 1);
    assert(prc[0] == 2 && prc[1] != prc[2] && !(fieldofs[1] >> 4));
    assert(vtop->r == (VT_LOCAL | VT_LVAL));
    vpushv(vtop);
    vtop->type.t = fieldofs[1] & VT_BTYPE;
    (aftercall ? store : load)(prc[1] == RC_INT ? REG_IRET : REG_FRET, vtop);
    vtop->c.i += fieldofs[2] >> 4;
    vtop->type.t = fieldofs[2] & VT_BTYPE;
    (aftercall ? store : load)(prc[2] == RC_INT ? REG_IRET : REG_FRET, vtop);
    vtop--;
}

ST_FUNC void gfunc_epilog(void)
{
    int v, saved_ind, d, large_ofs_ind;
    int i, mask = regcache_used_mask();

#ifdef CONFIG_TCC_BCHECK
    if (tcc_state->do_bounds_check)
        gen_bounds_epilog();
#endif

    /* backpatch the regcache saves into the reserved prolog area;
       registers the function never used stay nop padding */
    if (mask) {
        int rc_saved_ind = ind;
        ind = riscv_regcache_save_offset;
        for (i = 0; i < REGCACHE_NREGS; i++)
            if (mask & (1 << i))
                ES(0x23, 3, 8, ireg(regcache_regs[i]),
                   regcache_slot_base - 8 * (i + 1)); // sd sX, slot(s0)
            else
                EI(0x13, 0, 0, 0, 0); // nop
        ind = rc_saved_ind;
    }

    loc = (loc - num_va_regs * 8);
    d = v = (-loc + 15) & -16;

    /* restore the regcache registers the function used */
    for (i = 0; i < REGCACHE_NREGS; i++)
        if (mask & (1 << i))
            EI(0x03, 3, ireg(regcache_regs[i]), 8,
               regcache_slot_base - 8 * (i + 1)); // ld sX, slot(s0)
    EI(0x13, 0, 2, 8, num_va_regs * 8); // addi sp, s0, num_va_regs*8
    EI(0x03, 3, 1, 8, -8); // ld ra, -8(s0)
    EI(0x03, 3, 8, 8, -16); // ld s0, -16(s0)
    EI(0x67, 0, 0, 1, 0); // jalr x0, 0(x1), aka ret

    large_ofs_ind = ind;
    if (v >= (1 << 11)) {
        d = 16;
        EI(0x13, 0, 8, 2, d - num_va_regs * 8);      // addi s0, sp, d
        o(0x37 | (5 << 7) | UPPER(v-16)); //lui t0, upper(v)
        EI(0x13, 0, 5, 5, SIGN11(v-16)); // addi t0, t0, lo(v)
        ER(0x33, 0, 2, 2, 5, 0x20); // sub sp, sp, t0
        gjmp_addr(func_sub_sp_offset + 5*4);
    }
    saved_ind = ind;

    ind = func_sub_sp_offset;
    EI(0x13, 0, 2, 2, -d);     // addi sp, sp, -d
    ES(0x23, 3, 2, 1, d - 8 - num_va_regs * 8);  // sd ra, d-8(sp)
    ES(0x23, 3, 2, 8, d - 16 - num_va_regs * 8); // sd s0, d-16(sp)
    if (v < (1 << 11))
      EI(0x13, 0, 8, 2, d - num_va_regs * 8);      // addi s0, sp, d
    else
      gjmp_addr(large_ofs_ind);
    if ((ind - func_sub_sp_offset) != 5*4)
      EI(0x13, 0, 0, 0, 0);      // addi x0, x0, 0 == nop
    ind = saved_ind;
}

ST_FUNC void gen_va_start(void)
{
    vtop--;
    vset(&char_pointer_type, VT_LOCAL, func_va_list_ofs);
}

ST_FUNC void gen_fill_nops(int bytes)
{
    if ((bytes & 3))
      tcc_error("alignment of code section not multiple of 4");
    while (bytes > 0) {
        EI(0x13, 0, 0, 0, 0);      // addi x0, x0, 0 == nop
        bytes -= 4;
    }
}

// Generate forward branch to label:
ST_FUNC int gjmp(int t)
{
    if (nocode_wanted)
      return t;
    rvv_cfg_invalidate();
    o(t);
    return ind - 4;
}

// Generate branch to known address:
ST_FUNC void gjmp_addr(int a)
{
    uint32_t r = a - ind, imm;
    rvv_cfg_invalidate();
    if ((r + (1 << 21)) & ~((1U << 22) - 2)) {
        o(0x17 | (5 << 7) | UPPER(r)); // lui RR, up(r)
        r = SIGN11(r);
        EI(0x67, 0, 0, 5, r);      // jalr x0, r(t0)
    } else {
        imm = (((r >> 12) &  0xff) << 12)
            | (((r >> 11) &     1) << 20)
            | (((r >>  1) & 0x3ff) << 21)
            | (((r >> 20) &     1) << 31);
        o(0x6f | imm); // jal x0, imm ==  j imm
    }
}

ST_FUNC int gjmp_cond(int op, int t)
{
    int tmp;
    int a = vtop->cmp_r & 0xff;
    int b = (vtop->cmp_r >> 8) & 0xff;
    switch (op) {
        case TOK_ULT: op = 6; break;
        case TOK_UGE: op = 7; break;
        case TOK_ULE: op = 7; tmp = a; a = b; b = tmp; break;
        case TOK_UGT: op = 6; tmp = a; a = b; b = tmp; break;
        case TOK_LT:  op = 4; break;
        case TOK_GE:  op = 5; break;
        case TOK_LE:  op = 5; tmp = a; a = b; b = tmp; break;
        case TOK_GT:  op = 4; tmp = a; a = b; b = tmp; break;
        case TOK_NE:  op = 1; break;
        case TOK_EQ:  op = 0; break;
    }
    o(0x63 | (op ^ 1) << 12 | a << 15 | b << 20 | 8 << 7); // bOP a,b,+4
    return gjmp(t);
}

ST_FUNC int gjmp_append(int n, int t)
{
    void *p;
    /* insert jump list n into t */
    if (n) {
        uint32_t n1 = n, n2;
        while ((n2 = read32le(p = cur_text_section->data + n1)))
            n1 = n2;
        write32le(p, t);
        t = n;
    }
    return t;
}

static void gen_opil(int op, int ll)
{
    int a, b, d;
    int func3 = 0;
    ll = ll ? 0 : 8;
    if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == VT_CONST) {
        int fc = vtop->c.i;
        if (fc == vtop->c.i && !LOW_OVERFLOW(fc)) {
            int cll = 0;
            int m = ll ? 31 : 63;
            vswap();
            gv(RC_INT);
            a = ireg(vtop[0].r);
            --vtop;
            d = get_reg(RC_INT);
            ++vtop;
            vswap();
            switch (op) {
                case '-':
                    if (fc <= -(1 << 11))
                      break;
                    fc = -fc;
                case '+':
                    func3 = 0; // addi d, a, fc
                    cll = ll;
                do_cop:
                    EI(0x13 | cll, func3, ireg(d), a, fc);
                    --vtop;
                    if (op >= TOK_ULT && op <= TOK_GT) {
                      vset_VT_CMP(TOK_NE);
                      vtop->cmp_r = ireg(d) | 0 << 8;
                    } else
                      vtop[0].r = d;
                    return;
                case TOK_LE:
                    if (fc >= (1 << 11) - 1)
                      break;
                    ++fc;
                case TOK_LT:  func3 = 2; goto do_cop; // slti d, a, fc
                case TOK_ULE:
                    if (fc >= (1 << 11) - 1 || fc == -1)
                      break;
                    ++fc;
                case TOK_ULT: func3 = 3; goto do_cop; // sltiu d, a, fc
                case '^':     func3 = 4; goto do_cop; // xori d, a, fc
                case '|':     func3 = 6; goto do_cop; // ori  d, a, fc
                case '&':     func3 = 7; goto do_cop; // andi d, a, fc
                case TOK_SHL: func3 = 1; cll = ll; fc &= m; goto do_cop; // slli d, a, fc
                case TOK_SHR: func3 = 5; cll = ll; fc &= m; goto do_cop; // srli d, a, fc
                case TOK_SAR: func3 = 5; cll = ll; fc = 1024 | (fc & m); goto do_cop;

                case TOK_UGE: /* -> TOK_ULT */
                case TOK_UGT: /* -> TOK_ULE */
                case TOK_GE:  /* -> TOK_LT */
                case TOK_GT:  /* -> TOK_LE */
                    gen_opil(op - 1, !ll);
                    vtop->cmp_op ^= 1;
                    return;

                case TOK_NE:
                case TOK_EQ:
                    if (fc)
                      gen_opil('-', !ll), a = ireg(vtop++->r);
                    --vtop;
                    vset_VT_CMP(op);
                    vtop->cmp_r = a | 0 << 8;
                    return;
            }
        }
    }
    gv2(RC_INT, RC_INT);
    a = ireg(vtop[-1].r);
    b = ireg(vtop[0].r);
    vtop -= 2;
    d = get_reg(RC_INT);
    vtop++;
    vtop[0].r = d;
    d = ireg(d);
    switch (op) {
    default:
        if (op >= TOK_ULT && op <= TOK_GT) {
            vset_VT_CMP(op);
            vtop->cmp_r = a | b << 8;
            break;
        }
        tcc_error("implement me: %s(%s)", __FUNCTION__, get_tok_str(op, NULL));
        break;

    case '+':
        ER(0x33 | ll, 0, d, a, b, 0); // add d, a, b
        break;
    case '-':
        ER(0x33 | ll, 0, d, a, b, 0x20); // sub d, a, b
        break;
    case TOK_SAR:
        ER(0x33 | ll | ll, 5, d, a, b, 0x20); // sra d, a, b
        break;
    case TOK_SHR:
        ER(0x33 | ll | ll, 5, d, a, b, 0); // srl d, a, b
        break;
    case TOK_SHL:
        ER(0x33 | ll, 1, d, a, b, 0); // sll d, a, b
        break;
    case '*':
        ER(0x33 | ll, 0, d, a, b, 1); // mul d, a, b
        break;
    case '/':
    case TOK_PDIV:
        ER(0x33 | ll, 4, d, a, b, 1); // div d, a, b
        break;
    case '&':
        ER(0x33, 7, d, a, b, 0); // and d, a, b
        break;
    case '^':
        ER(0x33, 4, d, a, b, 0); // xor d, a, b
        break;
    case '|':
        ER(0x33, 6, d, a, b, 0); // or d, a, b
        break;
    case '%':
        ER(ll ? 0x3b:  0x33, 6, d, a, b, 1); // rem d, a, b
        break;
    case TOK_UMOD:
        ER(0x33 | ll, 7, d, a, b, 1); // remu d, a, b
        break;
    case TOK_UDIV:
        ER(0x33 | ll, 5, d, a, b, 1); // divu d, a, b
        break;
    }
}

ST_FUNC void gen_opi(int op)
{
    gen_opil(op, 0);
}

ST_FUNC void gen_opl(int op)
{
    gen_opil(op, 1);
}

ST_FUNC void gen_opf(int op)
{
    int rs1, rs2, rd, dbl, invert;
    if (vtop[0].type.t == VT_LDOUBLE) {
        CType type = vtop[0].type;
        int func = 0;
        int cond = -1;
        switch (op) {
        case '*': func = TOK___multf3; break;
        case '+': func = TOK___addtf3; break;
        case '-': func = TOK___subtf3; break;
        case '/': func = TOK___divtf3; break;
        case TOK_EQ: func = TOK___eqtf2; cond = 1; break;
        case TOK_NE: func = TOK___netf2; cond = 0; break;
        case TOK_LT: func = TOK___lttf2; cond = 10; break;
        case TOK_GE: func = TOK___getf2; cond = 11; break;
        case TOK_LE: func = TOK___letf2; cond = 12; break;
        case TOK_GT: func = TOK___gttf2; cond = 13; break;
        default: assert(0); break;
        }
        vpush_helper_func(func);
        vrott(3);
        gfunc_call(2);
        vpushi(0);
        vtop->r = REG_IRET;
        vtop->r2 = cond < 0 ? TREG_R(1) : VT_CONST;
        if (cond < 0)
            vtop->type = type;
        else {
            vpushi(0);
            gen_opil(op, 1);
        }
        return;
    }

    gv2(RC_FLOAT, RC_FLOAT);
    assert(vtop->type.t == VT_DOUBLE || vtop->type.t == VT_FLOAT);
    dbl = vtop->type.t == VT_DOUBLE;
    rs1 = freg(vtop[-1].r);
    rs2 = freg(vtop->r);
    vtop--;
    invert = 0;
    switch(op) {
    default:
        assert(0);
    case '+':
        op = 0; // fadd
    arithop:
        rd = get_reg(RC_FLOAT);
        vtop->r = rd;
        rd = freg(rd);
        ER(0x53, 7, rd, rs1, rs2, dbl | (op << 2)); // fop.[sd] RD, RS1, RS2 (dyn rm)
        break;
    case '-':
        op = 1; // fsub
        goto arithop;
    case '*':
        op = 2; // fmul
        goto arithop;
    case '/':
        op = 3; // fdiv
        goto arithop;
    case TOK_EQ:
        op = 2; // EQ
    cmpop:
        rd = get_reg(RC_INT);
        vtop->r = rd;
        rd = ireg(rd);
        ER(0x53, op, rd, rs1, rs2, dbl | 0x50); // fcmp.[sd] RD, RS1, RS2 (op == eq/lt/le)
        if (invert)
          EI(0x13, 4, rd, rd, 1); // xori RD, 1

        /* generate VT_CMP output */
        vset_VT_CMP(TOK_NE);
        vtop->cmp_r = rd | (0 << 8);
        break;
    case TOK_NE:
        invert = 1;
        op = 2; // EQ
        goto cmpop;
    case TOK_LT:
        op = 1; // LT
        goto cmpop;
    case TOK_LE:
        op = 0; // LE
        goto cmpop;
    case TOK_GT:
        op = 1; // LT
        rd = rs1, rs1 = rs2, rs2 = rd;
        goto cmpop;
    case TOK_GE:
        op = 0; // LE
        rd = rs1, rs1 = rs2, rs2 = rd;
        goto cmpop;
    }
}

ST_FUNC void gen_cvt_csti(int t)
{
    int r = ireg(gv(RC_INT));
    if ((t & VT_BTYPE) == VT_SHORT) {
        if (t & VT_UNSIGNED) {
            EI(0x13, 1, r, r, 48); // slli r, r, 48
            EI(0x13, 5, r, r, 48); // srli r, r, 48
        } else {
            EI(0x13, 1, r, r, 48); // slli r, r, 48
            EIu(0x13, 5, r, r, 0x400 | 48); // srai r, r, 48
        }
    } else {
        if (t & VT_UNSIGNED) {
            EI(0x13, 7, r, r, 0xff); // andi r, r, 0xff
        } else {
            EI(0x13, 1, r, r, 56); // slli r, r, 56
            EIu(0x13, 5, r, r, 0x400 | 56); // srai r, r, 56
        }
    }
}

ST_FUNC void gen_cvt_sxtw(void)
{
    int r = ireg(gv(RC_INT));
    EI(0x1b, 0, r, r, 0); // addiw r, r, 0
}

ST_FUNC void gen_cvt_itof(int t)
{
    int rr = ireg(gv(RC_INT)), dr;
    int u = vtop->type.t & VT_UNSIGNED;
    int l = (vtop->type.t & VT_BTYPE) == VT_LLONG;
    if (t == VT_LDOUBLE) {
        int func = l ?
          (u ? TOK___floatunditf : TOK___floatditf) :
          (u ? TOK___floatunsitf : TOK___floatsitf);
        vpush_helper_func(func);
        vrott(2);
        gfunc_call(1);
        vpushi(0);
        vtop->type.t = t;
        vtop->r = REG_IRET;
        vtop->r2 = TREG_R(1);
    } else {
        vtop--;
        dr = get_reg(RC_FLOAT);
        vtop++;
        vtop->r = dr;
        dr = freg(dr);
        EIu(0x53, 7, dr, rr, ((0x68 | (t == VT_DOUBLE ? 1 : 0)) << 5) | (u ? 1 : 0) | (l ? 2 : 0)); // fcvt.[sd].[wl][u]
    }
}

ST_FUNC void gen_cvt_ftoi(int t)
{
    int ft = vtop->type.t & VT_BTYPE;
    int l = (t & VT_BTYPE) == VT_LLONG;
    int u = t & VT_UNSIGNED;
    if (ft == VT_LDOUBLE) {
        int func = l ?
          (u ? TOK___fixunstfdi : TOK___fixtfdi) :
          (u ? TOK___fixunstfsi : TOK___fixtfsi);
        vpush_helper_func(func);
        vrott(2);
        gfunc_call(1);
        vpushi(0);
        vtop->type.t = t;
        vtop->r = REG_IRET;
    } else {
        int rr = freg(gv(RC_FLOAT)), dr;
        vtop--;
        dr = get_reg(RC_INT);
        vtop++;
        vtop->r = dr;
        dr = ireg(dr);
        EIu(0x53, 1, dr, rr, ((0x60 | (ft == VT_DOUBLE ? 1 : 0)) << 5) | (u ? 1 : 0) | (l ? 2 : 0)); // fcvt.[wl][u].[sd] rtz
    }
}

ST_FUNC void gen_cvt_ftof(int dt)
{
    int st = vtop->type.t & VT_BTYPE, rs, rd;
    dt &= VT_BTYPE;
    if (st == dt)
      return;
    if (dt == VT_LDOUBLE || st == VT_LDOUBLE) {
        int func = (dt == VT_LDOUBLE) ?
            (st == VT_FLOAT ? TOK___extendsftf2 : TOK___extenddftf2) :
            (dt == VT_FLOAT ? TOK___trunctfsf2 : TOK___trunctfdf2);
        /* We can't use gfunc_call, as func_old_type works like vararg
           functions, and on riscv unnamed float args are passed like
           integers.  But we really need them in the float argument registers
           for extendsftf2/extenddftf2.  So, do it explicitely.  */
        save_regs(1);
        if (dt == VT_LDOUBLE)
          gv(RC_F(0));
        else {
            gv(RC_R(0));
            assert(vtop->r2 < 7);
            if (vtop->r2 != 1 + vtop->r) {
                EI(0x13, 0, ireg(vtop->r) + 1, ireg(vtop->r2), 0); // mv Ra+1, RR2
                vtop->r2 = 1 + vtop->r;
            }
        }
        vpush_helper_func(func);
        gcall_or_jmp(1);
        rvv_cfg_invalidate();
        vtop -= 2;
        vpushi(0);
        vtop->type.t = dt;
        if (dt == VT_LDOUBLE)
          vtop->r = REG_IRET, vtop->r2 = REG_IRET+1;
        else
          vtop->r = REG_FRET;
    } else {
        assert (dt == VT_FLOAT || dt == VT_DOUBLE);
        assert (st == VT_FLOAT || st == VT_DOUBLE);
        rs = gv(RC_FLOAT);
        rd = get_reg(RC_FLOAT);
        if (dt == VT_DOUBLE)
          EI(0x53, 0, freg(rd), freg(rs), 0x21 << 5); // fcvt.d.s RD, RS (no rm)
        else
          EI(0x53, 7, freg(rd), freg(rs), (0x20 << 5) | 1); // fcvt.s.d RD, RS (dyn rm)
        vtop->r = rd;
    }
}

/* increment tcov counter */
ST_FUNC void gen_increment_tcov (SValue *sv)
{
    int r1, r2;
    Sym label = {0};
    label.type.t = VT_VOID | VT_STATIC;

    vpushv(sv);
    vtop->r = r1 = get_reg(RC_INT);
    r2 = get_reg(RC_INT);
    r1 = ireg(r1);
    r2 = ireg(r2);
    greloca(cur_text_section, sv->sym, ind, R_RISCV_PCREL_HI20, 0);
    put_extern_sym(&label, cur_text_section, ind, 0);
    o(0x17 | (r1 << 7)); // auipc RR, 0 %pcrel_hi(sym)
    greloca(cur_text_section, &label, ind, R_RISCV_PCREL_LO12_I, 0);
    EI(0x03, 3, r2, r1, 0); // ld r2, x[r1]
    EI(0x13, 0, r2, r2, 1); // addi r2, r2, #1
    greloca(cur_text_section, sv->sym, ind, R_RISCV_PCREL_HI20, 0);
    label.c = 0; /* force new local ELF symbol */
    put_extern_sym(&label, cur_text_section, ind, 0);
    o(0x17 | (r1 << 7)); // auipc RR, 0 %pcrel_hi(sym)
    greloca(cur_text_section, &label, ind, R_RISCV_PCREL_LO12_S, 0);
    ES(0x23, 3, r1, r2, 0); // sd r2, [r1]
    vpop();
}

ST_FUNC void ggoto(void)
{
    gcall_or_jmp(0);
    rvv_cfg_invalidate();
    vtop--;
}

ST_FUNC void gen_vla_sp_save(int addr)
{
    if (LOW_OVERFLOW(addr)) {
	o(0x37 | (5 << 7) | UPPER(addr)); //lui t0,upper(addr)
        ER(0x33, 0, 5, 5, 8, 0); // add t0, t0, s0
        ES(0x23, 3, 5, 2, SIGN11(addr)); // sd sp, fc(t0)
    }
    else
        ES(0x23, 3, 8, 2, addr); // sd sp, fc(s0)
}

ST_FUNC void gen_vla_sp_restore(int addr)
{
    if (LOW_OVERFLOW(addr)) {
	o(0x37 | (5 << 7) | UPPER(addr)); //lui t0,upper(addr)
        ER(0x33, 0, 5, 5, 8, 0); // add t0, t0, s0
        EI(0x03, 3, 2, 5, SIGN11(addr)); // ld sp, fc(t0)
    }
    else
        EI(0x03, 3, 2, 8, addr); // ld sp, fc(s0)
}

ST_FUNC void gen_vla_alloc(CType *type, int align)
{
    int rr;
#if defined(CONFIG_TCC_BCHECK)
    if (tcc_state->do_bounds_check)
        vpushv(vtop);
#endif
    rr = ireg(gv(RC_INT));
#if defined(CONFIG_TCC_BCHECK)
    if (tcc_state->do_bounds_check)
        EI(0x13, 0, rr, rr, 15+1);   // addi RR, RR, 15+1
    else
#endif
    EI(0x13, 0, rr, rr, 15);   // addi RR, RR, 15
    EI(0x13, 7, rr, rr, -16);  // andi, RR, RR, -16
    ER(0x33, 0, 2, 2, rr, 0x20); // sub sp, sp, rr
    vpop();
#if defined(CONFIG_TCC_BCHECK)
    if (tcc_state->do_bounds_check) {
        vpushi(0);
        vtop->r = TREG_R(0);
        o(0x00010513); /* mv a0,sp */
        vswap();
        vpush_helper_func(TOK___bound_new_region);
        vrott(3);
        gfunc_call(2);
        func_bound_add_epilog = 1;
    }
#endif
}

ST_FUNC void gen_clear_cache(void)
{
    /* Zifencei extension: fence + fence.i for I/D synchronization.
       Required by RISC-V Linux ABI, present on all Linux-capable cores. */
    o(0x0ff0000f); // fence iorw, iorw
    o(0x0000100f); // fence.i
}

/* ==================================================================
   RVV fixed-length vector support (design section 7).

   Vector values are always memory-resident: an SValue for a vector
   is an lvalue naming frame storage (a variable or a temp slot).
   An operation loads its operands into scratch register groups,
   runs under a vsetvli configuration derived from the type (SEW =
   element width, LMUL = smallest that holds the type at the
   target's minimum VLEN, VL = lane count), and stores the result
   group into a fresh temp slot.  Types too wide for one register
   group at the minimum VLEN stay usable as data (copies, argument
   passing) but operations on them are rejected by name.

   Scratch registers: t0-t2 (hardware 5-7) are never allocated by
   this backend.  Vector groups use bases 8 (result), 16 and 24
   (operands) and v0 (mask): argument registers, dead while an
   expression is evaluated, and multiples of 8 so the base is
   aligned for every LMUL.  Mask operands of masked instructions
   must sit in v0.
   ================================================================== */

#define RVV_T0 5
#define RVV_T1 6
#define RVV_T2 7
#define RVV_GR 8
#define RVV_GA 16
#define RVV_GB 24

typedef struct {
    int sew;   /* element width in bits: 8, 16, 32 or 64 */
    int lmul8; /* LMUL times 8: 1, 2, 4, 8, 16, 32 or 64 */
    int vl;    /* lane count */
} RvvCfg;

static RvvCfg rvv_cfg_cur;
static int rvv_cfg_valid;

static void rvv_cfg_invalidate(void)
{
    rvv_cfg_valid = 0;
}

static int rvv_vlen_min(void)
{
    int z = tcc_state->target_vlen_min;
    return z ? z : 128;
}

static void rvv_check_v(void)
{
    if (!(tcc_state->target_features & TFEAT_V))
        tcc_error("this vector operation requires the RISC-V V extension "
                  "(compile with -march=rv64gcv)");
}

/* element size in bytes and lane count of a vector type */
static void rvv_shape(CType *t, int *es, int *lanes)
{
    int a;
    *es = type_size(&t->ref->type, &a);
    *lanes = (int)t->ref->c;
}

static int rvv_is_mask(CType *t)
{
    return (t->ref->type.t & VT_BTYPE) == VT_BOOL;
}

/* bytes actually stored for a value of vector type t: mask types
   pack their lanes into bits, everything else is lanes * esize */
static int rvv_stored_size(CType *t)
{
    int es, lanes, a;
    if (rvv_is_mask(t)) {
        rvv_shape(t, &es, &lanes);
        return (lanes + 7) / 8;
    }
    return type_size(t, &a);
}

/* derive the vsetvli configuration for a vector type; returns 0
   when the type does not fit in one register group at the
   target's minimum VLEN */
static int rvv_cfg_for(CType *t, RvvCfg *c)
{
    int es, lanes, bits, z;
    rvv_shape(t, &es, &lanes);
    c->sew = es * 8;
    c->vl = lanes;
    bits = c->sew * lanes;
    z = rvv_vlen_min();
    if (bits > 8 * z)
        return 0;
    c->lmul8 = 1;
    while (c->lmul8 < 64 && c->lmul8 * z < 8 * bits)
        c->lmul8 <<= 1;
    return 1;
}

static void rvv_set_cfg(const RvvCfg *c)
{
    int vlmul, vsew;
    if (nocode_wanted)
        return;
    if (rvv_cfg_valid && rvv_cfg_cur.sew == c->sew
        && rvv_cfg_cur.lmul8 == c->lmul8 && rvv_cfg_cur.vl == c->vl)
        return;
    if (c->vl >= 0 && c->vl < 2048)
        EI(0x13, 0, RVV_T0, 0, c->vl); // addi t0, x0, vl
    else {
        o(0x37 | (RVV_T0 << 7) | UPPER(c->vl)); // lui t0, hi(vl)
        EI(0x13, 0, RVV_T0, RVV_T0, SIGN11(c->vl)); // addi t0, t0, lo(vl)
    }
    vlmul = c->lmul8 == 1 ? 5 : c->lmul8 == 2 ? 6 : c->lmul8 == 4 ? 7
          : c->lmul8 == 8 ? 0 : c->lmul8 == 16 ? 1 : c->lmul8 == 32 ? 2 : 3;
    vsew = c->sew == 8 ? 0 : c->sew == 16 ? 1 : c->sew == 32 ? 2 : 3;
    /* vsetvli x0, t0, vtypei with vta=1 (tail agnostic) and vma=0
       (masked-off lanes keep the passthrough value) */
    o(0x57 | (7 << 12) | (RVV_T0 << 15)
      | (((1 << 6) | (vsew << 3) | vlmul) << 20));
    rvv_cfg_cur = *c;
    rvv_cfg_valid = 1;
}

/* treg = s0 + off (treg is a hardware register number) */
static void rvv_frame_addr(int treg, int off)
{
    if (!LOW_OVERFLOW(off))
        EI(0x13, 0, treg, 8, SIGN11(off)); // addi treg, s0, off
    else {
        o(0x37 | (treg << 7) | UPPER(off)); // lui treg, hi(off)
        EI(0x13, 0, treg, treg, SIGN11(off)); // addi treg, treg, lo(off)
        ER(0x33, 0, treg, treg, 8, 0); // add treg, treg, s0
    }
}

/* put the address of the memory value named by sv into hardware
   register treg (one of RVV_T0/RVV_T1; RVV_T2 is scratch) */
static void rvv_addr(SValue *sv, int treg)
{
    int v = sv->r & VT_VALMASK;
    if (v == VT_LOCAL) {
        rvv_frame_addr(treg, (int)sv->c.i);
        return;
    }
    if (v == VT_LLOCAL) {
        int fc = (int)sv->c.i;
        if (!LOW_OVERFLOW(fc))
            EI(0x03, 3, treg, 8, SIGN11(fc)); // ld treg, fc(s0)
        else {
            rvv_frame_addr(RVV_T2, fc);
            EI(0x03, 3, treg, RVV_T2, 0); // ld treg, 0(t2)
        }
        return;
    }
    if ((sv->r & VT_LVAL) && v < VT_CONST) {
        /* register + offset */
        int base = ireg(v);
        if (sv->c.i == 0) {
            if (treg != base)
                EI(0x13, 0, treg, base, 0); // mv treg, base
        } else if (!LOW_OVERFLOW(sv->c.i))
            EI(0x13, 0, treg, base, SIGN11(sv->c.i)); // addi
        else {
            o(0x37 | (treg << 7) | UPPER(sv->c.i)); // lui
            EI(0x13, 0, treg, treg, SIGN11(sv->c.i)); // addi
            ER(0x33, 0, treg, treg, base, 0); // add
        }
        return;
    }
    if (sv->r & VT_SYM) {
        /* mirror load_symofs' addressing: PC-relative for statics,
           GOT-indirect for globals, thread-pointer-relative for TLS */
        Sym label = {0};
        label.type.t = VT_VOID | VT_STATIC;
        if (sv->sym->type.t & VT_TLS) {
            int fc = (int)sv->c.i;
            o(0x37 | (treg << 7)); // lui treg, %tprel_hi(sym)
            greloca(cur_text_section, sv->sym, ind - 4, R_RISCV_TPREL_HI20, fc);
            EI(0x13, 0, treg, treg, 0); // addi treg, treg, %tprel_lo(sym)
            greloca(cur_text_section, sv->sym, ind - 4, R_RISCV_TPREL_LO12_I, fc);
            ER(0x33, 0, treg, treg, 4, 0); // add treg, treg, tp
            return;
        }
        if (sv->sym->type.t & VT_STATIC)
            greloca(cur_text_section, sv->sym, ind, R_RISCV_PCREL_HI20, sv->c.i);
        else
            greloca(cur_text_section, sv->sym, ind, R_RISCV_GOT_HI20, 0);
        if (!nocode_wanted)
            put_extern_sym(&label, cur_text_section, ind, 0);
        o(0x17 | (treg << 7)); // auipc treg, 0
        if (sv->sym->type.t & VT_STATIC) {
            greloca(cur_text_section, &label, ind, R_RISCV_PCREL_LO12_I, 0);
            EI(0x13, 0, treg, treg, 0); // addi treg, treg, 0
        } else {
            greloca(cur_text_section, &label, ind, R_RISCV_PCREL_LO12_I, 0);
            EI(0x03, 3, treg, treg, 0); // ld treg, 0(treg)
            if (sv->c.i) {
                int fc = (int)sv->c.i;
                if (!LOW_OVERFLOW(fc))
                    EI(0x13, 0, treg, treg, SIGN11(fc)); // addi
                else {
                    o(0x37 | (RVV_T2 << 7) | UPPER(fc)); // lui t2
                    EI(0x13, 0, RVV_T2, RVV_T2, SIGN11(fc)); // addi t2
                    ER(0x33, 0, treg, treg, RVV_T2, 0); // add
                }
            }
        }
        return;
    }
    if (v == VT_CONST) {
        /* an absolute address */
        load_large_constant(treg, (int)sv->c.i, (uint32_t)(sv->c.i >> 32));
        return;
    }
    tcc_error("rvv: this vector operand has no addressable home");
}

static int rvv_width_code(int es)
{
    return es == 1 ? 0 : es == 2 ? 5 : es == 4 ? 6 : 7;
}

/* vle<sew>.v g, (areg) under t's configuration; masks use vlm.v,
   which moves exactly ceil(lanes/8) bytes */
static void rvv_load_group(int g, int areg, CType *t)
{
    RvvCfg c;
    int es, lanes;
    if (!rvv_cfg_for(t, &c))
        tcc_error("rvv: vector type is too wide for one register group "
                  "at this -march minimum VLEN");
    rvv_set_cfg(&c);
    rvv_shape(t, &es, &lanes);
    if (rvv_is_mask(t))
        o(0x07 | (1 << 25) | (11 << 20) | (areg << 15) | (g << 7)); // vlm.v
    else
        o(0x07 | (1 << 25) | (areg << 15) | (rvv_width_code(es) << 12)
          | (g << 7)); // vle<sew>.v
}

/* vse<sew>.v g, (areg); masks use vsm.v */
static void rvv_store_group(int g, int areg, CType *t)
{
    RvvCfg c;
    int es, lanes;
    if (!rvv_cfg_for(t, &c))
        tcc_error("rvv: vector type is too wide for one register group "
                  "at this -march minimum VLEN");
    rvv_set_cfg(&c);
    rvv_shape(t, &es, &lanes);
    if (rvv_is_mask(t))
        o(0x27 | (1 << 25) | (11 << 20) | (areg << 15) | (g << 7)); // vsm.v
    else
        o(0x27 | (1 << 25) | (areg << 15) | (rvv_width_code(es) << 12)
          | (g << 7)); // vse<sew>.v
}

/* scalar-chunk copy of n bytes from the address in hardware
   register sareg to the address in dareg (both are bumped) */
static void rvv_chunk_copy(int sareg, int dareg, int n)
{
    int done = 0;
    while (n - done >= 8) {
        EI(0x03, 3, RVV_T2, sareg, 0); // ld t2, 0(sa)
        ES(0x23, 3, RVV_T2, dareg, 0); // sd t2, 0(da)
        EI(0x13, 0, sareg, sareg, 8);
        EI(0x13, 0, dareg, dareg, 8);
        done += 8;
    }
    if (n - done >= 4) {
        EI(0x03, 2, RVV_T2, sareg, 0); // lw
        ES(0x23, 2, RVV_T2, dareg, 0); // sw
        EI(0x13, 0, sareg, sareg, 4);
        EI(0x13, 0, dareg, dareg, 4);
        done += 4;
    }
    if (n - done >= 2) {
        EI(0x03, 5, RVV_T2, sareg, 0); // lhu
        ES(0x23, 1, RVV_T2, dareg, 0); // sh
        EI(0x13, 0, sareg, sareg, 2);
        EI(0x13, 0, dareg, dareg, 2);
        done += 2;
    }
    if (n - done >= 1) {
        EI(0x03, 4, RVV_T2, sareg, 0); // lbu
        ES(0x23, 0, RVV_T2, dareg, 0); // sb
    }
}

/* copy the value named by src (of vector type t) to the memory
   named by dst.  Data movement is never gated on the V feature:
   without V, or for group-oversize types, the copy falls back to
   scalar chunks.  The destination address is materialized only
   after the source group is loaded, because configuring the
   vector state uses t0. */
static void rvv_copy(SValue *src, CType *t, SValue *dst)
{
    RvvCfg c;
    if ((tcc_state->target_features & TFEAT_V) && rvv_cfg_for(t, &c)) {
        rvv_addr(src, RVV_T1);
        rvv_load_group(RVV_GR, RVV_T1, t);
        rvv_addr(dst, RVV_T0);
        rvv_store_group(RVV_GR, RVV_T0, t);
        return;
    }
    rvv_addr(dst, RVV_T0);
    rvv_addr(src, RVV_T1);
    rvv_chunk_copy(RVV_T1, RVV_T0, rvv_stored_size(t));
}

static void rvv_store_vec(SValue *sv)
{
    /* the store() hook for vector destinations: the source value
       is on top of the value stack (vstore's flow) */
    SValue *src = vtop;
    if (!(src->type.t & VT_VECTOR))
        tcc_error("rvv: store of a non-vector value into a vector");
    rvv_copy(src, &sv->type, sv);
}

/* mark sv (on the value stack) as the fresh temp slot at the given
   frame offset */
static void rvv_set_slot(SValue *sv, CType *restype, int off, int r2)
{
    /* a computed vector value is an rvalue naming its slot: with
       VT_LVAL set, generic code would try to load it into a
       scalar register */
    sv->r = VT_LOCAL;
    sv->r2 = r2;
    sv->c.i = off;
    sv->sym = NULL;
    sv->type = *restype;
}

ST_FUNC void gen_vec_gv(void)
{
    /* normalize the vector value on top of the stack into the
       home form: frame-slot values already have one; anything
       else is copied into a fresh temp slot */
    SValue *sv = vtop;
    SValue dstsv;
    int size, align, r2, l;
    if ((sv->r & VT_VALMASK) == VT_LOCAL)
        return;
    size = type_size(&sv->type, &align);
    l = get_temp_local_var(size, align, &r2);
    dstsv.r = VT_LOCAL | VT_LVAL;
    dstsv.r2 = r2;
    dstsv.c.i = l;
    dstsv.sym = NULL;
    dstsv.type = sv->type;
    rvv_copy(sv, &sv->type, &dstsv);
    rvv_set_slot(sv, &sv->type, l, r2);
}

ST_FUNC void gen_vec_home(void)
{
    /* a vector rvalue must gain a memory home before its address
       can be taken (subscript): lvalues and slot values have one */
    if ((vtop->r & VT_LVAL) || (vtop->r & VT_VALMASK) == VT_LOCAL)
        return;
    gen_vec_gv();
}

ST_FUNC void gen_vec_ret(void)
{
    /* the return value goes to the caller-provided buffer whose
       address the prolog stored at func_vc; the pointer is loaded
       only after the source group, because configuring the vector
       state uses t0 */
    SValue *sv = vtop;
    CType *t = &sv->type;
    RvvCfg c;
    if ((tcc_state->target_features & TFEAT_V) && rvv_cfg_for(t, &c)) {
        rvv_addr(sv, RVV_T1);
        rvv_load_group(RVV_GR, RVV_T1, t);
        if (!LOW_OVERFLOW(func_vc))
            EI(0x03, 3, RVV_T0, 8, SIGN11(func_vc)); // ld t0, func_vc(s0)
        else {
            rvv_frame_addr(RVV_T0, func_vc);
            EI(0x03, 3, RVV_T0, RVV_T0, 0); // ld t0, 0(t0)
        }
        rvv_store_group(RVV_GR, RVV_T0, t);
        return;
    }
    if (!LOW_OVERFLOW(func_vc))
        EI(0x03, 3, RVV_T0, 8, SIGN11(func_vc)); // ld t0, func_vc(s0)
    else {
        rvv_frame_addr(RVV_T0, func_vc);
        EI(0x03, 3, RVV_T0, RVV_T0, 0); // ld t0, 0(t0)
    }
    rvv_addr(sv, RVV_T1);
    rvv_chunk_copy(RVV_T1, RVV_T0, rvv_stored_size(t));
}

/* ---- opcode tables for the generic RVV builtins ----
   The numeric opcodes are shared with the shipped riscv_vector.h;
   tools/riscv_spec.py is the source both are generated from. */

enum {
    RVK_VV = 0, /* second operand in a vector group */
    RVK_VX,     /* second operand is an integer scalar */
    RVK_VF,     /* second operand is a float scalar */
    RVK_V,      /* unary vector form; the vs1 field comes from aux */
    RVK_V0,     /* unary vector form with the vs2 field zeroed
                   (vmv.v.v: the decoder requires vs2 == 0) */
    RVK_GEN,    /* unary form that generates its result (vid.v):
                   the operand is not read */
    RVK_VVC,    /* vcompress: (data, mask value) -> packed result */
    RVK_RED,    /* reduction: like RVK_VV, but only element 0 of
                   the result is defined; the rest of the result
                   slot is zeroed */
    RVK_SX1,    /* vmv.s.x: passthrough plus a scalar for lane 0 */
    RVK_SF1,    /* vfmv.s.f: passthrough plus a float scalar */
    RVK_VX0,    /* unary form of a .vx instruction with rs1 = x0 */
    RVK_VI,     /* unary form of a .vi instruction; imm from aux */
    RVK_RX,     /* extract element 0 to an integer register */
    RVK_RF,     /* extract element 0 to a float register */
    RVK_SX,     /* splat an integer scalar (or an imm5 constant) */
    RVK_SF      /* splat a float scalar */
};

typedef struct {
    int opc;
    int f6;      /* funct6 */
    int f3;      /* funct3 */
    int kind;    /* RVK_* */
    int cfgres;  /* the instruction runs under the result type's
                    configuration (narrowing forms); otherwise it
                    runs under the first operand's (widening forms
                    included) */
    int aux;     /* op1: literal vs1 field, or -1 for "same as vs2";
                    op1 RVK_VI: the imm5 value */
} RvvOp;

/* RVV_TABLES_BEGIN (generated by tools/riscv_gen.py; do not edit) */
enum {
    RVVOP_VMV_V_V = 0,
    RVVOP_VNEG_I = 1,
    RVVOP_VNOT_I = 2,
    RVVOP_VFABS = 3,
    RVVOP_VFNEG = 4,
    RVVOP_VFCLASS = 5,
    RVVOP_VFSQRT = 6,
    RVVOP_VMNOT_M = 7,
    RVVOP_VMSBF_M = 8,
    RVVOP_VMSIF_M = 9,
    RVVOP_VMSOF_M = 10,
    RVVOP_VIOTA_M = 11,
    RVVOP_VID_V = 12,
    RVVOP_VFREC7 = 13,
    RVVOP_VFRSQRT7 = 14,
    RVVOP_VFCVT_X_F = 15,
    RVVOP_VFCVT_XU_F = 16,
    RVVOP_VFCVT_RTZ_X_F = 17,
    RVVOP_VFCVT_RTZ_XU_F = 18,
    RVVOP_VFCVT_F_X = 19,
    RVVOP_VFCVT_F_XU = 20,
    RVVOP_VFWCVT_X_F = 21,
    RVVOP_VFWCVT_XU_F = 22,
    RVVOP_VFWCVT_F_X = 23,
    RVVOP_VFWCVT_F_XU = 24,
    RVVOP_VFWCVT_F_F = 25,
    RVVOP_VFNCVT_X_F = 26,
    RVVOP_VFNCVT_XU_F = 27,
    RVVOP_VFNCVT_F_X = 28,
    RVVOP_VFNCVT_F_XU = 29,
    RVVOP_VFNCVT_F_F = 30,
    RVVOP_VFNCVT_RTZ_X_F = 31,
    RVVOP_VFNCVT_RTZ_XU_F = 32,
    RVVOP_VFNCVT_ROD_F_F = 33,
    RVVOP_VMV_X_S = 100,
    RVVOP_VFMV_F_S = 101,
    RVVOP_VCPOP_M = 102,
    RVVOP_VFIRST_M = 103,
    RVVOP_VADD_VV = 200,
    RVVOP_VADD_VX = 201,
    RVVOP_VSUB_VV = 202,
    RVVOP_VSUB_VX = 203,
    RVVOP_VMUL_VV = 204,
    RVVOP_VMUL_VX = 205,
    RVVOP_VDIV_VV = 206,
    RVVOP_VDIV_VX = 207,
    RVVOP_VDIVU_VV = 208,
    RVVOP_VDIVU_VX = 209,
    RVVOP_VREM_VV = 210,
    RVVOP_VREM_VX = 211,
    RVVOP_VREMU_VV = 212,
    RVVOP_VREMU_VX = 213,
    RVVOP_VAND_VV = 214,
    RVVOP_VAND_VX = 215,
    RVVOP_VOR_VV = 216,
    RVVOP_VOR_VX = 217,
    RVVOP_VXOR_VV = 218,
    RVVOP_VXOR_VX = 219,
    RVVOP_VMIN_VV = 220,
    RVVOP_VMIN_VX = 221,
    RVVOP_VMAX_VV = 222,
    RVVOP_VMAX_VX = 223,
    RVVOP_VMINU_VV = 224,
    RVVOP_VMINU_VX = 225,
    RVVOP_VMAXU_VV = 226,
    RVVOP_VMAXU_VX = 227,
    RVVOP_VSLL_VV = 228,
    RVVOP_VSLL_VX = 229,
    RVVOP_VSRL_VV = 230,
    RVVOP_VSRL_VX = 231,
    RVVOP_VSRA_VV = 232,
    RVVOP_VSRA_VX = 233,
    RVVOP_VRSUB_VX = 234,
    RVVOP_VMULH_VV = 235,
    RVVOP_VMULH_VX = 236,
    RVVOP_VMULHU_VV = 237,
    RVVOP_VMULHU_VX = 238,
    RVVOP_VMULHSU_VV = 239,
    RVVOP_VMULHSU_VX = 240,
    RVVOP_VSADD_VV = 241,
    RVVOP_VSADD_VX = 242,
    RVVOP_VSADDU_VV = 243,
    RVVOP_VSADDU_VX = 244,
    RVVOP_VSSUB_VV = 245,
    RVVOP_VSSUB_VX = 246,
    RVVOP_VSSUBU_VV = 247,
    RVVOP_VSSUBU_VX = 248,
    RVVOP_VAADD_VV = 249,
    RVVOP_VAADD_VX = 250,
    RVVOP_VAADDU_VV = 251,
    RVVOP_VAADDU_VX = 252,
    RVVOP_VASUB_VV = 253,
    RVVOP_VASUB_VX = 254,
    RVVOP_VASUBU_VV = 255,
    RVVOP_VASUBU_VX = 256,
    RVVOP_VWADDU_VV = 257,
    RVVOP_VWADDU_WV = 258,
    RVVOP_VWADDU_VX = 259,
    RVVOP_VWADDU_WX = 260,
    RVVOP_VWADD_VV = 261,
    RVVOP_VWADD_WV = 262,
    RVVOP_VWADD_VX = 263,
    RVVOP_VWADD_WX = 264,
    RVVOP_VWSUBU_VV = 265,
    RVVOP_VWSUBU_WV = 266,
    RVVOP_VWSUB_VV = 267,
    RVVOP_VWSUB_WV = 268,
    RVVOP_VWMUL_VV = 269,
    RVVOP_VWMULU_VV = 270,
    RVVOP_VWMULSU_VV = 271,
    RVVOP_VNSRL_WV = 272,
    RVVOP_VNSRL_WX = 273,
    RVVOP_VNSRA_WV = 274,
    RVVOP_VNSRA_WX = 275,
    RVVOP_VNCLIP_WV = 276,
    RVVOP_VNCLIP_WX = 277,
    RVVOP_VNCLIPU_WV = 278,
    RVVOP_VNCLIPU_WX = 279,
    RVVOP_VMAND_MM = 280,
    RVVOP_VMNAND_MM = 281,
    RVVOP_VMNOR_MM = 287,
    RVVOP_VMANDN_MM = 282,
    RVVOP_VMOR_MM = 283,
    RVVOP_VMORN_MM = 284,
    RVVOP_VMXOR_MM = 285,
    RVVOP_VMXNOR_MM = 286,
    RVVOP_VSLIDEUP_VX = 420,
    RVVOP_VSLIDEDOWN_VX = 421,
    RVVOP_VSLIDE1UP_VX = 422,
    RVVOP_VSLIDE1DOWN_VX = 423,
    RVVOP_VFSLIDE1UP_VF = 424,
    RVVOP_VFSLIDE1DOWN_VF = 425,
    RVVOP_VRGATHER_VV = 426,
    RVVOP_VRGATHER_VX = 427,
    RVVOP_VRGATHEREI16_VV = 428,
    RVVOP_VFWADD_VV = 429,
    RVVOP_VFWADD_WV = 430,
    RVVOP_VFWADD_VF = 431,
    RVVOP_VFWSUB_VV = 432,
    RVVOP_VFWSUB_WV = 433,
    RVVOP_VFWSUB_VF = 434,
    RVVOP_VFWMUL_VV = 435,
    RVVOP_VFWMUL_VF = 436,
    RVVOP_VFWADD_WF = 453,
    RVVOP_VFWSUB_WF = 454,
    RVVOP_VREDSUM = 437,
    RVVOP_VREDAND = 438,
    RVVOP_VREDOR = 439,
    RVVOP_VREDXOR = 440,
    RVVOP_VREDMINU = 441,
    RVVOP_VREDMIN = 442,
    RVVOP_VREDMAXU = 443,
    RVVOP_VREDMAX = 444,
    RVVOP_VWREDSUM = 445,
    RVVOP_VWREDSUMU = 446,
    RVVOP_VFREDUSUM = 447,
    RVVOP_VFREDOSUM = 448,
    RVVOP_VFREDMIN = 449,
    RVVOP_VFREDMAX = 450,
    RVVOP_VFWREDUSUM = 451,
    RVVOP_VFWREDOSUM = 452,
    RVVOP_VFADD_VV = 300,
    RVVOP_VFADD_VF = 301,
    RVVOP_VFSUB_VV = 302,
    RVVOP_VFSUB_VF = 303,
    RVVOP_VFRSUB_VF = 318,
    RVVOP_VFMUL_VV = 304,
    RVVOP_VFMUL_VF = 305,
    RVVOP_VFDIV_VV = 306,
    RVVOP_VFDIV_VF = 307,
    RVVOP_VFMIN_VV = 308,
    RVVOP_VFMIN_VF = 309,
    RVVOP_VFMAX_VV = 310,
    RVVOP_VFMAX_VF = 311,
    RVVOP_VFSGNJ_VV = 312,
    RVVOP_VFSGNJ_VF = 313,
    RVVOP_VFSGNJN_VV = 314,
    RVVOP_VFSGNJN_VF = 315,
    RVVOP_VFSGNJX_VV = 316,
    RVVOP_VFSGNJX_VF = 317,
    RVVOP_VMSEQ_VV = 400,
    RVVOP_VMSEQ_VX = 401,
    RVVOP_VMSNE_VV = 402,
    RVVOP_VMSNE_VX = 403,
    RVVOP_VMSLT_VV = 404,
    RVVOP_VMSLT_VX = 405,
    RVVOP_VMSLE_VV = 406,
    RVVOP_VMSLE_VX = 407,
    RVVOP_VMSLTU_VV = 408,
    RVVOP_VMSLTU_VX = 409,
    RVVOP_VMSLEU_VV = 410,
    RVVOP_VMSLEU_VX = 411,
    RVVOP_VMFEQ_VV = 412,
    RVVOP_VMFEQ_VF = 413,
    RVVOP_VMFNE_VV = 414,
    RVVOP_VMFNE_VF = 415,
    RVVOP_VMFLT_VV = 416,
    RVVOP_VMFLT_VF = 417,
    RVVOP_VMFLE_VV = 418,
    RVVOP_VMFLE_VF = 419,
    RVVOP_VSLL_VI = 500,
    RVVOP_VSRL_VI = 501,
    RVVOP_VSRA_VI = 502,
    RVVOP_VADD_VI = 503,
    RVVOP_VRSUB_VI = 504,
    RVVOP_VAND_VI = 505,
    RVVOP_VOR_VI = 506,
    RVVOP_VXOR_VI = 507,
    RVVOP_VMSEQ_VI = 508,
    RVVOP_VMSNE_VI = 509,
    RVVOP_VMSLE_VI = 511,
    RVVOP_VMSLEU_VI = 513,
    RVVOP_VNSRL_WI = 514,
    RVVOP_VNSRA_WI = 515,
    RVVOP_VNCLIP_WI = 516,
    RVVOP_VNCLIPU_WI = 517,
    RVVOP_SPLAT_X = 600,
    RVVOP_SPLAT_F = 601,
    RVVOP_VMACC_VV = 700,
    RVVOP_VMACC_VX = 701,
    RVVOP_VMADD_VV = 702,
    RVVOP_VMADD_VX = 703,
    RVVOP_VNMSAC_VV = 704,
    RVVOP_VNMSAC_VX = 705,
    RVVOP_VNMSUB_VV = 706,
    RVVOP_VNMSUB_VX = 707,
    RVVOP_VFMACC_VV = 708,
    RVVOP_VFMACC_VF = 709,
    RVVOP_VFMADD_VV = 710,
    RVVOP_VFMADD_VF = 711,
    RVVOP_VFNMSAC_VV = 712,
    RVVOP_VFNMSAC_VF = 713,
    RVVOP_VFNMSUB_VV = 714,
    RVVOP_VFNMSUB_VF = 715,
    RVVOP_VFMSAC_VV = 728,
    RVVOP_VFMSAC_VF = 729,
    RVVOP_VFMSUB_VV = 730,
    RVVOP_VFMSUB_VF = 731,
    RVVOP_VFNMACC_VV = 732,
    RVVOP_VFNMACC_VF = 733,
    RVVOP_VFNMADD_VV = 734,
    RVVOP_VFNMADD_VF = 735,
    RVVOP_VFWMACC_VV = 716,
    RVVOP_VFWMACC_VF = 717,
    RVVOP_VFWMSAC_VV = 718,
    RVVOP_VFWMSAC_VF = 719,
    RVVOP_VFWNMSAC_VV = 720,
    RVVOP_VFWNMSAC_VF = 721,
    RVVOP_VSLIDEUP_VX3 = 722,
    RVVOP_VSLIDEDOWN_VX3 = 723,
    RVVOP_VSLIDE1UP_VX3 = 724,
    RVVOP_VSLIDE1DOWN_VX3 = 725,
    RVVOP_VFSLIDE1UP_VF3 = 726,
    RVVOP_VFSLIDE1DOWN_VF3 = 727,
    RVVOP_VMERGE_VVM = 800,
    RVVOP_VMERGE_VXM = 801,
    RVVOP_VADC_VVM = 802,
    RVVOP_VSBC_VVM = 803,
    RVVOP_VCOMPRESS_VV = 804,
    RVVOP_VADC_VXM = 805,
    RVVOP_VSBC_VXM = 806,
    RVVOP_VFMERGE_VFM = 807,
    RVVOP_VMV_S_X = 900,
    RVVOP_VFMV_S_F = 901,
};

static const RvvOp rvv_op1_tab[] = {
    { RVVOP_VMV_V_V, 0x17, 0, RVK_V0, 0, -1 },
    { RVVOP_VNEG_I, 0x03, 4, RVK_VX0, 0, 0 },
    { RVVOP_VNOT_I, 0x0b, 3, RVK_VI, 0, -1 },
    { RVVOP_VFABS, 0x0a, 1, RVK_V, 0, -1 },
    { RVVOP_VFNEG, 0x09, 1, RVK_V, 0, -1 },
    { RVVOP_VFCLASS, 0x13, 1, RVK_V, 0, 16 },
    { RVVOP_VFSQRT, 0x13, 1, RVK_V, 0, 0 },
    { RVVOP_VMNOT_M, 0x1d, 2, RVK_V, 0, -1 },
    { RVVOP_VMSBF_M, 0x14, 2, RVK_V, 0, 1 },
    { RVVOP_VMSIF_M, 0x14, 2, RVK_V, 0, 3 },
    { RVVOP_VMSOF_M, 0x14, 2, RVK_V, 0, 2 },
    { RVVOP_VIOTA_M, 0x14, 2, RVK_V, 1, 16 },
    { RVVOP_VID_V, 0x14, 2, RVK_GEN, 0, 17 },
    { RVVOP_VFREC7, 0x13, 1, RVK_V, 0, 5 },
    { RVVOP_VFRSQRT7, 0x13, 1, RVK_V, 0, 4 },
    { RVVOP_VFCVT_X_F, 0x12, 1, RVK_V, 0, 1 },
    { RVVOP_VFCVT_XU_F, 0x12, 1, RVK_V, 0, 0 },
    { RVVOP_VFCVT_RTZ_X_F, 0x12, 1, RVK_V, 0, 7 },
    { RVVOP_VFCVT_RTZ_XU_F, 0x12, 1, RVK_V, 0, 6 },
    { RVVOP_VFCVT_F_X, 0x12, 1, RVK_V, 0, 3 },
    { RVVOP_VFCVT_F_XU, 0x12, 1, RVK_V, 0, 2 },
    { RVVOP_VFWCVT_X_F, 0x12, 1, RVK_V, 2, 9 },
    { RVVOP_VFWCVT_XU_F, 0x12, 1, RVK_V, 2, 8 },
    { RVVOP_VFWCVT_F_X, 0x12, 1, RVK_V, 2, 11 },
    { RVVOP_VFWCVT_F_XU, 0x12, 1, RVK_V, 2, 10 },
    { RVVOP_VFWCVT_F_F, 0x12, 1, RVK_V, 2, 12 },
    { RVVOP_VFNCVT_X_F, 0x12, 1, RVK_V, 1, 17 },
    { RVVOP_VFNCVT_XU_F, 0x12, 1, RVK_V, 1, 16 },
    { RVVOP_VFNCVT_F_X, 0x12, 1, RVK_V, 1, 19 },
    { RVVOP_VFNCVT_F_XU, 0x12, 1, RVK_V, 1, 18 },
    { RVVOP_VFNCVT_F_F, 0x12, 1, RVK_V, 1, 20 },
    { RVVOP_VFNCVT_RTZ_X_F, 0x12, 1, RVK_V, 1, 23 },
    { RVVOP_VFNCVT_RTZ_XU_F, 0x12, 1, RVK_V, 1, 22 },
    { RVVOP_VFNCVT_ROD_F_F, 0x12, 1, RVK_V, 1, 21 },
};

static const RvvOp rvv_op1r_tab[] = {
    { RVVOP_VMV_X_S, 0x10, 2, RVK_RX, 0, 0 },
    { RVVOP_VFMV_F_S, 0x10, 1, RVK_RF, 0, 0 },
    { RVVOP_VCPOP_M, 0x10, 2, RVK_RX, 0, 16 },
    { RVVOP_VFIRST_M, 0x10, 2, RVK_RX, 0, 17 },
};

static const RvvOp rvv_op2_tab[] = {
    { RVVOP_VADD_VV, 0x00, 0, RVK_VV, 0, 0 },
    { RVVOP_VADD_VX, 0x00, 4, RVK_VX, 0, 0 },
    { RVVOP_VSUB_VV, 0x02, 0, RVK_VV, 0, 0 },
    { RVVOP_VSUB_VX, 0x02, 4, RVK_VX, 0, 0 },
    { RVVOP_VMUL_VV, 0x25, 2, RVK_VV, 0, 0 },
    { RVVOP_VMUL_VX, 0x25, 6, RVK_VX, 0, 0 },
    { RVVOP_VDIV_VV, 0x21, 2, RVK_VV, 0, 0 },
    { RVVOP_VDIV_VX, 0x21, 6, RVK_VX, 0, 0 },
    { RVVOP_VDIVU_VV, 0x20, 2, RVK_VV, 0, 0 },
    { RVVOP_VDIVU_VX, 0x20, 6, RVK_VX, 0, 0 },
    { RVVOP_VREM_VV, 0x23, 2, RVK_VV, 0, 0 },
    { RVVOP_VREM_VX, 0x23, 6, RVK_VX, 0, 0 },
    { RVVOP_VREMU_VV, 0x22, 2, RVK_VV, 0, 0 },
    { RVVOP_VREMU_VX, 0x22, 6, RVK_VX, 0, 0 },
    { RVVOP_VAND_VV, 0x09, 0, RVK_VV, 0, 0 },
    { RVVOP_VAND_VX, 0x09, 4, RVK_VX, 0, 0 },
    { RVVOP_VOR_VV, 0x0a, 0, RVK_VV, 0, 0 },
    { RVVOP_VOR_VX, 0x0a, 4, RVK_VX, 0, 0 },
    { RVVOP_VXOR_VV, 0x0b, 0, RVK_VV, 0, 0 },
    { RVVOP_VXOR_VX, 0x0b, 4, RVK_VX, 0, 0 },
    { RVVOP_VMIN_VV, 0x05, 0, RVK_VV, 0, 0 },
    { RVVOP_VMIN_VX, 0x05, 4, RVK_VX, 0, 0 },
    { RVVOP_VMAX_VV, 0x07, 0, RVK_VV, 0, 0 },
    { RVVOP_VMAX_VX, 0x07, 4, RVK_VX, 0, 0 },
    { RVVOP_VMINU_VV, 0x04, 0, RVK_VV, 0, 0 },
    { RVVOP_VMINU_VX, 0x04, 4, RVK_VX, 0, 0 },
    { RVVOP_VMAXU_VV, 0x06, 0, RVK_VV, 0, 0 },
    { RVVOP_VMAXU_VX, 0x06, 4, RVK_VX, 0, 0 },
    { RVVOP_VSLL_VV, 0x25, 0, RVK_VV, 0, 0 },
    { RVVOP_VSLL_VX, 0x25, 4, RVK_VX, 0, 0 },
    { RVVOP_VSRL_VV, 0x28, 0, RVK_VV, 0, 0 },
    { RVVOP_VSRL_VX, 0x28, 4, RVK_VX, 0, 0 },
    { RVVOP_VSRA_VV, 0x29, 0, RVK_VV, 0, 0 },
    { RVVOP_VSRA_VX, 0x29, 4, RVK_VX, 0, 0 },
    { RVVOP_VRSUB_VX, 0x03, 4, RVK_VX, 0, 0 },
    { RVVOP_VFADD_VV, 0x00, 1, RVK_VV, 0, 0 },
    { RVVOP_VFADD_VF, 0x00, 5, RVK_VF, 0, 0 },
    { RVVOP_VFSUB_VV, 0x02, 1, RVK_VV, 0, 0 },
    { RVVOP_VFSUB_VF, 0x02, 5, RVK_VF, 0, 0 },
    { RVVOP_VFRSUB_VF, 0x27, 5, RVK_VF, 0, 0 },
    { RVVOP_VFMUL_VV, 0x24, 1, RVK_VV, 0, 0 },
    { RVVOP_VFMUL_VF, 0x24, 5, RVK_VF, 0, 0 },
    { RVVOP_VFDIV_VV, 0x20, 1, RVK_VV, 0, 0 },
    { RVVOP_VFDIV_VF, 0x20, 5, RVK_VF, 0, 0 },
    { RVVOP_VFMIN_VV, 0x04, 1, RVK_VV, 0, 0 },
    { RVVOP_VFMIN_VF, 0x04, 5, RVK_VF, 0, 0 },
    { RVVOP_VFMAX_VV, 0x06, 1, RVK_VV, 0, 0 },
    { RVVOP_VFMAX_VF, 0x06, 5, RVK_VF, 0, 0 },
    { RVVOP_VFSGNJ_VV, 0x08, 1, RVK_VV, 0, 0 },
    { RVVOP_VFSGNJ_VF, 0x08, 5, RVK_VF, 0, 0 },
    { RVVOP_VFSGNJN_VV, 0x09, 1, RVK_VV, 0, 0 },
    { RVVOP_VFSGNJN_VF, 0x09, 5, RVK_VF, 0, 0 },
    { RVVOP_VFSGNJX_VV, 0x0a, 1, RVK_VV, 0, 0 },
    { RVVOP_VFSGNJX_VF, 0x0a, 5, RVK_VF, 0, 0 },
    { RVVOP_VMSEQ_VV, 0x18, 0, RVK_VV, 0, 0 },
    { RVVOP_VMSEQ_VX, 0x18, 4, RVK_VX, 0, 0 },
    { RVVOP_VMSNE_VV, 0x19, 0, RVK_VV, 0, 0 },
    { RVVOP_VMSNE_VX, 0x19, 4, RVK_VX, 0, 0 },
    { RVVOP_VMSLT_VV, 0x1b, 0, RVK_VV, 0, 0 },
    { RVVOP_VMSLT_VX, 0x1b, 4, RVK_VX, 0, 0 },
    { RVVOP_VMSLE_VV, 0x1d, 0, RVK_VV, 0, 0 },
    { RVVOP_VMSLE_VX, 0x1d, 4, RVK_VX, 0, 0 },
    { RVVOP_VMSLTU_VV, 0x1a, 0, RVK_VV, 0, 0 },
    { RVVOP_VMSLTU_VX, 0x1a, 4, RVK_VX, 0, 0 },
    { RVVOP_VMSLEU_VV, 0x1c, 0, RVK_VV, 0, 0 },
    { RVVOP_VMSLEU_VX, 0x1c, 4, RVK_VX, 0, 0 },
    { RVVOP_VMFEQ_VV, 0x18, 1, RVK_VV, 0, 0 },
    { RVVOP_VMFEQ_VF, 0x18, 5, RVK_VF, 0, 0 },
    { RVVOP_VMFNE_VV, 0x1c, 1, RVK_VV, 0, 0 },
    { RVVOP_VMFNE_VF, 0x1c, 5, RVK_VF, 0, 0 },
    { RVVOP_VMFLT_VV, 0x1b, 1, RVK_VV, 0, 0 },
    { RVVOP_VMFLT_VF, 0x1b, 5, RVK_VF, 0, 0 },
    { RVVOP_VMFLE_VV, 0x19, 1, RVK_VV, 0, 0 },
    { RVVOP_VMFLE_VF, 0x19, 5, RVK_VF, 0, 0 },
    { RVVOP_VMULH_VV, 0x27, 2, RVK_VV, 0, 0 },
    { RVVOP_VMULH_VX, 0x27, 6, RVK_VX, 0, 0 },
    { RVVOP_VMULHU_VV, 0x24, 2, RVK_VV, 0, 0 },
    { RVVOP_VMULHU_VX, 0x24, 6, RVK_VX, 0, 0 },
    { RVVOP_VMULHSU_VV, 0x26, 2, RVK_VV, 0, 0 },
    { RVVOP_VMULHSU_VX, 0x26, 6, RVK_VX, 0, 0 },
    { RVVOP_VSADD_VV, 0x21, 0, RVK_VV, 0, 0 },
    { RVVOP_VSADD_VX, 0x21, 4, RVK_VX, 0, 0 },
    { RVVOP_VSADDU_VV, 0x20, 0, RVK_VV, 0, 0 },
    { RVVOP_VSADDU_VX, 0x20, 4, RVK_VX, 0, 0 },
    { RVVOP_VSSUB_VV, 0x23, 0, RVK_VV, 0, 0 },
    { RVVOP_VSSUB_VX, 0x23, 4, RVK_VX, 0, 0 },
    { RVVOP_VSSUBU_VV, 0x22, 0, RVK_VV, 0, 0 },
    { RVVOP_VSSUBU_VX, 0x22, 4, RVK_VX, 0, 0 },
    { RVVOP_VAADD_VV, 0x09, 2, RVK_VV, 0, 0 },
    { RVVOP_VAADD_VX, 0x09, 6, RVK_VX, 0, 0 },
    { RVVOP_VAADDU_VV, 0x08, 2, RVK_VV, 0, 0 },
    { RVVOP_VAADDU_VX, 0x08, 6, RVK_VX, 0, 0 },
    { RVVOP_VASUB_VV, 0x0b, 2, RVK_VV, 0, 0 },
    { RVVOP_VASUB_VX, 0x0b, 6, RVK_VX, 0, 0 },
    { RVVOP_VASUBU_VV, 0x0a, 2, RVK_VV, 0, 0 },
    { RVVOP_VASUBU_VX, 0x0a, 6, RVK_VX, 0, 0 },
    { RVVOP_VWADDU_VV, 0x30, 2, RVK_VV, 0, 0 },
    { RVVOP_VWADDU_WV, 0x34, 2, RVK_VV, 2, 0 },
    { RVVOP_VWADDU_VX, 0x30, 6, RVK_VX, 0, 0 },
    { RVVOP_VWADDU_WX, 0x34, 6, RVK_VX, 2, 0 },
    { RVVOP_VWADD_VV, 0x31, 2, RVK_VV, 0, 0 },
    { RVVOP_VWADD_WV, 0x35, 2, RVK_VV, 2, 0 },
    { RVVOP_VWADD_VX, 0x31, 6, RVK_VX, 0, 0 },
    { RVVOP_VWADD_WX, 0x35, 6, RVK_VX, 2, 0 },
    { RVVOP_VWSUBU_VV, 0x32, 2, RVK_VV, 0, 0 },
    { RVVOP_VWSUBU_WV, 0x36, 2, RVK_VV, 2, 0 },
    { RVVOP_VWSUB_VV, 0x33, 2, RVK_VV, 0, 0 },
    { RVVOP_VWSUB_WV, 0x37, 2, RVK_VV, 2, 0 },
    { RVVOP_VWMUL_VV, 0x3b, 2, RVK_VV, 0, 0 },
    { RVVOP_VWMULU_VV, 0x38, 2, RVK_VV, 0, 0 },
    { RVVOP_VWMULSU_VV, 0x3a, 2, RVK_VV, 0, 0 },
    { RVVOP_VNSRL_WV, 0x2c, 0, RVK_VV, 1, 0 },
    { RVVOP_VNSRL_WX, 0x2c, 4, RVK_VX, 1, 0 },
    { RVVOP_VNSRA_WV, 0x2d, 0, RVK_VV, 1, 0 },
    { RVVOP_VNSRA_WX, 0x2d, 4, RVK_VX, 1, 0 },
    { RVVOP_VNCLIP_WV, 0x2f, 0, RVK_VV, 1, 0 },
    { RVVOP_VNCLIP_WX, 0x2f, 4, RVK_VX, 1, 0 },
    { RVVOP_VNCLIPU_WV, 0x2e, 0, RVK_VV, 1, 0 },
    { RVVOP_VNCLIPU_WX, 0x2e, 4, RVK_VX, 1, 0 },
    { RVVOP_VMAND_MM, 0x19, 2, RVK_VV, 0, 0 },
    { RVVOP_VMNAND_MM, 0x1d, 2, RVK_VV, 0, 0 },
    { RVVOP_VMNOR_MM, 0x1e, 2, RVK_VV, 0, 0 },
    { RVVOP_VMANDN_MM, 0x18, 2, RVK_VV, 0, 0 },
    { RVVOP_VMOR_MM, 0x1a, 2, RVK_VV, 0, 0 },
    { RVVOP_VMORN_MM, 0x1c, 2, RVK_VV, 0, 0 },
    { RVVOP_VMXOR_MM, 0x1b, 2, RVK_VV, 0, 0 },
    { RVVOP_VMXNOR_MM, 0x1f, 2, RVK_VV, 0, 0 },
    { RVVOP_VRGATHER_VV, 0x0c, 0, RVK_VV, 0, 0 },
    { RVVOP_VRGATHER_VX, 0x0c, 4, RVK_VX, 0, 0 },
    { RVVOP_VRGATHEREI16_VV, 0x0e, 0, RVK_VV, 0, 0 },
    { RVVOP_VSLIDEDOWN_VX, 0x0f, 4, RVK_VX, 0, 0 },
    { RVVOP_VSLIDE1UP_VX, 0x0e, 6, RVK_VX, 0, 0 },
    { RVVOP_VSLIDE1DOWN_VX, 0x0f, 6, RVK_VX, 0, 0 },
    { RVVOP_VFSLIDE1UP_VF, 0x0e, 5, RVK_VF, 0, 0 },
    { RVVOP_VFSLIDE1DOWN_VF, 0x0f, 5, RVK_VF, 0, 0 },
    { RVVOP_VFWADD_VV, 0x30, 1, RVK_VV, 0, 0 },
    { RVVOP_VFWADD_WV, 0x34, 1, RVK_VV, 2, 0 },
    { RVVOP_VFWADD_VF, 0x30, 5, RVK_VF, 0, 0 },
    { RVVOP_VFWSUB_VV, 0x32, 1, RVK_VV, 0, 0 },
    { RVVOP_VFWSUB_WV, 0x36, 1, RVK_VV, 2, 0 },
    { RVVOP_VFWSUB_VF, 0x32, 5, RVK_VF, 0, 0 },
    { RVVOP_VFWMUL_VV, 0x38, 1, RVK_VV, 0, 0 },
    { RVVOP_VFWMUL_VF, 0x38, 5, RVK_VF, 0, 0 },
    { RVVOP_VFWADD_WF, 0x34, 5, RVK_VF, 2, 0 },
    { RVVOP_VFWSUB_WF, 0x36, 5, RVK_VF, 2, 0 },
    { RVVOP_VREDSUM, 0x00, 2, RVK_RED, 0, 0 },
    { RVVOP_VREDAND, 0x01, 2, RVK_RED, 0, 0 },
    { RVVOP_VREDOR, 0x02, 2, RVK_RED, 0, 0 },
    { RVVOP_VREDXOR, 0x03, 2, RVK_RED, 0, 0 },
    { RVVOP_VREDMINU, 0x04, 2, RVK_RED, 0, 0 },
    { RVVOP_VREDMIN, 0x05, 2, RVK_RED, 0, 0 },
    { RVVOP_VREDMAXU, 0x06, 2, RVK_RED, 0, 0 },
    { RVVOP_VREDMAX, 0x07, 2, RVK_RED, 0, 0 },
    { RVVOP_VWREDSUM, 0x31, 0, RVK_RED, 0, 0 },
    { RVVOP_VWREDSUMU, 0x30, 0, RVK_RED, 0, 0 },
    { RVVOP_VFREDUSUM, 0x01, 1, RVK_RED, 0, 0 },
    { RVVOP_VFREDOSUM, 0x03, 1, RVK_RED, 0, 0 },
    { RVVOP_VFREDMIN, 0x05, 1, RVK_RED, 0, 0 },
    { RVVOP_VFREDMAX, 0x07, 1, RVK_RED, 0, 0 },
    { RVVOP_VFWREDUSUM, 0x31, 1, RVK_RED, 0, 0 },
    { RVVOP_VFWREDOSUM, 0x33, 1, RVK_RED, 0, 0 },
};

static const RvvOp rvv_op2i_tab[] = {
    { RVVOP_VSLL_VI, 0x25, 3, RVK_VI, 0, 0 },
    { RVVOP_VSRL_VI, 0x28, 3, RVK_VI, 0, 0 },
    { RVVOP_VSRA_VI, 0x29, 3, RVK_VI, 0, 0 },
    { RVVOP_VADD_VI, 0x00, 3, RVK_VI, 0, 0 },
    { RVVOP_VRSUB_VI, 0x03, 3, RVK_VI, 0, 0 },
    { RVVOP_VAND_VI, 0x09, 3, RVK_VI, 0, 0 },
    { RVVOP_VOR_VI, 0x0a, 3, RVK_VI, 0, 0 },
    { RVVOP_VXOR_VI, 0x0b, 3, RVK_VI, 0, 0 },
    { RVVOP_VMSEQ_VI, 0x18, 3, RVK_VI, 0, 0 },
    { RVVOP_VMSNE_VI, 0x19, 3, RVK_VI, 0, 0 },
    { RVVOP_VMSLE_VI, 0x1d, 3, RVK_VI, 0, 0 },
    { RVVOP_VMSLEU_VI, 0x1c, 3, RVK_VI, 0, 0 },
    { RVVOP_VNSRL_WI, 0x2c, 3, RVK_VI, 1, 0 },
    { RVVOP_VNSRA_WI, 0x2d, 3, RVK_VI, 1, 0 },
    { RVVOP_VNCLIP_WI, 0x2f, 3, RVK_VI, 1, 0 },
    { RVVOP_VNCLIPU_WI, 0x2e, 3, RVK_VI, 1, 0 },
};

static const RvvOp rvv_op3_tab[] = {
    { RVVOP_VMACC_VV, 0x2d, 2, RVK_VV, 0, 0 },
    { RVVOP_VMACC_VX, 0x2d, 6, RVK_VX, 0, 0 },
    { RVVOP_VMADD_VV, 0x29, 2, RVK_VV, 0, 0 },
    { RVVOP_VMADD_VX, 0x29, 6, RVK_VX, 0, 0 },
    { RVVOP_VNMSAC_VV, 0x2f, 2, RVK_VV, 0, 0 },
    { RVVOP_VNMSAC_VX, 0x2f, 6, RVK_VX, 0, 0 },
    { RVVOP_VNMSUB_VV, 0x2b, 2, RVK_VV, 0, 0 },
    { RVVOP_VNMSUB_VX, 0x2b, 6, RVK_VX, 0, 0 },
    { RVVOP_VFMACC_VV, 0x2c, 1, RVK_VV, 0, 0 },
    { RVVOP_VFMACC_VF, 0x2c, 5, RVK_VF, 0, 0 },
    { RVVOP_VFMADD_VV, 0x28, 1, RVK_VV, 0, 0 },
    { RVVOP_VFMADD_VF, 0x28, 5, RVK_VF, 0, 0 },
    { RVVOP_VFNMSAC_VV, 0x2f, 1, RVK_VV, 0, 0 },
    { RVVOP_VFNMSAC_VF, 0x2f, 5, RVK_VF, 0, 0 },
    { RVVOP_VFNMSUB_VV, 0x2b, 1, RVK_VV, 0, 0 },
    { RVVOP_VFNMSUB_VF, 0x2b, 5, RVK_VF, 0, 0 },
    { RVVOP_VFMSAC_VV, 0x2e, 1, RVK_VV, 0, 0 },
    { RVVOP_VFMSAC_VF, 0x2e, 5, RVK_VF, 0, 0 },
    { RVVOP_VFMSUB_VV, 0x2a, 1, RVK_VV, 0, 0 },
    { RVVOP_VFMSUB_VF, 0x2a, 5, RVK_VF, 0, 0 },
    { RVVOP_VFNMACC_VV, 0x2d, 1, RVK_VV, 0, 0 },
    { RVVOP_VFNMACC_VF, 0x2d, 5, RVK_VF, 0, 0 },
    { RVVOP_VFNMADD_VV, 0x29, 1, RVK_VV, 0, 0 },
    { RVVOP_VFNMADD_VF, 0x29, 5, RVK_VF, 0, 0 },
    { RVVOP_VFWMACC_VV, 0x3c, 1, RVK_VV, 2, 0 },
    { RVVOP_VFWMACC_VF, 0x3c, 5, RVK_VF, 2, 0 },
    { RVVOP_VFWMSAC_VV, 0x3e, 1, RVK_VV, 2, 0 },
    { RVVOP_VFWMSAC_VF, 0x3e, 5, RVK_VF, 2, 0 },
    { RVVOP_VFWNMSAC_VV, 0x3f, 1, RVK_VV, 2, 0 },
    { RVVOP_VFWNMSAC_VF, 0x3f, 5, RVK_VF, 2, 0 },
    { RVVOP_VSLIDEUP_VX3, 0x0e, 4, RVK_VX, 0, 0 },
};

static const RvvOp rvv_splat_tab[] = {
    { RVVOP_SPLAT_X, 0x17, 4, RVK_SX, 0, 0 },
    { RVVOP_SPLAT_F, 0x17, 5, RVK_SF, 0, 0 },
};

static const RvvOp rvv_op1m2_tab[] = {
    { RVVOP_VMV_S_X, 0x10, 6, RVK_SX1, 0, 0 },
    { RVVOP_VFMV_S_F, 0x10, 5, RVK_SF1, 0, 0 },
};

static const RvvOp rvv_opm_tab[] = {
    { RVVOP_VMERGE_VVM, 0x17, 0, RVK_VV, 0, 0 },
    { RVVOP_VMERGE_VXM, 0x17, 4, RVK_VX, 0, 0 },
    { RVVOP_VADC_VVM, 0x10, 0, RVK_VV, 0, 0 },
    { RVVOP_VSBC_VVM, 0x12, 0, RVK_VV, 0, 0 },
    { RVVOP_VCOMPRESS_VV, 0x17, 2, RVK_VVC, 0, 0 },
    { RVVOP_VADC_VXM, 0x10, 4, RVK_VX, 0, 0 },
    { RVVOP_VSBC_VXM, 0x12, 4, RVK_VX, 0, 0 },
    { RVVOP_VFMERGE_VFM, 0x17, 5, RVK_VF, 0, 0 },
};

/* RVV_TABLES_END */

#define RVV_NTAB(t) ((int)(sizeof(t) / sizeof((t)[0])))

static void rvv_insn(int f6, int f3, int vd, int vs2, int vs1, int vm)
{
    o(0x57 | (f6 << 26) | (vm << 25) | (vs2 << 20) | (vs1 << 15)
      | (f3 << 12) | (vd << 7));
}

static const RvvOp *rvv_lookup(const RvvOp *tab, int n, int opc)
{
    int i;
    for (i = 0; i < n; i++)
        if (tab[i].opc == opc)
            return &tab[i];
    return NULL;
}

static const RvvOp *rvv_find(const RvvOp *tab, int n, int opc,
                             const char *what)
{
    const RvvOp *e = rvv_lookup(tab, n, opc);
    if (!e)
        tcc_error("rvv: unsupported %s vector builtin (opcode %d)",
                  what, opc);
    return e;
}

/* run the instruction under the configuration of the result type
   (cfgres 1), of the first operand's type (cfgres 0), or of the
   result type at half width (cfgres 2: the .wv/.wx widening forms,
   whose operands sit in groups of half the result's element width
   and half its group multiplier) */
static void rvv_op_cfg(CType *restype, CType *atype, int cfgres)
{
    RvvCfg c;
    if (!rvv_cfg_for(cfgres ? restype : atype, &c))
        tcc_error("rvv: vector type is too wide for one register group "
                  "at this -march minimum VLEN");
    if (cfgres == 2) {
        c.sew >>= 1;
        c.lmul8 >>= 1;
        if (c.sew < 8 || c.lmul8 < 1)
            tcc_error("rvv: vector type is too narrow for a widening "
                      "form");
    }
    rvv_set_cfg(&c);
}

/* load the vector operand sv into group g */
static void rvv_load_op(SValue *sv, int g)
{
    rvv_addr(sv, RVV_T1);
    rvv_load_group(g, RVV_T1, &sv->type);
}

/* allocate a temp slot for restype and store group g into it; the
   operands are still on the value stack, so their slots stay
   marked in use.  The result configuration is established before
   the slot address is materialized (configuring uses t0). */
static int rvv_store_result(int g, CType *restype, int *pr2)
{
    RvvCfg c;
    int size, align, off;
    if (rvv_cfg_for(restype, &c))
        rvv_set_cfg(&c);
    size = type_size(restype, &align);
    off = get_temp_local_var(size, align, pr2);
    rvv_frame_addr(RVV_T0, off);
    rvv_store_group(g, RVV_T0, restype);
    return off;
}

static void rvv_op1_emit(int opc, CType *restype)
{
    const RvvOp *e;
    CType at;
    int off, r2, vs1;
    rvv_check_v();
    e = rvv_find(rvv_op1_tab, RVV_NTAB(rvv_op1_tab), opc, "unary");
    at = vtop->type;
    if (e->kind == RVK_GEN) {
        /* the result is generated; the operand value is not read */
        rvv_op_cfg(restype, restype, 0);
        rvv_insn(e->f6, e->f3, RVV_GR, 0, e->aux, 1);
        off = rvv_store_result(RVV_GR, restype, &r2);
        rvv_set_slot(vtop, restype, off, r2);
        return;
    }
    rvv_load_op(vtop, RVV_GA);
    rvv_op_cfg(restype, &at, e->cfgres);
    vs1 = e->aux < 0 ? RVV_GA : e->aux;
    if (e->kind == RVK_VX0)
        rvv_insn(e->f6, 4, RVV_GR, RVV_GA, 0, 1);
    else if (e->kind == RVK_VI)
        rvv_insn(e->f6, 3, RVV_GR, RVV_GA, e->aux & 31, 1);
    else if (e->kind == RVK_V0)
        rvv_insn(e->f6, e->f3, RVV_GR, 0, RVV_GA, 1);
    else
        rvv_insn(e->f6, e->f3, RVV_GR, RVV_GA, vs1, 1);
    off = rvv_store_result(RVV_GR, restype, &r2);
    rvv_set_slot(vtop, restype, off, r2);
}

static void rvv_op1r_emit(int opc, CType *restype)
{
    const RvvOp *e;
    CType at;
    int r;
    rvv_check_v();
    e = rvv_find(rvv_op1r_tab, RVV_NTAB(rvv_op1r_tab), opc, "extract");
    at = vtop->type;
    r = get_reg(e->kind == RVK_RF ? RC_FLOAT : RC_INT);
    rvv_load_op(vtop, RVV_GA);
    rvv_op_cfg(&at, &at, 0);
    o(0x57 | (e->f6 << 26) | (1 << 25) | (RVV_GA << 20)
      | ((e->aux & 31) << 15) | (e->f3 << 12)
      | ((e->kind == RVK_RF ? freg(r) : ireg(r)) << 7));
    vtop->r = r;
    vtop->type = *restype;
}

static void rvv_op2_emit(int opc, CType *restype)
{
    const RvvOp *e;
    CType at;
    int b_hw = 0, off, r2;
    rvv_check_v();
    e = rvv_find(rvv_op2_tab, RVV_NTAB(rvv_op2_tab), opc, "binary");
    if (e->kind == RVK_VX) {
        gv(RC_INT);
        b_hw = ireg(vtop->r & VT_VALMASK);
    } else if (e->kind == RVK_VF) {
        gv(RC_FLOAT);
        b_hw = freg(vtop->r & VT_VALMASK);
    }
    at = vtop[-1].type;
    rvv_load_op(&vtop[-1], RVV_GA);
    if (e->kind == RVK_VV || e->kind == RVK_RED)
        rvv_load_op(&vtop[0], RVV_GB);
    rvv_op_cfg(restype, &at, e->cfgres);
    if (e->kind == RVK_VV || e->kind == RVK_RED)
        rvv_insn(e->f6, e->f3, RVV_GR, RVV_GA, RVV_GB, 1);
    else
        rvv_insn(e->f6, e->f3, RVV_GR, RVV_GA, b_hw, 1);
    off = rvv_store_result(RVV_GR, restype, &r2);
    if (e->kind == RVK_RED) {
        /* a reduction defines element 0 only; zero the rest of
           the result slot so the value is fully specified */
        int es, lanes, total, done;
        rvv_shape(restype, &es, &lanes);
        total = rvv_stored_size(restype);
        rvv_frame_addr(RVV_T0, off);
        done = es;
        while (total - done >= 8) { ES(0x23, 3, RVV_T0, 0, done); done += 8; }
        if (total - done >= 4) { ES(0x23, 2, RVV_T0, 0, done); done += 4; }
        if (total - done >= 2) { ES(0x23, 1, RVV_T0, 0, done); done += 2; }
        if (total - done >= 1) { ES(0x23, 0, RVV_T0, 0, done); }
    }
    vtop--;
    rvv_set_slot(vtop, restype, off, r2);
}

static void rvv_op2i_emit(int opc, int imm, CType *restype)
{
    const RvvOp *e;
    CType at;
    int off, r2;
    rvv_check_v();
    e = rvv_find(rvv_op2i_tab, RVV_NTAB(rvv_op2i_tab), opc, "immediate");
    at = vtop->type;
    rvv_load_op(vtop, RVV_GA);
    rvv_op_cfg(restype, &at, e->cfgres);
    rvv_insn(e->f6, e->f3, RVV_GR, RVV_GA, imm & 31, 1);
    off = rvv_store_result(RVV_GR, restype, &r2);
    rvv_set_slot(vtop, restype, off, r2);
}

static void rvv_op3_emit(int opc, CType *restype)
{
    /* (acc, vs1, vs2): the API's second operand lands in the
       instruction's vs1 field (or is the scalar), the third in the
       vs2 field */
    const RvvOp *e;
    CType at;
    int b_hw = 0, off, r2;
    rvv_check_v();
    e = rvv_find(rvv_op3_tab, RVV_NTAB(rvv_op3_tab), opc, "ternary");
    if (e->kind == RVK_VX) {
        /* the scalar is the middle operand, not vtop: load it
           directly instead of going through gv() */
        int r = get_reg(RC_INT);
        load(r, &vtop[-1]);
        b_hw = ireg(r);
    } else if (e->kind == RVK_VF) {
        int r = get_reg(RC_FLOAT);
        load(r, &vtop[-1]);
        b_hw = freg(r);
    }
    at = vtop[-2].type; /* the accumulator carries the vector type */
    rvv_load_op(&vtop[-2], RVV_GR); /* the accumulator */
    if (e->kind == RVK_VV)
        rvv_load_op(&vtop[-1], RVV_GB);
    rvv_load_op(&vtop[0], RVV_GA);
    rvv_op_cfg(restype, &at, e->cfgres);
    rvv_insn(e->f6, e->f3, RVV_GR, RVV_GA,
             e->kind == RVK_VV ? RVV_GB : b_hw, 1);
    off = rvv_store_result(RVV_GR, restype, &r2);
    vtop -= 2;
    rvv_set_slot(vtop, restype, off, r2);
}

static void rvv_splat_emit(int opc, CType *restype)
{
    const RvvOp *e;
    RvvCfg c;
    int off, r2;
    rvv_check_v();
    e = rvv_find(rvv_splat_tab, RVV_NTAB(rvv_splat_tab), opc, "splat");
    if (!rvv_cfg_for(restype, &c))
        tcc_error("rvv: vector type is too wide for one register group "
                  "at this -march minimum VLEN");
    rvv_set_cfg(&c);
    if (e->kind == RVK_SF) {
        gv(RC_FLOAT);
        rvv_insn(e->f6, 5, RVV_GR, 0, freg(vtop->r & VT_VALMASK), 1);
    } else if ((vtop->r & VT_VALMASK) == VT_CONST && !(vtop->r & VT_SYM)
               && vtop->c.i >= -16 && vtop->c.i <= 15) {
        rvv_insn(e->f6, 3, RVV_GR, 0, (int)vtop->c.i & 31, 1); /* vmv.v.i */
    } else {
        gv(RC_INT);
        rvv_insn(e->f6, 4, RVV_GR, 0, ireg(vtop->r & VT_VALMASK), 1);
    }
    off = rvv_store_result(RVV_GR, restype, &r2);
    rvv_set_slot(vtop, restype, off, r2);
}

static void rvv_op1m2_emit(int opc, CType *restype)
{
    /* (mask, passthrough, a): the masked form of a unary builtin,
       plus the lane-0 insert specials (vmv.s.x / vfmv.s.f), whose
       third operand is a scalar and which ignore the mask slot */
    const RvvOp *e;
    CType at;
    int off, r2, vs1;
    rvv_check_v();
    e = rvv_lookup(rvv_op1m2_tab, RVV_NTAB(rvv_op1m2_tab), opc);
    if (e && (e->kind == RVK_SX1 || e->kind == RVK_SF1)) {
        int r = get_reg(e->kind == RVK_SF1 ? RC_FLOAT : RC_INT);
        load(r, vtop);
        at = vtop[-1].type;
        rvv_load_op(&vtop[-1], RVV_GR); /* the passthrough */
        rvv_op_cfg(restype, &at, e->cfgres);
        rvv_insn(e->f6, e->f3, RVV_GR, 0,
                 e->kind == RVK_SF1 ? freg(r) : ireg(r), 1);
        off = rvv_store_result(RVV_GR, restype, &r2);
        vtop -= 2;
        rvv_set_slot(vtop, restype, off, r2);
        return;
    }
    if (!e)
        e = rvv_find(rvv_op1_tab, RVV_NTAB(rvv_op1_tab), opc,
                     "masked unary");
    at = vtop->type;
    rvv_load_op(&vtop[-2], 0); /* the mask must sit in v0 */
    rvv_load_op(&vtop[-1], RVV_GR); /* the passthrough */
    if (e->kind == RVK_GEN) {
        rvv_op_cfg(restype, restype, 0);
        rvv_insn(e->f6, e->f3, RVV_GR, 0, e->aux, 0);
    } else {
        rvv_load_op(&vtop[0], RVV_GA);
        rvv_op_cfg(restype, &at, e->cfgres);
        if (e->kind == RVK_VX0)
            rvv_insn(e->f6, 4, RVV_GR, RVV_GA, 0, 0);
        else if (e->kind == RVK_VI)
            rvv_insn(e->f6, 3, RVV_GR, RVV_GA, e->aux & 31, 0);
        else {
            vs1 = e->aux < 0 ? RVV_GA : e->aux;
            rvv_insn(e->f6, e->f3, RVV_GR, RVV_GA, vs1, 0);
        }
    }
    off = rvv_store_result(RVV_GR, restype, &r2);
    vtop -= 2;
    rvv_set_slot(vtop, restype, off, r2);
}

static void rvv_opm_emit(int opc, CType *restype)
{
    /* (mask, a, b): the destination is written directly, with no
       passthrough (merges, carry forms, masked compares) */
    const RvvOp *e;
    CType at;
    int b_hw = 0, off, r2;
    rvv_check_v();
    e = rvv_find(rvv_opm_tab, RVV_NTAB(rvv_opm_tab), opc, "masked");
    if (e->kind == RVK_VVC) {
        /* vcompress: (mask slot unused, data, mask value); the
           mask value's packed bits are read from a vector group */
        at = vtop[-1].type;
        rvv_load_op(&vtop[-1], RVV_GA);
        rvv_load_op(&vtop[0], RVV_GB);
        rvv_op_cfg(restype, &at, e->cfgres);
        rvv_insn(e->f6, e->f3, RVV_GR, RVV_GA, RVV_GB, 1);
        off = rvv_store_result(RVV_GR, restype, &r2);
        vtop -= 2;
        rvv_set_slot(vtop, restype, off, r2);
        return;
    }
    if (e->kind == RVK_VX) {
        gv(RC_INT);
        b_hw = ireg(vtop->r & VT_VALMASK);
    } else if (e->kind == RVK_VF) {
        gv(RC_FLOAT);
        b_hw = freg(vtop->r & VT_VALMASK);
    }
    at = vtop[-1].type;
    rvv_load_op(&vtop[-2], 0);
    rvv_load_op(&vtop[-1], RVV_GA);
    if (e->kind == RVK_VV)
        rvv_load_op(&vtop[0], RVV_GB);
    rvv_op_cfg(restype, &at, e->cfgres);
    if (e->kind == RVK_VV)
        rvv_insn(e->f6, e->f3, RVV_GR, RVV_GA, RVV_GB, 0);
    else
        rvv_insn(e->f6, e->f3, RVV_GR, RVV_GA, b_hw, 0);
    off = rvv_store_result(RVV_GR, restype, &r2);
    vtop -= 2;
    rvv_set_slot(vtop, restype, off, r2);
}

static void rvv_opm2_emit(int opc, CType *restype)
{
    /* (mask, passthrough, a, b): the masked form of a binary or
       multiply-accumulate builtin; the opcode space is shared
       with the unmasked tables */
    const RvvOp *e;
    CType at;
    int b_hw = 0, off, r2, from3 = 0;
    rvv_check_v();
    e = rvv_lookup(rvv_op2_tab, RVV_NTAB(rvv_op2_tab), opc);
    if (!e) {
        e = rvv_lookup(rvv_op3_tab, RVV_NTAB(rvv_op3_tab), opc);
        from3 = 1;
    }
    if (!e)
        tcc_error("rvv: unsupported masked vector builtin (opcode %d)",
                  opc);
    if (from3 && e->kind == RVK_VX) {
        /* a ternary scalar sits in the middle, like the
           unmasked ternary form */
        int r = get_reg(RC_INT);
        load(r, &vtop[-1]);
        b_hw = ireg(r);
    } else if (from3 && e->kind == RVK_VF) {
        int r = get_reg(RC_FLOAT);
        load(r, &vtop[-1]);
        b_hw = freg(r);
    } else if (e->kind == RVK_VX) {
        gv(RC_INT);
        b_hw = ireg(vtop->r & VT_VALMASK);
    } else if (e->kind == RVK_VF) {
        gv(RC_FLOAT);
        b_hw = freg(vtop->r & VT_VALMASK);
    }
    /* the configuration follows the first vector operand: for
       ternary forms that is the passthrough (the accumulator),
       for binary forms operand a */
    at = from3 ? vtop[-2].type : vtop[-1].type;
    rvv_load_op(&vtop[-3], 0); /* the mask */
    rvv_load_op(&vtop[-2], RVV_GR); /* the passthrough */
    if (from3) {
        /* ternary field order: the second operand goes to the
           vs1 field, the third to the vs2 field */
        if (e->kind == RVK_VV)
            rvv_load_op(&vtop[-1], RVV_GB);
        rvv_load_op(&vtop[0], RVV_GA);
    } else {
        rvv_load_op(&vtop[-1], RVV_GA);
        if (e->kind == RVK_VV)
            rvv_load_op(&vtop[0], RVV_GB);
    }
    rvv_op_cfg(restype, &at, e->cfgres);
    if (e->kind == RVK_VV)
        rvv_insn(e->f6, e->f3, RVV_GR, RVV_GA, RVV_GB, 0);
    else
        rvv_insn(e->f6, e->f3, RVV_GR, RVV_GA, b_hw, 0);
    off = rvv_store_result(RVV_GR, restype, &r2);
    vtop -= 3;
    rvv_set_slot(vtop, restype, off, r2);
}

ST_FUNC void gen_vec_op1(int opc, CType *restype)
{
    rvv_op1_emit(opc, restype);
}

ST_FUNC void gen_vec_op1r(int opc, CType *restype)
{
    rvv_op1r_emit(opc, restype);
}

ST_FUNC void gen_vec_op2(int opc, CType *restype)
{
    rvv_op2_emit(opc, restype);
}

ST_FUNC void gen_vec_op2i(int opc, int imm, CType *restype)
{
    rvv_op2i_emit(opc, imm, restype);
}

ST_FUNC void gen_vec_op3(int opc, CType *restype)
{
    rvv_op3_emit(opc, restype);
}

ST_FUNC void gen_vec_splat(int opc, CType *restype)
{
    rvv_splat_emit(opc, restype);
}

ST_FUNC void gen_vec_op1m(int opc, CType *restype)
{
    (void)opc; (void)restype;
    tcc_error("rvv: this masked builtin form is not supported");
}

ST_FUNC void gen_vec_op1m2(int opc, CType *restype)
{
    rvv_op1m2_emit(opc, restype);
}

ST_FUNC void gen_vec_opm(int opc, CType *restype)
{
    rvv_opm_emit(opc, restype);
}

ST_FUNC void gen_vec_opm2(int opc, CType *restype)
{
    rvv_opm2_emit(opc, restype);
}

ST_FUNC void gen_vec_op3m(int opc, CType *restype)
{
    (void)opc; (void)restype;
    tcc_error("rvv: this masked builtin form is not supported");
}

ST_FUNC void gen_vec_op3m2(int opc, CType *restype)
{
    (void)opc; (void)restype;
    tcc_error("rvv: this masked builtin form is not supported");
}

ST_FUNC void gen_vec_gather(int opc, int scale, CType *restype)
{
    (void)opc; (void)scale; (void)restype;
    tcc_error("rvv: use __builtin_rvv_vload for indexed loads");
}

ST_FUNC void gen_vec_gatherm(int opc, int scale, CType *restype)
{
    (void)opc; (void)scale; (void)restype;
    tcc_error("rvv: use __builtin_rvv_vload for indexed loads");
}

ST_FUNC void gen_vec_scatter(int opc, int scale)
{
    (void)opc; (void)scale;
    tcc_error("rvv: use __builtin_rvv_vstore for indexed stores");
}

ST_FUNC void gen_vec_scatterm(int opc, int scale)
{
    (void)opc; (void)scale;
    tcc_error("rvv: use __builtin_rvv_vstore for indexed stores");
}

ST_FUNC void gen_vec_loadm(int opc, CType *restype)
{
    (void)opc; (void)restype;
    tcc_error("rvv: masked loads are not part of the fixed-length surface");
}

ST_FUNC void gen_vec_storem(int opc)
{
    (void)opc;
    tcc_error("rvv: masked stores are not part of the fixed-length surface");
}

ST_FUNC void gen_vec_ntstore(int opc)
{
    (void)opc;
    tcc_error("rvv: non-temporal stores have no RVV form");
}

/* memory-form opcodes (shared by the header's vload/vstore
   generics): the aux operand is the byte stride for the strided
   forms, the index vector for the indexed forms */
#define RVVOP_VLSE    1000
#define RVVOP_VSSE    1001
#define RVVOP_VLUXEI  1002
#define RVVOP_VLOXEI  1003
#define RVVOP_VSUXEI  1004
#define RVVOP_VSOXEI  1005

ST_FUNC void gen_rvv_vload(int opc, CType *restype)
{
    /* the stack holds (ptr, aux) */
    RvvCfg c;
    int off, r2, base, es, lanes;
    rvv_check_v();
    if (!rvv_cfg_for(restype, &c))
        tcc_error("rvv: vector type is too wide for one register group "
                  "at this -march minimum VLEN");
    rvv_shape(restype, &es, &lanes);
    if (opc == RVVOP_VLSE) {
        int stride;
        gv(RC_INT); /* the stride */
        stride = ireg(vtop->r & VT_VALMASK);
        vswap();
        gv(RC_INT); /* the base pointer */
        base = ireg(vtop->r & VT_VALMASK);
        rvv_set_cfg(&c);
        o(0x07 | (1 << 25) | (2 << 26) | (stride << 20)
          | (rvv_width_code(es) << 12) | (base << 15) | (RVV_GR << 7));
    } else if (opc == RVVOP_VLUXEI || opc == RVVOP_VLOXEI) {
        int ies, ilanes;
        rvv_shape(&vtop->type, &ies, &ilanes); /* the index vector */
        rvv_load_op(vtop, RVV_GA);
        vswap();
        gv(RC_INT);
        base = ireg(vtop->r & VT_VALMASK);
        rvv_set_cfg(&c);
        o(0x07 | (1 << 25) | ((opc == RVVOP_VLUXEI ? 1 : 3) << 26)
          | (RVV_GA << 20) | (rvv_width_code(ies) << 12)
          | (base << 15) | (RVV_GR << 7));
    } else
        tcc_error("rvv: this load form is not supported");
    off = rvv_store_result(RVV_GR, restype, &r2);
    vtop -= 1;
    rvv_set_slot(vtop, restype, off, r2);
}

ST_FUNC void gen_rvv_vstore(int opc)
{
    /* the stack holds (ptr, aux, vec) */
    CType vt = vtop->type;
    RvvCfg c;
    int base, es, lanes;
    rvv_check_v();
    if (!rvv_cfg_for(&vt, &c))
        tcc_error("rvv: vector type is too wide for one register group "
                  "at this -march minimum VLEN");
    rvv_shape(&vt, &es, &lanes);
    if (opc == RVVOP_VSSE) {
        int stride;
        rvv_load_op(vtop, RVV_GA); /* the data */
        vtop--;
        gv(RC_INT); /* the stride */
        stride = ireg(vtop->r & VT_VALMASK);
        vswap();
        gv(RC_INT); /* the base pointer */
        base = ireg(vtop->r & VT_VALMASK);
        rvv_set_cfg(&c);
        o(0x27 | (1 << 25) | (2 << 26) | (stride << 20)
          | (rvv_width_code(es) << 12) | (base << 15) | (RVV_GA << 7));
        vtop -= 2;
    } else if (opc == RVVOP_VSUXEI || opc == RVVOP_VSOXEI) {
        int ies, ilanes;
        rvv_shape(&vtop[-1].type, &ies, &ilanes); /* the index vector */
        rvv_load_op(vtop, RVV_GA);     /* the data */
        rvv_load_op(&vtop[-1], RVV_GB); /* the indices */
        vtop -= 2;
        gv(RC_INT); /* the base pointer */
        base = ireg(vtop->r & VT_VALMASK);
        rvv_set_cfg(&c);
        o(0x27 | (1 << 25) | ((opc == RVVOP_VSUXEI ? 1 : 3) << 26)
          | (RVV_GB << 20) | (rvv_width_code(ies) << 12)
          | (base << 15) | (RVV_GA << 7));
        vtop--;
    } else
        tcc_error("rvv: this store form is not supported");
}

/* expand the mask in v0 into an integer vector of the operand
   shape in group RVV_GR: -1 where the mask bit is set, else 0 */
static void rvv_expand_mask(CType *t)
{
    RvvCfg c;
    if (!rvv_cfg_for(t, &c))
        tcc_error("rvv: vector type is too wide for one register group "
                  "at this -march minimum VLEN");
    rvv_set_cfg(&c);
    rvv_insn(0x17, 3, RVV_GA, 0, 0, 1); /* vmv.v.i GA, 0 */
    EI(0x13, 0, RVV_T2, 0, -1); /* addi t2, x0, -1 */
    rvv_insn(0x17, 4, RVV_GB, 0, RVV_T2, 1); /* vmv.v.x GB, t2 */
    rvv_insn(0x17, 0, RVV_GR, RVV_GA, RVV_GB, 0); /* vmerge.vvm */
}

static void rvv_opv_cmp(int opc, int swap)
{
    const RvvOp *e;
    CType t;
    int off, r2;
    rvv_check_v();
    e = rvv_find(rvv_op2_tab, RVV_NTAB(rvv_op2_tab), opc, "compare");
    t = vtop[-1].type;
    rvv_load_op(&vtop[-1], RVV_GA);
    rvv_load_op(&vtop[0], RVV_GB);
    rvv_op_cfg(&t, &t, 0);
    if (swap)
        rvv_insn(e->f6, e->f3, 0, RVV_GB, RVV_GA, 1);
    else
        rvv_insn(e->f6, e->f3, 0, RVV_GA, RVV_GB, 1);
    rvv_expand_mask(&t);
    off = rvv_store_result(RVV_GR, &t, &r2);
    vtop--;
    rvv_set_slot(vtop, &t, off, r2);
}

ST_FUNC void gen_opv(int op)
{
    CType *lt = &vtop[-1].type;
    int bt = lt->ref->type.t & VT_BTYPE;
    int fl = bt == VT_FLOAT || bt == VT_DOUBLE;
    int bvec = (vtop[0].type.t & VT_VECTOR) != 0;
    if (fl) {
        switch (op) {
        case '+': rvv_op2_emit(RVVOP_VFADD_VV, lt); break;
        case '-': rvv_op2_emit(RVVOP_VFSUB_VV, lt); break;
        case '*': rvv_op2_emit(RVVOP_VFMUL_VV, lt); break;
        case '/': rvv_op2_emit(RVVOP_VFDIV_VV, lt); break;
        case '%':
            tcc_error("rvv: floating-point vector remainder has no instruction");
            break;
        case TOK_EQ: rvv_opv_cmp(RVVOP_VMFEQ_VV, 0); break;
        case TOK_NE: rvv_opv_cmp(RVVOP_VMFNE_VV, 0); break;
        case TOK_LT: rvv_opv_cmp(RVVOP_VMFLT_VV, 0); break;
        case TOK_LE: rvv_opv_cmp(RVVOP_VMFLE_VV, 0); break;
        case TOK_GT: rvv_opv_cmp(RVVOP_VMFLT_VV, 1); break;
        case TOK_GE: rvv_opv_cmp(RVVOP_VMFLE_VV, 1); break;
        default:
            tcc_error("rvv: this vector operation is not supported");
        }
        return;
    }
    switch (op) {
    case '+': rvv_op2_emit(RVVOP_VADD_VV, lt); break;
    case '-': rvv_op2_emit(RVVOP_VSUB_VV, lt); break;
    case '*': rvv_op2_emit(RVVOP_VMUL_VV, lt); break;
    case '/': rvv_op2_emit(RVVOP_VDIV_VV, lt); break;
    case TOK_UDIV: rvv_op2_emit(RVVOP_VDIVU_VV, lt); break;
    case '%': rvv_op2_emit(RVVOP_VREM_VV, lt); break;
    case TOK_UMOD: rvv_op2_emit(RVVOP_VREMU_VV, lt); break;
    case '&': rvv_op2_emit(RVVOP_VAND_VV, lt); break;
    case '|': rvv_op2_emit(RVVOP_VOR_VV, lt); break;
    case '^': rvv_op2_emit(RVVOP_VXOR_VV, lt); break;
    case TOK_SHL:
        rvv_op2_emit(bvec ? RVVOP_VSLL_VV : RVVOP_VSLL_VX, lt);
        break;
    case TOK_SHR:
        rvv_op2_emit(bvec ? RVVOP_VSRL_VV : RVVOP_VSRL_VX, lt);
        break;
    case TOK_SAR:
        rvv_op2_emit(bvec ? RVVOP_VSRA_VV : RVVOP_VSRA_VX, lt);
        break;
    case TOK_EQ: rvv_opv_cmp(RVVOP_VMSEQ_VV, 0); break;
    case TOK_NE: rvv_opv_cmp(RVVOP_VMSNE_VV, 0); break;
    case TOK_LT: rvv_opv_cmp(RVVOP_VMSLT_VV, 0); break;
    case TOK_LE: rvv_opv_cmp(RVVOP_VMSLE_VV, 0); break;
    case TOK_GT: rvv_opv_cmp(RVVOP_VMSLT_VV, 1); break;
    case TOK_GE: rvv_opv_cmp(RVVOP_VMSLE_VV, 1); break;
    case TOK_ULT: rvv_opv_cmp(RVVOP_VMSLTU_VV, 0); break;
    case TOK_ULE: rvv_opv_cmp(RVVOP_VMSLEU_VV, 0); break;
    case TOK_UGT: rvv_opv_cmp(RVVOP_VMSLTU_VV, 1); break;
    case TOK_UGE: rvv_opv_cmp(RVVOP_VMSLEU_VV, 1); break;
    default:
        tcc_error("rvv: this vector operation is not supported");
    }
}

ST_FUNC void gen_vec_neg(void)
{
    int bt = vtop->type.ref->type.t & VT_BTYPE;
    if (bt == VT_FLOAT || bt == VT_DOUBLE)
        rvv_op1_emit(RVVOP_VFNEG, &vtop->type);
    else
        rvv_op1_emit(RVVOP_VNEG_I, &vtop->type);
}

ST_FUNC void gen_vec_not(void)
{
    rvv_op1_emit(RVVOP_VNOT_I, &vtop->type);
}
#endif
