/* Target-format long double software layer.

   All long double constant handling -- literal parsing, constant
   folding, casts and data emission -- is done in the *target*
   format, never in the host's long double, so any host produces
   byte-identical output for a given target.  The target format is
   selected by TCC_LDOUBLE_KIND (see tcc.h): IEEE double, x87
   80-bit extended or IEEE binary128 (quad).

   A value in flight is a canonical TccLdVal: sign, class, and a
   128-bit normalized mantissa whose top bit is bit 127:
       value = (-1)^sign * M * 2^(exp-127)     (TCC_LD_FINITE, M != 0)
   'sticky' records an inexact tail: the true value is M + delta,
   0 < delta < 1, in M units.  It is produced by parsing and by
   operations, and consumed by the final rounding in the encoder. */




typedef struct { uint64_t w[2]; } TccLDouble; /* w[0] = low 64 bits of the target layout */

enum { TCC_LD_ZERO, TCC_LD_FINITE, TCC_LD_INF, TCC_LD_NAN };
typedef struct {
    int sign;          /* 0 or 1 */
    int cls;           /* LD_* */
    int exp;           /* top-bit exponent when cls == TCC_LD_FINITE */
    uint64_t m[2];     /* m[1] = high 64 bits, m[0] = low 64 bits */
    int sticky;
} TccLdVal;

typedef struct {
    int p;             /* precision, including the integer/implicit bit */
    int emin;          /* top-bit exponent of the smallest normal */
    int emax;          /* top-bit exponent of the largest normal */
    int bias;
    int ebits;
    int x87;           /* explicit integer bit; exponent in a field above the mantissa */
} TccLdFmt;

static const TccLdFmt tcc_ld_fmt_float  = { 24, -126, 127, 127, 8, 0 };
static const TccLdFmt tcc_ld_fmt_double = { 53, -1022, 1023, 1023, 11, 0 };
static const TccLdFmt tcc_ld_fmt_x87    = { 64, -16382, 16383, 16383, 15, 1 };
static const TccLdFmt tcc_ld_fmt_quad   = { 113, -16382, 16383, 16383, 15, 0 };

/* The functions below are static in a header; translation units
   that include tcc.h without folding long doubles do not call
   them, hence the unused attribute. */
#define TCC_LD_FN static __attribute__((__unused__))

/* ---------------- 128-bit helpers (a[0] = low, a[1] = high) ---------------- */

TCC_LD_FN int tcc_ld_u128_top_bit(const uint64_t a[2]) /* 0..127, -1 when zero */
{
    if (a[1]) return 127 - __builtin_clzll(a[1]);
    if (a[0]) return 63 - __builtin_clzll(a[0]);
    return -1;
}

TCC_LD_FN int tcc_ld_u128_cmp(const uint64_t a[2], const uint64_t b[2])
{
    if (a[1] != b[1]) return a[1] > b[1] ? 1 : -1;
    if (a[0] != b[0]) return a[0] > b[0] ? 1 : -1;
    return 0;
}

/* r = a << s, 0 <= s < 128 */
TCC_LD_FN void tcc_ld_u128_shl(uint64_t r[2], const uint64_t a[2], int s)
{
    if (s == 0) { r[0] = a[0]; r[1] = a[1]; }
    else if (s < 64) {
        r[1] = (a[1] << s) | (a[0] >> (64 - s));
        r[0] = a[0] << s;
    } else {
        r[1] = a[0] << (s - 64);
        r[0] = 0;
    }
}

/* r = a >> s (0 <= s < 128); returns the shifted-out sticky (any lost bit) */
TCC_LD_FN int tcc_ld_u128_shr_sticky(uint64_t r[2], const uint64_t a[2], int s)
{
    int sticky = 0;
    if (s == 0) { r[0] = a[0]; r[1] = a[1]; return 0; }
    if (s < 64) {
        sticky = (a[0] & ((1ULL << s) - 1)) != 0;
        r[0] = (a[0] >> s) | (a[1] << (64 - s));
        r[1] = a[1] >> s;
    } else {
        sticky = (a[0] != 0) || (a[1] & ((1ULL << (s - 64)) - 1)) != 0;
        r[0] = a[1] >> (s - 64);
        r[1] = 0;
    }
    return sticky;
}

/* Round-to-nearest-even of (m + sticky tail) shifted right by sh bits.
   m is a 128-bit value; the result is a 128-bit integer.
   sh may be 0..128 (sh > 128 yields 0). */
TCC_LD_FN void tcc_ld_rne_shift(uint64_t r[2], const uint64_t m[2], int sh, int sticky)
{
    uint64_t hi, lo;
    int guard, rest;
    if (sh > 128) { r[0] = r[1] = 0; return; }
    if (sh == 128) {
        /* m / 2^128 lies in [0.5, 1) for normalized m: rounds to 1 iff
           m > 2^127, or m == 2^127 with a nonzero tail */
        r[1] = 0;
        r[0] = (m[1] > (1ULL << 63) ||
                (m[1] == (1ULL << 63) && (m[0] != 0 || sticky))) ? 1 : 0;
        return;
    }
    if (sh == 0) { r[0] = m[0]; r[1] = m[1]; return; }
    if (sh < 64) {
        lo = (m[0] >> sh) | (m[1] << (64 - sh));
        hi = m[1] >> sh;
        guard = (int)((m[0] >> (sh - 1)) & 1);
        rest = sticky || (sh > 1 && (m[0] & ((1ULL << (sh - 1)) - 1)) != 0);
    } else if (sh == 64) {
        lo = m[1]; hi = 0;
        guard = (int)(m[0] >> 63);
        rest = sticky || (m[0] & 0x7fffffffffffffffULL) != 0;
    } else {
        lo = m[1] >> (sh - 64); hi = 0;
        guard = (int)((m[1] >> (sh - 65)) & 1);
        rest = sticky || m[0] != 0 ||
               (sh > 65 && (m[1] & ((1ULL << (sh - 65)) - 1)) != 0);
    }
    if (guard && (rest || (lo & 1))) {
        lo += 1;
        if (lo == 0) hi += 1;
    }
    r[0] = lo; r[1] = hi;
}

/* Canonicalize an exact nonneg integer X (limbs little-endian) whose
   scale is value = X * 2^(scale_exp - 127): sets r's exp/m/sticky.
   X must be nonzero. */
TCC_LD_FN void tcc_ld_canon_from_big(const uint64_t *X, int nlimbs, int scale_exp,
                              int extra_sticky, TccLdVal *r)
{
    int i, bx = -1, drop, st;
    for (i = nlimbs - 1; i >= 0; i--)
        if (X[i]) { bx = i * 64 + 63 - __builtin_clzll(X[i]); break; }
    r->cls = TCC_LD_FINITE;
    r->exp = scale_exp + bx - 127;
    drop = bx - 127;
    st = extra_sticky;
    if (drop <= 0) {
        uint64_t t[2];
        t[0] = X[0]; t[1] = X[1];
        tcc_ld_u128_shl(r->m, t, -drop);
    } else {
        int limb_shift = drop / 64, bit_shift = drop % 64, j;
        r->m[0] = r->m[1] = 0;
        for (j = 0; j < 2; j++) {
            int src_i = j + limb_shift;
            uint64_t val = (src_i < nlimbs) ? X[src_i] : 0;
            if (bit_shift) {
                uint64_t hi = (src_i + 1 < nlimbs) ? X[src_i + 1] : 0;
                val = (val >> bit_shift) | (hi << (64 - bit_shift));
            }
            r->m[j] = val;
        }
        for (j = 0; j < drop && !st; j++)
            if ((X[j / 64] >> (j % 64)) & 1) st = 1;
    }
    r->sticky = st;
}

/* ---------------- decode / encode ---------------- */

TCC_LD_FN void tcc_ld_decode_fmt(const TccLdFmt *f, uint64_t blo, uint64_t bhi, TccLdVal *v)
{
    int fracbits = f->p - 1;
    uint64_t flo, fhi, expfield;
    int sign, k;

    v->sticky = 0;
    v->exp = 0;
    v->m[0] = v->m[1] = 0;
    if (f->x87) {
        uint64_t mant = blo;
        expfield = bhi & 0x7fff;
        sign = (int)((bhi >> 15) & 1);
        v->sign = sign;
        if (expfield == 0x7fff) {
            v->cls = (mant == (1ULL << 63)) ? TCC_LD_INF : TCC_LD_NAN;
            return;
        }
        if (expfield == 0) {
            if (mant == 0) { v->cls = TCC_LD_ZERO; return; }
            k = 63 - __builtin_clzll(mant);
            v->cls = TCC_LD_FINITE;
            v->exp = -16382 - 63 + k;
            v->m[1] = mant << (63 - k);
            return;
        }
        v->cls = TCC_LD_FINITE;
        v->exp = (int)expfield - f->bias;
        v->m[1] = mant;
        return;
    }

    if (fracbits <= 64) {
        flo = blo & ((fracbits == 64) ? ~0ULL : ((1ULL << fracbits) - 1));
        fhi = 0;
        expfield = (blo >> fracbits) & ((1ULL << f->ebits) - 1);
        sign = (int)((blo >> (fracbits + f->ebits)) & 1);
    } else {
        flo = blo;
        fhi = bhi & ((1ULL << (fracbits - 64)) - 1);
        expfield = (bhi >> (fracbits - 64)) & ((1ULL << f->ebits) - 1);
        sign = (int)((bhi >> (fracbits - 64 + f->ebits)) & 1);
    }
    v->sign = sign;
    if (expfield == (uint64_t)((1 << f->ebits) - 1)) {
        v->cls = (flo == 0 && fhi == 0) ? TCC_LD_INF : TCC_LD_NAN;
        return;
    }
    if (expfield == 0) {
        uint64_t fr[2];
        if (flo == 0 && fhi == 0) { v->cls = TCC_LD_ZERO; return; }
        fr[0] = flo; fr[1] = fhi;
        k = tcc_ld_u128_top_bit(fr);
        v->cls = TCC_LD_FINITE;
        v->exp = f->emin - fracbits + k;
        tcc_ld_u128_shl(v->m, fr, 127 - k);
        return;
    }
    v->cls = TCC_LD_FINITE;
    v->exp = (int)expfield - f->bias;
    {
        uint64_t fr[2];
        fr[0] = flo; fr[1] = fhi;
        tcc_ld_u128_shl(v->m, fr, 128 - f->p);
        v->m[1] |= (1ULL << 63); /* implicit bit */
    }
}

TCC_LD_FN void tcc_ld_place(const TccLdFmt *f, uint64_t out[2], int sign,
                     uint64_t expfield, const uint64_t F[2])
{
    int fracbits = f->p - 1;
    out[0] = out[1] = 0;
    if (f->x87) {
        out[0] = F[0];
        out[1] = ((uint64_t)(sign & 1) << 15) | expfield;
        return;
    }
    if (fracbits <= 64) {
        out[0] = F[0] | (expfield << fracbits) |
                 ((uint64_t)(sign & 1) << (fracbits + f->ebits));
    } else {
        out[0] = F[0];
        out[1] = F[1] | (expfield << (fracbits - 64)) |
                 ((uint64_t)(sign & 1) << (fracbits - 64 + f->ebits));
    }
}

TCC_LD_FN void tcc_ld_encode_fmt(const TccLdFmt *f, const TccLdVal *v, uint64_t out[2])
{
    uint64_t F[2];
    int e;

    out[0] = out[1] = 0;
    if (v->cls == TCC_LD_ZERO) {
        F[0] = F[1] = 0;
        tcc_ld_place(f, out, v->sign, 0, F);
        return;
    }
    if (v->cls == TCC_LD_INF || v->cls == TCC_LD_NAN) {
        if (v->cls == TCC_LD_INF) {
            F[0] = f->x87 ? (1ULL << 63) : 0;
            F[1] = 0;
        } else {
            /* canonical quiet NaN */
            if (f->x87) { F[0] = 0xc000000000000000ULL; F[1] = 0; }
            else if (f->p - 1 < 64) { F[0] = 1ULL << (f->p - 2); F[1] = 0; }
            else { F[0] = 0; F[1] = 1ULL << (f->p - 2 - 64); }
        }
        tcc_ld_place(f, out, v->sign, (1ULL << f->ebits) - 1, F);
        return;
    }
    e = v->exp;
    if (e > f->emax) goto overflow;
    if (e >= f->emin) {
        tcc_ld_rne_shift(F, v->m, 128 - f->p, v->sticky);
        /* a carry to 2^p shows as bit p of F */
        if ((f->p < 64 && ((F[0] >> f->p) & 1)) ||
            (f->p >= 64 && ((F[1] >> (f->p - 64)) & 1))) {
            F[0] = F[1] = 0;
            if (f->p - 1 < 64) F[0] = 1ULL << (f->p - 1);
            else F[1] = 1ULL << (f->p - 1 - 64);
            e += 1;
            if (e > f->emax) goto overflow;
        }
        if (!f->x87) {
            /* drop the implicit bit */
            if (f->p - 1 < 64) F[0] &= (1ULL << (f->p - 1)) - 1;
            else F[1] &= (1ULL << (f->p - 1 - 64)) - 1;
        }
        tcc_ld_place(f, out, v->sign, (uint64_t)(e + f->bias), F);
        return;
    }
    /* subnormal range (or zero) */
    {
        int sh = (128 - f->p) + (f->emin - e);
        uint64_t top[2];
        tcc_ld_rne_shift(F, v->m, sh, v->sticky);
        top[0] = top[1] = 0;
        if (f->p - 1 < 64) top[0] = 1ULL << (f->p - 1);
        else top[1] = 1ULL << (f->p - 1 - 64);
        if (F[0] == top[0] && F[1] == top[1]) {
            /* rounds up to the smallest normal */
            if (!f->x87) { F[0] = F[1] = 0; }
            tcc_ld_place(f, out, v->sign, 1, F);
            return;
        }
        tcc_ld_place(f, out, v->sign, 0, F);
        return;
    }
overflow:
    F[0] = f->x87 ? (1ULL << 63) : 0;
    F[1] = 0;
    tcc_ld_place(f, out, v->sign, (1ULL << f->ebits) - 1, F);
}

/* ---------------- arithmetic ---------------- */

TCC_LD_FN void tcc_ld_nan_result(TccLdVal *r, int sign)
{
    r->cls = TCC_LD_NAN; r->sign = sign; r->exp = 0;
    r->m[0] = r->m[1] = 0; r->sticky = 0;
}

TCC_LD_FN void tcc_ld_zero_result(TccLdVal *r, int sign)
{
    r->cls = TCC_LD_ZERO; r->sign = sign; r->exp = 0;
    r->m[0] = r->m[1] = 0; r->sticky = 0;
}

TCC_LD_FN void tcc_ld_inf_result(TccLdVal *r, int sign)
{
    r->cls = TCC_LD_INF; r->sign = sign; r->exp = 0;
    r->m[0] = r->m[1] = 0; r->sticky = 0;
}

/* magnitude compare of two finite nonzero values: -1, 0, 1 */
TCC_LD_FN int tcc_ld_mag_cmp(const TccLdVal *a, const TccLdVal *b)
{
    if (a->exp != b->exp) return a->exp > b->exp ? 1 : -1;
    return tcc_ld_u128_cmp(a->m, b->m);
}

/* X (6 limbs) = m << diff, exact; diff <= 200 */
TCC_LD_FN void tcc_ld_big_shift(uint64_t X[6], const uint64_t m[2], int diff)
{
    int i, limb_shift = diff / 64, bit_shift = diff % 64;
    for (i = 0; i < 6; i++) X[i] = 0;
    for (i = 0; i < 2; i++) {
        int j = i + limb_shift;
        X[j] |= m[i] << bit_shift;
        if (bit_shift) X[j + 1] |= m[i] >> (64 - bit_shift);
    }
}

/* a + b (sub == 0) or a - b (sub == 1); result canonical, unencoded */
TCC_LD_FN void tcc_ld_addsub(const TccLdVal *a, const TccLdVal *b, int sub, TccLdVal *r)
{
    int bsign = b->sign ^ (sub ? 1 : 0);
    const TccLdVal *big, *small;
    int diff, mc;

    if (a->cls == TCC_LD_NAN) { tcc_ld_nan_result(r, a->sign); return; }
    if (b->cls == TCC_LD_NAN) { tcc_ld_nan_result(r, bsign); return; }
    if (a->cls == TCC_LD_INF && b->cls == TCC_LD_INF) {
        if (a->sign == bsign) tcc_ld_inf_result(r, a->sign);
        else tcc_ld_nan_result(r, 0);
        return;
    }
    if (a->cls == TCC_LD_INF) { tcc_ld_inf_result(r, a->sign); return; }
    if (b->cls == TCC_LD_INF) { tcc_ld_inf_result(r, bsign); return; }
    if (a->cls == TCC_LD_ZERO && b->cls == TCC_LD_ZERO) {
        tcc_ld_zero_result(r, (a->sign == bsign) ? a->sign : 0);
        return;
    }
    if (a->cls == TCC_LD_ZERO) { *r = *b; r->sign = bsign; return; }
    if (b->cls == TCC_LD_ZERO) { *r = *a; return; }

    mc = tcc_ld_mag_cmp(a, b);
    if (mc == 0 && a->sign != bsign) { tcc_ld_zero_result(r, 0); return; }
    if (mc >= 0) { big = a; small = b; r->sign = a->sign; }
    else { big = b; small = a; r->sign = bsign; }
    diff = big->exp - small->exp;

    if (a->sign == bsign) {
        /* addition of magnitudes */
        if (diff > 200) {
            *r = *big;
            r->sign = (mc >= 0) ? a->sign : bsign;
            r->sticky = 1;
            return;
        }
        {
            uint64_t X[6];
            int i;
            tcc_ld_big_shift(X, big->m, diff);
            /* X += small.m */
            {
                uint64_t carry = 0;
                for (i = 0; i < 2; i++) {
                    uint64_t t = X[i] + carry;
                    uint64_t c1 = (t < X[i]);
                    uint64_t s = t + small->m[i];
                    uint64_t c2 = (s < t);
                    X[i] = s;
                    carry = c1 | c2;
                }
                for (i = 2; carry && i < 6; i++) {
                    X[i] += 1;
                    carry = (X[i] == 0);
                }
            }
            r->sign = (mc >= 0) ? a->sign : bsign;
            tcc_ld_canon_from_big(X, 6, small->exp, 0, r);
            return;
        }
    } else {
        /* subtraction of magnitudes: big - small, big >= small */
        if (diff > 200) {
            /* result = (big.m - 1) plus a positive tail below it */
            uint64_t T[2];
            T[0] = big->m[0]; T[1] = big->m[1];
            if (T[0] == 0) T[1] -= 1;
            T[0] -= 1;
            r->cls = TCC_LD_FINITE;
            r->sticky = 1;
            if (T[1] & (1ULL << 63)) {
                r->m[0] = T[0]; r->m[1] = T[1];
                r->exp = big->exp;
            } else {
                r->m[1] = (T[1] << 1) | (T[0] >> 63);
                r->m[0] = T[0] << 1;
                r->exp = big->exp - 1;
            }
            r->sign = (mc >= 0) ? a->sign : bsign;
            return;
        }
        {
            uint64_t X[6];
            int i, nonzero = 0;
            tcc_ld_big_shift(X, big->m, diff);
            /* X -= small.m */
            {
                uint64_t borrow = 0;
                for (i = 0; i < 2; i++) {
                    uint64_t t = X[i] - borrow;
                    uint64_t b1 = (t > X[i]);
                    uint64_t s = t - small->m[i];
                    uint64_t b2 = (s > t);
                    X[i] = s;
                    borrow = b1 | b2;
                }
                for (i = 2; borrow && i < 6; i++) {
                    borrow = (X[i] == 0);
                    X[i] -= 1;
                }
            }
            for (i = 0; i < 6; i++) nonzero |= (X[i] != 0);
            if (!nonzero) { tcc_ld_zero_result(r, 0); return; }
            r->sign = (mc >= 0) ? a->sign : bsign;
            tcc_ld_canon_from_big(X, 6, small->exp, 0, r);
            return;
        }
    }
}

/* 64x64 -> 128 multiply via 32-bit halves (portable) */
TCC_LD_FN void tcc_ld_mul64(uint64_t a, uint64_t b, uint64_t *hi, uint64_t *lo)
{
    uint64_t a0 = (uint32_t)a, a1 = a >> 32;
    uint64_t b0 = (uint32_t)b, b1 = b >> 32;
    uint64_t p0 = a0 * b0, p1 = a0 * b1, p2 = a1 * b0, p3 = a1 * b1;
    uint64_t t = p1 + (p0 >> 32) + (uint32_t)p2;
    *lo = (t << 32) | (uint32_t)p0;
    *hi = p3 + (p2 >> 32) + (t >> 32);
}

TCC_LD_FN void tcc_ld_mul(const TccLdVal *a, const TccLdVal *b, TccLdVal *r)
{
    int sign = a->sign ^ b->sign;
    if (a->cls == TCC_LD_NAN) { tcc_ld_nan_result(r, a->sign); return; }
    if (b->cls == TCC_LD_NAN) { tcc_ld_nan_result(r, b->sign); return; }
    if ((a->cls == TCC_LD_INF && b->cls == TCC_LD_ZERO) ||
        (b->cls == TCC_LD_INF && a->cls == TCC_LD_ZERO)) { tcc_ld_nan_result(r, 0); return; }
    if (a->cls == TCC_LD_INF || b->cls == TCC_LD_INF) { tcc_ld_inf_result(r, sign); return; }
    if (a->cls == TCC_LD_ZERO || b->cls == TCC_LD_ZERO) { tcc_ld_zero_result(r, sign); return; }
    {
        uint64_t P[4] = { 0, 0, 0, 0 };
        int i, j;
        for (i = 0; i < 2; i++) {
            uint64_t carry = 0;
            for (j = 0; j < 2; j++) {
                uint64_t h, l, t, c1, c2;
                tcc_ld_mul64(a->m[i], b->m[j], &h, &l);
                t = P[i + j] + carry;
                c1 = (t < P[i + j]);
                P[i + j] = t + l;
                c2 = (P[i + j] < t);
                carry = h + c1 + c2;
            }
            P[i + 2] += carry;
        }
        r->sign = sign;
        /* value = P * 2^(a.exp + b.exp - 254); canonical scale uses
           small_exp := a.exp + b.exp - 127 */
        tcc_ld_canon_from_big(P, 4, a->exp + b->exp - 127, 0, r);
    }
}

TCC_LD_FN void tcc_ld_div(const TccLdVal *a, const TccLdVal *b, TccLdVal *r)
{
    int sign = a->sign ^ b->sign;
    if (a->cls == TCC_LD_NAN) { tcc_ld_nan_result(r, a->sign); return; }
    if (b->cls == TCC_LD_NAN) { tcc_ld_nan_result(r, b->sign); return; }
    if (a->cls == TCC_LD_INF && b->cls == TCC_LD_INF) { tcc_ld_nan_result(r, 0); return; }
    if (a->cls == TCC_LD_ZERO && b->cls == TCC_LD_ZERO) { tcc_ld_nan_result(r, 0); return; }
    if (a->cls == TCC_LD_INF) { tcc_ld_inf_result(r, sign); return; }
    if (b->cls == TCC_LD_INF) { tcc_ld_zero_result(r, sign); return; }
    if (a->cls == TCC_LD_ZERO) { tcc_ld_zero_result(r, sign); return; }
    if (b->cls == TCC_LD_ZERO) { tcc_ld_inf_result(r, sign); return; }
    /* q = floor(A * 2^129 / B) by bit-by-bit long division;
       value = (q + rem/B) * 2^(a.exp - b.exp - 129) */
    {
        uint64_t rem[3] = { 0, 0, 0 };
        uint64_t q[3] = { 0, 0, 0 };
        int i;
        for (i = 256; i >= 0; i--) {
            int bit = 0;
            if (i >= 129) {
                int ai = i - 129;
                bit = (int)((a->m[ai / 64] >> (ai % 64)) & 1);
            }
            rem[2] = (rem[2] << 1) | (rem[1] >> 63);
            rem[1] = (rem[1] << 1) | (rem[0] >> 63);
            rem[0] = (rem[0] << 1) | (uint64_t)bit;
            if (rem[2] || rem[1] > b->m[1] ||
                (rem[1] == b->m[1] && rem[0] >= b->m[0])) {
                uint64_t old0 = rem[0], old1 = rem[1];
                uint64_t n0 = old0 - b->m[0];
                uint64_t borrow = (n0 > old0) ? 1 : 0;
                uint64_t mid = old1 - borrow;
                uint64_t borrow2 = (mid > old1) ? 1 : 0;
                uint64_t n1 = mid - b->m[1];
                borrow2 |= (n1 > mid) ? 1 : 0;
                rem[0] = n0; rem[1] = n1; rem[2] -= borrow2;
                if (i < 192) q[i / 64] |= 1ULL << (i % 64);
            }
        }
        r->sign = sign;
        tcc_ld_canon_from_big(q, 3, a->exp - b->exp - 2, (rem[0] | rem[1] | rem[2]) != 0, r);
    }
}

/* compare: returns -2 when unordered, else -1/0/1 for a vs b */
TCC_LD_FN int tcc_ld_cmp(const TccLdVal *a, const TccLdVal *b)
{
    int mc;
    if (a->cls == TCC_LD_NAN || b->cls == TCC_LD_NAN) return -2;
    if (a->cls == TCC_LD_INF && b->cls == TCC_LD_INF)
        return a->sign == b->sign ? 0 : (a->sign ? -1 : 1);
    if (a->cls == TCC_LD_INF) return a->sign ? -1 : 1;
    if (b->cls == TCC_LD_INF) return b->sign ? 1 : -1;
    if (a->cls == TCC_LD_ZERO && b->cls == TCC_LD_ZERO) return 0;
    if (a->sign != b->sign) return a->sign ? -1 : 1;
    if (a->cls == TCC_LD_ZERO) return a->sign ? 1 : -1;
    if (b->cls == TCC_LD_ZERO) return a->sign ? -1 : 1;
    mc = tcc_ld_mag_cmp(a, b);
    return a->sign ? -mc : mc;
}

/* ---------------- conversions ---------------- */

TCC_LD_FN void tcc_ld_from_u64(uint64_t x, int sign, TccLdVal *v)
{
    int k;
    v->sticky = 0;
    v->sign = sign;
    v->exp = 0;
    v->m[0] = v->m[1] = 0;
    if (x == 0) { v->cls = TCC_LD_ZERO; return; }
    k = 63 - __builtin_clzll(x);
    v->cls = TCC_LD_FINITE;
    v->exp = k;
    {
        uint64_t t[2];
        t[0] = x; t[1] = 0;
        tcc_ld_u128_shl(v->m, t, 127 - k);
    }
}

TCC_LD_FN void tcc_ld_from_i64(int64_t x, TccLdVal *v)
{
    uint64_t mag = (x < 0) ? (0 - (uint64_t)x) : (uint64_t)x;
    tcc_ld_from_u64(mag, x < 0, v);
}

/* Truncate toward zero into a 64-bit integer.  Overflow and NaN
   follow the x86-64 conversion convention: signed results take the
   "indefinite" value INT64_MIN; unsigned results take UINT64_MAX
   when too large, 0 when negative. */
TCC_LD_FN uint64_t tcc_ld_to_u64(const TccLdVal *v, int is_unsigned)
{
    uint64_t mag, t[2];
    if (v->cls == TCC_LD_ZERO) return 0;
    if (v->cls == TCC_LD_NAN) return is_unsigned ? ~0ULL : (1ULL << 63);
    if (v->cls == TCC_LD_INF) {
        if (is_unsigned) return v->sign ? 0 : ~0ULL;
        return 1ULL << 63;
    }
    if (v->exp < 0) return 0;
    if (v->exp >= 64) {
        if (is_unsigned) return v->sign ? 0 : ~0ULL;
        return 1ULL << 63;
    }
    tcc_ld_u128_shr_sticky(t, v->m, 127 - v->exp);
    mag = t[0];
    if (is_unsigned) {
        if (v->sign) return 0;
        return mag;
    }
    if (v->sign) {
        if (mag > (1ULL << 63)) return 1ULL << 63;
        return 0 - mag;
    }
    if (mag >= (1ULL << 63)) return 1ULL << 63;
    return mag;
}

TCC_LD_FN int tcc_ld_is_finite(const TccLdVal *v) { return v->cls == TCC_LD_FINITE || v->cls == TCC_LD_ZERO; }

/* ---------------- decimal parsing (exact bignum) ---------------- */

#define TCC_LD_BN_LIMBS 640 /* 32-bit limbs: 20480 bits */
typedef struct { uint32_t l[TCC_LD_BN_LIMBS]; int n; } TccLdBig; /* n >= 1 significant limbs */

TCC_LD_FN void tcc_ld_big_zero(TccLdBig *b) { b->n = 1; b->l[0] = 0; }
TCC_LD_FN int tcc_ld_big_is_zero(const TccLdBig *b) { return b->n == 1 && b->l[0] == 0; }

TCC_LD_FN int tcc_ld_big_bitlen(const TccLdBig *b)
{
    uint32_t top;
    if (tcc_ld_big_is_zero(b)) return 0;
    top = b->l[b->n - 1];
    return (b->n - 1) * 32 + 32 - __builtin_clz(top);
}

/* b *= m; returns 0 on overflow beyond capacity */
TCC_LD_FN int tcc_ld_big_mul_small(TccLdBig *b, uint32_t m)
{
    uint64_t carry = 0;
    int i;
    for (i = 0; i < b->n; i++) {
        uint64_t t = (uint64_t)b->l[i] * m + carry;
        b->l[i] = (uint32_t)t;
        carry = t >> 32;
    }
    if (carry) {
        if (b->n >= TCC_LD_BN_LIMBS) return 0;
        b->l[b->n++] = (uint32_t)carry;
    }
    return 1;
}

TCC_LD_FN void tcc_ld_big_add_small(TccLdBig *b, uint32_t a)
{
    uint64_t carry = a;
    uint64_t t;
    int i = 0;
    while (carry) {
        if (i >= b->n) b->l[b->n++] = 0;
        t = (uint64_t)b->l[i] + carry;
        b->l[i] = (uint32_t)t;
        carry = t >> 32;
        i++;
    }
}

/* b *= 10^k, k >= 0; returns 0 on overflow */
TCC_LD_FN int tcc_ld_big_mul_pow10(TccLdBig *b, int k)
{
    static const uint32_t pow10_[9] = { 1, 10, 100, 1000, 10000, 100000,
                                        1000000, 10000000, 100000000 };
    while (k >= 9) {
        if (!tcc_ld_big_mul_small(b, 1000000000u)) return 0;
        k -= 9;
    }
    if (k) return tcc_ld_big_mul_small(b, pow10_[k]);
    return 1;
}

/* b <<= s bits, s >= 0; returns 0 on overflow */
TCC_LD_FN int tcc_ld_big_shl(TccLdBig *b, int s)
{
    int limbs = s / 32, bits = s % 32, i, newn;
    newn = b->n + limbs + (bits ? 1 : 0);
    if (newn > TCC_LD_BN_LIMBS) return 0;
    for (i = newn - 1; i >= 0; i--) {
        uint32_t v = 0;
        int src = i - limbs;
        if (src >= 0 && src < b->n) v = b->l[src] << bits;
        if (bits && src - 1 >= 0 && src - 1 < b->n)
            v |= b->l[src - 1] >> (32 - bits);
        b->l[i] = v;
    }
    b->n = newn;
    while (b->n > 1 && b->l[b->n - 1] == 0) b->n--;
    return 1;
}

/* q = a / b, r = a % b (b > 0).  Bit-by-bit long division: slow but
   obviously correct; used only on the literal-parsing path. */
TCC_LD_FN void tcc_ld_big_divmod(const TccLdBig *a, const TccLdBig *b, TccLdBig *q, TccLdBig *r)
{
    TccLdBig rem, quo;
    int i, nb;
    tcc_ld_big_zero(&rem); tcc_ld_big_zero(&quo);
    quo.n = a->n;
    for (i = 0; i < a->n; i++) quo.l[i] = 0;
    nb = tcc_ld_big_bitlen(a);
    for (i = nb - 1; i >= 0; i--) {
        int bit = (a->l[i / 32] >> (i % 32)) & 1;
        /* rem = rem * 2 + bit */
        tcc_ld_big_shl(&rem, 1);
        if (bit) rem.l[0] |= 1;
        /* if rem >= b: rem -= b, quo bit i = 1 */
        {
            int cmp;
            if (rem.n != b->n) cmp = rem.n > b->n ? 1 : -1;
            else {
                int j;
                cmp = 0;
                for (j = rem.n - 1; j >= 0; j--)
                    if (rem.l[j] != b->l[j]) { cmp = rem.l[j] > b->l[j] ? 1 : -1; break; }
            }
            if (cmp >= 0) {
                uint64_t borrow = 0;
                int j;
                for (j = 0; j < rem.n; j++) {
                    uint64_t sub = (j < b->n ? b->l[j] : 0) + borrow;
                    uint64_t cur = rem.l[j];
                    rem.l[j] = (uint32_t)(cur - sub);
                    borrow = (cur < sub);
                }
                while (rem.n > 1 && rem.l[rem.n - 1] == 0) rem.n--;
                quo.l[i / 32] |= (uint32_t)1 << (i % 32);
            }
        }
    }
    while (quo.n > 1 && quo.l[quo.n - 1] == 0) quo.n--;
    *q = quo; *r = rem;
}

/* pack a TccLdBig into 64-bit limbs for tcc_ld_canon_from_big */
TCC_LD_FN int tcc_ld_big_to_u64(const TccLdBig *b, uint64_t *out)
{
    int n64 = (b->n + 1) / 2, i;
    for (i = 0; i < n64; i++) {
        uint64_t lo = b->l[2 * i];
        uint64_t hi = (2 * i + 1 < b->n) ? b->l[2 * i + 1] : 0;
        out[i] = lo | (hi << 32);
    }
    return n64;
}

/* Parse a decimal literal body (digits, optional '.', optional
   exponent; no sign, no suffix) into the target format, correctly
   rounded (round-to-nearest-even) in all cases. */
TCC_LD_FN void tcc_ld_parse_decimal(const TccLdFmt *f, const char *s, uint64_t out[2])
{
    TccLdBig D;
    int nfrac = 0, seen_digit = 0, seen_dot = 0, exp10 = 0;
    const char *p = s;
    TccLdVal v;
    uint64_t X[TCC_LD_BN_LIMBS / 2 + 2];

    tcc_ld_big_zero(&D);
    for (; *p; p++) {
        if (*p >= '0' && *p <= '9') {
            tcc_ld_big_mul_small(&D, 10);
            tcc_ld_big_add_small(&D, (uint32_t)(*p - '0'));
            if (seen_dot) nfrac++;
            seen_digit = 1;
        } else if (*p == '.' && !seen_dot) {
            seen_dot = 1;
        } else break;
    }
    if (*p == 'e' || *p == 'E') {
        int esign = 1;
        long ev = 0;
        p++;
        if (*p == '+') p++;
        else if (*p == '-') { esign = -1; p++; }
        for (; *p >= '0' && *p <= '9'; p++) {
            ev = ev * 10 + (*p - '0');
            if (ev > 1000000) ev = 1000000;
        }
        exp10 = esign * (int)ev;
    }
    v.sign = 0; v.sticky = 0;
    if (!seen_digit || tcc_ld_big_is_zero(&D)) {
        tcc_ld_zero_result(&v, 0);
        tcc_ld_encode_fmt(f, &v, out);
        return;
    }
    {
        int X10 = exp10 - nfrac;
        if (X10 >= 0) {
            TccLdBig B = D;
            /* value >= 10^X10; overflow when that alone exceeds
               2^(emax+1), the overflow threshold of the format */
            if (X10 > (int)((f->emax + 1) * 0.30103) + 1 || !tcc_ld_big_mul_pow10(&B, X10)) {
                tcc_ld_inf_result(&v, 0);
                tcc_ld_encode_fmt(f, &v, out);
                return;
            }
            {
                int n64 = tcc_ld_big_to_u64(&B, X);
                tcc_ld_canon_from_big(X, n64, 127, 0, &v);
            }
            v.sign = 0;
            tcc_ld_encode_fmt(f, &v, out);
            return;
        } else {
            int k10 = -X10;
            TccLdBig P, A, Q, R;
            int k, n64;
            /* value < 2^bitlen(D) * 10^-k10; it rounds to zero when
               even that bound is at most half of the smallest target
               subnormal, 2^(emin - p - 1) */
            if (k10 > (int)((tcc_ld_big_bitlen(&D) - f->emin + f->p + 2) * 0.30103) + 1) {
                tcc_ld_zero_result(&v, 0);
                tcc_ld_encode_fmt(f, &v, out);
                return;
            }
            tcc_ld_big_zero(&P); P.l[0] = 1;
            tcc_ld_big_mul_pow10(&P, k10);
            k = tcc_ld_big_bitlen(&P) - tcc_ld_big_bitlen(&D) + 200;
            if (k < 0) k = 0;
            A = D;
            tcc_ld_big_shl(&A, k);
            tcc_ld_big_divmod(&A, &P, &Q, &R);
            n64 = tcc_ld_big_to_u64(&Q, X);
            tcc_ld_canon_from_big(X, n64, 127 - k, !tcc_ld_big_is_zero(&R), &v);
            v.sign = 0;
            tcc_ld_encode_fmt(f, &v, out);
            return;
        }
    }
}

/* Convert the hex/binary-float bignum accumulated by the tokenizer:
   value = (B + eps) * 2^exp2 with B the 128-bit integer in bn
   (bn[0] = low limb) and 0 <= eps < 1, eps > 0 iff sticky. */
TCC_LD_FN void tcc_ld_from_hex_bignum(const TccLdFmt *f, const uint32_t bn[4],
                               int exp2, int sticky, uint64_t out[2])
{
    uint64_t B[2];
    TccLdVal v;
    B[0] = bn[0] | ((uint64_t)bn[1] << 32);
    B[1] = bn[2] | ((uint64_t)bn[3] << 32);
    v.sign = 0; v.sticky = 0;
    if ((B[0] | B[1]) == 0) {
        tcc_ld_zero_result(&v, 0);
        tcc_ld_encode_fmt(f, &v, out);
        return;
    }
    tcc_ld_canon_from_big(B, 2, exp2 + 127, sticky, &v);
    v.sign = 0;
    tcc_ld_encode_fmt(f, &v, out);
}

/* ---------------- target-format entry points ---------------- */

#if TCC_LDOUBLE_KIND == 1
# define tcc_ld_fmt_target tcc_ld_fmt_x87
#elif TCC_LDOUBLE_KIND == 2
# define tcc_ld_fmt_target tcc_ld_fmt_quad
#else
# define tcc_ld_fmt_target tcc_ld_fmt_double
#endif

TCC_LD_FN void tcc_ld_decode(const TccLDouble *in, TccLdVal *v)
{
    tcc_ld_decode_fmt(&tcc_ld_fmt_target, in->w[0], in->w[1], v);
}

TCC_LD_FN void tcc_ld_encode(const TccLdVal *v, TccLDouble *out)
{
    tcc_ld_encode_fmt(&tcc_ld_fmt_target, v, out->w);
}

TCC_LD_FN void tcc_ld_decode_f32(float x, TccLdVal *v)
{
    uint32_t u;
    memcpy(&u, &x, 4);
    tcc_ld_decode_fmt(&tcc_ld_fmt_float, u, 0, v);
}

TCC_LD_FN void tcc_ld_decode_f64(double x, TccLdVal *v)
{
    uint64_t u;
    memcpy(&u, &x, 8);
    tcc_ld_decode_fmt(&tcc_ld_fmt_double, u, 0, v);
}

TCC_LD_FN float tcc_ld_encode_f32(const TccLdVal *v)
{
    uint64_t o[2];
    uint32_t u;
    float x;
    tcc_ld_encode_fmt(&tcc_ld_fmt_float, v, o);
    u = (uint32_t)o[0];
    memcpy(&x, &u, 4);
    return x;
}

TCC_LD_FN double tcc_ld_encode_f64(const TccLdVal *v)
{
    uint64_t o[2];
    double x;
    tcc_ld_encode_fmt(&tcc_ld_fmt_double, v, o);
    memcpy(&x, o, 8);
    return x;
}

/* Parse a decimal literal body (digits, optional '.', optional
   exponent; no sign, no suffix) into the target format. */
TCC_LD_FN void tcc_ld_parse(const char *s, TccLDouble *out)
{
    tcc_ld_parse_decimal(&tcc_ld_fmt_target, s, out->w);
}

/* Convert the hex/binary-float bignum accumulated by the tokenizer:
   value = (B + eps) * 2^exp2, eps > 0 iff sticky. */
TCC_LD_FN void tcc_ld_parse_hex(const unsigned int *bn, int exp2, int sticky,
                             TccLDouble *out)
{
    tcc_ld_from_hex_bignum(&tcc_ld_fmt_target, bn, exp2, sticky, out->w);
}
