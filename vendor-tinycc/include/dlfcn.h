#if defined(__wasm__) || defined(__wasm32__)

#ifndef _DLFCN_H
#define _DLFCN_H

/* Dynamic loading.  In the tinyc-web process model these
   functions are provided by the host shim layer; the RTLD_*
   values are the dylink ABI's own. */

#define RTLD_LAZY	1
#define RTLD_NOW	2
#define RTLD_GLOBAL	4
#define RTLD_LOCAL	8

void *dlopen(const char *filename, int flag);
char *dlerror(void);
void *dlsym(void *handle, const char *symbol);
int dlclose(void *handle);

#endif /* _DLFCN_H */

#else

/* This header ships in the wasm target's include set.  For any
   other target the system's own dlfcn.h is the right one. */
#include_next <dlfcn.h>

#endif
