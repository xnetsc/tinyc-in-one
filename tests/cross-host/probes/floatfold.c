/* Cross-host differential probe: floating-point constant
 * expressions. All initializers are constant expressions the
 * compiler folds before code generation; the stored bit
 * patterns fingerprint the folder's working precision and
 * rounding (the compiler's own long double width differs
 * between a native build and a wasm build of the compiler,
 * which is exactly what this probe is here to detect). */
static const double d_vals[] = {
    1.0 / 3.0,
    0.1,
    0.1 + 0.2,
    2.0 / 3.0 * 3.0,
    1e308 / 7.0,
    1e-308 * 3.0,
    3.14159265358979323846,
    2.71828182845904523536,
    0.5, 0.25, 1.5, 16777217.0,
    9007199254740993.0,
    1.0 / 10.0 + 2.0 / 10.0,
    (double)(1.0L / 3.0L),
    (double)(0.1L * 0.1L),
    6.02214076e23,
    1.7976931348623157e308 / 16.0,
    4.9e-324 * 2.0,
    0x1.fffffffffffffp+1023 / 3.0,
    123456789.123456789
};

static const float f_vals[] = {
    1.0f / 3.0f,
    0.1f,
    (float)(1.0 / 3.0),
    (float)(1.0L / 7.0L),
    3.4028235e38f / 2.0f,
    1.17549435e-38f / 2.0f,
    16777217.0f,
    0.1f + 0.2f,
    (float)3.14159265358979323846,
    1e-45f * 4.0f
};

static const long double ld_vals[] = {
    1.0L / 3.0L,
    0.1L,
    3.14159265358979323846264338327950288L,
    1e4932L / 11.0L,
    2.0L / 7.0L * 7.0L,
    (long double)(1.0 / 3.0),
    0.1L + 0.2L - 0.3L
};

static const int fi_vals[] = {
    (int)(1.0e9 + 0.5),
    (int)(-1.0e9 - 0.5),
    (int)(2147483647.5),
    (int)(-2147483648.5),
    (unsigned)(4294967295.0 / 2.0),
    (long long)(1.0e18 / 3.0),
    (1.5 > 1.25) + (0.1 + 0.2 == 0.3) * 2 + (1.0 / 3.0 * 3.0 == 1.0) * 4,
    sizeof(double) * 100 + sizeof(float) * 10 + (int)sizeof(long double)
};

double probe_floatfold_anchor(void)
{
    return d_vals[0] + f_vals[0] + (double)ld_vals[0] + fi_vals[0];
}
