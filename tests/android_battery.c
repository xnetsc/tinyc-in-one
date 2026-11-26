/* Hosted battery for the android targets: one translation unit
 * that pulls a representative facility from each part of the
 * bundled bionic header set (stdio, stdlib, string, math,
 * stdatomic, dlfcn), so the matrix smoke test links it against
 * the bundled crt objects and link-time stubs and checks the
 * product's PT_INTERP and DT_NEEDED. The program itself runs
 * only on Android; here it is a link-level check. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdatomic.h>
#include <dlfcn.h>

static atomic_int counter;

int main(void)
{
    char buf[64];
    void *p;
    double v;
    void *h;

    strcpy(buf, "tinyc-android");
    if (strcmp(buf, "tinyc-android") != 0)
        return 1;
    p = malloc(128);
    if (!p)
        return 2;
    free(p);
    v = sqrt(144.0) + sin(0.0) + pow(2.0, 3.0);
    if (v < 19.9 || v > 20.1)
        return 3;
    atomic_fetch_add(&counter, 1);
    if (atomic_load(&counter) != 1)
        return 4;
    h = dlopen(NULL, RTLD_NOW);
    if (!h)
        return 5;
    snprintf(buf, sizeof buf, "%d", 42);
    printf("%s ok\n", buf);
    return 0;
}
