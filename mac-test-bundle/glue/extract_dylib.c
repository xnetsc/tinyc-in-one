/* extract_dylib.c - the macOS dual-form glue step.
 *
 * The dual-form distribution file is a Mach-O executable carrying the
 * library form of the same build as a prebuilt section
 * (__TINYC,__dylib). dyld cannot dlopen the executable itself
 * (MH_PIE is not MH_DYLIB), so the glue extracts the embedded dylib
 * bytes into a cache directory and dlopens that file instead.
 *
 * Usage: extract_dylib <dual-form-executable> <output-dylib-path>
 * Exit status 0 on success; the extracted byte count is printed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <mach-o/loader.h>

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <dual-form-executable> <output-dylib>\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize <= 0) { fprintf(stderr, "empty file\n"); fclose(f); return 1; }
    unsigned char *buf = malloc((size_t)fsize);
    if (!buf || fread(buf, 1, (size_t)fsize, f) != (size_t)fsize) {
        fprintf(stderr, "read failed\n"); fclose(f); return 1;
    }
    fclose(f);

    const struct mach_header_64 *hdr = (const struct mach_header_64 *)buf;
    if (hdr->magic != MH_MAGIC_64) {
        fprintf(stderr, "not a 64-bit Mach-O (magic 0x%x)\n", hdr->magic);
        return 1;
    }
    const unsigned char *p = buf + sizeof(*hdr);
    uint64_t found_off = 0, found_size = 0;
    for (uint32_t i = 0; i < hdr->ncmds; i++) {
        const struct load_command *lc = (const struct load_command *)p;
        if (lc->cmd == LC_SEGMENT_64) {
            const struct segment_command_64 *seg =
                (const struct segment_command_64 *)p;
            if (strncmp(seg->segname, "__TINYC", 16) == 0) {
                const struct section_64 *sec =
                    (const struct section_64 *)(p + sizeof(*seg));
                for (uint32_t j = 0; j < seg->nsects; j++, sec++) {
                    if (strncmp(sec->sectname, "__dylib", 16) == 0) {
                        found_off = sec->offset;
                        found_size = sec->size;
                    }
                }
            }
        }
        p += lc->cmdsize;
    }
    if (!found_size) {
        fprintf(stderr, "no __TINYC,__dylib section found\n");
        return 1;
    }
    FILE *o = fopen(argv[2], "wb");
    if (!o) { perror(argv[2]); return 1; }
    if (fwrite(buf + found_off, 1, (size_t)found_size, o) != (size_t)found_size) {
        fprintf(stderr, "write failed\n"); fclose(o); return 1;
    }
    fclose(o);
    chmod(argv[2], 0755);
    printf("extracted %llu bytes -> %s\n",
           (unsigned long long)found_size, argv[2]);
    return 0;
}
