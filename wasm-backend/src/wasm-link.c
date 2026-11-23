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
        /* entry size = locals vec (9 bytes: count + 4 groups) + body.
           Local index layout after the parameters (must match
           wasm-gen.c wloc_reg/wloc_fp/wloc_scratch/wloc_vec):
           +0..7 i32 regs, +8..15 f64 regs, +16..58 i32 (pseudo
           fp/sp slots, $fp, call scratch), +59..62 v128 slots */
        wb_uleb(&pay, body->len + 9);
        wb_uleb(&pay, 4);                    /* 4 local groups */
        wb_uleb(&pay, 8);  wb_putc(&pay, WT_I32); /* TCC int regs 0..7 */
        wb_uleb(&pay, 8);  wb_putc(&pay, WT_F64); /* TCC float regs 8..15 */
        wb_uleb(&pay, 43); wb_putc(&pay, WT_I32); /* pseudo fp/sp, $fp, scratch */
        wb_uleb(&pay, 4);  wb_putc(&pay, 0x7b);   /* v128 vector slots */
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

