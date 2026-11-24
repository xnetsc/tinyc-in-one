/* minimal runtime for freestanding aarch64 probes: TCC emits
   memset/memcpy calls for vector and struct initialization */
#include <stddef.h>
void *memset(void *d, int c, size_t n)
{
    unsigned char *p = d;
    while (n--)
        *p++ = (unsigned char)c;
    return d;
}
void *memcpy(void *d, const void *s, size_t n)
{
    unsigned char *p = d;
    const unsigned char *q = s;
    while (n--)
        *p++ = *q++;
    return d;
}
void *memmove(void *d, const void *s, size_t n)
{
    return memcpy(d, s, n);
}
