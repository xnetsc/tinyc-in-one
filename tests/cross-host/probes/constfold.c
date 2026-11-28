/* Cross-host differential probe: integer constant folding.
 * Every value below is an integer constant expression; the
 * compiler folds all of them at compile time, so the bytes of
 * the initialized arrays are a direct fingerprint of the
 * folder's arithmetic (widths, overflow, shifts, division
 * rounding, casts, sizeof in constant contexts). */
enum {
    E_SHIFT = (1 << 20) + (1u << 31) / 7,
    E_NEGDIV = -100 / 7,
    E_NEGMOD = -100 % 7,
    E_MIX = (int)(0xFFFFFFFFu >> 4) ^ 0x5A5A5A5A
};

static const long long ll_vals[] = {
    9223372036854775807LL / 3,
    (-9223372036854775807LL - 1) / -1 * 0 + 12345,
    (long long)(18446744073709551615ULL >> 13),
    1000003LL * 1000003LL * 999983LL,
    (1LL << 62) % 1000000007LL,
    -7LL / 2, -7LL % 2, 7LL / -2, 7LL % -2,
    (long long)sizeof(long long) * 1000 + (long long)sizeof(void *),
    E_SHIFT, E_NEGDIV, E_NEGMOD, E_MIX
};

static const unsigned int ui_vals[] = {
    4294967295u / 3u,
    4294967295u * 2u + 1u,
    (unsigned)-1 >> 1,
    ~(0x0F0F0F0Fu) & 0xFFFFFFFFu,
    1u << 31,
    (unsigned char)300 + (unsigned short)70000,
    (5 > 3) + (2 == 2) * 10 + (1 && 0) * 100 + (0 || 7) * 1000,
    sizeof(int) == 4 ? 111 : 222,
    'a' + ('\x41' << 8) + ('\101' << 16)
};

static const int cond_vals[] = {
    (sizeof(long) > sizeof(int)) ? 1 : 0,
    (-1 >> 1) == -1 ? 1 : 0,
    ((unsigned)-1 < 0) ? 1 : 0,
    (int)(-3.7) + (int)(3.7),
    (int)(2147483647.0 + 1.0) < 0 ? 1 : 0
};

int probe_constfold_anchor(void)
{
    return (int)(ll_vals[0] & 0xFF) + (int)ui_vals[0] + cond_vals[0];
}
