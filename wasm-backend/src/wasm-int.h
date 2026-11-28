/* wasm-int.h - state shared by the wasm32 backend's code generator
 * (wasm-gen.c) and output stage (wasm-link.c).
 *
 * The two files are one backend: the generator fills the function
 * and signature registries and captures function bodies in WBuf
 * form; the output stage reads them back to assemble the module.
 * Include after tcc.h (the types below use Sym and addr_t).  The
 * definitions live in wasm-gen.c. */
#ifndef WASM_INT_H
#define WASM_INT_H

/* growable byte buffer */
typedef struct WBuf {
    unsigned char *data;
    int len, cap;
} WBuf;

void wb_init(WBuf *b);
void wb_free(WBuf *b);
void wb_putc(WBuf *b, int c);
void wb_write(WBuf *b, const void *p, int n);
void wb_uleb(WBuf *b, uint32_t v);
void wb_sleb(WBuf *b, int64_t v);
void wb_patch_uleb5(WBuf *b, int pos, uint32_t v);
void wb_patch_sleb5(WBuf *b, int pos, int32_t v);

/* wasm value types */
#define WT_I32 0x7f
#define WT_V128 0x7b
#define WT_I64 0x7e
#define WT_F32 0x7d
#define WT_F64 0x7c

typedef struct WSig {
    int nparams;
    int params[16];
    int ret; /* WT_* or -1 for void */
    int ret2; /* second result (a struct return of 5..8 bytes travels
                 as two i32 results) or -1 when there is none */
} WSig;

#define WFIX_CALL 1 /* uleb5 at pos := final function index of sym */
#define WFIX_ADDR 2 /* sleb5 at pos := linear-memory address of sym + addend */
#define WFIX_DYGLOB 3 /* dylink: uleb5 at pos := index of the global the
                         address materialises from (a process base or a
                         GOT global), decided at output time */
#define WFIX_DYCONST 4 /* dylink: sleb5 at pos := the constant paired with
                          WFIX_DYGLOB (base-relative offset, canonical
                          table slot offset, or the bare addend for a
                          GOT reference) */

typedef struct WFix {
    int pos;      /* offset in the function's final body */
    char *name;   /* owned copy: Sym objects are freed by tccgen_finish
                     before output runs, so nothing may keep Sym* */
    int kind;
    int addend;
    int shndx;    /* >= 0: resolve the address as wsec_base[shndx] +
                     value at output time, from the ELF section and
                     offset captured when the fixup was created;
                     -1 when the symbol is resolved by name */
    addr_t value; /* st_value captured at fixup creation when shndx >= 0 */
    int sym_func; /* the C symbol the fixup was created from is a
                     function: its type's base kind is VT_FUNC, the
                     attribute put_extern_sym2 maps to STT_FUNC.
                     Captured at creation because the symbol itself
                     is gone by output time, and a symbol referenced
                     only from address materialisations may never
                     reach the ELF symtab at all; the dylink output
                     classifies undefined symbols by this attribute */
    int sym_tls;  /* the C symbol carries VT_TLS (a __thread
                     variable): the dylink output classifies a
                     defined one into the module's TLS span and an
                     undefined one as a GOT.tls reference */
    int dead_local; /* the symbol is anonymous or static and had no
                     ELF entry when the fixup was created: its only
                     definition site was parsed with code generation
                     suppressed (a compile-time-dead branch), so the
                     frontend emitted neither the symbol nor its
                     data.  Output resolves the fixup to the
                     module's own data base instead of importing a
                     symbol no module can define */
} WFix;

typedef struct WFunc {
    Sym *sym;         /* valid only while that TU is being generated */
    char *name;       /* owned copy (see WFix note) */
    int is_static;    /* captured at prolog from the function type */
    int sig;          /* index into wsigs */
    int sig_from_call; /* sig was synthesized from a call site's actual
                          arguments (undeclared FUNC_OLD callee); a later
                          prototype replaces it */
    int is_import;
    int defined;
    int final_index;  /* assigned by wasm-link */
    int nparams;
    int rettype;      /* WT_* or -1 */
    int rettype2;     /* second result WT_* or -1 (two-i32 struct return) */
    WBuf final_body;  /* assembled at gfunc_epilog */
    WFix *fixs;
    int nfixs, fixcap;
    int *spfixs;      /* dylink: positions in final_body of the padded
                         $__stack_pointer operands the prologue/epilogue
                         emitted; the output stage patches them with
                         the mode's stack-pointer global index */
    int nspfixs, spfixcap;
} WFunc;

/* function and signature registries, filled by the generator and
 * read back by the output stage.  Their contents belong to one
 * output cycle of one TCCState; the generator drains them when a
 * new cycle begins (see wasm-gen.c). */
extern WSig wsigs[128];
extern int nb_wsigs;
extern WFunc wfuns[512];
extern int nb_wfuns;

/* called by the output stage after a module was written: marks the
 * current registry contents as the previous cycle */
void wasm_registry_note_output(void);

/* intern a signature built from raw wasm value types (WT_*; ret -1
 * for void) and return its wsigs index.  Used by the output stage
 * for the synthesized dylink init/fini functions. */
int wasm_intern_sig(int nparams, const int *params, int ret);

#endif /* WASM_INT_H */
