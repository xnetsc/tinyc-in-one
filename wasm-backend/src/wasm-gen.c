/* wasm-gen.c - TinyCC code generator for wasm32 (WASI) - SKELETON
 *
 * Design (see ASSESSMENT.md):
 *  - TCC "registers" are mapped to wasm locals (8 i32 regs, 8 f64 regs).
 *  - C stack frame lives in linear memory; a wasm local ($fp) holds the
 *    frame base, a mutable wasm global ($__stack_pointer, global 0)
 *    holds the stack top.  Locals/params are addressed as $fp + c,
 *    with c the negative offsets assigned by tccgen/gfunc_set_param,
 *    exactly like the riscv64 backend addresses via its frame pointer.
 *  - Each function body is wrapped in one wasm block ($exit); a return
 *    jump (gjmp on the rsym chain) is emitted as "br $exit".  This is
 *    the seed of the basic-block dispatch design: control-flow
 *    constructs other than return are explicit stubs in this skeleton.
 *  - All LEB128 operands that may need patching at link time (call
 *    target indices, symbol addresses, frame sizes) are emitted in
 *    padded 5-byte form so patching never changes code size.
 *  - Emission also mirrors one byte per emitted byte into the ELF text
 *    section so that `ind' keeps its usual meaning for tccgen
 *    (function sizes, st_size patching at tccgen.c:8856).
 */

#if defined TARGET_DEFS_ONLY

#define NB_REGS 18 /* 0..7 int regs, 8..15 float regs, 16 fp, 17 sp (pseudo) */

#define RC_INT     (1 << 0)
#define RC_FLOAT   (1 << 1)
#define RC_R(x)    (1 << (2 + (x)))   /* x = 0..7 */
#define RC_F(x)    (1 << (10 + (x)))  /* x = 0..7 */

#define RC_IRET    RC_R(0)
#define RC_IRE2    RC_R(1)
#define RC_FRET    RC_F(0)

#define REG_IRET 0  /* int return register */
#define REG_IRE2 1  /* second int return register (long long pair) */
#define REG_FRET 8  /* float return register */

#define PTR_SIZE 4
#define LDOUBLE_SIZE  8   /* long double is downgraded to double */
#define LDOUBLE_ALIGN 8
#define MAX_ALIGN 8

/* Vector types use VT_VECTOR from tcc.h: the element type is in
   CType.ref->type and the element count in CType.ref->c.  Vector
   values live in four v128 locals per function (see wloc_vec);
   the SValue "register" WREG_VEC + slot names them, and slot 0 is
   the return-value slot.  WREG_VEC must be visible here because
   tccgen.c's R_RET refers to it in wasm builds. */
#define WREG_VEC 32 /* SValue register base for the v128 slots */

/* tccrun.c / tccelf.c consult this for the size of the return type
   register class on some paths; wasm returns scalars in one register */
#define PROMOTE_RET 1

#else /* !TARGET_DEFS_ONLY */

#define USING_GLOBALS
#include "tcc.h"
#include "wasm-int.h"
#include <assert.h>

ST_DATA const char * const target_machine_defs =
    "__wasm__\0"
    "__wasm32__\0"
    "__WASI__\0"
    "__wasm32_wasi__\0"
    ;

#define WREG_FP 16  /* pseudo register slots, never allocated by get_reg */
#define WREG_SP 17

ST_DATA const int reg_classes[NB_REGS] = {
  RC_INT | RC_R(0), RC_INT | RC_R(1), RC_INT | RC_R(2), RC_INT | RC_R(3),
  RC_INT | RC_R(4), RC_INT | RC_R(5), RC_INT | RC_R(6), RC_INT | RC_R(7),
  RC_FLOAT | RC_F(0), RC_FLOAT | RC_F(1), RC_FLOAT | RC_F(2), RC_FLOAT | RC_F(3),
  RC_FLOAT | RC_F(4), RC_FLOAT | RC_F(5), RC_FLOAT | RC_F(6), RC_FLOAT | RC_F(7),
  1 << WREG_FP, 1 << WREG_SP
};

/* ------------------------------------------------------------------ */
/* shared module state: types, registries and the wb_* buffer        */
/* functions are declared in wasm-int.h (used by wasm-link.c too);   */
/* the definitions live here.                                        */

void wb_init(WBuf *b) { b->data = NULL; b->len = b->cap = 0; }
void wb_free(WBuf *b) { tcc_free(b->data); wb_init(b); }
void wb_putc(WBuf *b, int c)
{
    if (b->len >= b->cap) {
        b->cap = b->cap ? b->cap * 2 : 256;
        b->data = tcc_realloc(b->data, b->cap);
    }
    b->data[b->len++] = (unsigned char)c;
}
void wb_write(WBuf *b, const void *p, int n)
{
    const unsigned char *s = p;
    while (n-- > 0) wb_putc(b, *s++);
}
void wb_uleb(WBuf *b, uint32_t v)
{
    do { int c = v & 0x7f; v >>= 7; if (v) c |= 0x80; wb_putc(b, c); } while (v);
}
/* fixed 5-byte padded unsigned LEB128 (legal wasm, size-stable) */
static void wb_uleb5(WBuf *b, uint32_t v)
{
    int i;
    for (i = 0; i < 4; i++) { wb_putc(b, (v & 0x7f) | 0x80); v >>= 7; }
    wb_putc(b, v & 0x7f);
}
void wb_sleb(WBuf *b, int64_t v)
{
    for (;;) {
        int c = v & 0x7f; v >>= 7;
        if ((v == 0 && !(c & 0x40)) || (v == -1 && (c & 0x40))) { wb_putc(b, c); break; }
        wb_putc(b, c | 0x80);
    }
}
/* fixed 5-byte padded signed LEB128 for i32 values */
static void wb_sleb5(WBuf *b, int32_t v)
{
    int i; int64_t x = v;
    for (i = 0; i < 4; i++) { wb_putc(b, (x & 0x7f) | 0x80); x >>= 7; }
    wb_putc(b, x & 0x7f);
}
void wb_patch_uleb5(WBuf *b, int pos, uint32_t v)
{
    int i;
    for (i = 0; i < 4; i++) { b->data[pos + i] = (v & 0x7f) | 0x80; v >>= 7; }
    b->data[pos + 4] = v & 0x7f;
}
void wb_patch_sleb5(WBuf *b, int pos, int32_t v)
{
    int i; int64_t x = v;
    for (i = 0; i < 4; i++) { b->data[pos + i] = (x & 0x7f) | 0x80; x >>= 7; }
    b->data[pos + 4] = x & 0x7f;
}

static int wasm_vec_bytes(CType *t); /* defined in the v128 section */

/* tcc.h declares this ST_DATA and every backend's gen file defines
 * it (x86_64-gen.c, arm64-gen.c, ...); tccgen.c sets and reads it on
 * the bounds-check path.  In the unity build the tcc.h declaration
 * itself was the definition (ST_DATA is static there); as a separate
 * translation unit the backend must provide it. */
ST_DATA int func_bound_add_epilog;

WSig wsigs[128];
int nb_wsigs;
WFunc wfuns[512];
int nb_wfuns;

static int wasm_type_of(CType *t)
{
    if (t->t & VT_VECTOR) {
        int n = wasm_vec_bytes(t);
        return (n == 8 || n == 16) ? WT_V128 : -2;
    }
    switch (t->t & VT_BTYPE) {
    case VT_VOID:   return -1;
    case VT_BYTE: case VT_SHORT: case VT_INT: case VT_BOOL:
    case VT_PTR: case VT_FUNC:   return WT_I32;
    case VT_LLONG:  return WT_I64;
    case VT_FLOAT:  return WT_F32;
    case VT_DOUBLE: case VT_LDOUBLE: return WT_F64;
    default: return -2; /* struct etc: unsupported */
    }
}

static int wasm_sig_intern(WSig *sig)
{
    int i, j;
    for (i = 0; i < nb_wsigs; i++) {
        if (wsigs[i].nparams == sig->nparams && wsigs[i].ret == sig->ret) {
            for (j = 0; j < sig->nparams; j++)
                if (wsigs[i].params[j] != sig->params[j]) break;
            if (j == sig->nparams) return i;
        }
    }
    if (nb_wsigs >= 128) tcc_error("wasm: too many distinct signatures");
    wsigs[nb_wsigs] = *sig;
    return nb_wsigs++;
}

/* Build a WSig from a TCC function type.  Returns 0 on success. */
static int wasm_sig_from_ftype(CType *ft, WSig *sig)
{
    Sym *s;
    sig->nparams = 0;
    /* the return type lives in the prototype sym (tccgen.c:8819:
       func_vt = sym->type.ref->type), not in the VT_FUNC type itself */
    sig->ret = ft->ref ? wasm_type_of(&ft->ref->type) : WT_I32;
    if (sig->ret == -2) return -1;
    for (s = ft->ref; (s = s->next) != NULL; ) {
        int wt;
        if (s->v == FUNC_ELLIPSIS) return -1; /* variadic: unsupported */
        wt = wasm_type_of(&s->type);
        if (wt < 0 || sig->nparams >= 16) return -1;
        sig->params[sig->nparams++] = wt;
    }
    return 0;
}

static int wasm_sig_equal(const WSig *a, const WSig *b)
{
    int i;
    if (a->nparams != b->nparams || a->ret != b->ret) return 0;
    for (i = 0; i < a->nparams; i++)
        if (a->params[i] != b->params[i]) return 0;
    return 1;
}

static WFunc *wasm_func_intern_sig(Sym *sym, WSig *sig, int from_call)
{
    int i;
    WFunc *f;
    const char *name = get_tok_str(sym->v, NULL);
    /* keyed by name, not by Sym*: Sym objects do not outlive their
       translation unit, names do.  (Two static functions with the
       same name in different files would collide - a documented
       skeleton limitation.) */
    for (i = 0; i < nb_wfuns; i++) {
        if (strcmp(wfuns[i].name, name)) continue;
        f = &wfuns[i];
        f->sym = sym;
        if (f->sig_from_call && !from_call) {
            /* a real prototype supersedes the call-site signature */
            f->sig = wasm_sig_intern(sig);
            f->nparams = sig->nparams;
            f->rettype = sig->ret;
            f->sig_from_call = 0;
        } else if (f->sig_from_call && from_call &&
                   !wasm_sig_equal(&wsigs[f->sig], sig))
            tcc_error("wasm: conflicting call signatures for undeclared "
                      "function '%s'", name);
        return f;
    }
    if (nb_wfuns >= 512) tcc_error("wasm: too many functions");
    f = &wfuns[nb_wfuns++];
    memset(f, 0, sizeof *f);
    f->sym = sym;
    f->name = tcc_strdup(name);
    f->sig = wasm_sig_intern(sig);
    f->sig_from_call = from_call;
    f->nparams = sig->nparams;
    f->rettype = sig->ret;
    f->final_index = -1;
    wb_init(&f->final_body);
    return f;
}

static WFunc *wasm_func_intern(Sym *sym, CType *ftype)
{
    WSig sig;
    if (wasm_sig_from_ftype(ftype, &sig) != 0)
        tcc_error("wasm: unsupported function signature for '%s'",
                  get_tok_str(sym->v, NULL));
    return wasm_func_intern_sig(sym, &sig, 0);
}

static void wasm_add_fix(WFunc *f, int kind, int pos, Sym *sym, int addend)
{
    WFix *x;
    if (f->nfixs >= f->fixcap) {
        f->fixcap = f->fixcap ? f->fixcap * 2 : 16;
        f->fixs = tcc_realloc(f->fixs, f->fixcap * sizeof(WFix));
    }
    x = &f->fixs[f->nfixs++];
    x->pos = pos;
    x->name = tcc_strdup(get_tok_str(sym->v, NULL));
    x->kind = kind; x->addend = addend;
    x->shndx = -1; x->value = 0;
    {
        /* A symbol already defined in a data section when the fixup
           is created - an anonymous symbol (float constants spilled
           into .rodata by gv(), compound literals), a static, or a
           global defined above its use - has a final ELF section and
           offset here.  Capture both.  Output-time resolution by name
           cannot find anonymous symbols (the name fabricated for
           them, e.g. "L.0", does not exist in the symtab) or local
           symbols (the symtab hash only holds global/weak symbols),
           and section+offset stays unambiguous even when two
           block-scope statics share a name.  Symbols not yet defined
           (externs, calls through prototypes) keep name resolution.
           Function symbols are excluded: a function's wasm address
           is its table index, not a memory offset. */
        ElfSym *es = elfsym(sym);
        if (es && es->st_shndx != SHN_UNDEF &&
            es->st_shndx < tcc_state->nb_sections) {
            Section *sec = tcc_state->sections[es->st_shndx];
            const char *nm = sec->name;
            if (nm && (!strcmp(nm, ".rodata") || !strncmp(nm, ".rodata.", 8) ||
                       !strcmp(nm, ".data") || !strncmp(nm, ".data.", 6) ||
                       !strcmp(nm, ".bss"))) {
                x->shndx = es->st_shndx;
                x->value = es->st_value;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* emission into the current function body                             */

static WBuf wcur;          /* body bytes of the function being compiled */
static WFunc *wcur_func;   /* its registry entry */
static int wcur_started;

/* local indices inside a wasm function */
static int wloc_reg(int r)   { return wcur_func->nparams + r; }       /* TCC reg r */
static int wloc_fp(void)     { return wcur_func->nparams + NB_REGS; } /* frame base */
static int wloc_scratch(int i) { return wcur_func->nparams + NB_REGS + 1 + i; }
static int wloc_pc(void)     { return wcur_func->nparams + 63; } /* basic-block dispatch $pc */
static int wloc_carry(void)  { return wcur_func->nparams + 64; } /* carry between ADDC1/ADDC2 */
static int wloc_i64(void)    { return wcur_func->nparams + 65; } /* i64 scratch (cvt/llong call glue) */

static void we(int c) /* emit one byte, mirrored into the ELF text section */
{
    int ind1;
    wb_putc(&wcur, c);
    ind1 = ind + 1;
    if (ind1 > cur_text_section->data_allocated)
        section_realloc(cur_text_section, ind1);
    cur_text_section->data[ind] = (unsigned char)c;
    ind = ind1;
}
static void we_uleb(uint32_t v)  { WBuf t; wb_init(&t); wb_uleb(&t, v);
                                   { int i; for (i = 0; i < t.len; i++) we(t.data[i]); } wb_free(&t); }
static void we_uleb5(uint32_t v) { int i; unsigned char tmp[5]; WBuf t; wb_init(&t);
                                   wb_uleb5(&t, v); for (i = 0; i < 5; i++) tmp[i] = t.data[i];
                                   wb_free(&t); for (i = 0; i < 5; i++) we(tmp[i]); }
static void we_sleb5(int32_t v)  { int i; unsigned char tmp[5]; WBuf t; wb_init(&t);
                                   wb_sleb5(&t, v); for (i = 0; i < 5; i++) tmp[i] = t.data[i];
                                   wb_free(&t); for (i = 0; i < 5; i++) we(tmp[i]); }
static void we_local_get(int idx) { we(0x20); we_uleb(idx); }
static void we_local_set(int idx) { we(0x21); we_uleb(idx); }
static void we_global_get(int idx) { we(0x23); we_uleb(idx); }
static void we_global_set(int idx) { we(0x24); we_uleb(idx); }
static void we_i32_const(int32_t v) { we(0x41); we_sleb5(v); }
static void we_sleb(int64_t v)
{
    for (;;) {
        int c = v & 0x7f; v >>= 7;
        if ((v == 0 && !(c & 0x40)) || (v == -1 && (c & 0x40))) { we(c); break; }
        we(c | 0x80);
    }
}
static void we_i64_const(int64_t v) { we(0x42); we_sleb(v); }

/* ------------------------------------------------------------------ */
/* control flow: basic-block dispatch                                  */
/*                                                                     */
/* Wasm has no patchable jumps, so the function body is assembled at   */
/* epilog into a dispatch loop: blocks start at every jump target,     */
/* a $pc local selects the block through a br_table, and every jump    */
/* site in the emitted stream is a 5-byte placeholder that the         */
/* assembly replaces with "pc = target block; br $dispatch".           */
/* Jump chains keep the generic tccgen protocol: a chain node is the   */
/* absolute ind of a site's second byte, the previous head is threaded */
/* through the word in the text-section mirror at that offset, and     */
/* gsym_addr records the resolution target on every site of the chain. */

typedef struct WJmp {
    int node;   /* chain node value (absolute ind), 0 when not chained */
    int off;    /* stream offset of the 5 placeholder bytes */
    int cond;   /* site sits inside an `if` (gjmp_cond): one more
                   enclosing construct when branching to the loop */
    int target; /* target absolute ind, -1 until resolved */
} WJmp;

static WJmp *wjmps;
static int nb_wjmps, cap_wjmps;
static int wfunc_ind0; /* ind of the first byte of the current
                          function's stream (wcur offset 0) */

/* record a jump site whose 5 placeholder bytes were just emitted
   starting at absolute ind 'site'; returns the record index */
static int wasm_jmp_record(int site, int node, int cond, int target)
{
    WJmp *j;
    if (nb_wjmps >= cap_wjmps) {
        cap_wjmps = cap_wjmps ? cap_wjmps * 2 : 32;
        wjmps = tcc_realloc(wjmps, cap_wjmps * sizeof(WJmp));
    }
    j = &wjmps[nb_wjmps];
    j->node = node;
    j->off = site - wfunc_ind0;
    j->cond = cond;
    j->target = target;
    return nb_wjmps++;
}

static void wasm_jmp_placeholders(void)
{
    we(0x01); we(0x01); we(0x01); we(0x01); we(0x01); /* nops */
}

/* cmp_r operand marker: the operand is the constant 0 (used for the
   bool register produced by a floating-point comparison) */
#define WCMP_ZERO 63

/* The operands of the most recent integer comparison this backend
   turned into a VT_CMP.  gen_opl's high-word dance (tccgen.c) is
   written for flag-based backends: after gvtst() consumes the
   high-word comparison it tests the SAME comparison again with
   TOK_NE via a VT_CMP whose cmp_r is zero (vpushi + vset_VT_CMP),
   relying on the condition still being live.  Wasm has no flags,
   so a VT_CMP with cmp_r == 0 arriving at gjmp_cond/load uses
   these ambient operands instead.  A genuine VT_CMP always records
   its own operands here when created, so the substitution is a
   no-op for it. */
static int amb_cmp_a, amb_cmp_b, amb_cmp_valid;

static void wasm_cmp_operands(SValue *sv, int *a, int *b)
{
    if (sv->cmp_r == 0 && amb_cmp_valid) {
        *a = amb_cmp_a;
        *b = amb_cmp_b;
    } else {
        *a = sv->cmp_r & 0xff;
        *b = (sv->cmp_r >> 8) & 0xff;
    }
}

static void wasm_cmp_operand(int r)
{
    if (r == WCMP_ZERO) we_i32_const(0);
    else we_local_get(wloc_reg(r));
}

static int wasm_icmp_opc(int op)
{
    switch (op) {
    case TOK_EQ:  return 0x46; /* i32.eq */
    case TOK_NE:  return 0x47; /* i32.ne */
    case TOK_LT:  return 0x48; /* i32.lt_s */
    case TOK_ULT: return 0x49; /* i32.lt_u */
    case TOK_GT:  return 0x4a; /* i32.gt_s */
    case TOK_UGT: return 0x4b; /* i32.gt_u */
    case TOK_LE:  return 0x4c; /* i32.le_s */
    case TOK_ULE: return 0x4d; /* i32.le_u */
    case TOK_GE:  return 0x4e; /* i32.ge_s */
    case TOK_UGE: return 0x4f; /* i32.ge_u */
    }
    tcc_error("wasm: comparison operator 0x%x not implemented", op);
    return 0;
}

/* emit the integer comparison described by a VT_CMP SValue's
   cmp_op/cmp_r onto the wasm stack (an i32 0/1) */
static void wasm_cmp_emit(SValue *sv)
{
    int a, b;
    wasm_cmp_operands(sv, &a, &b);
    wasm_cmp_operand(a);
    wasm_cmp_operand(b);
    we(wasm_icmp_opc(sv->cmp_op));
}

/* TCC's byte emitter: for wasm only the low byte is meaningful. */
ST_FUNC void o(unsigned int c)
{
    if (nocode_wanted) return;
    we(c & 0xff);
}

/* ------------------------------------------------------------------ */
/* load / store                                                        */

/* push the address described by an lvalue SValue onto the wasm stack */
static void wasm_push_addr(SValue *sv)
{
    int fr = sv->r, v = fr & VT_VALMASK;
    if (fr & VT_SYM) {
        /* Placeholder constant; wasm-link patches the sleb operand
           with (symbol's linear-memory address + addend), where the
           addend is sv->c.i.  Fixup offsets are relative to wcur and
           are shifted at epilog when the final body is assembled. */
        int pos = wcur.len;
        we(0x41); we_sleb5(0);
        wasm_add_fix(wcur_func, WFIX_ADDR, pos + 1, sv->sym, (int)sv->c.i);
        return;
    }
    if (v == VT_LOCAL || v == VT_LLOCAL) {
        we_local_get(wloc_fp());
        if (sv->c.i) { we_i32_const((int32_t)sv->c.i); we(0x6a); }
        if (v == VT_LLOCAL) { we(0x28); we(0x02); we(0x00); } /* i32.load */
        return;
    }
    if (v < VT_CONST) { /* address held in a register */
        /* The register holds the exact address: pointer arithmetic
           is materialised into the register value by gen_op, and
           sv->c.i on a register lvalue is a stale leftover of the
           lvalue the pointer was loaded from (e.g. the frame offset
           of the pointer variable itself after indir()'s gv()).
           The riscv64 backend discards it the same way
           ("fc = 0; // XXX store ofs in LVAL(reg)"). */
        we_local_get(wloc_reg(v));
        return;
    }
    if (v == VT_CONST) { /* constant is the address */
        we_i32_const((int32_t)sv->c.i);
        return;
    }
    tcc_error("wasm: unsupported lvalue kind in load/store");
}

/* ------------------------------------------------------------------ */
/* v128 (SIMD128) support                                              */
/*                                                                     */
/* Vector values live in four dedicated v128 locals (slots 0..3); a    */
/* vector SValue carries r = WREG_VEC + slot.  Slot 0 is the           */
/* return-value slot.  The opcode numbers are the wasm SIMD fixed      */
/* numbering (0xFD prefix), verified against a wasm engine.            */
/*                                                                     */
/* TCC's scalar register model cannot spill or reload these locals:    */
/* get_reg/save_regs only know the scalar slots.  This section         */
/* therefore manages the four slots itself: a value is materialised    */
/* into a slot on demand (wasm_vec_ensure) and, when all slots are     */
/* busy, the deepest live vector value is spilled to a fresh frame     */
/* slot (a memory home) and reloaded on next use.  Wasm locals are     */
/* not clobbered by calls, so the only caller-save hazard is a         */
/* vector return value landing in slot 0, which gfunc_call spills      */
/* around.                                                             */

static int wloc_vec(int i) { return wcur_func->nparams + 59 + i; }

static void we_v128_op(int opc) { we(0xfd); we_uleb(opc); }

/* total size in bytes of a vector type, 0 for non-vectors */
static int wasm_vec_bytes(CType *t)
{
    int align;
    if (!(t->t & VT_VECTOR)) return 0;
    return type_size(t, &align);
}

/* a vector the backend can hold in a v128 local: 64 or 128 bits */
static int wasm_is_vec(CType *t)
{
    int n = wasm_vec_bytes(t);
    return n == 8 || n == 16;
}

/* element shape of a vector type */
static void wasm_vec_shape(CType *t, int *esize, int *is_float, int *is_unsigned)
{
    CType *e = &t->ref->type;
    int align, bt = e->t & VT_BTYPE;
    *esize = type_size(e, &align);
    *is_float = (bt == VT_FLOAT || bt == VT_DOUBLE || bt == VT_LDOUBLE);
    *is_unsigned = (e->t & VT_UNSIGNED) != 0;
}

static int wasm_vec_sv_slot(SValue *sv)
{
    int v = sv->r & VT_VALMASK;
    return (v >= WREG_VEC && v < WREG_VEC + 4) ? v - WREG_VEC : -1;
}

/* spill the vector value held by 'sv' to a fresh frame slot and turn
   'sv' into an lvalue naming that slot */
static void wasm_vec_spill_sv(SValue *sv)
{
    int slot = wasm_vec_sv_slot(sv);
    SValue dest;
    if (slot < 0) return;
    loc = (loc - 16) & -16;
    dest.type = sv->type;
    dest.r = VT_LOCAL | VT_LVAL;
    dest.c.i = loc;
    dest.sym = NULL;
    store(WREG_VEC + slot, &dest);
    sv->r = VT_LOCAL | VT_LVAL;
    sv->c.i = loc;
    sv->sym = NULL;
}

/* slots pinned by instruction sequences in flight (not on the vstack) */
static int wvec_pinned;

static int wasm_vec_alloc(SValue *keep1, SValue *keep2)
{
    int used[4] = { 0, 0, 0, 0 };
    SValue *p;
    int i, s;
    for (p = vstack; p <= vtop; p++) {
        if (!(p->type.t & VT_VECTOR)) continue;
        s = wasm_vec_sv_slot(p);
        if (s >= 0) used[s] = 1;
    }
    for (i = 0; i < 4; i++)
        if (wvec_pinned & (1 << i)) used[i] = 1;
    for (i = 0; i < 4; i++)
        if (!used[i]) return i;
    for (p = vstack; p <= vtop; p++) {
        if (p == keep1 || p == keep2) continue;
        if (!(p->type.t & VT_VECTOR)) continue;
        s = wasm_vec_sv_slot(p);
        if (s >= 0 && !(wvec_pinned & (1 << s))) {
            wasm_vec_spill_sv(p);
            return s;
        }
    }
    tcc_error("wasm: out of vector slots");
    return 0;
}

/* make sure the vector value 'sv' is in a v128 slot; returns the
   slot and updates sv->r to name it.  'keep' is another SValue whose
   slot must not be taken or spilled. */
static int wasm_vec_ensure(SValue *sv, SValue *keep)
{
    int s = wasm_vec_sv_slot(sv);
    if (s >= 0) return s;
    if (!wasm_is_vec(&sv->type))
        tcc_error("wasm: a vector of %d bytes cannot be held by the wasm backend "
                  "(only 64/128-bit vectors)", wasm_vec_bytes(&sv->type));
    s = wasm_vec_alloc(sv, keep);
    load(WREG_VEC + s, sv);
    sv->r = WREG_VEC + s;
    return s;
}

ST_FUNC void gen_vec_gv(void)
{
    (void)wasm_vec_ensure(vtop, NULL);
}

/* give the vector value in vtop a memory home (spill it to a frame
   slot) so that lvalue operations - element subscript in
   particular - can address it; used for vector rvalues such as
   function-call results */
ST_FUNC void gen_vec_home(void)
{
    if (vtop->r & VT_LVAL)
        return;
    if (wasm_vec_sv_slot(vtop) < 0)
        (void)wasm_vec_ensure(vtop, NULL);
    wasm_vec_spill_sv(vtop);
}

/* the return value must end up in slot 0 (gfunc_epilog reads it);
   other values still live on the vstack at a return are dead */
ST_FUNC void gen_vec_ret(void)
{
    int s = wasm_vec_ensure(vtop, NULL);
    if (s != 0) {
        we_local_get(wloc_vec(s));
        we_local_set(wloc_vec(0));
    }
    vtop->r = WREG_VEC + 0;
}

/* opcode for a binary vector operation, or:
   -1 = no such wasm instruction (the caller errors out)
   -2 = i8x16 multiply, which has no wasm instruction and is emitted
        as a widening-multiply composition by gen_opv */
static int wasm_vec_binop_opc(CType *lt, int op)
{
    int es, fl, un;
    wasm_vec_shape(lt, &es, &fl, &un);
    if (op == '&') return 78;  /* v128.and */
    if (op == '|') return 80;  /* v128.or */
    if (op == '^') return 81;  /* v128.xor */
    if (fl) {
        if (es == 4) { /* f32x4 */
            switch (op) {
            case '+': return 228; case '-': return 229;
            case '*': return 230; case '/': case TOK_PDIV: return 231;
            case TOK_EQ: return 65; case TOK_NE: return 66;
            case TOK_LT: return 67; case TOK_GT: return 68;
            case TOK_LE: return 69; case TOK_GE: return 70;
            }
        } else { /* f64x2 */
            switch (op) {
            case '+': return 240; case '-': return 241;
            case '*': return 242; case '/': case TOK_PDIV: return 243;
            case TOK_EQ: return 71; case TOK_NE: return 72;
            case TOK_LT: return 73; case TOK_GT: return 74;
            case TOK_LE: return 75; case TOK_GE: return 76;
            }
        }
        return -1;
    }
    switch (op) {
    case '+':
        return es == 1 ? 110 : es == 2 ? 142 : es == 4 ? 174 : 206;
    case '-':
        return es == 1 ? 113 : es == 2 ? 145 : es == 4 ? 177 : 209;
    case '*':
        if (es == 1) return -2; /* no i8x16.mul in wasm SIMD */
        return es == 2 ? 149 : es == 4 ? 181 : 213;
    case TOK_EQ: return es == 1 ? 35 : es == 2 ? 45 : es == 4 ? 55 : 214;
    case TOK_NE: return es == 1 ? 36 : es == 2 ? 46 : es == 4 ? 56 : 215;
    case TOK_LT: return es == 1 ? 37 : es == 2 ? 47 : es == 4 ? 57 : 216;
    case TOK_ULT: return es == 8 ? -1 : es == 1 ? 38 : es == 2 ? 48 : 58;
    case TOK_GT: return es == 1 ? 39 : es == 2 ? 49 : es == 4 ? 59 : 217;
    case TOK_UGT: return es == 8 ? -1 : es == 1 ? 40 : es == 2 ? 50 : 60;
    case TOK_LE: return es == 1 ? 41 : es == 2 ? 51 : es == 4 ? 61 : 218;
    case TOK_ULE: return es == 8 ? -1 : es == 1 ? 42 : es == 2 ? 52 : 62;
    case TOK_GE: return es == 1 ? 43 : es == 2 ? 53 : es == 4 ? 63 : 219;
    case TOK_UGE: return es == 8 ? -1 : es == 1 ? 44 : es == 2 ? 54 : 64;
    }
    return -1;
}

static int wasm_vec_shift_opc(int esize, int op)
{
    if (op == TOK_SHL)
        return esize == 1 ? 107 : esize == 2 ? 139 : esize == 4 ? 171 : 203;
    if (op == TOK_SAR) /* arithmetic shift right */
        return esize == 1 ? 108 : esize == 2 ? 140 : esize == 4 ? 172 : 204;
    /* TOK_SHR: logical shift right */
    return esize == 1 ? 109 : esize == 2 ? 141 : esize == 4 ? 173 : 205;
}

/* vector binary operation: consumes vtop[-1] and vtop[0], leaves
   the result in vtop[-1]'s slot.  The caller (tccgen) has already
   validated the operand shapes and sets the result type. */
ST_FUNC void gen_opv(int op)
{
    CType *lt = &vtop[-1].type;
    int les, lfl, lun;
    if (op == TOK_NEG) { gen_vec_neg(); return; }
    wasm_vec_shape(lt, &les, &lfl, &lun);
    if (op == TOK_SHL || op == TOK_SAR || op == TOK_SHR) {
        /* uniform shift: the count is a scalar (per-lane counts
           have no wasm instruction) */
        int s, rreg, opc;
        if (vtop[0].type.t & VT_VECTOR)
            tcc_error("wasm: per-lane vector shift amounts have no wasm SIMD "
                      "instruction (the headers provide a scalar implementation)");
        if (lfl)
            tcc_error("wasm: shift of a floating-point vector");
        s = wasm_vec_ensure(&vtop[-1], &vtop[0]);
        rreg = gv(RC_INT);
        opc = wasm_vec_shift_opc(les, op);
        we_local_get(wloc_vec(s));
        we_local_get(wloc_reg(rreg));
        we_v128_op(opc);
        we_local_set(wloc_vec(s));
        vtop--;
        vtop[0].r = WREG_VEC + s;
        return;
    }
    {
        int s1, s2, opc;
        opc = wasm_vec_binop_opc(lt, op);
        if (opc == -1)
            tcc_error("wasm: this vector operation has no wasm SIMD instruction");
        s2 = wasm_vec_ensure(&vtop[0], &vtop[-1]);
        s1 = wasm_vec_ensure(&vtop[-1], &vtop[0]);
        we_local_get(wloc_vec(s1));
        we_local_get(wloc_vec(s2));
        if (opc == -2) {
            /* i8x16.mul does not exist: widen both halves with
               extmul, then shuffle the low bytes back together
               (exact wraparound semantics) */
            static const unsigned char lanes[16] =
                { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 };
            int t1, t2, i;
            t1 = wasm_vec_alloc(&vtop[-1], &vtop[0]);
            wvec_pinned |= (1 << t1);
            we_v128_op(lun ? 158 : 156); /* extmul_low_i8x16 u/s */
            we_local_set(wloc_vec(t1));
            t2 = wasm_vec_alloc(&vtop[-1], &vtop[0]);
            wvec_pinned |= (1 << t2);
            we_local_get(wloc_vec(s1));
            we_local_get(wloc_vec(s2));
            we_v128_op(lun ? 159 : 157); /* extmul_high_i8x16 u/s */
            we_local_set(wloc_vec(t2));
            we_local_get(wloc_vec(t1));
            we_local_get(wloc_vec(t2));
            we_v128_op(13); /* i8x16.shuffle */
            for (i = 0; i < 16; i++) we(lanes[i]);
            wvec_pinned &= ~((1 << t1) | (1 << t2));
        } else {
            we_v128_op(opc);
        }
        we_local_set(wloc_vec(s1));
        vtop--;
        vtop[0].r = WREG_VEC + s1;
    }
}

ST_FUNC void gen_vec_neg(void)
{
    int es, fl, un, s;
    wasm_vec_shape(&vtop->type, &es, &fl, &un);
    s = wasm_vec_ensure(vtop, NULL);
    if (fl) {
        we_local_get(wloc_vec(s));
        we_v128_op(es == 4 ? 225 : 237); /* f32x4.neg / f64x2.neg */
    } else {
        int i;
        we_v128_op(12); /* v128.const 0 */
        for (i = 0; i < 16; i++) we(0);
        we_local_get(wloc_vec(s));
        we_v128_op(es == 1 ? 113 : es == 2 ? 145 : es == 4 ? 177 : 209);
    }
    we_local_set(wloc_vec(s));
    vtop->r = WREG_VEC + s;
}

ST_FUNC void gen_vec_not(void)
{
    int s = wasm_vec_ensure(vtop, NULL);
    we_local_get(wloc_vec(s));
    we_v128_op(77); /* v128.not */
    we_local_set(wloc_vec(s));
    vtop->r = WREG_VEC + s;
}

/* one-operand vector operations reachable through
   __builtin_wasm_vop1(opc, vec, restype).  in_es/in_fl describe the
   operand lanes, out_es/out_fl the result lanes; scalar_out means
   the wasm result is an i32 (bitmask / all_true / any_true). */
typedef struct { short opc, in_es, in_fl, out_es, out_fl, scalar_out; } WVop1;
static const WVop1 wvop1_table[] = {
    { 96, 1, 0, 1, 0, 0 }, { 128, 2, 0, 2, 0, 0 },   /* abs (int) */
    { 160, 4, 0, 4, 0, 0 }, { 192, 8, 0, 8, 0, 0 },
    { 224, 4, 1, 4, 1, 0 }, { 236, 8, 1, 8, 1, 0 },  /* abs (float) */
    { 225, 4, 1, 4, 1, 0 }, { 237, 8, 1, 8, 1, 0 },  /* neg (float) */
    { 227, 4, 1, 4, 1, 0 }, { 239, 8, 1, 8, 1, 0 },  /* sqrt */
    { 103, 4, 1, 4, 1, 0 }, { 104, 4, 1, 4, 1, 0 },  /* f32x4 ceil/floor */
    { 105, 4, 1, 4, 1, 0 }, { 106, 4, 1, 4, 1, 0 },  /* f32x4 trunc/nearest */
    { 116, 8, 1, 8, 1, 0 }, { 117, 8, 1, 8, 1, 0 },  /* f64x2 ceil/floor */
    { 122, 8, 1, 8, 1, 0 }, { 148, 8, 1, 8, 1, 0 },  /* f64x2 trunc/nearest */
    { 98, 1, 0, 1, 0, 0 },                            /* i8x16.popcnt */
    { 77, 0, 0, 0, 0, 0 },                            /* v128.not (any shape) */
    /* extend low/high: the lane width doubles */
    { 135, 1, 0, 2, 0, 0 }, { 136, 1, 0, 2, 0, 0 },
    { 137, 1, 0, 2, 0, 0 }, { 138, 1, 0, 2, 0, 0 },
    { 167, 2, 0, 4, 0, 0 }, { 168, 2, 0, 4, 0, 0 },
    { 169, 2, 0, 4, 0, 0 }, { 170, 2, 0, 4, 0, 0 },
    { 199, 4, 0, 8, 0, 0 }, { 200, 4, 0, 8, 0, 0 },
    { 201, 4, 0, 8, 0, 0 }, { 202, 4, 0, 8, 0, 0 },
    /* conversions between integer and float lanes */
    { 248, 4, 1, 4, 0, 0 }, { 249, 4, 1, 4, 0, 0 },  /* i32x4.trunc_sat_f32x4 s/u */
    { 250, 4, 0, 4, 1, 0 }, { 251, 4, 0, 4, 1, 0 },  /* f32x4.convert_i32x4 s/u */
    { 252, 8, 1, 4, 0, 0 }, { 253, 8, 1, 4, 0, 0 },  /* i32x4.trunc_sat_f64x2 s/u zero */
    { 254, 4, 0, 8, 1, 0 }, { 255, 4, 0, 8, 1, 0 },  /* f64x2.convert_i32x4 s/u (low lanes) */
    { 94, 8, 1, 4, 1, 0 },                            /* f32x4.demote_f64x2_zero */
    { 95, 4, 1, 8, 1, 0 },                            /* f64x2.promote_low_f32x4 */
    /* scalar reductions to an i32 */
    { 100, 1, 0, 0, 0, 1 }, { 132, 2, 0, 0, 0, 1 },  /* bitmask */
    { 164, 4, 0, 0, 0, 1 }, { 196, 8, 0, 0, 0, 1 },
    { 99, 1, 0, 0, 0, 1 }, { 131, 2, 0, 0, 0, 1 },   /* all_true */
    { 163, 4, 0, 0, 0, 1 }, { 195, 8, 0, 0, 0, 1 },
    { 83, 0, 0, 0, 0, 1 },                            /* v128.any_true (any shape) */
};

ST_FUNC void gen_vec_op1(int opc, CType *restype)
{
    const WVop1 *info = NULL;
    int i, es, fl, un, s;
    for (i = 0; i < (int)(sizeof(wvop1_table) / sizeof(wvop1_table[0])); i++)
        if (wvop1_table[i].opc == opc) { info = &wvop1_table[i]; break; }
    if (!info)
        tcc_error("wasm: vop1 opcode %d is not implemented", opc);
    if (!(vtop->type.t & VT_VECTOR))
        tcc_error("wasm: vop1 on a non-vector value");
    wasm_vec_shape(&vtop->type, &es, &fl, &un);
    if (info->in_es && (info->in_es != es || info->in_fl != fl))
        tcc_error("wasm: vop1 opcode %d does not apply to this vector shape", opc);
    if (info->scalar_out) {
        if (restype->t & VT_VECTOR)
            tcc_error("wasm: vop1 opcode %d produces a scalar, not a vector", opc);
    } else {
        int oes, ofl, oun;
        if (!(restype->t & VT_VECTOR) || !wasm_is_vec(restype))
            tcc_error("wasm: vop1 opcode %d needs a 64/128-bit vector result type", opc);
        wasm_vec_shape(restype, &oes, &ofl, &oun);
        if (info->in_es && (info->out_es != oes || info->out_fl != ofl))
            tcc_error("wasm: vop1 opcode %d result shape mismatch", opc);
    }
    s = wasm_vec_ensure(vtop, NULL);
    if (info->scalar_out)
        save_reg_upstack(REG_IRET, 1); /* direct result-register
                                          write: see gen_vec_extract */
    we_local_get(wloc_vec(s));
    we_v128_op(opc);
    if (info->scalar_out) {
        we_local_set(wloc_reg(REG_IRET));
        vtop->r = REG_IRET;
    } else {
        we_local_set(wloc_vec(s));
        vtop->r = WREG_VEC + s;
    }
    vtop->type = *restype;
}

/* two-operand vector operations reachable through
   __builtin_wasm_vop2(opc, a, b, restype).  es/fl describe the
   operand lanes; out_es is the result lane size when it differs
   from the operands' (narrow / extmul / dot), else 0. */
typedef struct { short opc, es, fl, out_es; } WVop2;
static const WVop2 wvop2_table[] = {
    { 111, 1, 0, 0 }, { 112, 1, 0, 0 },   /* i8x16 add_sat s/u */
    { 114, 1, 0, 0 }, { 115, 1, 0, 0 },   /* i8x16 sub_sat s/u */
    { 143, 2, 0, 0 }, { 144, 2, 0, 0 },   /* i16x8 add_sat s/u */
    { 146, 2, 0, 0 }, { 147, 2, 0, 0 },   /* i16x8 sub_sat s/u */
    { 118, 1, 0, 0 }, { 119, 1, 0, 0 },   /* i8x16 min_s/min_u */
    { 120, 1, 0, 0 }, { 121, 1, 0, 0 },   /* i8x16 max_s/max_u */
    { 150, 2, 0, 0 }, { 151, 2, 0, 0 },   /* i16x8 min_s/min_u */
    { 152, 2, 0, 0 }, { 153, 2, 0, 0 },   /* i16x8 max_s/max_u */
    { 182, 4, 0, 0 }, { 183, 4, 0, 0 },   /* i32x4 min_s/min_u */
    { 184, 4, 0, 0 }, { 185, 4, 0, 0 },   /* i32x4 max_s/max_u */
    { 232, 4, 1, 0 }, { 233, 4, 1, 0 },   /* f32x4 min/max */
    { 244, 8, 1, 0 }, { 245, 8, 1, 0 },   /* f64x2 min/max */
    { 234, 4, 1, 0 }, { 235, 4, 1, 0 },   /* f32x4 pmin/pmax */
    { 246, 8, 1, 0 }, { 247, 8, 1, 0 },   /* f64x2 pmin/pmax */
    { 123, 1, 0, 0 }, { 155, 2, 0, 0 },   /* avgr_u */
    { 101, 2, 0, 1 }, { 102, 2, 0, 1 },   /* narrow i16x8 -> i8x16 s/u */
    { 133, 4, 0, 2 }, { 134, 4, 0, 2 },   /* narrow i32x4 -> i16x8 s/u */
    { 14, 1, 0, 0 },                       /* i8x16.swizzle */
    { 156, 1, 0, 2 }, { 157, 1, 0, 2 },   /* extmul low/high i8x16 s */
    { 158, 1, 0, 2 }, { 159, 1, 0, 2 },   /* extmul low/high i8x16 u */
    { 186, 2, 0, 4 },                      /* i32x4.dot_i16x8_s */
};

ST_FUNC void gen_vec_op2(int opc, CType *restype)
{
    const WVop2 *info = NULL;
    int i, es, fl, un, es2, fl2, un2, s1, s2;
    for (i = 0; i < (int)(sizeof(wvop2_table) / sizeof(wvop2_table[0])); i++)
        if (wvop2_table[i].opc == opc) { info = &wvop2_table[i]; break; }
    if (!info)
        tcc_error("wasm: vop2 opcode %d is not implemented", opc);
    if (!(vtop[-1].type.t & VT_VECTOR) || !(vtop[0].type.t & VT_VECTOR))
        tcc_error("wasm: vop2 on a non-vector value");
    wasm_vec_shape(&vtop[-1].type, &es, &fl, &un);
    wasm_vec_shape(&vtop[0].type, &es2, &fl2, &un2);
    if (es != es2 || fl != fl2 || vtop[-1].type.ref->c != vtop[0].type.ref->c)
        tcc_error("wasm: vop2 operand shapes differ");
    if (info->es != es || info->fl != fl)
        tcc_error("wasm: vop2 opcode %d does not apply to this vector shape", opc);
    if (!(restype->t & VT_VECTOR))
        tcc_error("wasm: vop2 result type is not a vector type");
    {
        int oes, ofl, oun;
        wasm_vec_shape(restype, &oes, &ofl, &oun);
        if (info->out_es) {
            if (oes != info->out_es || wasm_vec_bytes(restype) != 16)
                tcc_error("wasm: vop2 opcode %d result shape mismatch", opc);
        } else if (oes != es || ofl != fl || restype->ref->c != vtop[-1].type.ref->c) {
            tcc_error("wasm: vop2 opcode %d result shape mismatch", opc);
        }
    }
    s2 = wasm_vec_ensure(&vtop[0], &vtop[-1]);
    s1 = wasm_vec_ensure(&vtop[-1], &vtop[0]);
    we_local_get(wloc_vec(s1));
    we_local_get(wloc_vec(s2));
    we_v128_op(opc);
    we_local_set(wloc_vec(s1));
    vtop--;
    vtop[0].r = WREG_VEC + s1;
    vtop[0].type = *restype;
}

ST_FUNC void gen_vec_splat(int opc, CType *restype)
{
    int es, fl, un, rreg, s, want_es = 0, want_fl = 0;
    switch (opc) {
    case 15: want_es = 1; want_fl = 0; break;  /* i8x16.splat */
    case 16: want_es = 2; want_fl = 0; break;  /* i16x8.splat */
    case 17: want_es = 4; want_fl = 0; break;  /* i32x4.splat */
    case 18: want_es = 8; want_fl = 0; break;  /* i64x2.splat */
    case 19: want_es = 4; want_fl = 1; break;  /* f32x4.splat */
    case 20: want_es = 8; want_fl = 1; break;  /* f64x2.splat */
    default: tcc_error("wasm: splat opcode %d is not implemented", opc);
    }
    if (!(restype->t & VT_VECTOR) || !wasm_is_vec(restype))
        tcc_error("wasm: splat result is not a 64/128-bit vector type");
    wasm_vec_shape(restype, &es, &fl, &un);
    if (es != want_es || fl != want_fl)
        tcc_error("wasm: splat opcode %d result shape mismatch", opc);
    if (want_es == 8 && !want_fl)
        tcc_error("wasm: i64x2 splat needs a 64-bit scalar (unsupported in this backend)");
    rreg = gv(want_fl ? RC_FLOAT : RC_INT);
    s = wasm_vec_alloc(NULL, NULL);
    we_local_get(wloc_reg(rreg));
    if (want_fl && want_es == 4)
        we(0xb6); /* f32.demote_f64: float registers hold f64,
                     f32x4.splat pops an f32 */
    we_v128_op(opc);
    we_local_set(wloc_vec(s));
    vtop->r = WREG_VEC + s;
    vtop->type = *restype;
}

ST_FUNC void gen_vec_extract(int opc, int lane, CType *restype)
{
    int es, fl, un, s, rreg, want_es = 0, want_fl = 0;
    switch (opc) {
    case 21: case 22: want_es = 1; want_fl = 0; break;  /* i8x16 extract s/u */
    case 24: case 25: want_es = 2; want_fl = 0; break;  /* i16x8 extract s/u */
    case 27: want_es = 4; want_fl = 0; break;           /* i32x4 extract */
    case 29: want_es = 8; want_fl = 0; break;           /* i64x2 extract */
    case 31: want_es = 4; want_fl = 1; break;           /* f32x4 extract */
    case 33: want_es = 8; want_fl = 1; break;           /* f64x2 extract */
    default: tcc_error("wasm: extract opcode %d is not implemented", opc);
    }
    if (!(vtop->type.t & VT_VECTOR))
        tcc_error("wasm: extract on a non-vector value");
    wasm_vec_shape(&vtop->type, &es, &fl, &un);
    if (es != want_es || fl != want_fl)
        tcc_error("wasm: extract opcode %d does not apply to this vector shape", opc);
    if (lane < 0 || lane >= vtop->type.ref->c)
        tcc_error("wasm: extract lane %d out of range", lane);
    if (want_es == 8 && !want_fl)
        tcc_error("wasm: i64 lane extract needs a 64-bit register (unsupported in this backend)");
    rreg = want_fl ? REG_FRET : REG_IRET;
    /* The result is written straight into rreg, bypassing the
       register discipline gv() normally enforces: spill any other
       vstack value still living in rreg first (e.g. the left
       operand of an enclosing binary expression, as in
       extract(a) + extract(b)), or it would be silently overwritten. */
    save_reg_upstack(rreg, 1);
    s = wasm_vec_ensure(vtop, NULL);
    we_local_get(wloc_vec(s));
    we_v128_op(opc);
    we(lane);
    if (want_fl && want_es == 4)
        we(0xbb); /* f64.promote_f32: float registers hold f64,
                     f32x4.extract_lane pushes an f32 */
    we_local_set(wloc_reg(rreg));
    vtop->r = rreg;
    vtop->type = *restype;
}

ST_FUNC void gen_vec_replace(int opc, int lane)
{
    int es, fl, un, s, rreg, want_es = 0, want_fl = 0;
    switch (opc) {
    case 23: want_es = 1; want_fl = 0; break;  /* i8x16 replace */
    case 26: want_es = 2; want_fl = 0; break;  /* i16x8 replace */
    case 28: want_es = 4; want_fl = 0; break;  /* i32x4 replace */
    case 30: want_es = 8; want_fl = 0; break;  /* i64x2 replace */
    case 32: want_es = 4; want_fl = 1; break;  /* f32x4 replace */
    case 34: want_es = 8; want_fl = 1; break;  /* f64x2 replace */
    default: tcc_error("wasm: replace opcode %d is not implemented", opc);
    }
    if (!(vtop[-1].type.t & VT_VECTOR))
        tcc_error("wasm: replace on a non-vector value");
    wasm_vec_shape(&vtop[-1].type, &es, &fl, &un);
    if (es != want_es || fl != want_fl)
        tcc_error("wasm: replace opcode %d does not apply to this vector shape", opc);
    if (lane < 0 || lane >= vtop[-1].type.ref->c)
        tcc_error("wasm: replace lane %d out of range", lane);
    if (want_es == 8 && !want_fl)
        tcc_error("wasm: i64 lane replace needs a 64-bit scalar (unsupported in this backend)");
    s = wasm_vec_ensure(&vtop[-1], &vtop[0]);
    rreg = gv(fl ? RC_FLOAT : RC_INT);
    we_local_get(wloc_vec(s));
    we_local_get(wloc_reg(rreg));
    if (fl && es == 4)
        we(0xb6); /* f32.demote_f64: float registers hold f64,
                     f32x4.replace_lane pops an f32 */
    we_v128_op(opc);
    we(lane);
    we_local_set(wloc_vec(s));
    vtop--;
    vtop[0].r = WREG_VEC + s;
}

ST_FUNC void gen_vec_shuffle(const int *lanes)
{
    int i, s1, s2;
    for (i = 0; i < 16; i++)
        if (lanes[i] < 0 || lanes[i] >= 32)
            tcc_error("wasm: shuffle lane %d out of range", lanes[i]);
    if (!(vtop[-1].type.t & VT_VECTOR) || !(vtop[0].type.t & VT_VECTOR))
        tcc_error("wasm: shuffle on a non-vector value");
    s2 = wasm_vec_ensure(&vtop[0], &vtop[-1]);
    s1 = wasm_vec_ensure(&vtop[-1], &vtop[0]);
    we_local_get(wloc_vec(s1));
    we_local_get(wloc_vec(s2));
    we_v128_op(13); /* i8x16.shuffle */
    for (i = 0; i < 16; i++) we(lanes[i]);
    we_local_set(wloc_vec(s1));
    vtop--;
    vtop[0].r = WREG_VEC + s1;
}



static int wasm_load_op(CType *type, int *is_float)
{
    int align, size = type_size(type, &align);
    int bt = type->t & VT_BTYPE;
    *is_float = 0;
    if (bt == VT_PTR || bt == VT_FUNC) size = 4;
    switch (size) {
    case 1: return (type->t & VT_UNSIGNED) ? 0x2d : 0x2c; /* i32.load8_u/s */
    case 2: return (type->t & VT_UNSIGNED) ? 0x2f : 0x2e; /* i32.load16_u/s */
    case 4:
        if (bt == VT_FLOAT) { *is_float = 1; return 0x2a; } /* f32.load */
        return 0x28; /* i32.load */
    case 8:
        if (bt == VT_DOUBLE || bt == VT_LDOUBLE) { *is_float = 1; return 0x2b; } /* f64.load */
        /* a 64-bit integer reaches load() one word at a time: the
           generic layer splits register pairs into PTRDIFF_T words
           itself, and save/load of a spilled pair member passes the
           full type with a single register */
        return 0x28; /* i32.load */
    }
    tcc_error("wasm: unsupported load size %d", size);
    return 0;
}

static int wasm_store_op(CType *type, int *is_float)
{
    int align, size = type_size(type, &align);
    int bt = type->t & VT_BTYPE;
    *is_float = 0;
    if (bt == VT_PTR || bt == VT_FUNC) size = 4;
    switch (size) {
    case 1: return 0x3a; /* i32.store8 */
    case 2: return 0x3b; /* i32.store16 */
    case 4:
        if (bt == VT_FLOAT) { *is_float = 1; return 0x38; } /* f32.store */
        return 0x36; /* i32.store */
    case 8:
        if (bt == VT_DOUBLE || bt == VT_LDOUBLE) { *is_float = 1; return 0x39; } /* f64.store */
        /* single word of a 64-bit integer pair, as in load() above:
           save_reg_upstack stores a spilled pair member this way and
           issues the second word's store itself */
        return 0x36; /* i32.store */
    }
    tcc_error("wasm: unsupported store size %d", size);
    return 0;
}

ST_FUNC void load(int r, SValue *sv)
{
    int fr = sv->r, v = fr & VT_VALMASK;
    int is_float;
    if (sv->type.t & VT_VECTOR) {
        int bytes = wasm_vec_bytes(&sv->type);
        if (bytes != 8 && bytes != 16)
            tcc_error("wasm: a vector of %d bytes cannot be loaded by the wasm "
                      "backend (only 64/128-bit vectors)", bytes);
        if (r < WREG_VEC || r >= WREG_VEC + 4)
            tcc_error("wasm: vector value not allocated a vector slot");
        if (fr & VT_LVAL) {
            wasm_push_addr(sv);
            if (bytes == 16) {
                we_v128_op(0); /* v128.load */
                we(0x00); we(0x00); /* memarg */
            } else {
                we_v128_op(93); /* v128.load64_zero */
                we(0x00); we(0x00); /* memarg */
            }
        } else if (v >= WREG_VEC && v < WREG_VEC + 4) {
            if (v != r) { we_local_get(wloc_vec(v - WREG_VEC)); }
            else return;
        } else {
            tcc_error("wasm: vector load of this value kind not implemented");
        }
        we_local_set(wloc_vec(r - WREG_VEC));
        return;
    }
    if (fr & VT_LVAL) {
        int op = wasm_load_op(&sv->type, &is_float);
        wasm_push_addr(sv);
        we(op); we(0x00); we(0x00); /* memarg: align 0, offset 0 */
        if ((sv->type.t & VT_BTYPE) == VT_FLOAT)
            we(0xbb); /* f64.promote_f32: float registers hold f64 */
        we_local_set(wloc_reg(r));
        return;
    }
    switch (v) {
    case VT_CONST:
        if (fr & VT_SYM) {
            int pos;
            we(0x41); pos = wcur.len; we_sleb5(0);
            wasm_add_fix(wcur_func, WFIX_ADDR, pos, sv->sym, (int)sv->c.i);
            we_local_set(wloc_reg(r));
            return;
        }
        switch (sv->type.t & VT_BTYPE) {
        case VT_FLOAT: {
            float f = sv->c.f; uint32_t u; memcpy(&u, &f, 4);
            we(0x43); we(u & 0xff); we((u >> 8) & 0xff); we((u >> 16) & 0xff); we((u >> 24) & 0xff);
            break; }
        case VT_DOUBLE: case VT_LDOUBLE: {
            double d = sv->c.d; uint64_t u; int i; memcpy(&u, &d, 8);
            we(0x44); for (i = 0; i < 8; i++) we((u >> (8 * i)) & 0xff);
            break; }
        case VT_LLONG:
            /* gv()'s register-pair protocol loads a 64-bit constant
               as two 32-bit words: this call loads the low word of
               c.i; gv then pushes the high word as a separate int
               constant and loads it into the second register */
            we_i32_const((int32_t)sv->c.i);
            break;
        default:
            we_i32_const((int32_t)sv->c.i);
            break;
        }
        if ((sv->type.t & VT_BTYPE) == VT_FLOAT)
            we(0xbb); /* f64.promote_f32: float registers hold f64 */
        we_local_set(wloc_reg(r));
        return;
    case VT_LOCAL: /* the value is the local's own address */
        we_local_get(wloc_fp());
        if (sv->c.i) { we_i32_const((int32_t)sv->c.i); we(0x6a); }
        we_local_set(wloc_reg(r));
        return;
    case VT_LLOCAL:
        we_local_get(wloc_fp());
        if (sv->c.i) { we_i32_const((int32_t)sv->c.i); we(0x6a); }
        we(0x28); we(0x02); we(0x00);
        we_local_set(wloc_reg(r));
        return;
    case VT_CMP:
        /* materialise the comparison described by cmp_op/cmp_r */
        if (sv->cmp_op < 2)
            we_i32_const(sv->cmp_op); /* folded constant test */
        else
            wasm_cmp_emit(sv);
        we_local_set(wloc_reg(r));
        return;
    default:
        if ((v & ~1) == VT_JMP) {
            /* a conditional value with pending jump chains (from
               &&/||): fall through with one truth value, resolve the
               chain to the other assignment, hop over it (the same
               shape as riscv64-gen.c's load of VT_JMP) */
            int t = v & 1, hop;
            we_i32_const(t);
            we_local_set(wloc_reg(r));
            hop = wasm_jmp_record(ind, 0, 0, -1);
            wasm_jmp_placeholders();
            gsym(sv->c.i); /* chain lands on the second assignment */
            we_i32_const(t ^ 1);
            we_local_set(wloc_reg(r));
            wjmps[hop].target = ind;
            return;
        }
        if (v < VT_CONST) { /* register to register */
            if (r != v) { we_local_get(wloc_reg(v)); we_local_set(wloc_reg(r)); }
            return;
        }
        tcc_error("wasm: unsupported load kind 0x%x", v);
    }
}

ST_FUNC void store(int r, SValue *sv)
{
    int is_float;
    int op;
    if (!(sv->r & VT_LVAL))
        tcc_error("wasm: store to a non-lvalue");
    if (sv->type.t & VT_VECTOR) {
        int bytes = wasm_vec_bytes(&sv->type);
        if (bytes != 8 && bytes != 16)
            tcc_error("wasm: a vector of %d bytes cannot be stored by the wasm "
                      "backend (only 64/128-bit vectors)", bytes);
        if (r < WREG_VEC || r >= WREG_VEC + 4)
            tcc_error("wasm: vector store from a non-vector slot");
        wasm_push_addr(sv);
        we_local_get(wloc_vec(r - WREG_VEC));
        if (bytes == 16) {
            we_v128_op(11); /* v128.store */
            we(0x00); we(0x00);
        } else {
            we_v128_op(91); /* v128.store64_lane */
            we(0x00); we(0x00); /* memarg */
            we(0x00); /* lane 0 */
        }
        return;
    }
    op = wasm_store_op(&sv->type, &is_float);
    wasm_push_addr(sv);
    we_local_get(wloc_reg(r));
    if ((sv->type.t & VT_BTYPE) == VT_FLOAT)
        we(0xb6); /* f32.demote_f64: float registers hold f64 */
    we(op); we(0x00); we(0x00);
}

/* ------------------------------------------------------------------ */
/* jumps / labels: basic-block dispatch (see the WJmp comment above)   */

/* Jump chains follow the generic tccgen protocol (tccgen.c:173):
   gsym(t) calls gsym_addr(t, ind); the chain is threaded through
   32-bit link words stored in the *text section mirror* at the
   offsets returned by gjmp/gjmp_cond.  gsym_addr walks the chain
   and records the resolution target on every site in it; the
   epilog's dispatch assembly turns each site into a real branch.
   (gsym itself is provided by tccgen.c and must not be redefined
   here.) */
ST_FUNC void gsym_addr(int t, int a)
{
    while (t) {
        unsigned char *ptr = cur_text_section->data + t;
        uint32_t next = read32le(ptr);
        int i;
        for (i = 0; i < nb_wjmps; i++)
            if (wjmps[i].node == t) { wjmps[i].target = a; break; }
        t = (int)next;
    }
}

ST_FUNC int gjmp(int t)
{
    /* Unconditional jump: a 5-byte placeholder site, replaced at
       epilog by "pc = target block; br $dispatch".  The chain link
       word overwrites mirror bytes at site+1..site+4, which exist
       only in the mirror; the body keeps the placeholder bytes. */
    int site;
    if (nocode_wanted) return t;
    site = ind;
    wasm_jmp_record(site, site + 1, 0, -1);
    wasm_jmp_placeholders();
    write32le(cur_text_section->data + site + 1, (uint32_t)t);
    return site + 1;
}

ST_FUNC void gjmp_addr(int a)
{
    /* Jump to an already-emitted address (loop back-edges, defined
       labels): the target is known here, no chain is involved. */
    int site = ind;
    wasm_jmp_record(site, 0, 0, a);
    wasm_jmp_placeholders();
}

ST_FUNC int gjmp_cond(int op, int t)
{
    /* Conditional jump on the VT_CMP value in vtop: emit the
       comparison, and inside an `if` a placeholder site that the
       epilog turns into the dispatch branch taken when the
       comparison (op, already inverted by the caller as needed)
       is true.  False falls through to the next stream position. */
    int site, a, b;
    /* the operands come from vtop's cmp_r, the condition from the
       'op' argument (gvtst passes cmp_op ^ inv) */
    wasm_cmp_operands(vtop, &a, &b);
    wasm_cmp_operand(a);
    wasm_cmp_operand(b);
    we(wasm_icmp_opc(op));
    we(0x04); we(0x40); /* if, void block type */
    site = ind;
    wasm_jmp_record(site, site + 1, 1, -1);
    wasm_jmp_placeholders();
    write32le(cur_text_section->data + site + 1, (uint32_t)t);
    we(0x0b); /* end if */
    return site + 1;
}

ST_FUNC int gjmp_append(int n, int t)
{
    /* insert jump list n into t (chain merge through mirror words,
       same shape as riscv64-gen.c) */
    if (n) {
        uint32_t n1 = n, n2;
        while ((n2 = read32le(cur_text_section->data + n1)))
            n1 = n2;
        write32le(cur_text_section->data + n1, t);
        t = n;
    }
    return t;
}

ST_FUNC void ggoto(void)
{
    tcc_error("wasm: goto not implemented in skeleton");
}

/* ------------------------------------------------------------------ */
/* operators                                                           */

ST_FUNC void gen_opi(int op)
{
    int a, b, d;
    int opc = -1;
    int uns = (vtop[-1].type.t & VT_UNSIGNED) != 0;
    if ((vtop[-1].type.t & VT_VECTOR) || (vtop[0].type.t & VT_VECTOR)) {
        /* safety net: tccgen routes vector operations through
           gen_op_vector/gen_opv; anything reaching here directly
           takes the same path */
        gen_opv(op);
        return;
    }
    /* contract (mirrors riscv64-gen.c gen_opil general path):
       gv both operands, pop two, push one result in a fresh reg.
       A constant operand is simply loaded into a register by gv -
       wasm has no separate immediate form to exploit here.
       Comparisons follow the generic protocol: the operands stay
       in their registers, the result is a VT_CMP SValue describing
       them (materialised by load(), branched on by gjmp_cond). */
    gv2(RC_INT, RC_INT);
    a = vtop[-1].r & VT_VALMASK;
    b = vtop[0].r & VT_VALMASK;
    if (op == TOK_ADDC1 || op == TOK_ADDC2 ||
        op == TOK_SUBC1 || op == TOK_SUBC2) {
        /* the frontend's long long add/sub (gen_opl) works on word
           pairs through a carry: C1 produces it, C2 consumes it.
           Wasm has no flags register, so the carry lives in its own
           local between the two calls (nothing else is emitted in
           between). */
        int with_carry = op == TOK_ADDC2 || op == TOK_SUBC2;
        int is_sub = op == TOK_SUBC1 || op == TOK_SUBC2;
        vtop -= 2;
        d = get_reg(RC_INT);
        vtop++;
        vtop[0].r = d;
        if (!with_carry) {
            /* the carry out depends only on the operands; compute
               it before the result store, which may overwrite an
               operand register (d can be a's or b's register):
               add: a + b overflows iff a > ~b; sub: borrow iff a < b */
            we_local_get(wloc_reg(a));
            if (is_sub) {
                we_local_get(wloc_reg(b));
                we(0x49); /* i32.lt_u */
            } else {
                we_local_get(wloc_reg(b));
                we_i32_const(-1);
                we(0x73); /* i32.xor: ~b */
                we(0x4b); /* i32.gt_u */
            }
            we_local_set(wloc_carry());
        }
        we_local_get(wloc_reg(a));
        we_local_get(wloc_reg(b));
        we(is_sub ? 0x6b : 0x6a); /* i32.sub / i32.add */
        if (with_carry) {
            we_local_get(wloc_carry());
            we(is_sub ? 0x6b : 0x6a);
        }
        we_local_set(wloc_reg(d));
        return;
    }
    if (op == TOK_UMULL) {
        /* unsigned 32x32 -> 64 multiply (gen_opl): result pair in
           r (low) / r2 (high), computed through the i64 scratch */
        int d2;
        vtop -= 2;
        d = get_reg(RC_INT);
        vtop++;
        vtop[0].r = d;
        d2 = get_reg(RC_INT);
        vtop[0].r2 = d2;
        we_local_get(wloc_reg(a));
        we(0xad); /* i64.extend_i32_u */
        we_local_get(wloc_reg(b));
        we(0xad); /* i64.extend_i32_u */
        we(0x7e); /* i64.mul */
        we_local_set(wloc_i64());
        we_local_get(wloc_i64());
        we(0xa7); /* i32.wrap_i64 */
        we_local_set(wloc_reg(d));
        we_local_get(wloc_i64());
        we_i64_const(32);
        we(0x88); /* i64.shr_u */
        we(0xa7); /* i32.wrap_i64 */
        we_local_set(wloc_reg(d2));
        return;
    }
    if (op >= TOK_ULT && op <= TOK_GT) {
        vtop -= 2;
        vtop++;
        vset_VT_CMP(op);
        vtop->cmp_r = a | b << 8;
        amb_cmp_a = a; amb_cmp_b = b; amb_cmp_valid = 1;
        return;
    }
    vtop -= 2;
    d = get_reg(RC_INT);
    vtop++;
    vtop[0].r = d;
    switch (op) {
    case '+': opc = 0x6a; break; /* i32.add */
    case '-': opc = 0x6b; break; /* i32.sub */
    case '*': opc = 0x6c; break; /* i32.mul */
    case '/': opc = uns ? 0x6e : 0x6d; break; /* div_u / div_s */
    case TOK_UDIV: opc = 0x6e; break;
    case TOK_PDIV: opc = 0x6d; break;
    case '%': opc = uns ? 0x70 : 0x6f; break; /* rem_u / rem_s */
    case TOK_UMOD: opc = 0x70; break;
    case '&': opc = 0x71; break;
    case '|': opc = 0x72; break;
    case '^': opc = 0x73; break;
    case TOK_SHL: opc = 0x74; break; /* shl */
    case TOK_SAR: opc = 0x75; break; /* shr_s */
    case TOK_SHR: opc = 0x76; break; /* shr_u */
    default:
        tcc_error("wasm: gen_opi operator 0x%x not implemented in skeleton", op);
    }
    we_local_get(wloc_reg(a));
    we_local_get(wloc_reg(b));
    we(opc);
    we_local_set(wloc_reg(d));
}

ST_FUNC void gen_opf(int op)
{
    int a, b, d, bt, opc;
    if ((vtop[-1].type.t & VT_VECTOR) || (vtop[0].type.t & VT_VECTOR)) {
        gen_opv(op);
        return;
    }
    /* both operands were cast to the common type by gen_op, so
       vtop's type is the operation's float type; float registers
       hold f64, with float (f32) values promoted into them */
    bt = vtop[0].type.t & VT_BTYPE;
    gv2(RC_FLOAT, RC_FLOAT);
    a = vtop[-1].r & VT_VALMASK;
    b = vtop[0].r & VT_VALMASK;
    if (op >= TOK_ULT && op <= TOK_GT) {
        /* float comparison: compute the 0/1 result into an int
           register, then describe it as VT_CMP against zero (the
           riscv64-gen.c shape), so load()/gjmp_cond only ever see
           integer comparisons */
        switch (op) {
        case TOK_EQ: opc = 0x61; break; /* f64.eq */
        case TOK_NE: opc = 0x62; break; /* f64.ne */
        case TOK_LT: opc = 0x63; break; /* f64.lt */
        case TOK_GT: opc = 0x64; break; /* f64.gt */
        case TOK_LE: opc = 0x65; break; /* f64.le */
        case TOK_GE: opc = 0x66; break; /* f64.ge */
        default:
            tcc_error("wasm: float comparison operator 0x%x not implemented", op);
            return;
        }
        vtop -= 2;
        d = get_reg(RC_INT);
        vtop++;
        we_local_get(wloc_reg(a));
        we_local_get(wloc_reg(b));
        we(opc);
        we_local_set(wloc_reg(d));
        vset_VT_CMP(TOK_NE);
        vtop->cmp_r = d | WCMP_ZERO << 8;
        return;
    }
    switch (op) {
    case '+': opc = bt == VT_FLOAT ? 0x92 : 0xa0; break; /* f32/f64.add */
    case '-': opc = bt == VT_FLOAT ? 0x93 : 0xa1; break; /* f32/f64.sub */
    case '*': opc = bt == VT_FLOAT ? 0x94 : 0xa2; break; /* f32/f64.mul */
    case '/': case TOK_PDIV:
              opc = bt == VT_FLOAT ? 0x95 : 0xa3; break; /* f32/f64.div */
    default:
        tcc_error("wasm: floating-point operator 0x%x not implemented", op);
        return;
    }
    vtop -= 2;
    d = get_reg(RC_FLOAT);
    vtop++;
    vtop[0].r = d;
    we_local_get(wloc_reg(a));
    if (bt == VT_FLOAT) we(0xb6); /* f32.demote_f64 */
    we_local_get(wloc_reg(b));
    if (bt == VT_FLOAT) we(0xb6);
    we(opc);
    if (bt == VT_FLOAT) we(0xbb); /* f64.promote_f32 */
    we_local_set(wloc_reg(d));
}

/* combine the register pair in vtop (r = low word, r2 = high word)
   into the i64 scratch local: the high word's bits land in bits
   32..63 either way, so one shape serves signed and unsigned */
static void wasm_pair_to_i64(void)
{
    we_local_get(wloc_reg(vtop->r2 & VT_VALMASK));
    we(0xac); /* i64.extend_i32_s */
    we_i64_const(32);
    we(0x86); /* i64.shl */
    we_local_get(wloc_reg(vtop->r & VT_VALMASK));
    we(0xad); /* i64.extend_i32_u */
    we(0x84); /* i64.or */
    we_local_set(wloc_i64());
}

ST_FUNC void gen_cvt_itof(int t)
{
    int u = vtop->type.t & VT_UNSIGNED;
    int l = (vtop->type.t & VT_BTYPE) == VT_LLONG;
    int rr, dr;
    if (l) {
        /* 64-bit source: a register pair; combine it in the i64
           scratch local and convert from there */
        gv(RC_INT);
        wasm_pair_to_i64();
        vtop--;
        dr = get_reg(RC_FLOAT);
        vtop++;
        vtop->r = dr;
        we_local_get(wloc_i64());
        if (t == VT_FLOAT) {
            we(u ? 0xb5 : 0xb4); /* f32.convert_i64_u/s */
            we(0xbb); /* f64.promote_f32: float registers hold f64 */
        } else
            we(u ? 0xba : 0xb9); /* f64.convert_i64_u/s */
        we_local_set(wloc_reg(dr));
        return;
    }
    rr = gv(RC_INT);
    vtop--;
    dr = get_reg(RC_FLOAT);
    vtop++;
    vtop->r = dr;
    we_local_get(wloc_reg(rr));
    if (t == VT_FLOAT) {
        we(u ? 0xb3 : 0xb2); /* f32.convert_i32_u/s */
        we(0xbb); /* f64.promote_f32: float registers hold f64 */
    } else
        we(u ? 0xb8 : 0xb7); /* f64.convert_i32_u/s */
    we_local_set(wloc_reg(dr));
}

ST_FUNC void gen_cvt_ftoi(int t)
{
    int l = (t & VT_BTYPE) == VT_LLONG;
    int u = t & VT_UNSIGNED;
    int rr, dr;
    rr = gv(RC_FLOAT);
    if (l) {
        /* 64-bit result: truncate into the i64 scratch local, then
           split it into the register pair.  trunc_sat (not the
           trapping trunc) keeps out-of-range conversions defined:
           they saturate, the same choice the SSE headers document. */
        int r2;
        we_local_get(wloc_reg(rr));
        we(0xfc); we_uleb(u ? 7 : 6); /* i64.trunc_sat_f64_u/s */
        we_local_set(wloc_i64());
        vtop--;
        dr = get_reg(RC_INT);
        vtop++;
        vtop->r = dr; /* on the stack before r2 is allocated, so
                         get_reg cannot hand out dr twice */
        r2 = get_reg(RC_INT);
        vtop->r2 = r2;
        we_local_get(wloc_i64());
        we(0xa7); /* i32.wrap_i64 */
        we_local_set(wloc_reg(dr));
        we_local_get(wloc_i64());
        we_i64_const(32);
        we(u ? 0x88 : 0x87); /* i64.shr_u/s */
        we(0xa7); /* i32.wrap_i64 */
        we_local_set(wloc_reg(r2));
        return;
    }
    vtop--;
    dr = get_reg(RC_INT);
    vtop++;
    vtop->r = dr;
    we_local_get(wloc_reg(rr));
    we(0xfc); we_uleb(u ? 3 : 2); /* i32.trunc_sat_f64_u/s */
    we_local_set(wloc_reg(dr));
}

ST_FUNC void gen_cvt_ftof(int t)
{
    int st = vtop->type.t & VT_BTYPE;
    int dt = t & VT_BTYPE;
    int rs, rd;
    if (st == dt)
        return;
    rs = gv(RC_FLOAT);
    if (dt == VT_DOUBLE) {
        /* the register already holds the exact value as an f64
           (float values are kept promoted) */
        vtop->r = rs;
        return;
    }
    /* double -> float: round to f32 precision, keep in f64 form */
    vtop--;
    rd = get_reg(RC_FLOAT);
    vtop++;
    vtop->r = rd;
    we_local_get(wloc_reg(rs));
    we(0xb6); /* f32.demote_f64 */
    we(0xbb); /* f64.promote_f32 */
    we_local_set(wloc_reg(rd));
}

ST_FUNC void gen_cvt_sxtw(void)
{
    /* sign-extend vtop's 32-bit register into a register pair
       (r2 = sign bits).  On this 32-bit target the frontend's
       int -> long long path builds the pair itself (gv_dup + SAR),
       so this hook is not reached; it is defined in pair terms for
       completeness. */
    int r, r2;
    r = gv(RC_INT);
    r2 = get_reg(RC_INT);
    we_local_get(wloc_reg(r));
    we_i32_const(31);
    we(0x75); /* i32.shr_s */
    we_local_set(wloc_reg(r2));
    vtop->r2 = r2;
}

/* sign/zero extend the value in vtop's register to type t (real impl:
   char/short/int narrowing is common even in integer-only code) */
ST_FUNC void gen_cvt_csti(int t)
{
    int r = vtop->r & VT_VALMASK;
    int bits;
    if (r >= VT_CONST) return; /* constant folding already handled by tccgen */
    switch (t & VT_BTYPE) {
    case VT_BYTE:  bits = 8; break;
    case VT_SHORT: bits = 16; break;
    case VT_INT: case VT_PTR: return;
    default:
        tcc_error("wasm: gen_cvt_csti to type 0x%x not implemented", t);
        return;
    }
    we_local_get(wloc_reg(r));
    if (t & VT_UNSIGNED) {
        we_i32_const((bits == 8) ? 0xff : 0xffff);
        we(0x71); /* i32.and */
    } else {
        we_i32_const(32 - bits); we(0x74); /* shl */
        we_i32_const(32 - bits); we(0x75); /* shr_s */
    }
    we_local_set(wloc_reg(r));
}

/* ------------------------------------------------------------------ */
/* function calls                                                      */

ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret,
                       int *ret_align, int *regsize)
{
    int align, size = type_size(vt, &align);
    (void)variadic;
    *ret_align = 1;
    if (size <= 4) { *regsize = 4; ret->t = VT_INT; ret->ref = NULL; return 1; }
    if (size <= 8) { *regsize = 4; ret->t = VT_INT; ret->ref = NULL; return 2; }
    return 0; /* returned via hidden pointer; prolog rejects >8-byte
                 struct returns for the skeleton */
}

ST_FUNC void gfunc_call(int nb_args)
{
    SValue *func_sv = &vtop[-nb_args];
    WFunc *callee;
    int i;
    if (!(func_sv->r & VT_SYM))
        tcc_error("wasm: indirect calls (function pointers) not implemented "
                  "in skeleton; call_indirect table is emitted by wasm-link "
                  "but gen does not emit it yet");
    if ((func_sv->sym->type.t & VT_INLINE) && !func_sv->sym->c) {
        /* A static inline callee has no out-of-line body unless it is
           marked used.  Native backends mark it in greloca() when
           they emit the call relocation, and gen_inline_functions()
           then emits the body at the end of the translation unit.
           This backend records calls by name instead of emitting
           relocations, so mark the callee here; without this, calls
           to header-defined static inline functions (the SIMD
           headers are all static inline) would stay unresolved and
           turn into imports. */
        put_extern_sym(func_sv->sym, NULL, 0, 0);
    }
    if (func_sv->type.ref && func_sv->type.ref->f.func_type == FUNC_OLD) {
        /* Undeclared (K&R) callee: the frontend's helper functions
           (memset, memcpy, ... reached through vpush_helper_func)
           carry func_old_type, whose parameter list is empty, so a
           signature built from the declared type would consume none
           of the arguments pushed below.  The only signature
           information available is this call site: synthesize the
           signature from the actual arguments. */
        WSig sig;
        int i2;
        if (nb_args > 16)
            tcc_error("wasm: too many arguments in call to undeclared "
                      "function '%s'", get_tok_str(func_sv->sym->v, NULL));
        sig.nparams = 0;
        sig.ret = wasm_type_of(&func_sv->type.ref->type);
        if (sig.ret == -2)
            tcc_error("wasm: unsupported return type in call to "
                      "undeclared function '%s'",
                      get_tok_str(func_sv->sym->v, NULL));
        for (i2 = 0; i2 < nb_args; i2++) {
            int wt = wasm_type_of(&vtop[1 + i2 - nb_args].type);
            if (wt < 0)
                tcc_error("wasm: unsupported argument type in call to "
                          "undeclared function '%s'",
                          get_tok_str(func_sv->sym->v, NULL));
            sig.params[sig.nparams++] = wt;
        }
        callee = wasm_func_intern_sig(func_sv->sym, &sig, 1);
    } else
        callee = wasm_func_intern(func_sv->sym, &func_sv->type);
    if (!callee->defined) callee->is_import = 1;
    if (nb_args > 40)
        tcc_error("wasm: more than 40 call arguments not implemented in skeleton");
    /* Spill every vstack value below the function+args group that
       currently lives in a register (upstream mechanism, tccgen.c
       save_regs: it spares the top nb_args+1 entries).  Wasm locals
       are per-function, so a callee cannot clobber them - but this
       function's own return-value store to REG_IRET/REG_FRET can:
       a live intermediate sitting in reg 0 would otherwise be
       overwritten by the call result.  Spilled values get memory
       homes and are reloaded by gv() on next use. */
    /* Vector values below the call group are not covered by
       save_regs (it only knows scalar registers): a vector return
       value lands in slot 0, so spill any slot resident below the
       group to its memory home first. */
    {
        SValue *p;
        for (p = vstack; p <= vtop - (nb_args + 1); p++)
            if (p->type.t & VT_VECTOR)
                wasm_vec_spill_sv(p);
    }
    save_regs(nb_args + 1);
    /* Stage arguments: materialise each into a scratch local, then
       push them in order onto the wasm operand stack.  load() takes a
       TCC register number r and writes local wloc_reg(r)=nparams+r;
       passing r = NB_REGS+1+i therefore lands on scratch local i.
       Vector arguments skip the scalar scratch locals: they are
       materialised into a v128 slot and pushed from there (pushing
       in argument order, interleaved with the scalar gets, keeps the
       operand stack order correct). */
    for (i = 0; i < nb_args; i++) {
        SValue *sv = &vtop[1 + i - nb_args];
        if (sv->type.t & VT_VECTOR) {
            int s = wasm_vec_ensure(sv, NULL);
            we_local_get(wloc_vec(s));
        } else if ((sv->type.t & VT_BTYPE) == VT_LLONG) {
            /* the wasm signature takes one i64 operand; TCC holds
               the value as a constant or a register pair - combine
               it on the operand stack, low word first */
            if ((sv->r & (VT_VALMASK | VT_LVAL)) == VT_CONST) {
                we_i64_const(sv->c.i);
            } else if (sv->r & VT_LVAL) {
                wasm_push_addr(sv);
                we(0x28); we(0x00); we(0x00); /* i32.load (low word) */
                we(0xad); /* i64.extend_i32_u */
                wasm_push_addr(sv);
                we_i32_const(4); we(0x6a); /* i32.add */
                we(0x28); we(0x00); we(0x00); /* i32.load (high word) */
                we(0xac); /* i64.extend_i32_s */
                we_i64_const(32); we(0x86); /* i64.shl */
                we(0x84); /* i64.or */
            } else {
                we_local_get(wloc_reg(sv->r & VT_VALMASK));
                we(0xad); /* i64.extend_i32_u */
                we_local_get(wloc_reg(sv->r2 & VT_VALMASK));
                we(0xac); /* i64.extend_i32_s */
                we_i64_const(32); we(0x86); /* i64.shl */
                we(0x84); /* i64.or */
            }
        } else if ((sv->type.t & VT_BTYPE) == VT_FLOAT ||
                   (sv->type.t & VT_BTYPE) == VT_DOUBLE ||
                   (sv->type.t & VT_BTYPE) == VT_LDOUBLE) {
            /* float registers hold f64, the scratch locals are i32:
               stage through REG_FRET and narrow back to the
               argument's own type on the operand stack */
            load(REG_FRET, sv);
            we_local_get(wloc_reg(REG_FRET));
            if ((sv->type.t & VT_BTYPE) == VT_FLOAT)
                we(0xb6); /* f32.demote_f64 */
        } else {
            load(NB_REGS + 1 + i, sv);
            we_local_get(wloc_scratch(i));
        }
    }
    /* call with a padded placeholder index, patched at output */
    {
        int pos = wcur.len;
        we(0x10); we_uleb5(0); /* call */
        wasm_add_fix(wcur_func, WFIX_CALL, pos + 1, func_sv->sym, 0);
    }
    /* vstack contract (same as riscv64-gen.c gfunc_call tail):
       consume function + args; tccgen then pushes the return value
       itself, expecting it in REG_IRET / REG_FRET. */
    vtop -= nb_args + 1;
    if (callee->rettype == WT_V128)
        we_local_set(wloc_vec(0)); /* vector return value: slot 0 */
    else if (callee->rettype == WT_I64) {
        /* split the i64 result into the REG_IRET/REG_IRE2 pair */
        we_local_set(wloc_i64());
        we_local_get(wloc_i64());
        we(0xa7); /* i32.wrap_i64 */
        we_local_set(wloc_reg(REG_IRET));
        we_local_get(wloc_i64());
        we_i64_const(32);
        we(0x87); /* i64.shr_s */
        we(0xa7); /* i32.wrap_i64 */
        we_local_set(wloc_reg(REG_IRE2));
    }
    else if (callee->rettype == WT_F32) {
        we(0xbb); /* f64.promote_f32: float registers hold f64 */
        we_local_set(wloc_reg(REG_FRET));
    }
    else if (callee->rettype == WT_F64)
        we_local_set(wloc_reg(REG_FRET));
    else if (callee->rettype != -1)
        we_local_set(wloc_reg(REG_IRET));
}

/* ------------------------------------------------------------------ */
/* prolog / epilog                                                     */

static int wfunc_frame_size; /* filled at epilog from the global `loc' */

ST_FUNC void gfunc_prolog(Sym *func_sym)
{
    CType *func_type = &func_sym->type;
    Sym *sym;
    int param_addr = 0, i = 0;
    if (func_var)
        tcc_error("wasm: variadic functions not implemented in skeleton ('%s')",
                  get_tok_str(func_sym->v, NULL));
    if ((func_vt.t & VT_BTYPE) == VT_STRUCT) {
        int al, sz = type_size(&func_vt, &al);
        if (sz > 8)
            tcc_error("wasm: struct return > 8 bytes not implemented in skeleton");
    }
    wcur_func = wasm_func_intern(func_sym, func_type);
    wcur_func->defined = 1;
    wcur_func->is_static = (func_type->t & VT_STATIC) != 0;
    wcur_func->is_import = 0;
    wb_init(&wcur);
    wcur_started = 1;
    wfunc_ind0 = ind; /* stream offset 0 of this function's body */
    nb_wjmps = 0;     /* jump records are per-function */
    amb_cmp_valid = 0;
    loc = 0;
    /* define parameters: each gets a frame slot at a negative offset
       from $fp; the incoming wasm argument is stored there */
    for (sym = func_type->ref; (sym = sym->next) != NULL; i++) {
        int align, size = type_size(&sym->type, &align);
        int is_vec = (sym->type.t & VT_VECTOR) != 0;
        if (is_vec) {
            if (size != 8 && size != 16)
                tcc_error("wasm: vector parameter of %d bytes not supported "
                          "(only 64/128-bit)", size);
        } else if (size > 8 || ((sym->type.t & VT_BTYPE) == VT_STRUCT && size > 4))
            tcc_error("wasm: parameter type not implemented in skeleton ('%s')",
                      get_tok_str(func_sym->v, NULL));
        if (size < 4) size = 4;
        loc -= size;
        param_addr = loc;
        gfunc_set_param(sym, param_addr, 0);
        /* addr = $fp + param_addr ; local.get arg_i ; store
           (wasm store operand order is [address, value]) */
        we_local_get(wloc_fp());
        if (param_addr) { we_i32_const(param_addr); we(0x6a); }
        we_local_get(i);
        if (is_vec) {
            if (size == 16) {
                we_v128_op(11); /* v128.store */
                we(0x00); we(0x00);
            } else {
                we_v128_op(91); /* v128.store64_lane */
                we(0x00); we(0x00); we(0x00); /* memarg, lane 0 */
            }
        } else if (size == 8 && (sym->type.t & VT_BTYPE) == VT_LLONG) {
            /* the incoming wasm argument is the whole i64 value */
            we(0x37); we(0x00); we(0x00); /* i64.store */
        } else {
            int isf = 0; int op = wasm_store_op(&sym->type, &isf);
            we(op); we(0x00); we(0x00);
        }
    }
    (void)param_addr;
}

/* append a "pc = block; br $dispatch" transfer to 'final'; 'depth'
   is the br depth of the dispatch loop at the emission point */
static void wb_transfer(WBuf *final, int block, int depth, int pc)
{
    wb_putc(final, 0x41); wb_sleb5(final, block);   /* i32.const */
    wb_putc(final, 0x21); wb_uleb(final, pc);       /* local.set $pc */
    wb_putc(final, 0x0c); wb_uleb(final, depth);    /* br */
}

ST_FUNC void gfunc_epilog(void)
{
    WBuf final, pro;
    int frame, i;
    if (!wcur_started) return;
    frame = (-loc + 15) & -16;
    wfunc_frame_size = frame;
    /* prolog bytes: $fp = $sp_old; $sp -= frame
       global.get $sp ; local.set scratch0   (save old sp in scratch 0)
       global.get $sp ; i32.const frame ; i32.sub ; global.set $sp
       local.get scratch0 ; local.set $fp */
    wb_init(&pro);
    wb_putc(&pro, 0x23); wb_uleb(&pro, 0);                 /* global.get $sp */
    wb_putc(&pro, 0x21); wb_uleb(&pro, wloc_scratch(0));   /* local.set save */
    wb_putc(&pro, 0x23); wb_uleb(&pro, 0);
    wb_putc(&pro, 0x41); wb_sleb5(&pro, frame);            /* i32.const frame */
    wb_putc(&pro, 0x6b);                                   /* i32.sub */
    wb_putc(&pro, 0x24); wb_uleb(&pro, 0);                 /* global.set $sp */
    wb_putc(&pro, 0x20); wb_uleb(&pro, wloc_scratch(0));   /* local.get save */
    wb_putc(&pro, 0x21); wb_uleb(&pro, wloc_fp());         /* local.set $fp */

    /* ---- basic-block dispatch assembly -----------------------------
       Blocks start at offset 0, at the end of the stream (an empty
       exit block) and at every jump target.  Layout:
         block $exit
           prolog ; pc = 0
           loop $dispatch
             block $case_{N-1} ... block $case_0
               local.get $pc ; br_table case_0..case_{N-1}, default $exit
             end $case_0   ; block 0 code ; transfer to block 1
             end $case_1   ; block 1 code ; transfer to block 2
             ...
             end $case_{N-1} ; block N-1 code ; transfer to block N
           end loop
         end $exit
         restore $sp ; return value ; end function
       Branching to $case_i lands on block i's code; pc == N (one
       past the last block) takes the br_table default out of $exit.
       A jump site inside block i branches to the loop at depth
       N-1-i (a gjmp_cond site, nested in its `if`, at depth N-i);
       fixup operands are copied verbatim and their positions are
       rewritten to the assembled body as they are placed. */
    {
        int stream_len = wcur.len;
        int *bounds, nb = 0, N, bi, ji = 0, fi = 0, pc = wloc_pc();
        bounds = tcc_malloc((nb_wjmps + 2) * sizeof(int));
        bounds[nb++] = 0;
        for (i = 0; i < nb_wjmps; i++) {
            int toff;
            if (wjmps[i].target < 0)
                tcc_error("wasm: unresolved jump in function '%s'",
                          wcur_func->name);
            toff = wjmps[i].target - wfunc_ind0;
            if (toff < 0 || toff > stream_len ||
                wjmps[i].off < 0 || wjmps[i].off + 5 > stream_len)
                tcc_error("wasm: jump outside the body in function '%s'",
                          wcur_func->name);
            bounds[nb++] = toff;
        }
        bounds[nb++] = stream_len;
        /* sort + unique */
        for (i = 1; i < nb; i++) {
            int v = bounds[i], k = i - 1;
            while (k >= 0 && bounds[k] > v) { bounds[k + 1] = bounds[k]; k--; }
            bounds[k + 1] = v;
        }
        {
            int w = 1;
            for (i = 1; i < nb; i++)
                if (bounds[i] != bounds[w - 1]) bounds[w++] = bounds[i];
            nb = w;
        }
        N = nb; /* every boundary starts a block; the last block is
                   the empty range at the end of the stream */
        wb_init(&final);
        wb_putc(&final, 0x02); wb_putc(&final, 0x40); /* block $exit */
        wb_write(&final, pro.data, pro.len);
        wb_putc(&final, 0x41); wb_sleb5(&final, 0);   /* i32.const 0 */
        wb_putc(&final, 0x21); wb_uleb(&final, pc);   /* local.set $pc */
        wb_putc(&final, 0x03); wb_putc(&final, 0x40); /* loop $dispatch */
        for (i = 0; i < N; i++) { wb_putc(&final, 0x02); wb_putc(&final, 0x40); }
        wb_putc(&final, 0x20); wb_uleb(&final, pc);   /* local.get $pc */
        wb_putc(&final, 0x0e); wb_uleb(&final, N);    /* br_table */
        for (i = 0; i < N; i++) wb_uleb(&final, i);   /* pc i -> depth i */
        wb_uleb(&final, N + 1);                       /* default -> $exit */
        for (bi = 0; bi < N; bi++) {
            int bstart = bounds[bi];
            int bend = bi + 1 < N ? bounds[bi + 1] : stream_len;
            int cursor = bstart;
            wb_putc(&final, 0x0b); /* end $case_bi: block bi's code follows */
            for (;;) {
                int joff = ji < nb_wjmps && wjmps[ji].off < bend
                           ? wjmps[ji].off : -1;
                int foff = fi < wcur_func->nfixs && wcur_func->fixs[fi].pos < bend
                           ? wcur_func->fixs[fi].pos : -1;
                if (joff < 0 && foff < 0) break;
                if (foff >= 0 && (joff < 0 || foff < joff)) {
                    WFix *x = &wcur_func->fixs[fi];
                    if (foff + 5 > bend)
                        tcc_error("wasm: fixup straddles a block boundary "
                                  "in function '%s'", wcur_func->name);
                    wb_write(&final, wcur.data + cursor, foff - cursor);
                    x->pos = final.len; /* assembled position of the operand */
                    wb_write(&final, wcur.data + foff, 5);
                    cursor = foff + 5;
                    fi++;
                } else {
                    WJmp *j = &wjmps[ji];
                    int toff = j->target - wfunc_ind0, tb = 0;
                    if (joff + 5 > bend)
                        tcc_error("wasm: jump site straddles a block boundary "
                                  "in function '%s'", wcur_func->name);
                    while (bounds[tb] != toff) tb++;
                    wb_write(&final, wcur.data + cursor, joff - cursor);
                    wb_transfer(&final, tb, N - 1 - bi + j->cond, pc);
                    cursor = joff + 5;
                    ji++;
                }
            }
            wb_write(&final, wcur.data + cursor, bend - cursor);
            /* fall through to the next block (or out, past the last) */
            wb_transfer(&final, bi + 1, N - 1 - bi, pc);
        }
        wb_putc(&final, 0x0b); /* end loop */
        wb_putc(&final, 0x0b); /* end $exit */
        /* restore stack pointer and fetch the return value */
        wb_putc(&final, 0x20); wb_uleb(&final, wloc_fp());
        wb_putc(&final, 0x24); wb_uleb(&final, 0);    /* global.set $sp */
        if (wcur_func->rettype == WT_V128) {
            wb_putc(&final, 0x20); wb_uleb(&final, wloc_vec(0));
        } else if (wcur_func->rettype == WT_F32) {
            wb_putc(&final, 0x20); wb_uleb(&final, wloc_reg(REG_FRET));
            wb_putc(&final, 0xb6); /* f32.demote_f64 */
        } else if (wcur_func->rettype == WT_F64) {
            wb_putc(&final, 0x20); wb_uleb(&final, wloc_reg(REG_FRET));
        } else if (wcur_func->rettype == WT_I64) {
            /* combine the REG_IRET/REG_IRE2 pair into the i64 result */
            wb_putc(&final, 0x20); wb_uleb(&final, wloc_reg(REG_IRE2));
            wb_putc(&final, 0xac); /* i64.extend_i32_s */
            wb_putc(&final, 0x42); wb_sleb(&final, 32); /* i64.const 32 */
            wb_putc(&final, 0x86); /* i64.shl */
            wb_putc(&final, 0x20); wb_uleb(&final, wloc_reg(REG_IRET));
            wb_putc(&final, 0xad); /* i64.extend_i32_u */
            wb_putc(&final, 0x84); /* i64.or */
        } else if (wcur_func->rettype != -1) {
            wb_putc(&final, 0x20); wb_uleb(&final, wloc_reg(REG_IRET));
        }
        wb_putc(&final, 0x0b); /* end function */
        tcc_free(bounds);
    }
    wb_free(&wcur_func->final_body);
    wcur_func->final_body = final;
    wb_free(&pro);
    wb_free(&wcur);
    wcur_started = 0;
}

/* ------------------------------------------------------------------ */
/* remaining backend surface: explicit stubs                           */

ST_FUNC void gen_fill_nops(int bytes)
{
    while (bytes-- > 0) we(0x01); /* wasm nop */
}

ST_FUNC void ggoto(void); /* defined above */

ST_FUNC void gen_vla_sp_save(int addr)   { (void)addr; tcc_error("wasm: VLA not implemented in skeleton"); }
ST_FUNC void gen_vla_sp_restore(int addr){ (void)addr; tcc_error("wasm: VLA not implemented in skeleton"); }
ST_FUNC void gen_vla_alloc(CType *type, int align)
{
    (void)type; (void)align;
    tcc_error("wasm: VLA/alloca not implemented in skeleton "
              "(design: adjust $__stack_pointer, see ASSESSMENT.md)");
}

ST_FUNC void gen_va_start(void)
{
    tcc_error("wasm: va_start not implemented in skeleton (variadic unsupported)");
}

ST_FUNC void gen_va_arg(CType *t)
{
    (void)t;
    tcc_error("wasm: va_arg not implemented in skeleton (variadic unsupported)");
}

ST_FUNC void gen_clear_cache(void)
{
    /* no icache to flush for wasm output */
}

#endif /* !TARGET_DEFS_ONLY */
