/*
 *  A64 code generator for TCC
 *
 *  Copyright (c) 2014-2015 Edmund Grimley Evans
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright
 * notice and this notice are preserved.  This file is offered as-is,
 * without any warranty.
 */

#ifdef TARGET_DEFS_ONLY

// Number of registers available to allocator:
#define NB_REGS 33 // x0-x18, x30, v0-v7, x19-x23 (regcache only)

#define TREG_R(x) (x) // x = 0..18
#define TREG_R30  19
#define TREG_F(x) (x + 20) // x = 0..7

// Register classes sorted from more general to more precise:
#define RC_INT (1 << 0)
#define RC_FLOAT (1 << 1)
#define RC_R(x) (1 << (2 + (x))) // x = 0..18
#define RC_R30  (1 << 21)
#define RC_F(x) (1 << (22 + (x))) // x = 0..7

#define RC_IRET (RC_R(0)) // int return register class
#define RC_FRET (RC_F(0)) // float return register class
#define RC_VEC (1 << 30) // vector values live in v0-v7, the FP/SIMD registers

/* this backend implements the generic vector layer hooks
   (gen_opv, gen_vec_*) used by tccgen.c */
#define TCC_VEC_HOOKS

#define REG_IRET (TREG_R(0)) // int return register number
#define REG_FRET (TREG_F(0)) // float return register number

#define PTR_SIZE 8

#define LDOUBLE_SIZE 16
#define LDOUBLE_ALIGN 16

#define MAX_ALIGN 16

#if !defined(TCC_TARGET_MACHO) && !defined(TCC_TARGET_PE)
#define CHAR_IS_UNSIGNED
#endif

/* define if return values need to be extended explicitely
   at caller side (for interfacing with non-TCC compilers) */
#define PROMOTE_RET

/* Hardware x19-x23 have indices past the float registers in the
   allocator's numbering; intr() maps the indices back to the
   hardware numbers.  (The TREG_X* names in arm64-asm.c already use
   the hardware numbering, hence CS here.)  They carry class 0 in
   reg_classes, so the expression allocator never hands them out. */
#define TREG_CS19 28
#define TREG_CS20 29
#define TREG_CS21 30
#define TREG_CS22 31
#define TREG_CS23 32

#ifndef TCC_TARGET_PE
/* Dedicated registers for the tccgen.c local-variable register cache:
   x19-x23, callee-saved under AAPCS64.  The PE target is not
   enabled: prolog saves there would need unwind data entries. */
#define REGCACHE_NREGS 5
#define REGCACHE_REGS regcache_regs
ST_DATA const int regcache_regs[REGCACHE_NREGS];
ST_FUNC void regcache_flush_all(void);
ST_FUNC void regcache_disable(void);
ST_FUNC int regcache_used_mask(void);
#endif
/******************************************************/
#else /* ! TARGET_DEFS_ONLY */
/******************************************************/
#define USING_GLOBALS
#include "tcc.h"
#include <assert.h>

ST_DATA const char * const target_machine_defs =
    "__aarch64__\0"
#if defined(TCC_TARGET_MACHO)
    "__arm64__\0"
#endif
    "__AARCH64EL__\0"
    ;

ST_DATA const int reg_classes[NB_REGS] = {
  RC_INT | RC_R(0),
  RC_INT | RC_R(1),
  RC_INT | RC_R(2),
  RC_INT | RC_R(3),
  RC_INT | RC_R(4),
  RC_INT | RC_R(5),
  RC_INT | RC_R(6),
  RC_INT | RC_R(7),
  RC_INT | RC_R(8),
  RC_INT | RC_R(9),
  RC_INT | RC_R(10),
  RC_INT | RC_R(11),
  RC_INT | RC_R(12),
  RC_INT | RC_R(13),
  RC_INT | RC_R(14),
  RC_INT | RC_R(15),
  RC_R(16), /* reserved for generator (arm64_sym()) */
  RC_R(17), /* reserved for generator (arm64_sym()) */
  RC_R(18), /* reserved on Windows/OSX */
  RC_R30,   /* not in RC_INT as we make special use of x30 */
  RC_FLOAT | RC_F(0) | RC_VEC,
  RC_FLOAT | RC_F(1) | RC_VEC,
  RC_FLOAT | RC_F(2) | RC_VEC,
  RC_FLOAT | RC_F(3) | RC_VEC,
  RC_FLOAT | RC_F(4) | RC_VEC,
  RC_FLOAT | RC_F(5) | RC_VEC,
  RC_FLOAT | RC_F(6) | RC_VEC,
  RC_FLOAT | RC_F(7) | RC_VEC,
  0, /* x19: regcache only, never allocated */
  0, /* x20: regcache only, never allocated */
  0, /* x21: regcache only, never allocated */
  0, /* x22: regcache only, never allocated */
  0  /* x23: regcache only, never allocated */
};

#if defined(CONFIG_TCC_BCHECK)
static addr_t func_bound_offset;
static unsigned long func_bound_ind;
ST_DATA int func_bound_add_epilog;
#endif

#define IS_FREG(x) ((x) >= TREG_F(0) && (x) <= TREG_F(7))

static uint32_t intr(int r)
{
    assert((TREG_R(0) <= r && r <= TREG_R30) ||
           (TREG_CS19 <= r && r <= TREG_CS23));
    if (r >= TREG_CS19)
        return r - TREG_CS19 + 19; /* regcache index -> hardware x19-x23 */
    return r < TREG_R30 ? r : 30;
}

static uint32_t fltr(int r)
{
    assert(TREG_F(0) <= r && r <= TREG_F(7));
    return r - TREG_F(0);
}

// Add an instruction to text section:
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

static int arm64_encode_bimm64(uint64_t x)
{
    int neg = x & 1;
    int rep, pos, len;

    if (neg)
        x = ~x;
    if (!x)
        return -1;

    if (x >> 2 == (x & (((uint64_t)1 << (64 - 2)) - 1)))
        rep = 2, x &= ((uint64_t)1 << 2) - 1;
    else if (x >> 4 == (x & (((uint64_t)1 << (64 - 4)) - 1)))
        rep = 4, x &= ((uint64_t)1 <<  4) - 1;
    else if (x >> 8 == (x & (((uint64_t)1 << (64 - 8)) - 1)))
        rep = 8, x &= ((uint64_t)1 <<  8) - 1;
    else if (x >> 16 == (x & (((uint64_t)1 << (64 - 16)) - 1)))
        rep = 16, x &= ((uint64_t)1 << 16) - 1;
    else if (x >> 32 == (x & (((uint64_t)1 << (64 - 32)) - 1)))
        rep = 32, x &= ((uint64_t)1 << 32) - 1;
    else
        rep = 64;

    pos = 0;
    if (!(x & (((uint64_t)1 << 32) - 1))) x >>= 32, pos += 32;
    if (!(x & (((uint64_t)1 << 16) - 1))) x >>= 16, pos += 16;
    if (!(x & (((uint64_t)1 <<  8) - 1))) x >>= 8, pos += 8;
    if (!(x & (((uint64_t)1 <<  4) - 1))) x >>= 4, pos += 4;
    if (!(x & (((uint64_t)1 <<  2) - 1))) x >>= 2, pos += 2;
    if (!(x & (((uint64_t)1 <<  1) - 1))) x >>= 1, pos += 1;

    len = 0;
    if (!(~x & (((uint64_t)1 << 32) - 1))) x >>= 32, len += 32;
    if (!(~x & (((uint64_t)1 << 16) - 1))) x >>= 16, len += 16;
    if (!(~x & (((uint64_t)1 << 8) - 1))) x >>= 8, len += 8;
    if (!(~x & (((uint64_t)1 << 4) - 1))) x >>= 4, len += 4;
    if (!(~x & (((uint64_t)1 << 2) - 1))) x >>= 2, len += 2;
    if (!(~x & (((uint64_t)1 << 1) - 1))) x >>= 1, len += 1;

    if (x)
        return -1;
    if (neg) {
        pos = (pos + len) & (rep - 1);
        len = rep - len;
    }
    return ((0x1000 & rep << 6) | (((rep - 1) ^ 31) << 1 & 63) |
            ((rep - pos) & (rep - 1)) << 6 | (len - 1));
}

static uint32_t arm64_movi(int r, uint64_t x)
{
    uint64_t m = 0xffff;
    int e;
    if (!(x & ~m))
        return ARM64_MOVZ | r | x << 5; // movz w(r),#(x)
    if (!(x & ~(m << 16)))
        return (ARM64_MOVZ | ARM64_HW(1) | r | x >> 11); // movz w(r),#(x >> 16),lsl #16
    if (!(x & ~(m << 32)))
        return (ARM64_MOVZ64 | ARM64_HW(2) | r | x >> 27); // movz x(r),#(x >> 32),lsl #32
    if (!(x & ~(m << 48)))
        return (ARM64_MOVZ64 | ARM64_HW(3) | r | x >> 43); // movz x(r),#(x >> 48),lsl #48
    if ((x & ~m) == m << 16)
        return (ARM64_MOVN | r |
                (~x << 5 & 0x1fffe0)); // movn w(r),#(~x)
    if ((x & ~(m << 16)) == m)
        return (ARM64_MOVN | ARM64_HW(1) | r |
                (~x >> 11 & 0x1fffe0)); // movn w(r),#(~x >> 16),lsl #16
    if (!~(x | m))
        return (ARM64_MOVN64 | r |
                (~x << 5 & 0x1fffe0)); // movn x(r),#(~x)
    if (!~(x | m << 16))
        return (ARM64_MOVN64 | ARM64_HW(1) | r |
                (~x >> 11 & 0x1fffe0)); // movn x(r),#(~x >> 16),lsl #16
    if (!~(x | m << 32))
        return (ARM64_MOVN64 | ARM64_HW(2) | r |
                (~x >> 27 & 0x1fffe0)); // movn x(r),#(~x >> 32),lsl #32
    if (!~(x | m << 48))
        return (ARM64_MOVN64 | ARM64_HW(3) | r |
                (~x >> 43 & 0x1fffe0)); // movn x(r),#(~x >> 32),lsl #32
    if (!(x >> 32) && (e = arm64_encode_bimm64(x | x << 32)) >= 0)
        return (ARM64_ORR_IMM | r | (uint32_t)e << 10); // movi w(r),#(x)
    if ((e = arm64_encode_bimm64(x)) >= 0)
        return (ARM64_ORR_IMM | ARM64_SF(1) | r | (uint32_t)e << 10); // movi x(r),#(x)
    return 0;
}

static void arm64_movimm(int r, uint64_t x)
{
    uint32_t i;
    if ((i = arm64_movi(r, x)))
        o(i); // a single MOV
    else {
        // MOVZ/MOVN and 1-3 MOVKs
        int z = 0, m = 0;
        uint32_t mov1 = ARM64_MOVZ64; // movz
        uint64_t x1 = x;
        for (i = 0; i < 64; i += 16) {
            z += !(x >> i & 0xffff);
            m += !(~x >> i & 0xffff);
        }
        if (m > z) {
            x1 = ~x;
            mov1 = ARM64_MOVN64; // movn
        }
        for (i = 0; i < 64; i += 16)
            if (x1 >> i & 0xffff) {
                o(mov1 | r | (x1 >> i & 0xffff) << 5 | i << 17);
                // movz/movn x(r),#(*),lsl #(i)
                break;
            }
        for (i += 16; i < 64; i += 16)
            if (x1 >> i & 0xffff)
                o(ARM64_MOVK | ARM64_SF(1) | r | (x >> i & 0xffff) << 5 | i << 17);
                // movk x(r),#(*),lsl #(i)
    }
}

// Patch all branches in list pointed to by t to branch to a:
ST_FUNC void gsym_addr(int t_, int a_)
{
    uint32_t t = t_;
    uint32_t a = a_;
    while (t) {
        unsigned char *ptr = cur_text_section->data + t;
        uint32_t next = read32le(ptr);
        if (a - t + 0x8000000 >= 0x10000000)
            tcc_error("branch out of range");
        write32le(ptr, (a - t == 4 ? ARM64_NOP :
                        ARM64_B | ((a - t) >> 2 & 0x3ffffff)));
        t = next;
    }
}

static int arm64_type_size(int t)
{
    /*
     * case values are in increasing order (from 1 to 11).
     * which 'may' help compiler optimizers. See tcc.h
     */
    switch (t & VT_BTYPE) {
    case VT_BYTE: return 0;
    case VT_SHORT: return 1;
    case VT_INT: return 2;
    case VT_LLONG: return 3;
    case VT_PTR: return 3;
    case VT_FUNC: return 3;
    case VT_STRUCT: return 3;
    case VT_FLOAT: return 2;
    case VT_DOUBLE: return 3;
    case VT_LDOUBLE: return 4;
    case VT_BOOL: return 0;
    }
    assert(0);
    return 0;
}

/* the sz operand of the FP/SIMD load/store forms for a value of
   type t: scalar floats by their type, vectors by their total
   size (the d view for 8 bytes, the q view for 16) */
static int arm64_ldst_sz(CType *t)
{
    if (t->t & VT_VECTOR) {
        int a;
        return type_size(t, &a) == 16 ? 4 : 3;
    }
    return arm64_type_size(t->t);
}

static void arm64_spoff(int reg, uint64_t off)
{
    uint32_t sub = off >> 63;
    if (sub)
        off = -off;
    if (off < 4096)
        o(ARM64_ADD_IMM | ARM64_SF(1) | ARM64_RN(31) | ARM64_RD(reg) | ARM64_IMM12(off));
    else {
        arm64_movimm(30, off);
        // (add|sub) x(reg),sp,x30: in the shifted-register form, Rn 31 is xzr
        o(ARM64_ADD_EXT | ARM64_SF(1) | ARM64_RM(30) | ARM64_EXTEND_UXTX |
          ARM64_RN(31) | ARM64_RD(reg) | (sub << 30));
    }
}

static uint64_t arm64_check_offset(int sz_, uint64_t off)
{
#ifndef TCC_TARGET_PE
    uint32_t sz = sz_;
    uint64_t scaled_mask = 0xffful << sz;
    if (!(off & ~scaled_mask) || (off < 256 || -off <= 256))
        return ~0ull;
    if (off & scaled_mask)
        return scaled_mask;
    if (off & 0x1fful)
        return 0x1fful;
#endif
    return 0;
}

static void arm64_ldrx(int sg, int sz_, int dst, int bas, uint64_t off)
{
    uint32_t sz = sz_;
    uint64_t scaled_mask = 0xffful << sz;
    if (sz >= 2)
        sg = 0;
    if (!(off & ~scaled_mask))
        o(ARM64_LDR_B | dst | bas << 5 | off << (10 - sz) |
          (uint32_t)!!sg << 23 | sz << 30); // ldr(*) x(dst),[x(bas),#(off)]
    else if (off < 256 || -off <= 256)
        o(ARM64_LDUR_B | dst | bas << 5 | (off & 511) << 12 |
          (uint32_t)!!sg << 23 | sz << 30); // ldur(*) x(dst),[x(bas),#(off)]
    else {
        arm64_movimm(30, off); // use x30 for offset
        o(ARM64_LDR_B_REG | dst | bas << 5 | (uint32_t)30 << 16 |
          (uint32_t)(!!sg + 1) << 22 | sz << 30); // ldr(*) x(dst),[x(bas),x30]
    }
}

static void arm64_ldrv(int sz_, int dst, int bas, uint64_t off)
{
    uint32_t sz = sz_;
    uint64_t scaled_mask = 0xffful << sz;

    if (!(off & ~scaled_mask))
        o(ARM64_LDR_SCALAR | dst | bas << 5 | off << (10 - sz) |
          (sz & 4) << 21 | (sz & 3) << 30); // ldr (s|d|q)(dst),[x(bas),#(off)]
    else if (off < 256 || -off <= 256)
        o(ARM64_LDUR_Q_SIMD | dst | bas << 5 | (off & 511) << 12 |
          (sz & 4) << 21 | (sz & 3) << 30); // ldur (s|d|q)(dst),[x(bas),#(off)]
    else {
        arm64_movimm(30, off); // use x30 for offset
        o(ARM64_LDR_Q_REG | dst | bas << 5 | (uint32_t)30 << 16 |
          sz << 30 | (sz & 4) << 21); // ldr (s|d|q)(dst),[x(bas),x30]
    }
}

static void arm64_ldrs(int reg_, int size)
{
    uint32_t reg = reg_;
    // Use x30 for intermediate value in some cases.
    switch (size) {
    default: assert(0); break;
    case 0:
        /* Can happen with zero size structs */
        break;
    case 1:
        arm64_ldrx(0, 0, reg, reg, 0);
        break;
    case 2:
        arm64_ldrx(0, 1, reg, reg, 0);
        break;
    case 3:
        arm64_ldrx(0, 1, 30, reg, 0);
        arm64_ldrx(0, 0, reg, reg, 2);
        o(0x2a0043c0 | reg | reg << 16); // orr x(reg),x30,x(reg),lsl #16
        break;
    case 4:
        arm64_ldrx(0, 2, reg, reg, 0);
        break;
    case 5:
        arm64_ldrx(0, 2, 30, reg, 0);
        arm64_ldrx(0, 0, reg, reg, 4);
        o(0xaa0083c0 | reg | reg << 16); // orr x(reg),x30,x(reg),lsl #32
        break;
    case 6:
        arm64_ldrx(0, 2, 30, reg, 0);
        arm64_ldrx(0, 1, reg, reg, 4);
        o(0xaa0083c0 | reg | reg << 16); // orr x(reg),x30,x(reg),lsl #32
        break;
    case 7:
        arm64_ldrx(0, 2, 30, reg, 0);
        arm64_ldrx(0, 2, reg, reg, 3);
        o(0x53087c00 | reg | reg << 5); // lsr w(reg), w(reg), #8
        o(0xaa0083c0 | reg | reg << 16); // orr x(reg),x30,x(reg),lsl #32
        break;
    case 8:
        arm64_ldrx(0, 3, reg, reg, 0);
        break;
    case 9:
        arm64_ldrx(0, 0, reg + 1, reg, 8);
        arm64_ldrx(0, 3, reg, reg, 0);
        break;
    case 10:
        arm64_ldrx(0, 1, reg + 1, reg, 8);
        arm64_ldrx(0, 3, reg, reg, 0);
        break;
    case 11:
        arm64_ldrx(0, 2, reg + 1, reg, 7);
        o(0x53087c00 | (reg+1) | (reg+1) << 5); // lsr w(reg+1), w(reg+1), #8
        arm64_ldrx(0, 3, reg, reg, 0);
        break;
    case 12:
        arm64_ldrx(0, 2, reg + 1, reg, 8);
        arm64_ldrx(0, 3, reg, reg, 0);
        break;
    case 13:
        arm64_ldrx(0, 3, reg + 1, reg, 5);
        o(0xd358fc00 | (reg+1) | (reg+1) << 5); // lsr x(reg+1), x(reg+1), #24
        arm64_ldrx(0, 3, reg, reg, 0);
        break;
    case 14:
        arm64_ldrx(0, 3, reg + 1, reg, 6);
        o(0xd350fc00 | (reg+1) | (reg+1) << 5); // lsr x(reg+1), x(reg+1), #16
        arm64_ldrx(0, 3, reg, reg, 0);
        break;
    case 15:
        arm64_ldrx(0, 3, reg + 1, reg, 7);
        o(0xd348fc00 | (reg+1) | (reg+1) << 5); // lsr x(reg+1), x(reg+1), #8
        arm64_ldrx(0, 3, reg, reg, 0);
        break;
    case 16:
        o(0xa9400000 | reg | (reg+1) << 10 | reg << 5);
        // ldp x(reg),x(reg+1),[x(reg)]
        break;
    }
}

static void arm64_strx(int sz_, int dst, int bas, uint64_t off)
{
    uint32_t sz = sz_;
    uint64_t scaled_mask = 0xffful << sz;

    if (!(off & ~scaled_mask))
        o(0x39000000 | dst | bas << 5 | off << (10 - sz) | sz << 30);
        // str(*) x(dst),[x(bas],#(off)]
    else if (off < 256 || -off <= 256)
        o(0x38000000 | dst | bas << 5 | (off & 511) << 12 | sz << 30);
        // stur(*) x(dst),[x(bas],#(off)]
    else {
        arm64_movimm(30, off); // use x30 for offset
        o(0x38206800 | dst | bas << 5 | (uint32_t)30 << 16 | sz << 30);
        // str(*) x(dst),[x(bas),x30]
    }
}

static void arm64_strv(int sz_, int dst, int bas, uint64_t off)
{
    uint32_t sz = sz_;
    uint64_t scaled_mask = 0xffful << sz;

    if (!(off & ~scaled_mask))
        o(0x3d000000 | dst | bas << 5 | off << (10 - sz) |
          (sz & 4) << 21 | (sz & 3) << 30); // str (s|d|q)(dst),[x(bas),#(off)]
    else if (off < 256 || -off <= 256)
        o(0x3c000000 | dst | bas << 5 | (off & 511) << 12 |
          (sz & 4) << 21 | (sz & 3) << 30); // stur (s|d|q)(dst),[x(bas),#(off)]
    else {
        arm64_movimm(30, off); // use x30 for offset
        o(0x3c206800 | dst | bas << 5 | (uint32_t)30 << 16 |
          sz << 30 | (sz & 4) << 21); // str (s|d|q)(dst),[x(bas),x30]
    }
}

static void arm64_reloca(Sym *sym, int reloc_type, uint64_t addend)
{
    greloca(cur_text_section, sym, ind, reloc_type, addend);
}

static void arm64_sym(int r, Sym *sym, addr_t addend)
{
    if (sym->type.t & VT_TLS) {
#ifdef TCC_TARGET_PE
        Sym *s2 = external_global_sym(TOK___tls_index, &int_type);
        int r2 = get_reg(RC_R(16));
        arm64_sym(30, s2, 0);
        o(0xb94003de); // ldr w30, [x30]
        o(0xf9402e40 | r2); // ldr r2, [x18, #88]
        o(0x8b1e0c1e | r2 << 5); // add x30, r2, x30, lsl #3
        o(0xf94003c0 | r); // ldr xr, [x30]
#else
        o(0xd53bd040 | r); /* mrs xr, tpidr_el0 */
#endif
        arm64_reloca(sym, R_AARCH64_TLSLE_ADD_TPREL_HI12, addend);
        o(ARM64_ADD_IMM | ARM64_SF(1) | ARM64_SH(1) | ARM64_RN(r) | ARM64_RD(r)); /* add xr, xr, #0, lsl #12 */
        arm64_reloca(sym, R_AARCH64_TLSLE_ADD_TPREL_LO12, addend);
        o(ARM64_ADD_IMM | ARM64_SF(1) | ARM64_RN(r) | ARM64_RD(r)); /* add xr, xr, #0 */
#ifndef TCC_TARGET_PE /* no elf-style GOT on Windows */
    } else if (!(sym->type.t & VT_STATIC)) {
        arm64_reloca(sym, R_AARCH64_ADR_GOT_PAGE, 0);
        o(ARM64_ADRP | r); /* adrp xr, #sym */
        arm64_reloca(sym, R_AARCH64_LD64_GOT_LO12_NC, 0);
        o(ARM64_LDR_X | ARM64_RN(r) | r); /* ld xr,[xr, #sym] */
        if (addend > 0xffffff) {
            int t = get_reg(RC_R(16));
            arm64_movimm(t, addend); // use xt for addent
            o(ARM64_ADD_REG | ARM64_SF(1) | ARM64_RM(t) | ARM64_RN(r) | r); /* add xr, xr, xt */
        } else {
           if (addend & 0xfff) /* add xr, xr, #addend */
                o(ARM64_ADD_IMM | ARM64_SF(1)
                    | ARM64_RN(r) | r | (addend & 0xfff) << 10);
            addend >>= 12;
            if (addend & 0xfff) /* add xr, xr, #addend, lsl #12 */
                o(ARM64_ADD_IMM | ARM64_SF(1) | ARM64_SH(1)
                    | ARM64_RN(r) | r | (addend & 0xfff) << 10);
        }
#endif
    } else {
        arm64_reloca(sym, R_AARCH64_ADR_PREL_PG_HI21, addend);
        o(ARM64_ADRP | r); /* adrp xr, #sym */
        arm64_reloca(sym, R_AARCH64_ADD_ABS_LO12_NC, addend);
        o(ARM64_ADD_IMM | ARM64_SF(1) | ARM64_RN(r) | r); /* add xr, xr, #sym */
    }
}

static int arm64_type_bytes(CType *t)
{
    int a;
    return type_size(t, &a);
}

static void arm64_load_cmp(int r, SValue *sv);

ST_FUNC void load(int r, SValue *sv)
{
    int svtt = sv->type.t;
    int svr = sv->r & (VT_VALMASK | VT_LVAL | VT_SYM);
    int svrv = svr & VT_VALMASK;
    uint64_t svcul = sv->c.i;
    uint64_t svcoff = (int32_t)sv->c.i;
    int sb = !(svtt & VT_UNSIGNED);
    /* a vector loads through the FP/SIMD path with the d view for
       8 bytes and the q view for 16, like double and long double */
    int sz = arm64_ldst_sz(&sv->type);

    if (svr == (VT_LOCAL | VT_LVAL)) {
        if (IS_FREG(r))
            arm64_ldrv(sz, fltr(r), 29, svcoff);
        else
            arm64_ldrx(sb, sz, intr(r), 29, svcoff);
        return;
    }

    if (svr == (VT_CONST | VT_LVAL)) {
	arm64_movimm (30, svcul);
        if (IS_FREG(r))
            arm64_ldrv(sz, fltr(r), 30, 0);
        else
            arm64_ldrx(sb, sz, intr(r), 30, 0);
        return;
    }

    if (svrv < VT_CONST && (svr & VT_LVAL)) {
        if (IS_FREG(r))
            arm64_ldrv(sz, fltr(r), intr(svrv), 0);
        else
            arm64_ldrx(sb, sz, intr(r), intr(svrv), 0);
        return;
    }

    if (svr == (VT_CONST | VT_LVAL | VT_SYM)) {
        uint64_t mask = arm64_check_offset(sz, svcoff);
        arm64_sym(30, sv->sym, svcoff & ~mask);
        if (IS_FREG(r))
            arm64_ldrv(sz, fltr(r), 30, svcoff & mask);
        else
            arm64_ldrx(sb, sz, intr(r), 30, svcoff & mask);
        return;
    }

    if (svr == (VT_CONST | VT_SYM)) {
        arm64_sym(intr(r), sv->sym, svcul);
        return;
    }

    if (svr == VT_CONST) {
        arm64_movimm(intr(r), sz == 3 ? svcul : (uint32_t)svcul);
        return;
    }

    if (svr < VT_CONST) {
        if (IS_FREG(r) && IS_FREG(svr))
            if (svtt == VT_LDOUBLE ||
                ((svtt & VT_VECTOR) && arm64_type_bytes(&sv->type) == 16))
                o(ARM64_MOV_V16B | fltr(r) | fltr(svr) * 0x10020);
                    // mov v(r).16b,v(svr).16b
            else
                o(ARM64_FMOV_SCALAR | fltr(r) | fltr(svr) << 5); // fmov d(r),d(svr)
        else if (!IS_FREG(r) && !IS_FREG(svr))
            /* copy a 32-bit integer value with a w move, which zeroes
               the upper half of the destination just like a 32-bit
               load does; the unsigned 32-to-64-bit cast emits no code
               and relies on that zeroed upper half */
            o(ARM64_MOV_REG | ARM64_SF((svtt & VT_BTYPE) != VT_INT) |
              intr(r) | intr(svr) << 16); // mov x/w(r),x/w(svr)
        else
            assert(0);
      return;
    }

    if (svr == VT_LOCAL) {
        if (-svcoff < 0x1000)
            o(0xd10003a0 | intr(r) | -svcoff << 10); // sub x(r),x29,#...
        else {
            arm64_movimm(30, -svcoff); // use x30 for offset
            o(0xcb0003a0 | intr(r) | (uint32_t)30 << 16); // sub x(r),x29,x30
        }
        return;
    }

    if (svr == VT_JMP || svr == VT_JMPI) {
        int t = (svr == VT_JMPI);
        arm64_movimm(intr(r), t);
        o(ARM64_B | 2); // b .+8
        gsym(svcul);
        arm64_movimm(intr(r), t ^ 1);
        return;
    }

    if (svr == (VT_LLOCAL | VT_LVAL)) {
        arm64_ldrx(0, 3, 30, 29, svcoff); // use x30 for offset
        if (IS_FREG(r))
            arm64_ldrv(sz, fltr(r), 30, 0);
        else
            arm64_ldrx(sb, sz,
                       intr(r), 30, 0);
        return;
    }

    if (svr == VT_CMP) {
        arm64_load_cmp(r, sv);
        return;
    }

    printf("load(%x, (%x, %x, %lx))\n", r, svtt, sv->r, (long)svcul);
    assert(0);
}

ST_FUNC void store(int r, SValue *sv)
{
    int svtt = sv->type.t;
    int svr = sv->r & (VT_VALMASK | VT_LVAL | VT_SYM);
    int svrv = svr & VT_VALMASK;
    uint64_t svcoff = (int32_t)sv->c.i;
    int sz = arm64_ldst_sz(&sv->type);

    if (svr == (VT_LOCAL | VT_LVAL)) {
        if (IS_FREG(r))
            arm64_strv(sz, fltr(r), 29, svcoff);
        else
            arm64_strx(sz, intr(r), 29, svcoff);
        return;
    }

    if (svr == (VT_CONST | VT_LVAL)) {
	arm64_movimm (30, sv->c.i);
        if (IS_FREG(r))
            arm64_strv(sz, fltr(r), 30, 0);
        else
            arm64_strx(sz, intr(r), 30, 0);
        return;
    }

    if (svrv < VT_CONST && (svr & VT_LVAL)) {
        if (IS_FREG(r))
            arm64_strv(sz, fltr(r), intr(svrv), 0);
        else
            arm64_strx(sz, intr(r), intr(svrv), 0);
        return;
    }

    if (svr == (VT_CONST | VT_LVAL | VT_SYM)) {
        uint64_t mask = arm64_check_offset(sz, svcoff);
        arm64_sym(30, sv->sym, svcoff & ~mask);
        if (IS_FREG(r))
            arm64_strv(sz, fltr(r), 30, svcoff & mask);
        else
            arm64_strx(sz, intr(r), 30, svcoff & mask);
        return;
    }

    printf("store(%x, (%x, %x, %lx))\n", r, svtt, sv->r, (long)svcoff);
    assert(0);
}

static void arm64_gen_bl_or_b(int b)
{
    if ((vtop->r & (VT_VALMASK | VT_LVAL | VT_SYM)) == (VT_CONST | VT_SYM)) {
	arm64_reloca(vtop->sym,
                b ? R_AARCH64_JUMP26 :  R_AARCH64_CALL26, 0);
	o(b ? ARM64_B : ARM64_BL); // b/bl .
    }
    else {
#ifdef CONFIG_TCC_BCHECK
        vtop->r &= ~VT_MUSTBOUND;
#endif
        o((b ? ARM64_BR : ARM64_BLR) | intr(gv(RC_R30)) << 5); // br/blr
    }
}

#if defined CONFIG_TCC_BCHECK || defined TCC_TARGET_PE
static void gen_static_call(int v)
{
    Sym *sym = external_helper_sym(v);
    arm64_reloca(sym, R_AARCH64_CALL26, 0);
    o(ARM64_BL); // bl
}
#endif

#if defined(CONFIG_TCC_BCHECK)
static void gen_bounds_prolog(void)
{
    /* leave some room for bound checking code */
    func_bound_offset = lbounds_section->data_offset;
    func_bound_ind = ind;
    func_bound_add_epilog = 0;
    o(ARM64_NOP);  /* nop -> mov x0, lbound section pointer */
    o(ARM64_NOP);
    o(ARM64_NOP);
    o(ARM64_NOP);  /* nop -> call __bound_local_new */
}

static void gen_bounds_epilog(void)
{
    addr_t saved_ind;
    addr_t *bounds_ptr;
    Sym *sym_data;
    int offset_modified = func_bound_offset != lbounds_section->data_offset;

    if (!offset_modified && !func_bound_add_epilog)
        return;

    /* add end of table info */
    bounds_ptr = section_ptr_add(lbounds_section, sizeof(addr_t));
    *bounds_ptr = 0;

    sym_data = get_sym_ref(&char_pointer_type, lbounds_section,
                           func_bound_offset, PTR_SIZE);

    /* generate bound local allocation */
    if (offset_modified) {
        saved_ind = ind;
        ind = func_bound_ind;
        arm64_sym(0, sym_data, 0);
        gen_static_call(TOK___bound_local_new);
        ind = saved_ind;
    }

    /* generate bound check local freeing */
    o(0xa9bf07e0); /* stp x0, x1, [sp, #-16]! */
    o(0x3c9f0fe0); /* str q0, [sp, #-16]! */
    arm64_sym(0, sym_data, 0);
    gen_static_call(TOK___bound_local_delete);
    o(0x3cc107e0); /* ldr q0, [sp], #16 */
    o(0xa8c107e0); /* ldp x0, x1, [sp], #16 */
}
#endif

static int arm64_hfa_aux(CType *type, int *fsize, int num)
{
    if ((type->t & VT_VECTOR) && !is_float(type->t)) {
        /* a vector is an FP/SIMD member of its total size (float
           element vectors take the is_float branch below, which
           measures the same total size) */
        int a, n = type_size(type, &a);
        if (num >= 4 || (*fsize && *fsize != n))
            return -1;
        *fsize = n;
        return num + 1;
    }
    if (is_float(type->t)) {
        int a, n = type_size(type, &a);
        if (num >= 4 || (*fsize && *fsize != n))
            return -1;
        *fsize = n;
        return num + 1;
    }
    else if ((type->t & VT_BTYPE) == VT_STRUCT) {
        Sym *field;
        if (!IS_UNION(type->t)) {
            int num0 = num;
            for (field = type->ref->next; field; field = field->next) {
                if (field->c != (num - num0) * *fsize)
                    return -1;
                num = arm64_hfa_aux(&field->type, fsize, num);
                if (num == -1)
                    return -1;
            }
            if (type->ref->c != (num - num0) * *fsize)
                return -1;
            return num;
        }
        else { // union
            int num0 = num;
            for (field = type->ref->next; field; field = field->next) {
                int num1 = arm64_hfa_aux(&field->type, fsize, num0);
                if (num1 == -1)
                    return -1;
                num = num1 < num ? num : num1;
            }
            if (type->ref->c != (num - num0) * *fsize)
                return -1;
            return num;
        }
    }
    else if (type->t & VT_ARRAY) { /* handle float array within struct */
        int num1;
        if (!type->ref->c)
            return num;
        num1 = arm64_hfa_aux(&type->ref->type, fsize, num);
        if (num1 == -1 || (num1 != num && type->ref->c > 4))
            return -1;
        num1 = num + type->ref->c * (num1 - num);
        if (num1 > 4)
            return -1;
        return num1;
    }
    return -1;
}

static int arm64_hfa(CType *type, unsigned *fsize)
{
    if ((type->t & VT_BTYPE) == VT_STRUCT) {
        int sz = 0;
        int n = arm64_hfa_aux(type, &sz, 0);
        if (0 < n && n <= 4) {
            if (fsize)
                *fsize = sz;
            return n;
        }
    }
    return 0;
}

static unsigned long arm64_pcs_aux(int variadic, int n, CType **type, unsigned long *a)
{
    int nx = 0; // next integer register
    int nv = 0; // next vector register
    unsigned long ns = 32; // next stack offset
    int i;

    for (i = 0; i < n; i++) {
        int hfa = arm64_hfa(type[i], 0);
        int size, align, bt;

        bt = type[i]->t & VT_BTYPE;
        if (bt == VT_PTR || bt == VT_FUNC)
            size = align = 8;
        else
            size = type_size(type[i], &align);

#if defined(TCC_TARGET_MACHO)
        if (variadic && i == variadic) {
            nx = 8;
            nv = 8;
	}

#elif defined(TCC_TARGET_PE)
        if (variadic && i >= variadic) {
            hfa = 0;
            if (is_float(bt))
                bt = VT_INT, size = align = 8;
        }
#endif
        if (hfa)
            // B.2
            ;
        else if (size > 16) {
            // B.3: replace with pointer
            if (nx < 8)
                a[i] = nx++ << 1 | 1;
            else {
                ns = (ns + 7) & ~7;
                a[i] = ns | 1;
                ns += 8;
            }
            continue;
        }
        else if (bt == VT_STRUCT)
            // B.4
            size = (size + 7) & ~7;

        // C.1
        if ((is_float(bt) || (type[i]->t & VT_VECTOR)) && nv < 8) {
            a[i] = 16 + (nv++ << 1);
            continue;
        }

        // C.2
        if (hfa && nv + hfa <= 8) {
            a[i] = 16 + (nv << 1);
            nv += hfa;
            continue;
        }

        // C.3
        if (hfa) {
            nv = 8;
            size = (size + 7) & ~7;
        }

        // C.4
        if (hfa || bt == VT_LDOUBLE || (type[i]->t & VT_VECTOR)) {
            ns = (ns + 7) & ~7;
            ns = (ns + align - 1) & -align;
        }

        // C.5
        if (bt == VT_FLOAT && !(type[i]->t & VT_VECTOR))
            size = 8;

        // C.6
        if (hfa || is_float(bt) || (type[i]->t & VT_VECTOR)) {
            a[i] = ns;
            ns += size;
            continue;
        }

        // C.7
        if (bt != VT_STRUCT && size <= 8 && nx < 8) {
            a[i] = nx++ << 1;
            continue;
        }

        // C.8
        if (align == 16)
            nx = (nx + 1) & ~1;

        // C.9
        if (bt != VT_STRUCT && size == 16 && nx < 7) {
            a[i] = nx << 1;
            nx += 2;
            continue;
        }

        // C.10
        if (bt == VT_STRUCT && size <= (8 - nx) * 8) {
            a[i] = nx << 1;
            nx += (size + 7) >> 3;
            continue;
        }

        // C.11
        nx = 8;

        // C.12
        ns = (ns + 7) & ~7;
        ns = (ns + align - 1) & -align;

        // C.13
        if (bt == VT_STRUCT) {
            a[i] = ns;
            ns += size;
            continue;
        }

        // C.14
        if (size < 8)
            size = 8;

        // C.15
        a[i] = ns;
        ns += size;
    }

    return ns - 32;
}

static unsigned long arm64_pcs(int variadic, int n, CType **type, unsigned long *a)
{
    unsigned long stack;

    // Return type:
    if ((type[0]->t & VT_BTYPE) == VT_VOID)
        a[0] = -1;
    else {
        arm64_pcs_aux(0, 1, type, a);
        assert(a[0] == 0 || a[0] == 1 || a[0] == 16);
    }

    // Argument types:
    stack = arm64_pcs_aux(variadic, n - 1, type + 1, a + 1);

    if (0) {
        int i;
        for (i = 0; i < n; i++) {
            if (!i)
                printf("arm64_pcs return: ");
            else
                printf("arm64_pcs arg %d: ", i);
            if (a[i] == (unsigned long)-1)
                printf("void\n");
            else if (a[i] == 1 && !i)
                printf("X8 pointer\n");
            else if (a[i] < 16)
                printf("X%lu%s\n", a[i] / 2, a[i] & 1 ? " pointer" : "");
            else if (a[i] < 32)
                printf("V%lu\n", a[i] / 2 - 8);
            else
                printf("stack %lu%s\n",
                       (a[i] - 32) & ~1, a[i] & 1 ? " pointer" : "");
        }
    }

    return stack;
}

static int n_func_args(CType *type)
{
    int n_args = 0;
    Sym *arg;

    for (arg = type->ref->next; arg; arg = arg->next)
        n_args++;
    return n_args;
}

static void arm64_sub_sp(uint64_t diff)
{
    if (!diff)
        return;
#ifdef TCC_TARGET_PE
    if (diff >= 4096) {
        arm64_movimm(15, diff >> 4);
        gen_static_call(TOK___chkstk);
        o(0xcb2f73ff); // sub sp,sp,x15,lsl #4
        return;
    }
#endif
    if (!(diff >> 24)) {
        if (diff & 0xffful)
            o(ARM64_SUB_IMM | ARM64_SF(1) | 0           | ARM64_RN(31) | ARM64_RD(31) | ARM64_IMM12(diff & 0xfff));
        if (diff >> 12)
            o(ARM64_SUB_IMM | ARM64_SF(1) | ARM64_SH(1) | ARM64_RN(31) | ARM64_RD(31) | ARM64_IMM12((diff >> 12) & 0xfff));
    } else {
        arm64_movimm(16, diff);
        o(0xCB3063FFU); // sub sp,sp,x16
    }
}

static int gv_addr(int r)
{
    gaddrof();
    vtop->type.t = VT_PTR;
    return gv(r);
}

#ifndef TCC_TARGET_PE
ST_DATA const int regcache_regs[REGCACHE_NREGS] = {
    TREG_CS19, TREG_CS20, TREG_CS21, TREG_CS22, TREG_CS23
};

/* The prolog reserves REGCACHE_FRAME bytes of frame below the
   parameter area (register i of REGCACHE_REGS gets the slot at
   regcache_slot_base - 8*(i+1)) and REGCACHE_NREGS instruction slots
   after the stack setup slots, which the epilog backpatches with the
   saves of the registers actually used (a save is a single stur;
   unused slots stay nop padding).  The reservation is made at the
   end of gfunc_prolog so the parameter home slots in the fixed
   224-byte area keep their stock offsets. */
#define REGCACHE_FRAME (REGCACHE_NREGS * 8)
static unsigned long arm64_regcache_save_offset;
static int regcache_slot_base;
#endif

ST_FUNC void gfunc_call(int nb_args)
{
    CType *return_type;
    CType **t;
    unsigned long *a, *a1;
    unsigned long stack;
    int i;
    int func_type = vtop[-nb_args].type.ref->f.func_type;
    int variadic = (func_type == FUNC_ELLIPSIS);
    int old_style = (func_type == FUNC_OLD);
    int var_nb_arg = variadic ? n_func_args(&vtop[-nb_args].type) : 0;

#ifndef TCC_TARGET_PE
    regcache_flush_all(); /* F3: no cached value crosses a call */
#endif
    save_regs(nb_args + 1);

#ifdef CONFIG_TCC_BCHECK
    if (tcc_state->do_bounds_check)
        gbound_args(nb_args);
#endif

    return_type = &vtop[-nb_args].type.ref->type;
    if ((return_type->t & VT_BTYPE) == VT_STRUCT)
        --nb_args;

    t = tcc_malloc((nb_args + 1) * sizeof(*t));
    a = tcc_malloc((nb_args + 1) * sizeof(*a));
    a1 = tcc_malloc((nb_args + 1) * sizeof(*a1));

    t[0] = return_type;
    for (i = 0; i < nb_args; i++)
        t[nb_args - i] = &vtop[-i].type;

    stack = arm64_pcs(
#ifdef TCC_TARGET_PE
        old_style ? /* all args like varargs in K&R style */ -1 :
#endif
        var_nb_arg, nb_args + 1, t, a);

    // Allocate space for structs replaced by pointer:
    for (i = nb_args; i; i--)
        if (a[i] & 1) {
            SValue *arg = &vtop[i - nb_args];
            int align, size = type_size(&arg->type, &align);
            assert((arg->type.t & VT_BTYPE) == VT_STRUCT);
            stack = (stack + align - 1) & -align;
            a1[i] = stack;
            stack += size;
        }

    stack = (stack + 15) >> 4 << 4;

    if (stack >= 0x1000000) // 16Mb
        tcc_error("stack size too big %lu", stack);
    arm64_sub_sp(stack);

    // First pass: set all values on stack
    for (i = nb_args; i; i--) {
        vpushv(vtop - nb_args + i);

        if (a[i] & 1) {
            // struct replaced by pointer
            int r = get_reg(RC_INT);
            arm64_spoff(intr(r), a1[i]);
            vset(&vtop->type, r | VT_LVAL, 0);
            vswap();
            vstore();
            if (a[i] >= 32) {
                // pointer on stack
                r = get_reg(RC_INT);
                arm64_spoff(intr(r), a1[i]);
                arm64_strx(3, intr(r), 31, (a[i] - 32) >> 1 << 1);
            }
        }
        else if (a[i] >= 32) {
            // value on stack
            if ((vtop->type.t & VT_BTYPE) == VT_STRUCT) {
                int r = get_reg(RC_INT);
                arm64_spoff(intr(r), a[i] - 32);
                vset(&vtop->type, r | VT_LVAL, 0);
                vswap();
                vstore();
            }
            else if (is_float(vtop->type.t) || (vtop->type.t & VT_VECTOR)) {
                gv(RC_FLOAT);
                arm64_strv(arm64_ldst_sz(&vtop[0].type),
                           fltr(vtop[0].r), 31, a[i] - 32);
            }
            else {
                gv(RC_INT);
                arm64_strx(3, // arm64_type_size(vtop[0].type.t),
                           intr(vtop[0].r), 31, a[i] - 32);
            }
        }

        --vtop;
    }

    // Second pass: assign values to registers
    for (i = nb_args; i; i--, vtop--) {
        if (a[i] < 16 && !(a[i] & 1)) {
            // value in general-purpose registers
            if ((variadic || old_style) && i > var_nb_arg && is_float(vtop->type.t)) {
                gv(RC_FLOAT);
                if ((vtop->type.t & VT_BTYPE) == VT_DOUBLE)
                    o(ARM64_FMOV_XD | intr(a[i] / 2) | fltr(vtop->r) << 5); // fmov xN,dM
                else
                    o(ARM64_FMOV_WS | intr(a[i] / 2) | fltr(vtop->r) << 5); // fmov wN,sM
            }
            else if ((vtop->type.t & VT_BTYPE) == VT_STRUCT) {
                int align, size = type_size(&vtop->type, &align);
                if (size) {
                    gv_addr(RC_R(a[i] / 2));
                    arm64_ldrs(a[i] / 2, size);
                }
            }
            else
                gv(RC_R(a[i] / 2));
        }
        else if (a[i] < 16)
            // struct replaced by pointer in register
            arm64_spoff(a[i] / 2, a1[i]);
        else if (a[i] < 32) {
            // value in floating-point registers
            if ((vtop->type.t & VT_BTYPE) == VT_STRUCT) {
                uint32_t j, sz, n = arm64_hfa(&vtop->type, &sz);
                if (n > 0) {
                    /* HFA struct - load from memory into float registers */
                    gv_addr(RC_R30);
                    for (j = 0; j < n; j++)
                        o(0x3d4003c0 |
                          (sz & 16) << 19 | -(sz & 8) << 27 | (sz & 4) << 29 |
                          (a[i] / 2 - 8 + j) |
                          j << 10); // ldr ([sdq])(*),[x30,#(j * sz)]
                } else {
                    /* Non-HFA struct in float register slot - shouldn't happen */
                    gv(RC_F(a[i] / 2 - 8));
                }
            }
            else
                gv(RC_F(a[i] / 2 - 8));
        }
    }

    if ((return_type->t & VT_BTYPE) == VT_STRUCT) {
        if (a[0] == 1) {
            // indirect return: set x8 and discard the stack value
            gv_addr(RC_R(8));
            --vtop;
        }
        else
            // return in registers: keep the address for after the call
            vswap();
    }

    arm64_gen_bl_or_b(0);
    --vtop;
    if (stack & 0xfff)
        o(0x910003ff | (stack & 0xfff) << 10); // add sp,sp,#(n)
    if (stack >> 12)
        o(0x914003ff | (stack >> 12) << 10);

    {
        int rt = return_type->t;
        int bt = rt & VT_BTYPE;
        if (bt == VT_STRUCT && !(a[0] & 1)) {
            // A struct was returned in registers, so write it out:
            gv_addr(RC_R(8));
            --vtop;
            if (a[0] == 0) {
                int align, size = type_size(return_type, &align);
                assert(size <= 16);
                if (size > 8)
                    o(0xa9000500); // stp x0,x1,[x8]
                else if (size)
                    arm64_strx(size > 4 ? 3 : size > 2 ? 2 : size > 1, 0, 8, 0);

            }
            else if (a[0] == 16) {
                /* HFA struct return - store from float registers to the address in x8 */
                uint32_t j, sz, n = arm64_hfa(return_type, &sz);
                for (j = 0; j < n; j++)
                    o(0x3d000100 |
                      (sz & 16) << 19 | -(sz & 8) << 27 | (sz & 4) << 29 |
                      (fltr(REG_FRET) + j) |
                      j << 10); // str ([sdq])(j),[x8,#(j * sz)]
            }
        }
    }

    tcc_free(a1);
    tcc_free(a);
    tcc_free(t);
}

static unsigned long arm64_func_va_list_stack;
static int arm64_func_va_list_gr_offs;
static int arm64_func_va_list_vr_offs;
static int arm64_func_sub_sp_offset;

static unsigned arm64_func_start_offset;
#define ARM64_FUNC_STACK_SETUP_SLOTS 6

#ifdef TCC_TARGET_PE
static unsigned long arm64_pe_param_off(unsigned long a)
{
    return a < 16 ? 160 + a / 2 * 8 :
           a < 32 ? 16 + (a - 16) / 2 * 16 :
           224 + ((a - 32) >> 1 << 1);
}
#endif

ST_FUNC void gfunc_prolog(Sym *func_sym)
{
    CType *func_type = &func_sym->type;
    int n = 0;
    int i = 0;
    Sym *sym;
    CType **t;
    unsigned long *a;
    int use_x8 = 0;
    int last_int = 0;
    int last_float = 0;
    int variadic = func_sym->type.ref->f.func_type == FUNC_ELLIPSIS;
    int var_nb_arg = variadic ? n_func_args(&func_sym->type) : 0;

    func_vc = 144; // offset of where x8 is stored

    for (sym = func_type->ref; sym; sym = sym->next)
        ++n;

#ifdef TCC_TARGET_PE
    n += variadic;
#endif

    t = tcc_malloc(n * sizeof(*t));
    a = tcc_malloc(n * sizeof(*a));
    for (sym = func_type->ref; sym; sym = sym->next)
        t[i++] = &sym->type;

#ifdef TCC_TARGET_PE
    if (variadic)
        t[i] = &int_type;
#endif

    arm64_func_va_list_stack = arm64_pcs(var_nb_arg, n, t, a);

#ifdef TCC_TARGET_PE
    if (variadic)
        arm64_func_va_list_stack = arm64_pe_param_off(a[n - 1]);
#endif

#if !defined(TCC_TARGET_MACHO)
    if (variadic) {
        use_x8 = 1;
        last_int = 4;
        last_float = 4;
    }
#endif

    if (a && a[0] == 1)
        use_x8 = 1;
    for (i = 1, sym = func_type->ref->next; sym; i++, sym = sym->next) {
        if (a[i] < 16) {
            int last, align, size = type_size(&sym->type, &align);
	    last = a[i] / 4 + 1 + (size - 1) / 8;
	    last_int = last > last_int ? last : last_int;
	}
        else if (a[i] < 32) {
            int last, hfa = arm64_hfa(&sym->type, 0);
	    last = a[i] / 4 - 3 + (hfa ? hfa - 1 : 0);
	    last_float = last > last_float ? last : last_float;
	}
    }

    last_int = last_int > 4 ? 4 : last_int;
    last_float = last_float > 4 ? 4 : last_float;

    arm64_func_start_offset = ind;
    o(0xa9b27bfd); // stp x29,x30,[sp,#-224]!
    o(0x910003fd); // mov x29,sp

    for (i = 0; i < last_float; i++)
        // stp q0,q1,[sp,#16], stp q2,q3,[sp,#48]
        // stp q4,q5,[sp,#80], stp q6,q7,[sp,#112]
        o(0xad0087e0 + i * 0x10000 + (i << 11) + (i << 1));
    if (use_x8)
        o(0xa90923e8); // stp x8,x8,[sp,#144]
    for (i = 0; i < last_int; i++)
        // stp x0,x1,[sp,#160], stp x2,x3,[sp,#176]
        // stp x4,x5,[sp,#192], stp x6,x7,[sp,#208]
        o(0xa90a07e0 + i * 0x10000 + (i << 11) + (i << 1));

    arm64_func_va_list_gr_offs = -64;
    arm64_func_va_list_vr_offs = -128;

    for (i = 1, sym = func_type->ref->next; sym; i++, sym = sym->next) {
        int off = (a[i] < 16 ? 160 + a[i] / 2 * 8 :
                   a[i] < 32 ? 16 + (a[i] - 16) / 2 * 16 :
                   224 + ((a[i] - 32) >> 1 << 1));

        gfunc_set_param(sym, off, a[i] & 1);

        if (a[i] < 16) {
            int align, size = type_size(&sym->type, &align);
            arm64_func_va_list_gr_offs = (a[i] / 2 - 7 +
                                          (!(a[i] & 1) && size > 8)) * 8;
        }
        else if (a[i] < 32) {
            uint32_t hfa = arm64_hfa(&sym->type, 0);
            arm64_func_va_list_vr_offs = (a[i] / 2 - 16 +
                                          (hfa ? hfa : 1)) * 16;
        }

        // HFAs of float and double need to be written differently:
        if (16 <= a[i] && a[i] < 32 && (sym->type.t & VT_BTYPE) == VT_STRUCT) {
            uint32_t j, sz, k = arm64_hfa(&sym->type, &sz);
            if (k > 0 && sz < 16)
                for (j = 0; j < k; j++) {
                    o(0x3d0003e0 | -(sz & 8) << 27 | (sz & 4) << 29 |
                      ((a[i] - 16) / 2 + j) | (off / sz + j) << 10);
                    // str ([sdq])(j),[sp,#(j * sz)]
                }
        }
    }

    tcc_free(a);
    tcc_free(t);

    arm64_func_sub_sp_offset = ind;
    /* In gfunc_epilog these will be replaced with stack setup code. */
    for (i = 0; i < ARM64_FUNC_STACK_SETUP_SLOTS; ++i)
        o(ARM64_NOP); // nop
#ifndef TCC_TARGET_PE
    arm64_regcache_save_offset = ind;
    /* In gfunc_epilog these will be replaced with the saves of the
       regcache registers the function actually used. */
    for (i = 0; i < REGCACHE_NREGS; ++i)
        o(ARM64_NOP); // nop
#endif
    loc = 0;
#ifndef TCC_TARGET_PE
    /* reserve the regcache save slots below everything the prolog
       proper allocated */
    regcache_slot_base = loc;
    loc -= REGCACHE_FRAME;
#endif
#ifdef CONFIG_TCC_BCHECK
    if (tcc_state->do_bounds_check)
        gen_bounds_prolog();
#endif
}

ST_FUNC void gen_va_start(void)
{
    int r;
    --vtop; // we don't need the "arg"
    r = intr(gv_addr(RC_INT));

#ifdef TCC_TARGET_PE
    if (arm64_func_va_list_stack) {
        arm64_movimm(30, arm64_func_va_list_stack);
        o(0x8b1e03be); // add x30,x29,x30
    } else
        o(0x910283be); // add x30,x29,#160
    o(0xf900001e | r << 5); // str x30,[x(r)]
#else
    if (arm64_func_va_list_stack) {
        //xx could use add (immediate) here
        arm64_movimm(30, arm64_func_va_list_stack + 224);
        o(0x8b1e03be); // add x30,x29,x30
    }
    else
        o(0x910383be); // add x30,x29,#224
    o(0xf900001e | r << 5); // str x30,[x(r)]

#if !defined(TCC_TARGET_MACHO)
    if (arm64_func_va_list_gr_offs) {
        if (arm64_func_va_list_stack)
            o(0x910383be); // add x30,x29,#224
        o(0xf900041e | r << 5); // str x30,[x(r),#8]
    }

    if (arm64_func_va_list_vr_offs) {
        o(0x910243be); // add x30,x29,#144
        o(0xf900081e | r << 5); // str x30,[x(r),#16]
    }

    arm64_movimm(30, arm64_func_va_list_gr_offs);
    o(0xb900181e | r << 5); // str w30,[x(r),#24]

    arm64_movimm(30, arm64_func_va_list_vr_offs);
    o(0xb9001c1e | r << 5); // str w30,[x(r),#28]
#endif
#endif

    --vtop;
}

ST_FUNC void gen_va_arg(CType *t)
{
    int align, size = type_size(t, &align);
    uint32_t r0, r1;

#ifdef TCC_TARGET_PE
    int indirect = 0, slot = (size + 7) & -8;

    if (size > 16)
        indirect = 1, slot = 8;

    r0 = intr(gv_addr(RC_INT));
    r1 = get_reg(RC_INT);
    vtop[0].r = r1 | VT_LVAL;
    r1 = intr(r1);

    o(ARM64_LDR_X | ARM64_RN(r0) | r1); // ldr x(r1),[x(r0)] // ap
    if (slot) {
        if (slot == 16) {
            o(0x910363be); // add x30,x29,#216
            o(0xeb1e003f | r1 << 5); // cmp x(r1),x30
            o(0x54000041); // b.ne .+8
            o(0x910383a0 | r1 | 29 << 5); // add x(r1),x29,#224
        }
        if (align == 16) {
            o(0x91003c00 | r1 | r1 << 5); // add x(r1),x(r1),#15
            o(0x927cec00 | r1 | r1 << 5); // and x(r1),x(r1),#-16
        }
        o(0x9100001e | r1 << 5 | slot << 10); // add x30,x(r1),#(slot)
        o(0xf900001e | r0 << 5); // str x30,[x(r0)] // ap += slot
    }
    if (indirect)
        o(ARM64_LDR_X | ARM64_RN(r1) | r1); // ldr x(r1),[x(r1)]

#else /* !PE */
    unsigned fsize = size, hfa = 1;

    if (!is_float(t->t)) {
        if (t->t & VT_VECTOR)
            hfa = 1; /* a vector is a single FP/SIMD member */
        else
            hfa = arm64_hfa(t, &fsize);
    }

    r0 = intr(gv_addr(RC_INT));
    r1 = get_reg(RC_INT);
    vtop[0].r = r1 | VT_LVAL;
    r1 = intr(r1);

    if (!hfa) {
        uint32_t n = size > 16 ? 8 : (size + 7) & -8;

#if !defined(TCC_TARGET_MACHO)
        o(0xb940181e | r0 << 5); // ldr w30,[x(r0),#24] // __gr_offs
        if (align == 16) {
            assert(0); // this path untested but needed for __uint128_t
            o(0x11003fde); // add w30,w30,#15
            o(0x121c6fde); // and w30,w30,#-16
        }
        o(0x310003c0 | r1 | n << 10); // adds w(r1),w30,#(n)
        o(0x540000ad); // b.le .+20
#endif

        o(ARM64_LDR_X | ARM64_RN(r0) | r1); // ldr x(r1),[x(r0)] // __stack
        if (align == 16) {
            o(0x91003c00 | r1 | r1 << 5); // add x(r1),x(r1),#15
            o(0x927cec00 | r1 | r1 << 5); // and x(r1),x(r1),#-16
        }
        o(0x9100001e | r1 << 5 | n << 10); // add x30,x(r1),#(n)
        o(0xf900001e | r0 << 5); // str x30,[x(r0)] // __stack

#if !defined(TCC_TARGET_MACHO)
        o(ARM64_B | 4); // b .+16
        o(0xb9001800 | r1 | r0 << 5); // str w(r1),[x(r0),#24] // __gr_offs
        o(0xf9400400 | r1 | r0 << 5); // ldr x(r1),[x(r0),#8] // __gr_top
        o(0x8b3ec000 | r1 | r1 << 5); // add x(r1),x(r1),w30,sxtw
#endif

        if (size > 16)
            o(ARM64_LDR_X | ARM64_RN(r1) | r1); // ldr x(r1),[x(r1)]
    }
    else {
        uint32_t ssz = (size + 7) & -(uint32_t)8;
#if !defined(TCC_TARGET_MACHO)
        uint32_t rsz = hfa << 4;
        uint32_t b1, b2;
        o(0xb9401c1e | r0 << 5); // ldr w30,[x(r0),#28] // __vr_offs
        o(0x310003c0 | r1 | rsz << 10); // adds w(r1),w30,#(rsz)
        b1 = ind; o(0x5400000d); // b.le lab1
#endif
        o(ARM64_LDR_X | ARM64_RN(r0) | r1); // ldr x(r1),[x(r0)] // __stack
        if (fsize == 16) {
            o(0x91003c00 | r1 | r1 << 5); // add x(r1),x(r1),#15
            o(0x927cec00 | r1 | r1 << 5); // and x(r1),x(r1),#-16
        }
        o(0x9100001e | r1 << 5 | ssz << 10); // add x30,x(r1),#(ssz)
        o(0xf900001e | r0 << 5); // str x30,[x(r0)] // __stack
#if !defined(TCC_TARGET_MACHO)
        b2 = ind; o(ARM64_B); // b lab2
        // lab1:
        write32le(cur_text_section->data + b1, 0x5400000d | (ind - b1) << 3);
        o(0xb9001c00 | r1 | r0 << 5); // str w(r1),[x(r0),#28] // __vr_offs
        o(0xf9400800 | r1 | r0 << 5); // ldr x(r1),[x(r0),#16] // __vr_top
        if (hfa == 1 || fsize == 16)
            o(0x8b3ec000 | r1 | r1 << 5); // add x(r1),x(r1),w30,sxtw
        else {
            // We need to change the layout of this HFA.
            // Get some space on the stack using global variable "loc":
            loc = (loc - size) & -(uint32_t)align;
            o(0x8b3ec000 | 30 | r1 << 5); // add x30,x(r1),w30,sxtw
            arm64_movimm(r1, loc);
            o(0x8b0003a0 | r1 | r1 << 16); // add x(r1),x29,x(r1)
            o(0x4c402bdc | (uint32_t)fsize << 7 |
              (uint32_t)(hfa == 2) << 15 |
              (uint32_t)(hfa == 3) << 14); // ld1 {v28.(4s|2d),...},[x30]
            o(0x0d00801c | r1 << 5 | (fsize == 8) << 10 |
              (uint32_t)(hfa != 2) << 13 |
              (uint32_t)(hfa != 3) << 21); // st(hfa) {v28.(s|d),...}[0],[x(r1)]
        }
        // lab2:
        write32le(cur_text_section->data + b2, ARM64_B | ((ind - b2) >> 2));
#endif
    }
#endif /* not pe */
}

ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret,
                       int *align, int *regsize)
{
    return 0;
}

ST_FUNC void gfunc_return(CType *func_type)
{
    CType *t = func_type;
    unsigned long a;

    if (func_type->t & VT_VECTOR) {
        /* vector return value: move it into the vector return
           register (see gen_vec_ret and gfunc_epilog) */
        gen_vec_ret();
        vtop--; /* NOT vpop() because it would flush the fp stack */
        return;
    }

    arm64_pcs(0, 1, &t, &a);

    switch (a) {
    case -1:
        break;
    case 0:
        if ((func_type->t & VT_BTYPE) == VT_STRUCT) {
            int align, size = type_size(func_type, &align);
            gv_addr(RC_R(0));
            arm64_ldrs(0, size);
        }
        else
            gv(RC_IRET);
        break;
    case 1: {
        CType type = *func_type;
        mk_pointer(&type);
        vset(&type, VT_LOCAL | VT_LVAL, func_vc);
        indir();
        vswap();
        vstore();
        break;
    }
    case 16:
        if ((func_type->t & VT_BTYPE) == VT_STRUCT) {
          /* HFA struct return - load from the address on vtop into float registers */
          uint32_t j, sz, n = arm64_hfa(func_type, &sz);
          gv_addr(RC_R(0));
          for (j = 0; j < n; j++)
              o(0x3d400000 |
                (sz & 16) << 19 | -(sz & 8) << 27 | (sz & 4) << 29 |
                (fltr(REG_FRET) + j) | j << 10); // ldr ([sdq])(j),[x0,#(j * sz)]
        }
        else
            gv(RC_FRET);
        break;
    default:
      assert(0);
    }
    vtop--;
}

ST_FUNC void gfunc_epilog(void)
{
#ifndef TCC_TARGET_PE
    int i, mask = regcache_used_mask();
#endif
#ifdef CONFIG_TCC_BCHECK
    if (tcc_state->do_bounds_check)
        gen_bounds_epilog();
#endif

#ifndef TCC_TARGET_PE
    /* backpatch the regcache saves into the reserved prolog area;
       registers the function never used stay nop padding */
    if (mask) {
        addr_t saved_ind = ind;
        ind = arm64_regcache_save_offset;
        for (i = 0; i < REGCACHE_NREGS; i++)
            if (mask & (1 << i))
                arm64_strx(3, intr(regcache_regs[i]), 29,
                           regcache_slot_base - 8 * (i + 1));
            else
                o(ARM64_NOP);
        ind = saved_ind;
    }
#endif

    if (loc) {
        // Insert instructions to subtract size of stack frame from SP.
        int i;
        addr_t saved_ind = ind;
        addr_t patch_end = arm64_func_sub_sp_offset + ARM64_FUNC_STACK_SETUP_SLOTS * 4;
        uint64_t diff = (-loc + 15) & ~15;
        ind = arm64_func_sub_sp_offset;
        arm64_sub_sp(diff);
        for (i = ind; i < patch_end; i += 4)
            write32le(cur_text_section->data + i, ARM64_NOP); // nop
        ind = saved_ind;
    }
#ifndef TCC_TARGET_PE
    /* restore the regcache registers the function used */
    for (i = 0; i < REGCACHE_NREGS; i++)
        if (mask & (1 << i))
            arm64_ldrx(0, 3, intr(regcache_regs[i]), 29,
                       regcache_slot_base - 8 * (i + 1));
#endif
    o(0x910003bf); // mov sp,x29
    o(0xa8ce7bfd); // ldp x29,x30,[sp],#224

    o(0xd65f03c0); // ret

#ifdef TCC_TARGET_PE
    pe_add_unwind_data(arm64_func_start_offset, ind, -loc);
#endif
}

ST_FUNC void gen_fill_nops(int bytes)
{
    if ((bytes & 3))
      tcc_error("alignment of code section not multiple of 4");
    while (bytes > 0) {
	o(ARM64_NOP); // nop
	bytes -= 4;
    }
}

// Generate forward branch to label:
ST_FUNC int gjmp(int t)
{
    int r = ind;
    if (nocode_wanted)
        return t;
    o(t);
    return r;
}

// Generate branch to known address:
ST_FUNC void gjmp_addr(int a)
{
    assert(a - ind + 0x8000000 < 0x10000000);
    o(ARM64_B | (((a - ind) >> 2) & 0x3ffffff));
}

ST_FUNC int gjmp_append(int n, int t)
{
    void *p;
    /* insert vtop->c jump list in t */
    if (n) {
        uint32_t n1 = n, n2;
        while ((n2 = read32le(p = cur_text_section->data + n1)))
            n1 = n2;
        write32le(p, t);
        t = n;
    }
    return t;
}

void arm64_vset_VT_CMP(int op)
{
    if (op >= TOK_ULT && op <= TOK_GT) {
        vtop->cmp_r = vtop->r;
        vset_VT_CMP(0x80);
    }
}

static void arm64_gen_opil(int op, uint32_t l);

static void arm64_load_cmp(int r, SValue *sv)
{
    sv->r = sv->cmp_r;
    if (sv->c.i & 1) {
        vpushi(1);
        arm64_gen_opil('^', 0);
    }
    if (r != sv->r) {
        load(r, sv);
        sv->r = r;
    }
}

ST_FUNC int gjmp_cond(int op, int t)
{
    int bt = vtop->type.t & VT_BTYPE;

    int inv = op & 1;
    vtop->r = vtop->cmp_r;

    if (bt == VT_LDOUBLE) {
        uint32_t a, b, f = fltr(gv(RC_FLOAT));
        a = get_reg(RC_INT);
        vpushi(0);
        vtop[0].r = a;
        b = get_reg(RC_INT);
        a = intr(a);
        b = intr(b);
        o(0x4e083c00 | a | f << 5); // mov x(a),v(f).d[0]
        o(0x4e183c00 | b | f << 5); // mov x(b),v(f).d[1]
        o(0xaa000400 | a | a << 5 | b << 16); // orr x(a),x(a),x(b),lsl #1
        o(0xb4000040 | a | !!inv << 24); // cbz/cbnz x(a),.+8
        --vtop;
    }
    else if (bt == VT_FLOAT || bt == VT_DOUBLE) {
        uint32_t a = fltr(gv(RC_FLOAT));
        o(0x1e202008 | a << 5 | (bt != VT_FLOAT) << 22); // fcmp
        o(0x54000040 | !!inv); // b.eq/b.ne .+8
    }
    else {
        uint32_t ll = (bt == VT_PTR || bt == VT_LLONG);
        uint32_t a = intr(gv(RC_INT));
        o(0x34000040 | a | !!inv << 24 | ll << 31); // cbz/cbnz wA,.+8
    }
    return gjmp(t);
}

static int arm64_iconst(uint64_t *val, SValue *sv)
{
    if ((sv->r & (VT_VALMASK | VT_LVAL | VT_SYM)) != VT_CONST)
        return 0;
    if (val) {
        int t = sv->type.t;
	int bt = t & VT_BTYPE;
        *val = ((bt == VT_LLONG || bt == VT_PTR) ? sv->c.i :
                (uint32_t)sv->c.i |
                (t & VT_UNSIGNED ? 0 : -(sv->c.i & 0x80000000)));
    }
    return 1;
}

static int arm64_gen_opic(int op, uint32_t l, int rev, uint64_t val,
                          uint32_t x, uint32_t a)
{
    if (op == '-' && !rev) {
        val = -val;
        op = '+';
    }
    val = l ? val : (uint32_t)val;

    switch (op) {

    case '+': {
        uint32_t s = l ? val >> 63 : val >> 31;
        val = s ? -val : val;
        val = l ? val : (uint32_t)val;
        if (!(val & ~0xfffull))
            o(0x11000000 | l << 31 | s << 30 | x | a << 5 | val << 10);
        else if (!(val & ~0xfff000ull))
            o(0x11400000 | l << 31 | s << 30 | x | a << 5 | val >> 12 << 10);
        else {
            arm64_movimm(30, val); // use x30
            o(0x0b1e0000 | l << 31 | s << 30 | x | a << 5);
        }
        return 1;
      }

    case '-':
        if (!val)
            o(0x4b0003e0 | l << 31 | x | a << 16); // neg
        else if (val == (l ? (uint64_t)-1 : (uint32_t)-1))
            o(0x2a2003e0 | l << 31 | x | a << 16); // mvn
        else {
            arm64_movimm(30, val); // use x30
            o(0x4b0003c0 | l << 31 | x | a << 16); // sub
        }
        return 1;

    case '^':
        if (val == -1 || (val == 0xffffffff && !l)) {
            o(0x2a2003e0 | l << 31 | x | a << 16); // mvn
            return 1;
        }
        // fall through
    case '&':
    case '|': {
        int e = arm64_encode_bimm64(l ? val : val | val << 32);
        if (e < 0)
            return 0;
        o((op == '&' ? 0x12000000 :
           op == '|' ? 0x32000000 : 0x52000000) |
          l << 31 | x | a << 5 | (uint32_t)e << 10);
        return 1;
    }

    case TOK_SAR:
    case TOK_SHL:
    case TOK_SHR: {
        uint32_t n = 32 << l;
        val = val & (n - 1);
        if (rev)
            return 0;
        if (!val) {
            // tcc_warning("shift count >= width of type");
            o(0x2a0003e0 | l << 31 | a << 16);
            return 1;
        }
        else if (op == TOK_SHL)
            o(0x53000000 | l << 31 | l << 22 | x | a << 5 |
              (n - val) << 16 | (n - 1 - val) << 10); // lsl
        else
            o(0x13000000 | (op == TOK_SHR) << 30 | l << 31 | l << 22 |
              x | a << 5 | val << 16 | (n - 1) << 10); // lsr/asr
        return 1;
    }

    }
    return 0;
}

static void arm64_gen_opil(int op, uint32_t l)
{
    uint32_t x, a, b;

    // Special treatment for operations with a constant operand:
    {
        uint64_t val;
        int rev = 1;

        if (arm64_iconst(0, &vtop[0])) {
            vswap();
            rev = 0;
        }
        if (arm64_iconst(&val, &vtop[-1])) {
            gv(RC_INT);
            a = intr(vtop[0].r);
            --vtop;
            x = get_reg(RC_INT);
            ++vtop;
            if (arm64_gen_opic(op, l, rev, val, intr(x), a)) {
                vtop[0].r = x;
                vswap();
                --vtop;
                return;
            }
        }
        if (!rev)
            vswap();
    }

    gv2(RC_INT, RC_INT);
    assert(vtop[-1].r < VT_CONST && vtop[0].r < VT_CONST);
    a = intr(vtop[-1].r);
    b = intr(vtop[0].r);
    vtop -= 2;
    x = get_reg(RC_INT);
    ++vtop;
    vtop[0].r = x;
    x = intr(x);

    switch (op) {
    case '%':
        // Use x30 for quotient:
        o(0x1ac00c00 | l << 31 | 30 | a << 5 | b << 16); // sdiv
        o(0x1b008000 | l << 31 | x | (uint32_t)30 << 5 |
          b << 16 | a << 10); // msub
        break;
    case '&':
        o(0x0a000000 | l << 31 | x | a << 5 | b << 16); // and
        break;
    case '*':
        o(0x1b007c00 | l << 31 | x | a << 5 | b << 16); // mul
        break;
    case '+':
        o(0x0b000000 | l << 31 | x | a << 5 | b << 16); // add
        break;
    case '-':
        o(0x4b000000 | l << 31 | x | a << 5 | b << 16); // sub
        break;
    case '/':
    case TOK_PDIV:
        o(0x1ac00c00 | l << 31 | x | a << 5 | b << 16); // sdiv
        break;
    case '^':
        o(0x4a000000 | l << 31 | x | a << 5 | b << 16); // eor
        break;
    case '|':
        o(0x2a000000 | l << 31 | x | a << 5 | b << 16); // orr
        break;
    case TOK_EQ:
        o(0x6b00001f | l << 31 | a << 5 | b << 16); // cmp
        o(0x1a9f17e0 | x); // cset wA,eq
        break;
    case TOK_GE:
        o(0x6b00001f | l << 31 | a << 5 | b << 16); // cmp
        o(0x1a9fb7e0 | x); // cset wA,ge
        break;
    case TOK_GT:
        o(0x6b00001f | l << 31 | a << 5 | b << 16); // cmp
        o(0x1a9fd7e0 | x); // cset wA,gt
        break;
    case TOK_LE:
        o(0x6b00001f | l << 31 | a << 5 | b << 16); // cmp
        o(0x1a9fc7e0 | x); // cset wA,le
        break;
    case TOK_LT:
        o(0x6b00001f | l << 31 | a << 5 | b << 16); // cmp
        o(0x1a9fa7e0 | x); // cset wA,lt
        break;
    case TOK_NE:
        o(0x6b00001f | l << 31 | a << 5 | b << 16); // cmp
        o(0x1a9f07e0 | x); // cset wA,ne
        break;
    case TOK_SAR:
        o(0x1ac02800 | l << 31 | x | a << 5 | b << 16); // asr
        break;
    case TOK_SHL:
        o(0x1ac02000 | l << 31 | x | a << 5 | b << 16); // lsl
        break;
    case TOK_SHR:
        o(0x1ac02400 | l << 31 | x | a << 5 | b << 16); // lsr
        break;
    case TOK_UDIV:
        o(0x1ac00800 | l << 31 | x | a << 5 | b << 16); // udiv
        break;
    case TOK_UGE:
        o(0x6b00001f | l << 31 | a << 5 | b << 16); // cmp
        o(0x1a9f37e0 | x); // cset wA,cs
        break;
    case TOK_UGT:
        o(0x6b00001f | l << 31 | a << 5 | b << 16); // cmp
        o(0x1a9f97e0 | x); // cset wA,hi
        break;
    case TOK_ULT:
        o(0x6b00001f | l << 31 | a << 5 | b << 16); // cmp
        o(0x1a9f27e0 | x); // cset wA,cc
        break;
    case TOK_ULE:
        o(0x6b00001f | l << 31 | a << 5 | b << 16); // cmp
        o(0x1a9f87e0 | x); // cset wA,ls
        break;
    case TOK_UMOD:
        // Use x30 for quotient:
        o(0x1ac00800 | l << 31 | 30 | a << 5 | b << 16); // udiv
        o(0x1b008000 | l << 31 | x | (uint32_t)30 << 5 |
          b << 16 | a << 10); // msub
        break;
    default:
        assert(0);
    }
}

/* ------------------------------------------------------------------ */
/* NEON vector builtins, reached through the generic layer: the
   __builtin_aarch64_* generics in the frontend and the gen_vec_*
   hooks.  An opcode is (operation << 11) | (immediate << 4) |
   shape, the shape being one of the twelve fixed lane shapes
   below; each table entry also carries the target feature the
   operation needs beyond the NEON baseline, checked against the
   feature word before any code is emitted.  The tables between
   the NEON_TABLES markers are generated from the operation spec
   by tools/neon_gen.py. */

enum {
    A64SH_I8X8, A64SH_I8X16, A64SH_I16X4, A64SH_I16X8,
    A64SH_I32X2, A64SH_I32X4, A64SH_I64X1, A64SH_I64X2,
    A64SH_F32X2, A64SH_F32X4, A64SH_F64X1, A64SH_F64X2
};

/* how an operation's operand and result shapes relate */
enum {
    A64K_SAME,       /* every operand and the result share a shape */
    A64K_CVTN,       /* one operand; the result has oes/ofl
                        elements and the same lane count */
    A64K_NARROW,     /* a 16-byte operand narrows into 8 bytes of
                        half-size elements */
    A64K_WIDEN_LO,   /* 8-byte operand(s) widen into 16 bytes of
                        double-size elements */
    A64K_WIDEN_HI,   /* like WIDEN_LO from the high half of a
                        16-byte operand */
    A64K_BYTES,      /* only the total size must match */
    A64K_SHIFT,      /* the count operand shares the value shape */
    A64K_JOIN,       /* two 8-byte operands join into 16 bytes */
    A64K_DOT,        /* an accumulator plus two oes-element
                        operands of the same total size */
    A64K_ACCW_LO,    /* a 16-byte accumulator plus two 8-byte
                        operands of half the element size */
    A64K_ACCW_HI,    /* like ACCW_LO with 16-byte operands */
    A64K_NARROW_HI,  /* an 8-byte low half plus a 16-byte operand
                        of double element size */
    A64K_NARROW2,    /* two 16-byte operands narrow into 8 bytes */
    A64K_NARROW2_HI, /* an 8-byte low half plus two 16-byte
                        operands of double element size */
    A64K_ACCW1,      /* a wide accumulator plus one narrow operand
                        of the same total size */
    A64K_HALF_LO,    /* the low 8 bytes of a 16-byte operand */
    A64K_HALF_HI,    /* the high 8 bytes of a 16-byte operand */
    A64K_PAIRW,      /* pairwise widening: the result element is
                        twice as wide, the total size is unchanged */
    A64K_ADDW_LO,    /* a 16-byte accumulator plus one 8-byte
                        operand of half the element size */
    A64K_ADDW_HI,    /* like ADDW_LO with a 16-byte operand */
    A64K_TBLQ,       /* table lookup with a 16-byte table and a
                        result of the entry's total size */
    A64K_SCALAR      /* the result is one element (oes/ofl) */
};

/* how an operation's encoding is emitted */
enum {
    A64F_2REG,         /* op rd, rn */
    A64F_2REG_IMM,     /* op rd, rn, #imm */
    A64F_2REG_IMM_ACC, /* op rd, rn, #imm accumulating into rn */
    A64F_2REG_ACC,     /* op rd, rm accumulating into the first
                          operand's register */
    A64F_3REG,         /* op rd, rn, rm */
    A64F_3REG_ACC,     /* op rd, rn, rm accumulating into the
                          first operand's register */
    A64F_3REG_IMM,     /* op rd, rn, #imm accumulating into the
                          first operand's register */
    A64F_3REG_IDX,     /* op rd, rn, rm[lane] */
    A64F_JOIN,         /* hand-formed: the two halves combined */
    A64F_CVT64I,       /* scalar convert, GP source: the operand
                          moves to a GP register first */
    A64F_CVT64F,       /* scalar convert, GP result: the result
                          moves back from a GP register */
    A64F_3REG_FMA64    /* scalar fused multiply-add: the addend
                          field repeats the destination */
};

/* one opcode-table entry.  opc is the opcode with the immediate
   bits clear; oes is the element size of the other operand(s) or
   of the result where the kind says so (0 = same as es); ofl is
   the result's float flag for conversions; enc is the instruction
   word with the register fields and the immediate field clear;
   the immediate field itself is imm_base + imm_sign * imm at
   imm_pos, or, when imm_sign == 2, a bit spread packed into
   imm_spread (5 bits per imm bit, biased by 1). */
typedef struct {
    int opc;
    short es, fl, oes, ofl, kind, form;
    short imm_lo, imm_hi, imm_pos, imm_base, imm_sign;
    unsigned feat;
    uint32_t enc, imm_spread;
} AVop;
typedef AVop AVop1, AVop2, AVop3;

/* NEON_TABLES_BEGIN (generated by tools/neon_gen.py; do not edit) */
enum { A64OP1_ABS, A64OP1_FABS, A64OP1_SQABS, A64OP1_NEG, A64OP1_FNEG, A64OP1_SQNEG, A64OP1_NOT, A64OP1_CLS, A64OP1_CLZ, A64OP1_CNT, A64OP1_RBIT, A64OP1_REV64, A64OP1_REV32, A64OP1_REV16, A64OP1_CMEQZ, A64OP1_CMGTZ, A64OP1_CMGEZ, A64OP1_CMLEZ, A64OP1_CMLTZ, A64OP1_FCMEQZ, A64OP1_FCMGTZ, A64OP1_FCMGEZ, A64OP1_FCMLEZ, A64OP1_FCMLTZ, A64OP1_FSQRT, A64OP1_FRECPE, A64OP1_FRSQRTE, A64OP1_FRINTN, A64OP1_FRINTP, A64OP1_FRINTM, A64OP1_FRINTZ, A64OP1_FRINTA, A64OP1_FRINTX, A64OP1_FRINTI, A64OP1_MOVLO, A64OP1_MOVHI, A64OP1_SCVTF, A64OP1_FCVTZS, A64OP1_FCVTNS, A64OP1_FCVTPS, A64OP1_FCVTMS, A64OP1_FCVTAS, A64OP1_UCVTF, A64OP1_FCVTZU, A64OP1_FCVTNU, A64OP1_FCVTPU, A64OP1_FCVTMU, A64OP1_FCVTAU, A64OP1_FCVTN, A64OP1_FCVTL, A64OP1_SCVTFI, A64OP1_FCVTZIS, A64OP1_UCVTFI, A64OP1_FCVTZIU, A64OP1_SXTL, A64OP1_UXTL, A64OP1_SXTL2, A64OP1_UXTL2, A64OP1_SADDLP, A64OP1_UADDLP, A64OP1_XTN, A64OP1_SQXTN, A64OP1_UQXTN, A64OP1_SQXTUN, A64OP1_SSHR, A64OP1_USHR, A64OP1_SRSHR, A64OP1_URSHR, A64OP1_SHL, A64OP1_SQSHL_N, A64OP1_UQSHL_N, A64OP1_SQSHLU_N, A64OP1_SSHLL_N, A64OP1_USHLL_N2, A64OP1_SSHLL_N2, A64OP1_USHLL_N2H, A64OP1_SHRN, A64OP1_RSHRN, A64OP1_SQSHRN, A64OP1_UQSHRN, A64OP1_SQSHRUN, A64OP1_SQRSHRN, A64OP1_UQRSHRN, A64OP1_SQRSHRUN, A64OP1_USHRN, A64OP1_URSHRN, A64OP1_URECPE, A64OP1_URSQRTE, A64OP1_FCVTL2, A64OP1_CVTX };
static const AVop1 a64_op1_table[] = {
    { .opc = 0, .es = 1, .form = A64F_2REG, .enc = 0x0e20b800 },
    { .opc = 1, .es = 1, .form = A64F_2REG, .enc = 0x4e20b800 },
    { .opc = 2, .es = 2, .form = A64F_2REG, .enc = 0x0e60b800 },
    { .opc = 3, .es = 2, .form = A64F_2REG, .enc = 0x4e60b800 },
    { .opc = 4, .es = 4, .form = A64F_2REG, .enc = 0x0ea0b800 },
    { .opc = 5, .es = 4, .form = A64F_2REG, .enc = 0x4ea0b800 },
    { .opc = 6, .es = 8, .form = A64F_2REG, .enc = 0x5ee0b800 },
    { .opc = 7, .es = 8, .form = A64F_2REG, .enc = 0x4ee0b800 },
    { .opc = 2056, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x0ea0f800 },
    { .opc = 2057, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x4ea0f800 },
    { .opc = 2058, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x1e60c000 },
    { .opc = 2059, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x4ee0f800 },
    { .opc = 4096, .es = 1, .form = A64F_2REG, .enc = 0x0e207800 },
    { .opc = 4097, .es = 1, .form = A64F_2REG, .enc = 0x4e207800 },
    { .opc = 4098, .es = 2, .form = A64F_2REG, .enc = 0x0e607800 },
    { .opc = 4099, .es = 2, .form = A64F_2REG, .enc = 0x4e607800 },
    { .opc = 4100, .es = 4, .form = A64F_2REG, .enc = 0x0ea07800 },
    { .opc = 4101, .es = 4, .form = A64F_2REG, .enc = 0x4ea07800 },
    { .opc = 4102, .es = 8, .form = A64F_2REG, .enc = 0x5ee07800 },
    { .opc = 4103, .es = 8, .form = A64F_2REG, .enc = 0x4ee07800 },
    { .opc = 6144, .es = 1, .form = A64F_2REG, .enc = 0x2e20b800 },
    { .opc = 6145, .es = 1, .form = A64F_2REG, .enc = 0x6e20b800 },
    { .opc = 6146, .es = 2, .form = A64F_2REG, .enc = 0x2e60b800 },
    { .opc = 6147, .es = 2, .form = A64F_2REG, .enc = 0x6e60b800 },
    { .opc = 6148, .es = 4, .form = A64F_2REG, .enc = 0x2ea0b800 },
    { .opc = 6149, .es = 4, .form = A64F_2REG, .enc = 0x6ea0b800 },
    { .opc = 6150, .es = 8, .form = A64F_2REG, .enc = 0x7ee0b800 },
    { .opc = 6151, .es = 8, .form = A64F_2REG, .enc = 0x6ee0b800 },
    { .opc = 8200, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x2ea0f800 },
    { .opc = 8201, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x6ea0f800 },
    { .opc = 8202, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x1e614000 },
    { .opc = 8203, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x6ee0f800 },
    { .opc = 10240, .es = 1, .form = A64F_2REG, .enc = 0x2e207800 },
    { .opc = 10241, .es = 1, .form = A64F_2REG, .enc = 0x6e207800 },
    { .opc = 10242, .es = 2, .form = A64F_2REG, .enc = 0x2e607800 },
    { .opc = 10243, .es = 2, .form = A64F_2REG, .enc = 0x6e607800 },
    { .opc = 10244, .es = 4, .form = A64F_2REG, .enc = 0x2ea07800 },
    { .opc = 10245, .es = 4, .form = A64F_2REG, .enc = 0x6ea07800 },
    { .opc = 10246, .es = 8, .form = A64F_2REG, .enc = 0x7ee07800 },
    { .opc = 10247, .es = 8, .form = A64F_2REG, .enc = 0x6ee07800 },
    { .opc = 12288, .es = 1, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x2e205800 },
    { .opc = 12289, .es = 1, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x6e205800 },
    { .opc = 12290, .es = 2, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x2e205800 },
    { .opc = 12291, .es = 2, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x6e205800 },
    { .opc = 12292, .es = 4, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x2e205800 },
    { .opc = 12293, .es = 4, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x6e205800 },
    { .opc = 12288, .es = 1, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x2e205800 },
    { .opc = 12289, .es = 1, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x6e205800 },
    { .opc = 12290, .es = 2, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x2e205800 },
    { .opc = 12291, .es = 2, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x6e205800 },
    { .opc = 12292, .es = 4, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x2e205800 },
    { .opc = 12293, .es = 4, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x6e205800 },
    { .opc = 12288, .es = 1, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x2e205800 },
    { .opc = 12289, .es = 1, .kind = A64K_BYTES, .form = A64F_2REG, .enc = 0x6e205800 },
    { .opc = 14336, .es = 1, .form = A64F_2REG, .enc = 0x0e204800 },
    { .opc = 14337, .es = 1, .form = A64F_2REG, .enc = 0x4e204800 },
    { .opc = 14338, .es = 2, .form = A64F_2REG, .enc = 0x0e604800 },
    { .opc = 14339, .es = 2, .form = A64F_2REG, .enc = 0x4e604800 },
    { .opc = 14340, .es = 4, .form = A64F_2REG, .enc = 0x0ea04800 },
    { .opc = 14341, .es = 4, .form = A64F_2REG, .enc = 0x4ea04800 },
    { .opc = 14336, .es = 1, .form = A64F_2REG, .enc = 0x0e204800 },
    { .opc = 14337, .es = 1, .form = A64F_2REG, .enc = 0x4e204800 },
    { .opc = 14338, .es = 2, .form = A64F_2REG, .enc = 0x0e604800 },
    { .opc = 14339, .es = 2, .form = A64F_2REG, .enc = 0x4e604800 },
    { .opc = 14340, .es = 4, .form = A64F_2REG, .enc = 0x0ea04800 },
    { .opc = 14341, .es = 4, .form = A64F_2REG, .enc = 0x4ea04800 },
    { .opc = 16384, .es = 1, .form = A64F_2REG, .enc = 0x2e204800 },
    { .opc = 16385, .es = 1, .form = A64F_2REG, .enc = 0x6e204800 },
    { .opc = 16386, .es = 2, .form = A64F_2REG, .enc = 0x2e604800 },
    { .opc = 16387, .es = 2, .form = A64F_2REG, .enc = 0x6e604800 },
    { .opc = 16388, .es = 4, .form = A64F_2REG, .enc = 0x2ea04800 },
    { .opc = 16389, .es = 4, .form = A64F_2REG, .enc = 0x6ea04800 },
    { .opc = 16384, .es = 1, .form = A64F_2REG, .enc = 0x2e204800 },
    { .opc = 16385, .es = 1, .form = A64F_2REG, .enc = 0x6e204800 },
    { .opc = 16386, .es = 2, .form = A64F_2REG, .enc = 0x2e604800 },
    { .opc = 16387, .es = 2, .form = A64F_2REG, .enc = 0x6e604800 },
    { .opc = 16388, .es = 4, .form = A64F_2REG, .enc = 0x2ea04800 },
    { .opc = 16389, .es = 4, .form = A64F_2REG, .enc = 0x6ea04800 },
    { .opc = 18432, .es = 1, .form = A64F_2REG, .enc = 0x0e205800 },
    { .opc = 18433, .es = 1, .form = A64F_2REG, .enc = 0x4e205800 },
    { .opc = 18432, .es = 1, .form = A64F_2REG, .enc = 0x0e205800 },
    { .opc = 18433, .es = 1, .form = A64F_2REG, .enc = 0x4e205800 },
    { .opc = 18432, .es = 1, .form = A64F_2REG, .enc = 0x0e205800 },
    { .opc = 18433, .es = 1, .form = A64F_2REG, .enc = 0x4e205800 },
    { .opc = 20480, .es = 1, .form = A64F_2REG, .enc = 0x2e605800 },
    { .opc = 20481, .es = 1, .form = A64F_2REG, .enc = 0x6e605800 },
    { .opc = 20480, .es = 1, .form = A64F_2REG, .enc = 0x2e605800 },
    { .opc = 20481, .es = 1, .form = A64F_2REG, .enc = 0x6e605800 },
    { .opc = 20480, .es = 1, .form = A64F_2REG, .enc = 0x2e605800 },
    { .opc = 20481, .es = 1, .form = A64F_2REG, .enc = 0x6e605800 },
    { .opc = 22528, .es = 1, .form = A64F_2REG, .enc = 0x0e200800 },
    { .opc = 22529, .es = 1, .form = A64F_2REG, .enc = 0x4e200800 },
    { .opc = 22530, .es = 2, .form = A64F_2REG, .enc = 0x0e600800 },
    { .opc = 22531, .es = 2, .form = A64F_2REG, .enc = 0x4e600800 },
    { .opc = 22532, .es = 4, .form = A64F_2REG, .enc = 0x0ea00800 },
    { .opc = 22533, .es = 4, .form = A64F_2REG, .enc = 0x4ea00800 },
    { .opc = 22528, .es = 1, .form = A64F_2REG, .enc = 0x0e200800 },
    { .opc = 22529, .es = 1, .form = A64F_2REG, .enc = 0x4e200800 },
    { .opc = 22530, .es = 2, .form = A64F_2REG, .enc = 0x0e600800 },
    { .opc = 22531, .es = 2, .form = A64F_2REG, .enc = 0x4e600800 },
    { .opc = 22532, .es = 4, .form = A64F_2REG, .enc = 0x0ea00800 },
    { .opc = 22533, .es = 4, .form = A64F_2REG, .enc = 0x4ea00800 },
    { .opc = 22528, .es = 1, .form = A64F_2REG, .enc = 0x0e200800 },
    { .opc = 22529, .es = 1, .form = A64F_2REG, .enc = 0x4e200800 },
    { .opc = 22530, .es = 2, .form = A64F_2REG, .enc = 0x0e600800 },
    { .opc = 22531, .es = 2, .form = A64F_2REG, .enc = 0x4e600800 },
    { .opc = 22536, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x0ea00800 },
    { .opc = 22537, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x4ea00800 },
    { .opc = 24576, .es = 1, .form = A64F_2REG, .enc = 0x2e200800 },
    { .opc = 24577, .es = 1, .form = A64F_2REG, .enc = 0x6e200800 },
    { .opc = 24578, .es = 2, .form = A64F_2REG, .enc = 0x2e600800 },
    { .opc = 24579, .es = 2, .form = A64F_2REG, .enc = 0x6e600800 },
    { .opc = 24576, .es = 1, .form = A64F_2REG, .enc = 0x2e200800 },
    { .opc = 24577, .es = 1, .form = A64F_2REG, .enc = 0x6e200800 },
    { .opc = 24578, .es = 2, .form = A64F_2REG, .enc = 0x2e600800 },
    { .opc = 24579, .es = 2, .form = A64F_2REG, .enc = 0x6e600800 },
    { .opc = 24576, .es = 1, .form = A64F_2REG, .enc = 0x2e200800 },
    { .opc = 24577, .es = 1, .form = A64F_2REG, .enc = 0x6e200800 },
    { .opc = 24578, .es = 2, .form = A64F_2REG, .enc = 0x2e600800 },
    { .opc = 24579, .es = 2, .form = A64F_2REG, .enc = 0x6e600800 },
    { .opc = 26624, .es = 1, .form = A64F_2REG, .enc = 0x0e201800 },
    { .opc = 26625, .es = 1, .form = A64F_2REG, .enc = 0x4e201800 },
    { .opc = 26624, .es = 1, .form = A64F_2REG, .enc = 0x0e201800 },
    { .opc = 26625, .es = 1, .form = A64F_2REG, .enc = 0x4e201800 },
    { .opc = 26624, .es = 1, .form = A64F_2REG, .enc = 0x0e201800 },
    { .opc = 26625, .es = 1, .form = A64F_2REG, .enc = 0x4e201800 },
    { .opc = 28672, .es = 1, .form = A64F_2REG, .enc = 0x0e209800 },
    { .opc = 28673, .es = 1, .form = A64F_2REG, .enc = 0x4e209800 },
    { .opc = 28674, .es = 2, .form = A64F_2REG, .enc = 0x0e609800 },
    { .opc = 28675, .es = 2, .form = A64F_2REG, .enc = 0x4e609800 },
    { .opc = 28676, .es = 4, .form = A64F_2REG, .enc = 0x0ea09800 },
    { .opc = 28677, .es = 4, .form = A64F_2REG, .enc = 0x4ea09800 },
    { .opc = 28678, .es = 8, .form = A64F_2REG, .enc = 0x5ee09800 },
    { .opc = 28679, .es = 8, .form = A64F_2REG, .enc = 0x4ee09800 },
    { .opc = 28672, .es = 1, .form = A64F_2REG, .enc = 0x0e209800 },
    { .opc = 28673, .es = 1, .form = A64F_2REG, .enc = 0x4e209800 },
    { .opc = 28674, .es = 2, .form = A64F_2REG, .enc = 0x0e609800 },
    { .opc = 28675, .es = 2, .form = A64F_2REG, .enc = 0x4e609800 },
    { .opc = 28676, .es = 4, .form = A64F_2REG, .enc = 0x0ea09800 },
    { .opc = 28677, .es = 4, .form = A64F_2REG, .enc = 0x4ea09800 },
    { .opc = 28678, .es = 8, .form = A64F_2REG, .enc = 0x5ee09800 },
    { .opc = 28679, .es = 8, .form = A64F_2REG, .enc = 0x4ee09800 },
    { .opc = 28672, .es = 1, .form = A64F_2REG, .enc = 0x0e209800 },
    { .opc = 28673, .es = 1, .form = A64F_2REG, .enc = 0x4e209800 },
    { .opc = 30720, .es = 1, .form = A64F_2REG, .enc = 0x0e208800 },
    { .opc = 30721, .es = 1, .form = A64F_2REG, .enc = 0x4e208800 },
    { .opc = 30722, .es = 2, .form = A64F_2REG, .enc = 0x0e608800 },
    { .opc = 30723, .es = 2, .form = A64F_2REG, .enc = 0x4e608800 },
    { .opc = 30724, .es = 4, .form = A64F_2REG, .enc = 0x0ea08800 },
    { .opc = 30725, .es = 4, .form = A64F_2REG, .enc = 0x4ea08800 },
    { .opc = 30726, .es = 8, .form = A64F_2REG, .enc = 0x5ee08800 },
    { .opc = 30727, .es = 8, .form = A64F_2REG, .enc = 0x4ee08800 },
    { .opc = 32768, .es = 1, .form = A64F_2REG, .enc = 0x2e208800 },
    { .opc = 32769, .es = 1, .form = A64F_2REG, .enc = 0x6e208800 },
    { .opc = 32770, .es = 2, .form = A64F_2REG, .enc = 0x2e608800 },
    { .opc = 32771, .es = 2, .form = A64F_2REG, .enc = 0x6e608800 },
    { .opc = 32772, .es = 4, .form = A64F_2REG, .enc = 0x2ea08800 },
    { .opc = 32773, .es = 4, .form = A64F_2REG, .enc = 0x6ea08800 },
    { .opc = 32774, .es = 8, .form = A64F_2REG, .enc = 0x7ee08800 },
    { .opc = 32775, .es = 8, .form = A64F_2REG, .enc = 0x6ee08800 },
    { .opc = 34816, .es = 1, .form = A64F_2REG, .enc = 0x2e209800 },
    { .opc = 34817, .es = 1, .form = A64F_2REG, .enc = 0x6e209800 },
    { .opc = 34818, .es = 2, .form = A64F_2REG, .enc = 0x2e609800 },
    { .opc = 34819, .es = 2, .form = A64F_2REG, .enc = 0x6e609800 },
    { .opc = 34820, .es = 4, .form = A64F_2REG, .enc = 0x2ea09800 },
    { .opc = 34821, .es = 4, .form = A64F_2REG, .enc = 0x6ea09800 },
    { .opc = 34822, .es = 8, .form = A64F_2REG, .enc = 0x7ee09800 },
    { .opc = 34823, .es = 8, .form = A64F_2REG, .enc = 0x6ee09800 },
    { .opc = 36864, .es = 1, .form = A64F_2REG, .enc = 0x0e20a800 },
    { .opc = 36865, .es = 1, .form = A64F_2REG, .enc = 0x4e20a800 },
    { .opc = 36866, .es = 2, .form = A64F_2REG, .enc = 0x0e60a800 },
    { .opc = 36867, .es = 2, .form = A64F_2REG, .enc = 0x4e60a800 },
    { .opc = 36868, .es = 4, .form = A64F_2REG, .enc = 0x0ea0a800 },
    { .opc = 36869, .es = 4, .form = A64F_2REG, .enc = 0x4ea0a800 },
    { .opc = 36870, .es = 8, .form = A64F_2REG, .enc = 0x5ee0a800 },
    { .opc = 36871, .es = 8, .form = A64F_2REG, .enc = 0x4ee0a800 },
    { .opc = 38920, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x0ea0d800 },
    { .opc = 38921, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4ea0d800 },
    { .opc = 38922, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x5ee0d800 },
    { .opc = 38923, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4ee0d800 },
    { .opc = 40968, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x0ea0c800 },
    { .opc = 40969, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4ea0c800 },
    { .opc = 40970, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x5ee0c800 },
    { .opc = 40971, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4ee0c800 },
    { .opc = 43016, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x2ea0c800 },
    { .opc = 43017, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6ea0c800 },
    { .opc = 43018, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x7ee0c800 },
    { .opc = 43019, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6ee0c800 },
    { .opc = 45064, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x2ea0d800 },
    { .opc = 45065, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6ea0d800 },
    { .opc = 45066, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x7ee0d800 },
    { .opc = 45067, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6ee0d800 },
    { .opc = 47112, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x0ea0e800 },
    { .opc = 47113, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4ea0e800 },
    { .opc = 47114, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x5ee0e800 },
    { .opc = 47115, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4ee0e800 },
    { .opc = 49160, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x2ea1f800 },
    { .opc = 49161, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x6ea1f800 },
    { .opc = 49162, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x1e61c000 },
    { .opc = 49163, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x6ee1f800 },
    { .opc = 51208, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x0ea1d800 },
    { .opc = 51209, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x4ea1d800 },
    { .opc = 51210, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x5ee1d800 },
    { .opc = 51211, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x4ee1d800 },
    { .opc = 53256, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x2ea1d800 },
    { .opc = 53257, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x6ea1d800 },
    { .opc = 53258, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x7ee1d800 },
    { .opc = 53259, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x6ee1d800 },
    { .opc = 55304, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x0e218800 },
    { .opc = 55305, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x4e218800 },
    { .opc = 55306, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x1e644000 },
    { .opc = 55307, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x4e618800 },
    { .opc = 57352, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x0ea18800 },
    { .opc = 57353, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x4ea18800 },
    { .opc = 57354, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x1e64c000 },
    { .opc = 57355, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x4ee18800 },
    { .opc = 59400, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x0e219800 },
    { .opc = 59401, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x4e219800 },
    { .opc = 59402, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x1e654000 },
    { .opc = 59403, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x4e619800 },
    { .opc = 61448, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x0ea19800 },
    { .opc = 61449, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x4ea19800 },
    { .opc = 61450, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x1e65c000 },
    { .opc = 61451, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x4ee19800 },
    { .opc = 63496, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x2e218800 },
    { .opc = 63497, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x6e218800 },
    { .opc = 63498, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x1e664000 },
    { .opc = 63499, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x6e618800 },
    { .opc = 65544, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x2e219800 },
    { .opc = 65545, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x6e219800 },
    { .opc = 65546, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x1e674000 },
    { .opc = 65547, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x6e619800 },
    { .opc = 67592, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x2ea19800 },
    { .opc = 67593, .es = 4, .fl = 1, .form = A64F_2REG, .enc = 0x6ea19800 },
    { .opc = 67594, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x1e67c000 },
    { .opc = 67595, .es = 8, .fl = 1, .form = A64F_2REG, .enc = 0x6ee19800 },
    { .opc = 69633, .es = 1, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 69635, .es = 2, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 69637, .es = 4, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 69639, .es = 8, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 69633, .es = 1, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 69635, .es = 2, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 69637, .es = 4, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 69639, .es = 8, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 69633, .es = 1, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 69635, .es = 2, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 69639, .es = 8, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 69641, .es = 4, .fl = 1, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 69643, .es = 8, .fl = 1, .kind = A64K_HALF_LO, .form = A64F_2REG, .enc = 0x1e604000 },
    { .opc = 71681, .es = 1, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 71683, .es = 2, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 71685, .es = 4, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 71687, .es = 8, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 71681, .es = 1, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 71683, .es = 2, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 71685, .es = 4, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 71687, .es = 8, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 71681, .es = 1, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 71683, .es = 2, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 71687, .es = 8, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 71689, .es = 4, .fl = 1, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 71691, .es = 8, .fl = 1, .kind = A64K_HALF_HI, .form = A64F_2REG, .enc = 0x6e084400 },
    { .opc = 73732, .es = 4, .ofl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x0e21d800 },
    { .opc = 73733, .es = 4, .ofl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4e21d800 },
    { .opc = 73734, .es = 8, .ofl = 1, .kind = A64K_CVTN, .form = A64F_CVT64I, .enc = 0x9e620000 },
    { .opc = 73735, .es = 8, .ofl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4e61d800 },
    { .opc = 75784, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x0ea1b800 },
    { .opc = 75785, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4ea1b800 },
    { .opc = 75786, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_CVT64F, .enc = 0x9e780000 },
    { .opc = 75787, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4ee1b800 },
    { .opc = 77832, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x0e21a800 },
    { .opc = 77833, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4e21a800 },
    { .opc = 77834, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_CVT64F, .enc = 0x9e600000 },
    { .opc = 77835, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4e61a800 },
    { .opc = 79880, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x0ea1a800 },
    { .opc = 79881, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4ea1a800 },
    { .opc = 79882, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_CVT64F, .enc = 0x9e680000 },
    { .opc = 79883, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4ee1a800 },
    { .opc = 81928, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x0e21b800 },
    { .opc = 81929, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4e21b800 },
    { .opc = 81930, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_CVT64F, .enc = 0x9e700000 },
    { .opc = 81931, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4e61b800 },
    { .opc = 83976, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x0e21c800 },
    { .opc = 83977, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4e21c800 },
    { .opc = 83978, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_CVT64F, .enc = 0x9e640000 },
    { .opc = 83979, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x4e61c800 },
    { .opc = 86020, .es = 4, .ofl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x2e21d800 },
    { .opc = 86021, .es = 4, .ofl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6e21d800 },
    { .opc = 86022, .es = 8, .ofl = 1, .kind = A64K_CVTN, .form = A64F_CVT64I, .enc = 0x9e630000 },
    { .opc = 86023, .es = 8, .ofl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6e61d800 },
    { .opc = 88072, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x2ea1b800 },
    { .opc = 88073, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6ea1b800 },
    { .opc = 88074, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_CVT64F, .enc = 0x9e790000 },
    { .opc = 88075, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6ee1b800 },
    { .opc = 90120, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x2e21a800 },
    { .opc = 90121, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6e21a800 },
    { .opc = 90122, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_CVT64F, .enc = 0x9e610000 },
    { .opc = 90123, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6e61a800 },
    { .opc = 92168, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x2ea1a800 },
    { .opc = 92169, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6ea1a800 },
    { .opc = 92170, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_CVT64F, .enc = 0x9e690000 },
    { .opc = 92171, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6ee1a800 },
    { .opc = 94216, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x2e21b800 },
    { .opc = 94217, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6e21b800 },
    { .opc = 94218, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_CVT64F, .enc = 0x9e710000 },
    { .opc = 94219, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6e61b800 },
    { .opc = 96264, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x2e21c800 },
    { .opc = 96265, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6e21c800 },
    { .opc = 96266, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_CVT64F, .enc = 0x9e650000 },
    { .opc = 96267, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG, .enc = 0x6e61c800 },
    { .opc = 98315, .es = 8, .fl = 1, .oes = 4, .ofl = 1, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x0e616800 },
    { .opc = 100360, .es = 4, .fl = 1, .oes = 8, .ofl = 1, .kind = A64K_WIDEN_LO, .form = A64F_2REG, .enc = 0x0e617800 },
    { .opc = 102404, .es = 4, .ofl = 1, .kind = A64K_CVTN, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x0f20e400 },
    { .opc = 102405, .es = 4, .ofl = 1, .kind = A64K_CVTN, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x4f20e400 },
    { .opc = 102406, .es = 8, .ofl = 1, .kind = A64K_CVTN, .form = A64F_CVT64I, .imm_lo = 1, .imm_hi = 64, .imm_pos = 10, .imm_base = 64, .imm_sign = -1, .enc = 0x9e420000 },
    { .opc = 102407, .es = 8, .ofl = 1, .kind = A64K_CVTN, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x4f40e400 },
    { .opc = 104456, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x0f20fc00 },
    { .opc = 104457, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x4f20fc00 },
    { .opc = 104458, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_CVT64F, .imm_lo = 1, .imm_hi = 64, .imm_pos = 10, .imm_base = 64, .imm_sign = -1, .enc = 0x9e580000 },
    { .opc = 104459, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x4f40fc00 },
    { .opc = 106500, .es = 4, .ofl = 1, .kind = A64K_CVTN, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x2f20e400 },
    { .opc = 106501, .es = 4, .ofl = 1, .kind = A64K_CVTN, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x6f20e400 },
    { .opc = 106502, .es = 8, .ofl = 1, .kind = A64K_CVTN, .form = A64F_CVT64I, .imm_lo = 1, .imm_hi = 64, .imm_pos = 10, .imm_base = 64, .imm_sign = -1, .enc = 0x9e430000 },
    { .opc = 106503, .es = 8, .ofl = 1, .kind = A64K_CVTN, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x6f40e400 },
    { .opc = 108552, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x2f20fc00 },
    { .opc = 108553, .es = 4, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x6f20fc00 },
    { .opc = 108554, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_CVT64F, .imm_lo = 1, .imm_hi = 64, .imm_pos = 10, .imm_base = 64, .imm_sign = -1, .enc = 0x9e590000 },
    { .opc = 108555, .es = 8, .fl = 1, .kind = A64K_CVTN, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x6f40fc00 },
    { .opc = 110592, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_2REG, .enc = 0x0f08a400 },
    { .opc = 110594, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_2REG, .enc = 0x0f10a400 },
    { .opc = 110596, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_2REG, .enc = 0x0f20a400 },
    { .opc = 112640, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_2REG, .enc = 0x2f08a400 },
    { .opc = 112642, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_2REG, .enc = 0x2f10a400 },
    { .opc = 112644, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_2REG, .enc = 0x2f20a400 },
    { .opc = 114689, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_2REG, .enc = 0x4f08a400 },
    { .opc = 114691, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_2REG, .enc = 0x4f10a400 },
    { .opc = 114693, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_2REG, .enc = 0x4f20a400 },
    { .opc = 116737, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_2REG, .enc = 0x6f08a400 },
    { .opc = 116739, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_2REG, .enc = 0x6f10a400 },
    { .opc = 116741, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_2REG, .enc = 0x6f20a400 },
    { .opc = 118784, .es = 1, .oes = 2, .kind = A64K_PAIRW, .form = A64F_2REG, .enc = 0x0e202800 },
    { .opc = 118785, .es = 1, .oes = 2, .kind = A64K_PAIRW, .form = A64F_2REG, .enc = 0x4e202800 },
    { .opc = 118786, .es = 2, .oes = 4, .kind = A64K_PAIRW, .form = A64F_2REG, .enc = 0x0e602800 },
    { .opc = 118787, .es = 2, .oes = 4, .kind = A64K_PAIRW, .form = A64F_2REG, .enc = 0x4e602800 },
    { .opc = 118788, .es = 4, .oes = 8, .kind = A64K_PAIRW, .form = A64F_2REG, .enc = 0x0ea02800 },
    { .opc = 118789, .es = 4, .oes = 8, .kind = A64K_PAIRW, .form = A64F_2REG, .enc = 0x4ea02800 },
    { .opc = 120832, .es = 1, .oes = 2, .kind = A64K_PAIRW, .form = A64F_2REG, .enc = 0x2e202800 },
    { .opc = 120833, .es = 1, .oes = 2, .kind = A64K_PAIRW, .form = A64F_2REG, .enc = 0x6e202800 },
    { .opc = 120834, .es = 2, .oes = 4, .kind = A64K_PAIRW, .form = A64F_2REG, .enc = 0x2e602800 },
    { .opc = 120835, .es = 2, .oes = 4, .kind = A64K_PAIRW, .form = A64F_2REG, .enc = 0x6e602800 },
    { .opc = 120836, .es = 4, .oes = 8, .kind = A64K_PAIRW, .form = A64F_2REG, .enc = 0x2ea02800 },
    { .opc = 120837, .es = 4, .oes = 8, .kind = A64K_PAIRW, .form = A64F_2REG, .enc = 0x6ea02800 },
    { .opc = 122883, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x0e212800 },
    { .opc = 122885, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x0e612800 },
    { .opc = 122887, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x0ea12800 },
    { .opc = 122883, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x0e212800 },
    { .opc = 122885, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x0e612800 },
    { .opc = 122887, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x0ea12800 },
    { .opc = 124931, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x0e214800 },
    { .opc = 124933, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x0e614800 },
    { .opc = 124935, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x0ea14800 },
    { .opc = 126979, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x2e214800 },
    { .opc = 126981, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x2e614800 },
    { .opc = 126983, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x2ea14800 },
    { .opc = 129027, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x2e212800 },
    { .opc = 129029, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x2e612800 },
    { .opc = 129031, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x2ea12800 },
    { .opc = 131072, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x0f080400 },
    { .opc = 131073, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x4f080400 },
    { .opc = 131074, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x0f100400 },
    { .opc = 131075, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x4f100400 },
    { .opc = 131076, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x0f200400 },
    { .opc = 131077, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x4f200400 },
    { .opc = 131078, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x5f400400 },
    { .opc = 131079, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x4f400400 },
    { .opc = 133120, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x2f080400 },
    { .opc = 133121, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x6f080400 },
    { .opc = 133122, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x2f100400 },
    { .opc = 133123, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x6f100400 },
    { .opc = 133124, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x2f200400 },
    { .opc = 133125, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x6f200400 },
    { .opc = 133126, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x7f400400 },
    { .opc = 133127, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x6f400400 },
    { .opc = 135168, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x0f082400 },
    { .opc = 135169, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x4f082400 },
    { .opc = 135170, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x0f102400 },
    { .opc = 135171, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x4f102400 },
    { .opc = 135172, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x0f202400 },
    { .opc = 135173, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x4f202400 },
    { .opc = 135174, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x5f402400 },
    { .opc = 135175, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x4f402400 },
    { .opc = 137216, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x2f082400 },
    { .opc = 137217, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x6f082400 },
    { .opc = 137218, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x2f102400 },
    { .opc = 137219, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x6f102400 },
    { .opc = 137220, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x2f202400 },
    { .opc = 137221, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x6f202400 },
    { .opc = 137222, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x7f402400 },
    { .opc = 137223, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x6f402400 },
    { .opc = 139264, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x0f085400 },
    { .opc = 139265, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f085400 },
    { .opc = 139266, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x0f105400 },
    { .opc = 139267, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f105400 },
    { .opc = 139268, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x0f205400 },
    { .opc = 139269, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f205400 },
    { .opc = 139270, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x5f405400 },
    { .opc = 139271, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f405400 },
    { .opc = 139264, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x0f085400 },
    { .opc = 139265, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f085400 },
    { .opc = 139266, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x0f105400 },
    { .opc = 139267, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f105400 },
    { .opc = 139268, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x0f205400 },
    { .opc = 139269, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f205400 },
    { .opc = 139270, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x5f405400 },
    { .opc = 139271, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f405400 },
    { .opc = 141312, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x0f087400 },
    { .opc = 141313, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f087400 },
    { .opc = 141314, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x0f107400 },
    { .opc = 141315, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f107400 },
    { .opc = 141316, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x0f207400 },
    { .opc = 141317, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f207400 },
    { .opc = 141318, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x5f407400 },
    { .opc = 141319, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f407400 },
    { .opc = 143360, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f087400 },
    { .opc = 143361, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f087400 },
    { .opc = 143362, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f107400 },
    { .opc = 143363, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f107400 },
    { .opc = 143364, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f207400 },
    { .opc = 143365, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f207400 },
    { .opc = 143366, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x7f407400 },
    { .opc = 143367, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f407400 },
    { .opc = 145408, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f086400 },
    { .opc = 145409, .es = 1, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f086400 },
    { .opc = 145410, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f106400 },
    { .opc = 145411, .es = 2, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f106400 },
    { .opc = 145412, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f206400 },
    { .opc = 145413, .es = 4, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f206400 },
    { .opc = 145414, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x7f406400 },
    { .opc = 145415, .es = 8, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f406400 },
    { .opc = 147456, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x0f08a400 },
    { .opc = 147458, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x0f10a400 },
    { .opc = 147460, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x0f20a400 },
    { .opc = 149504, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f08a400 },
    { .opc = 149506, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f10a400 },
    { .opc = 149508, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f20a400 },
    { .opc = 151553, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f08a400 },
    { .opc = 151555, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f10a400 },
    { .opc = 151557, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x4f20a400 },
    { .opc = 153601, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f08a400 },
    { .opc = 153603, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f10a400 },
    { .opc = 153605, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_2REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f20a400 },
    { .opc = 155651, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x0f088400 },
    { .opc = 155653, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x0f108400 },
    { .opc = 155655, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x0f208400 },
    { .opc = 157699, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x0f088c00 },
    { .opc = 157701, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x0f108c00 },
    { .opc = 157703, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x0f208c00 },
    { .opc = 159747, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x0f089400 },
    { .opc = 159749, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x0f109400 },
    { .opc = 159751, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x0f209400 },
    { .opc = 161795, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x2f089400 },
    { .opc = 161797, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x2f109400 },
    { .opc = 161799, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x2f209400 },
    { .opc = 163843, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x2f088400 },
    { .opc = 163845, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x2f108400 },
    { .opc = 163847, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x2f208400 },
    { .opc = 165891, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x0f089c00 },
    { .opc = 165893, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x0f109c00 },
    { .opc = 165895, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x0f209c00 },
    { .opc = 167939, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x2f089c00 },
    { .opc = 167941, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x2f109c00 },
    { .opc = 167943, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x2f209c00 },
    { .opc = 169987, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x2f088c00 },
    { .opc = 169989, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x2f108c00 },
    { .opc = 169991, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x2f208c00 },
    { .opc = 172035, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x0f088400 },
    { .opc = 172037, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x0f108400 },
    { .opc = 172039, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x0f208400 },
    { .opc = 174083, .es = 2, .oes = 1, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x0f088c00 },
    { .opc = 174085, .es = 4, .oes = 2, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x0f108c00 },
    { .opc = 174087, .es = 8, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x0f208c00 },
    { .opc = 176132, .es = 4, .form = A64F_2REG, .enc = 0x0ea1c800 },
    { .opc = 176133, .es = 4, .form = A64F_2REG, .enc = 0x4ea1c800 },
    { .opc = 178180, .es = 4, .form = A64F_2REG, .enc = 0x2ea1c800 },
    { .opc = 178181, .es = 4, .form = A64F_2REG, .enc = 0x6ea1c800 },
    { .opc = 180233, .es = 4, .fl = 1, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_2REG, .enc = 0x4e617800 },
    { .opc = 182283, .es = 8, .fl = 1, .oes = 4, .kind = A64K_NARROW, .form = A64F_2REG, .enc = 0x2e616800 },
    { .opc = -1 }
};
enum { A64OP2_ADD, A64OP2_ADDPOLY, A64OP2_SUB, A64OP2_MUL, A64OP2_FADD, A64OP2_FSUB, A64OP2_FMUL, A64OP2_FDIV, A64OP2_AND, A64OP2_ORR, A64OP2_EOR, A64OP2_BIC, A64OP2_ORN, A64OP2_FABD, A64OP2_FRECPS, A64OP2_FRSQRTS, A64OP2_SQADD, A64OP2_UQADD, A64OP2_SQSUB, A64OP2_UQSUB, A64OP2_SHADD, A64OP2_UHADD, A64OP2_SRHADD, A64OP2_URHADD, A64OP2_SHSUB, A64OP2_UHSUB, A64OP2_SABD, A64OP2_UABD, A64OP2_SMAX, A64OP2_UMAX, A64OP2_SMIN, A64OP2_UMIN, A64OP2_FMAX, A64OP2_FMIN, A64OP2_FMAXNM, A64OP2_FMINNM, A64OP2_SMAXP, A64OP2_UMAXP, A64OP2_SMINP, A64OP2_UMINP, A64OP2_FMAXP, A64OP2_FMINP, A64OP2_ADDP, A64OP2_FADDP, A64OP2_SMULL, A64OP2_UMULL, A64OP2_SMULL2, A64OP2_UMULL2, A64OP2_PMULL, A64OP2_PMULL2, A64OP2_PMUL, A64OP2_SQDMULH, A64OP2_SQRDMULH, A64OP2_SSHL, A64OP2_USHL, A64OP2_SRSHL, A64OP2_URSHL, A64OP2_SQSHL, A64OP2_UQSHL, A64OP2_CMEQ, A64OP2_CMGT, A64OP2_CMGE, A64OP2_CMHI, A64OP2_CMHS, A64OP2_CMTST, A64OP2_FCMEQ, A64OP2_FCMGT, A64OP2_FCMGE, A64OP2_TBL, A64OP2_TBLQ, A64OP2_JOIN, A64OP2_FCVTN2, A64OP2_SADALP, A64OP2_UADALP, A64OP2_XTN2, A64OP2_SQXTN2, A64OP2_UQXTN2, A64OP2_SQXTUN2, A64OP2_UXTN2, A64OP2_SSRA, A64OP2_USRA, A64OP2_SRSRA, A64OP2_URSRA, A64OP2_ADDHN, A64OP2_RADDHN, A64OP2_SUBHN, A64OP2_RSUBHN, A64OP2_SABDL, A64OP2_UABDL, A64OP2_SABDL2, A64OP2_UABDL2, A64OP2_SQDMULL, A64OP2_SQDMULL2, A64OP2_FMAXNMP, A64OP2_FMINNMP, A64OP2_MUL_L, A64OP2_FMUL_L, A64OP2_FMULX_L, A64OP2_SQDMULH_L, A64OP2_SQRDMULH_L, A64OP2_SMULL_L, A64OP2_UMULL_L, A64OP2_CEQ, A64OP2_CGT, A64OP2_CGE, A64OP2_CLT, A64OP2_CLE, A64OP2_SADDL, A64OP2_UADDL, A64OP2_SADDL2, A64OP2_UADDL2, A64OP2_SSUBL, A64OP2_USUBL, A64OP2_SSUBL2, A64OP2_USUBL2, A64OP2_SADDW, A64OP2_UADDW, A64OP2_SADDW2, A64OP2_UADDW2, A64OP2_SSUBW, A64OP2_USUBW, A64OP2_SSUBW2, A64OP2_USUBW2, A64OP2_SQRSHL, A64OP2_UQRSHL, A64OP2_SMULL2_L, A64OP2_UMULL2_L, A64OP2_SHRN2, A64OP2_RSHRN2, A64OP2_SQSHRN2, A64OP2_SQRSHRN2, A64OP2_SQSHRUN2, A64OP2_SQRSHRUN2, A64OP2_UQSHRN2, A64OP2_USHRN2, A64OP2_URSHRN2, A64OP2_UQRSHRN2, A64OP2_USQADD, A64OP2_SUQADD, A64OP2_SQDMULL_L, A64OP2_SQDMULL2_L, A64OP2_FMULX, A64OP2_CVTX2 };
static const AVop2 a64_op2_table[] = {
    { .opc = 0, .es = 1, .form = A64F_3REG, .enc = 0x0e208400 },
    { .opc = 1, .es = 1, .form = A64F_3REG, .enc = 0x4e208400 },
    { .opc = 2, .es = 2, .form = A64F_3REG, .enc = 0x0e608400 },
    { .opc = 3, .es = 2, .form = A64F_3REG, .enc = 0x4e608400 },
    { .opc = 4, .es = 4, .form = A64F_3REG, .enc = 0x0ea08400 },
    { .opc = 5, .es = 4, .form = A64F_3REG, .enc = 0x4ea08400 },
    { .opc = 6, .es = 8, .form = A64F_3REG, .enc = 0x5ee08400 },
    { .opc = 7, .es = 8, .form = A64F_3REG, .enc = 0x4ee08400 },
    { .opc = 0, .es = 1, .form = A64F_3REG, .enc = 0x0e208400 },
    { .opc = 1, .es = 1, .form = A64F_3REG, .enc = 0x4e208400 },
    { .opc = 2, .es = 2, .form = A64F_3REG, .enc = 0x0e608400 },
    { .opc = 3, .es = 2, .form = A64F_3REG, .enc = 0x4e608400 },
    { .opc = 4, .es = 4, .form = A64F_3REG, .enc = 0x0ea08400 },
    { .opc = 5, .es = 4, .form = A64F_3REG, .enc = 0x4ea08400 },
    { .opc = 6, .es = 8, .form = A64F_3REG, .enc = 0x5ee08400 },
    { .opc = 7, .es = 8, .form = A64F_3REG, .enc = 0x4ee08400 },
    { .opc = 2048, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x2e201c00 },
    { .opc = 2049, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x6e201c00 },
    { .opc = 2050, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x2e201c00 },
    { .opc = 2051, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x6e201c00 },
    { .opc = 4096, .es = 1, .form = A64F_3REG, .enc = 0x2e208400 },
    { .opc = 4097, .es = 1, .form = A64F_3REG, .enc = 0x6e208400 },
    { .opc = 4098, .es = 2, .form = A64F_3REG, .enc = 0x2e608400 },
    { .opc = 4099, .es = 2, .form = A64F_3REG, .enc = 0x6e608400 },
    { .opc = 4100, .es = 4, .form = A64F_3REG, .enc = 0x2ea08400 },
    { .opc = 4101, .es = 4, .form = A64F_3REG, .enc = 0x6ea08400 },
    { .opc = 4102, .es = 8, .form = A64F_3REG, .enc = 0x7ee08400 },
    { .opc = 4103, .es = 8, .form = A64F_3REG, .enc = 0x6ee08400 },
    { .opc = 4096, .es = 1, .form = A64F_3REG, .enc = 0x2e208400 },
    { .opc = 4097, .es = 1, .form = A64F_3REG, .enc = 0x6e208400 },
    { .opc = 4098, .es = 2, .form = A64F_3REG, .enc = 0x2e608400 },
    { .opc = 4099, .es = 2, .form = A64F_3REG, .enc = 0x6e608400 },
    { .opc = 4100, .es = 4, .form = A64F_3REG, .enc = 0x2ea08400 },
    { .opc = 4101, .es = 4, .form = A64F_3REG, .enc = 0x6ea08400 },
    { .opc = 4102, .es = 8, .form = A64F_3REG, .enc = 0x7ee08400 },
    { .opc = 4103, .es = 8, .form = A64F_3REG, .enc = 0x6ee08400 },
    { .opc = 6144, .es = 1, .form = A64F_3REG, .enc = 0x0e209c00 },
    { .opc = 6145, .es = 1, .form = A64F_3REG, .enc = 0x4e209c00 },
    { .opc = 6146, .es = 2, .form = A64F_3REG, .enc = 0x0e609c00 },
    { .opc = 6147, .es = 2, .form = A64F_3REG, .enc = 0x4e609c00 },
    { .opc = 6148, .es = 4, .form = A64F_3REG, .enc = 0x0ea09c00 },
    { .opc = 6149, .es = 4, .form = A64F_3REG, .enc = 0x4ea09c00 },
    { .opc = 6144, .es = 1, .form = A64F_3REG, .enc = 0x0e209c00 },
    { .opc = 6145, .es = 1, .form = A64F_3REG, .enc = 0x4e209c00 },
    { .opc = 6146, .es = 2, .form = A64F_3REG, .enc = 0x0e609c00 },
    { .opc = 6147, .es = 2, .form = A64F_3REG, .enc = 0x4e609c00 },
    { .opc = 6148, .es = 4, .form = A64F_3REG, .enc = 0x0ea09c00 },
    { .opc = 6149, .es = 4, .form = A64F_3REG, .enc = 0x4ea09c00 },
    { .opc = 8200, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0e20d400 },
    { .opc = 8201, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4e20d400 },
    { .opc = 8202, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x1e602800 },
    { .opc = 8203, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4e60d400 },
    { .opc = 10248, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0ea0d400 },
    { .opc = 10249, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4ea0d400 },
    { .opc = 10250, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x1e603800 },
    { .opc = 10251, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4ee0d400 },
    { .opc = 12296, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x2e20dc00 },
    { .opc = 12297, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x6e20dc00 },
    { .opc = 12298, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x1e600800 },
    { .opc = 12299, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x6e60dc00 },
    { .opc = 14344, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x2e20fc00 },
    { .opc = 14345, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x6e20fc00 },
    { .opc = 14346, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x1e601800 },
    { .opc = 14347, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x6e60fc00 },
    { .opc = 16384, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e201c00 },
    { .opc = 16385, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e201c00 },
    { .opc = 16386, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e201c00 },
    { .opc = 16387, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e201c00 },
    { .opc = 16388, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e201c00 },
    { .opc = 16389, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e201c00 },
    { .opc = 16390, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e201c00 },
    { .opc = 16391, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e201c00 },
    { .opc = 16384, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e201c00 },
    { .opc = 16385, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e201c00 },
    { .opc = 16386, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e201c00 },
    { .opc = 16387, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e201c00 },
    { .opc = 16388, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e201c00 },
    { .opc = 16389, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e201c00 },
    { .opc = 16390, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e201c00 },
    { .opc = 16391, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e201c00 },
    { .opc = 16384, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e201c00 },
    { .opc = 16385, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e201c00 },
    { .opc = 16386, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e201c00 },
    { .opc = 16387, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e201c00 },
    { .opc = 18432, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ea01c00 },
    { .opc = 18433, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ea01c00 },
    { .opc = 18434, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ea01c00 },
    { .opc = 18435, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ea01c00 },
    { .opc = 18436, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ea01c00 },
    { .opc = 18437, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ea01c00 },
    { .opc = 18438, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ea01c00 },
    { .opc = 18439, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ea01c00 },
    { .opc = 18432, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ea01c00 },
    { .opc = 18433, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ea01c00 },
    { .opc = 18434, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ea01c00 },
    { .opc = 18435, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ea01c00 },
    { .opc = 18436, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ea01c00 },
    { .opc = 18437, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ea01c00 },
    { .opc = 18438, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ea01c00 },
    { .opc = 18439, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ea01c00 },
    { .opc = 18432, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ea01c00 },
    { .opc = 18433, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ea01c00 },
    { .opc = 18434, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ea01c00 },
    { .opc = 18435, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ea01c00 },
    { .opc = 20480, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x2e201c00 },
    { .opc = 20481, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x6e201c00 },
    { .opc = 20482, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x2e201c00 },
    { .opc = 20483, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x6e201c00 },
    { .opc = 20484, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x2e201c00 },
    { .opc = 20485, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x6e201c00 },
    { .opc = 20486, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x2e201c00 },
    { .opc = 20487, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x6e201c00 },
    { .opc = 20480, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x2e201c00 },
    { .opc = 20481, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x6e201c00 },
    { .opc = 20482, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x2e201c00 },
    { .opc = 20483, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x6e201c00 },
    { .opc = 20484, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x2e201c00 },
    { .opc = 20485, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x6e201c00 },
    { .opc = 20486, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x2e201c00 },
    { .opc = 20487, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x6e201c00 },
    { .opc = 20480, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x2e201c00 },
    { .opc = 20481, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x6e201c00 },
    { .opc = 20482, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x2e201c00 },
    { .opc = 20483, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x6e201c00 },
    { .opc = 22528, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e601c00 },
    { .opc = 22529, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e601c00 },
    { .opc = 22530, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e601c00 },
    { .opc = 22531, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e601c00 },
    { .opc = 22532, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e601c00 },
    { .opc = 22533, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e601c00 },
    { .opc = 22534, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e601c00 },
    { .opc = 22535, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e601c00 },
    { .opc = 22528, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e601c00 },
    { .opc = 22529, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e601c00 },
    { .opc = 22530, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e601c00 },
    { .opc = 22531, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e601c00 },
    { .opc = 22532, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e601c00 },
    { .opc = 22533, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e601c00 },
    { .opc = 22534, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e601c00 },
    { .opc = 22535, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e601c00 },
    { .opc = 22528, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e601c00 },
    { .opc = 22529, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e601c00 },
    { .opc = 22530, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e601c00 },
    { .opc = 22531, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4e601c00 },
    { .opc = 24576, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ee01c00 },
    { .opc = 24577, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ee01c00 },
    { .opc = 24578, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ee01c00 },
    { .opc = 24579, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ee01c00 },
    { .opc = 24580, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ee01c00 },
    { .opc = 24581, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ee01c00 },
    { .opc = 24582, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ee01c00 },
    { .opc = 24583, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ee01c00 },
    { .opc = 24576, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ee01c00 },
    { .opc = 24577, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ee01c00 },
    { .opc = 24578, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ee01c00 },
    { .opc = 24579, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ee01c00 },
    { .opc = 24580, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ee01c00 },
    { .opc = 24581, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ee01c00 },
    { .opc = 24582, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ee01c00 },
    { .opc = 24583, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ee01c00 },
    { .opc = 24576, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ee01c00 },
    { .opc = 24577, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ee01c00 },
    { .opc = 24578, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0ee01c00 },
    { .opc = 24579, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x4ee01c00 },
    { .opc = 26632, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x2ea0d400 },
    { .opc = 26633, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x6ea0d400 },
    { .opc = 26634, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x7ee0d400 },
    { .opc = 26635, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x6ee0d400 },
    { .opc = 28680, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0e20fc00 },
    { .opc = 28681, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4e20fc00 },
    { .opc = 28682, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x5e60fc00 },
    { .opc = 28683, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4e60fc00 },
    { .opc = 30728, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0ea0fc00 },
    { .opc = 30729, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4ea0fc00 },
    { .opc = 30730, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x5ee0fc00 },
    { .opc = 30731, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4ee0fc00 },
    { .opc = 32768, .es = 1, .form = A64F_3REG, .enc = 0x0e200c00 },
    { .opc = 32769, .es = 1, .form = A64F_3REG, .enc = 0x4e200c00 },
    { .opc = 32770, .es = 2, .form = A64F_3REG, .enc = 0x0e600c00 },
    { .opc = 32771, .es = 2, .form = A64F_3REG, .enc = 0x4e600c00 },
    { .opc = 32772, .es = 4, .form = A64F_3REG, .enc = 0x0ea00c00 },
    { .opc = 32773, .es = 4, .form = A64F_3REG, .enc = 0x4ea00c00 },
    { .opc = 32774, .es = 8, .form = A64F_3REG, .enc = 0x5ee00c00 },
    { .opc = 32775, .es = 8, .form = A64F_3REG, .enc = 0x4ee00c00 },
    { .opc = 34816, .es = 1, .form = A64F_3REG, .enc = 0x2e200c00 },
    { .opc = 34817, .es = 1, .form = A64F_3REG, .enc = 0x6e200c00 },
    { .opc = 34818, .es = 2, .form = A64F_3REG, .enc = 0x2e600c00 },
    { .opc = 34819, .es = 2, .form = A64F_3REG, .enc = 0x6e600c00 },
    { .opc = 34820, .es = 4, .form = A64F_3REG, .enc = 0x2ea00c00 },
    { .opc = 34821, .es = 4, .form = A64F_3REG, .enc = 0x6ea00c00 },
    { .opc = 34822, .es = 8, .form = A64F_3REG, .enc = 0x7ee00c00 },
    { .opc = 34823, .es = 8, .form = A64F_3REG, .enc = 0x6ee00c00 },
    { .opc = 36864, .es = 1, .form = A64F_3REG, .enc = 0x0e202c00 },
    { .opc = 36865, .es = 1, .form = A64F_3REG, .enc = 0x4e202c00 },
    { .opc = 36866, .es = 2, .form = A64F_3REG, .enc = 0x0e602c00 },
    { .opc = 36867, .es = 2, .form = A64F_3REG, .enc = 0x4e602c00 },
    { .opc = 36868, .es = 4, .form = A64F_3REG, .enc = 0x0ea02c00 },
    { .opc = 36869, .es = 4, .form = A64F_3REG, .enc = 0x4ea02c00 },
    { .opc = 36870, .es = 8, .form = A64F_3REG, .enc = 0x5ee02c00 },
    { .opc = 36871, .es = 8, .form = A64F_3REG, .enc = 0x4ee02c00 },
    { .opc = 38912, .es = 1, .form = A64F_3REG, .enc = 0x2e202c00 },
    { .opc = 38913, .es = 1, .form = A64F_3REG, .enc = 0x6e202c00 },
    { .opc = 38914, .es = 2, .form = A64F_3REG, .enc = 0x2e602c00 },
    { .opc = 38915, .es = 2, .form = A64F_3REG, .enc = 0x6e602c00 },
    { .opc = 38916, .es = 4, .form = A64F_3REG, .enc = 0x2ea02c00 },
    { .opc = 38917, .es = 4, .form = A64F_3REG, .enc = 0x6ea02c00 },
    { .opc = 38918, .es = 8, .form = A64F_3REG, .enc = 0x7ee02c00 },
    { .opc = 38919, .es = 8, .form = A64F_3REG, .enc = 0x6ee02c00 },
    { .opc = 40960, .es = 1, .form = A64F_3REG, .enc = 0x0e200400 },
    { .opc = 40961, .es = 1, .form = A64F_3REG, .enc = 0x4e200400 },
    { .opc = 40962, .es = 2, .form = A64F_3REG, .enc = 0x0e600400 },
    { .opc = 40963, .es = 2, .form = A64F_3REG, .enc = 0x4e600400 },
    { .opc = 40964, .es = 4, .form = A64F_3REG, .enc = 0x0ea00400 },
    { .opc = 40965, .es = 4, .form = A64F_3REG, .enc = 0x4ea00400 },
    { .opc = 43008, .es = 1, .form = A64F_3REG, .enc = 0x2e200400 },
    { .opc = 43009, .es = 1, .form = A64F_3REG, .enc = 0x6e200400 },
    { .opc = 43010, .es = 2, .form = A64F_3REG, .enc = 0x2e600400 },
    { .opc = 43011, .es = 2, .form = A64F_3REG, .enc = 0x6e600400 },
    { .opc = 43012, .es = 4, .form = A64F_3REG, .enc = 0x2ea00400 },
    { .opc = 43013, .es = 4, .form = A64F_3REG, .enc = 0x6ea00400 },
    { .opc = 45056, .es = 1, .form = A64F_3REG, .enc = 0x0e201400 },
    { .opc = 45057, .es = 1, .form = A64F_3REG, .enc = 0x4e201400 },
    { .opc = 45058, .es = 2, .form = A64F_3REG, .enc = 0x0e601400 },
    { .opc = 45059, .es = 2, .form = A64F_3REG, .enc = 0x4e601400 },
    { .opc = 45060, .es = 4, .form = A64F_3REG, .enc = 0x0ea01400 },
    { .opc = 45061, .es = 4, .form = A64F_3REG, .enc = 0x4ea01400 },
    { .opc = 47104, .es = 1, .form = A64F_3REG, .enc = 0x2e201400 },
    { .opc = 47105, .es = 1, .form = A64F_3REG, .enc = 0x6e201400 },
    { .opc = 47106, .es = 2, .form = A64F_3REG, .enc = 0x2e601400 },
    { .opc = 47107, .es = 2, .form = A64F_3REG, .enc = 0x6e601400 },
    { .opc = 47108, .es = 4, .form = A64F_3REG, .enc = 0x2ea01400 },
    { .opc = 47109, .es = 4, .form = A64F_3REG, .enc = 0x6ea01400 },
    { .opc = 49152, .es = 1, .form = A64F_3REG, .enc = 0x0e202400 },
    { .opc = 49153, .es = 1, .form = A64F_3REG, .enc = 0x4e202400 },
    { .opc = 49154, .es = 2, .form = A64F_3REG, .enc = 0x0e602400 },
    { .opc = 49155, .es = 2, .form = A64F_3REG, .enc = 0x4e602400 },
    { .opc = 49156, .es = 4, .form = A64F_3REG, .enc = 0x0ea02400 },
    { .opc = 49157, .es = 4, .form = A64F_3REG, .enc = 0x4ea02400 },
    { .opc = 51200, .es = 1, .form = A64F_3REG, .enc = 0x2e202400 },
    { .opc = 51201, .es = 1, .form = A64F_3REG, .enc = 0x6e202400 },
    { .opc = 51202, .es = 2, .form = A64F_3REG, .enc = 0x2e602400 },
    { .opc = 51203, .es = 2, .form = A64F_3REG, .enc = 0x6e602400 },
    { .opc = 51204, .es = 4, .form = A64F_3REG, .enc = 0x2ea02400 },
    { .opc = 51205, .es = 4, .form = A64F_3REG, .enc = 0x6ea02400 },
    { .opc = 53248, .es = 1, .form = A64F_3REG, .enc = 0x0e207400 },
    { .opc = 53249, .es = 1, .form = A64F_3REG, .enc = 0x4e207400 },
    { .opc = 53250, .es = 2, .form = A64F_3REG, .enc = 0x0e607400 },
    { .opc = 53251, .es = 2, .form = A64F_3REG, .enc = 0x4e607400 },
    { .opc = 53252, .es = 4, .form = A64F_3REG, .enc = 0x0ea07400 },
    { .opc = 53253, .es = 4, .form = A64F_3REG, .enc = 0x4ea07400 },
    { .opc = 55296, .es = 1, .form = A64F_3REG, .enc = 0x2e207400 },
    { .opc = 55297, .es = 1, .form = A64F_3REG, .enc = 0x6e207400 },
    { .opc = 55298, .es = 2, .form = A64F_3REG, .enc = 0x2e607400 },
    { .opc = 55299, .es = 2, .form = A64F_3REG, .enc = 0x6e607400 },
    { .opc = 55300, .es = 4, .form = A64F_3REG, .enc = 0x2ea07400 },
    { .opc = 55301, .es = 4, .form = A64F_3REG, .enc = 0x6ea07400 },
    { .opc = 57344, .es = 1, .form = A64F_3REG, .enc = 0x0e206400 },
    { .opc = 57345, .es = 1, .form = A64F_3REG, .enc = 0x4e206400 },
    { .opc = 57346, .es = 2, .form = A64F_3REG, .enc = 0x0e606400 },
    { .opc = 57347, .es = 2, .form = A64F_3REG, .enc = 0x4e606400 },
    { .opc = 57348, .es = 4, .form = A64F_3REG, .enc = 0x0ea06400 },
    { .opc = 57349, .es = 4, .form = A64F_3REG, .enc = 0x4ea06400 },
    { .opc = 59392, .es = 1, .form = A64F_3REG, .enc = 0x2e206400 },
    { .opc = 59393, .es = 1, .form = A64F_3REG, .enc = 0x6e206400 },
    { .opc = 59394, .es = 2, .form = A64F_3REG, .enc = 0x2e606400 },
    { .opc = 59395, .es = 2, .form = A64F_3REG, .enc = 0x6e606400 },
    { .opc = 59396, .es = 4, .form = A64F_3REG, .enc = 0x2ea06400 },
    { .opc = 59397, .es = 4, .form = A64F_3REG, .enc = 0x6ea06400 },
    { .opc = 61440, .es = 1, .form = A64F_3REG, .enc = 0x0e206c00 },
    { .opc = 61441, .es = 1, .form = A64F_3REG, .enc = 0x4e206c00 },
    { .opc = 61442, .es = 2, .form = A64F_3REG, .enc = 0x0e606c00 },
    { .opc = 61443, .es = 2, .form = A64F_3REG, .enc = 0x4e606c00 },
    { .opc = 61444, .es = 4, .form = A64F_3REG, .enc = 0x0ea06c00 },
    { .opc = 61445, .es = 4, .form = A64F_3REG, .enc = 0x4ea06c00 },
    { .opc = 63488, .es = 1, .form = A64F_3REG, .enc = 0x2e206c00 },
    { .opc = 63489, .es = 1, .form = A64F_3REG, .enc = 0x6e206c00 },
    { .opc = 63490, .es = 2, .form = A64F_3REG, .enc = 0x2e606c00 },
    { .opc = 63491, .es = 2, .form = A64F_3REG, .enc = 0x6e606c00 },
    { .opc = 63492, .es = 4, .form = A64F_3REG, .enc = 0x2ea06c00 },
    { .opc = 63493, .es = 4, .form = A64F_3REG, .enc = 0x6ea06c00 },
    { .opc = 65544, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0e20f400 },
    { .opc = 65545, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4e20f400 },
    { .opc = 65546, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x1e604800 },
    { .opc = 65547, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4e60f400 },
    { .opc = 67592, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0ea0f400 },
    { .opc = 67593, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4ea0f400 },
    { .opc = 67594, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x1e605800 },
    { .opc = 67595, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4ee0f400 },
    { .opc = 69640, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0e20c400 },
    { .opc = 69641, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4e20c400 },
    { .opc = 69642, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x1e606800 },
    { .opc = 69643, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4e60c400 },
    { .opc = 71688, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0ea0c400 },
    { .opc = 71689, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4ea0c400 },
    { .opc = 71690, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x1e607800 },
    { .opc = 71691, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4ee0c400 },
    { .opc = 73728, .es = 1, .form = A64F_3REG, .enc = 0x0e20a400 },
    { .opc = 73729, .es = 1, .form = A64F_3REG, .enc = 0x4e20a400 },
    { .opc = 73730, .es = 2, .form = A64F_3REG, .enc = 0x0e60a400 },
    { .opc = 73731, .es = 2, .form = A64F_3REG, .enc = 0x4e60a400 },
    { .opc = 73732, .es = 4, .form = A64F_3REG, .enc = 0x0ea0a400 },
    { .opc = 73733, .es = 4, .form = A64F_3REG, .enc = 0x4ea0a400 },
    { .opc = 75776, .es = 1, .form = A64F_3REG, .enc = 0x2e20a400 },
    { .opc = 75777, .es = 1, .form = A64F_3REG, .enc = 0x6e20a400 },
    { .opc = 75778, .es = 2, .form = A64F_3REG, .enc = 0x2e60a400 },
    { .opc = 75779, .es = 2, .form = A64F_3REG, .enc = 0x6e60a400 },
    { .opc = 75780, .es = 4, .form = A64F_3REG, .enc = 0x2ea0a400 },
    { .opc = 75781, .es = 4, .form = A64F_3REG, .enc = 0x6ea0a400 },
    { .opc = 77824, .es = 1, .form = A64F_3REG, .enc = 0x0e20ac00 },
    { .opc = 77825, .es = 1, .form = A64F_3REG, .enc = 0x4e20ac00 },
    { .opc = 77826, .es = 2, .form = A64F_3REG, .enc = 0x0e60ac00 },
    { .opc = 77827, .es = 2, .form = A64F_3REG, .enc = 0x4e60ac00 },
    { .opc = 77828, .es = 4, .form = A64F_3REG, .enc = 0x0ea0ac00 },
    { .opc = 77829, .es = 4, .form = A64F_3REG, .enc = 0x4ea0ac00 },
    { .opc = 79872, .es = 1, .form = A64F_3REG, .enc = 0x2e20ac00 },
    { .opc = 79873, .es = 1, .form = A64F_3REG, .enc = 0x6e20ac00 },
    { .opc = 79874, .es = 2, .form = A64F_3REG, .enc = 0x2e60ac00 },
    { .opc = 79875, .es = 2, .form = A64F_3REG, .enc = 0x6e60ac00 },
    { .opc = 79876, .es = 4, .form = A64F_3REG, .enc = 0x2ea0ac00 },
    { .opc = 79877, .es = 4, .form = A64F_3REG, .enc = 0x6ea0ac00 },
    { .opc = 81928, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x2e20f400 },
    { .opc = 81929, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x6e20f400 },
    { .opc = 81931, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x6e60f400 },
    { .opc = 83976, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x2ea0f400 },
    { .opc = 83977, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x6ea0f400 },
    { .opc = 83979, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x6ee0f400 },
    { .opc = 86016, .es = 1, .form = A64F_3REG, .enc = 0x0e20bc00 },
    { .opc = 86017, .es = 1, .form = A64F_3REG, .enc = 0x4e20bc00 },
    { .opc = 86018, .es = 2, .form = A64F_3REG, .enc = 0x0e60bc00 },
    { .opc = 86019, .es = 2, .form = A64F_3REG, .enc = 0x4e60bc00 },
    { .opc = 86020, .es = 4, .form = A64F_3REG, .enc = 0x0ea0bc00 },
    { .opc = 86021, .es = 4, .form = A64F_3REG, .enc = 0x4ea0bc00 },
    { .opc = 86023, .es = 8, .form = A64F_3REG, .enc = 0x4ee0bc00 },
    { .opc = 86016, .es = 1, .form = A64F_3REG, .enc = 0x0e20bc00 },
    { .opc = 86017, .es = 1, .form = A64F_3REG, .enc = 0x4e20bc00 },
    { .opc = 86018, .es = 2, .form = A64F_3REG, .enc = 0x0e60bc00 },
    { .opc = 86019, .es = 2, .form = A64F_3REG, .enc = 0x4e60bc00 },
    { .opc = 86020, .es = 4, .form = A64F_3REG, .enc = 0x0ea0bc00 },
    { .opc = 86021, .es = 4, .form = A64F_3REG, .enc = 0x4ea0bc00 },
    { .opc = 86023, .es = 8, .form = A64F_3REG, .enc = 0x4ee0bc00 },
    { .opc = 88072, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x2e20d400 },
    { .opc = 88073, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x6e20d400 },
    { .opc = 88075, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x6e60d400 },
    { .opc = 90112, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0e20c000 },
    { .opc = 90114, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0e60c000 },
    { .opc = 90116, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0ea0c000 },
    { .opc = 92160, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x2e20c000 },
    { .opc = 92162, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x2e60c000 },
    { .opc = 92164, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x2ea0c000 },
    { .opc = 94209, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4e20c000 },
    { .opc = 94211, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4e60c000 },
    { .opc = 94213, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4ea0c000 },
    { .opc = 96257, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x6e20c000 },
    { .opc = 96259, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x6e60c000 },
    { .opc = 96261, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x6ea0c000 },
    { .opc = 98304, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0e20e000 },
    { .opc = 100353, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4e20e000 },
    { .opc = 102400, .es = 1, .form = A64F_3REG, .enc = 0x2e209c00 },
    { .opc = 102401, .es = 1, .form = A64F_3REG, .enc = 0x6e209c00 },
    { .opc = 104450, .es = 2, .form = A64F_3REG, .enc = 0x0e60b400 },
    { .opc = 104451, .es = 2, .form = A64F_3REG, .enc = 0x4e60b400 },
    { .opc = 104452, .es = 4, .form = A64F_3REG, .enc = 0x0ea0b400 },
    { .opc = 104453, .es = 4, .form = A64F_3REG, .enc = 0x4ea0b400 },
    { .opc = 106498, .es = 2, .form = A64F_3REG, .enc = 0x2e60b400 },
    { .opc = 106499, .es = 2, .form = A64F_3REG, .enc = 0x6e60b400 },
    { .opc = 106500, .es = 4, .form = A64F_3REG, .enc = 0x2ea0b400 },
    { .opc = 106501, .es = 4, .form = A64F_3REG, .enc = 0x6ea0b400 },
    { .opc = 108544, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x0e204400 },
    { .opc = 108545, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4e204400 },
    { .opc = 108546, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x0e604400 },
    { .opc = 108547, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4e604400 },
    { .opc = 108548, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x0ea04400 },
    { .opc = 108549, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4ea04400 },
    { .opc = 108550, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x5ee04400 },
    { .opc = 108551, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4ee04400 },
    { .opc = 110592, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x2e204400 },
    { .opc = 110593, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6e204400 },
    { .opc = 110594, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x2e604400 },
    { .opc = 110595, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6e604400 },
    { .opc = 110596, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x2ea04400 },
    { .opc = 110597, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6ea04400 },
    { .opc = 110598, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x7ee04400 },
    { .opc = 110599, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6ee04400 },
    { .opc = 112640, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x0e205400 },
    { .opc = 112641, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4e205400 },
    { .opc = 112642, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x0e605400 },
    { .opc = 112643, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4e605400 },
    { .opc = 112644, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x0ea05400 },
    { .opc = 112645, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4ea05400 },
    { .opc = 112646, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x5ee05400 },
    { .opc = 112647, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4ee05400 },
    { .opc = 114688, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x2e205400 },
    { .opc = 114689, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6e205400 },
    { .opc = 114690, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x2e605400 },
    { .opc = 114691, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6e605400 },
    { .opc = 114692, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x2ea05400 },
    { .opc = 114693, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6ea05400 },
    { .opc = 114694, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x7ee05400 },
    { .opc = 114695, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6ee05400 },
    { .opc = 116736, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x0e204c00 },
    { .opc = 116737, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4e204c00 },
    { .opc = 116738, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x0e604c00 },
    { .opc = 116739, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4e604c00 },
    { .opc = 116740, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x0ea04c00 },
    { .opc = 116741, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4ea04c00 },
    { .opc = 116742, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x5ee04c00 },
    { .opc = 116743, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4ee04c00 },
    { .opc = 118784, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x2e204c00 },
    { .opc = 118785, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6e204c00 },
    { .opc = 118786, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x2e604c00 },
    { .opc = 118787, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6e604c00 },
    { .opc = 118788, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x2ea04c00 },
    { .opc = 118789, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6ea04c00 },
    { .opc = 118790, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x7ee04c00 },
    { .opc = 118791, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6ee04c00 },
    { .opc = 120832, .es = 1, .form = A64F_3REG, .enc = 0x2e208c00 },
    { .opc = 120833, .es = 1, .form = A64F_3REG, .enc = 0x6e208c00 },
    { .opc = 120834, .es = 2, .form = A64F_3REG, .enc = 0x2e608c00 },
    { .opc = 120835, .es = 2, .form = A64F_3REG, .enc = 0x6e608c00 },
    { .opc = 120836, .es = 4, .form = A64F_3REG, .enc = 0x2ea08c00 },
    { .opc = 120837, .es = 4, .form = A64F_3REG, .enc = 0x6ea08c00 },
    { .opc = 120838, .es = 8, .form = A64F_3REG, .enc = 0x7ee08c00 },
    { .opc = 120839, .es = 8, .form = A64F_3REG, .enc = 0x6ee08c00 },
    { .opc = 120832, .es = 1, .form = A64F_3REG, .enc = 0x2e208c00 },
    { .opc = 120833, .es = 1, .form = A64F_3REG, .enc = 0x6e208c00 },
    { .opc = 120834, .es = 2, .form = A64F_3REG, .enc = 0x2e608c00 },
    { .opc = 120835, .es = 2, .form = A64F_3REG, .enc = 0x6e608c00 },
    { .opc = 120836, .es = 4, .form = A64F_3REG, .enc = 0x2ea08c00 },
    { .opc = 120837, .es = 4, .form = A64F_3REG, .enc = 0x6ea08c00 },
    { .opc = 120838, .es = 8, .form = A64F_3REG, .enc = 0x7ee08c00 },
    { .opc = 120839, .es = 8, .form = A64F_3REG, .enc = 0x6ee08c00 },
    { .opc = 122880, .es = 1, .form = A64F_3REG, .enc = 0x0e203400 },
    { .opc = 122881, .es = 1, .form = A64F_3REG, .enc = 0x4e203400 },
    { .opc = 122882, .es = 2, .form = A64F_3REG, .enc = 0x0e603400 },
    { .opc = 122883, .es = 2, .form = A64F_3REG, .enc = 0x4e603400 },
    { .opc = 122884, .es = 4, .form = A64F_3REG, .enc = 0x0ea03400 },
    { .opc = 122885, .es = 4, .form = A64F_3REG, .enc = 0x4ea03400 },
    { .opc = 122886, .es = 8, .form = A64F_3REG, .enc = 0x5ee03400 },
    { .opc = 122887, .es = 8, .form = A64F_3REG, .enc = 0x4ee03400 },
    { .opc = 124928, .es = 1, .form = A64F_3REG, .enc = 0x0e203c00 },
    { .opc = 124929, .es = 1, .form = A64F_3REG, .enc = 0x4e203c00 },
    { .opc = 124930, .es = 2, .form = A64F_3REG, .enc = 0x0e603c00 },
    { .opc = 124931, .es = 2, .form = A64F_3REG, .enc = 0x4e603c00 },
    { .opc = 124932, .es = 4, .form = A64F_3REG, .enc = 0x0ea03c00 },
    { .opc = 124933, .es = 4, .form = A64F_3REG, .enc = 0x4ea03c00 },
    { .opc = 124934, .es = 8, .form = A64F_3REG, .enc = 0x5ee03c00 },
    { .opc = 124935, .es = 8, .form = A64F_3REG, .enc = 0x4ee03c00 },
    { .opc = 126976, .es = 1, .form = A64F_3REG, .enc = 0x2e203400 },
    { .opc = 126977, .es = 1, .form = A64F_3REG, .enc = 0x6e203400 },
    { .opc = 126978, .es = 2, .form = A64F_3REG, .enc = 0x2e603400 },
    { .opc = 126979, .es = 2, .form = A64F_3REG, .enc = 0x6e603400 },
    { .opc = 126980, .es = 4, .form = A64F_3REG, .enc = 0x2ea03400 },
    { .opc = 126981, .es = 4, .form = A64F_3REG, .enc = 0x6ea03400 },
    { .opc = 126982, .es = 8, .form = A64F_3REG, .enc = 0x7ee03400 },
    { .opc = 126983, .es = 8, .form = A64F_3REG, .enc = 0x6ee03400 },
    { .opc = 129024, .es = 1, .form = A64F_3REG, .enc = 0x2e203c00 },
    { .opc = 129025, .es = 1, .form = A64F_3REG, .enc = 0x6e203c00 },
    { .opc = 129026, .es = 2, .form = A64F_3REG, .enc = 0x2e603c00 },
    { .opc = 129027, .es = 2, .form = A64F_3REG, .enc = 0x6e603c00 },
    { .opc = 129028, .es = 4, .form = A64F_3REG, .enc = 0x2ea03c00 },
    { .opc = 129029, .es = 4, .form = A64F_3REG, .enc = 0x6ea03c00 },
    { .opc = 129030, .es = 8, .form = A64F_3REG, .enc = 0x7ee03c00 },
    { .opc = 129031, .es = 8, .form = A64F_3REG, .enc = 0x6ee03c00 },
    { .opc = 131072, .es = 1, .form = A64F_3REG, .enc = 0x0e208c00 },
    { .opc = 131073, .es = 1, .form = A64F_3REG, .enc = 0x4e208c00 },
    { .opc = 131074, .es = 2, .form = A64F_3REG, .enc = 0x0e608c00 },
    { .opc = 131075, .es = 2, .form = A64F_3REG, .enc = 0x4e608c00 },
    { .opc = 131076, .es = 4, .form = A64F_3REG, .enc = 0x0ea08c00 },
    { .opc = 131077, .es = 4, .form = A64F_3REG, .enc = 0x4ea08c00 },
    { .opc = 131078, .es = 8, .form = A64F_3REG, .enc = 0x5ee08c00 },
    { .opc = 131079, .es = 8, .form = A64F_3REG, .enc = 0x4ee08c00 },
    { .opc = 131072, .es = 1, .form = A64F_3REG, .enc = 0x0e208c00 },
    { .opc = 131073, .es = 1, .form = A64F_3REG, .enc = 0x4e208c00 },
    { .opc = 131074, .es = 2, .form = A64F_3REG, .enc = 0x0e608c00 },
    { .opc = 131075, .es = 2, .form = A64F_3REG, .enc = 0x4e608c00 },
    { .opc = 131076, .es = 4, .form = A64F_3REG, .enc = 0x0ea08c00 },
    { .opc = 131077, .es = 4, .form = A64F_3REG, .enc = 0x4ea08c00 },
    { .opc = 131078, .es = 8, .form = A64F_3REG, .enc = 0x5ee08c00 },
    { .opc = 131079, .es = 8, .form = A64F_3REG, .enc = 0x4ee08c00 },
    { .opc = 131072, .es = 1, .form = A64F_3REG, .enc = 0x0e208c00 },
    { .opc = 131073, .es = 1, .form = A64F_3REG, .enc = 0x4e208c00 },
    { .opc = 131074, .es = 2, .form = A64F_3REG, .enc = 0x0e608c00 },
    { .opc = 131075, .es = 2, .form = A64F_3REG, .enc = 0x4e608c00 },
    { .opc = 133128, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0e20e400 },
    { .opc = 133129, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4e20e400 },
    { .opc = 133130, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x5e60e400 },
    { .opc = 133131, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4e60e400 },
    { .opc = 135176, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x2ea0e400 },
    { .opc = 135177, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x6ea0e400 },
    { .opc = 135178, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x7ee0e400 },
    { .opc = 135179, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x6ee0e400 },
    { .opc = 137224, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x2e20e400 },
    { .opc = 137225, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x6e20e400 },
    { .opc = 137226, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x7e60e400 },
    { .opc = 137227, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x6e60e400 },
    { .opc = 139264, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e000000 },
    { .opc = 139264, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e000000 },
    { .opc = 139264, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG, .enc = 0x0e000000 },
    { .opc = 141312, .es = 1, .kind = A64K_TBLQ, .form = A64F_3REG, .enc = 0x0e000000 },
    { .opc = 141313, .es = 1, .kind = A64K_TBLQ, .form = A64F_3REG, .enc = 0x4e000000 },
    { .opc = 141312, .es = 1, .kind = A64K_TBLQ, .form = A64F_3REG, .enc = 0x0e000000 },
    { .opc = 141313, .es = 1, .kind = A64K_TBLQ, .form = A64F_3REG, .enc = 0x4e000000 },
    { .opc = 141312, .es = 1, .kind = A64K_TBLQ, .form = A64F_3REG, .enc = 0x0e000000 },
    { .opc = 141313, .es = 1, .kind = A64K_TBLQ, .form = A64F_3REG, .enc = 0x4e000000 },
    { .opc = 143360, .es = 1, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 143362, .es = 2, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 143364, .es = 4, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 143366, .es = 8, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 143360, .es = 1, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 143362, .es = 2, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 143364, .es = 4, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 143366, .es = 8, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 143360, .es = 1, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 143362, .es = 2, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 143366, .es = 8, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 143368, .es = 4, .fl = 1, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 143370, .es = 8, .fl = 1, .kind = A64K_JOIN, .form = A64F_JOIN },
    { .opc = 145416, .es = 4, .fl = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x4e616800 },
    { .opc = 147458, .es = 2, .kind = A64K_ACCW1, .form = A64F_2REG_ACC, .enc = 0x0e206800 },
    { .opc = 147459, .es = 2, .kind = A64K_ACCW1, .form = A64F_2REG_ACC, .enc = 0x4e206800 },
    { .opc = 147460, .es = 4, .kind = A64K_ACCW1, .form = A64F_2REG_ACC, .enc = 0x0e606800 },
    { .opc = 147461, .es = 4, .kind = A64K_ACCW1, .form = A64F_2REG_ACC, .enc = 0x4e606800 },
    { .opc = 147462, .es = 8, .kind = A64K_ACCW1, .form = A64F_2REG_ACC, .enc = 0x0ea06800 },
    { .opc = 147463, .es = 8, .kind = A64K_ACCW1, .form = A64F_2REG_ACC, .enc = 0x4ea06800 },
    { .opc = 149506, .es = 2, .kind = A64K_ACCW1, .form = A64F_2REG_ACC, .enc = 0x2e206800 },
    { .opc = 149507, .es = 2, .kind = A64K_ACCW1, .form = A64F_2REG_ACC, .enc = 0x6e206800 },
    { .opc = 149508, .es = 4, .kind = A64K_ACCW1, .form = A64F_2REG_ACC, .enc = 0x2e606800 },
    { .opc = 149509, .es = 4, .kind = A64K_ACCW1, .form = A64F_2REG_ACC, .enc = 0x6e606800 },
    { .opc = 149510, .es = 8, .kind = A64K_ACCW1, .form = A64F_2REG_ACC, .enc = 0x2ea06800 },
    { .opc = 149511, .es = 8, .kind = A64K_ACCW1, .form = A64F_2REG_ACC, .enc = 0x6ea06800 },
    { .opc = 151552, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x4e212800 },
    { .opc = 151554, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x4e612800 },
    { .opc = 151556, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x4ea12800 },
    { .opc = 153600, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x4e214800 },
    { .opc = 153602, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x4e614800 },
    { .opc = 153604, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x4ea14800 },
    { .opc = 155648, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x6e214800 },
    { .opc = 155650, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x6e614800 },
    { .opc = 155652, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x6ea14800 },
    { .opc = 157696, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x6e212800 },
    { .opc = 157698, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x6e612800 },
    { .opc = 157700, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x6ea12800 },
    { .opc = 159744, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x4e212800 },
    { .opc = 159746, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x4e612800 },
    { .opc = 159748, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x4ea12800 },
    { .opc = 161792, .es = 1, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x0f081400 },
    { .opc = 161793, .es = 1, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x4f081400 },
    { .opc = 161794, .es = 2, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x0f101400 },
    { .opc = 161795, .es = 2, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x4f101400 },
    { .opc = 161796, .es = 4, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x0f201400 },
    { .opc = 161797, .es = 4, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x4f201400 },
    { .opc = 161798, .es = 8, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x5f401400 },
    { .opc = 161799, .es = 8, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x4f401400 },
    { .opc = 163840, .es = 1, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x2f081400 },
    { .opc = 163841, .es = 1, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x6f081400 },
    { .opc = 163842, .es = 2, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x2f101400 },
    { .opc = 163843, .es = 2, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x6f101400 },
    { .opc = 163844, .es = 4, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x2f201400 },
    { .opc = 163845, .es = 4, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x6f201400 },
    { .opc = 163846, .es = 8, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x7f401400 },
    { .opc = 163847, .es = 8, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x6f401400 },
    { .opc = 165888, .es = 1, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x0f083400 },
    { .opc = 165889, .es = 1, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x4f083400 },
    { .opc = 165890, .es = 2, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x0f103400 },
    { .opc = 165891, .es = 2, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x4f103400 },
    { .opc = 165892, .es = 4, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x0f203400 },
    { .opc = 165893, .es = 4, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x4f203400 },
    { .opc = 165894, .es = 8, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x5f403400 },
    { .opc = 165895, .es = 8, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x4f403400 },
    { .opc = 167936, .es = 1, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x2f083400 },
    { .opc = 167937, .es = 1, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x6f083400 },
    { .opc = 167938, .es = 2, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x2f103400 },
    { .opc = 167939, .es = 2, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x6f103400 },
    { .opc = 167940, .es = 4, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x2f203400 },
    { .opc = 167941, .es = 4, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x6f203400 },
    { .opc = 167942, .es = 8, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x7f403400 },
    { .opc = 167943, .es = 8, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x6f403400 },
    { .opc = 169987, .es = 2, .oes = 1, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x0e204000 },
    { .opc = 169989, .es = 4, .oes = 2, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x0e604000 },
    { .opc = 169991, .es = 8, .oes = 4, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x0ea04000 },
    { .opc = 169987, .es = 2, .oes = 1, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x0e204000 },
    { .opc = 169989, .es = 4, .oes = 2, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x0e604000 },
    { .opc = 169991, .es = 8, .oes = 4, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x0ea04000 },
    { .opc = 172035, .es = 2, .oes = 1, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x2e204000 },
    { .opc = 172037, .es = 4, .oes = 2, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x2e604000 },
    { .opc = 172039, .es = 8, .oes = 4, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x2ea04000 },
    { .opc = 172035, .es = 2, .oes = 1, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x2e204000 },
    { .opc = 172037, .es = 4, .oes = 2, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x2e604000 },
    { .opc = 172039, .es = 8, .oes = 4, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x2ea04000 },
    { .opc = 174083, .es = 2, .oes = 1, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x0e206000 },
    { .opc = 174085, .es = 4, .oes = 2, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x0e606000 },
    { .opc = 174087, .es = 8, .oes = 4, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x0ea06000 },
    { .opc = 174083, .es = 2, .oes = 1, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x0e206000 },
    { .opc = 174085, .es = 4, .oes = 2, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x0e606000 },
    { .opc = 174087, .es = 8, .oes = 4, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x0ea06000 },
    { .opc = 176131, .es = 2, .oes = 1, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x2e206000 },
    { .opc = 176133, .es = 4, .oes = 2, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x2e606000 },
    { .opc = 176135, .es = 8, .oes = 4, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x2ea06000 },
    { .opc = 176131, .es = 2, .oes = 1, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x2e206000 },
    { .opc = 176133, .es = 4, .oes = 2, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x2e606000 },
    { .opc = 176135, .es = 8, .oes = 4, .kind = A64K_NARROW2, .form = A64F_3REG, .enc = 0x2ea06000 },
    { .opc = 178176, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0e207000 },
    { .opc = 178178, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0e607000 },
    { .opc = 178180, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0ea07000 },
    { .opc = 180224, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x2e207000 },
    { .opc = 180226, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x2e607000 },
    { .opc = 180228, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x2ea07000 },
    { .opc = 182273, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4e207000 },
    { .opc = 182275, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4e607000 },
    { .opc = 182277, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4ea07000 },
    { .opc = 184321, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x6e207000 },
    { .opc = 184323, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x6e607000 },
    { .opc = 184325, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x6ea07000 },
    { .opc = 186370, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0e60d000 },
    { .opc = 186372, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0ea0d000 },
    { .opc = 188419, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4e60d000 },
    { .opc = 188421, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4ea0d000 },
    { .opc = 190472, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x2e20c400 },
    { .opc = 190473, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x6e20c400 },
    { .opc = 190475, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x6e60c400 },
    { .opc = 192520, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x2ea0c400 },
    { .opc = 192521, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x6ea0c400 },
    { .opc = 192523, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x6ee0c400 },
    { .opc = 194562, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x0f408000 },
    { .opc = 194563, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x4f408000 },
    { .opc = 194564, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f808000 },
    { .opc = 194565, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x4f808000 },
    { .opc = 194562, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x0f408000 },
    { .opc = 194563, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x4f408000 },
    { .opc = 194564, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f808000 },
    { .opc = 194565, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x4f808000 },
    { .opc = 196616, .es = 4, .fl = 1, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f809000 },
    { .opc = 196617, .es = 4, .fl = 1, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x4f809000 },
    { .opc = 196618, .es = 8, .fl = 1, .form = A64F_3REG_IDX, .imm_pos = 11, .imm_sign = 1, .enc = 0x5fc09000 },
    { .opc = 196619, .es = 8, .fl = 1, .form = A64F_3REG_IDX, .imm_pos = 11, .imm_sign = 1, .enc = 0x4fc09000 },
    { .opc = 198664, .es = 4, .fl = 1, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x2f809000 },
    { .opc = 198665, .es = 4, .fl = 1, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x6f809000 },
    { .opc = 198666, .es = 8, .fl = 1, .form = A64F_3REG_IDX, .imm_pos = 11, .imm_sign = 1, .enc = 0x7fc09000 },
    { .opc = 198667, .es = 8, .fl = 1, .form = A64F_3REG_IDX, .imm_pos = 11, .imm_sign = 1, .enc = 0x6fc09000 },
    { .opc = 200706, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x0f40c000 },
    { .opc = 200707, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x4f40c000 },
    { .opc = 200708, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f80c000 },
    { .opc = 200709, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x4f80c000 },
    { .opc = 202754, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x0f40d000 },
    { .opc = 202755, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x4f40d000 },
    { .opc = 202756, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f80d000 },
    { .opc = 202757, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x4f80d000 },
    { .opc = 204802, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x0f40a000 },
    { .opc = 204804, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f80a000 },
    { .opc = 206850, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x2f40a000 },
    { .opc = 206852, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x2f80a000 },
    { .opc = 219136, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0e200000 },
    { .opc = 219138, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0e600000 },
    { .opc = 219140, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0ea00000 },
    { .opc = 221184, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x2e200000 },
    { .opc = 221186, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x2e600000 },
    { .opc = 221188, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x2ea00000 },
    { .opc = 223233, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4e200000 },
    { .opc = 223235, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4e600000 },
    { .opc = 223237, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4ea00000 },
    { .opc = 225281, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x6e200000 },
    { .opc = 225283, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x6e600000 },
    { .opc = 225285, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x6ea00000 },
    { .opc = 227328, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0e202000 },
    { .opc = 227330, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0e602000 },
    { .opc = 227332, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x0ea02000 },
    { .opc = 229376, .es = 1, .oes = 2, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x2e202000 },
    { .opc = 229378, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x2e602000 },
    { .opc = 229380, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_3REG, .enc = 0x2ea02000 },
    { .opc = 231425, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4e202000 },
    { .opc = 231427, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4e602000 },
    { .opc = 231429, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x4ea02000 },
    { .opc = 233473, .es = 1, .oes = 2, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x6e202000 },
    { .opc = 233475, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x6e602000 },
    { .opc = 233477, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_3REG, .enc = 0x6ea02000 },
    { .opc = 235523, .es = 2, .kind = A64K_ADDW_LO, .form = A64F_3REG, .enc = 0x0e201000 },
    { .opc = 235525, .es = 4, .kind = A64K_ADDW_LO, .form = A64F_3REG, .enc = 0x0e601000 },
    { .opc = 235527, .es = 8, .kind = A64K_ADDW_LO, .form = A64F_3REG, .enc = 0x0ea01000 },
    { .opc = 237571, .es = 2, .kind = A64K_ADDW_LO, .form = A64F_3REG, .enc = 0x2e201000 },
    { .opc = 237573, .es = 4, .kind = A64K_ADDW_LO, .form = A64F_3REG, .enc = 0x2e601000 },
    { .opc = 237575, .es = 8, .kind = A64K_ADDW_LO, .form = A64F_3REG, .enc = 0x2ea01000 },
    { .opc = 239619, .es = 2, .kind = A64K_ADDW_HI, .form = A64F_3REG, .enc = 0x4e201000 },
    { .opc = 239621, .es = 4, .kind = A64K_ADDW_HI, .form = A64F_3REG, .enc = 0x4e601000 },
    { .opc = 239623, .es = 8, .kind = A64K_ADDW_HI, .form = A64F_3REG, .enc = 0x4ea01000 },
    { .opc = 241667, .es = 2, .kind = A64K_ADDW_HI, .form = A64F_3REG, .enc = 0x6e201000 },
    { .opc = 241669, .es = 4, .kind = A64K_ADDW_HI, .form = A64F_3REG, .enc = 0x6e601000 },
    { .opc = 241671, .es = 8, .kind = A64K_ADDW_HI, .form = A64F_3REG, .enc = 0x6ea01000 },
    { .opc = 243715, .es = 2, .kind = A64K_ADDW_LO, .form = A64F_3REG, .enc = 0x0e203000 },
    { .opc = 243717, .es = 4, .kind = A64K_ADDW_LO, .form = A64F_3REG, .enc = 0x0e603000 },
    { .opc = 243719, .es = 8, .kind = A64K_ADDW_LO, .form = A64F_3REG, .enc = 0x0ea03000 },
    { .opc = 245763, .es = 2, .kind = A64K_ADDW_LO, .form = A64F_3REG, .enc = 0x2e203000 },
    { .opc = 245765, .es = 4, .kind = A64K_ADDW_LO, .form = A64F_3REG, .enc = 0x2e603000 },
    { .opc = 245767, .es = 8, .kind = A64K_ADDW_LO, .form = A64F_3REG, .enc = 0x2ea03000 },
    { .opc = 247811, .es = 2, .kind = A64K_ADDW_HI, .form = A64F_3REG, .enc = 0x4e203000 },
    { .opc = 247813, .es = 4, .kind = A64K_ADDW_HI, .form = A64F_3REG, .enc = 0x4e603000 },
    { .opc = 247815, .es = 8, .kind = A64K_ADDW_HI, .form = A64F_3REG, .enc = 0x4ea03000 },
    { .opc = 249859, .es = 2, .kind = A64K_ADDW_HI, .form = A64F_3REG, .enc = 0x6e203000 },
    { .opc = 249861, .es = 4, .kind = A64K_ADDW_HI, .form = A64F_3REG, .enc = 0x6e603000 },
    { .opc = 249863, .es = 8, .kind = A64K_ADDW_HI, .form = A64F_3REG, .enc = 0x6ea03000 },
    { .opc = 251904, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x0e205c00 },
    { .opc = 251905, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4e205c00 },
    { .opc = 251906, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x0e605c00 },
    { .opc = 251907, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4e605c00 },
    { .opc = 251908, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x0ea05c00 },
    { .opc = 251909, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4ea05c00 },
    { .opc = 251910, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x5ee05c00 },
    { .opc = 251911, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x4ee05c00 },
    { .opc = 253952, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x2e205c00 },
    { .opc = 253953, .es = 1, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6e205c00 },
    { .opc = 253954, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x2e605c00 },
    { .opc = 253955, .es = 2, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6e605c00 },
    { .opc = 253956, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x2ea05c00 },
    { .opc = 253957, .es = 4, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6ea05c00 },
    { .opc = 253958, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x7ee05c00 },
    { .opc = 253959, .es = 8, .kind = A64K_SHIFT, .form = A64F_3REG, .enc = 0x6ee05c00 },
    { .opc = 256003, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x4f40a000 },
    { .opc = 256005, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x4f80a000 },
    { .opc = 258051, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x6f40a000 },
    { .opc = 258053, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x6f80a000 },
    { .opc = 260096, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x4f088400 },
    { .opc = 260098, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x4f108400 },
    { .opc = 260100, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x4f208400 },
    { .opc = 262144, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x4f088c00 },
    { .opc = 262146, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x4f108c00 },
    { .opc = 262148, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x4f208c00 },
    { .opc = 264192, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x4f089400 },
    { .opc = 264194, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x4f109400 },
    { .opc = 264196, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x4f209400 },
    { .opc = 266240, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x4f089c00 },
    { .opc = 266242, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x4f109c00 },
    { .opc = 266244, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x4f209c00 },
    { .opc = 268288, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x6f088400 },
    { .opc = 268290, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x6f108400 },
    { .opc = 268292, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x6f208400 },
    { .opc = 270336, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x6f088c00 },
    { .opc = 270338, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x6f108c00 },
    { .opc = 270340, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x6f208c00 },
    { .opc = 272384, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x6f089400 },
    { .opc = 272386, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x6f109400 },
    { .opc = 272388, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x6f209400 },
    { .opc = 274432, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x4f088400 },
    { .opc = 274434, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x4f108400 },
    { .opc = 274436, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x4f208400 },
    { .opc = 276480, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x4f088c00 },
    { .opc = 276482, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x4f108c00 },
    { .opc = 276484, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x4f208c00 },
    { .opc = 278528, .es = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x6f089c00 },
    { .opc = 278530, .es = 2, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x6f109c00 },
    { .opc = 278532, .es = 4, .kind = A64K_NARROW_HI, .form = A64F_2REG_IMM_ACC, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x6f209c00 },
    { .opc = 280576, .es = 1, .form = A64F_2REG_ACC, .enc = 0x2e203800 },
    { .opc = 280577, .es = 1, .form = A64F_2REG_ACC, .enc = 0x6e203800 },
    { .opc = 280578, .es = 2, .form = A64F_2REG_ACC, .enc = 0x2e603800 },
    { .opc = 280579, .es = 2, .form = A64F_2REG_ACC, .enc = 0x6e603800 },
    { .opc = 280580, .es = 4, .form = A64F_2REG_ACC, .enc = 0x2ea03800 },
    { .opc = 280581, .es = 4, .form = A64F_2REG_ACC, .enc = 0x6ea03800 },
    { .opc = 280582, .es = 8, .form = A64F_2REG_ACC, .enc = 0x7ee03800 },
    { .opc = 280583, .es = 8, .form = A64F_2REG_ACC, .enc = 0x6ee03800 },
    { .opc = 282624, .es = 1, .form = A64F_2REG_ACC, .enc = 0x0e203800 },
    { .opc = 282625, .es = 1, .form = A64F_2REG_ACC, .enc = 0x4e203800 },
    { .opc = 282626, .es = 2, .form = A64F_2REG_ACC, .enc = 0x0e603800 },
    { .opc = 282627, .es = 2, .form = A64F_2REG_ACC, .enc = 0x4e603800 },
    { .opc = 282628, .es = 4, .form = A64F_2REG_ACC, .enc = 0x0ea03800 },
    { .opc = 282629, .es = 4, .form = A64F_2REG_ACC, .enc = 0x4ea03800 },
    { .opc = 282630, .es = 8, .form = A64F_2REG_ACC, .enc = 0x5ee03800 },
    { .opc = 282631, .es = 8, .form = A64F_2REG_ACC, .enc = 0x4ee03800 },
    { .opc = 284674, .es = 2, .oes = 4, .kind = A64K_WIDEN_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x0f40b000 },
    { .opc = 284676, .es = 4, .oes = 8, .kind = A64K_WIDEN_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f80b000 },
    { .opc = 286723, .es = 2, .oes = 4, .kind = A64K_WIDEN_HI, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x4f40b000 },
    { .opc = 286725, .es = 4, .oes = 8, .kind = A64K_WIDEN_HI, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x4f80b000 },
    { .opc = 288776, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0e20dc00 },
    { .opc = 288777, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4e20dc00 },
    { .opc = 288778, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x5e60dc00 },
    { .opc = 288779, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4e60dc00 },
    { .opc = 290824, .es = 4, .fl = 1, .kind = A64K_NARROW_HI, .form = A64F_2REG_ACC, .enc = 0x6e616800 },
    { .opc = -1 }
};
enum { A64OP3_MLA, A64OP3_MLS, A64OP3_FMLA, A64OP3_FMLS, A64OP3_SABA, A64OP3_UABA, A64OP3_TBX, A64OP3_TBXQ, A64OP3_BSL, A64OP3_BIT, A64OP3_BIF, A64OP3_SMLAL, A64OP3_UMLAL, A64OP3_SMLAL2, A64OP3_UMLAL2, A64OP3_SMLSL, A64OP3_UMLSL, A64OP3_SMLSL2, A64OP3_UMLSL2, A64OP3_SDOT, A64OP3_UDOT, A64OP3_ADDHN2, A64OP3_RADDHN2, A64OP3_SUBHN2, A64OP3_RSUBHN2, A64OP3_SABAL, A64OP3_UABAL, A64OP3_SABAL2, A64OP3_UABAL2, A64OP3_SRI, A64OP3_SLI, A64OP3_MLA_L, A64OP3_MLS_L, A64OP3_FMLA_L, A64OP3_FMLS_L, A64OP3_SMLAL_L, A64OP3_UMLAL_L, A64OP3_SMLSL_L, A64OP3_UMLSL_L, A64OP3_SDOT_L, A64OP3_UDOT_L, A64OP3_SQRDMLAH, A64OP3_SQRDMLSH, A64OP3_SQRDMLAH_L, A64OP3_SQRDMLSH_L, A64OP3_SQDMLAL, A64OP3_SQDMLSL, A64OP3_SQDMLAL2, A64OP3_SQDMLSL2, A64OP3_SQDMLAL_L, A64OP3_SQDMLSL_L, A64OP3_SQDMLAL2_L, A64OP3_SQDMLSL2_L };
static const AVop3 a64_op3_table[] = {
    { .opc = 0, .es = 1, .form = A64F_3REG_ACC, .enc = 0x0e209400 },
    { .opc = 1, .es = 1, .form = A64F_3REG_ACC, .enc = 0x4e209400 },
    { .opc = 2, .es = 2, .form = A64F_3REG_ACC, .enc = 0x0e609400 },
    { .opc = 3, .es = 2, .form = A64F_3REG_ACC, .enc = 0x4e609400 },
    { .opc = 4, .es = 4, .form = A64F_3REG_ACC, .enc = 0x0ea09400 },
    { .opc = 5, .es = 4, .form = A64F_3REG_ACC, .enc = 0x4ea09400 },
    { .opc = 0, .es = 1, .form = A64F_3REG_ACC, .enc = 0x0e209400 },
    { .opc = 1, .es = 1, .form = A64F_3REG_ACC, .enc = 0x4e209400 },
    { .opc = 2, .es = 2, .form = A64F_3REG_ACC, .enc = 0x0e609400 },
    { .opc = 3, .es = 2, .form = A64F_3REG_ACC, .enc = 0x4e609400 },
    { .opc = 4, .es = 4, .form = A64F_3REG_ACC, .enc = 0x0ea09400 },
    { .opc = 5, .es = 4, .form = A64F_3REG_ACC, .enc = 0x4ea09400 },
    { .opc = 2048, .es = 1, .form = A64F_3REG_ACC, .enc = 0x2e209400 },
    { .opc = 2049, .es = 1, .form = A64F_3REG_ACC, .enc = 0x6e209400 },
    { .opc = 2050, .es = 2, .form = A64F_3REG_ACC, .enc = 0x2e609400 },
    { .opc = 2051, .es = 2, .form = A64F_3REG_ACC, .enc = 0x6e609400 },
    { .opc = 2052, .es = 4, .form = A64F_3REG_ACC, .enc = 0x2ea09400 },
    { .opc = 2053, .es = 4, .form = A64F_3REG_ACC, .enc = 0x6ea09400 },
    { .opc = 2048, .es = 1, .form = A64F_3REG_ACC, .enc = 0x2e209400 },
    { .opc = 2049, .es = 1, .form = A64F_3REG_ACC, .enc = 0x6e209400 },
    { .opc = 2050, .es = 2, .form = A64F_3REG_ACC, .enc = 0x2e609400 },
    { .opc = 2051, .es = 2, .form = A64F_3REG_ACC, .enc = 0x6e609400 },
    { .opc = 2052, .es = 4, .form = A64F_3REG_ACC, .enc = 0x2ea09400 },
    { .opc = 2053, .es = 4, .form = A64F_3REG_ACC, .enc = 0x6ea09400 },
    { .opc = 4104, .es = 4, .fl = 1, .form = A64F_3REG_ACC, .enc = 0x0e20cc00 },
    { .opc = 4105, .es = 4, .fl = 1, .form = A64F_3REG_ACC, .enc = 0x4e20cc00 },
    { .opc = 4106, .es = 8, .fl = 1, .form = A64F_3REG_FMA64, .enc = 0x1f400000 },
    { .opc = 4107, .es = 8, .fl = 1, .form = A64F_3REG_ACC, .enc = 0x4e60cc00 },
    { .opc = 6152, .es = 4, .fl = 1, .form = A64F_3REG_ACC, .enc = 0x0ea0cc00 },
    { .opc = 6153, .es = 4, .fl = 1, .form = A64F_3REG_ACC, .enc = 0x4ea0cc00 },
    { .opc = 6154, .es = 8, .fl = 1, .form = A64F_3REG_FMA64, .enc = 0x1f408000 },
    { .opc = 6155, .es = 8, .fl = 1, .form = A64F_3REG_ACC, .enc = 0x4ee0cc00 },
    { .opc = 8192, .es = 1, .form = A64F_3REG_ACC, .enc = 0x0e207c00 },
    { .opc = 8193, .es = 1, .form = A64F_3REG_ACC, .enc = 0x4e207c00 },
    { .opc = 8194, .es = 2, .form = A64F_3REG_ACC, .enc = 0x0e607c00 },
    { .opc = 8195, .es = 2, .form = A64F_3REG_ACC, .enc = 0x4e607c00 },
    { .opc = 8196, .es = 4, .form = A64F_3REG_ACC, .enc = 0x0ea07c00 },
    { .opc = 8197, .es = 4, .form = A64F_3REG_ACC, .enc = 0x4ea07c00 },
    { .opc = 10240, .es = 1, .form = A64F_3REG_ACC, .enc = 0x2e207c00 },
    { .opc = 10241, .es = 1, .form = A64F_3REG_ACC, .enc = 0x6e207c00 },
    { .opc = 10242, .es = 2, .form = A64F_3REG_ACC, .enc = 0x2e607c00 },
    { .opc = 10243, .es = 2, .form = A64F_3REG_ACC, .enc = 0x6e607c00 },
    { .opc = 10244, .es = 4, .form = A64F_3REG_ACC, .enc = 0x2ea07c00 },
    { .opc = 10245, .es = 4, .form = A64F_3REG_ACC, .enc = 0x6ea07c00 },
    { .opc = 12288, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x0e001000 },
    { .opc = 12288, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x0e001000 },
    { .opc = 12288, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x0e001000 },
    { .opc = 14336, .es = 1, .kind = A64K_TBLQ, .form = A64F_3REG_ACC, .enc = 0x0e001000 },
    { .opc = 14337, .es = 1, .kind = A64K_TBLQ, .form = A64F_3REG_ACC, .enc = 0x4e001000 },
    { .opc = 14336, .es = 1, .kind = A64K_TBLQ, .form = A64F_3REG_ACC, .enc = 0x0e001000 },
    { .opc = 14337, .es = 1, .kind = A64K_TBLQ, .form = A64F_3REG_ACC, .enc = 0x4e001000 },
    { .opc = 14336, .es = 1, .kind = A64K_TBLQ, .form = A64F_3REG_ACC, .enc = 0x0e001000 },
    { .opc = 14337, .es = 1, .kind = A64K_TBLQ, .form = A64F_3REG_ACC, .enc = 0x4e001000 },
    { .opc = 16384, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2e601c00 },
    { .opc = 16385, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6e601c00 },
    { .opc = 16386, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2e601c00 },
    { .opc = 16387, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6e601c00 },
    { .opc = 16388, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2e601c00 },
    { .opc = 16389, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6e601c00 },
    { .opc = 16390, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2e601c00 },
    { .opc = 16391, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6e601c00 },
    { .opc = 16384, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2e601c00 },
    { .opc = 16385, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6e601c00 },
    { .opc = 16386, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2e601c00 },
    { .opc = 16387, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6e601c00 },
    { .opc = 16388, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2e601c00 },
    { .opc = 16389, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6e601c00 },
    { .opc = 16390, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2e601c00 },
    { .opc = 16391, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6e601c00 },
    { .opc = 16384, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2e601c00 },
    { .opc = 16385, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6e601c00 },
    { .opc = 16386, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2e601c00 },
    { .opc = 16387, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6e601c00 },
    { .opc = 16392, .es = 4, .fl = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2e601c00 },
    { .opc = 16393, .es = 4, .fl = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6e601c00 },
    { .opc = 16394, .es = 8, .fl = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2e601c00 },
    { .opc = 16395, .es = 8, .fl = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6e601c00 },
    { .opc = 18432, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ea01c00 },
    { .opc = 18433, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ea01c00 },
    { .opc = 18434, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ea01c00 },
    { .opc = 18435, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ea01c00 },
    { .opc = 18436, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ea01c00 },
    { .opc = 18437, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ea01c00 },
    { .opc = 18438, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ea01c00 },
    { .opc = 18439, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ea01c00 },
    { .opc = 18432, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ea01c00 },
    { .opc = 18433, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ea01c00 },
    { .opc = 18434, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ea01c00 },
    { .opc = 18435, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ea01c00 },
    { .opc = 18436, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ea01c00 },
    { .opc = 18437, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ea01c00 },
    { .opc = 18438, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ea01c00 },
    { .opc = 18439, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ea01c00 },
    { .opc = 18432, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ea01c00 },
    { .opc = 18433, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ea01c00 },
    { .opc = 18434, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ea01c00 },
    { .opc = 18435, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ea01c00 },
    { .opc = 20480, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ee01c00 },
    { .opc = 20481, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ee01c00 },
    { .opc = 20482, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ee01c00 },
    { .opc = 20483, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ee01c00 },
    { .opc = 20484, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ee01c00 },
    { .opc = 20485, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ee01c00 },
    { .opc = 20486, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ee01c00 },
    { .opc = 20487, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ee01c00 },
    { .opc = 20480, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ee01c00 },
    { .opc = 20481, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ee01c00 },
    { .opc = 20482, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ee01c00 },
    { .opc = 20483, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ee01c00 },
    { .opc = 20484, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ee01c00 },
    { .opc = 20485, .es = 4, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ee01c00 },
    { .opc = 20486, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ee01c00 },
    { .opc = 20487, .es = 8, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ee01c00 },
    { .opc = 20480, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ee01c00 },
    { .opc = 20481, .es = 1, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ee01c00 },
    { .opc = 20482, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x2ee01c00 },
    { .opc = 20483, .es = 2, .kind = A64K_BYTES, .form = A64F_3REG_ACC, .enc = 0x6ee01c00 },
    { .opc = 22531, .es = 2, .oes = 1, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0e208000 },
    { .opc = 22533, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0e608000 },
    { .opc = 22535, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0ea08000 },
    { .opc = 24579, .es = 2, .oes = 1, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x2e208000 },
    { .opc = 24581, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x2e608000 },
    { .opc = 24583, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x2ea08000 },
    { .opc = 26627, .es = 2, .oes = 1, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4e208000 },
    { .opc = 26629, .es = 4, .oes = 2, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4e608000 },
    { .opc = 26631, .es = 8, .oes = 4, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4ea08000 },
    { .opc = 28675, .es = 2, .oes = 1, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x6e208000 },
    { .opc = 28677, .es = 4, .oes = 2, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x6e608000 },
    { .opc = 28679, .es = 8, .oes = 4, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x6ea08000 },
    { .opc = 30723, .es = 2, .oes = 1, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0e20a000 },
    { .opc = 30725, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0e60a000 },
    { .opc = 30727, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0ea0a000 },
    { .opc = 32771, .es = 2, .oes = 1, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x2e20a000 },
    { .opc = 32773, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x2e60a000 },
    { .opc = 32775, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x2ea0a000 },
    { .opc = 34819, .es = 2, .oes = 1, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4e20a000 },
    { .opc = 34821, .es = 4, .oes = 2, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4e60a000 },
    { .opc = 34823, .es = 8, .oes = 4, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4ea0a000 },
    { .opc = 36867, .es = 2, .oes = 1, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x6e20a000 },
    { .opc = 36869, .es = 4, .oes = 2, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x6e60a000 },
    { .opc = 36871, .es = 8, .oes = 4, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x6ea0a000 },
    { .opc = 38916, .es = 4, .oes = 1, .kind = A64K_DOT, .form = A64F_3REG_ACC, .feat = TFEAT_DOTPROD, .enc = 0x0e809400 },
    { .opc = 38917, .es = 4, .oes = 1, .kind = A64K_DOT, .form = A64F_3REG_ACC, .feat = TFEAT_DOTPROD, .enc = 0x4e809400 },
    { .opc = 40964, .es = 4, .oes = 1, .kind = A64K_DOT, .form = A64F_3REG_ACC, .feat = TFEAT_DOTPROD, .enc = 0x2e809400 },
    { .opc = 40965, .es = 4, .oes = 1, .kind = A64K_DOT, .form = A64F_3REG_ACC, .feat = TFEAT_DOTPROD, .enc = 0x6e809400 },
    { .opc = 43008, .es = 1, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x4e204000 },
    { .opc = 43010, .es = 2, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x4e604000 },
    { .opc = 43012, .es = 4, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x4ea04000 },
    { .opc = 43008, .es = 1, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x4e204000 },
    { .opc = 43010, .es = 2, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x4e604000 },
    { .opc = 43012, .es = 4, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x4ea04000 },
    { .opc = 45056, .es = 1, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x6e204000 },
    { .opc = 45058, .es = 2, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x6e604000 },
    { .opc = 45060, .es = 4, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x6ea04000 },
    { .opc = 45056, .es = 1, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x6e204000 },
    { .opc = 45058, .es = 2, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x6e604000 },
    { .opc = 45060, .es = 4, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x6ea04000 },
    { .opc = 47104, .es = 1, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x4e206000 },
    { .opc = 47106, .es = 2, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x4e606000 },
    { .opc = 47108, .es = 4, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x4ea06000 },
    { .opc = 47104, .es = 1, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x4e206000 },
    { .opc = 47106, .es = 2, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x4e606000 },
    { .opc = 47108, .es = 4, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x4ea06000 },
    { .opc = 49152, .es = 1, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x6e206000 },
    { .opc = 49154, .es = 2, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x6e606000 },
    { .opc = 49156, .es = 4, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x6ea06000 },
    { .opc = 49152, .es = 1, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x6e206000 },
    { .opc = 49154, .es = 2, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x6e606000 },
    { .opc = 49156, .es = 4, .kind = A64K_NARROW2_HI, .form = A64F_3REG_ACC, .enc = 0x6ea06000 },
    { .opc = 51203, .es = 2, .oes = 1, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0e205000 },
    { .opc = 51205, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0e605000 },
    { .opc = 51207, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0ea05000 },
    { .opc = 53251, .es = 2, .oes = 1, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x2e205000 },
    { .opc = 53253, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x2e605000 },
    { .opc = 53255, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x2ea05000 },
    { .opc = 55299, .es = 2, .oes = 1, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4e205000 },
    { .opc = 55301, .es = 4, .oes = 2, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4e605000 },
    { .opc = 55303, .es = 8, .oes = 4, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4ea05000 },
    { .opc = 57347, .es = 2, .oes = 1, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x6e205000 },
    { .opc = 57349, .es = 4, .oes = 2, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x6e605000 },
    { .opc = 57351, .es = 8, .oes = 4, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x6ea05000 },
    { .opc = 59392, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x2f084400 },
    { .opc = 59393, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x6f084400 },
    { .opc = 59394, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x2f104400 },
    { .opc = 59395, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x6f104400 },
    { .opc = 59396, .es = 4, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x2f204400 },
    { .opc = 59397, .es = 4, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x6f204400 },
    { .opc = 59398, .es = 8, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x7f404400 },
    { .opc = 59399, .es = 8, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x6f404400 },
    { .opc = 59392, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x2f084400 },
    { .opc = 59393, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x6f084400 },
    { .opc = 59394, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x2f104400 },
    { .opc = 59395, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x6f104400 },
    { .opc = 59396, .es = 4, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x2f204400 },
    { .opc = 59397, .es = 4, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 32, .imm_pos = 16, .imm_base = 32, .imm_sign = -1, .enc = 0x6f204400 },
    { .opc = 59398, .es = 8, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x7f404400 },
    { .opc = 59399, .es = 8, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 64, .imm_pos = 16, .imm_base = 64, .imm_sign = -1, .enc = 0x6f404400 },
    { .opc = 59392, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x2f084400 },
    { .opc = 59393, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 8, .imm_pos = 16, .imm_base = 8, .imm_sign = -1, .enc = 0x6f084400 },
    { .opc = 59394, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x2f104400 },
    { .opc = 59395, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 1, .imm_hi = 16, .imm_pos = 16, .imm_base = 16, .imm_sign = -1, .enc = 0x6f104400 },
    { .opc = 61440, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f085400 },
    { .opc = 61441, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f085400 },
    { .opc = 61442, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f105400 },
    { .opc = 61443, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f105400 },
    { .opc = 61444, .es = 4, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f205400 },
    { .opc = 61445, .es = 4, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f205400 },
    { .opc = 61446, .es = 8, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x7f405400 },
    { .opc = 61447, .es = 8, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f405400 },
    { .opc = 61440, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f085400 },
    { .opc = 61441, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f085400 },
    { .opc = 61442, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f105400 },
    { .opc = 61443, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f105400 },
    { .opc = 61444, .es = 4, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f205400 },
    { .opc = 61445, .es = 4, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 31, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f205400 },
    { .opc = 61446, .es = 8, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x7f405400 },
    { .opc = 61447, .es = 8, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 63, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f405400 },
    { .opc = 61440, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f085400 },
    { .opc = 61441, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f085400 },
    { .opc = 61442, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x2f105400 },
    { .opc = 61443, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 16, .imm_sign = 1, .enc = 0x6f105400 },
    { .opc = 63490, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x2f400000 },
    { .opc = 63491, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x6f400000 },
    { .opc = 63492, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x2f800000 },
    { .opc = 63493, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x6f800000 },
    { .opc = 63490, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x2f400000 },
    { .opc = 63491, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x6f400000 },
    { .opc = 63492, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x2f800000 },
    { .opc = 63493, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x6f800000 },
    { .opc = 65538, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x2f404000 },
    { .opc = 65539, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x6f404000 },
    { .opc = 65540, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x2f804000 },
    { .opc = 65541, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x6f804000 },
    { .opc = 65538, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x2f404000 },
    { .opc = 65539, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x6f404000 },
    { .opc = 65540, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x2f804000 },
    { .opc = 65541, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x6f804000 },
    { .opc = 67592, .es = 4, .fl = 1, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f801000 },
    { .opc = 67593, .es = 4, .fl = 1, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x4f801000 },
    { .opc = 67594, .es = 8, .fl = 1, .form = A64F_3REG_IDX, .imm_pos = 11, .imm_sign = 1, .enc = 0x5fc01000 },
    { .opc = 67595, .es = 8, .fl = 1, .form = A64F_3REG_IDX, .imm_pos = 11, .imm_sign = 1, .enc = 0x4fc01000 },
    { .opc = 69640, .es = 4, .fl = 1, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f805000 },
    { .opc = 69641, .es = 4, .fl = 1, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x4f805000 },
    { .opc = 69642, .es = 8, .fl = 1, .form = A64F_3REG_IDX, .imm_pos = 11, .imm_sign = 1, .enc = 0x5fc05000 },
    { .opc = 69643, .es = 8, .fl = 1, .form = A64F_3REG_IDX, .imm_pos = 11, .imm_sign = 1, .enc = 0x4fc05000 },
    { .opc = 71685, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x0f402000 },
    { .opc = 71687, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f802000 },
    { .opc = 73733, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x2f402000 },
    { .opc = 73735, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x2f802000 },
    { .opc = 75781, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x0f406000 },
    { .opc = 75783, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f806000 },
    { .opc = 77829, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x2f406000 },
    { .opc = 77831, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x2f806000 },
    { .opc = 79876, .es = 4, .oes = 1, .kind = A64K_DOT, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .feat = TFEAT_DOTPROD, .enc = 0x0f80e000 },
    { .opc = 79877, .es = 4, .oes = 1, .kind = A64K_DOT, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .feat = TFEAT_DOTPROD, .enc = 0x4f80e000 },
    { .opc = 81924, .es = 4, .oes = 1, .kind = A64K_DOT, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .feat = TFEAT_DOTPROD, .enc = 0x2f80e000 },
    { .opc = 81925, .es = 4, .oes = 1, .kind = A64K_DOT, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .feat = TFEAT_DOTPROD, .enc = 0x6f80e000 },
    { .opc = 83970, .es = 2, .form = A64F_3REG_ACC, .enc = 0x2e408400 },
    { .opc = 83971, .es = 2, .form = A64F_3REG_ACC, .enc = 0x6e408400 },
    { .opc = 83972, .es = 4, .form = A64F_3REG_ACC, .enc = 0x2e808400 },
    { .opc = 83973, .es = 4, .form = A64F_3REG_ACC, .enc = 0x6e808400 },
    { .opc = 86018, .es = 2, .form = A64F_3REG_ACC, .enc = 0x2e408c00 },
    { .opc = 86019, .es = 2, .form = A64F_3REG_ACC, .enc = 0x6e408c00 },
    { .opc = 86020, .es = 4, .form = A64F_3REG_ACC, .enc = 0x2e808c00 },
    { .opc = 86021, .es = 4, .form = A64F_3REG_ACC, .enc = 0x6e808c00 },
    { .opc = 88066, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x2f40d000 },
    { .opc = 88067, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x6f40d000 },
    { .opc = 88068, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x2f80d000 },
    { .opc = 88069, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x6f80d000 },
    { .opc = 90114, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x2f40f000 },
    { .opc = 90115, .es = 2, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x6f40f000 },
    { .opc = 90116, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x2f80f000 },
    { .opc = 90117, .es = 4, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x6f80f000 },
    { .opc = 92165, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0e609000 },
    { .opc = 92167, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0ea09000 },
    { .opc = 94213, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0e60b000 },
    { .opc = 94215, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_ACC, .enc = 0x0ea0b000 },
    { .opc = 96261, .es = 4, .oes = 2, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4e609000 },
    { .opc = 96263, .es = 8, .oes = 4, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4ea09000 },
    { .opc = 98309, .es = 4, .oes = 2, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4e60b000 },
    { .opc = 98311, .es = 8, .oes = 4, .kind = A64K_ACCW_HI, .form = A64F_3REG_ACC, .enc = 0x4ea0b000 },
    { .opc = 100357, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x0f403000 },
    { .opc = 100359, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f803000 },
    { .opc = 102405, .es = 4, .oes = 2, .kind = A64K_ACCW_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x0f407000 },
    { .opc = 102407, .es = 8, .oes = 4, .kind = A64K_ACCW_LO, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x0f807000 },
    { .opc = 104453, .es = 4, .oes = 2, .kind = A64K_ACCW_HI, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x4f403000 },
    { .opc = 104455, .es = 8, .oes = 4, .kind = A64K_ACCW_HI, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x4f803000 },
    { .opc = 106501, .es = 4, .oes = 2, .kind = A64K_ACCW_HI, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x32d5, .enc = 0x4f407000 },
    { .opc = 106503, .es = 8, .oes = 4, .kind = A64K_ACCW_HI, .form = A64F_3REG_IDX, .imm_sign = 2, .imm_spread = 0x196, .enc = 0x4f807000 },
    { .opc = -1 }
};
enum { A64OP1R_ADDV, A64OP1R_SMAXV, A64OP1R_UMAXV, A64OP1R_SMINV, A64OP1R_UMINV, A64OP1R_FMAXV, A64OP1R_FMINV, A64OP1R_FADDVD, A64OP1R_FMAXVD, A64OP1R_FMINVD, A64OP1R_ADDPD, A64OP1R_FADDPD, A64OP1R_FMAXNMV, A64OP1R_FMINNMV };
static const AVop1 a64_op1r_table[] = {
    { .opc = 0, .es = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x0e31b800 },
    { .opc = 1, .es = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x4e31b800 },
    { .opc = 2, .es = 2, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x0e71b800 },
    { .opc = 3, .es = 2, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x4e71b800 },
    { .opc = 5, .es = 4, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x4eb1b800 },
    { .opc = 0, .es = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x0e31b800 },
    { .opc = 1, .es = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x4e31b800 },
    { .opc = 2, .es = 2, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x0e71b800 },
    { .opc = 3, .es = 2, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x4e71b800 },
    { .opc = 5, .es = 4, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x4eb1b800 },
    { .opc = 2048, .es = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x0e30a800 },
    { .opc = 2049, .es = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x4e30a800 },
    { .opc = 2050, .es = 2, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x0e70a800 },
    { .opc = 2051, .es = 2, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x4e70a800 },
    { .opc = 2053, .es = 4, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x4eb0a800 },
    { .opc = 4096, .es = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x2e30a800 },
    { .opc = 4097, .es = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x6e30a800 },
    { .opc = 4098, .es = 2, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x2e70a800 },
    { .opc = 4099, .es = 2, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x6e70a800 },
    { .opc = 4101, .es = 4, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x6eb0a800 },
    { .opc = 6144, .es = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x0e31a800 },
    { .opc = 6145, .es = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x4e31a800 },
    { .opc = 6146, .es = 2, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x0e71a800 },
    { .opc = 6147, .es = 2, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x4e71a800 },
    { .opc = 6149, .es = 4, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x4eb1a800 },
    { .opc = 8192, .es = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x2e31a800 },
    { .opc = 8193, .es = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x6e31a800 },
    { .opc = 8194, .es = 2, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x2e71a800 },
    { .opc = 8195, .es = 2, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x6e71a800 },
    { .opc = 8197, .es = 4, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x6eb1a800 },
    { .opc = 10249, .es = 4, .fl = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x6e30f800 },
    { .opc = 12297, .es = 4, .fl = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x6eb0f800 },
    { .opc = 14344, .es = 4, .fl = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x7e30d800 },
    { .opc = 16392, .es = 4, .fl = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x7e30f800 },
    { .opc = 18440, .es = 4, .fl = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x7eb0f800 },
    { .opc = 20487, .es = 8, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x5ef1b800 },
    { .opc = 20487, .es = 8, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x5ef1b800 },
    { .opc = 22539, .es = 8, .fl = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x7e70d800 },
    { .opc = 24585, .es = 4, .fl = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x6e30c800 },
    { .opc = 24587, .es = 8, .fl = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x7e70c800 },
    { .opc = 26633, .es = 4, .fl = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x6eb0c800 },
    { .opc = 26635, .es = 8, .fl = 1, .kind = A64K_SCALAR, .form = A64F_2REG, .enc = 0x7ef0c800 },
    { .opc = -1 }
};
enum { A64SHUF_ZIP1, A64SHUF_ZIP2, A64SHUF_UZP1, A64SHUF_UZP2, A64SHUF_TRN1, A64SHUF_TRN2, A64SHUF_EXT };
static const AVop2 a64_shuf_table[] = {
    { .opc = 0, .es = 1, .form = A64F_3REG, .enc = 0x0e003800 },
    { .opc = 1, .es = 1, .form = A64F_3REG, .enc = 0x4e003800 },
    { .opc = 2, .es = 2, .form = A64F_3REG, .enc = 0x0e403800 },
    { .opc = 3, .es = 2, .form = A64F_3REG, .enc = 0x4e403800 },
    { .opc = 4, .es = 4, .form = A64F_3REG, .enc = 0x0e803800 },
    { .opc = 5, .es = 4, .form = A64F_3REG, .enc = 0x4e803800 },
    { .opc = 7, .es = 8, .form = A64F_3REG, .enc = 0x4ec03800 },
    { .opc = 0, .es = 1, .form = A64F_3REG, .enc = 0x0e003800 },
    { .opc = 1, .es = 1, .form = A64F_3REG, .enc = 0x4e003800 },
    { .opc = 2, .es = 2, .form = A64F_3REG, .enc = 0x0e403800 },
    { .opc = 3, .es = 2, .form = A64F_3REG, .enc = 0x4e403800 },
    { .opc = 4, .es = 4, .form = A64F_3REG, .enc = 0x0e803800 },
    { .opc = 5, .es = 4, .form = A64F_3REG, .enc = 0x4e803800 },
    { .opc = 7, .es = 8, .form = A64F_3REG, .enc = 0x4ec03800 },
    { .opc = 0, .es = 1, .form = A64F_3REG, .enc = 0x0e003800 },
    { .opc = 1, .es = 1, .form = A64F_3REG, .enc = 0x4e003800 },
    { .opc = 2, .es = 2, .form = A64F_3REG, .enc = 0x0e403800 },
    { .opc = 3, .es = 2, .form = A64F_3REG, .enc = 0x4e403800 },
    { .opc = 8, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0e803800 },
    { .opc = 9, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4e803800 },
    { .opc = 11, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4ec03800 },
    { .opc = 2048, .es = 1, .form = A64F_3REG, .enc = 0x0e007800 },
    { .opc = 2049, .es = 1, .form = A64F_3REG, .enc = 0x4e007800 },
    { .opc = 2050, .es = 2, .form = A64F_3REG, .enc = 0x0e407800 },
    { .opc = 2051, .es = 2, .form = A64F_3REG, .enc = 0x4e407800 },
    { .opc = 2052, .es = 4, .form = A64F_3REG, .enc = 0x0e807800 },
    { .opc = 2053, .es = 4, .form = A64F_3REG, .enc = 0x4e807800 },
    { .opc = 2055, .es = 8, .form = A64F_3REG, .enc = 0x4ec07800 },
    { .opc = 2048, .es = 1, .form = A64F_3REG, .enc = 0x0e007800 },
    { .opc = 2049, .es = 1, .form = A64F_3REG, .enc = 0x4e007800 },
    { .opc = 2050, .es = 2, .form = A64F_3REG, .enc = 0x0e407800 },
    { .opc = 2051, .es = 2, .form = A64F_3REG, .enc = 0x4e407800 },
    { .opc = 2052, .es = 4, .form = A64F_3REG, .enc = 0x0e807800 },
    { .opc = 2053, .es = 4, .form = A64F_3REG, .enc = 0x4e807800 },
    { .opc = 2055, .es = 8, .form = A64F_3REG, .enc = 0x4ec07800 },
    { .opc = 2048, .es = 1, .form = A64F_3REG, .enc = 0x0e007800 },
    { .opc = 2049, .es = 1, .form = A64F_3REG, .enc = 0x4e007800 },
    { .opc = 2050, .es = 2, .form = A64F_3REG, .enc = 0x0e407800 },
    { .opc = 2051, .es = 2, .form = A64F_3REG, .enc = 0x4e407800 },
    { .opc = 2056, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0e807800 },
    { .opc = 2057, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4e807800 },
    { .opc = 2059, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4ec07800 },
    { .opc = 4096, .es = 1, .form = A64F_3REG, .enc = 0x0e001800 },
    { .opc = 4097, .es = 1, .form = A64F_3REG, .enc = 0x4e001800 },
    { .opc = 4098, .es = 2, .form = A64F_3REG, .enc = 0x0e401800 },
    { .opc = 4099, .es = 2, .form = A64F_3REG, .enc = 0x4e401800 },
    { .opc = 4100, .es = 4, .form = A64F_3REG, .enc = 0x0e801800 },
    { .opc = 4101, .es = 4, .form = A64F_3REG, .enc = 0x4e801800 },
    { .opc = 4103, .es = 8, .form = A64F_3REG, .enc = 0x4ec01800 },
    { .opc = 4096, .es = 1, .form = A64F_3REG, .enc = 0x0e001800 },
    { .opc = 4097, .es = 1, .form = A64F_3REG, .enc = 0x4e001800 },
    { .opc = 4098, .es = 2, .form = A64F_3REG, .enc = 0x0e401800 },
    { .opc = 4099, .es = 2, .form = A64F_3REG, .enc = 0x4e401800 },
    { .opc = 4100, .es = 4, .form = A64F_3REG, .enc = 0x0e801800 },
    { .opc = 4101, .es = 4, .form = A64F_3REG, .enc = 0x4e801800 },
    { .opc = 4103, .es = 8, .form = A64F_3REG, .enc = 0x4ec01800 },
    { .opc = 4096, .es = 1, .form = A64F_3REG, .enc = 0x0e001800 },
    { .opc = 4097, .es = 1, .form = A64F_3REG, .enc = 0x4e001800 },
    { .opc = 4098, .es = 2, .form = A64F_3REG, .enc = 0x0e401800 },
    { .opc = 4099, .es = 2, .form = A64F_3REG, .enc = 0x4e401800 },
    { .opc = 4104, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0e801800 },
    { .opc = 4105, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4e801800 },
    { .opc = 4107, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4ec01800 },
    { .opc = 6144, .es = 1, .form = A64F_3REG, .enc = 0x0e005800 },
    { .opc = 6145, .es = 1, .form = A64F_3REG, .enc = 0x4e005800 },
    { .opc = 6146, .es = 2, .form = A64F_3REG, .enc = 0x0e405800 },
    { .opc = 6147, .es = 2, .form = A64F_3REG, .enc = 0x4e405800 },
    { .opc = 6148, .es = 4, .form = A64F_3REG, .enc = 0x0e805800 },
    { .opc = 6149, .es = 4, .form = A64F_3REG, .enc = 0x4e805800 },
    { .opc = 6151, .es = 8, .form = A64F_3REG, .enc = 0x4ec05800 },
    { .opc = 6144, .es = 1, .form = A64F_3REG, .enc = 0x0e005800 },
    { .opc = 6145, .es = 1, .form = A64F_3REG, .enc = 0x4e005800 },
    { .opc = 6146, .es = 2, .form = A64F_3REG, .enc = 0x0e405800 },
    { .opc = 6147, .es = 2, .form = A64F_3REG, .enc = 0x4e405800 },
    { .opc = 6148, .es = 4, .form = A64F_3REG, .enc = 0x0e805800 },
    { .opc = 6149, .es = 4, .form = A64F_3REG, .enc = 0x4e805800 },
    { .opc = 6151, .es = 8, .form = A64F_3REG, .enc = 0x4ec05800 },
    { .opc = 6144, .es = 1, .form = A64F_3REG, .enc = 0x0e005800 },
    { .opc = 6145, .es = 1, .form = A64F_3REG, .enc = 0x4e005800 },
    { .opc = 6146, .es = 2, .form = A64F_3REG, .enc = 0x0e405800 },
    { .opc = 6147, .es = 2, .form = A64F_3REG, .enc = 0x4e405800 },
    { .opc = 6152, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0e805800 },
    { .opc = 6153, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4e805800 },
    { .opc = 6155, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4ec05800 },
    { .opc = 8192, .es = 1, .form = A64F_3REG, .enc = 0x0e002800 },
    { .opc = 8193, .es = 1, .form = A64F_3REG, .enc = 0x4e002800 },
    { .opc = 8194, .es = 2, .form = A64F_3REG, .enc = 0x0e402800 },
    { .opc = 8195, .es = 2, .form = A64F_3REG, .enc = 0x4e402800 },
    { .opc = 8196, .es = 4, .form = A64F_3REG, .enc = 0x0e802800 },
    { .opc = 8197, .es = 4, .form = A64F_3REG, .enc = 0x4e802800 },
    { .opc = 8199, .es = 8, .form = A64F_3REG, .enc = 0x4ec02800 },
    { .opc = 8192, .es = 1, .form = A64F_3REG, .enc = 0x0e002800 },
    { .opc = 8193, .es = 1, .form = A64F_3REG, .enc = 0x4e002800 },
    { .opc = 8194, .es = 2, .form = A64F_3REG, .enc = 0x0e402800 },
    { .opc = 8195, .es = 2, .form = A64F_3REG, .enc = 0x4e402800 },
    { .opc = 8196, .es = 4, .form = A64F_3REG, .enc = 0x0e802800 },
    { .opc = 8197, .es = 4, .form = A64F_3REG, .enc = 0x4e802800 },
    { .opc = 8199, .es = 8, .form = A64F_3REG, .enc = 0x4ec02800 },
    { .opc = 8192, .es = 1, .form = A64F_3REG, .enc = 0x0e002800 },
    { .opc = 8193, .es = 1, .form = A64F_3REG, .enc = 0x4e002800 },
    { .opc = 8194, .es = 2, .form = A64F_3REG, .enc = 0x0e402800 },
    { .opc = 8195, .es = 2, .form = A64F_3REG, .enc = 0x4e402800 },
    { .opc = 8200, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0e802800 },
    { .opc = 8201, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4e802800 },
    { .opc = 8203, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4ec02800 },
    { .opc = 10240, .es = 1, .form = A64F_3REG, .enc = 0x0e006800 },
    { .opc = 10241, .es = 1, .form = A64F_3REG, .enc = 0x4e006800 },
    { .opc = 10242, .es = 2, .form = A64F_3REG, .enc = 0x0e406800 },
    { .opc = 10243, .es = 2, .form = A64F_3REG, .enc = 0x4e406800 },
    { .opc = 10244, .es = 4, .form = A64F_3REG, .enc = 0x0e806800 },
    { .opc = 10245, .es = 4, .form = A64F_3REG, .enc = 0x4e806800 },
    { .opc = 10247, .es = 8, .form = A64F_3REG, .enc = 0x4ec06800 },
    { .opc = 10240, .es = 1, .form = A64F_3REG, .enc = 0x0e006800 },
    { .opc = 10241, .es = 1, .form = A64F_3REG, .enc = 0x4e006800 },
    { .opc = 10242, .es = 2, .form = A64F_3REG, .enc = 0x0e406800 },
    { .opc = 10243, .es = 2, .form = A64F_3REG, .enc = 0x4e406800 },
    { .opc = 10244, .es = 4, .form = A64F_3REG, .enc = 0x0e806800 },
    { .opc = 10245, .es = 4, .form = A64F_3REG, .enc = 0x4e806800 },
    { .opc = 10247, .es = 8, .form = A64F_3REG, .enc = 0x4ec06800 },
    { .opc = 10240, .es = 1, .form = A64F_3REG, .enc = 0x0e006800 },
    { .opc = 10241, .es = 1, .form = A64F_3REG, .enc = 0x4e006800 },
    { .opc = 10242, .es = 2, .form = A64F_3REG, .enc = 0x0e406800 },
    { .opc = 10243, .es = 2, .form = A64F_3REG, .enc = 0x4e406800 },
    { .opc = 10248, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x0e806800 },
    { .opc = 10249, .es = 4, .fl = 1, .form = A64F_3REG, .enc = 0x4e806800 },
    { .opc = 10251, .es = 8, .fl = 1, .form = A64F_3REG, .enc = 0x4ec06800 },
    { .opc = 12288, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 11, .imm_sign = 1, .enc = 0x2e000000 },
    { .opc = 12289, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 11, .imm_sign = 1, .enc = 0x6e000000 },
    { .opc = 12290, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 11, .imm_sign = 1, .enc = 0x2e000000 },
    { .opc = 12291, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 11, .imm_sign = 1, .enc = 0x6e000000 },
    { .opc = 12292, .es = 4, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 11, .imm_sign = 1, .enc = 0x2e000000 },
    { .opc = 12293, .es = 4, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 11, .imm_sign = 1, .enc = 0x6e000000 },
    { .opc = 12294, .es = 8, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 11, .imm_sign = 1, .enc = 0x2e000000 },
    { .opc = 12295, .es = 8, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 11, .imm_sign = 1, .enc = 0x6e000000 },
    { .opc = 12288, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 11, .imm_sign = 1, .enc = 0x2e000000 },
    { .opc = 12289, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 11, .imm_sign = 1, .enc = 0x6e000000 },
    { .opc = 12290, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 11, .imm_sign = 1, .enc = 0x2e000000 },
    { .opc = 12291, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 11, .imm_sign = 1, .enc = 0x6e000000 },
    { .opc = 12292, .es = 4, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 11, .imm_sign = 1, .enc = 0x2e000000 },
    { .opc = 12293, .es = 4, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 11, .imm_sign = 1, .enc = 0x6e000000 },
    { .opc = 12294, .es = 8, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 11, .imm_sign = 1, .enc = 0x2e000000 },
    { .opc = 12295, .es = 8, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 11, .imm_sign = 1, .enc = 0x6e000000 },
    { .opc = 12288, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 11, .imm_sign = 1, .enc = 0x2e000000 },
    { .opc = 12289, .es = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 11, .imm_sign = 1, .enc = 0x6e000000 },
    { .opc = 12290, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 11, .imm_sign = 1, .enc = 0x2e000000 },
    { .opc = 12291, .es = 2, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 11, .imm_sign = 1, .enc = 0x6e000000 },
    { .opc = 12296, .es = 4, .fl = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 11, .imm_sign = 1, .enc = 0x2e000000 },
    { .opc = 12297, .es = 4, .fl = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 11, .imm_sign = 1, .enc = 0x6e000000 },
    { .opc = 12298, .es = 8, .fl = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 7, .imm_pos = 11, .imm_sign = 1, .enc = 0x2e000000 },
    { .opc = 12299, .es = 8, .fl = 1, .form = A64F_3REG_IMM, .imm_lo = 0, .imm_hi = 15, .imm_pos = 11, .imm_sign = 1, .enc = 0x6e000000 },
    { .opc = -1 }
};
/* NEON_TABLES_END */

/* lane shape of a vector type: element size in bytes, float flag,
   lane count */
static void a64_vec_shape(CType *t, int *es, int *fl, int *n)
{
    int bt = t->ref->type.t & VT_BTYPE;
    *fl = (bt == VT_FLOAT || bt == VT_DOUBLE);
    *es = (*fl) ? (bt == VT_FLOAT ? 4 : 8)
                : (bt == VT_BYTE || bt == VT_BOOL) ? 1
                : bt == VT_SHORT ? 2
                : bt == VT_INT ? 4 : 8;
    *n = (int)t->ref->c;
}

static int a64_shape_id(int es, int fl, int n)
{
    if (fl)
        return es == 4 ? (n == 2 ? A64SH_F32X2 : A64SH_F32X4)
                       : (n == 1 ? A64SH_F64X1 : A64SH_F64X2);
    switch (es) {
    case 1: return n == 8 ? A64SH_I8X8 : A64SH_I8X16;
    case 2: return n == 4 ? A64SH_I16X4 : A64SH_I16X8;
    case 4: return n == 2 ? A64SH_I32X2 : A64SH_I32X4;
    default: return n == 1 ? A64SH_I64X1 : A64SH_I64X2;
    }
}

typedef struct { int es, fl, tot; } A64Shape;

static void a64_shape_of(CType *t, A64Shape *s)
{
    int n;
    a64_vec_shape(t, &s->es, &s->fl, &n);
    s->tot = s->es * n;
}

static void a64_shape_decode(int id, A64Shape *s)
{
    static const signed char tab[12][3] = {
        {1, 0, 8}, {1, 0, 16}, {2, 0, 8}, {2, 0, 16},
        {4, 0, 8}, {4, 0, 16}, {8, 0, 8}, {8, 0, 16},
        {4, 1, 8}, {4, 1, 16}, {8, 1, 8}, {8, 1, 16}
    };
    s->es = tab[id][0];
    s->fl = tab[id][1];
    s->tot = tab[id][2];
}

static void a64_vec_check(CType *t, const char *builtin)
{
    if (!(t->t & VT_VECTOR))
        tcc_error("%s on a non-vector value", builtin);
}

/* the required-feature check of the opcode tables: naming the
   missing feature and the level the target actually has */
static void a64_check_feature(const char *builtin, int opc, unsigned feat)
{
    unsigned missing = feat & ~tcc_effective_features(tcc_state);
    if (missing) {
        unsigned bit = 1;
        char buf[96];
        while (!(missing & bit))
            bit <<= 1;
        tcc_describe_features(tcc_state, buf, sizeof(buf));
        tcc_error("%s opcode %d requires %s, target features: %s",
                  builtin, opc, tcc_feature_name(bit), buf);
    }
}

static const AVop *a64_find(const AVop *tab, int opc)
{
    int key = opc & ~(127 << 4);
    const AVop *e;
    for (e = tab; e->opc >= 0; e++)
        if (e->opc == key)
            return e;
    return NULL;
}

static uint32_t a64_imm_bits(const AVop *e, int imm)
{
    if (e->imm_sign == 2) {
        uint32_t b = 0, sp = e->imm_spread;
        int i = 0;
        while (sp) {
            int pos = (int)(sp & 31) - 1;
            if ((imm >> i) & 1)
                b |= 1u << pos;
            sp >>= 5;
            i++;
        }
        return b;
    }
    if (e->imm_pos)
        return (uint32_t)(e->imm_base + e->imm_sign * imm) << e->imm_pos;
    return 0;
}

static void a64_check_imm(const AVop *e, const char *bn, int opc, int imm)
{
    if (e->imm_pos || e->imm_sign == 2) {
        if (imm < e->imm_lo || imm > e->imm_hi)
            tcc_error("%s opcode %d immediate %d out of range",
                      bn, opc, imm);
    } else if (imm != 0)
        tcc_error("%s opcode %d takes no immediate", bn, opc);
}

/* the result shape an entry produces from its first operand */
static void a64_result_shape(const AVop *e, const A64Shape *a,
                             A64Shape *r)
{
    *r = *a;
    switch (e->kind) {
    case A64K_CVTN:
        r->es = e->oes ? e->oes : a->es;
        r->fl = e->ofl;
        r->tot = a->tot / a->es * r->es;
        break;
    case A64K_NARROW: case A64K_NARROW2:
        r->es = a->es / 2;
        r->tot = 8;
        break;
    case A64K_WIDEN_LO: case A64K_WIDEN_HI:
        r->es = a->es * 2;
        r->tot = 16;
        break;
    case A64K_PAIRW:
        r->es = a->es * 2;
        break;
    case A64K_JOIN: case A64K_NARROW_HI: case A64K_NARROW2_HI:
        r->tot = 16;
        break;
    case A64K_HALF_LO: case A64K_HALF_HI:
        r->tot = 8;
        break;
    case A64K_SCALAR:
        r->es = e->oes ? e->oes : a->es;
        r->fl = e->ofl ? 1 : a->fl;
        r->tot = 0;
        break;
    }
}

/* the shape an entry expects of operand `which` (1 or 2) */
static void a64_operand_shape(const AVop *e, const A64Shape *a,
                              int which, A64Shape *o)
{
    (void)which;
    *o = *a;
    switch (e->kind) {
    case A64K_BYTES:
        o->es = 1;
        o->fl = 0;
        break;
    case A64K_ACCW_LO:
        o->es = a->es / 2;
        o->tot = 8;
        break;
    case A64K_ACCW_HI:
        o->es = a->es / 2;
        break;
    case A64K_DOT:
        o->es = e->oes ? e->oes : a->es;
        o->fl = 0;
        break;
    case A64K_NARROW_HI: case A64K_NARROW2_HI:
        o->es = a->es * 2;
        o->tot = 16;
        break;
    case A64K_ACCW1:
        o->es = a->es / 2;
        break;
    case A64K_ADDW_LO:
        o->es = a->es / 2;
        o->tot = 8;
        break;
    case A64K_ADDW_HI:
        o->es = a->es / 2;
        break;
    }
}

/* validate the nops value operands (operand 0 at
   vtop[-(nops-1)]) and the result type against a table entry */
static void a64_validate(const AVop *e, const char *bn, int opc,
                         int nops, CType *restype)
{
    A64Shape s[3], want, r, rs;
    int i;
    for (i = 0; i < nops; i++) {
        a64_vec_check(&vtop[i - (nops - 1)].type, bn);
        a64_shape_of(&vtop[i - (nops - 1)].type, &s[i]);
    }
    if (e->kind == A64K_TBLQ) {
        /* operand totals: the table is always 16 bytes; the
           index and the default operand match the result */
        A64Shape os;
        int rt;
        a64_shape_decode(opc & 15, &os);
        rt = os.tot;
        if (nops == 2) {
            if (s[0].tot != 16 || s[1].tot != rt)
                tcc_error("%s opcode %d operand shapes differ", bn, opc);
        } else {
            if (s[0].tot != rt || s[1].tot != 16 || s[2].tot != rt)
                tcc_error("%s opcode %d operand shapes differ", bn, opc);
        }
    } else if (e->kind == A64K_BYTES) {
        A64Shape os;
        a64_shape_decode(opc & 15, &os);
        if (s[0].tot != os.tot)
            tcc_error("%s opcode %d does not apply to this vector shape",
                      bn, opc);
    } else if (s[0].es != e->es || s[0].fl != e->fl ||
        a64_shape_id(s[0].es, s[0].fl, s[0].tot / s[0].es) != (opc & 15))
        tcc_error("%s opcode %d does not apply to this vector shape",
                  bn, opc);
    for (i = 1; i < nops; i++) {
        if (e->kind == A64K_TBLQ)
            continue; /* totals checked above */
        a64_operand_shape(e, &s[0], i, &want);
        /* the lane source of an indexed form may be a full or a
           half register regardless of the result width */
        if ((s[i].tot != want.tot &&
             !(e->form == A64F_3REG_IDX && i == nops - 1)) ||
            (e->kind != A64K_BYTES &&
             (s[i].es != want.es || s[i].fl != want.fl)))
            tcc_error("%s opcode %d operand shapes differ", bn, opc);
    }
    if (e->kind == A64K_BYTES || e->kind == A64K_TBLQ)
        a64_shape_decode(opc & 15, &r);
    else
        a64_result_shape(e, &s[0], &r);
    if (e->kind == A64K_SCALAR) {
        int sz, al, bt;
        if (restype->t & VT_VECTOR)
            tcc_error("%s result type is not a scalar type", bn);
        sz = type_size(restype, &al);
        bt = restype->t & VT_BTYPE;
        if (sz != r.es ||
            ((bt == VT_FLOAT || bt == VT_DOUBLE) != r.fl))
            tcc_error("%s opcode %d result shape mismatch", bn, opc);
    } else {
        if (!(restype->t & VT_VECTOR))
            tcc_error("%s result type is not a vector type", bn);
        a64_shape_of(restype, &rs);
        if (rs.es != r.es || rs.tot != r.tot || rs.fl != r.fl)
            tcc_error("%s opcode %d result shape mismatch", bn, opc);
    }
}

static void a64_vec_mov(int rd, int rn, int tot)
{
    if (rd == rn)
        return;
    if (tot == 16)
        o(ARM64_MOV_V16B | fltr(rd) | fltr(rn) * 0x10020);
    else
        o(ARM64_FMOV_SCALAR | fltr(rd) | fltr(rn) << 5);
}

/* spill a value-stack entry held in a vector register to a fresh
   stack temporary of its full vector size; save_reg_upstack
   cannot be used for vectors (it sizes the slot by the element
   type, not by the vector) */
static void a64_spill_vec(SValue *p)
{
    int r, r2, l, size, align;
    SValue sv;
    if ((p->r & VT_VALMASK) >= VT_CONST || (p->r & VT_LVAL) ||
        !IS_FREG(p->r & VT_VALMASK))
        return;
    r = p->r & VT_VALMASK;
    size = type_size(&p->type, &align);
    l = get_temp_local_var(size, align, &r2);
    sv.type = p->type;
    sv.r = VT_LOCAL | VT_LVAL;
    sv.c.i = l;
    sv.sym = NULL;
    store(r, &sv);
    p->r = VT_LVAL | VT_LOCAL;
    p->sym = NULL;
    p->r2 = r2;
    p->c.i = l;
}

/* apply a unary opcode-table operation to the value on top of
   the stack, in place (used for vector negation, complement and
   shift-count negation) */
static void a64_apply_op1(int opidx)
{
    A64Shape a;
    const AVop *e;
    int rn, rd;
    a64_shape_of(&vtop[0].type, &a);
    e = a64_find(a64_op1_table,
                 (opidx << 11) | a64_shape_id(a.es, a.fl, a.tot / a.es));
    if (!e)
        tcc_error("unary vector operation is not supported for this shape");
    gv(RC_VEC);
    rn = vtop[0].r;
    rd = get_reg(RC_VEC);
    o(e->enc | fltr(rn) << 5 | fltr(rd));
    vtop[0].r = rd;
}

/* ------------------------------------------------------------------ */

ST_FUNC void gen_vec_gv(void)
{
    gv(RC_VEC);
}

ST_FUNC void gen_vec_ret(void)
{
    /* a vector returns in v0, the FP/SIMD return register */
    gv(RC_FRET);
}

ST_FUNC void gen_vec_neg(void)
{
    int bt = vtop[0].type.ref->type.t & VT_BTYPE;
    a64_apply_op1(bt == VT_FLOAT || bt == VT_DOUBLE
                  ? A64OP1_FNEG : A64OP1_NEG);
}

ST_FUNC void gen_vec_not(void)
{
    a64_apply_op1(A64OP1_NOT);
}

ST_FUNC void gen_vec_home(void)
{
    /* move the rvalue into a stack temporary so the subscript
       code can address its elements in memory */
    gv(RC_VEC);
    a64_spill_vec(vtop);
}

ST_FUNC void gen_vec_op1(int opc, CType *restype)
{
    const AVop *e = a64_find(a64_op1_table, opc);
    int imm = (opc >> 4) & 127, rn, rd;
    if (!e)
        tcc_error("__builtin_aarch64_vop1 opcode %d is not implemented", opc);
    a64_check_feature("__builtin_aarch64_vop1", opc, e->feat);
    a64_validate(e, "__builtin_aarch64_vop1", opc, 1, restype);
    a64_check_imm(e, "__builtin_aarch64_vop1", opc, imm);
    gv(RC_VEC);
    rn = vtop[0].r;
    if (e->form == A64F_CVT64I) {
        /* scvtf/ucvtf scalar form: the source is a GP register */
        int tmp = get_reg(RC_INT);
        o(ARM64_FMOV_XD | intr(tmp) | fltr(rn) << 5);
        rd = get_reg(RC_VEC);
        o(e->enc | a64_imm_bits(e, imm) | intr(tmp) << 5 | fltr(rd));
    } else if (e->form == A64F_CVT64F) {
        /* fcvt* scalar form: the result is a GP register */
        int tmp = get_reg(RC_INT);
        o(e->enc | a64_imm_bits(e, imm) | fltr(rn) << 5 | intr(tmp));
        rd = get_reg(RC_VEC);
        o(0x9E670000U | fltr(rd) | intr(tmp) << 5); /* fmov d, x */
    } else {
        rd = get_reg(RC_VEC);
        o(e->enc | a64_imm_bits(e, imm) | fltr(rn) << 5 | fltr(rd));
    }
    vtop[0].r = rd;
    vtop[0].type = *restype;
}

ST_FUNC void gen_vec_op1r(int opc, CType *restype)
{
    const AVop *e = a64_find(a64_op1r_table, opc);
    int rn, rd;
    A64Shape a, r;
    if (!e)
        tcc_error("__builtin_aarch64_vop1r opcode %d is not implemented", opc);
    a64_check_feature("__builtin_aarch64_vop1r", opc, e->feat);
    a64_validate(e, "__builtin_aarch64_vop1r", opc, 1, restype);
    a64_shape_of(&vtop[0].type, &a);
    a64_result_shape(e, &a, &r);
    gv(RC_VEC);
    rn = vtop[0].r;
    if (r.fl) {
        rd = get_reg(RC_FLOAT);
        o(e->enc | fltr(rn) << 5 | fltr(rd));
    } else if (r.es == 8) {
        int vr = get_reg(RC_VEC);
        o(e->enc | fltr(rn) << 5 | fltr(vr));
        rd = get_reg(RC_INT);
        o(ARM64_FMOV_XD | intr(rd) | fltr(vr) << 5);
    } else {
        int vr = get_reg(RC_VEC);
        int imm5 = r.es; /* lane 0: (0 << (tz + 1)) | es */
        int uns = (restype->t & VT_UNSIGNED) != 0;
        o(e->enc | fltr(rn) << 5 | fltr(vr));
        rd = get_reg(RC_INT);
        o((uns ? 0x0e003c00u : 0x0e002c00u) |
          (uint32_t)(!uns && r.es == 4) << 30 | (uint32_t)imm5 << 16 |
          fltr(vr) << 5 | intr(rd));
    }
    vtop[0].r = rd;
    vtop[0].type = *restype;
}

ST_FUNC void gen_vec_op2(int opc, CType *restype)
{
    const AVop *e = a64_find(a64_op2_table, opc);
    int imm = (opc >> 4) & 127, ra, rb, rd;
    if (!e)
        tcc_error("__builtin_aarch64_vop2 opcode %d is not implemented", opc);
    a64_check_feature("__builtin_aarch64_vop2", opc, e->feat);
    a64_validate(e, "__builtin_aarch64_vop2", opc, 2, restype);
    if (e->form == A64F_3REG_IDX) {
        A64Shape b;
        a64_shape_of(&vtop[0].type, &b);
        if (imm >= b.tot / b.es)
            tcc_error("__builtin_aarch64_vop2 opcode %d lane %d out of range",
                      opc, imm);
    } else
        a64_check_imm(e, "__builtin_aarch64_vop2", opc, imm);
    gv2(RC_VEC, RC_VEC);
    ra = vtop[-1].r;
    rb = vtop[0].r;
    switch (e->form) {
    case A64F_2REG_ACC:
        o(e->enc | fltr(rb) << 5 | fltr(ra));
        rd = ra;
        break;
    case A64F_2REG_IMM_ACC:
        o(e->enc | a64_imm_bits(e, imm) | fltr(rb) << 5 | fltr(ra));
        rd = ra;
        break;
    case A64F_JOIN:
        rd = get_reg(RC_VEC);
        if (rd != ra)
            a64_vec_mov(rd, ra, 8);
        /* ins rd.d[1], rb.d[0] */
        o(0x6e000400 | 24 << 16 | fltr(rb) << 5 | fltr(rd));
        break;
    default:
        rd = get_reg(RC_VEC);
        o(e->enc | a64_imm_bits(e, imm) | fltr(rb) << 16 |
          fltr(ra) << 5 | fltr(rd));
        break;
    }
    vtop--;
    vtop[0].r = rd;
    vtop[0].type = *restype;
}

ST_FUNC void gen_vec_op2i(int opc, int imm, CType *restype)
{
    const AVop *e = a64_find(a64_shuf_table, opc);
    int ra, rb, rd;
    if (!e)
        tcc_error("__builtin_aarch64_shuffle opcode %d is not implemented", opc);
    a64_check_feature("__builtin_aarch64_shuffle", opc, e->feat);
    a64_validate(e, "__builtin_aarch64_shuffle", opc, 2, restype);
    a64_check_imm(e, "__builtin_aarch64_shuffle", opc, imm);
    gv2(RC_VEC, RC_VEC);
    ra = vtop[-1].r;
    rb = vtop[0].r;
    rd = get_reg(RC_VEC);
    o(e->enc | a64_imm_bits(e, imm) | fltr(rb) << 16 |
      fltr(ra) << 5 | fltr(rd));
    vtop--;
    vtop[0].r = rd;
    vtop[0].type = *restype;
}

ST_FUNC void gen_vec_op3(int opc, CType *restype)
{
    const AVop *e = a64_find(a64_op3_table, opc);
    int imm = (opc >> 4) & 127, ra, rb, rc;
    if (!e)
        tcc_error("__builtin_aarch64_vop3 opcode %d is not implemented", opc);
    a64_check_feature("__builtin_aarch64_vop3", opc, e->feat);
    if (e->form == A64F_3REG_IMM)
        vtop--; /* the dummy third operand of the immediate forms */
    a64_validate(e, "__builtin_aarch64_vop3", opc,
                 e->form == A64F_3REG_IMM ? 2 : 3, restype);
    if (e->form == A64F_3REG_IDX) {
        A64Shape c;
        int unit = e->kind == A64K_DOT ? e->es : 0;
        a64_shape_of(&vtop[0].type, &c);
        if (!unit)
            unit = c.es;
        if (imm >= c.tot / unit)
            tcc_error("__builtin_aarch64_vop3 opcode %d lane %d out of range",
                      opc, imm);
    } else
        a64_check_imm(e, "__builtin_aarch64_vop3", opc, imm);
    if (e->form == A64F_3REG_IMM) {
        /* the dummy third operand was dropped above; accumulate
           a over b */
        gv2(RC_VEC, RC_VEC);
        ra = vtop[-1].r;
        rb = vtop[0].r;
        o(e->enc | a64_imm_bits(e, imm) | fltr(rb) << 5 | fltr(ra));
        vtop--;
        vtop[0].r = ra;
        vtop[0].type = *restype;
        return;
    }
    gv(RC_VEC);
    rc = vtop[0].r;
    vswap();
    gv(RC_VEC);
    rb = vtop[0].r;
    vswap();
    vrott(3);
    vrott(3); /* a is on top now, b and c below it in registers */
    gv(RC_VEC);
    ra = vtop[0].r;
    if ((opc >> 11) == A64OP3_TBX && type_size(restype, &(int){0}) == 8) {
        /* the 64-bit table form: the instruction tests indices
           against the full 128-bit table register, but the ACLE
           type has an 8-byte table, so indices 8..15 must keep
           the default operand: look up with TBL and blend */
        const AVop *cmhs = a64_find(a64_op2_table,
                                    (A64OP2_CMHS << 11) | (opc & 15));
        const AVop *tbl = a64_find(a64_op2_table,
                                   (A64OP2_TBL << 11) | (opc & 15));
        const AVop *bsl = a64_find(a64_op3_table,
                                   (A64OP3_BSL << 11) | (opc & 15));
        int gr = get_reg(RC_INT);
        int t1 = get_reg(RC_VEC), t2, t3;
        /* park each scratch register on the value stack so the
           next get_reg cannot hand it out again */
        vpushi(0); vtop[0].r = t1; vtop[0].type = *restype;
        t2 = get_reg(RC_VEC);
        vpushi(0); vtop[0].r = t2; vtop[0].type = *restype;
        t3 = get_reg(RC_VEC);
        vpushi(0); vtop[0].r = t3; vtop[0].type = *restype;
        o(0x52800100u | intr(gr)); /* movz wgr, #8 */
        o(0x0e010c00u | intr(gr) << 5 | fltr(t1)); /* dup t1.8b, wgr */
        o(cmhs->enc | fltr(t1) << 16 | fltr(rc) << 5 | fltr(t2));
        o(tbl->enc | fltr(rc) << 16 | fltr(rb) << 5 | fltr(t3));
        o(bsl->enc | fltr(t3) << 16 | fltr(ra) << 5 | fltr(t2));
        vtop -= 5; /* the three parks and two of the operands */
        vtop[0].r = t2;
        vtop[0].type = *restype;
        return;
    }
    if (e->form == A64F_3REG_FMA64)
        o(e->enc | fltr(rc) << 16 | fltr(ra) << 10 |
          fltr(rb) << 5 | fltr(ra));
    else
        o(e->enc | a64_imm_bits(e, imm) | fltr(rc) << 16 |
          fltr(rb) << 5 | fltr(ra));
    vtop -= 2;
    vtop[0].r = ra;
    vtop[0].type = *restype;
}

/* ------------------------------------------------------------------ */
/* the C operators on vectors, reached from gen_op_vector: the
   left operand is at vtop[-1], the right at vtop[0] */

static void a64_opv_emit(int opidx, int swap, int notafter)
{
    A64Shape a;
    const AVop *e;
    int rm, rn, rd;
    a64_shape_of(&vtop[-1].type, &a);
    e = a64_find(a64_op2_table,
                 (opidx << 11) | a64_shape_id(a.es, a.fl, a.tot / a.es));
    if (!e)
        tcc_error("vector operation is not supported for this shape");
    gv2(RC_VEC, RC_VEC);
    rn = vtop[-1].r;
    rm = vtop[0].r;
    if (swap) {
        int t = rm;
        rm = rn;
        rn = t;
    }
    rd = get_reg(RC_VEC);
    o(e->enc | fltr(rm) << 16 | fltr(rn) << 5 | fltr(rd));
    vtop--;
    vtop[0].r = rd;
    if (notafter)
        a64_apply_op1(A64OP1_NOT);
}

ST_FUNC void gen_opv(int op)
{
    CType *lt = &vtop[-1].type, *rt = &vtop[0].type;
    int bt = lt->ref->type.t & VT_BTYPE;
    int fl = bt == VT_FLOAT || bt == VT_DOUBLE;
    if (op == TOK_SHL || op == TOK_SAR || op == TOK_SHR) {
        int opidx, negcnt = 0;
        if (op == TOK_SHL)
            opidx = (lt->t & VT_UNSIGNED) ? A64OP2_USHL : A64OP2_SSHL;
        else {
            opidx = op == TOK_SAR ? A64OP2_SSHL : A64OP2_USHL;
            negcnt = 1;
        }
        if (!(rt->t & VT_VECTOR)) {
            /* a scalar count is splatted across the left
               operand's shape first */
            A64Shape a;
            int gr, rd;
            a64_shape_of(lt, &a);
            gv(RC_INT);
            gr = vtop[0].r;
            rd = get_reg(RC_VEC);
            o(0x0e000c00 | (uint32_t)(a.tot == 16) << 30 |
              (uint32_t)a.es << 16 | intr(gr) << 5 | fltr(rd));
            vtop[0].r = rd;
            vtop[0].type = *lt;
        }
        if (negcnt)
            /* the count is on top: negate it in place, turning
               the left shift into a right shift */
            a64_apply_op1(A64OP1_NEG);
        a64_opv_emit(opidx, 0, 0);
        return;
    }
    if (fl) {
        switch (op) {
        case '+': a64_opv_emit(A64OP2_FADD, 0, 0); break;
        case '-': a64_opv_emit(A64OP2_FSUB, 0, 0); break;
        case '*': a64_opv_emit(A64OP2_FMUL, 0, 0); break;
        case '/': a64_opv_emit(A64OP2_FDIV, 0, 0); break;
        case TOK_EQ: a64_opv_emit(A64OP2_FCMEQ, 0, 0); break;
        case TOK_NE: a64_opv_emit(A64OP2_FCMEQ, 0, 1); break;
        case TOK_LT: a64_opv_emit(A64OP2_FCMGT, 1, 0); break;
        case TOK_LE: a64_opv_emit(A64OP2_FCMGE, 1, 0); break;
        case TOK_GT: a64_opv_emit(A64OP2_FCMGT, 0, 0); break;
        case TOK_GE: a64_opv_emit(A64OP2_FCMGE, 0, 0); break;
        default:
            tcc_error("vector operation is not supported for this target");
        }
        return;
    }
    switch (op) {
    case '+': a64_opv_emit(A64OP2_ADD, 0, 0); break;
    case '-': a64_opv_emit(A64OP2_SUB, 0, 0); break;
    case '*': a64_opv_emit(A64OP2_MUL, 0, 0); break;
    case '&': a64_opv_emit(A64OP2_AND, 0, 0); break;
    case '|': a64_opv_emit(A64OP2_ORR, 0, 0); break;
    case '^': a64_opv_emit(A64OP2_EOR, 0, 0); break;
    case '/': case TOK_UDIV:
        tcc_error("vector integer division has no NEON instruction");
        break;
    case '%': case TOK_UMOD:
        tcc_error("vector integer remainder has no NEON instruction");
        break;
    case TOK_EQ: a64_opv_emit(A64OP2_CMEQ, 0, 0); break;
    case TOK_NE: a64_opv_emit(A64OP2_CMEQ, 0, 1); break;
    case TOK_LT: a64_opv_emit(A64OP2_CMGT, 1, 0); break;
    case TOK_LE: a64_opv_emit(A64OP2_CMGE, 1, 0); break;
    case TOK_GT: a64_opv_emit(A64OP2_CMGT, 0, 0); break;
    case TOK_GE: a64_opv_emit(A64OP2_CMGE, 0, 0); break;
    case TOK_ULT: a64_opv_emit(A64OP2_CMHI, 1, 0); break;
    case TOK_ULE: a64_opv_emit(A64OP2_CMHS, 1, 0); break;
    case TOK_UGT: a64_opv_emit(A64OP2_CMHI, 0, 0); break;
    case TOK_UGE: a64_opv_emit(A64OP2_CMHS, 0, 0); break;
    default:
        tcc_error("vector operation is not supported for this target");
    }
}

/* ------------------------------------------------------------------ */
/* splat, lane extract and lane replace */

ST_FUNC void gen_vec_splat(int opc, CType *restype)
{
    A64Shape a, rs;
    int rd;
    a64_shape_decode(opc & 15, &a);
    if (!(restype->t & VT_VECTOR))
        tcc_error("__builtin_aarch64_splat result type is not a vector type");
    a64_shape_of(restype, &rs);
    if (rs.es != a.es || rs.fl != a.fl || rs.tot != a.tot)
        tcc_error("__builtin_aarch64_splat opcode %d result shape mismatch", opc);
    if (a.fl) {
        int rn;
        if (!((vtop[0].type.t & VT_BTYPE) == VT_FLOAT ||
              (vtop[0].type.t & VT_BTYPE) == VT_DOUBLE))
            gen_cast(&restype->ref->type);
        gv(RC_FLOAT);
        rn = vtop[0].r;
        if (a.tot == a.es) {
            /* a one-lane result (float64x1_t) is the scalar itself:
               no dup encoding exists for it */
            vtop[0].r = rn;
            vtop[0].type = *restype;
            return;
        }
        rd = get_reg(RC_VEC);
        /* dup rd.T, rn.T[0] */
        o(0x0e000400 | (uint32_t)(a.tot == 16) << 30 |
          (uint32_t)a.es << 16 | fltr(rn) << 5 | fltr(rd));
    } else {
        int gr;
        if ((vtop[0].type.t & VT_BTYPE) == VT_FLOAT ||
            (vtop[0].type.t & VT_BTYPE) == VT_DOUBLE)
            gen_cast(&restype->ref->type);
        gv(RC_INT);
        gr = vtop[0].r;
        rd = get_reg(RC_VEC);
        if (a.tot == a.es)
            /* a one-lane result (int64x1_t): fmov rd, gr */
            o(0x9e670000 | intr(gr) << 5 | fltr(rd));
        else
            /* dup rd.T, gr */
            o(0x0e000c00 | (uint32_t)(a.tot == 16) << 30 |
              (uint32_t)a.es << 16 | intr(gr) << 5 | fltr(rd));
    }
    vtop[0].r = rd;
    vtop[0].type = *restype;
}

ST_FUNC void gen_vec_extract(int opc, int lane, CType *restype)
{
    A64Shape a;
    int rn, rd, tz;
    a64_shape_decode(opc & 15, &a);
    if (lane < 0 || lane >= a.tot / a.es)
        tcc_error("__builtin_aarch64_extract lane %d out of range", lane);
    if (restype->t & VT_VECTOR)
        tcc_error("__builtin_aarch64_extract result type is not a scalar type");
    gv(RC_VEC);
    rn = vtop[0].r;
    tz = a.es == 1 ? 0 : a.es == 2 ? 1 : a.es == 4 ? 2 : 3;
    if (a.fl) {
        rd = get_reg(RC_FLOAT);
        if (lane == 0) {
            if (rd != rn)
                a64_vec_mov(rd, rn, 8);
        } else {
            /* mov rd, rn.T[lane]: the scalar-destination element
               move; imm5 encodes the source lane */
            o(0x5e000400 |
              (uint32_t)((lane << (tz + 1)) | a.es) << 16 |
              fltr(rn) << 5 | fltr(rd));
        }
    } else if (a.es == 8) {
        rd = get_reg(RC_INT);
        if (a.tot == 8)
            o(ARM64_FMOV_XD | intr(rd) | fltr(rn) << 5);
        else
            /* umov rd, rn.d[lane] */
            o(0x4e003c00 | (uint32_t)((lane << 4) | 8) << 16 |
              fltr(rn) << 5 | intr(rd));
    } else {
        int imm5 = (lane << (tz + 1)) | a.es;
        rd = get_reg(RC_INT);
        if (restype->t & VT_UNSIGNED)
            /* umov rd, rn.T[lane] */
            o(0x0e003c00 | (uint32_t)imm5 << 16 |
              fltr(rn) << 5 | intr(rd));
        else
            /* smov rd, rn.T[lane] */
            o(0x0e002c00 |
              (uint32_t)(a.es == 4 || a.tot == 16) << 30 |
              (uint32_t)imm5 << 16 | fltr(rn) << 5 | intr(rd));
    }
    vtop[0].r = rd;
    vtop[0].type = *restype;
}

ST_FUNC void gen_vec_replace(int opc, int lane)
{
    A64Shape a;
    int rvec, rval, rd, tz;
    a64_shape_decode(opc & 15, &a);
    if (lane < 0 || lane >= a.tot / a.es)
        tcc_error("__builtin_aarch64_replace lane %d out of range", lane);
    /* the vector is on top, the scalar value below it */
    gv(RC_VEC);
    rvec = vtop[0].r;
    vswap();
    if (a.fl) {
        if (!((vtop[0].type.t & VT_BTYPE) == VT_FLOAT ||
              (vtop[0].type.t & VT_BTYPE) == VT_DOUBLE))
            gen_cast(&vtop[-1].type.ref->type);
        gv(RC_FLOAT);
    } else {
        if ((vtop[0].type.t & VT_BTYPE) == VT_FLOAT ||
            (vtop[0].type.t & VT_BTYPE) == VT_DOUBLE)
            gen_cast(&vtop[-1].type.ref->type);
        gv(RC_INT);
    }
    rval = vtop[0].r;
    /* allocate the result register while the value is still on the
       stack: for a float value its register is in the same class and
       must not be handed out again for the result */
    rd = get_reg(RC_VEC);
    vtop--; /* drop the value; the vector is on top again */
    if (rd != rvec)
        a64_vec_mov(rd, rvec, a.tot);
    tz = a.es == 1 ? 0 : a.es == 2 ? 1 : a.es == 4 ? 2 : 3;
    if (a.fl)
        /* mov rd.T[lane], rval.T[0] */
        o(0x6e000400 |
          (uint32_t)((lane << (tz + 1)) | a.es) << 16 |
          fltr(rval) << 5 | fltr(rd));
    else
        /* ins rd.T[lane], rval */
        o(0x4e001c00 | (uint32_t)((lane << (tz + 1)) | a.es) << 16 |
          intr(rval) << 5 | fltr(rd));
    vtop[0].r = rd;
}

/* the generic shuffle hook is not used on this target: the
   shuffle intrinsics go through gen_vec_op2i (see the frontend) */
ST_FUNC void gen_vec_shuffle(const int *lanes)
{
    (void)lanes;
    tcc_error("gen_vec_shuffle is not used for this target");
}

/* ------------------------------------------------------------------ */
/* structured loads and stores: the vectors move between memory
   and a run of consecutive vector registers, which the fields of
   the struct at the address operand are then stored to / loaded
   from one by one */

/* base words of the single-structure (lane) forms at index 0,
   indexed by [count - 2][log2 element size]; the store forms
   clear the load bit (22) */
static const uint32_t a64_ldn_lane_base[4][4] = {
    { 0x0d400000, 0x0d404000, 0x0d408000, 0x0d408400 },
    { 0x0d600000, 0x0d604000, 0x0d608000, 0x0d608400 },
    { 0x0d402000, 0x0d406000, 0x0d40a000, 0x0d40a400 },
    { 0x0d602000, 0x0d606000, 0x0d60a000, 0x0d60a400 }
};

/* find a run of cnt consecutive vector registers that no value
   stack entry outside the top nops (the operands being consumed)
   uses; spill what blocks the first run if none is free */
static int a64_vec_group(int cnt, int nops)
{
    int base, i, j, n = (int)(vtop - vstack) + 1 - nops;
    for (base = 0; base + cnt <= 8; base++) {
        int ok = 1;
        for (i = 0; i < cnt && ok; i++)
            for (j = 0; j < n; j++)
                if (!(vstack[j].r & VT_LVAL) &&
                    (vstack[j].r & VT_VALMASK) == TREG_F(base + i))
                    ok = 0;
        if (ok)
            return base;
    }
    for (j = 0; j < n; j++)
        if (!(vstack[j].r & VT_LVAL) &&
            (vstack[j].r & VT_VALMASK) >= (unsigned)TREG_F(0) &&
            (vstack[j].r & VT_VALMASK) < (unsigned)TREG_F(cnt))
            a64_spill_vec(&vstack[j]);
    return 0;
}

static void a64_ldstn(int opc, int is_load)
{
    int op = opc >> 11, lane = (opc >> 4) & 127;
    int cnt, inter, is_lane, dup, tz, base, i, rptr, raddr, sz;
    A64Shape a;
    uint32_t w;
    a64_shape_decode(opc & 15, &a);
    dup = 0;
    if (op < 4) {
        inter = 1; is_lane = 0; cnt = op + 2;
    } else if (op < 8) {
        inter = 1; is_lane = 1; cnt = op - 2;
    } else if (op < 12) {
        inter = 0; is_lane = 0; cnt = op - 6;
    } else if (op < 15) {
        /* replicate loads (vld2/3/4_dup): one element value is
           broadcast into every lane of cnt registers */
        inter = 0; is_lane = 0; dup = 1; cnt = op - 10;
    } else
        tcc_error("__builtin_aarch64_v%cn opcode %d is not implemented",
                  is_load ? 'l' : 's', opc);
    if (is_lane && lane >= a.tot / a.es)
        tcc_error("structured %s lane %d out of range",
                  is_load ? "load" : "store", lane);
    /* the data pointer is at vtop[-1], the struct address on top */
    gv(RC_INT);
    raddr = intr(vtop[0].r);
    vswap();
    gv(RC_INT);
    rptr = intr(vtop[0].r);
    vswap();
    base = a64_vec_group(cnt, 2);
    sz = a.tot == 16 ? 4 : 3;
    tz = a.es == 1 ? 0 : a.es == 2 ? 1 : a.es == 4 ? 2 : 3;
    if (!is_load || is_lane)
        for (i = 0; i < cnt; i++)
            arm64_ldrv(sz, base + i, raddr, i * a.tot);
    if (is_lane) {
        w = a64_ldn_lane_base[cnt - 1][tz];
        if (!is_load)
            w &= ~(1u << 22);
        w |= ((uint32_t)((lane << tz) & 7) << 10) |
             ((uint32_t)((lane << tz) & 8) << 27);
    } else if (dup) {
        if (!is_load)
            tcc_error("__builtin_aarch64_vstn opcode %d is not "
                      "implemented", opc);
        w = 0x0d40c000 | (cnt != 3 ? 0x00200000u : 0) |
            (cnt >= 3 ? 0x2000u : 0) |
            (uint32_t)(a.tot == 16) << 30 | (uint32_t)tz << 10;
    } else {
        /* the interleaved encodings have no 64-bit-element D form
           (size 3 with Q 0 is unallocated); two/three/four 64-bit
           lanes deinterleave to a plain consecutive block, which is
           the multi-register form (what gcc emits for vld2_s64) */
        int op12 = (inter && !(a.es == 8 && a.tot == 8))
                       ? (cnt == 2 ? 8 : cnt == 3 ? 4 : 0)
                       : (cnt == 2 ? 10 : cnt == 3 ? 6 : 2);
        w = 0x0c000000 | (uint32_t)(a.tot == 16) << 30 |
            (uint32_t)op12 << 12 | (uint32_t)tz << 10;
        if (is_load)
            w |= 1u << 22;
    }
    o(w | (uint32_t)rptr << 5 | (uint32_t)base);
    if (is_load)
        for (i = 0; i < cnt; i++)
            arm64_strv(sz, base + i, raddr, i * a.tot);
    vtop -= 2;
}

ST_FUNC void gen_vec_vldn(int opc)
{
    a64_ldstn(opc, 1);
}

ST_FUNC void gen_vec_vstn(int opc)
{
    a64_ldstn(opc, 0);
}


ST_FUNC void gen_opi(int op)
{
    arm64_gen_opil(op, 0);
    arm64_vset_VT_CMP(op);
}

ST_FUNC void gen_opl(int op)
{
    arm64_gen_opil(op, 1);
    arm64_vset_VT_CMP(op);
}

ST_FUNC void gen_opf(int op)
{
    uint32_t x, a, b, dbl;
    int bt = vtop[0].type.t & VT_BTYPE;

    if (op == TOK_NEG) {
        if (bt == VT_LDOUBLE) {
            vpush_helper_func(TOK___negtf2);
            vrott(2);
            gfunc_call(1);
            vpushi(0);
            vtop->type.t = bt;
            vtop->r = REG_FRET;
        } else {
            gv(RC_FLOAT);
            dbl = bt == VT_DOUBLE;
            a = fltr(vtop[0].r);
            o(0x1e214000 | dbl << 22 | a | a << 5);
        }
        return;
    }

    if (bt == VT_LDOUBLE) {
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
        vtop->r = cond < 0 ? REG_FRET : REG_IRET;
        if (cond < 0)
            vtop->type = type;
        else {
            o(0x7100001f); // cmp w0,#0
            o(0x1a9f07e0 | (uint32_t)cond << 12); // cset w0,(cond)
        }
        arm64_vset_VT_CMP(op);
        return;
    }

    dbl = bt != VT_FLOAT;
    gv2(RC_FLOAT, RC_FLOAT);
    assert(vtop[-1].r < VT_CONST && vtop[0].r < VT_CONST);
    a = fltr(vtop[-1].r);
    b = fltr(vtop[0].r);
    vtop -= 2;
    switch (op) {
    case TOK_EQ: case TOK_NE:
    case TOK_LT: case TOK_GE: case TOK_LE: case TOK_GT:
        x = get_reg(RC_INT);
        ++vtop;
        vtop[0].r = x;
        x = intr(x);
        break;
    default:
        x = get_reg(RC_FLOAT);
        ++vtop;
        vtop[0].r = x;
        x = fltr(x);
        break;
    }

    switch (op) {
    case '*':
        o(0x1e200800 | dbl << 22 | x | a << 5 | b << 16); // fmul
        break;
    case '+':
        o(0x1e202800 | dbl << 22 | x | a << 5 | b << 16); // fadd
        break;
    case '-':
        o(0x1e203800 | dbl << 22 | x | a << 5 | b << 16); // fsub
        break;
    case '/':
        o(0x1e201800 | dbl << 22 | x | a << 5 | b << 16); // fdiv
        break;
    case TOK_EQ:
        o(0x1e202000 | dbl << 22 | a << 5 | b << 16); // fcmp
        o(0x1a9f17e0 | x); // cset w(x),eq
        break;
    case TOK_GE:
        o(0x1e202000 | dbl << 22 | a << 5 | b << 16); // fcmp
        o(0x1a9fb7e0 | x); // cset w(x),ge
        break;
    case TOK_GT:
        o(0x1e202000 | dbl << 22 | a << 5 | b << 16); // fcmp
        o(0x1a9fd7e0 | x); // cset w(x),gt
        break;
    case TOK_LE:
        o(0x1e202000 | dbl << 22 | a << 5 | b << 16); // fcmp
        o(0x1a9f87e0 | x); // cset w(x),ls
        break;
    case TOK_LT:
        o(0x1e202000 | dbl << 22 | a << 5 | b << 16); // fcmp
        o(0x1a9f57e0 | x); // cset w(x),mi
        break;
    case TOK_NE:
        o(0x1e202000 | dbl << 22 | a << 5 | b << 16); // fcmp
        o(0x1a9f07e0 | x); // cset w(x),ne
        break;
    default:
        assert(0);
    }
    arm64_vset_VT_CMP(op);
}

// Generate sign extension from 32 to 64 bits:
ST_FUNC void gen_cvt_sxtw(void)
{
    uint32_t r = intr(gv(RC_INT));
    o(0x93407c00 | r | r << 5); // sxtw x(r),w(r)
}

/* char/short to int conversion */
ST_FUNC void gen_cvt_csti(int t)
{
    int r = intr(gv(RC_INT));
    o(0x13001c00
        | ((t & VT_BTYPE) == VT_SHORT) << 13
        | (uint32_t)!!(t & VT_UNSIGNED) << 30
        | r | r << 5); // [su]xt[bh] w(r),w(r)
}

ST_FUNC void gen_cvt_itof(int t)
{
    if (t == VT_LDOUBLE) {
        int f = vtop->type.t;
        int func = (f & VT_BTYPE) == VT_LLONG ?
          (f & VT_UNSIGNED ? TOK___floatunditf : TOK___floatditf) :
          (f & VT_UNSIGNED ? TOK___floatunsitf : TOK___floatsitf);
        vpush_helper_func(func);
        vrott(2);
        gfunc_call(1);
        vpushi(0);
        vtop->type.t = t;
        vtop->r = REG_FRET;
        return;
    }
    else {
        int d, n = intr(gv(RC_INT));
        int s = !(vtop->type.t & VT_UNSIGNED);
        uint32_t l = ((vtop->type.t & VT_BTYPE) == VT_LLONG);
        --vtop;
        d = get_reg(RC_FLOAT);
        ++vtop;
        vtop[0].r = d;
        o(0x1e220000 | (uint32_t)!s << 16 |
          (uint32_t)(t != VT_FLOAT) << 22 | fltr(d) |
          l << 31 | n << 5); // [us]cvtf [sd](d),[wx](n)
    }
}

ST_FUNC void gen_cvt_ftoi(int t)
{
    if ((vtop->type.t & VT_BTYPE) == VT_LDOUBLE) {
        int func = (t & VT_BTYPE) == VT_LLONG ?
          (t & VT_UNSIGNED ? TOK___fixunstfdi : TOK___fixtfdi) :
          (t & VT_UNSIGNED ? TOK___fixunstfsi : TOK___fixtfsi);
        vpush_helper_func(func);
        vrott(2);
        gfunc_call(1);
        vpushi(0);
        vtop->type.t = t;
        vtop->r = REG_IRET;
        return;
    }
    else {
        int d, n = fltr(gv(RC_FLOAT));
        uint32_t l = ((vtop->type.t & VT_BTYPE) != VT_FLOAT);
        --vtop;
        d = get_reg(RC_INT);
        ++vtop;
        vtop[0].r = d;
        o(0x1e380000 |
          (uint32_t)!!(t & VT_UNSIGNED) << 16 |
          (uint32_t)((t & VT_BTYPE) == VT_LLONG) << 31 | intr(d) |
          l << 22 | n << 5); // fcvtz[su] [wx](d),[sd](n)
    }
}

ST_FUNC void gen_cvt_ftof(int t)
{
    int f = vtop[0].type.t & VT_BTYPE;
    assert(t == VT_FLOAT || t == VT_DOUBLE || t == VT_LDOUBLE);
    assert(f == VT_FLOAT || f == VT_DOUBLE || f == VT_LDOUBLE);
    if (t == f)
        return;

    if (t == VT_LDOUBLE || f == VT_LDOUBLE) {
        int func = (t == VT_LDOUBLE) ?
            (f == VT_FLOAT ? TOK___extendsftf2 : TOK___extenddftf2) :
            (t == VT_FLOAT ? TOK___trunctfsf2 : TOK___trunctfdf2);
        vpush_helper_func(func);
        vrott(2);
        gfunc_call(1);
        vpushi(0);
        vtop->type.t = t;
        vtop->r = REG_FRET;
    }
    else {
        int x, a;
        gv(RC_FLOAT);
        assert(vtop[0].r < VT_CONST);
        a = fltr(vtop[0].r);
        x = a;
        if (f == VT_FLOAT)
            o(0x1e22c000 | x | a << 5); // fcvt d(x),s(a)
        else
            o(0x1e624000 | x | a << 5); // fcvt s(x),d(a)
    }
}

/* increment tcov counter */
ST_FUNC void gen_increment_tcov (SValue *sv)
{
    int r1, r2;

    vpushv(sv);
    vtop->r = r1 = get_reg(RC_INT);
    r2 = get_reg(RC_INT);
    arm64_sym(r1, sv->sym, 0);
    o(ARM64_LDR_X | ARM64_RN(intr(r1)) | intr(r2)); // ldr r2, [r1]
    o(0x91000400 | (intr(r2)<<5) | intr(r2)); // add r2, r2, #1
    o(0xf9000000 | (intr(r1)<<5) | intr(r2)); // str r2, [r1]
    vpop();
}

ST_FUNC void ggoto(void)
{
    arm64_gen_bl_or_b(1);
    --vtop;
}

ST_FUNC void gen_clear_cache(void)
{
    uint32_t beg, end, dsz, isz, p, lab1, b1;
    gv2(RC_INT, RC_INT);
    vpushi(0);
    vtop->r = get_reg(RC_INT);
    vpushi(0);
    vtop->r = get_reg(RC_INT);
    vpushi(0);
    vtop->r = get_reg(RC_INT);
    beg = intr(vtop[-4].r); // x0
    end = intr(vtop[-3].r); // x1
    dsz = intr(vtop[-2].r); // x2
    isz = intr(vtop[-1].r); // x3
    p = intr(vtop[0].r);    // x4
    vtop -= 5;

    o(0xd53b0020 | isz); // mrs x(isz),ctr_el0
    o(0x52800080 | p); // mov w(p),#4
    o(0x53104c00 | dsz | isz << 5); // ubfx w(dsz),w(isz),#16,#4
    o(0x1ac02000 | dsz | p << 5 | dsz << 16); // lsl w(dsz),w(p),w(dsz)
    o(0x12000c00 | isz | isz << 5); // and w(isz),w(isz),#15
    o(0x1ac02000 | isz | p << 5 | isz << 16); // lsl w(isz),w(p),w(isz)
    o(0x51000400 | p | dsz << 5); // sub w(p),w(dsz),#1
    o(0x8a240004 | p | beg << 5 | p << 16); // bic x(p),x(beg),x(p)
    b1 = ind; o(ARM64_B); // b
    lab1 = ind;
    o(0xd50b7b20 | p); // dc cvau,x(p)
    o(0x8b000000 | p | p << 5 | dsz << 16); // add x(p),x(p),x(dsz)
    write32le(cur_text_section->data + b1, ARM64_B | ((ind - b1) >> 2));
    o(0xeb00001f | p << 5 | end << 16); // cmp x(p),x(end)
    o(0x54ffffa3 | ((lab1 - ind) << 3 & 0xffffe0)); // b.cc lab1
    o(0xd5033b9f); // dsb ish
    o(0x51000400 | p | isz << 5); // sub w(p),w(isz),#1
    o(0x8a240004 | p | beg << 5 | p << 16); // bic x(p),x(beg),x(p)
    b1 = ind; o(ARM64_B); // b
    lab1 = ind;
    o(0xd50b7520 | p); // ic ivau,x(p)
    o(0x8b000000 | p | p << 5 | isz << 16); // add x(p),x(p),x(isz)
    write32le(cur_text_section->data + b1, ARM64_B | ((ind - b1) >> 2));
    o(0xeb00001f | p << 5 | end << 16); // cmp x(p),x(end)
    o(0x54ffffa3 | ((lab1 - ind) << 3 & 0xffffe0)); // b.cc lab1
    o(0xd5033b9f); // dsb ish
    o(0xd5033fdf); // isb
}

ST_FUNC void gen_vla_sp_save(int addr) {
    uint32_t r = intr(get_reg(RC_INT));
    o(0x910003e0 | r); // mov x(r),sp
    arm64_strx(3, r, 29, addr);
}

ST_FUNC void gen_vla_sp_restore(int addr) {
    // Use x30 because this function can be called when there
    // is a live return value in x0 but there is nothing on
    // the value stack to prevent get_reg from returning x0.
    uint32_t r = 30;
    arm64_ldrx(0, 3, r, 29, addr);
    o(0x9100001f | r << 5); // mov sp,x(r)
}

ST_FUNC void gen_vla_alloc(CType *type, int align) {
    uint32_t r;
#if defined(CONFIG_TCC_BCHECK)
    if (tcc_state->do_bounds_check)
        vpushv(vtop);
#endif
    r = intr(gv(RC_INT));
#if defined(CONFIG_TCC_BCHECK)
    if (tcc_state->do_bounds_check)
        o(0x91004000 | r | r << 5); // add x(r),x(r),#15+1
    else
#endif
    o(0x91003c00 | r | r << 5); // add x(r),x(r),#15
    o(0x927cec00 | r | r << 5); // bic x(r),x(r),#15
    o(0xcb2063ff | r << 16); // sub sp,sp,x(r)
    vpop();
#if defined(CONFIG_TCC_BCHECK)
    if (tcc_state->do_bounds_check) {
        vpushi(0);
        vtop->r = TREG_R(0);
        o(0x910003e0 | vtop->r); // mov r0,sp
        vswap();
        vpush_helper_func(TOK___bound_new_region);
        vrott(3);
        gfunc_call(2);
        func_bound_add_epilog = 1;
    }
#endif
}

/* end of A64 code generator */
/*************************************************************/
#endif
/*************************************************************/
