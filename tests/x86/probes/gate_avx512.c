/* AVX-512 gating pair: a 512-bit byte add through the raw
   builtin (vop2 opcode 3: add, 64 x 8-bit lanes).  Below
   x86-64-v4 the compile must fail naming AVX512BW; at v4 it
   runs. */
typedef signed char v64qi __attribute__((__vector_size__(64)));

int main(void)
{
    v64qi a, b, r;
    int i;
    for (i = 0; i < 64; i++) {
        a[i] = 1;
        b[i] = 2;
    }
    r = __builtin_ia32_vop2(3, a, b, v64qi);
    return r[0] + r[63] - 6; /* 3 + 3 - 6 */
}
