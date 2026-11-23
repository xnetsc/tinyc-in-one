typedef int v4si __attribute__((vector_size(16)));
typedef float v4sf __attribute__((vector_size(16)));
typedef short v8hi __attribute__((vector_size(16)));
typedef signed char v16qi __attribute__((vector_size(16)));
typedef int v2si __attribute__((vector_size(8)));

int sz(void) { return sizeof(v4si); }

/* splat + add + extract (direct mapping) */
int t_add(int x, int y)
{
    v4si a = __builtin_wasm_splat(17, x, v4si);
    v4si b = __builtin_wasm_splat(17, y, v4si);
    v4si c = a + b;
    return __builtin_wasm_extract(27, c, 2, int);
}

/* brace init, load/store through memory, mul */
int t_arr(void)
{
    int in[4] = { 1, 2, 3, 4 };
    int out[4];
    v4si a = *(v4si *)in;
    v4si b = a * a;
    *(v4si *)out = b;
    return out[0] + out[1] + out[2] + out[3]; /* 1+4+9+16 = 30 */
}

/* local brace initializer */
int t_init(void)
{
    v4si a = { 10, 20, 30, 40 };
    return a[0] + a[1] + a[2] + a[3]; /* 100 */
}

/* subscript write + read on a named local */
int t_sub(void)
{
    v4si a = __builtin_wasm_splat(17, 7, v4si);
    a[2] = 100;
    return a[0] * 1000 + a[2]; /* 7100 */
}

/* comparison produces a -1/0 mask vector */
int t_cmp(int x)
{
    v4si a = __builtin_wasm_splat(17, x, v4si);
    v4si b = __builtin_wasm_splat(17, 10, v4si);
    v4si m = (a > b);
    v4si e = (a == b);
    return __builtin_wasm_extract(27, m, 1, int)
         + __builtin_wasm_extract(27, e, 3, int);
}

/* vector parameter + vector return value */
v4si t_id(v4si x) { return x; }
int t_call(int v)
{
    v4si a = __builtin_wasm_splat(17, v, v4si);
    v4si b = t_id(a + a);
    return __builtin_wasm_extract(27, b, 1, int); /* 2*v */
}

/* float vector: add, compare mask out as int */
int t_fadd(void)
{
    v4sf a = { 1.5f, 2.5f, 3.5f, 4.5f };
    v4sf b = a + a;
    v4sf e = { 3.0f, 5.0f, 7.0f, 9.0f };
    v4si m = (b == e);
    return __builtin_wasm_extract(27, m, 0, int)
         + __builtin_wasm_extract(27, m, 3, int); /* -2 */
}

/* i8x16 multiply: no wasm instruction, gen composition path */
int t_i8mul(void)
{
    v16qi a = { 1, -2, 3, -4, 5, -6, 7, -8, 9, -10, 11, -12, 13, -14, 15, -16 };
    v16qi b = { 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    v16qi c = a * b;
    return c[0] + c[1] + c[15]; /* 16 + -30 + -16 = -30 */
}

/* 64-bit vector (8 bytes): load64_zero/store64_lane path */
int t_v2(void)
{
    v2si a = { 3, 4 };
    v2si b = a + a;
    int out[2];
    *(v2si *)out = b;
    return out[0] * 10 + out[1]; /* 68 */
}

/* uniform shift by a scalar count */
int t_shl(int x, int n)
{
    v4si a = __builtin_wasm_splat(17, x, v4si);
    v4si b = a << n;
    return __builtin_wasm_extract(27, b, 0, int);
}

/* shuffle via builtin (lanes are byte indices): reverse the i32 lanes */
int t_shuf(void)
{
    v4si a = { 1, 2, 3, 4 };
    v4si b = { 5, 6, 7, 8 };
    v4si c = __builtin_wasm_shuffle(a, b,
        12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);
    return __builtin_wasm_extract(27, c, 0, int) * 10
         + __builtin_wasm_extract(27, c, 3, int); /* 4*10+1 = 41 */
}

/* replace lane via builtin */
int t_rep(void)
{
    v4si a = { 1, 2, 3, 4 };
    v4si b = __builtin_wasm_replace(28, a, 2, 99);
    return b[0] + b[2]; /* 100 */
}

/* unary minus / bitwise not on vectors */
int t_un(void)
{
    v4si a = { 5, -3, 0, 7 };
    v4si b = -a;
    v4si c = ~a;
    return b[1] + c[2]; /* 3 + -1 = 2 */
}

/* global vector */
v4si g_vec = { 100, 200, 300, 400 };
int t_glob(void) { return g_vec[1] + g_vec[3]; /* 600 */ }

/* vop1: abs */
int t_abs(void)
{
    v4si a = { -5, 3, -1, 0 };
    v4si b = __builtin_wasm_vop1(160, a, v4si);
    return b[0] + b[2]; /* 6 */
}

/* vop2: saturating add i16 */
int t_sat(void)
{
    v8hi a = { 30000, -30000, 100, 0, 0, 0, 0, 0 };
    v8hi b = { 10000, -10000, 100, 0, 0, 0, 0, 0 };
    v8hi c = __builtin_wasm_vop2(143, a, b, v8hi);
    return c[0] + c[1] + c[2]; /* 32767 + -32768 + 200 = 199 */
}
