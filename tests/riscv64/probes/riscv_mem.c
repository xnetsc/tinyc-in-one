/* riscv64 RVV differential probe: strided and indexed memory
   forms (family F4).  Lane counts come from __riscv_vlenb(); the
   runner exercises the zvl128b and zvl256b configurations. */
#include <riscv_vector.h>
#include <stdio.h>
#include <string.h>

static int ia[256], ib[256], io[256];
static float fa2[256];
static short ha[256];
static long la[64];
static double da[64];
static unsigned int idx32[64];
static unsigned short idx16[64];
static unsigned long idx64[64];
static unsigned char idx8[64];

static void pi(const char *tag, vint32m1_t v, int n)
{
    int out[64], i;
    __riscv_vse32_v_i32m1(out, v, n);
    printf("%s", tag);
    for (i = 0; i < n; i++) printf(" %d", out[i]);
    printf("\n");
}

int main(void)
{
    int L32 = (int)(__riscv_vlenb() / 4);
    int i;

    for (i = 0; i < 256; i++) {
        ia[i] = i * 3 - 100;
        ib[i] = 1000 + i;
        io[i] = 0;
        fa2[i] = i * 0.25f - 5.0f;
        ha[i] = (short)(i - 50);
    }
    for (i = 0; i < 64; i++) {
        la[i] = i * 100 - 7;
        da[i] = i * 0.5 - 3.0;
        idx32[i] = (unsigned)((L32 - 1 - i) * 4);   /* reversed lanes, bytes */
        idx16[i] = (unsigned short)((L32 - 1 - i) * 4);
        idx64[i] = (unsigned long)((L32 - 1 - i) * 4);
        idx8[i] = (unsigned char)(i * 8);
    }

    /* strided load/store, 32-bit elements, stride 12 bytes */
    {
        vint32m1_t v = __riscv_vlse32_v_i32m1(ia, 12, L32);
        pi("lse32", v, L32);
        __riscv_vsse32_v_i32m1(io, 12, v, L32);
        printf("sse32");
        for (i = 0; i < L32; i++) printf(" %d", io[i * 3]);
        printf("\n");
    }
    /* strided 16-bit and 64-bit forms */
    {
        int L16 = (int)(__riscv_vlenb() / 2);
        vint16m1_t v = __riscv_vlse16_v_i16m1(ha, 6, L16);
        short out[64];
        __riscv_vse16_v_i16m1(out, v, L16);
        printf("lse16");
        for (i = 0; i < L16; i++) printf(" %d", out[i]);
        printf("\n");
    }
    {
        int L64 = (int)(__riscv_vlenb() / 8);
        vfloat64m1_t v = __riscv_vlse64_v_f64m1(da, 16, L64);
        double out[64];
        __riscv_vse64_v_f64m1(out, v, L64);
        printf("lse64");
        for (i = 0; i < L64; i++) printf(" %g", out[i]);
        printf("\n");
        vint64m1_t w = __riscv_vlse64_v_i64m1(la, 24, L64);
        long o2[64];
        __riscv_vse64_v_i64m1(o2, w, L64);
        printf("lse64i");
        for (i = 0; i < L64; i++) printf(" %ld", o2[i]);
        printf("\n");
    }
    /* indexed gather/scatter, ei32 */
    {
        vuint32m1_t idx = __riscv_vle32_v_u32m1(idx32, L32);
        vint32m1_t v = __riscv_vluxei32_v_i32m1(ib, idx, L32);
        pi("lux32", v, L32);
        vint32m1_t w = __riscv_vloxei32_v_i32m1(ib, idx, L32);
        pi("lox32", w, L32);
        for (i = 0; i < 256; i++) io[i] = 0;
        __riscv_vsuxei32_v_i32m1(io, idx, v, L32);
        printf("sux32");
        for (i = 0; i < L32; i++) printf(" %d", io[(L32 - 1 - i)]);
        printf("\n");
        for (i = 0; i < 256; i++) io[i] = 0;
        __riscv_vsoxei32_v_i32m1(io, idx, v, L32);
        printf("sox32");
        for (i = 0; i < L32; i++) printf(" %d", io[(L32 - 1 - i)]);
        printf("\n");
    }
    /* indexed with ei16 and ei64 index vectors */
    {
        vuint16mf2_t idx = __riscv_vle16_v_u16mf2(idx16, L32);
        vint32m1_t v = __riscv_vluxei16_v_i32m1(ib, idx, L32);
        pi("lux16", v, L32);
    }
    {
        vuint64m2_t idx = __riscv_vle64_v_u64m2(idx64, L32);
        vint32m1_t v = __riscv_vluxei64_v_i32m1(ib, idx, L32);
        pi("lux64", v, L32);
    }
    /* indexed for 16-bit data (ei8 indices) */
    {
        int L16 = (int)(__riscv_vlenb() / 2);
        unsigned char bidx[64];
        short out[64];
        for (i = 0; i < L16; i++) bidx[i] = (unsigned char)((L16 - 1 - i) * 2);
        {
            vuint8mf2_t idx = __riscv_vle8_v_u8mf2(bidx, L16);
            vint16m1_t v = __riscv_vluxei8_v_i16m1(ha, idx, L16);
            __riscv_vse16_v_i16m1(out, v, L16);
            printf("lux8_16");
            for (i = 0; i < L16; i++) printf(" %d", out[i]);
            printf("\n");
        }
    }
    /* indexed ei8 with float data (indices are byte offsets) */
    {
        unsigned char bidx[64];
        float fout[64];
        for (i = 0; i < L32; i++) bidx[i] = (unsigned char)((L32 - 1 - i) * 4);
        {
            vuint8mf4_t idx = __riscv_vle8_v_u8mf4(bidx, L32);
            vfloat32m1_t v = __riscv_vluxei8_v_f32m1(fa2, idx, L32);
            __riscv_vse32_v_f32m1(fout, v, L32);
            printf("lux8_f");
            for (i = 0; i < L32; i++) printf(" %g", (double)fout[i]);
            printf("\n");
            __riscv_vsoxei8_v_f32m1(fa2, idx, v, L32);
            printf("sox8_f %g %g\n", (double)fa2[0],
                   (double)fa2[L32 - 1]);
        }
    }
    /* indexed ei8 with unsigned-byte data */
    {
        unsigned char ub[256], uout[256], bidx[256];
        int L8 = (int)__riscv_vlenb();
        for (i = 0; i < L8; i++) {
            ub[i] = (unsigned char)(i * 5 + 1);
            bidx[i] = (unsigned char)(L8 - 1 - i);
        }
        {
            vuint8m1_t idx = __riscv_vle8_v_u8m1(bidx, L8);
            vuint8m1_t v = __riscv_vluxei8_v_u8m1(ub, idx, L8);
            __riscv_vse8_v_u8m1(uout, v, L8);
            printf("lux8_u");
            for (i = 0; i < L8; i++) printf(" %u", (unsigned)uout[i]);
            printf("\n");
        }
    }
    (void)idx8;
    return 0;
}
