/* dlfcn.h - dynamic linking, Darwin.
 *
 * Written for this package from the POSIX <dlfcn.h>
 * requirements with the Darwin flag values (RTLD_LOCAL is 4 and
 * RTLD_GLOBAL is 8 here; the glibc assignment differs) and the
 * Darwin Dl_info layout.
 */

#ifndef _TINYC_DARWIN_DLFCN_H
#define _TINYC_DARWIN_DLFCN_H

#include <stddef.h>

#define RTLD_LAZY	0x1
#define RTLD_NOW	0x2
#define RTLD_LOCAL	0x4
#define RTLD_GLOBAL	0x8
#define RTLD_NOLOAD	0x10
#define RTLD_NODELETE	0x80
#define RTLD_FIRST	0x100

#define RTLD_NEXT	((void *)-1)
#define RTLD_DEFAULT	((void *)0)
#define RTLD_SELF	((void *)-3)
#define RTLD_MAIN_ONLY	((void *)-5)

typedef struct {
	const char *dli_fname;
	void *dli_fbase;
	const char *dli_sname;
	void *dli_saddr;
} Dl_info;

void *dlopen(const char *path, int mode);
void *dlsym(void *handle, const char *symbol);
int dlclose(void *handle);
char *dlerror(void);
int dladdr(const void *addr, Dl_info *info);

#endif
