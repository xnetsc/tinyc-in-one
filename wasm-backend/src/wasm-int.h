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
} WSig;

#define WFIX_CALL 1 /* uleb5 at pos := final function index of sym */
#define WFIX_ADDR 2 /* sleb5 at pos := linear-memory address of sym + addend */

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
    WBuf final_body;  /* assembled at gfunc_epilog */
    WFix *fixs;
    int nfixs, fixcap;
} WFunc;

/* function and signature registries, filled by the generator and
 * read back by the output stage */
extern WSig wsigs[128];
extern int nb_wsigs;
extern WFunc wfuns[512];
extern int nb_wfuns;

#endif /* WASM_INT_H */
