/* test-varargs-printf.c - calls to an imported variadic
 * function (the stdio shim ABI: fixed arguments plus a trailing
 * va_list pointer).  The host captures printf's output; the
 * runner asserts the exact text. */
int printf(const char *fmt, ...);

int probe(void)
{
    printf("n=%d s=%s x=%x c=%c %% ll=%lld f=%f u=%u\n",
           -7, "ok", 255, 'q', 1234567890123LL, 2.5, 4000000000u);
    printf("plain\n");
    return 0;
}
