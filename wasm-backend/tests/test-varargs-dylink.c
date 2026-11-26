/* test-varargs-dylink.c - dylink main module: printf/sprintf
 * resolve to the process loader's stdio shims, whose ABI is
 * (fixed args..., va_list pointer).  sprintf's result is
 * verified in-guest; printf's text is checked by the runner. */
int printf(const char *fmt, ...);
int sprintf(char *dst, const char *fmt, ...);
static int streq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}
int main(void)
{
    char buf[64];
    int n = sprintf(buf, "%d-%s-%x-%lld", 42, "mid", 255,
                    1234567890123LL);
    if (!streq(buf, "42-mid-ff-1234567890123")) return 1;
    if (n != 23) return 2;
    printf("P:%d:%s:%f\n", 7, "tail", 0.5);
    return 0;
}
