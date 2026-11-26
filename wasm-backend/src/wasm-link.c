/* wasm-link.c - TinyCC "linker" for wasm32 - SKELETON
 *
 * Two halves:
 *  1. The ELF-facing surface that tcc.h (xxx-link.c section, tcc.h:1606)
 *     and tccelf.c expect from every backend.  For wasm these are
 *     mostly stubs: there is no ELF relocation, no GOT/PLT.  Input
 *     object files are not supported (single-pass whole-program
 *     output only), so relocate() is a hard error.
 *  2. wasm_output_file(): the real output path, hooked into
 *     tcc_output_file() (tccelf.c) by the build patch.  It assembles
 *     one wasm module from the function bodies captured by
 *     wasm-gen.c plus the static data sections built by tccelf.c.
 *
 * Module layout:
 *   linear memory: [0,1024) reserved, then .rodata/.data/.bss bases,
 *   then a 64 KiB C stack growing down from $__stack_pointer init.
 *   funcref table covers every function (imports first, then defined)
 *   with table index == function index, ready for call_indirect.
 */

#define EM_TCC_TARGET 0x5741 /* private 'WA': wasm output never writes an
                                ELF header; this only feeds the input
                                object validator in tccelf.c, which the
                                wasm target rejects anyway */

/* Relocation constants that tccelf.c references for every target
   (mirrors riscv64-link.c:6-18).  Wasm has no ELF relocations; these
   private, mutually distinct values only satisfy the compiler - the
   code paths using them are bypassed by wasm_output_file. */
#define R_DATA_32  0x5700
#define R_DATA_PTR 0x5701
#define R_JMP_SLOT 0x5702
#define R_GLOB_DAT 0x5703
#define R_COPY     0x5704
#define R_RELATIVE 0x5705
#define R_NUM      0x5706
#define ELF_START_ADDR 0x00010000
#define ELF_PAGE_SIZE  0x1000
#define PCRELATIVE_DLLPLT 1
#define RELOCATE_DLLPLT 1

#if defined TARGET_DEFS_ONLY
/* nothing target-specific is defined here for tcc.h's defs pass */
#else

#define USING_GLOBALS
#include "tcc.h"
#include "wasm-int.h"

/* ---- ELF-facing stubs (tcc.h:1609-1628 interface) ------------------ */

ST_FUNC int code_reloc(int reloc_type)
{
    (void)reloc_type;
    return 0; /* no relocation is ever a code relocation for wasm */
}

ST_FUNC int gotplt_entry_type(int reloc_type)
{
    (void)reloc_type;
    return NO_GOTPLT_ENTRY; /* no GOT/PLT in a wasm module */
}

ST_FUNC unsigned create_plt_entry(TCCState *s1, unsigned got_offset,
                                  struct sym_attr *attr)
{
    (void)s1; (void)got_offset; (void)attr;
    tcc_error("wasm: PLT entries do not exist (ELF output path taken "
              "for a wasm target?)");
    return 0;
}

ST_FUNC void relocate_plt(TCCState *s1)
{
    (void)s1; /* nothing to do */
}

ST_FUNC void relocate(TCCState *s1, ElfW_Rel *rel, int type,
                      unsigned char *ptr, addr_t addr, addr_t val)
{
    (void)s1; (void)rel; (void)type; (void)ptr; (void)addr; (void)val;
    tcc_error("wasm: ELF relocation of input object files is not "
              "supported (single-pass whole-program output only)");
}

/* ---- module writer ------------------------------------------------- */

static void sec_begin(WBuf *out, int id, WBuf *payload)
{
    wb_putc(out, id);
    wb_uleb(out, payload->len);
    wb_write(out, payload->data, payload->len);
}

static void wb_name(WBuf *b, const char *s)
{
    int n = strlen(s);
    wb_uleb(b, n);
    wb_write(b, s, n);
}

static const char *wasi_names[] = {
    "args_get", "args_sizes_get", "environ_get", "environ_sizes_get",
    "fd_close", "fd_fdstat_get", "fd_read", "fd_seek", "fd_write",
    "path_open", "proc_exit", "clock_time_get", "random_get",
    "sched_yield", "poll_oneoff", "sock_recv", "sock_send", NULL
};

static const char *wasm_import_module(const char *name)
{
    int i;
    for (i = 0; wasi_names[i]; i++)
        if (!strcmp(name, wasi_names[i]))
            return "wasi_snapshot_preview1";
    return "env";
}

/* linear-memory bases per ELF section index, filled by wasm_output_file */
static addr_t *wsec_base;

/* Resolve a symbol by name at output time.  Sym objects are freed
   before output (tccgen_finish), but the ELF symtab section persists,
   so all output-stage lookups go through find_elf_sym. */
static int wasm_sym_addr(TCCState *s, const char *name, addr_t *out)
{
    int idx = find_elf_sym(symtab_section, name);
    ElfSym *es;
    Section *sec;
    if (idx <= 0) {
        /* find_elf_sym walks the symtab hash, which only holds
           global/weak symbols; fall back to a linear scan so local
           (static) symbols not captured at fixup creation - e.g. a
           static function called through an earlier prototype -
           resolve by name too.  First defined match wins. */
        int i, nb = symtab_section->data_offset / sizeof(ElfSym);
        idx = -1;
        for (i = 1; i < nb; i++) {
            ElfSym *cand = &((ElfSym *)symtab_section->data)[i];
            const char *cand_name;
            if (cand->st_shndx == SHN_UNDEF || !cand->st_name) continue;
            cand_name = (const char *)symtab_section->link->data + cand->st_name;
            if (!strcmp(cand_name, name)) { idx = i; break; }
        }
        if (idx < 0) return -1;
    }
    es = &((ElfSym *)symtab_section->data)[idx];
    if (es->st_shndx == SHN_UNDEF || es->st_shndx >= s->nb_sections)
        return -1;
    sec = s->sections[es->st_shndx];
    if (sec->name && !strcmp(sec->name, ".text")) {
        /* address of a function := its index in the funcref table,
           which equals its final function index */
        int i;
        for (i = 0; i < nb_wfuns; i++)
            if (!strcmp(wfuns[i].name, name)) { *out = wfuns[i].final_index; return 0; }
        return -1;
    }
    if (wsec_base[es->st_shndx] == (addr_t)-1) return -1;
    *out = wsec_base[es->st_shndx] + es->st_value;
    return 0;
}

/* ---- dylink output ---------------------------------------------------
 *
 * The loadable flavors (s->wasm_dylink_flavor: 1 = main, 2 = side).
 * The module imports its memory, table and bases and materialises
 * addresses base-relative (the generator emitted WFIX_DYGLOB /
 * WFIX_DYCONST pairs for them); it exports synthesized
 * __tinyc_dylink_init / __tinyc_dylink_fini functions.  The output
 * file is the dylink unit file: a JS envelope holding the metadata
 * object and the module bytes as base64 — the same form a
 * translated library takes; the module bytes carry no second copy
 * of the metadata.  The local layout (section bases, the 64 KiB
 * stack span) is the standalone layout; only the module's relation
 * to its environment differs. */

/* SHA-256 (FIPS 180-4), one-shot over an in-memory buffer: the
   unit file records the hash of the module bytes it carries. */
static void dy_sha256(const unsigned char *data, size_t len,
                      unsigned char out[32])
{
    static const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
        0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
        0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
        0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
        0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
        0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
        0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
        0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };
    uint32_t h[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                      0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    uint64_t bits = (uint64_t)len * 8;
    size_t total = ((len + 9 + 63) / 64) * 64, off;
    for (off = 0; off < total; off += 64) {
        uint32_t w[64];
        int t;
        for (t = 0; t < 16; t++) {
            uint32_t v = 0;
            int j;
            for (j = 0; j < 4; j++) {
                size_t at = off + t * 4 + j;
                unsigned char c;
                if (at < len) c = data[at];
                else if (at == len) c = 0x80;
                else if (at >= total - 8)
                    c = (unsigned char)(bits >> (8 * (total - 1 - at)));
                else c = 0;
                v = (v << 8) | c;
            }
            w[t] = v;
        }
        for (t = 16; t < 64; t++) {
            uint32_t s0 = ((w[t-15] >> 7) | (w[t-15] << 25)) ^
                          ((w[t-15] >> 18) | (w[t-15] << 14)) ^ (w[t-15] >> 3);
            uint32_t s1 = ((w[t-2] >> 17) | (w[t-2] << 15)) ^
                          ((w[t-2] >> 19) | (w[t-2] << 13)) ^ (w[t-2] >> 10);
            w[t] = w[t-16] + s0 + w[t-7] + s1;
        }
        {
            uint32_t a=h[0], b=h[1], c=h[2], d=h[3],
                     e=h[4], f=h[5], g=h[6], hh=h[7];
            for (t = 0; t < 64; t++) {
                uint32_t s1 = ((e >> 6) | (e << 26)) ^
                              ((e >> 11) | (e << 21)) ^ ((e >> 25) | (e << 7));
                uint32_t ch = (e & f) ^ (~e & g);
                uint32_t t1 = hh + s1 + ch + k[t] + w[t];
                uint32_t s0 = ((a >> 2) | (a << 30)) ^
                              ((a >> 13) | (a << 19)) ^ ((a >> 22) | (a << 10));
                uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
                hh = g; g = f; f = e; e = d + t1;
                d = c; c = b; b = a; a = t1 + s0 + mj;
            }
            h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d;
            h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
        }
    }
    for (off = 0; off < 8; off++) {
        out[off*4]   = (unsigned char)(h[off] >> 24);
        out[off*4+1] = (unsigned char)(h[off] >> 16);
        out[off*4+2] = (unsigned char)(h[off] >> 8);
        out[off*4+3] = (unsigned char)(h[off]);
    }
}

static void dy_b64(WBuf *out, const unsigned char *data, size_t len)
{
    static const char tab[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t i;
    for (i = 0; i < len; i += 3) {
        uint32_t v = (uint32_t)data[i] << 16;
        int rem = (int)(len - i);
        if (rem > 1) v |= (uint32_t)data[i+1] << 8;
        if (rem > 2) v |= data[i+2];
        wb_putc(out, tab[(v >> 18) & 63]);
        wb_putc(out, tab[(v >> 12) & 63]);
        wb_putc(out, rem > 1 ? tab[(v >> 6) & 63] : '=');
        wb_putc(out, rem > 2 ? tab[v & 63] : '=');
    }
}

/* a JSON string literal, quoted and escaped */
static void dy_json_str(WBuf *out, const char *s)
{
    wb_putc(out, '"');
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') { wb_putc(out, '\\'); wb_putc(out, c); }
        else if (c < 0x20) {
            char tmp[8];
            snprintf(tmp, sizeof tmp, "\\u%04x", c);
            wb_write(out, tmp, (int)strlen(tmp));
        } else wb_putc(out, c);
    }
    wb_putc(out, '"');
}

static void dy_json_num(WBuf *out, unsigned long long v)
{
    char tmp[24];
    snprintf(tmp, sizeof tmp, "%llu", v);
    wb_write(out, tmp, (int)strlen(tmp));
}

#define DY_DEFDATA 1 /* defined data: __memory_base + local address */
#define DY_DEFFUNC 2 /* defined function: __table_base + defined order */
#define DY_GOTFUNC 3 /* undefined function: GOT.func global + addend */
#define DY_GOTMEM  4 /* undefined variable: GOT.mem global + addend */

typedef struct DyGot {
    const char *name; /* borrowed: owned by the fixup that named it */
    int kind;         /* DY_GOTFUNC or DY_GOTMEM */
    int gindex;       /* index of the imported GOT global */
} DyGot;

typedef struct DyVar {
    const char *name; /* borrowed: symtab string storage */
    addr_t value;     /* module-local address */
} DyVar;

static int wasm_find_wfunc(const char *name, int want_defined)
{
    int i;
    for (i = 0; i < nb_wfuns; i++)
        if (!strcmp(wfuns[i].name, name) &&
            (!want_defined || wfuns[i].defined))
            return i;
    return -1;
}

/* index of a defined function among the module's defined functions */
static int wasm_defined_order(int wf)
{
    int i, n = 0;
    for (i = 0; i < wf; i++)
        if (wfuns[i].defined) n++;
    return n;
}

/* ---- data-section relocations ---------------------------------------
 *
 * Initialised pointers in the data sections (char *s = "hi",
 * int *p = &gv, function-pointer tables) are recorded by the
 * frontend as R_DATA_PTR entries in the section's reloc array, with
 * the addend stored in place and the slot itself holding only that
 * addend.  The ELF output path applies the entries in
 * relocate_sections(); the wasm path applies them here, against its
 * own layout, or the slots keep the bare addend (typically 0). */

typedef struct DyDataRel {
    addr_t slot;  /* module-local offset of the pointer slot */
    int is_func;  /* the slot holds a function address (a table
                     slot), not a memory address */
} DyDataRel;

/* A data slot whose initial value is the address of an undefined
   symbol (int (*fp)(int) = imported_fn; int *p = &imported_var;):
   the value only the process linker knows, delivered through the
   symbol's GOT global.  The synthesized init copies the GOT value
   (plus the in-place addend) into the slot at load time. */
typedef struct DyDataGot {
    addr_t slot;      /* module-local offset of the pointer slot */
    const char *name; /* borrowed: symtab string storage */
    int kind;         /* DY_GOTFUNC or DY_GOTMEM */
    int32_t addend;   /* the in-place addend the slot starts with */
} DyDataGot;

static void dy_data_got_add(DyDataGot **listp, int *np, int *capp,
                            addr_t slot, const char *name, int kind,
                            int32_t addend)
{
    DyDataGot *list = *listp;
    if (*np >= *capp) {
        *capp = *capp ? *capp * 2 : 8;
        list = tcc_realloc(list, *capp * sizeof(DyDataGot));
        *listp = list;
    }
    list[*np].slot = slot;
    list[*np].name = name;
    list[*np].kind = kind;
    list[*np].addend = addend;
    (*np)++;
}

/* The frontend's relocation types as they survive in a stored
   entry: ELF32 R_INFO keeps only the low byte of the type, so the
   backend's private R_DATA_PTR / R_DATA_32 constants (0x5701 /
   0x5700) read back as 1 / 0. */
#define DY_RDATA_PTR (R_DATA_PTR & 0xff)
#define DY_RDATA_32  (R_DATA_32 & 0xff)

/* Apply the data relocations of every placed section against the
   layout in wsec_base[], patching the section bytes in place: each
   slot becomes target + addend in module-local coordinates (the
   standalone layout is its own absolute layout, so the values are
   final there).  When list_out is non-NULL (dylink output), the
   applied slots are also recorded, so the synthesized init function
   can add the process base (__memory_base, or __table_base for
   function addresses) to each of them at load time.  Slots whose
   target is an undefined symbol cannot be patched at output time
   at all: in dylink output they are recorded in the glist_out list
   instead, for the init to fill from the target's GOT global. */
static void wasm_apply_data_relocs(TCCState *s,
                                   DyDataRel **list_out, int *n_out,
                                   DyDataGot **glist_out, int *ng_out)
{
    int i, n = 0, cap = 0, gn = 0, gcap = 0;
    DyDataRel *list = NULL;
    DyDataGot *glist = NULL;
    if (list_out) *list_out = NULL;
    if (n_out) *n_out = 0;
    if (glist_out) *glist_out = NULL;
    if (ng_out) *ng_out = 0;
    for (i = 1; i < s->nb_sections; i++) {
        Section *sec = s->sections[i];
        int nrel, k;
        if (!sec->reloc || !sec->data || sec->sh_type == SHT_NOBITS)
            continue;
        if (wsec_base[i] == (addr_t)-1) continue;
        nrel = sec->reloc->data_offset / sizeof(ElfW_Rel);
        for (k = 0; k < nrel; k++) {
            ElfW_Rel *rel = &((ElfW_Rel *)sec->reloc->data)[k];
            ElfSym *es;
            Section *tsec;
            int type = ELFW(R_TYPE)(rel->r_info);
            int sym_index = ELFW(R_SYM)(rel->r_info);
            addr_t value;
            int is_func = 0;
            if (type != DY_RDATA_PTR && type != DY_RDATA_32) continue;
            if (rel->r_offset + 4 > sec->data_offset) continue;
            if (sym_index <= 0 ||
                sym_index >= (int)(symtab_section->data_offset /
                                   sizeof(ElfSym)))
                continue;
            es = &((ElfSym *)symtab_section->data)[sym_index];
            if (es->st_shndx == SHN_UNDEF) {
                /* Undefined target: a cross-module reference whose
                   value only the process linker knows.  In dylink
                   output, record it for the synthesized init to
                   fill from the symbol's GOT global at load time
                   (the slot keeps its in-place addend, which the
                   init adds to the GOT value).  A weak undefined
                   reference keeps the historical behaviour — the
                   slot reads its addend, typically 0 — and so does
                   standalone output, which has no GOT. */
                if (glist_out &&
                    ELFW(ST_BIND)(es->st_info) != STB_WEAK) {
                    const char *name =
                        (const char *)symtab_section->link->data +
                        es->st_name;
                    int is_func =
                        ELFW(ST_TYPE)(es->st_info) == STT_FUNC;
                    int wf;
                    if (!is_func &&
                        (wf = wasm_find_wfunc(name, 0)) >= 0 &&
                        !wfuns[wf].defined)
                        is_func = 1;
                    dy_data_got_add(&glist, &gn, &gcap,
                                    wsec_base[i] + rel->r_offset, name,
                                    is_func ? DY_GOTFUNC : DY_GOTMEM,
                                    (int32_t)read32le(sec->data +
                                                      rel->r_offset));
                }
                continue;
            }
            if (es->st_shndx >= s->nb_sections)
                continue;
            tsec = s->sections[es->st_shndx];
            if (tsec->name && !strcmp(tsec->name, ".text")) {
                const char *name =
                    (const char *)symtab_section->link->data + es->st_name;
                int wf = wasm_find_wfunc(name, 0);
                if (wf < 0) continue;
                if (list_out) {
                    /* dylink: only this module's own functions have
                       a slot in its table window; an imported
                       function's address is a GOT.func value, which
                       the init copies into the slot at load time */
                    if (!wfuns[wf].defined) {
                        if (glist_out)
                            dy_data_got_add(&glist, &gn, &gcap,
                                            wsec_base[i] + rel->r_offset,
                                            name, DY_GOTFUNC,
                                            (int32_t)read32le(sec->data +
                                                              rel->r_offset));
                        continue;
                    }
                    value = wasm_defined_order(wf);
                } else
                    value = wfuns[wf].final_index;
                is_func = 1;
            } else {
                if (wsec_base[es->st_shndx] == (addr_t)-1) continue;
                value = wsec_base[es->st_shndx] + es->st_value;
            }
            write32le(sec->data + rel->r_offset,
                      value + read32le(sec->data + rel->r_offset));
            if (list_out) {
                if (n >= cap) {
                    cap = cap ? cap * 2 : 16;
                    list = tcc_realloc(list, cap * sizeof(DyDataRel));
                }
                list[n].slot = wsec_base[i] + rel->r_offset;
                list[n].is_func = is_func;
                n++;
            }
        }
    }
    if (list_out) { *list_out = list; *n_out = n; }
    if (glist_out) { *glist_out = glist; *ng_out = gn; }
}

/* Symtab lookup by name: the hash hit when there is one, else a
   linear scan preferring a defined entry (local symbols are not in
   the hash).  Returns the symtab index or -1. */
static int wasm_symtab_find(const char *name)
{
    int idx = find_elf_sym(symtab_section, name);
    int i, nb, undef = -1;
    if (idx > 0) return idx;
    nb = symtab_section->data_offset / sizeof(ElfSym);
    for (i = 1; i < nb; i++) {
        ElfSym *cand = &((ElfSym *)symtab_section->data)[i];
        const char *cand_name;
        if (!cand->st_name) continue;
        cand_name = (const char *)symtab_section->link->data + cand->st_name;
        if (strcmp(cand_name, name)) continue;
        if (cand->st_shndx != SHN_UNDEF) return i;
        if (undef < 0) undef = i;
    }
    return undef;
}

/* Classify the symbol a WFIX_DYGLOB/WFIX_DYCONST pair refers to.
   Returns a DY_* kind and stores the module-local value (data
   address or defined-function order) in *local; 0 when the symbol
   cannot be resolved at output time. */
static int wasm_dy_classify(TCCState *s, WFix *x, addr_t *local)
{
    int idx, wf;
    ElfSym *es;
    if (x->shndx >= 0) {
        if (x->shndx >= s->nb_sections || wsec_base[x->shndx] == (addr_t)-1)
            return 0;
        *local = wsec_base[x->shndx] + x->value;
        return DY_DEFDATA;
    }
    idx = wasm_symtab_find(x->name);
    if (idx > 0) {
        es = &((ElfSym *)symtab_section->data)[idx];
        if (es->st_shndx != SHN_UNDEF && es->st_shndx < s->nb_sections) {
            Section *sec = s->sections[es->st_shndx];
            if (sec->name && !strcmp(sec->name, ".text")) {
                wf = wasm_find_wfunc(x->name, 1);
                if (wf < 0) return 0;
                *local = wasm_defined_order(wf);
                return DY_DEFFUNC;
            }
            if (wsec_base[es->st_shndx] != (addr_t)-1) {
                *local = wsec_base[es->st_shndx] + es->st_value;
                return DY_DEFDATA;
            }
            return 0; /* defined in a section the layout does not place */
        }
        /* undefined: the symbol's own type decides — the symtab
           attribute when an entry exists, the attribute the fixup
           captured from the C symbol otherwise */
        return (ELFW(ST_TYPE)(es->st_info) == STT_FUNC || x->sym_func)
            ? DY_GOTFUNC : DY_GOTMEM;
    }
    /* No symtab entry at all: a symbol referenced only from code
       address materialisations never reaches the ELF symtab (no
       relocation ever names it, and it is interned as a WFunc only
       when something calls it).  Classify by the type the fixup
       captured from the C symbol itself — the same attribute
       put_extern_sym2 would have written into the symtab — so a
       function goes through GOT.func whether or not anything ever
       called it directly, and a variable through GOT.mem. */
    return x->sym_func ? DY_GOTFUNC : DY_GOTMEM;
}

static int wasm_dy_got(DyGot *gots, int ngots, int kind, const char *name)
{
    int i;
    for (i = 0; i < ngots; i++)
        if (gots[i].kind == kind && !strcmp(gots[i].name, name))
            return gots[i].gindex;
    return -1;
}

/* The function names recorded in a section's relocation array
   (.init_array / .fini_array entries, in declaration order).
   Returns a tcc_malloc'd array of borrowed name pointers. */
static const char **wasm_array_entries(TCCState *s, const char *secname,
                                       int *out_n)
{
    Section *sec = NULL;
    const char **names;
    int i, n;
    *out_n = 0;
    for (i = 1; i < s->nb_sections; i++)
        if (s->sections[i]->name && !strcmp(s->sections[i]->name, secname)) {
            sec = s->sections[i];
            break;
        }
    if (!sec || !sec->reloc) return NULL;
    n = sec->reloc->data_offset / sizeof(ElfW_Rel);
    if (!n) return NULL;
    names = tcc_malloc(n * sizeof(char *));
    for (i = 0; i < n; i++) {
        ElfW_Rel *r = &((ElfW_Rel *)sec->reloc->data)[i];
        ElfSym *es = &((ElfSym *)symtab_section->data)[ELFW(R_SYM)(r->r_info)];
        names[i] = (const char *)symtab_section->link->data + es->st_name;
    }
    *out_n = n;
    return names;
}

/* Look a load hook up among the defined functions and check its
   signature.  Returns the wfuns index, -1 when absent, -2 when
   defined with the wrong signature. */
static int wasm_dy_hook(const char *name, int nparams, int ret)
{
    int wf = wasm_find_wfunc(name, 1), i;
    WSig *sig;
    if (wf < 0) return -1;
    sig = &wsigs[wfuns[wf].sig];
    if (sig->nparams != nparams || sig->ret != ret) return -2;
    for (i = 0; i < nparams; i++)
        if (sig->params[i] != WT_I32) return -2;
    return wf;
}

/* Append a synthesized function to the registry, before index
   assignment, so it takes an ordinary function index. */
static int wasm_dy_add_synth(const char *name, int sig, int nparams,
                             int rettype)
{
    WFunc *fn;
    if (nb_wfuns >= 512) return -1;
    fn = &wfuns[nb_wfuns];
    memset(fn, 0, sizeof *fn);
    fn->name = tcc_strdup(name);
    fn->sig = sig;
    fn->nparams = nparams;
    fn->rettype = rettype;
    fn->defined = 1;
    fn->final_index = -1;
    wb_init(&fn->final_body);
    return nb_wfuns++;
}

static int wasm_output_dylink(TCCState *s, const char *filename)
{
    WBuf mod, pay;
    FILE *f;
    int i, j, nimports = 0, ndefined = 0, total;
    addr_t data_end = 1024, sp_init, mem_bytes, img_end = 0;
    addr_t bss_off = 0, bss_size = 0;
    int init_wf, fini_wf;
    int hook_init, hook_fini, hook_dll;
    const char **ctors = NULL, **dtors = NULL;
    int nctors = 0, ndtors = 0;
    DyGot *gots = NULL;
    int ngots = 0, gotcap = 0, sp_gindex;
    DyVar *vars = NULL;
    int nvars = 0, varcap = 0;
    DyDataRel *drels = NULL;
    int ndrels = 0;
    DyDataGot *dgots = NULL;
    int ndgots = 0;
    int rc = 0;

    if (wasm_find_wfunc("__tinyc_dylink_init", 0) >= 0 ||
        wasm_find_wfunc("__tinyc_dylink_fini", 0) >= 0)
        return tcc_error_noabort("wasm: '__tinyc_dylink_init' and "
            "'__tinyc_dylink_fini' are reserved in dylink mode");
    {
        int p1[1] = { WT_I32 }, p2[2] = { WT_I32, WT_I32 };
        init_wf = wasm_dy_add_synth("__tinyc_dylink_init",
                                    wasm_intern_sig(1, p1, WT_I32), 1, WT_I32);
        fini_wf = wasm_dy_add_synth("__tinyc_dylink_fini",
                                    wasm_intern_sig(2, p2, -1), 2, -1);
    }
    if (init_wf < 0 || fini_wf < 0)
        return tcc_error_noabort("wasm: too many functions");

    /* ---- assign final function indices: imports first ------------- */
    for (i = 0; i < nb_wfuns; i++) {
        if (wfuns[i].defined) continue;
        wfuns[i].is_import = 1;
        wfuns[i].final_index = nimports++;
    }
    for (i = 0; i < nb_wfuns; i++) {
        if (!wfuns[i].defined) continue;
        wfuns[i].final_index = nimports + ndefined++;
    }
    total = nimports + ndefined;

    /* ---- lay out static data sections in linear memory ------------ */
    wsec_base = tcc_mallocz(s->nb_sections * sizeof(addr_t));
    for (i = 0; i < s->nb_sections; i++) wsec_base[i] = (addr_t)-1;
    for (i = 1; i < s->nb_sections; i++) {
        Section *sec = s->sections[i];
        const char *nm = sec->name;
        if (!nm) continue;
        if (!strcmp(nm, ".rodata") || !strncmp(nm, ".rodata.", 8) ||
            !strcmp(nm, ".data") || !strncmp(nm, ".data.", 6) ||
            !strcmp(nm, ".bss")) {
            data_end = (data_end + 15) & -16;
            wsec_base[i] = data_end;
            data_end += sec->data_offset;
            if (!strcmp(nm, ".bss")) {
                bss_off = wsec_base[i];
                bss_size = sec->data_offset;
            } else if (sec->sh_type != SHT_NOBITS &&
                       wsec_base[i] + sec->data_offset > img_end)
                img_end = wsec_base[i] + sec->data_offset;
        }
    }
    sp_init = ((data_end + 15) & -16) + 65536; /* 64 KiB C stack */
    mem_bytes = (sp_init + 65535) & -65536;

    /* ---- data relocations: patch the slots to module-local values;
       the synthesized init adds the process base at load time.
       Slots referencing undefined symbols are collected as GOT
       references (dgots) for the init to fill from the GOT -------- */
    wasm_apply_data_relocs(s, &drels, &ndrels, &dgots, &ndgots);

    /* ---- load hooks ------------------------------------------------- */
    hook_init = wasm_dy_hook("_init", 0, -1);
    if (hook_init == -2) {
        rc = tcc_error_noabort("wasm: dylink load hook '_init' must "
                               "have signature 'void _init(void)'");
        goto out;
    }
    hook_fini = wasm_dy_hook("_fini", 0, -1);
    if (hook_fini == -2) {
        rc = tcc_error_noabort("wasm: dylink load hook '_fini' must "
                               "have signature 'void _fini(void)'");
        goto out;
    }
    hook_dll = wasm_dy_hook("DllMain", 3, WT_I32);
    if (hook_dll == -2) {
        rc = tcc_error_noabort("wasm: dylink load hook 'DllMain' must "
                               "have signature 'int DllMain(int, int, int)'");
        goto out;
    }
    ctors = wasm_array_entries(s, ".init_array", &nctors);
    dtors = wasm_array_entries(s, ".fini_array", &ndtors);
    for (i = 0; i < nctors; i++)
        if (wasm_find_wfunc(ctors[i], 1) < 0) {
            rc = tcc_error_noabort("wasm: constructor '%s' in .init_array "
                                   "is not a defined function", ctors[i]);
            goto out;
        }
    for (i = 0; i < ndtors; i++)
        if (wasm_find_wfunc(dtors[i], 1) < 0) {
            rc = tcc_error_noabort("wasm: destructor '%s' in .fini_array "
                                   "is not a defined function", dtors[i]);
            goto out;
        }

    /* ---- classify address materialisations, collect GOT globals ---- */
    for (i = 0; i < nb_wfuns; i++) {
        WFunc *fn = &wfuns[i];
        if (!fn->defined) continue;
        for (j = 0; j < fn->nfixs; j++) {
            WFix *x = &fn->fixs[j];
            addr_t local;
            int kind;
            if (x->kind == WFIX_CALL) continue;
            if (x->kind != WFIX_DYGLOB || j + 1 >= fn->nfixs ||
                fn->fixs[j + 1].kind != WFIX_DYCONST) {
                rc = tcc_error_noabort("wasm: unpaired dylink address "
                                       "fixup in function '%s'", fn->name);
                goto out;
            }
            kind = wasm_dy_classify(s, x, &local);
            if (!kind) {
                rc = tcc_error_noabort("wasm: cannot resolve address "
                                       "of '%s'", x->name);
                goto out;
            }
            if ((kind == DY_GOTFUNC || kind == DY_GOTMEM) &&
                wasm_dy_got(gots, ngots, kind, x->name) < 0) {
                if (ngots >= gotcap) {
                    gotcap = gotcap ? gotcap * 2 : 16;
                    gots = tcc_realloc(gots, gotcap * sizeof(DyGot));
                }
                gots[ngots].name = x->name;
                gots[ngots].kind = kind;
                gots[ngots].gindex = 2 + ngots;
                ngots++;
            }
            j++; /* the DYCONST half is classified with its DYGLOB */
        }
    }
    /* ---- register the GOT globals the data slots reference ------- */
    for (i = 0; i < ndgots; i++) {
        if (wasm_dy_got(gots, ngots, dgots[i].kind, dgots[i].name) < 0) {
            if (ngots >= gotcap) {
                gotcap = gotcap ? gotcap * 2 : 16;
                gots = tcc_realloc(gots, gotcap * sizeof(DyGot));
            }
            gots[ngots].name = dgots[i].name;
            gots[ngots].kind = dgots[i].kind;
            gots[ngots].gindex = 2 + ngots;
            ngots++;
        }
    }
    sp_gindex = 2 + ngots;

    /* ---- collect the exported variables for the metadata ---------- */
    {
        int nb_syms = symtab_section->data_offset / sizeof(ElfSym);
        for (i = 1; i < nb_syms; i++) {
            ElfSym *es = &((ElfSym *)symtab_section->data)[i];
            const char *nm;
            int bind, dup = 0, k;
            if (!es->st_name) continue;
            bind = ELFW(ST_BIND)(es->st_info);
            if (bind != STB_GLOBAL && bind != STB_WEAK) continue;
            if (es->st_shndx == SHN_UNDEF || es->st_shndx >= s->nb_sections)
                continue;
            if (wsec_base[es->st_shndx] == (addr_t)-1) continue;
            if (ELFW(ST_TYPE)(es->st_info) == STT_FUNC) continue;
            nm = (const char *)symtab_section->link->data + es->st_name;
            for (k = 0; k < nvars; k++)
                if (!strcmp(vars[k].name, nm)) { dup = 1; break; }
            if (dup) continue;
            if (nvars >= varcap) {
                varcap = varcap ? varcap * 2 : 16;
                vars = tcc_realloc(vars, varcap * sizeof(DyVar));
            }
            vars[nvars].name = nm;
            vars[nvars].value = wsec_base[es->st_shndx] + es->st_value;
            nvars++;
        }
    }

    /* ---- assemble the synthesized init/fini bodies ------------------ */
    {
        WBuf *b = &wfuns[init_wf].final_body;
        wb_putc(b, 0x23); wb_uleb(b, 0);         /* global.get __memory_base */
        wb_putc(b, 0x41); wb_sleb(b, (int32_t)sp_init);
        wb_putc(b, 0x6a);                        /* i32.add */
        wb_putc(b, 0x24); wb_uleb(b, sp_gindex); /* global.set $sp */
        if (bss_size) {
            wb_putc(b, 0x23); wb_uleb(b, 0);
            wb_putc(b, 0x41); wb_sleb(b, (int32_t)bss_off);
            wb_putc(b, 0x6a);
            wb_putc(b, 0x41); wb_putc(b, 0x00);  /* fill value 0 */
            wb_putc(b, 0x41); wb_sleb(b, (int32_t)bss_size);
            wb_putc(b, 0xfc); wb_putc(b, 0x0b); wb_putc(b, 0x00); /* memory.fill */
        }
        /* data relocations: the output stage left module-local
           values in the pointer slots; add the process base to each
           (__memory_base for memory addresses, __table_base for
           function addresses) before any hook or constructor runs.
           The slot itself always lives in memory: its address is
           __memory_base + slot regardless of what it holds. */
        for (i = 0; i < ndrels; i++) {
            int bg = drels[i].is_func ? 1 : 0;
            /* scratch local 0: nparams + NB_REGS + 1, inside the
               standard locals block every function body carries */
            int L = wfuns[init_wf].nparams + 19;
            wb_putc(b, 0x23); wb_putc(b, 0x00);           /* global.get __memory_base */
            wb_putc(b, 0x41); wb_sleb(b, (int32_t)drels[i].slot);
            wb_putc(b, 0x6a);                            /* i32.add */
            wb_putc(b, 0x21); wb_uleb(b, L);             /* local.set L */
            wb_putc(b, 0x20); wb_uleb(b, L);             /* local.get L (address) */
            wb_putc(b, 0x20); wb_uleb(b, L);             /* local.get L */
            wb_putc(b, 0x28); wb_putc(b, 0x00); wb_putc(b, 0x00); /* i32.load */
            wb_putc(b, 0x23); wb_uleb(b, bg);            /* global.get value base */
            wb_putc(b, 0x6a);                            /* i32.add */
            wb_putc(b, 0x36); wb_putc(b, 0x00); wb_putc(b, 0x00); /* i32.store */
        }
        /* data slots referencing undefined symbols: store the
           symbol's GOT value (plus the slot's in-place addend).
           The GOT global is the canonical per-process value — a
           function's table slot or a variable's absolute address —
           so the copied pointer is identical to what a runtime
           address materialisation or dlsym produces.  The loader
           binds the GOT.func entries named by these slots eagerly
           (metadata "data_got_func"): the value leaves the GOT
           here, so a later binding could never repair the copy. */
        for (i = 0; i < ndgots; i++) {
            int gindex = wasm_dy_got(gots, ngots, dgots[i].kind,
                                     dgots[i].name);
            wb_putc(b, 0x23); wb_putc(b, 0x00);           /* global.get __memory_base */
            wb_putc(b, 0x41); wb_sleb(b, (int32_t)dgots[i].slot);
            wb_putc(b, 0x6a);                            /* i32.add: slot address */
            wb_putc(b, 0x23); wb_uleb(b, gindex);        /* global.get GOT */
            if (dgots[i].addend) {
                wb_putc(b, 0x41); wb_sleb(b, dgots[i].addend);
                wb_putc(b, 0x6a);                        /* i32.add */
            }
            wb_putc(b, 0x36); wb_putc(b, 0x00); wb_putc(b, 0x00); /* i32.store */
        }
        if (hook_init >= 0) {
            wb_putc(b, 0x10); wb_uleb(b, wfuns[hook_init].final_index);
        }
        for (i = 0; i < nctors; i++) {
            int wf = wasm_find_wfunc(ctors[i], 1);
            wb_putc(b, 0x10); wb_uleb(b, wfuns[wf].final_index);
        }
        if (hook_dll >= 0) {
            wb_putc(b, 0x20); wb_uleb(b, 0);     /* local.get handle */
            wb_putc(b, 0x41); wb_putc(b, 0x01);  /* reason: process attach */
            wb_putc(b, 0x41); wb_putc(b, 0x00);  /* reserved */
            wb_putc(b, 0x10); wb_uleb(b, wfuns[hook_dll].final_index);
            wb_putc(b, 0x45);                    /* i32.eqz */
            wb_putc(b, 0x04); wb_putc(b, 0x40);  /* if */
            wb_putc(b, 0x41); wb_putc(b, 0x01);  /* i32.const 1 */
            wb_putc(b, 0x0f);                    /* return */
            wb_putc(b, 0x0b);                    /* end if */
        }
        wb_putc(b, 0x41); wb_putc(b, 0x00);      /* i32.const 0 */
        wb_putc(b, 0x0b);                        /* end function */
    }
    {
        WBuf *b = &wfuns[fini_wf].final_body;
        if (hook_dll >= 0) {
            wb_putc(b, 0x20); wb_uleb(b, 0);     /* local.get handle */
            wb_putc(b, 0x41); wb_putc(b, 0x00);  /* reason: process detach */
            wb_putc(b, 0x20); wb_uleb(b, 1);     /* local.get reserved */
            wb_putc(b, 0x10); wb_uleb(b, wfuns[hook_dll].final_index);
            wb_putc(b, 0x1a);                    /* drop */
        }
        for (i = ndtors - 1; i >= 0; i--) {
            int wf = wasm_find_wfunc(dtors[i], 1);
            wb_putc(b, 0x10); wb_uleb(b, wfuns[wf].final_index);
        }
        if (hook_fini >= 0) {
            wb_putc(b, 0x10); wb_uleb(b, wfuns[hook_fini].final_index);
        }
        wb_putc(b, 0x0b);                        /* end function */
    }

    /* ---- patch fixups in every defined function body ----------------- */
    for (i = 0; i < nb_wfuns; i++) {
        WFunc *fn = &wfuns[i];
        if (!fn->defined) continue;
        for (j = 0; j < fn->nfixs; j++) {
            WFix *x = &fn->fixs[j];
            if (x->kind == WFIX_CALL) {
                WFunc *callee = NULL;
                int k;
                for (k = 0; k < nb_wfuns; k++)
                    if (!strcmp(wfuns[k].name, x->name)) { callee = &wfuns[k]; break; }
                if (!callee) {
                    rc = tcc_error_noabort("wasm: call to unknown "
                                           "function '%s'", x->name);
                    goto out;
                }
                wb_patch_uleb5(&fn->final_body, x->pos, callee->final_index);
            } else if (x->kind == WFIX_DYGLOB) {
                WFix *y = &fn->fixs[j + 1];
                addr_t local = 0;
                int kind = wasm_dy_classify(s, x, &local);
                int gindex;
                if (kind == DY_DEFDATA) gindex = 0;
                else if (kind == DY_DEFFUNC) gindex = 1;
                else gindex = wasm_dy_got(gots, ngots, kind, x->name);
                wb_patch_uleb5(&fn->final_body, x->pos, (uint32_t)gindex);
                wb_patch_sleb5(&fn->final_body, y->pos,
                               (int32_t)((kind == DY_GOTFUNC ||
                                          kind == DY_GOTMEM)
                                         ? x->addend
                                         : local + x->addend));
                j++;
            } else {
                rc = tcc_error_noabort("wasm: unexpected fixup in dylink "
                                       "output for '%s'", x->name);
                goto out;
            }
        }
        for (j = 0; j < fn->nspfixs; j++)
            wb_patch_uleb5(&fn->final_body, fn->spfixs[j],
                           (uint32_t)sp_gindex);
    }

    wb_init(&mod);
    wb_write(&mod, "\0asm\x01\0\0\0", 8);

    /* ---- type section (1) ------------------------------------------- */
    wb_init(&pay);
    wb_uleb(&pay, nb_wsigs);
    for (i = 0; i < nb_wsigs; i++) {
        wb_putc(&pay, 0x60);
        wb_uleb(&pay, wsigs[i].nparams);
        for (j = 0; j < wsigs[i].nparams; j++) wb_putc(&pay, wsigs[i].params[j]);
        if (wsigs[i].ret < 0) wb_uleb(&pay, 0);
        else { wb_uleb(&pay, 1); wb_putc(&pay, wsigs[i].ret); }
    }
    sec_begin(&mod, 1, &pay); wb_free(&pay);

    /* ---- import section (2) ------------------------------------------ */
    wb_init(&pay);
    wb_uleb(&pay, nimports + 4 + ngots);
    for (i = 0; i < nb_wfuns; i++) {
        if (wfuns[i].defined) continue;
        wb_name(&pay, wasm_import_module(wfuns[i].name));
        wb_name(&pay, wfuns[i].name);
        wb_putc(&pay, 0x00); /* func */
        wb_uleb(&pay, wfuns[i].sig);
    }
    wb_name(&pay, "env"); wb_name(&pay, "memory");
    wb_putc(&pay, 0x02); /* memory */
    wb_putc(&pay, 0x00); /* limits: min only */
    wb_uleb(&pay, mem_bytes / 65536);
    wb_name(&pay, "env"); wb_name(&pay, "__indirect_function_table");
    wb_putc(&pay, 0x01); /* table */
    wb_putc(&pay, 0x70); /* funcref */
    wb_putc(&pay, 0x00);
    wb_uleb(&pay, ndefined); /* the module's own window in the table */
    wb_name(&pay, "env"); wb_name(&pay, "__memory_base");
    wb_putc(&pay, 0x03); wb_putc(&pay, WT_I32); wb_putc(&pay, 0x00); /* immutable */
    wb_name(&pay, "env"); wb_name(&pay, "__table_base");
    wb_putc(&pay, 0x03); wb_putc(&pay, WT_I32); wb_putc(&pay, 0x00); /* immutable */
    for (i = 0; i < ngots; i++) {
        wb_name(&pay, gots[i].kind == DY_GOTFUNC ? "GOT.func" : "GOT.mem");
        wb_name(&pay, gots[i].name);
        wb_putc(&pay, 0x03); wb_putc(&pay, WT_I32);
        wb_putc(&pay, 0x01); /* mutable: one shared Global per symbol,
                                filled and updated by the linker */
    }
    sec_begin(&mod, 2, &pay); wb_free(&pay);

    /* ---- function section (3) ---------------------------------------- */
    wb_init(&pay);
    wb_uleb(&pay, ndefined);
    for (i = 0; i < nb_wfuns; i++)
        if (wfuns[i].defined) wb_uleb(&pay, wfuns[i].sig);
    sec_begin(&mod, 3, &pay); wb_free(&pay);

    /* ---- global section (6): the defined $__stack_pointer ------------ */
    wb_init(&pay);
    wb_uleb(&pay, 1);
    wb_putc(&pay, WT_I32); wb_putc(&pay, 0x01); /* mutable */
    wb_putc(&pay, 0x41); wb_sleb(&pay, 0); wb_putc(&pay, 0x0b); /* placeholder */
    sec_begin(&mod, 6, &pay); wb_free(&pay);

    /* ---- export section (7): defined non-static functions ------------- */
    wb_init(&pay);
    {
        int nexp = 0;
        for (i = 0; i < nb_wfuns; i++)
            if (wfuns[i].defined && !wfuns[i].is_static) nexp++;
        wb_uleb(&pay, nexp);
        for (i = 0; i < nb_wfuns; i++) {
            if (!wfuns[i].defined || wfuns[i].is_static) continue;
            wb_name(&pay, wfuns[i].name);
            wb_putc(&pay, 0x00);
            wb_uleb(&pay, wfuns[i].final_index);
        }
    }
    sec_begin(&mod, 7, &pay); wb_free(&pay);

    /* ---- element section (9): one segment at __table_base ------------- */
    wb_init(&pay);
    wb_uleb(&pay, 1);
    wb_uleb(&pay, 0); /* active, table 0 */
    wb_putc(&pay, 0x23); wb_uleb(&pay, 1); wb_putc(&pay, 0x0b); /* global.get */
    wb_uleb(&pay, ndefined);
    for (i = 0; i < nb_wfuns; i++)
        if (wfuns[i].defined) wb_uleb(&pay, wfuns[i].final_index);
    sec_begin(&mod, 9, &pay); wb_free(&pay);

    /* ---- code section (10) -------------------------------------------- */
    wb_init(&pay);
    wb_uleb(&pay, ndefined);
    for (i = 0; i < nb_wfuns; i++) {
        WBuf *body;
        if (!wfuns[i].defined) continue;
        body = &wfuns[i].final_body;
        wb_uleb(&pay, body->len + 13);
        wb_uleb(&pay, 6);
        wb_uleb(&pay, 8);  wb_putc(&pay, WT_I32);
        wb_uleb(&pay, 8);  wb_putc(&pay, WT_F64);
        wb_uleb(&pay, 43); wb_putc(&pay, WT_I32);
        wb_uleb(&pay, 4);  wb_putc(&pay, 0x7b);
        wb_uleb(&pay, 2);  wb_putc(&pay, WT_I32);
        wb_uleb(&pay, 1);  wb_putc(&pay, WT_I64);
        wb_write(&pay, body->data, body->len);
    }
    sec_begin(&mod, 10, &pay); wb_free(&pay);

    /* ---- data section (11): one segment at __memory_base -------------- */
    if (img_end > 0) {
        static const unsigned char zeros[1024];
        WBuf img;
        wb_init(&img);
        while (img.len < (int)img_end) {
            int n = (int)img_end - img.len;
            if (n > 1024) n = 1024;
            wb_write(&img, zeros, n);
        }
        for (i = 1; i < s->nb_sections; i++) {
            Section *sec = s->sections[i];
            if (wsec_base[i] == (addr_t)-1 || sec->data_offset == 0 ||
                sec->sh_type == SHT_NOBITS) continue;
            memcpy(img.data + wsec_base[i], sec->data, sec->data_offset);
        }
        wb_init(&pay);
        wb_uleb(&pay, 1);
        wb_uleb(&pay, 0); /* active, memory 0 */
        wb_putc(&pay, 0x23); wb_uleb(&pay, 0); wb_putc(&pay, 0x0b);
        wb_uleb(&pay, img.len);
        wb_write(&pay, img.data, img.len);
        sec_begin(&mod, 11, &pay); wb_free(&pay);
        wb_free(&img);
    }

    /* ---- the unit file: the metadata object and the base64 body
       in the JS envelope, keyed by the output name.  The metadata
       exists only here; the module bytes in `mod` are the body. */
    {
        WBuf text;
        unsigned char digest[32];
        char hex[65];
        dy_sha256((const unsigned char *)mod.data, mod.len, digest);
        for (i = 0; i < 32; i++)
            snprintf(hex + 2 * i, 3, "%02x", digest[i]);
        wb_init(&text);
        wb_write(&text,
            "globalThis.__TINYC_DYLINK_REGISTRY__ ??= Object.create(null);\n",
            (int)strlen("globalThis.__TINYC_DYLINK_REGISTRY__ ??= Object.create(null);\n"));
        wb_write(&text, "globalThis.__TINYC_DYLINK_REGISTRY__[", (int)strlen("globalThis.__TINYC_DYLINK_REGISTRY__["));
        dy_json_str(&text, filename);
        wb_write(&text, "] = {", (int)strlen("] = {"));
        wb_write(&text, "\"version\":1,\"flavor\":", (int)strlen("\"version\":1,\"flavor\":"));
        dy_json_num(&text, s->wasm_dylink_flavor);
        wb_write(&text, ",\"mem_size\":", (int)strlen(",\"mem_size\":"));
        dy_json_num(&text, (uint32_t)sp_init);
        wb_write(&text, ",\"mem_align\":16,\"bss_off\":", (int)strlen(",\"mem_align\":16,\"bss_off\":"));
        dy_json_num(&text, (uint32_t)bss_off);
        wb_write(&text, ",\"bss_size\":", (int)strlen(",\"bss_size\":"));
        dy_json_num(&text, (uint32_t)bss_size);
        wb_write(&text, ",\"table_size\":", (int)strlen(",\"table_size\":"));
        dy_json_num(&text, ndefined);
        wb_write(&text, ",\"needed\":[", (int)strlen(",\"needed\":["));
        for (i = 0; i < s->nb_wasm_dylink_needs; i++) {
            if (i) wb_putc(&text, ',');
            dy_json_str(&text, s->wasm_dylink_needs[i]);
        }
        /* The GOT.func symbols static data references: the loader
           binds these eagerly (the init copies their values into
           data slots, where a later binding could not reach). */
        wb_write(&text, "],\"data_got_func\":[", (int)strlen("],\"data_got_func\":["));
        {
            int first = 1;
            for (i = 0; i < ndgots; i++) {
                int dup = 0, k;
                if (dgots[i].kind != DY_GOTFUNC) continue;
                for (k = 0; k < i; k++)
                    if (dgots[k].kind == DY_GOTFUNC &&
                        !strcmp(dgots[k].name, dgots[i].name)) {
                        dup = 1;
                        break;
                    }
                if (dup) continue;
                if (!first) wb_putc(&text, ',');
                first = 0;
                dy_json_str(&text, dgots[i].name);
            }
        }
        wb_write(&text, "],\"exports\":[", (int)strlen("],\"exports\":["));
        {
            int first = 1;
            for (i = 0; i < nb_wfuns; i++) {
                if (!wfuns[i].defined || wfuns[i].is_static ||
                    i == init_wf || i == fini_wf) continue;
                if (!first) wb_putc(&text, ',');
                first = 0;
                wb_write(&text, "{\"name\":", (int)strlen("{\"name\":"));
                dy_json_str(&text, wfuns[i].name);
                wb_write(&text, ",\"kind\":0,\"value\":", (int)strlen(",\"kind\":0,\"value\":"));
                dy_json_num(&text, wasm_defined_order(i));
                wb_putc(&text, '}');
            }
            for (i = 0; i < nvars; i++) {
                if (!first) wb_putc(&text, ',');
                first = 0;
                wb_write(&text, "{\"name\":", (int)strlen("{\"name\":"));
                dy_json_str(&text, vars[i].name);
                wb_write(&text, ",\"kind\":1,\"value\":", (int)strlen(",\"kind\":1,\"value\":"));
                dy_json_num(&text, (uint32_t)vars[i].value);
                wb_putc(&text, '}');
            }
        }
        wb_write(&text, "],\"hash\":\"", (int)strlen("],\"hash\":\""));
        wb_write(&text, hex, 64);
        wb_write(&text, "\",\"body\":\"", (int)strlen("\",\"body\":\""));
        dy_b64(&text, (const unsigned char *)mod.data, mod.len);
        wb_write(&text, "\"};\n", (int)strlen("\"};\n"));

        f = fopen(filename, "wb");
        if (!f) {
            wb_free(&text);
            wb_free(&mod);
            rc = tcc_error_noabort("wasm: could not write '%s'", filename);
            goto out;
        }
        fwrite(text.data, 1, text.len, f);
        fclose(f);
        wb_free(&text);
    }
    wb_free(&mod);
out:
    tcc_free(gots);
    tcc_free(ctors);
    tcc_free(dtors);
    tcc_free(vars);
    tcc_free(drels);
    tcc_free(dgots);
    return rc;
}

static int wasm_output_module(TCCState *s, const char *filename)
{
    WBuf mod, pay;
    FILE *f;
    int i, j, nimports = 0, ndefined = 0, total;
    addr_t data_end = 1024, sp_init, mem_bytes;

    if (s->output_type == TCC_OUTPUT_OBJ)
        return tcc_error_noabort("wasm: object-file output not supported "
                                 "(single module output only)");
    if (s->output_type == TCC_OUTPUT_MEMORY)
        return tcc_error_noabort("wasm: -run / in-memory execution not "
                                 "supported for the wasm target");
    if (s->wasm_dylink_flavor)
        return wasm_output_dylink(s, filename);

    /* ---- assign final function indices: imports first ------------- */
    for (i = 0; i < nb_wfuns; i++) {
        if (wfuns[i].defined) continue;
        wfuns[i].is_import = 1;
        wfuns[i].final_index = nimports++;
    }
    for (i = 0; i < nb_wfuns; i++) {
        if (!wfuns[i].defined) continue;
        wfuns[i].final_index = nimports + ndefined++;
    }
    total = nimports + ndefined;
    if (total == 0)
        return tcc_error_noabort("wasm: no functions were compiled");

    /* ---- lay out static data sections in linear memory ------------ */
    wsec_base = tcc_mallocz(s->nb_sections * sizeof(addr_t));
    for (i = 0; i < s->nb_sections; i++) wsec_base[i] = (addr_t)-1;
    for (i = 1; i < s->nb_sections; i++) {
        Section *sec = s->sections[i];
        const char *nm = sec->name;
        if (!nm) continue;
        if (!strcmp(nm, ".rodata") || !strncmp(nm, ".rodata.", 8) ||
            !strcmp(nm, ".data") || !strncmp(nm, ".data.", 6) ||
            !strcmp(nm, ".bss")) {
            data_end = (data_end + 15) & -16;
            wsec_base[i] = data_end;
            data_end += sec->data_offset;
        }
    }
    sp_init = ((data_end + 15) & -16) + 65536; /* 64 KiB C stack */
    mem_bytes = (sp_init + 65535) & -65536;

    /* ---- apply the data-section relocations against the layout:
       the standalone layout is absolute (memory starts at 0), so
       the patched values are final ------------------------------- */
    wasm_apply_data_relocs(s, NULL, NULL, NULL, NULL);

    /* ---- patch fixups in every defined function body -------------- */
    for (i = 0; i < nb_wfuns; i++) {
        WFunc *fn = &wfuns[i];
        if (!fn->defined) continue;
        for (j = 0; j < fn->nfixs; j++) {
            WFix *x = &fn->fixs[j];
            if (x->kind == WFIX_CALL) {
                WFunc *callee = NULL;
                int k;
                for (k = 0; k < nb_wfuns; k++)
                    if (!strcmp(wfuns[k].name, x->name)) { callee = &wfuns[k]; break; }
                if (!callee)
                    return tcc_error_noabort("wasm: call to unknown function '%s'",
                                             x->name);
                wb_patch_uleb5(&fn->final_body, x->pos, callee->final_index);
            } else { /* WFIX_ADDR */
                addr_t a;
                if (x->shndx >= 0) {
                    /* address from the section base assigned above
                       plus the offset captured when the fixup was
                       created (anonymous and local symbols) */
                    if (x->shndx >= s->nb_sections ||
                        wsec_base[x->shndx] == (addr_t)-1)
                        return tcc_error_noabort("wasm: cannot resolve "
                                                 "address of '%s'", x->name);
                    a = wsec_base[x->shndx] + x->value;
                } else if (wasm_sym_addr(s, x->name, &a) != 0)
                    return tcc_error_noabort("wasm: cannot resolve address of '%s'",
                                             x->name);
                wb_patch_sleb5(&fn->final_body, x->pos, (int32_t)(a + x->addend));
            }
        }
    }

    wb_init(&mod);
    wb_write(&mod, "\0asm\x01\0\0\0", 8);

    /* ---- type section (1) ------------------------------------------ */
    wb_init(&pay);
    wb_uleb(&pay, nb_wsigs);
    for (i = 0; i < nb_wsigs; i++) {
        wb_putc(&pay, 0x60);
        wb_uleb(&pay, wsigs[i].nparams);
        for (j = 0; j < wsigs[i].nparams; j++) wb_putc(&pay, wsigs[i].params[j]);
        if (wsigs[i].ret < 0) wb_uleb(&pay, 0);
        else { wb_uleb(&pay, 1); wb_putc(&pay, wsigs[i].ret); }
    }
    sec_begin(&mod, 1, &pay); wb_free(&pay);

    /* ---- import section (2) ---------------------------------------- */
    if (nimports) {
        wb_init(&pay);
        wb_uleb(&pay, nimports);
        for (i = 0; i < nb_wfuns; i++) {
            if (wfuns[i].defined) continue;
            wb_name(&pay, wasm_import_module(wfuns[i].name));
            wb_name(&pay, wfuns[i].name);
            wb_putc(&pay, 0x00); /* func */
            wb_uleb(&pay, wfuns[i].sig);
        }
        sec_begin(&mod, 2, &pay); wb_free(&pay);
    }

    /* ---- function section (3) -------------------------------------- */
    wb_init(&pay);
    wb_uleb(&pay, ndefined);
    for (i = 0; i < nb_wfuns; i++)
        if (wfuns[i].defined) wb_uleb(&pay, wfuns[i].sig);
    sec_begin(&mod, 3, &pay); wb_free(&pay);

    /* ---- table section (4): one funcref table, index == func index  */
    wb_init(&pay);
    wb_uleb(&pay, 1);
    wb_putc(&pay, 0x70); /* funcref */
    wb_putc(&pay, 0x00); /* limits: min only */
    wb_uleb(&pay, total);
    sec_begin(&mod, 4, &pay); wb_free(&pay);

    /* ---- memory section (5) ---------------------------------------- */
    wb_init(&pay);
    wb_uleb(&pay, 1);
    wb_putc(&pay, 0x00);
    wb_uleb(&pay, mem_bytes / 65536);
    sec_begin(&mod, 5, &pay); wb_free(&pay);

    /* ---- global section (6): $__stack_pointer ----------------------- */
    wb_init(&pay);
    wb_uleb(&pay, 1);
    wb_putc(&pay, WT_I32); wb_putc(&pay, 0x01); /* mutable */
    wb_putc(&pay, 0x41); wb_sleb(&pay, (int32_t)sp_init); wb_putc(&pay, 0x0b);
    sec_begin(&mod, 6, &pay); wb_free(&pay);

    /* ---- export section (7) ----------------------------------------- */
    wb_init(&pay);
    {
        int nexp = 1; /* memory */
        for (i = 0; i < nb_wfuns; i++)
            if (wfuns[i].defined && !wfuns[i].is_static) nexp++;
        wb_uleb(&pay, nexp);
        wb_name(&pay, "memory"); wb_putc(&pay, 0x02); wb_uleb(&pay, 0);
        for (i = 0; i < nb_wfuns; i++) {
            if (!wfuns[i].defined || wfuns[i].is_static) continue;
            wb_name(&pay, wfuns[i].name);
            wb_putc(&pay, 0x00);
            wb_uleb(&pay, wfuns[i].final_index);
        }
    }
    sec_begin(&mod, 7, &pay); wb_free(&pay);

    /* ---- element section (9): fill the table with all functions ----- */
    wb_init(&pay);
    wb_uleb(&pay, 1);
    wb_uleb(&pay, 0); /* active, table 0 */
    wb_putc(&pay, 0x41); wb_sleb(&pay, 0); wb_putc(&pay, 0x0b); /* offset 0 */
    wb_uleb(&pay, total);
    for (i = 0; i < total; i++) wb_uleb(&pay, i);
    sec_begin(&mod, 9, &pay); wb_free(&pay);

    /* ---- code section (10) ------------------------------------------ */
    wb_init(&pay);
    wb_uleb(&pay, ndefined);
    for (i = 0; i < nb_wfuns; i++) {
        WBuf *body;
        if (!wfuns[i].defined) continue;
        body = &wfuns[i].final_body;
        /* entry size = locals vec (13 bytes: count + 6 groups) + body.
           Local index layout after the parameters (must match
           wasm-gen.c wloc_reg/wloc_fp/wloc_scratch/wloc_vec/
           wloc_pc/wloc_i64): +0..7 i32 regs, +8..15 f64 regs,
           +16..58 i32 (pseudo fp/sp slots, $fp, call scratch),
           +59..62 v128 slots, +63 i32 dispatch $pc, +64 i32
           carry (llong add/sub), +65 i64 scratch */
        wb_uleb(&pay, body->len + 13);
        wb_uleb(&pay, 6);                    /* 6 local groups */
        wb_uleb(&pay, 8);  wb_putc(&pay, WT_I32); /* TCC int regs 0..7 */
        wb_uleb(&pay, 8);  wb_putc(&pay, WT_F64); /* TCC float regs 8..15 */
        wb_uleb(&pay, 43); wb_putc(&pay, WT_I32); /* pseudo fp/sp, $fp, scratch */
        wb_uleb(&pay, 4);  wb_putc(&pay, 0x7b);   /* v128 vector slots */
        wb_uleb(&pay, 2);  wb_putc(&pay, WT_I32); /* dispatch $pc, carry */
        wb_uleb(&pay, 1);  wb_putc(&pay, WT_I64); /* i64 scratch */
        wb_write(&pay, body->data, body->len);
    }
    sec_begin(&mod, 10, &pay); wb_free(&pay);

    /* ---- data section (11) ------------------------------------------- */
    {
        int nseg = 0;
        for (i = 1; i < s->nb_sections; i++)
            if (wsec_base[i] != (addr_t)-1 && s->sections[i]->data_offset > 0 &&
                s->sections[i]->sh_type != SHT_NOBITS) nseg++;
        if (nseg) {
            wb_init(&pay);
            wb_uleb(&pay, nseg);
            for (i = 1; i < s->nb_sections; i++) {
                Section *sec = s->sections[i];
                if (wsec_base[i] == (addr_t)-1 || sec->data_offset == 0 ||
                    sec->sh_type == SHT_NOBITS) continue;
                wb_uleb(&pay, 0); /* active, memory 0 */
                wb_putc(&pay, 0x41); wb_sleb(&pay, (int32_t)wsec_base[i]);
                wb_putc(&pay, 0x0b);
                wb_uleb(&pay, sec->data_offset);
                wb_write(&pay, sec->data, sec->data_offset);
            }
            sec_begin(&mod, 11, &pay); wb_free(&pay);
        }
    }
    f = fopen(filename, "wb");
    if (!f) {
        wb_free(&mod);
        return tcc_error_noabort("wasm: could not write '%s'", filename);
    }
    fwrite(mod.data, 1, mod.len, f);
    fclose(f);
    wb_free(&mod);
    return 0;
}

/* Public entry (declared in tcc.h, called from tcc_output_file):
   enter the TCC state so the TCC_STATE_VAR macros used in this file
   (symtab_section and friends) resolve against the right state. */
ST_FUNC int wasm_output_file(TCCState *s, const char *filename)
{
    int r;
    tcc_enter_state(s);
    r = wasm_output_module(s, filename);
    if (r == 0)
        wasm_registry_note_output();
    tcc_exit_state(s);
    return r;
}

/* tcc_run is explicitly NOT supported for the wasm target: the
   generated wasm code cannot execute inside the TCC process.
   Running the module is the host's job, with its own wasm engine
   (browser, Node, wasmtime, a JVM/embedded runtime, ...).
   Upstream tccrun.c only defines tcc_run under TCC_IS_NATIVE
   (tccrun.c:24), so a cross build would otherwise leave libtcc
   users with an undefined symbol instead of this clear error.
   The CLI path is covered too: libtcc.c's TCC_OPTION_run handling
   is patched by build (see patches/apply-patches.py) to return a
   wasm-specific message, and wasm_output_module rejects
   TCC_OUTPUT_MEMORY above. */
LIBTCCAPI int tcc_run(TCCState *s1, int argc, char **argv)
{
    int r;
    (void)argc; (void)argv;
    tcc_enter_state(s1);
    r = tcc_error_noabort(
        "wasm: tcc_run is not supported for the wasm target; "
        "instantiate the generated module with a host wasm engine");
    tcc_exit_state(s1);
    return r;
}

#endif /* !TARGET_DEFS_ONLY */

