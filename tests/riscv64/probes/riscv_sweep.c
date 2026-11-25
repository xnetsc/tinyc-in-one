/* riscv64 RVV differential probe: generated sweep of the
   core families over every type shape (tools/riscv_gen.py).
   Lane counts come from __riscv_vlenb(). */
#include <riscv_vector.h>
#include <stdio.h>

static signed char buf8[8192];
static short buf16[4096];
static int buf32[2048];
static long long buf64[1024];
static float buff[2048];
static double bufd[1024];
static unsigned char obuf[8192];

int main(void)
{
    int i;
    for (i = 0; i < 8192; i++) buf8[i] = (signed char)(i * 37 + 11);
    for (i = 0; i < 4096; i++) buf16[i] = (short)(i * 37 + 11);
    for (i = 0; i < 2048; i++) buf32[i] = i * 37 + 11;
    for (i = 0; i < 1024; i++) buf64[i] = i * 37 + 11;
    for (i = 0; i < 2048; i++) buff[i] = (i % 13) * 0.5f - 2.0f;
    for (i = 0; i < 1024; i++) bufd[i] = (i % 13) * 0.5 - 2.0;
    {
        int n = (__riscv_vlenb() * 1 * 8 / (8 * 8)), i;
        vint8mf8_t a = __riscv_vle8_v_i8mf8(buf8, n);
        vint8mf8_t b = __riscv_vle8_v_i8mf8(buf8 + n, n);
        vint8mf8_t r = __riscv_vadd_vv_i8mf8(a, b, n);
        r = __riscv_vmul_vv_i8mf8(r, a, n);
        r = __riscv_vxor_vv_i8mf8(r, b, n);
        __riscv_vse8_v_i8mf8(obuf, r, n);
        printf("i8mf8");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (4 * 8)), i;
        vint8mf4_t a = __riscv_vle8_v_i8mf4(buf8, n);
        vint8mf4_t b = __riscv_vle8_v_i8mf4(buf8 + n, n);
        vint8mf4_t r = __riscv_vadd_vv_i8mf4(a, b, n);
        r = __riscv_vmul_vv_i8mf4(r, a, n);
        r = __riscv_vxor_vv_i8mf4(r, b, n);
        __riscv_vse8_v_i8mf4(obuf, r, n);
        printf("i8mf4");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (2 * 8)), i;
        vint8mf2_t a = __riscv_vle8_v_i8mf2(buf8, n);
        vint8mf2_t b = __riscv_vle8_v_i8mf2(buf8 + n, n);
        vint8mf2_t r = __riscv_vadd_vv_i8mf2(a, b, n);
        r = __riscv_vmul_vv_i8mf2(r, a, n);
        r = __riscv_vxor_vv_i8mf2(r, b, n);
        __riscv_vse8_v_i8mf2(obuf, r, n);
        printf("i8mf2");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (1 * 8)), i;
        vint8m1_t a = __riscv_vle8_v_i8m1(buf8, n);
        vint8m1_t b = __riscv_vle8_v_i8m1(buf8 + n, n);
        vint8m1_t r = __riscv_vadd_vv_i8m1(a, b, n);
        r = __riscv_vmul_vv_i8m1(r, a, n);
        r = __riscv_vxor_vv_i8m1(r, b, n);
        __riscv_vse8_v_i8m1(obuf, r, n);
        printf("i8m1");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 2 * 8 / (1 * 8)), i;
        vint8m2_t a = __riscv_vle8_v_i8m2(buf8, n);
        vint8m2_t b = __riscv_vle8_v_i8m2(buf8 + n, n);
        vint8m2_t r = __riscv_vadd_vv_i8m2(a, b, n);
        r = __riscv_vmul_vv_i8m2(r, a, n);
        r = __riscv_vxor_vv_i8m2(r, b, n);
        __riscv_vse8_v_i8m2(obuf, r, n);
        printf("i8m2");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 4 * 8 / (1 * 8)), i;
        vint8m4_t a = __riscv_vle8_v_i8m4(buf8, n);
        vint8m4_t b = __riscv_vle8_v_i8m4(buf8 + n, n);
        vint8m4_t r = __riscv_vadd_vv_i8m4(a, b, n);
        r = __riscv_vmul_vv_i8m4(r, a, n);
        r = __riscv_vxor_vv_i8m4(r, b, n);
        __riscv_vse8_v_i8m4(obuf, r, n);
        printf("i8m4");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 8 * 8 / (1 * 8)), i;
        vint8m8_t a = __riscv_vle8_v_i8m8(buf8, n);
        vint8m8_t b = __riscv_vle8_v_i8m8(buf8 + n, n);
        vint8m8_t r = __riscv_vadd_vv_i8m8(a, b, n);
        r = __riscv_vmul_vv_i8m8(r, a, n);
        r = __riscv_vxor_vv_i8m8(r, b, n);
        __riscv_vse8_v_i8m8(obuf, r, n);
        printf("i8m8");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (4 * 16)), i;
        vint16mf4_t a = __riscv_vle16_v_i16mf4(buf16, n);
        vint16mf4_t b = __riscv_vle16_v_i16mf4(buf16 + n, n);
        vint16mf4_t r = __riscv_vadd_vv_i16mf4(a, b, n);
        r = __riscv_vmul_vv_i16mf4(r, a, n);
        r = __riscv_vxor_vv_i16mf4(r, b, n);
        __riscv_vse16_v_i16mf4(obuf, r, n);
        printf("i16mf4");
        for (i = 0; i < n * (16 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (2 * 16)), i;
        vint16mf2_t a = __riscv_vle16_v_i16mf2(buf16, n);
        vint16mf2_t b = __riscv_vle16_v_i16mf2(buf16 + n, n);
        vint16mf2_t r = __riscv_vadd_vv_i16mf2(a, b, n);
        r = __riscv_vmul_vv_i16mf2(r, a, n);
        r = __riscv_vxor_vv_i16mf2(r, b, n);
        __riscv_vse16_v_i16mf2(obuf, r, n);
        printf("i16mf2");
        for (i = 0; i < n * (16 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (1 * 16)), i;
        vint16m1_t a = __riscv_vle16_v_i16m1(buf16, n);
        vint16m1_t b = __riscv_vle16_v_i16m1(buf16 + n, n);
        vint16m1_t r = __riscv_vadd_vv_i16m1(a, b, n);
        r = __riscv_vmul_vv_i16m1(r, a, n);
        r = __riscv_vxor_vv_i16m1(r, b, n);
        __riscv_vse16_v_i16m1(obuf, r, n);
        printf("i16m1");
        for (i = 0; i < n * (16 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 2 * 8 / (1 * 16)), i;
        vint16m2_t a = __riscv_vle16_v_i16m2(buf16, n);
        vint16m2_t b = __riscv_vle16_v_i16m2(buf16 + n, n);
        vint16m2_t r = __riscv_vadd_vv_i16m2(a, b, n);
        r = __riscv_vmul_vv_i16m2(r, a, n);
        r = __riscv_vxor_vv_i16m2(r, b, n);
        __riscv_vse16_v_i16m2(obuf, r, n);
        printf("i16m2");
        for (i = 0; i < n * (16 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 4 * 8 / (1 * 16)), i;
        vint16m4_t a = __riscv_vle16_v_i16m4(buf16, n);
        vint16m4_t b = __riscv_vle16_v_i16m4(buf16 + n, n);
        vint16m4_t r = __riscv_vadd_vv_i16m4(a, b, n);
        r = __riscv_vmul_vv_i16m4(r, a, n);
        r = __riscv_vxor_vv_i16m4(r, b, n);
        __riscv_vse16_v_i16m4(obuf, r, n);
        printf("i16m4");
        for (i = 0; i < n * (16 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 8 * 8 / (1 * 16)), i;
        vint16m8_t a = __riscv_vle16_v_i16m8(buf16, n);
        vint16m8_t b = __riscv_vle16_v_i16m8(buf16 + n, n);
        vint16m8_t r = __riscv_vadd_vv_i16m8(a, b, n);
        r = __riscv_vmul_vv_i16m8(r, a, n);
        r = __riscv_vxor_vv_i16m8(r, b, n);
        __riscv_vse16_v_i16m8(obuf, r, n);
        printf("i16m8");
        for (i = 0; i < n * (16 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (2 * 32)), i;
        vint32mf2_t a = __riscv_vle32_v_i32mf2(buf32, n);
        vint32mf2_t b = __riscv_vle32_v_i32mf2(buf32 + n, n);
        vint32mf2_t r = __riscv_vadd_vv_i32mf2(a, b, n);
        r = __riscv_vmul_vv_i32mf2(r, a, n);
        r = __riscv_vxor_vv_i32mf2(r, b, n);
        __riscv_vse32_v_i32mf2(obuf, r, n);
        printf("i32mf2");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (1 * 32)), i;
        vint32m1_t a = __riscv_vle32_v_i32m1(buf32, n);
        vint32m1_t b = __riscv_vle32_v_i32m1(buf32 + n, n);
        vint32m1_t r = __riscv_vadd_vv_i32m1(a, b, n);
        r = __riscv_vmul_vv_i32m1(r, a, n);
        r = __riscv_vxor_vv_i32m1(r, b, n);
        __riscv_vse32_v_i32m1(obuf, r, n);
        printf("i32m1");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 2 * 8 / (1 * 32)), i;
        vint32m2_t a = __riscv_vle32_v_i32m2(buf32, n);
        vint32m2_t b = __riscv_vle32_v_i32m2(buf32 + n, n);
        vint32m2_t r = __riscv_vadd_vv_i32m2(a, b, n);
        r = __riscv_vmul_vv_i32m2(r, a, n);
        r = __riscv_vxor_vv_i32m2(r, b, n);
        __riscv_vse32_v_i32m2(obuf, r, n);
        printf("i32m2");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 4 * 8 / (1 * 32)), i;
        vint32m4_t a = __riscv_vle32_v_i32m4(buf32, n);
        vint32m4_t b = __riscv_vle32_v_i32m4(buf32 + n, n);
        vint32m4_t r = __riscv_vadd_vv_i32m4(a, b, n);
        r = __riscv_vmul_vv_i32m4(r, a, n);
        r = __riscv_vxor_vv_i32m4(r, b, n);
        __riscv_vse32_v_i32m4(obuf, r, n);
        printf("i32m4");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 8 * 8 / (1 * 32)), i;
        vint32m8_t a = __riscv_vle32_v_i32m8(buf32, n);
        vint32m8_t b = __riscv_vle32_v_i32m8(buf32 + n, n);
        vint32m8_t r = __riscv_vadd_vv_i32m8(a, b, n);
        r = __riscv_vmul_vv_i32m8(r, a, n);
        r = __riscv_vxor_vv_i32m8(r, b, n);
        __riscv_vse32_v_i32m8(obuf, r, n);
        printf("i32m8");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (1 * 64)), i;
        vint64m1_t a = __riscv_vle64_v_i64m1(buf64, n);
        vint64m1_t b = __riscv_vle64_v_i64m1(buf64 + n, n);
        vint64m1_t r = __riscv_vadd_vv_i64m1(a, b, n);
        r = __riscv_vmul_vv_i64m1(r, a, n);
        r = __riscv_vxor_vv_i64m1(r, b, n);
        __riscv_vse64_v_i64m1(obuf, r, n);
        printf("i64m1");
        for (i = 0; i < n * (64 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 2 * 8 / (1 * 64)), i;
        vint64m2_t a = __riscv_vle64_v_i64m2(buf64, n);
        vint64m2_t b = __riscv_vle64_v_i64m2(buf64 + n, n);
        vint64m2_t r = __riscv_vadd_vv_i64m2(a, b, n);
        r = __riscv_vmul_vv_i64m2(r, a, n);
        r = __riscv_vxor_vv_i64m2(r, b, n);
        __riscv_vse64_v_i64m2(obuf, r, n);
        printf("i64m2");
        for (i = 0; i < n * (64 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 4 * 8 / (1 * 64)), i;
        vint64m4_t a = __riscv_vle64_v_i64m4(buf64, n);
        vint64m4_t b = __riscv_vle64_v_i64m4(buf64 + n, n);
        vint64m4_t r = __riscv_vadd_vv_i64m4(a, b, n);
        r = __riscv_vmul_vv_i64m4(r, a, n);
        r = __riscv_vxor_vv_i64m4(r, b, n);
        __riscv_vse64_v_i64m4(obuf, r, n);
        printf("i64m4");
        for (i = 0; i < n * (64 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 8 * 8 / (1 * 64)), i;
        vint64m8_t a = __riscv_vle64_v_i64m8(buf64, n);
        vint64m8_t b = __riscv_vle64_v_i64m8(buf64 + n, n);
        vint64m8_t r = __riscv_vadd_vv_i64m8(a, b, n);
        r = __riscv_vmul_vv_i64m8(r, a, n);
        r = __riscv_vxor_vv_i64m8(r, b, n);
        __riscv_vse64_v_i64m8(obuf, r, n);
        printf("i64m8");
        for (i = 0; i < n * (64 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (8 * 8)), i;
        vuint8mf8_t a = __riscv_vle8_v_u8mf8(buf8, n);
        vuint8mf8_t b = __riscv_vle8_v_u8mf8(buf8 + n, n);
        vuint8mf8_t r = __riscv_vadd_vv_u8mf8(a, b, n);
        r = __riscv_vmul_vv_u8mf8(r, a, n);
        r = __riscv_vxor_vv_u8mf8(r, b, n);
        __riscv_vse8_v_u8mf8(obuf, r, n);
        printf("u8mf8");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (4 * 8)), i;
        vuint8mf4_t a = __riscv_vle8_v_u8mf4(buf8, n);
        vuint8mf4_t b = __riscv_vle8_v_u8mf4(buf8 + n, n);
        vuint8mf4_t r = __riscv_vadd_vv_u8mf4(a, b, n);
        r = __riscv_vmul_vv_u8mf4(r, a, n);
        r = __riscv_vxor_vv_u8mf4(r, b, n);
        __riscv_vse8_v_u8mf4(obuf, r, n);
        printf("u8mf4");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (2 * 8)), i;
        vuint8mf2_t a = __riscv_vle8_v_u8mf2(buf8, n);
        vuint8mf2_t b = __riscv_vle8_v_u8mf2(buf8 + n, n);
        vuint8mf2_t r = __riscv_vadd_vv_u8mf2(a, b, n);
        r = __riscv_vmul_vv_u8mf2(r, a, n);
        r = __riscv_vxor_vv_u8mf2(r, b, n);
        __riscv_vse8_v_u8mf2(obuf, r, n);
        printf("u8mf2");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (1 * 8)), i;
        vuint8m1_t a = __riscv_vle8_v_u8m1(buf8, n);
        vuint8m1_t b = __riscv_vle8_v_u8m1(buf8 + n, n);
        vuint8m1_t r = __riscv_vadd_vv_u8m1(a, b, n);
        r = __riscv_vmul_vv_u8m1(r, a, n);
        r = __riscv_vxor_vv_u8m1(r, b, n);
        __riscv_vse8_v_u8m1(obuf, r, n);
        printf("u8m1");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 2 * 8 / (1 * 8)), i;
        vuint8m2_t a = __riscv_vle8_v_u8m2(buf8, n);
        vuint8m2_t b = __riscv_vle8_v_u8m2(buf8 + n, n);
        vuint8m2_t r = __riscv_vadd_vv_u8m2(a, b, n);
        r = __riscv_vmul_vv_u8m2(r, a, n);
        r = __riscv_vxor_vv_u8m2(r, b, n);
        __riscv_vse8_v_u8m2(obuf, r, n);
        printf("u8m2");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 4 * 8 / (1 * 8)), i;
        vuint8m4_t a = __riscv_vle8_v_u8m4(buf8, n);
        vuint8m4_t b = __riscv_vle8_v_u8m4(buf8 + n, n);
        vuint8m4_t r = __riscv_vadd_vv_u8m4(a, b, n);
        r = __riscv_vmul_vv_u8m4(r, a, n);
        r = __riscv_vxor_vv_u8m4(r, b, n);
        __riscv_vse8_v_u8m4(obuf, r, n);
        printf("u8m4");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 8 * 8 / (1 * 8)), i;
        vuint8m8_t a = __riscv_vle8_v_u8m8(buf8, n);
        vuint8m8_t b = __riscv_vle8_v_u8m8(buf8 + n, n);
        vuint8m8_t r = __riscv_vadd_vv_u8m8(a, b, n);
        r = __riscv_vmul_vv_u8m8(r, a, n);
        r = __riscv_vxor_vv_u8m8(r, b, n);
        __riscv_vse8_v_u8m8(obuf, r, n);
        printf("u8m8");
        for (i = 0; i < n * (8 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (4 * 16)), i;
        vuint16mf4_t a = __riscv_vle16_v_u16mf4(buf16, n);
        vuint16mf4_t b = __riscv_vle16_v_u16mf4(buf16 + n, n);
        vuint16mf4_t r = __riscv_vadd_vv_u16mf4(a, b, n);
        r = __riscv_vmul_vv_u16mf4(r, a, n);
        r = __riscv_vxor_vv_u16mf4(r, b, n);
        __riscv_vse16_v_u16mf4(obuf, r, n);
        printf("u16mf4");
        for (i = 0; i < n * (16 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (2 * 16)), i;
        vuint16mf2_t a = __riscv_vle16_v_u16mf2(buf16, n);
        vuint16mf2_t b = __riscv_vle16_v_u16mf2(buf16 + n, n);
        vuint16mf2_t r = __riscv_vadd_vv_u16mf2(a, b, n);
        r = __riscv_vmul_vv_u16mf2(r, a, n);
        r = __riscv_vxor_vv_u16mf2(r, b, n);
        __riscv_vse16_v_u16mf2(obuf, r, n);
        printf("u16mf2");
        for (i = 0; i < n * (16 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (1 * 16)), i;
        vuint16m1_t a = __riscv_vle16_v_u16m1(buf16, n);
        vuint16m1_t b = __riscv_vle16_v_u16m1(buf16 + n, n);
        vuint16m1_t r = __riscv_vadd_vv_u16m1(a, b, n);
        r = __riscv_vmul_vv_u16m1(r, a, n);
        r = __riscv_vxor_vv_u16m1(r, b, n);
        __riscv_vse16_v_u16m1(obuf, r, n);
        printf("u16m1");
        for (i = 0; i < n * (16 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 2 * 8 / (1 * 16)), i;
        vuint16m2_t a = __riscv_vle16_v_u16m2(buf16, n);
        vuint16m2_t b = __riscv_vle16_v_u16m2(buf16 + n, n);
        vuint16m2_t r = __riscv_vadd_vv_u16m2(a, b, n);
        r = __riscv_vmul_vv_u16m2(r, a, n);
        r = __riscv_vxor_vv_u16m2(r, b, n);
        __riscv_vse16_v_u16m2(obuf, r, n);
        printf("u16m2");
        for (i = 0; i < n * (16 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 4 * 8 / (1 * 16)), i;
        vuint16m4_t a = __riscv_vle16_v_u16m4(buf16, n);
        vuint16m4_t b = __riscv_vle16_v_u16m4(buf16 + n, n);
        vuint16m4_t r = __riscv_vadd_vv_u16m4(a, b, n);
        r = __riscv_vmul_vv_u16m4(r, a, n);
        r = __riscv_vxor_vv_u16m4(r, b, n);
        __riscv_vse16_v_u16m4(obuf, r, n);
        printf("u16m4");
        for (i = 0; i < n * (16 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 8 * 8 / (1 * 16)), i;
        vuint16m8_t a = __riscv_vle16_v_u16m8(buf16, n);
        vuint16m8_t b = __riscv_vle16_v_u16m8(buf16 + n, n);
        vuint16m8_t r = __riscv_vadd_vv_u16m8(a, b, n);
        r = __riscv_vmul_vv_u16m8(r, a, n);
        r = __riscv_vxor_vv_u16m8(r, b, n);
        __riscv_vse16_v_u16m8(obuf, r, n);
        printf("u16m8");
        for (i = 0; i < n * (16 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (2 * 32)), i;
        vuint32mf2_t a = __riscv_vle32_v_u32mf2(buf32, n);
        vuint32mf2_t b = __riscv_vle32_v_u32mf2(buf32 + n, n);
        vuint32mf2_t r = __riscv_vadd_vv_u32mf2(a, b, n);
        r = __riscv_vmul_vv_u32mf2(r, a, n);
        r = __riscv_vxor_vv_u32mf2(r, b, n);
        __riscv_vse32_v_u32mf2(obuf, r, n);
        printf("u32mf2");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (1 * 32)), i;
        vuint32m1_t a = __riscv_vle32_v_u32m1(buf32, n);
        vuint32m1_t b = __riscv_vle32_v_u32m1(buf32 + n, n);
        vuint32m1_t r = __riscv_vadd_vv_u32m1(a, b, n);
        r = __riscv_vmul_vv_u32m1(r, a, n);
        r = __riscv_vxor_vv_u32m1(r, b, n);
        __riscv_vse32_v_u32m1(obuf, r, n);
        printf("u32m1");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 2 * 8 / (1 * 32)), i;
        vuint32m2_t a = __riscv_vle32_v_u32m2(buf32, n);
        vuint32m2_t b = __riscv_vle32_v_u32m2(buf32 + n, n);
        vuint32m2_t r = __riscv_vadd_vv_u32m2(a, b, n);
        r = __riscv_vmul_vv_u32m2(r, a, n);
        r = __riscv_vxor_vv_u32m2(r, b, n);
        __riscv_vse32_v_u32m2(obuf, r, n);
        printf("u32m2");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 4 * 8 / (1 * 32)), i;
        vuint32m4_t a = __riscv_vle32_v_u32m4(buf32, n);
        vuint32m4_t b = __riscv_vle32_v_u32m4(buf32 + n, n);
        vuint32m4_t r = __riscv_vadd_vv_u32m4(a, b, n);
        r = __riscv_vmul_vv_u32m4(r, a, n);
        r = __riscv_vxor_vv_u32m4(r, b, n);
        __riscv_vse32_v_u32m4(obuf, r, n);
        printf("u32m4");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 8 * 8 / (1 * 32)), i;
        vuint32m8_t a = __riscv_vle32_v_u32m8(buf32, n);
        vuint32m8_t b = __riscv_vle32_v_u32m8(buf32 + n, n);
        vuint32m8_t r = __riscv_vadd_vv_u32m8(a, b, n);
        r = __riscv_vmul_vv_u32m8(r, a, n);
        r = __riscv_vxor_vv_u32m8(r, b, n);
        __riscv_vse32_v_u32m8(obuf, r, n);
        printf("u32m8");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (1 * 64)), i;
        vuint64m1_t a = __riscv_vle64_v_u64m1(buf64, n);
        vuint64m1_t b = __riscv_vle64_v_u64m1(buf64 + n, n);
        vuint64m1_t r = __riscv_vadd_vv_u64m1(a, b, n);
        r = __riscv_vmul_vv_u64m1(r, a, n);
        r = __riscv_vxor_vv_u64m1(r, b, n);
        __riscv_vse64_v_u64m1(obuf, r, n);
        printf("u64m1");
        for (i = 0; i < n * (64 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 2 * 8 / (1 * 64)), i;
        vuint64m2_t a = __riscv_vle64_v_u64m2(buf64, n);
        vuint64m2_t b = __riscv_vle64_v_u64m2(buf64 + n, n);
        vuint64m2_t r = __riscv_vadd_vv_u64m2(a, b, n);
        r = __riscv_vmul_vv_u64m2(r, a, n);
        r = __riscv_vxor_vv_u64m2(r, b, n);
        __riscv_vse64_v_u64m2(obuf, r, n);
        printf("u64m2");
        for (i = 0; i < n * (64 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 4 * 8 / (1 * 64)), i;
        vuint64m4_t a = __riscv_vle64_v_u64m4(buf64, n);
        vuint64m4_t b = __riscv_vle64_v_u64m4(buf64 + n, n);
        vuint64m4_t r = __riscv_vadd_vv_u64m4(a, b, n);
        r = __riscv_vmul_vv_u64m4(r, a, n);
        r = __riscv_vxor_vv_u64m4(r, b, n);
        __riscv_vse64_v_u64m4(obuf, r, n);
        printf("u64m4");
        for (i = 0; i < n * (64 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 8 * 8 / (1 * 64)), i;
        vuint64m8_t a = __riscv_vle64_v_u64m8(buf64, n);
        vuint64m8_t b = __riscv_vle64_v_u64m8(buf64 + n, n);
        vuint64m8_t r = __riscv_vadd_vv_u64m8(a, b, n);
        r = __riscv_vmul_vv_u64m8(r, a, n);
        r = __riscv_vxor_vv_u64m8(r, b, n);
        __riscv_vse64_v_u64m8(obuf, r, n);
        printf("u64m8");
        for (i = 0; i < n * (64 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (2 * 32)), i;
        vfloat32mf2_t a = __riscv_vle32_v_f32mf2(buff, n);
        vfloat32mf2_t b = __riscv_vle32_v_f32mf2(buff + n, n);
        vfloat32mf2_t r = __riscv_vfadd_vv_f32mf2(a, b, n);
        r = __riscv_vfmul_vv_f32mf2(r, a, n);
        __riscv_vse32_v_f32mf2(obuf, r, n);
        printf("f32mf2");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (1 * 32)), i;
        vfloat32m1_t a = __riscv_vle32_v_f32m1(buff, n);
        vfloat32m1_t b = __riscv_vle32_v_f32m1(buff + n, n);
        vfloat32m1_t r = __riscv_vfadd_vv_f32m1(a, b, n);
        r = __riscv_vfmul_vv_f32m1(r, a, n);
        __riscv_vse32_v_f32m1(obuf, r, n);
        printf("f32m1");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 2 * 8 / (1 * 32)), i;
        vfloat32m2_t a = __riscv_vle32_v_f32m2(buff, n);
        vfloat32m2_t b = __riscv_vle32_v_f32m2(buff + n, n);
        vfloat32m2_t r = __riscv_vfadd_vv_f32m2(a, b, n);
        r = __riscv_vfmul_vv_f32m2(r, a, n);
        __riscv_vse32_v_f32m2(obuf, r, n);
        printf("f32m2");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 4 * 8 / (1 * 32)), i;
        vfloat32m4_t a = __riscv_vle32_v_f32m4(buff, n);
        vfloat32m4_t b = __riscv_vle32_v_f32m4(buff + n, n);
        vfloat32m4_t r = __riscv_vfadd_vv_f32m4(a, b, n);
        r = __riscv_vfmul_vv_f32m4(r, a, n);
        __riscv_vse32_v_f32m4(obuf, r, n);
        printf("f32m4");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 8 * 8 / (1 * 32)), i;
        vfloat32m8_t a = __riscv_vle32_v_f32m8(buff, n);
        vfloat32m8_t b = __riscv_vle32_v_f32m8(buff + n, n);
        vfloat32m8_t r = __riscv_vfadd_vv_f32m8(a, b, n);
        r = __riscv_vfmul_vv_f32m8(r, a, n);
        __riscv_vse32_v_f32m8(obuf, r, n);
        printf("f32m8");
        for (i = 0; i < n * (32 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 1 * 8 / (1 * 64)), i;
        vfloat64m1_t a = __riscv_vle64_v_f64m1(bufd, n);
        vfloat64m1_t b = __riscv_vle64_v_f64m1(bufd + n, n);
        vfloat64m1_t r = __riscv_vfadd_vv_f64m1(a, b, n);
        r = __riscv_vfmul_vv_f64m1(r, a, n);
        __riscv_vse64_v_f64m1(obuf, r, n);
        printf("f64m1");
        for (i = 0; i < n * (64 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 2 * 8 / (1 * 64)), i;
        vfloat64m2_t a = __riscv_vle64_v_f64m2(bufd, n);
        vfloat64m2_t b = __riscv_vle64_v_f64m2(bufd + n, n);
        vfloat64m2_t r = __riscv_vfadd_vv_f64m2(a, b, n);
        r = __riscv_vfmul_vv_f64m2(r, a, n);
        __riscv_vse64_v_f64m2(obuf, r, n);
        printf("f64m2");
        for (i = 0; i < n * (64 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 4 * 8 / (1 * 64)), i;
        vfloat64m4_t a = __riscv_vle64_v_f64m4(bufd, n);
        vfloat64m4_t b = __riscv_vle64_v_f64m4(bufd + n, n);
        vfloat64m4_t r = __riscv_vfadd_vv_f64m4(a, b, n);
        r = __riscv_vfmul_vv_f64m4(r, a, n);
        __riscv_vse64_v_f64m4(obuf, r, n);
        printf("f64m4");
        for (i = 0; i < n * (64 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    {
        int n = (__riscv_vlenb() * 8 * 8 / (1 * 64)), i;
        vfloat64m8_t a = __riscv_vle64_v_f64m8(bufd, n);
        vfloat64m8_t b = __riscv_vle64_v_f64m8(bufd + n, n);
        vfloat64m8_t r = __riscv_vfadd_vv_f64m8(a, b, n);
        r = __riscv_vfmul_vv_f64m8(r, a, n);
        __riscv_vse64_v_f64m8(obuf, r, n);
        printf("f64m8");
        for (i = 0; i < n * (64 / 8); i++) printf(" %02x", obuf[i]);
        printf("\n");
    }
    return 0;
}
