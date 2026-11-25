/* SSE4.1 gating pair: a rounding operation through the raw
   builtin (vop1i opcode 81: round, 4 x float, imm 0 = nearest
   even).  At the default level the compile must fail naming
   SSE41; at x86-64-v2 it runs. */
typedef float v4sf __attribute__((__vector_size__(16)));

int main(void)
{
    v4sf a = { 1.5f, 2.5f, 3.5f, 4.5f };
    v4sf r = __builtin_ia32_vop1i(81, a, 0, v4sf);
    return (int)(r[0] + r[1] + r[2] + r[3]) - 12; /* 2+2+4+4-12 */
}
