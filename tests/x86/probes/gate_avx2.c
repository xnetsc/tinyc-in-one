/* AVX2 gating pair: a 256-bit integer add through the raw
   builtin (vop2 opcode 10: add, 8 x 32-bit lanes).  Below
   x86-64-v3 the compile must fail naming AVX2; at v3 it runs. */
typedef int v8si __attribute__((__vector_size__(32)));

int main(void)
{
    v8si a = { 1, 2, 3, 4, 5, 6, 7, 8 };
    v8si b = { 10, 20, 30, 40, 50, 60, 70, 80 };
    v8si r = __builtin_ia32_vop2(10, a, b, v8si);
    return r[0] + r[7] - 99; /* 11 + 88 - 99 */
}
