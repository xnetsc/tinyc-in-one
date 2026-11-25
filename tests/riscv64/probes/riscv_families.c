/* riscv64 RVV differential probe: generated sweep of
   every shipped family at its seed shapes
   (tools/riscv_gen.py).  Lane counts come from
   __riscv_vlenb(). */
#include <riscv_vector.h>
#include <stdio.h>
#include <string.h>

static signed char buf8[8192];
static short buf16[4096];
static int buf32[2048];
static long long buf64[1024];
static float buff[2048];
static double bufd[1024];
static unsigned char obuf[8192];
static signed char so8[8192];
static short so16[4096];
static int so32[2048];
static long long so64[1024];
static float sof[2048];
static double sod[1024];
static unsigned char sbuf[8192];
static unsigned char mbuf[64];
static unsigned char xb8[2048], gb8[2048];
static unsigned short xb16[1024], gb16[1024];
static unsigned int xb32[512], gb32[512];
static unsigned long long xb64[256], gb64[256];

static void pb(const char *tag, const unsigned char *p, int nbytes)
{
    int i;
    printf("%s", tag);
    for (i = 0; i < nbytes; i++) printf(" %02x", p[i]);
    printf("\n");
}

int main(void)
{
    int i;
    for (i = 0; i < 8192; i++) buf8[i] = (signed char)(i * 37 + 11);
    for (i = 0; i < 4096; i++) buf16[i] = (short)(i * 37 + 11);
    for (i = 0; i < 2048; i++) buf32[i] = i * 37 + 11;
    for (i = 0; i < 1024; i++) buf64[i] = i * 37 + 11;
    for (i = 0; i < 2048; i++) buff[i] = (i % 13) * 0.5f - 2.0f;
    for (i = 0; i < 1024; i++) bufd[i] = (i % 13) * 0.5 - 2.0;
    for (i = 0; i < 64; i++) mbuf[i] = (unsigned char)(0xA5 + i * 37);
    for (i = 0; i < 2048; i++) { xb8[i] = (unsigned char)((i % 16) * 8); gb8[i] = (unsigned char)(i % 8); }
    for (i = 0; i < 1024; i++) { xb16[i] = (unsigned short)((i % 16) * 8); gb16[i] = (unsigned short)(i % 8); }
    for (i = 0; i < 512; i++) { xb32[i] = (unsigned int)((i % 16) * 8); gb32[i] = (unsigned int)(i % 8); }
    for (i = 0; i < 256; i++) { xb64[i] = (unsigned long long)((i % 16) * 8); gb64[i] = (unsigned long long)(i % 8); }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vmv_v_v_f32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmv_v_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmv_v_v_i32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmv_v_v_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vmv_v_v_i64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vmv_v_v_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vmv_v_v_i8m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vmv_v_v_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmv_v_v_u32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmv_v_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint16m1_t r = __riscv_vmv_v_x_i16m1(3, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vmv_v_x_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint32m1_t r = __riscv_vmv_v_x_i32m1(3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmv_v_x_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m2_t r = __riscv_vmv_v_x_i32m2(3, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vmv_v_x_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32mf2_t r = __riscv_vmv_v_x_i32mf2(3, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_i32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vmv_v_x_i32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t r = __riscv_vmv_v_x_i64m1(3, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vmv_v_x_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t r = __riscv_vmv_v_x_i8m1(3, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vmv_v_x_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t r = __riscv_vmv_v_x_u32m1(3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmv_v_x_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m1_t r = __riscv_vmv_v_x_u64m1(3, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_u64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vmv_v_x_u64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint8m1_t r = __riscv_vmv_v_x_u8m1(3, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_u8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vmv_v_x_u8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vfloat32m1_t r = __riscv_vfmv_v_f_f32m1(0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmv_v_f_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t r = __riscv_vfmv_v_f_f64m1(0.5, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfmv_v_f_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        {
            long long r = (long long)__riscv_vmv_x_s_i32m1_i32(va);
            printf("__riscv_vmv_x_s_i32m1_i32 %lld\n", r);
        }
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        {
            long long r = (long long)__riscv_vmv_x_s_u32m1_u32(va);
            printf("__riscv_vmv_x_s_u32m1_u32 %lld\n", r);
        }
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        {
            double r = (double)__riscv_vfmv_f_s_f32m1_f32(va);
            unsigned long long u;
            memcpy(&u, &r, 8);
            printf("__riscv_vfmv_f_s_f32m1_f32 %llx\n", u);
        }
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vadd_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m2_t va = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t vb = __riscv_vle32_v_i32m2((buf32 + (__riscv_vlenb() * 2 * 8 / (1 * 32))), (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t r = __riscv_vadd_vv_i32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vadd_vv_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32mf2_t va = __riscv_vle32_v_i32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint32mf2_t vb = __riscv_vle32_v_i32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint32mf2_t r = __riscv_vadd_vv_i32mf2(va, vb, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_i32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vadd_vv_i32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t vb = __riscv_vle64_v_i64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vadd_vv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vadd_vv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vb = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vadd_vv_i8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vadd_vv_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vadd_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vadd_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vadd_vx_i64m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vadd_vx_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vadd_vx_i8m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vadd_vx_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vadd_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsub_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t vb = __riscv_vle64_v_i64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vsub_vv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vsub_vv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vb = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vsub_vv_i8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vsub_vv_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsub_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsub_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsub_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrsub_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrsub_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vrsub_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrsub_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrsub_vx_i32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vrsub_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vrsub_vx_u32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vrsub_vx_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrsub_vx_i32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrsub_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vrsub_vx_u32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrsub_vx_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrsub_vx_i32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrsub_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vrsub_vx_u32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrsub_vx_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrsub_vx_i32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrsub_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vrsub_vx_u32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrsub_vx_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmul_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m2_t va = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t vb = __riscv_vle32_v_i32m2((buf32 + (__riscv_vlenb() * 2 * 8 / (1 * 32))), (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t r = __riscv_vmul_vv_i32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vmul_vv_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32mf2_t va = __riscv_vle32_v_i32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint32mf2_t vb = __riscv_vle32_v_i32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint32mf2_t r = __riscv_vmul_vv_i32mf2(va, vb, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_i32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vmul_vv_i32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t vb = __riscv_vle64_v_i64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vmul_vv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vmul_vv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vb = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vmul_vv_i8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vmul_vv_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmul_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmul_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmul_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vdiv_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdiv_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vdiv_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdiv_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrem_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrem_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrem_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrem_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vand_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vand_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t vb = __riscv_vle64_v_i64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vand_vv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vand_vv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vb = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vand_vv_i8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vand_vv_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vand_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vand_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vand_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vand_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vand_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vand_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vor_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vor_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t vb = __riscv_vle64_v_i64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vor_vv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vor_vv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vb = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vor_vv_i8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vor_vv_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vor_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vor_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vor_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vor_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vor_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vor_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vxor_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vxor_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t vb = __riscv_vle64_v_i64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vxor_vv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vxor_vv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vb = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vxor_vv_i8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vxor_vv_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vxor_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vxor_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vxor_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vxor_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vxor_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vxor_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmin_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmin_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vb = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vmin_vv_i8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vmin_vv_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmin_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmin_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmax_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmax_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vb = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vmax_vv_i8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vmax_vv_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmax_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmax_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsll_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint64m1_t vb = __riscv_vle64_v_u64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vsll_vv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vsll_vv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vuint8m1_t vb = __riscv_vle8_v_u8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vsll_vv_i8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vsll_vv_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsll_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsll_vx_i32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsll_vx_u32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsra_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsra_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint64m1_t vb = __riscv_vle64_v_u64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vsra_vv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vsra_vv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vuint8m1_t vb = __riscv_vle8_v_u8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vsra_vv_i8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vsra_vv_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsra_vx_i32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsra_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vneg_v_i32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vneg_v_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vneg_v_i64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vneg_v_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vneg_v_i8m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vneg_v_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vneg_v_u32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vneg_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnot_v_i32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnot_v_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vnot_v_i8m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vnot_v_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vnot_v_u32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnot_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmseq_vv_i32m1_b32(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmseq_vv_i32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmseq_vv_u32m1_b32(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmseq_vv_u32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmseq_vx_i32m1_b32(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmseq_vx_i32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmseq_vx_u32m1_b32(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmseq_vx_u32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsne_vv_i32m1_b32(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsne_vv_i32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsne_vv_u32m1_b32(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsne_vv_u32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsne_vx_i32m1_b32(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsne_vx_i32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsne_vx_u32m1_b32(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsne_vx_u32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmslt_vv_i32m1_b32(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmslt_vv_i32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmslt_vx_i32m1_b32(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmslt_vx_i32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsle_vv_i32m1_b32(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsle_vv_i32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsle_vx_i32m1_b32(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsle_vx_i32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vdivu_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdivu_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vdivu_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdivu_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vremu_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vremu_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vremu_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vremu_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vminu_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vminu_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vminu_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vminu_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmaxu_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmaxu_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmaxu_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmaxu_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsrl_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsrl_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsrl_vx_u32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsrl_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsltu_vv_u32m1_b32(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsltu_vv_u32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsltu_vx_u32m1_b32(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsltu_vx_u32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsleu_vv_u32m1_b32(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsleu_vv_u32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsleu_vx_u32m1_b32(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsleu_vx_u32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vb = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vbool8_t r = __riscv_vmseq_vv_i8m1_b8(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 8)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 8)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 8)) % 8)) - 1);
        pb("__riscv_vmseq_vv_i8m1_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 8)) + 7) / 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vb = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vbool8_t r = __riscv_vmslt_vv_i8m1_b8(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 8)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 8)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 8)) % 8)) - 1);
        pb("__riscv_vmslt_vv_i8m1_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 8)) + 7) / 8));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t vb = __riscv_vle64_v_i64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vbool64_t r = __riscv_vmseq_vv_i64m1_b64(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 64)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 64)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 64)) % 8)) - 1);
        pb("__riscv_vmseq_vv_i64m1_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 64)) + 7) / 8));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t vb = __riscv_vle64_v_i64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vbool64_t r = __riscv_vmslt_vv_i64m1_b64(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 64)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 64)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 64)) % 8)) - 1);
        pb("__riscv_vmslt_vv_i64m1_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 64)) + 7) / 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfadd_vv_f32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfadd_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfadd_vv_f64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfadd_vv_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfadd_vf_f32m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfadd_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfadd_vf_f64m1(va, 0.5, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfadd_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsub_vv_f32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsub_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfsub_vv_f64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfsub_vv_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsub_vf_f32m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsub_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfsub_vf_f64m1(va, 0.5, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfsub_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmul_vv_f32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmul_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfmul_vv_f64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfmul_vv_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmul_vf_f32m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmul_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfmul_vf_f64m1(va, 0.5, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfmul_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfdiv_vv_f32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfdiv_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfdiv_vv_f64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfdiv_vv_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfdiv_vf_f32m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfdiv_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfdiv_vf_f64m1(va, 0.5, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfdiv_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmin_vv_f32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmin_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmin_vf_f32m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmin_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmax_vv_f32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmax_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmax_vf_f32m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmax_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsgnj_vv_f32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsgnj_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsgnj_vf_f32m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsgnj_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsgnjn_vv_f32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsgnjn_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsgnjn_vf_f32m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsgnjn_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsgnjx_vv_f32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsgnjx_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsgnjx_vf_f32m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsgnjx_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfabs_v_f32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfabs_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfabs_v_f64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfabs_v_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfneg_v_f32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfneg_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfneg_v_f64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfneg_v_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vfclass_v_u32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfclass_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsqrt_v_f32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsqrt_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfeq_vv_f32m1_b32(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfeq_vv_f32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfeq_vf_f32m1_b32(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfeq_vf_f32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfne_vv_f32m1_b32(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfne_vv_f32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfne_vf_f32m1_b32(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfne_vf_f32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmflt_vv_f32m1_b32(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmflt_vv_f32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmflt_vf_f32m1_b32(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmflt_vf_f32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfle_vv_f32m1_b32(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfle_vv_f32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfle_vf_f32m1_b32(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfle_vf_f32m1_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vbool64_t r = __riscv_vmfeq_vv_f64m1_b64(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 64)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 64)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 64)) % 8)) - 1);
        pb("__riscv_vmfeq_vv_f64m1_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 64)) + 7) / 8));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vbool64_t r = __riscv_vmflt_vv_f64m1_b64(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 64)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 64)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 64)) % 8)) - 1);
        pb("__riscv_vmflt_vv_f64m1_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 64)) + 7) / 8));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmulh_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmulh_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmulh_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmulh_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmulhu_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmulhu_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmulhu_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmulhu_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmulhsu_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmulhsu_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmulhsu_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmulhsu_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsadd_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsadd_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsadd_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsadd_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vssub_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vssub_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vssub_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vssub_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsaddu_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsaddu_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsaddu_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsaddu_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vssubu_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vssubu_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vssubu_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vssubu_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vaadd_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vaadd_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vaadd_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vaadd_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vasub_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vasub_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vasub_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vasub_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vaaddu_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vaaddu_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vaaddu_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vaaddu_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vasubu_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vasubu_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vasubu_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vasubu_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t vd = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmacc_vv_i32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmacc_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t vd = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmacc_vv_u32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmacc_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t vd = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmacc_vx_i32m1(vd, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmacc_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t vd = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmacc_vx_u32m1(vd, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmacc_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t vd = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmadd_vv_i32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmadd_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t vd = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmadd_vv_u32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmadd_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t vd = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmadd_vx_i32m1(vd, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmadd_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t vd = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmadd_vx_u32m1(vd, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmadd_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t vd = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsac_vv_i32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsac_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t vd = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vnmsac_vv_u32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsac_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t vd = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsac_vx_i32m1(vd, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsac_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t vd = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vnmsac_vx_u32m1(vd, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsac_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t vd = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsub_vv_i32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsub_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t vd = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vnmsub_vv_u32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsub_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t vd = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsub_vx_i32m1(vd, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsub_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t vd = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vnmsub_vx_u32m1(vd, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsub_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmacc_vv_f32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmacc_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfmacc_vv_f64m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfmacc_vv_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmacc_vf_f32m1(vd, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmacc_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfmacc_vf_f64m1(vd, 0.5, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfmacc_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmadd_vv_f32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmadd_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfmadd_vv_f64m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfmadd_vv_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmadd_vf_f32m1(vd, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmadd_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfmadd_vf_f64m1(vd, 0.5, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfmadd_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmsac_vv_f32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmsac_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfnmsac_vv_f64m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfnmsac_vv_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmsac_vf_f32m1(vd, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmsac_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfnmsac_vf_f64m1(vd, 0.5, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfnmsac_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmsub_vv_f32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmsub_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfnmsub_vv_f64m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfnmsub_vv_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmsub_vf_f32m1(vd, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmsub_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfnmsub_vf_f64m1(vd, 0.5, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfnmsub_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8mf2_t va = __riscv_vle8_v_i8mf2(buf8, (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vint8mf2_t vb = __riscv_vle8_v_i8mf2((buf8 + (__riscv_vlenb() * 1 * 8 / (2 * 8))), (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vint16m1_t r = __riscv_vwadd_vv_i16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwadd_vv_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint16mf2_t va = __riscv_vle16_v_i16mf2(buf16, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vint16mf2_t vb = __riscv_vle16_v_i16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vint32m1_t r = __riscv_vwadd_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwadd_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t vb = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint32m2_t r = __riscv_vwadd_vv_i32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwadd_vv_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32mf2_t va = __riscv_vle32_v_i32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint32mf2_t vb = __riscv_vle32_v_i32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint64m1_t r = __riscv_vwadd_vv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwadd_vv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint8mf2_t vb = __riscv_vle8_v_i8mf2((buf8 + (__riscv_vlenb() * 1 * 8 / (2 * 8))), (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vint16m1_t r = __riscv_vwadd_wv_i16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwadd_wv_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint16mf2_t vb = __riscv_vle16_v_i16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vint32m1_t r = __riscv_vwadd_wv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwadd_wv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m2_t va = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint16m1_t vb = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint32m2_t r = __riscv_vwadd_wv_i32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwadd_wv_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint32mf2_t vb = __riscv_vle32_v_i32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint64m1_t r = __riscv_vwadd_wv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwadd_wv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8mf2_t va = __riscv_vle8_v_i8mf2(buf8, (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vint16m1_t r = __riscv_vwadd_vx_i16m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwadd_vx_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint16mf2_t va = __riscv_vle16_v_i16mf2(buf16, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vint32m1_t r = __riscv_vwadd_vx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwadd_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint32m2_t r = __riscv_vwadd_vx_i32m2(va, 3, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwadd_vx_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32mf2_t va = __riscv_vle32_v_i32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint64m1_t r = __riscv_vwadd_vx_i64m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwadd_vx_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vwadd_wx_i16m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwadd_wx_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vwadd_wx_i32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwadd_wx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m2_t va = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t r = __riscv_vwadd_wx_i32m2(va, 3, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwadd_wx_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vwadd_wx_i64m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwadd_wx_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8mf2_t va = __riscv_vle8_v_i8mf2(buf8, (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vint8mf2_t vb = __riscv_vle8_v_i8mf2((buf8 + (__riscv_vlenb() * 1 * 8 / (2 * 8))), (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vint16m1_t r = __riscv_vwsub_vv_i16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwsub_vv_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint16mf2_t va = __riscv_vle16_v_i16mf2(buf16, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vint16mf2_t vb = __riscv_vle16_v_i16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vint32m1_t r = __riscv_vwsub_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwsub_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t vb = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint32m2_t r = __riscv_vwsub_vv_i32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwsub_vv_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32mf2_t va = __riscv_vle32_v_i32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint32mf2_t vb = __riscv_vle32_v_i32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint64m1_t r = __riscv_vwsub_vv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwsub_vv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint8mf2_t vb = __riscv_vle8_v_i8mf2((buf8 + (__riscv_vlenb() * 1 * 8 / (2 * 8))), (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vint16m1_t r = __riscv_vwsub_wv_i16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwsub_wv_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint16mf2_t vb = __riscv_vle16_v_i16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vint32m1_t r = __riscv_vwsub_wv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwsub_wv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m2_t va = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint16m1_t vb = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint32m2_t r = __riscv_vwsub_wv_i32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwsub_wv_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint32mf2_t vb = __riscv_vle32_v_i32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint64m1_t r = __riscv_vwsub_wv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwsub_wv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8mf2_t va = __riscv_vle8_v_i8mf2(buf8, (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vint8mf2_t vb = __riscv_vle8_v_i8mf2((buf8 + (__riscv_vlenb() * 1 * 8 / (2 * 8))), (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vint16m1_t r = __riscv_vwmul_vv_i16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwmul_vv_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint16mf2_t va = __riscv_vle16_v_i16mf2(buf16, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vint16mf2_t vb = __riscv_vle16_v_i16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vint32m1_t r = __riscv_vwmul_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwmul_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t vb = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint32m2_t r = __riscv_vwmul_vv_i32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwmul_vv_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32mf2_t va = __riscv_vle32_v_i32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint32mf2_t vb = __riscv_vle32_v_i32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint64m1_t r = __riscv_vwmul_vv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwmul_vv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8mf2_t va = __riscv_vle8_v_i8mf2(buf8, (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vuint8mf2_t vb = __riscv_vle8_v_u8mf2((buf8 + (__riscv_vlenb() * 1 * 8 / (2 * 8))), (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vint16m1_t r = __riscv_vwmulsu_vv_i16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwmulsu_vv_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint16mf2_t va = __riscv_vle16_v_i16mf2(buf16, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vuint16mf2_t vb = __riscv_vle16_v_u16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vint32m1_t r = __riscv_vwmulsu_vv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwmulsu_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint16m1_t vb = __riscv_vle16_v_u16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint32m2_t r = __riscv_vwmulsu_vv_i32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwmulsu_vv_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32mf2_t va = __riscv_vle32_v_i32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint32mf2_t vb = __riscv_vle32_v_u32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint64m1_t r = __riscv_vwmulsu_vv_i64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwmulsu_vv_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint8mf2_t va = __riscv_vle8_v_u8mf2(buf8, (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vuint8mf2_t vb = __riscv_vle8_v_u8mf2((buf8 + (__riscv_vlenb() * 1 * 8 / (2 * 8))), (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vuint16m1_t r = __riscv_vwaddu_vv_u16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_u16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwaddu_vv_u16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint16mf2_t va = __riscv_vle16_v_u16mf2(buf16, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vuint16mf2_t vb = __riscv_vle16_v_u16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vuint32m1_t r = __riscv_vwaddu_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwaddu_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint16m1_t va = __riscv_vle16_v_u16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint16m1_t vb = __riscv_vle16_v_u16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint32m2_t r = __riscv_vwaddu_vv_u32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_u32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwaddu_vv_u32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32mf2_t va = __riscv_vle32_v_u32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint32mf2_t vb = __riscv_vle32_v_u32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint64m1_t r = __riscv_vwaddu_vv_u64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_u64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwaddu_vv_u64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint16m1_t va = __riscv_vle16_v_u16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint8mf2_t vb = __riscv_vle8_v_u8mf2((buf8 + (__riscv_vlenb() * 1 * 8 / (2 * 8))), (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vuint16m1_t r = __riscv_vwaddu_wv_u16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_u16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwaddu_wv_u16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint16mf2_t vb = __riscv_vle16_v_u16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vuint32m1_t r = __riscv_vwaddu_wv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwaddu_wv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m2_t va = __riscv_vle32_v_u32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vuint16m1_t vb = __riscv_vle16_v_u16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint32m2_t r = __riscv_vwaddu_wv_u32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_u32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwaddu_wv_u32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m1_t va = __riscv_vle64_v_u64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint32mf2_t vb = __riscv_vle32_v_u32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint64m1_t r = __riscv_vwaddu_wv_u64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_u64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwaddu_wv_u64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint8mf2_t va = __riscv_vle8_v_u8mf2(buf8, (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vuint16m1_t r = __riscv_vwaddu_vx_u16m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_u16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwaddu_vx_u16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint16mf2_t va = __riscv_vle16_v_u16mf2(buf16, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vuint32m1_t r = __riscv_vwaddu_vx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwaddu_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint16m1_t va = __riscv_vle16_v_u16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint32m2_t r = __riscv_vwaddu_vx_u32m2(va, 3, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_u32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwaddu_vx_u32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32mf2_t va = __riscv_vle32_v_u32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint64m1_t r = __riscv_vwaddu_vx_u64m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_u64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwaddu_vx_u64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint16m1_t va = __riscv_vle16_v_u16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint16m1_t r = __riscv_vwaddu_wx_u16m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_u16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwaddu_wx_u16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vwaddu_wx_u32m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwaddu_wx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m2_t va = __riscv_vle32_v_u32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vuint32m2_t r = __riscv_vwaddu_wx_u32m2(va, 3, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_u32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwaddu_wx_u32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m1_t va = __riscv_vle64_v_u64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint64m1_t r = __riscv_vwaddu_wx_u64m1(va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_u64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwaddu_wx_u64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint8mf2_t va = __riscv_vle8_v_u8mf2(buf8, (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vuint8mf2_t vb = __riscv_vle8_v_u8mf2((buf8 + (__riscv_vlenb() * 1 * 8 / (2 * 8))), (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vuint16m1_t r = __riscv_vwsubu_vv_u16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_u16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwsubu_vv_u16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint16mf2_t va = __riscv_vle16_v_u16mf2(buf16, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vuint16mf2_t vb = __riscv_vle16_v_u16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vuint32m1_t r = __riscv_vwsubu_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwsubu_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint16m1_t va = __riscv_vle16_v_u16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint16m1_t vb = __riscv_vle16_v_u16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint32m2_t r = __riscv_vwsubu_vv_u32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_u32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwsubu_vv_u32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32mf2_t va = __riscv_vle32_v_u32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint32mf2_t vb = __riscv_vle32_v_u32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint64m1_t r = __riscv_vwsubu_vv_u64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_u64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwsubu_vv_u64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint16m1_t va = __riscv_vle16_v_u16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint8mf2_t vb = __riscv_vle8_v_u8mf2((buf8 + (__riscv_vlenb() * 1 * 8 / (2 * 8))), (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vuint16m1_t r = __riscv_vwsubu_wv_u16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_u16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwsubu_wv_u16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint16mf2_t vb = __riscv_vle16_v_u16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vuint32m1_t r = __riscv_vwsubu_wv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwsubu_wv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m2_t va = __riscv_vle32_v_u32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vuint16m1_t vb = __riscv_vle16_v_u16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint32m2_t r = __riscv_vwsubu_wv_u32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_u32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwsubu_wv_u32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m1_t va = __riscv_vle64_v_u64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint32mf2_t vb = __riscv_vle32_v_u32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint64m1_t r = __riscv_vwsubu_wv_u64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_u64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwsubu_wv_u64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint8mf2_t va = __riscv_vle8_v_u8mf2(buf8, (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vuint8mf2_t vb = __riscv_vle8_v_u8mf2((buf8 + (__riscv_vlenb() * 1 * 8 / (2 * 8))), (__riscv_vlenb() * 1 * 8 / (2 * 8)));
        vuint16m1_t r = __riscv_vwmulu_vv_u16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_u16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vwmulu_vv_u16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint16mf2_t va = __riscv_vle16_v_u16mf2(buf16, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vuint16mf2_t vb = __riscv_vle16_v_u16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vuint32m1_t r = __riscv_vwmulu_vv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwmulu_vv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint16m1_t va = __riscv_vle16_v_u16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint16m1_t vb = __riscv_vle16_v_u16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint32m2_t r = __riscv_vwmulu_vv_u32m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_u32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vwmulu_vv_u32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32mf2_t va = __riscv_vle32_v_u32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint32mf2_t vb = __riscv_vle32_v_u32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint64m1_t r = __riscv_vwmulu_vv_u64m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_u64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vwmulu_vv_u64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint32m2_t va = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vuint16m1_t vb = __riscv_vle16_v_u16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vnsra_wv_i16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vnsra_wv_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint16mf2_t vb = __riscv_vle16_v_u16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vint16mf2_t r = __riscv_vnsra_wv_i16mf2(va, vb, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
__riscv_vse16_v_i16mf2(so16, r, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        pb("__riscv_vnsra_wv_i16mf2", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (2 * 16)) * 2));
    }
    {
        int i;
        vint64m2_t va = __riscv_vle64_v_i64m2(buf64, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnsra_wv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnsra_wv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint32mf2_t vb = __riscv_vle32_v_u32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint32mf2_t r = __riscv_vnsra_wv_i32mf2(va, vb, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_i32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vnsra_wv_i32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vint16m2_t va = __riscv_vle16_v_i16m2(buf16, (__riscv_vlenb() * 2 * 8 / (1 * 16)));
        vuint8m1_t vb = __riscv_vle8_v_u8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vnsra_wv_i8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vnsra_wv_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vint32m2_t va = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint16m1_t r = __riscv_vnsra_wx_i16m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vnsra_wx_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint16mf2_t r = __riscv_vnsra_wx_i16mf2(va, 1, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
__riscv_vse16_v_i16mf2(so16, r, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        pb("__riscv_vnsra_wx_i16mf2", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (2 * 16)) * 2));
    }
    {
        int i;
        vint64m2_t va = __riscv_vle64_v_i64m2(buf64, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vint32m1_t r = __riscv_vnsra_wx_i32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnsra_wx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint32mf2_t r = __riscv_vnsra_wx_i32mf2(va, 1, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_i32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vnsra_wx_i32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vint16m2_t va = __riscv_vle16_v_i16m2(buf16, (__riscv_vlenb() * 2 * 8 / (1 * 16)));
        vint8m1_t r = __riscv_vnsra_wx_i8m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vnsra_wx_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vint32m2_t va = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vuint16m1_t vb = __riscv_vle16_v_u16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vnclip_wv_i16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vnclip_wv_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint16mf2_t vb = __riscv_vle16_v_u16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vint16mf2_t r = __riscv_vnclip_wv_i16mf2(va, vb, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
__riscv_vse16_v_i16mf2(so16, r, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        pb("__riscv_vnclip_wv_i16mf2", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (2 * 16)) * 2));
    }
    {
        int i;
        vint64m2_t va = __riscv_vle64_v_i64m2(buf64, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnclip_wv_i32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnclip_wv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint32mf2_t vb = __riscv_vle32_v_u32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint32mf2_t r = __riscv_vnclip_wv_i32mf2(va, vb, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_i32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vnclip_wv_i32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vint16m2_t va = __riscv_vle16_v_i16m2(buf16, (__riscv_vlenb() * 2 * 8 / (1 * 16)));
        vuint8m1_t vb = __riscv_vle8_v_u8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vnclip_wv_i8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vnclip_wv_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vint32m2_t va = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint16m1_t r = __riscv_vnclip_wx_i16m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vnclip_wx_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint16mf2_t r = __riscv_vnclip_wx_i16mf2(va, 1, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
__riscv_vse16_v_i16mf2(so16, r, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        pb("__riscv_vnclip_wx_i16mf2", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (2 * 16)) * 2));
    }
    {
        int i;
        vint64m2_t va = __riscv_vle64_v_i64m2(buf64, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vint32m1_t r = __riscv_vnclip_wx_i32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnclip_wx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint32mf2_t r = __riscv_vnclip_wx_i32mf2(va, 1, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_i32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vnclip_wx_i32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vint16m2_t va = __riscv_vle16_v_i16m2(buf16, (__riscv_vlenb() * 2 * 8 / (1 * 16)));
        vint8m1_t r = __riscv_vnclip_wx_i8m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vnclip_wx_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m2_t va = __riscv_vle32_v_u32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vuint16m1_t vb = __riscv_vle16_v_u16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint16m1_t r = __riscv_vnsrl_wv_u16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_u16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vnsrl_wv_u16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint16mf2_t vb = __riscv_vle16_v_u16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vuint16mf2_t r = __riscv_vnsrl_wv_u16mf2(va, vb, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
__riscv_vse16_v_u16mf2(so16, r, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        pb("__riscv_vnsrl_wv_u16mf2", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (2 * 16)) * 2));
    }
    {
        int i;
        vuint64m2_t va = __riscv_vle64_v_u64m2(buf64, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vnsrl_wv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnsrl_wv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m1_t va = __riscv_vle64_v_u64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint32mf2_t vb = __riscv_vle32_v_u32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint32mf2_t r = __riscv_vnsrl_wv_u32mf2(va, vb, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_u32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vnsrl_wv_u32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vuint16m2_t va = __riscv_vle16_v_u16m2(buf16, (__riscv_vlenb() * 2 * 8 / (1 * 16)));
        vuint8m1_t vb = __riscv_vle8_v_u8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vuint8m1_t r = __riscv_vnsrl_wv_u8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_u8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vnsrl_wv_u8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m2_t va = __riscv_vle32_v_u32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vuint16m1_t r = __riscv_vnsrl_wx_u16m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_u16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vnsrl_wx_u16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint16mf2_t r = __riscv_vnsrl_wx_u16mf2(va, 1, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
__riscv_vse16_v_u16mf2(so16, r, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        pb("__riscv_vnsrl_wx_u16mf2", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (2 * 16)) * 2));
    }
    {
        int i;
        vuint64m2_t va = __riscv_vle64_v_u64m2(buf64, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vuint32m1_t r = __riscv_vnsrl_wx_u32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnsrl_wx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m1_t va = __riscv_vle64_v_u64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint32mf2_t r = __riscv_vnsrl_wx_u32mf2(va, 1, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_u32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vnsrl_wx_u32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vuint16m2_t va = __riscv_vle16_v_u16m2(buf16, (__riscv_vlenb() * 2 * 8 / (1 * 16)));
        vuint8m1_t r = __riscv_vnsrl_wx_u8m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_u8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vnsrl_wx_u8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m2_t va = __riscv_vle32_v_u32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vuint16m1_t vb = __riscv_vle16_v_u16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vuint16m1_t r = __riscv_vnclipu_wv_u16m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_u16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vnclipu_wv_u16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint16mf2_t vb = __riscv_vle16_v_u16mf2((buf16 + (__riscv_vlenb() * 1 * 8 / (2 * 16))), (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        vuint16mf2_t r = __riscv_vnclipu_wv_u16mf2(va, vb, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
__riscv_vse16_v_u16mf2(so16, r, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        pb("__riscv_vnclipu_wv_u16mf2", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (2 * 16)) * 2));
    }
    {
        int i;
        vuint64m2_t va = __riscv_vle64_v_u64m2(buf64, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vnclipu_wv_u32m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnclipu_wv_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m1_t va = __riscv_vle64_v_u64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint32mf2_t vb = __riscv_vle32_v_u32mf2((buf32 + (__riscv_vlenb() * 1 * 8 / (2 * 32))), (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint32mf2_t r = __riscv_vnclipu_wv_u32mf2(va, vb, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_u32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vnclipu_wv_u32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vuint16m2_t va = __riscv_vle16_v_u16m2(buf16, (__riscv_vlenb() * 2 * 8 / (1 * 16)));
        vuint8m1_t vb = __riscv_vle8_v_u8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vuint8m1_t r = __riscv_vnclipu_wv_u8m1(va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_u8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vnclipu_wv_u8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m2_t va = __riscv_vle32_v_u32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vuint16m1_t r = __riscv_vnclipu_wx_u16m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_u16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vnclipu_wx_u16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint16mf2_t r = __riscv_vnclipu_wx_u16mf2(va, 1, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
__riscv_vse16_v_u16mf2(so16, r, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        pb("__riscv_vnclipu_wx_u16mf2", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (2 * 16)) * 2));
    }
    {
        int i;
        vuint64m2_t va = __riscv_vle64_v_u64m2(buf64, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vuint32m1_t r = __riscv_vnclipu_wx_u32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnclipu_wx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m1_t va = __riscv_vle64_v_u64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint32mf2_t r = __riscv_vnclipu_wx_u32mf2(va, 1, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_u32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vnclipu_wx_u32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vuint16m2_t va = __riscv_vle16_v_u16m2(buf16, (__riscv_vlenb() * 2 * 8 / (1 * 16)));
        vuint8m1_t r = __riscv_vnclipu_wx_u8m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_u8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vnclipu_wx_u8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vadd_vv_i32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vadd_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vadd_vv_u32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vadd_vv_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vadd_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vadd_vv_u32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vv_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vadd_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vadd_vv_u32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vv_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vadd_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vadd_vv_u32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vv_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vadd_vx_i32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vadd_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vadd_vx_u32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vadd_vx_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vadd_vx_i32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vadd_vx_u32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vx_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vadd_vx_i32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vadd_vx_u32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vx_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vadd_vx_i32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vadd_vx_u32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadd_vx_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsub_vv_i32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vsub_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsub_vv_u32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vsub_vv_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsub_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsub_vv_u32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vv_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsub_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsub_vv_u32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vv_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsub_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsub_vv_u32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vv_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsub_vx_i32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vsub_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsub_vx_u32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vsub_vx_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsub_vx_i32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsub_vx_u32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vx_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsub_vx_i32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsub_vx_u32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vx_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsub_vx_i32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsub_vx_u32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsub_vx_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmul_vv_i32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmul_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmul_vv_u32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmul_vv_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmul_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmul_vv_u32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vv_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmul_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmul_vv_u32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vv_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmul_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmul_vv_u32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vv_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmul_vx_i32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmul_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmul_vx_u32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmul_vx_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmul_vx_i32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmul_vx_u32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vx_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmul_vx_i32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmul_vx_u32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vx_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmul_vx_i32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmul_vx_u32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmul_vx_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vdiv_vv_i32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vdiv_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vdiv_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdiv_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vdiv_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdiv_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vdiv_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdiv_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vdiv_vx_i32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vdiv_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vdiv_vx_i32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdiv_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vdiv_vx_i32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdiv_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vdiv_vx_i32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdiv_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrem_vv_i32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vrem_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrem_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrem_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrem_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrem_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrem_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrem_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrem_vx_i32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vrem_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrem_vx_i32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrem_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrem_vx_i32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrem_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrem_vx_i32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrem_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vand_vv_i32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vand_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vand_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vand_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vand_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vand_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vand_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vand_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vand_vx_i32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vand_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vand_vx_i32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vand_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vand_vx_i32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vand_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vand_vx_i32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vand_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vor_vv_i32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vor_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vor_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vor_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vor_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vor_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vor_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vor_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vor_vx_i32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vor_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vor_vx_i32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vor_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vor_vx_i32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vor_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vor_vx_i32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vor_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vxor_vv_i32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vxor_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vxor_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vxor_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vxor_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vxor_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vxor_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vxor_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vxor_vx_i32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vxor_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vxor_vx_i32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vxor_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vxor_vx_i32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vxor_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vxor_vx_i32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vxor_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmin_vv_i32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmin_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmin_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmin_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmin_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmin_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmin_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmin_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmin_vx_i32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmin_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmin_vx_i32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmin_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmin_vx_i32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmin_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmin_vx_i32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmin_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmax_vv_i32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmax_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmax_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmax_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmax_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmax_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmax_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmax_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmax_vx_i32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmax_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmax_vx_i32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmax_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmax_vx_i32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmax_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmax_vx_i32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmax_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsll_vv_i32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vsll_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsll_vv_u32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vsll_vv_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsll_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsll_vv_u32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vv_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsll_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsll_vv_u32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vv_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsll_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsll_vv_u32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vv_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsll_vx_i32m1_m(vm, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vsll_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsll_vx_u32m1_m(vm, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vsll_vx_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsll_vx_i32m1_mu(vm, vo, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsll_vx_u32m1_mu(vm, vo, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vx_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsll_vx_i32m1_tum(vm, vo, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsll_vx_u32m1_tum(vm, vo, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vx_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsll_vx_i32m1_tumu(vm, vo, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsll_vx_u32m1_tumu(vm, vo, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsll_vx_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsra_vv_i32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vsra_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsra_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsra_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsra_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsra_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsra_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsra_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsra_vx_i32m1_m(vm, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vsra_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsra_vx_i32m1_mu(vm, vo, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsra_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsra_vx_i32m1_tum(vm, vo, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsra_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsra_vx_i32m1_tumu(vm, vo, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsra_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmseq_vv_i32m1_b32_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmseq_vv_i32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmseq_vv_i32m1_b32_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmseq_vv_i32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmseq_vv_u32m1_b32_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmseq_vv_u32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmseq_vx_i32m1_b32_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmseq_vx_i32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmseq_vx_i32m1_b32_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmseq_vx_i32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmseq_vx_u32m1_b32_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmseq_vx_u32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsne_vv_i32m1_b32_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsne_vv_i32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsne_vv_i32m1_b32_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsne_vv_i32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsne_vv_u32m1_b32_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsne_vv_u32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsne_vx_i32m1_b32_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsne_vx_i32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsne_vx_i32m1_b32_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsne_vx_i32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsne_vx_u32m1_b32_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsne_vx_u32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsne_vv_u32m1_b32_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsne_vv_u32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsne_vx_u32m1_b32_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsne_vx_u32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmslt_vv_i32m1_b32_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmslt_vv_i32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmslt_vv_i32m1_b32_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmslt_vv_i32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmslt_vx_i32m1_b32_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmslt_vx_i32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmslt_vx_i32m1_b32_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmslt_vx_i32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsle_vv_i32m1_b32_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsle_vv_i32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsle_vv_i32m1_b32_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsle_vv_i32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsle_vx_i32m1_b32_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsle_vx_i32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsle_vx_i32m1_b32_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsle_vx_i32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmacc_vv_i32m1_m(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmacc_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmacc_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmacc_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmacc_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmacc_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmacc_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmacc_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmacc_vx_i32m1_m(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmacc_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmacc_vx_i32m1_mu(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmacc_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmacc_vx_i32m1_tum(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmacc_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmacc_vx_i32m1_tumu(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmacc_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmadd_vv_i32m1_m(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmadd_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmadd_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmadd_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmadd_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmadd_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmadd_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmadd_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmadd_vx_i32m1_m(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmadd_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmadd_vx_i32m1_mu(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmadd_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmadd_vx_i32m1_tum(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmadd_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmadd_vx_i32m1_tumu(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmadd_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsac_vv_i32m1_m(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vnmsac_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsac_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsac_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsac_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsac_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsac_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsac_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsac_vx_i32m1_m(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vnmsac_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsac_vx_i32m1_mu(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsac_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsac_vx_i32m1_tum(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsac_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsac_vx_i32m1_tumu(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsac_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsub_vv_i32m1_m(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vnmsub_vv_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsub_vv_i32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsub_vv_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsub_vv_i32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsub_vv_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsub_vv_i32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsub_vv_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsub_vx_i32m1_m(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vnmsub_vx_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsub_vx_i32m1_mu(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsub_vx_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsub_vx_i32m1_tum(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsub_vx_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vo = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnmsub_vx_i32m1_tumu(vm, vo, 3, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnmsub_vx_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vneg_v_i32m1_m(vm, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vneg_v_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vneg_v_u32m1_m(vm, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vneg_v_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vo = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vneg_v_i32m1_mu(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vneg_v_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vo = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vneg_v_u32m1_mu(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vneg_v_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vo = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vneg_v_i32m1_tum(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vneg_v_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vo = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vneg_v_u32m1_tum(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vneg_v_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vo = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vneg_v_i32m1_tumu(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vneg_v_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vo = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vneg_v_u32m1_tumu(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vneg_v_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnot_v_i32m1_m(vm, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vnot_v_i32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vnot_v_u32m1_m(vm, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vnot_v_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vo = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnot_v_i32m1_mu(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnot_v_i32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vo = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vnot_v_u32m1_mu(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnot_v_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vo = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnot_v_i32m1_tum(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnot_v_i32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vo = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vnot_v_u32m1_tum(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnot_v_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vo = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vnot_v_i32m1_tumu(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnot_v_i32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vo = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vnot_v_u32m1_tumu(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vnot_v_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vdivu_vv_u32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vdivu_vv_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vdivu_vv_u32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdivu_vv_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vdivu_vv_u32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdivu_vv_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vdivu_vv_u32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdivu_vv_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vdivu_vx_u32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vdivu_vx_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vdivu_vx_u32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdivu_vx_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vdivu_vx_u32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdivu_vx_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vdivu_vx_u32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vdivu_vx_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vminu_vv_u32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vminu_vv_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vminu_vv_u32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vminu_vv_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vminu_vv_u32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vminu_vv_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vminu_vv_u32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vminu_vv_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vminu_vx_u32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vminu_vx_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vminu_vx_u32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vminu_vx_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vminu_vx_u32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vminu_vx_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vminu_vx_u32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vminu_vx_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmaxu_vv_u32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmaxu_vv_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmaxu_vv_u32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmaxu_vv_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmaxu_vv_u32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmaxu_vv_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmaxu_vv_u32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmaxu_vv_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmaxu_vx_u32m1_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vmaxu_vx_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmaxu_vx_u32m1_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmaxu_vx_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmaxu_vx_u32m1_tum(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmaxu_vx_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmaxu_vx_u32m1_tumu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmaxu_vx_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsrl_vv_u32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vsrl_vv_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsrl_vv_u32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsrl_vv_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsrl_vv_u32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsrl_vv_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsrl_vv_u32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsrl_vv_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsrl_vx_u32m1_m(vm, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)so32 + i * 4, 0xEE, 4);
        pb("__riscv_vsrl_vx_u32m1_m", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsrl_vx_u32m1_mu(vm, vo, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsrl_vx_u32m1_mu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsrl_vx_u32m1_tum(vm, vo, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsrl_vx_u32m1_tum", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vo = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsrl_vx_u32m1_tumu(vm, vo, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsrl_vx_u32m1_tumu", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmseq_vv_u32m1_b32_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmseq_vv_u32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmseq_vx_u32m1_b32_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmseq_vx_u32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsltu_vv_u32m1_b32_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsltu_vv_u32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsltu_vv_u32m1_b32_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsltu_vv_u32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsltu_vx_u32m1_b32_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsltu_vx_u32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsltu_vx_u32m1_b32_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsltu_vx_u32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsleu_vv_u32m1_b32_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsleu_vv_u32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsleu_vv_u32m1_b32_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsleu_vv_u32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsleu_vx_u32m1_b32_m(vm, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsleu_vx_u32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmsleu_vx_u32m1_b32_mu(vm, vo, va, 3, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmsleu_vx_u32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfadd_vv_f32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfadd_vv_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfadd_vv_f32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfadd_vv_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfadd_vv_f32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfadd_vv_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfadd_vv_f32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfadd_vv_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfadd_vf_f32m1_m(vm, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfadd_vf_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfadd_vf_f32m1_mu(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfadd_vf_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfadd_vf_f32m1_tum(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfadd_vf_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfadd_vf_f32m1_tumu(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfadd_vf_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsub_vv_f32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfsub_vv_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsub_vv_f32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsub_vv_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsub_vv_f32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsub_vv_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsub_vv_f32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsub_vv_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsub_vf_f32m1_m(vm, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfsub_vf_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsub_vf_f32m1_mu(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsub_vf_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsub_vf_f32m1_tum(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsub_vf_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfsub_vf_f32m1_tumu(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfsub_vf_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmul_vv_f32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfmul_vv_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmul_vv_f32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmul_vv_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmul_vv_f32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmul_vv_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmul_vv_f32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmul_vv_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmul_vf_f32m1_m(vm, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfmul_vf_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmul_vf_f32m1_mu(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmul_vf_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmul_vf_f32m1_tum(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmul_vf_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmul_vf_f32m1_tumu(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmul_vf_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfdiv_vv_f32m1_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfdiv_vv_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfdiv_vv_f32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfdiv_vv_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfdiv_vv_f32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfdiv_vv_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfdiv_vv_f32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfdiv_vv_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfdiv_vf_f32m1_m(vm, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfdiv_vf_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfdiv_vf_f32m1_mu(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfdiv_vf_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfdiv_vf_f32m1_tum(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfdiv_vf_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfdiv_vf_f32m1_tumu(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfdiv_vf_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfeq_vv_f32m1_b32_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfeq_vv_f32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfeq_vv_f32m1_b32_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfeq_vv_f32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfeq_vf_f32m1_b32_m(vm, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfeq_vf_f32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfeq_vf_f32m1_b32_mu(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfeq_vf_f32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfne_vv_f32m1_b32_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfne_vv_f32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfne_vv_f32m1_b32_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfne_vv_f32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfne_vf_f32m1_b32_m(vm, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfne_vf_f32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfne_vf_f32m1_b32_mu(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfne_vf_f32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmflt_vv_f32m1_b32_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmflt_vv_f32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmflt_vv_f32m1_b32_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmflt_vv_f32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmflt_vf_f32m1_b32_m(vm, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmflt_vf_f32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmflt_vf_f32m1_b32_mu(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmflt_vf_f32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfle_vv_f32m1_b32_m(vm, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfle_vv_f32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfle_vv_f32m1_b32_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfle_vv_f32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfle_vf_f32m1_b32_m(vm, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) obuf[i >> 3] |= (unsigned char)(1u << (i & 7));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfle_vf_f32m1_b32_m", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vo = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t r = __riscv_vmfle_vf_f32m1_b32_mu(vm, vo, va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
if ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8) obuf[((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 1 * 8 / (1 * 32)) % 8)) - 1);
        pb("__riscv_vmfle_vf_f32m1_b32_mu", (unsigned char *)obuf, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) + 7) / 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmacc_vv_f32m1_m(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfmacc_vv_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmacc_vv_f32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmacc_vv_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmacc_vv_f32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmacc_vv_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmacc_vv_f32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmacc_vv_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmacc_vf_f32m1_m(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfmacc_vf_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmacc_vf_f32m1_mu(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmacc_vf_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmacc_vf_f32m1_tum(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmacc_vf_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmacc_vf_f32m1_tumu(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmacc_vf_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsac_vv_f32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsac_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfmsac_vv_f64m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfmsac_vv_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsac_vf_f32m1(vd, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsac_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfmsac_vf_f64m1(vd, 0.5, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfmsac_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsac_vv_f32m1_m(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfmsac_vv_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsac_vv_f32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsac_vv_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsac_vv_f32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsac_vv_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsac_vv_f32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsac_vv_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsac_vf_f32m1_m(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfmsac_vf_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsac_vf_f32m1_mu(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsac_vf_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsac_vf_f32m1_tum(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsac_vf_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsac_vf_f32m1_tumu(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsac_vf_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsub_vv_f32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsub_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfmsub_vv_f64m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfmsub_vv_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsub_vf_f32m1(vd, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsub_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfmsub_vf_f64m1(vd, 0.5, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfmsub_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsub_vv_f32m1_m(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfmsub_vv_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsub_vv_f32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsub_vv_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsub_vv_f32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsub_vv_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsub_vv_f32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsub_vv_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsub_vf_f32m1_m(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfmsub_vf_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsub_vf_f32m1_mu(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsub_vf_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsub_vf_f32m1_tum(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsub_vf_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmsub_vf_f32m1_tumu(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmsub_vf_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmacc_vv_f32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmacc_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfnmacc_vv_f64m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfnmacc_vv_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmacc_vf_f32m1(vd, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmacc_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfnmacc_vf_f64m1(vd, 0.5, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfnmacc_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmacc_vv_f32m1_m(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfnmacc_vv_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmacc_vv_f32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmacc_vv_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmacc_vv_f32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmacc_vv_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmacc_vv_f32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmacc_vv_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmacc_vf_f32m1_m(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfnmacc_vf_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmacc_vf_f32m1_mu(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmacc_vf_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmacc_vf_f32m1_tum(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmacc_vf_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmacc_vf_f32m1_tumu(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmacc_vf_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmadd_vv_f32m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmadd_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfnmadd_vv_f64m1(vd, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfnmadd_vv_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmadd_vf_f32m1(vd, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmadd_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t vb = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfnmadd_vf_f64m1(vd, 0.5, vb, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfnmadd_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmadd_vv_f32m1_m(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfnmadd_vv_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmadd_vv_f32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmadd_vv_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmadd_vv_f32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmadd_vv_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmadd_vv_f32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmadd_vv_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmadd_vf_f32m1_m(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfnmadd_vf_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmadd_vf_f32m1_mu(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmadd_vf_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmadd_vf_f32m1_tum(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmadd_vf_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfnmadd_vf_f32m1_tumu(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfnmadd_vf_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmadd_vv_f32m1_m(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfmadd_vv_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmadd_vv_f32m1_mu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmadd_vv_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmadd_vv_f32m1_tum(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmadd_vv_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmadd_vv_f32m1_tumu(vm, vo, va, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmadd_vv_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmadd_vf_f32m1_m(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfmadd_vf_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmadd_vf_f32m1_mu(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmadd_vf_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmadd_vf_f32m1_tum(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmadd_vf_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfmadd_vf_f32m1_tumu(vm, vo, 0.5f, vb, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmadd_vf_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfabs_v_f32m1_m(vm, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfabs_v_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfabs_v_f32m1_mu(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfabs_v_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfabs_v_f32m1_tum(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfabs_v_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfabs_v_f32m1_tumu(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfabs_v_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfneg_v_f32m1_m(vm, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
for (i = 0; i < (__riscv_vlenb() * 1 * 8 / (1 * 32)); i++) if (!((mbuf[i >> 3] >> (i & 7)) & 1)) memset((unsigned char *)sof + i * 4, 0xEE, 4);
        pb("__riscv_vfneg_v_f32m1_m", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfneg_v_f32m1_mu(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfneg_v_f32m1_mu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfneg_v_f32m1_tum(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfneg_v_f32m1_tum", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vo = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfneg_v_f32m1_tumu(vm, vo, va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfneg_v_f32m1_tumu", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vmerge_vvm_f32m1(va, vb, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmerge_vvm_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vmerge_vvm_i32m1(va, vb, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmerge_vvm_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vmerge_vvm_u32m1(va, vb, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmerge_vvm_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t r = __riscv_vmerge_vxm_i32m1(va, 5, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmerge_vxm_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t r = __riscv_vmerge_vxm_u32m1(va, 5, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmerge_vxm_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vadc_vvm_i32m1(va, vb, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadc_vvm_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vadc_vvm_u32m1(va, vb, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadc_vvm_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t vb = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vsbc_vvm_i32m1(va, vb, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsbc_vvm_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t vb = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vsbc_vvm_u32m1(va, vb, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsbc_vvm_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        int np = (int)__riscv_vcpop_m_b32(vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vcompress_vm_f32m1(va, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vcompress_vm_f32m1", (unsigned char *)sof, (np * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        int np = (int)__riscv_vcpop_m_b32(vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vcompress_vm_i32m1(va, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vcompress_vm_i32m1", (unsigned char *)so32, (np * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        int np = (int)__riscv_vcpop_m_b32(vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vcompress_vm_u32m1(va, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vcompress_vm_u32m1", (unsigned char *)so32, (np * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t r = __riscv_vadc_vxm_i32m1(va, 5, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadc_vxm_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t r = __riscv_vadc_vxm_u32m1(va, 5, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vadc_vxm_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vint32m1_t r = __riscv_vsbc_vxm_i32m1(va, 5, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsbc_vxm_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t r = __riscv_vsbc_vxm_u32m1(va, 5, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsbc_vxm_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vbool32_t vm = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vfloat32m1_t r = __riscv_vfmerge_vfm_f32m1(va, 0.5f, vm, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmerge_vfm_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t r = __riscv_vmv_s_x_i32m1(7, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmv_s_x_i32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vuint32m1_t r = __riscv_vmv_s_x_u32m1(7, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vmv_s_x_u32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vfloat32m1_t r = __riscv_vfmv_s_f_f32m1(0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfmv_s_f_f32m1", (unsigned char *)sof, 4);
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t vb = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t r = __riscv_vmand_mm_b8(va, vb, (__riscv_vlenb() * 8 / 8));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 8 / 8));
if ((__riscv_vlenb() * 8 / 8) % 8) obuf[((__riscv_vlenb() * 8 / 8) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 8) % 8)) - 1);
        pb("__riscv_vmand_mm_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 8) + 7) / 8));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t vb = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t r = __riscv_vmnand_mm_b8(va, vb, (__riscv_vlenb() * 8 / 8));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 8 / 8));
if ((__riscv_vlenb() * 8 / 8) % 8) obuf[((__riscv_vlenb() * 8 / 8) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 8) % 8)) - 1);
        pb("__riscv_vmnand_mm_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 8) + 7) / 8));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t vb = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t r = __riscv_vmandn_mm_b8(va, vb, (__riscv_vlenb() * 8 / 8));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 8 / 8));
if ((__riscv_vlenb() * 8 / 8) % 8) obuf[((__riscv_vlenb() * 8 / 8) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 8) % 8)) - 1);
        pb("__riscv_vmandn_mm_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 8) + 7) / 8));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t vb = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t r = __riscv_vmor_mm_b8(va, vb, (__riscv_vlenb() * 8 / 8));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 8 / 8));
if ((__riscv_vlenb() * 8 / 8) % 8) obuf[((__riscv_vlenb() * 8 / 8) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 8) % 8)) - 1);
        pb("__riscv_vmor_mm_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 8) + 7) / 8));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t vb = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t r = __riscv_vmorn_mm_b8(va, vb, (__riscv_vlenb() * 8 / 8));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 8 / 8));
if ((__riscv_vlenb() * 8 / 8) % 8) obuf[((__riscv_vlenb() * 8 / 8) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 8) % 8)) - 1);
        pb("__riscv_vmorn_mm_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 8) + 7) / 8));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t vb = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t r = __riscv_vmxor_mm_b8(va, vb, (__riscv_vlenb() * 8 / 8));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 8 / 8));
if ((__riscv_vlenb() * 8 / 8) % 8) obuf[((__riscv_vlenb() * 8 / 8) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 8) % 8)) - 1);
        pb("__riscv_vmxor_mm_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 8) + 7) / 8));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t vb = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t r = __riscv_vmxnor_mm_b8(va, vb, (__riscv_vlenb() * 8 / 8));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 8 / 8));
if ((__riscv_vlenb() * 8 / 8) % 8) obuf[((__riscv_vlenb() * 8 / 8) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 8) % 8)) - 1);
        pb("__riscv_vmxnor_mm_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 8) + 7) / 8));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t r = __riscv_vmnot_m_b8(va, (__riscv_vlenb() * 8 / 8));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 8 / 8));
if ((__riscv_vlenb() * 8 / 8) % 8) obuf[((__riscv_vlenb() * 8 / 8) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 8) % 8)) - 1);
        pb("__riscv_vmnot_m_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 8) + 7) / 8));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t r = __riscv_vmsbf_m_b8(va, (__riscv_vlenb() * 8 / 8));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 8 / 8));
if ((__riscv_vlenb() * 8 / 8) % 8) obuf[((__riscv_vlenb() * 8 / 8) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 8) % 8)) - 1);
        pb("__riscv_vmsbf_m_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 8) + 7) / 8));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t r = __riscv_vmsif_m_b8(va, (__riscv_vlenb() * 8 / 8));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 8 / 8));
if ((__riscv_vlenb() * 8 / 8) % 8) obuf[((__riscv_vlenb() * 8 / 8) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 8) % 8)) - 1);
        pb("__riscv_vmsif_m_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 8) + 7) / 8));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t r = __riscv_vmsof_m_b8(va, (__riscv_vlenb() * 8 / 8));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 8 / 8));
if ((__riscv_vlenb() * 8 / 8) % 8) obuf[((__riscv_vlenb() * 8 / 8) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 8) % 8)) - 1);
        pb("__riscv_vmsof_m_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 8) + 7) / 8));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        printf("__riscv_vcpop_m_b8 %lld\n", (long long)__riscv_vcpop_m_b8(va, (__riscv_vlenb() * 8 / 8)));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        printf("__riscv_vfirst_m_b8 %lld\n", (long long)__riscv_vfirst_m_b8(va, (__riscv_vlenb() * 8 / 8)));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t vb = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t r = __riscv_vmand_mm_b16(va, vb, (__riscv_vlenb() * 8 / 16));
__riscv_vsm_v_b16(obuf, r, (__riscv_vlenb() * 8 / 16));
if ((__riscv_vlenb() * 8 / 16) % 8) obuf[((__riscv_vlenb() * 8 / 16) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 16) % 8)) - 1);
        pb("__riscv_vmand_mm_b16", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 16) + 7) / 8));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t vb = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t r = __riscv_vmnand_mm_b16(va, vb, (__riscv_vlenb() * 8 / 16));
__riscv_vsm_v_b16(obuf, r, (__riscv_vlenb() * 8 / 16));
if ((__riscv_vlenb() * 8 / 16) % 8) obuf[((__riscv_vlenb() * 8 / 16) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 16) % 8)) - 1);
        pb("__riscv_vmnand_mm_b16", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 16) + 7) / 8));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t vb = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t r = __riscv_vmandn_mm_b16(va, vb, (__riscv_vlenb() * 8 / 16));
__riscv_vsm_v_b16(obuf, r, (__riscv_vlenb() * 8 / 16));
if ((__riscv_vlenb() * 8 / 16) % 8) obuf[((__riscv_vlenb() * 8 / 16) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 16) % 8)) - 1);
        pb("__riscv_vmandn_mm_b16", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 16) + 7) / 8));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t vb = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t r = __riscv_vmor_mm_b16(va, vb, (__riscv_vlenb() * 8 / 16));
__riscv_vsm_v_b16(obuf, r, (__riscv_vlenb() * 8 / 16));
if ((__riscv_vlenb() * 8 / 16) % 8) obuf[((__riscv_vlenb() * 8 / 16) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 16) % 8)) - 1);
        pb("__riscv_vmor_mm_b16", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 16) + 7) / 8));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t vb = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t r = __riscv_vmorn_mm_b16(va, vb, (__riscv_vlenb() * 8 / 16));
__riscv_vsm_v_b16(obuf, r, (__riscv_vlenb() * 8 / 16));
if ((__riscv_vlenb() * 8 / 16) % 8) obuf[((__riscv_vlenb() * 8 / 16) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 16) % 8)) - 1);
        pb("__riscv_vmorn_mm_b16", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 16) + 7) / 8));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t vb = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t r = __riscv_vmxor_mm_b16(va, vb, (__riscv_vlenb() * 8 / 16));
__riscv_vsm_v_b16(obuf, r, (__riscv_vlenb() * 8 / 16));
if ((__riscv_vlenb() * 8 / 16) % 8) obuf[((__riscv_vlenb() * 8 / 16) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 16) % 8)) - 1);
        pb("__riscv_vmxor_mm_b16", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 16) + 7) / 8));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t vb = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t r = __riscv_vmxnor_mm_b16(va, vb, (__riscv_vlenb() * 8 / 16));
__riscv_vsm_v_b16(obuf, r, (__riscv_vlenb() * 8 / 16));
if ((__riscv_vlenb() * 8 / 16) % 8) obuf[((__riscv_vlenb() * 8 / 16) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 16) % 8)) - 1);
        pb("__riscv_vmxnor_mm_b16", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 16) + 7) / 8));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t r = __riscv_vmnot_m_b16(va, (__riscv_vlenb() * 8 / 16));
__riscv_vsm_v_b16(obuf, r, (__riscv_vlenb() * 8 / 16));
if ((__riscv_vlenb() * 8 / 16) % 8) obuf[((__riscv_vlenb() * 8 / 16) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 16) % 8)) - 1);
        pb("__riscv_vmnot_m_b16", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 16) + 7) / 8));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t r = __riscv_vmsbf_m_b16(va, (__riscv_vlenb() * 8 / 16));
__riscv_vsm_v_b16(obuf, r, (__riscv_vlenb() * 8 / 16));
if ((__riscv_vlenb() * 8 / 16) % 8) obuf[((__riscv_vlenb() * 8 / 16) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 16) % 8)) - 1);
        pb("__riscv_vmsbf_m_b16", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 16) + 7) / 8));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t r = __riscv_vmsif_m_b16(va, (__riscv_vlenb() * 8 / 16));
__riscv_vsm_v_b16(obuf, r, (__riscv_vlenb() * 8 / 16));
if ((__riscv_vlenb() * 8 / 16) % 8) obuf[((__riscv_vlenb() * 8 / 16) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 16) % 8)) - 1);
        pb("__riscv_vmsif_m_b16", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 16) + 7) / 8));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        vbool16_t r = __riscv_vmsof_m_b16(va, (__riscv_vlenb() * 8 / 16));
__riscv_vsm_v_b16(obuf, r, (__riscv_vlenb() * 8 / 16));
if ((__riscv_vlenb() * 8 / 16) % 8) obuf[((__riscv_vlenb() * 8 / 16) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 16) % 8)) - 1);
        pb("__riscv_vmsof_m_b16", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 16) + 7) / 8));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        printf("__riscv_vcpop_m_b16 %lld\n", (long long)__riscv_vcpop_m_b16(va, (__riscv_vlenb() * 8 / 16)));
    }
    {
        int i;
        vbool16_t va = __riscv_vlm_v_b16(mbuf, (__riscv_vlenb() * 8 / 16));
        printf("__riscv_vfirst_m_b16 %lld\n", (long long)__riscv_vfirst_m_b16(va, (__riscv_vlenb() * 8 / 16)));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vb = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t r = __riscv_vmand_mm_b32(va, vb, (__riscv_vlenb() * 8 / 32));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 8 / 32));
if ((__riscv_vlenb() * 8 / 32) % 8) obuf[((__riscv_vlenb() * 8 / 32) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 32) % 8)) - 1);
        pb("__riscv_vmand_mm_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 32) + 7) / 8));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vb = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t r = __riscv_vmnand_mm_b32(va, vb, (__riscv_vlenb() * 8 / 32));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 8 / 32));
if ((__riscv_vlenb() * 8 / 32) % 8) obuf[((__riscv_vlenb() * 8 / 32) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 32) % 8)) - 1);
        pb("__riscv_vmnand_mm_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 32) + 7) / 8));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vb = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t r = __riscv_vmandn_mm_b32(va, vb, (__riscv_vlenb() * 8 / 32));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 8 / 32));
if ((__riscv_vlenb() * 8 / 32) % 8) obuf[((__riscv_vlenb() * 8 / 32) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 32) % 8)) - 1);
        pb("__riscv_vmandn_mm_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 32) + 7) / 8));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vb = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t r = __riscv_vmor_mm_b32(va, vb, (__riscv_vlenb() * 8 / 32));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 8 / 32));
if ((__riscv_vlenb() * 8 / 32) % 8) obuf[((__riscv_vlenb() * 8 / 32) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 32) % 8)) - 1);
        pb("__riscv_vmor_mm_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 32) + 7) / 8));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vb = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t r = __riscv_vmorn_mm_b32(va, vb, (__riscv_vlenb() * 8 / 32));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 8 / 32));
if ((__riscv_vlenb() * 8 / 32) % 8) obuf[((__riscv_vlenb() * 8 / 32) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 32) % 8)) - 1);
        pb("__riscv_vmorn_mm_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 32) + 7) / 8));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vb = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t r = __riscv_vmxor_mm_b32(va, vb, (__riscv_vlenb() * 8 / 32));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 8 / 32));
if ((__riscv_vlenb() * 8 / 32) % 8) obuf[((__riscv_vlenb() * 8 / 32) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 32) % 8)) - 1);
        pb("__riscv_vmxor_mm_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 32) + 7) / 8));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t vb = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t r = __riscv_vmxnor_mm_b32(va, vb, (__riscv_vlenb() * 8 / 32));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 8 / 32));
if ((__riscv_vlenb() * 8 / 32) % 8) obuf[((__riscv_vlenb() * 8 / 32) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 32) % 8)) - 1);
        pb("__riscv_vmxnor_mm_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 32) + 7) / 8));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t r = __riscv_vmnot_m_b32(va, (__riscv_vlenb() * 8 / 32));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 8 / 32));
if ((__riscv_vlenb() * 8 / 32) % 8) obuf[((__riscv_vlenb() * 8 / 32) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 32) % 8)) - 1);
        pb("__riscv_vmnot_m_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 32) + 7) / 8));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t r = __riscv_vmsbf_m_b32(va, (__riscv_vlenb() * 8 / 32));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 8 / 32));
if ((__riscv_vlenb() * 8 / 32) % 8) obuf[((__riscv_vlenb() * 8 / 32) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 32) % 8)) - 1);
        pb("__riscv_vmsbf_m_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 32) + 7) / 8));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t r = __riscv_vmsif_m_b32(va, (__riscv_vlenb() * 8 / 32));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 8 / 32));
if ((__riscv_vlenb() * 8 / 32) % 8) obuf[((__riscv_vlenb() * 8 / 32) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 32) % 8)) - 1);
        pb("__riscv_vmsif_m_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 32) + 7) / 8));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vbool32_t r = __riscv_vmsof_m_b32(va, (__riscv_vlenb() * 8 / 32));
__riscv_vsm_v_b32(obuf, r, (__riscv_vlenb() * 8 / 32));
if ((__riscv_vlenb() * 8 / 32) % 8) obuf[((__riscv_vlenb() * 8 / 32) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 32) % 8)) - 1);
        pb("__riscv_vmsof_m_b32", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 32) + 7) / 8));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        printf("__riscv_vcpop_m_b32 %lld\n", (long long)__riscv_vcpop_m_b32(va, (__riscv_vlenb() * 8 / 32)));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        printf("__riscv_vfirst_m_b32 %lld\n", (long long)__riscv_vfirst_m_b32(va, (__riscv_vlenb() * 8 / 32)));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t vb = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t r = __riscv_vmand_mm_b64(va, vb, (__riscv_vlenb() * 8 / 64));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 8 / 64));
if ((__riscv_vlenb() * 8 / 64) % 8) obuf[((__riscv_vlenb() * 8 / 64) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 64) % 8)) - 1);
        pb("__riscv_vmand_mm_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 64) + 7) / 8));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t vb = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t r = __riscv_vmnand_mm_b64(va, vb, (__riscv_vlenb() * 8 / 64));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 8 / 64));
if ((__riscv_vlenb() * 8 / 64) % 8) obuf[((__riscv_vlenb() * 8 / 64) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 64) % 8)) - 1);
        pb("__riscv_vmnand_mm_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 64) + 7) / 8));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t vb = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t r = __riscv_vmandn_mm_b64(va, vb, (__riscv_vlenb() * 8 / 64));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 8 / 64));
if ((__riscv_vlenb() * 8 / 64) % 8) obuf[((__riscv_vlenb() * 8 / 64) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 64) % 8)) - 1);
        pb("__riscv_vmandn_mm_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 64) + 7) / 8));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t vb = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t r = __riscv_vmor_mm_b64(va, vb, (__riscv_vlenb() * 8 / 64));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 8 / 64));
if ((__riscv_vlenb() * 8 / 64) % 8) obuf[((__riscv_vlenb() * 8 / 64) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 64) % 8)) - 1);
        pb("__riscv_vmor_mm_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 64) + 7) / 8));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t vb = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t r = __riscv_vmorn_mm_b64(va, vb, (__riscv_vlenb() * 8 / 64));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 8 / 64));
if ((__riscv_vlenb() * 8 / 64) % 8) obuf[((__riscv_vlenb() * 8 / 64) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 64) % 8)) - 1);
        pb("__riscv_vmorn_mm_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 64) + 7) / 8));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t vb = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t r = __riscv_vmxor_mm_b64(va, vb, (__riscv_vlenb() * 8 / 64));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 8 / 64));
if ((__riscv_vlenb() * 8 / 64) % 8) obuf[((__riscv_vlenb() * 8 / 64) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 64) % 8)) - 1);
        pb("__riscv_vmxor_mm_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 64) + 7) / 8));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t vb = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t r = __riscv_vmxnor_mm_b64(va, vb, (__riscv_vlenb() * 8 / 64));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 8 / 64));
if ((__riscv_vlenb() * 8 / 64) % 8) obuf[((__riscv_vlenb() * 8 / 64) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 64) % 8)) - 1);
        pb("__riscv_vmxnor_mm_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 64) + 7) / 8));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t r = __riscv_vmnot_m_b64(va, (__riscv_vlenb() * 8 / 64));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 8 / 64));
if ((__riscv_vlenb() * 8 / 64) % 8) obuf[((__riscv_vlenb() * 8 / 64) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 64) % 8)) - 1);
        pb("__riscv_vmnot_m_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 64) + 7) / 8));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t r = __riscv_vmsbf_m_b64(va, (__riscv_vlenb() * 8 / 64));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 8 / 64));
if ((__riscv_vlenb() * 8 / 64) % 8) obuf[((__riscv_vlenb() * 8 / 64) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 64) % 8)) - 1);
        pb("__riscv_vmsbf_m_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 64) + 7) / 8));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t r = __riscv_vmsif_m_b64(va, (__riscv_vlenb() * 8 / 64));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 8 / 64));
if ((__riscv_vlenb() * 8 / 64) % 8) obuf[((__riscv_vlenb() * 8 / 64) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 64) % 8)) - 1);
        pb("__riscv_vmsif_m_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 64) + 7) / 8));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        vbool64_t r = __riscv_vmsof_m_b64(va, (__riscv_vlenb() * 8 / 64));
__riscv_vsm_v_b64(obuf, r, (__riscv_vlenb() * 8 / 64));
if ((__riscv_vlenb() * 8 / 64) % 8) obuf[((__riscv_vlenb() * 8 / 64) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 64) % 8)) - 1);
        pb("__riscv_vmsof_m_b64", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 64) + 7) / 8));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        printf("__riscv_vcpop_m_b64 %lld\n", (long long)__riscv_vcpop_m_b64(va, (__riscv_vlenb() * 8 / 64)));
    }
    {
        int i;
        vbool64_t va = __riscv_vlm_v_b64(mbuf, (__riscv_vlenb() * 8 / 64));
        printf("__riscv_vfirst_m_b64 %lld\n", (long long)__riscv_vfirst_m_b64(va, (__riscv_vlenb() * 8 / 64)));
    }
    {
        int i;
        vbool1_t va = __riscv_vlm_v_b1(mbuf, (__riscv_vlenb() * 8 / 1));
        printf("__riscv_vcpop_m_b1 %lld\n", (long long)__riscv_vcpop_m_b1(va, (__riscv_vlenb() * 8 / 1)));
    }
    {
        int i;
        vbool1_t va = __riscv_vlm_v_b1(mbuf, (__riscv_vlenb() * 8 / 1));
        printf("__riscv_vfirst_m_b1 %lld\n", (long long)__riscv_vfirst_m_b1(va, (__riscv_vlenb() * 8 / 1)));
    }
    {
        int i;
        vbool2_t va = __riscv_vlm_v_b2(mbuf, (__riscv_vlenb() * 8 / 2));
        printf("__riscv_vcpop_m_b2 %lld\n", (long long)__riscv_vcpop_m_b2(va, (__riscv_vlenb() * 8 / 2)));
    }
    {
        int i;
        vbool2_t va = __riscv_vlm_v_b2(mbuf, (__riscv_vlenb() * 8 / 2));
        printf("__riscv_vfirst_m_b2 %lld\n", (long long)__riscv_vfirst_m_b2(va, (__riscv_vlenb() * 8 / 2)));
    }
    {
        int i;
        vbool4_t va = __riscv_vlm_v_b4(mbuf, (__riscv_vlenb() * 8 / 4));
        printf("__riscv_vcpop_m_b4 %lld\n", (long long)__riscv_vcpop_m_b4(va, (__riscv_vlenb() * 8 / 4)));
    }
    {
        int i;
        vbool4_t va = __riscv_vlm_v_b4(mbuf, (__riscv_vlenb() * 8 / 4));
        printf("__riscv_vfirst_m_b4 %lld\n", (long long)__riscv_vfirst_m_b4(va, (__riscv_vlenb() * 8 / 4)));
    }
    {
        int i;
        vbool32_t va = __riscv_vlm_v_b32(mbuf, (__riscv_vlenb() * 8 / 32));
        vuint32m1_t r = __riscv_viota_m_u32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_viota_m_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint16m1_t r = __riscv_vid_v_u16m1((__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_u16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vid_v_u16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint32m1_t r = __riscv_vid_v_u32m1((__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vid_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint8m1_t r = __riscv_vid_v_u8m1((__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_u8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vid_v_u8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vfloat32m1_t r = __riscv_vlse32_v_f32m1(buff, 4, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vlse32_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint8m1_t r = __riscv_vlse8_v_i8m1(buf8, 1, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vlse8_v_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint8m1_t r = __riscv_vlse8_v_u8m1(buf8, 1, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_u8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vlse8_v_u8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vfloat32m1_t vv = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        __riscv_vsse32_v_f32m1(sbuf, 4, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsse32_v_f32m1", sbuf, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint8m1_t vv = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        memset(sbuf, 0, sizeof sbuf);
        __riscv_vsse8_v_i8m1(sbuf, 1, vv, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vsse8_v_i8m1", sbuf, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint8m1_t vv = __riscv_vle8_v_u8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        memset(sbuf, 0, sizeof sbuf);
        __riscv_vsse8_v_u8m1(sbuf, 1, vv, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vsse8_v_u8m1", sbuf, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vluxei16_v_f32m1(buff, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vluxei16_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint16m1_t vi = __riscv_vle16_v_u16m1(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vluxei16_v_i16m1(buf16, vi, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vluxei16_v_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vluxei16_v_i32m1(buf32, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vluxei16_v_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint16m1_t vi = __riscv_vle16_v_u16m1(xb16, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t r = __riscv_vluxei16_v_i32m2(buf32, vi, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vluxei16_v_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint16m2_t vi = __riscv_vle16_v_u16m2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vluxei16_v_i8m1(buf8, vi, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vluxei16_v_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vluxei16_v_u32m1(buf32, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vluxei16_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vloxei16_v_f32m1(buff, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vloxei16_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint16m1_t vi = __riscv_vle16_v_u16m1(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vloxei16_v_i16m1(buf16, vi, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vloxei16_v_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vloxei16_v_i32m1(buf32, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vloxei16_v_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint16m1_t vi = __riscv_vle16_v_u16m1(xb16, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t r = __riscv_vloxei16_v_i32m2(buf32, vi, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vloxei16_v_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint16m2_t vi = __riscv_vle16_v_u16m2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vloxei16_v_i8m1(buf8, vi, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vloxei16_v_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vloxei16_v_u32m1(buf32, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vloxei16_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t vv = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsuxei16_v_f32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsuxei16_v_f32m1", sbuf, 256);
    }
    {
        int i;
        vint16m1_t vv = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        memset(sbuf, 0, sizeof sbuf);
        vuint16m1_t vi = __riscv_vle16_v_u16m1(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        __riscv_vsuxei16_v_i16m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vsuxei16_v_i16m1", sbuf, 256);
    }
    {
        int i;
        vint32m1_t vv = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsuxei16_v_i32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsuxei16_v_i32m1", sbuf, 256);
    }
    {
        int i;
        vint32m2_t vv = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint16m1_t vi = __riscv_vle16_v_u16m1(xb16, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        __riscv_vsuxei16_v_i32m2(sbuf, vi, vv, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vsuxei16_v_i32m2", sbuf, 256);
    }
    {
        int i;
        vint8m1_t vv = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        memset(sbuf, 0, sizeof sbuf);
        vuint16m2_t vi = __riscv_vle16_v_u16m2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        __riscv_vsuxei16_v_i8m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vsuxei16_v_i8m1", sbuf, 256);
    }
    {
        int i;
        vuint32m1_t vv = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsuxei16_v_u32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsuxei16_v_u32m1", sbuf, 256);
    }
    {
        int i;
        vfloat32m1_t vv = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsoxei16_v_f32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsoxei16_v_f32m1", sbuf, 256);
    }
    {
        int i;
        vint16m1_t vv = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        memset(sbuf, 0, sizeof sbuf);
        vuint16m1_t vi = __riscv_vle16_v_u16m1(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        __riscv_vsoxei16_v_i16m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vsoxei16_v_i16m1", sbuf, 256);
    }
    {
        int i;
        vint32m1_t vv = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsoxei16_v_i32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsoxei16_v_i32m1", sbuf, 256);
    }
    {
        int i;
        vint32m2_t vv = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint16m1_t vi = __riscv_vle16_v_u16m1(xb16, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        __riscv_vsoxei16_v_i32m2(sbuf, vi, vv, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vsoxei16_v_i32m2", sbuf, 256);
    }
    {
        int i;
        vint8m1_t vv = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        memset(sbuf, 0, sizeof sbuf);
        vuint16m2_t vi = __riscv_vle16_v_u16m2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        __riscv_vsoxei16_v_i8m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vsoxei16_v_i8m1", sbuf, 256);
    }
    {
        int i;
        vuint32m1_t vv = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(xb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsoxei16_v_u32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsoxei16_v_u32m1", sbuf, 256);
    }
    {
        int i;
        vuint32m1_t vi = __riscv_vle32_v_u32m1(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vluxei32_v_f32m1(buff, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vluxei32_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m2_t vi = __riscv_vle32_v_u32m2(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vluxei32_v_i16m1(buf16, vi, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vluxei32_v_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint32m1_t vi = __riscv_vle32_v_u32m1(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vluxei32_v_i32m1(buf32, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vluxei32_v_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m2_t vi = __riscv_vle32_v_u32m2(xb32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t r = __riscv_vluxei32_v_i32m2(buf32, vi, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vluxei32_v_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32mf2_t vi = __riscv_vle32_v_u32mf2(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vluxei32_v_i64m1(buf64, vi, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vluxei32_v_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint32m1_t vi = __riscv_vle32_v_u32m1(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vluxei32_v_u32m1(buf32, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vluxei32_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t vi = __riscv_vle32_v_u32m1(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vloxei32_v_f32m1(buff, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vloxei32_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m2_t vi = __riscv_vle32_v_u32m2(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vloxei32_v_i16m1(buf16, vi, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vloxei32_v_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint32m1_t vi = __riscv_vle32_v_u32m1(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vloxei32_v_i32m1(buf32, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vloxei32_v_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m2_t vi = __riscv_vle32_v_u32m2(xb32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t r = __riscv_vloxei32_v_i32m2(buf32, vi, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vloxei32_v_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32mf2_t vi = __riscv_vle32_v_u32mf2(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vloxei32_v_i64m1(buf64, vi, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vloxei32_v_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint32m1_t vi = __riscv_vle32_v_u32m1(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vloxei32_v_u32m1(buf32, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vloxei32_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t vv = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint32m1_t vi = __riscv_vle32_v_u32m1(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsuxei32_v_f32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsuxei32_v_f32m1", sbuf, 256);
    }
    {
        int i;
        vint16m1_t vv = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        memset(sbuf, 0, sizeof sbuf);
        vuint32m2_t vi = __riscv_vle32_v_u32m2(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        __riscv_vsuxei32_v_i16m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vsuxei32_v_i16m1", sbuf, 256);
    }
    {
        int i;
        vint32m1_t vv = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint32m1_t vi = __riscv_vle32_v_u32m1(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsuxei32_v_i32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsuxei32_v_i32m1", sbuf, 256);
    }
    {
        int i;
        vint32m2_t vv = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint32m2_t vi = __riscv_vle32_v_u32m2(xb32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        __riscv_vsuxei32_v_i32m2(sbuf, vi, vv, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vsuxei32_v_i32m2", sbuf, 256);
    }
    {
        int i;
        vint64m1_t vv = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        memset(sbuf, 0, sizeof sbuf);
        vuint32mf2_t vi = __riscv_vle32_v_u32mf2(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        __riscv_vsuxei32_v_i64m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vsuxei32_v_i64m1", sbuf, 256);
    }
    {
        int i;
        vuint32m1_t vv = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint32m1_t vi = __riscv_vle32_v_u32m1(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsuxei32_v_u32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsuxei32_v_u32m1", sbuf, 256);
    }
    {
        int i;
        vfloat32m1_t vv = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint32m1_t vi = __riscv_vle32_v_u32m1(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsoxei32_v_f32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsoxei32_v_f32m1", sbuf, 256);
    }
    {
        int i;
        vint16m1_t vv = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        memset(sbuf, 0, sizeof sbuf);
        vuint32m2_t vi = __riscv_vle32_v_u32m2(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        __riscv_vsoxei32_v_i16m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vsoxei32_v_i16m1", sbuf, 256);
    }
    {
        int i;
        vint32m1_t vv = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint32m1_t vi = __riscv_vle32_v_u32m1(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsoxei32_v_i32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsoxei32_v_i32m1", sbuf, 256);
    }
    {
        int i;
        vint32m2_t vv = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint32m2_t vi = __riscv_vle32_v_u32m2(xb32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        __riscv_vsoxei32_v_i32m2(sbuf, vi, vv, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vsoxei32_v_i32m2", sbuf, 256);
    }
    {
        int i;
        vint64m1_t vv = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        memset(sbuf, 0, sizeof sbuf);
        vuint32mf2_t vi = __riscv_vle32_v_u32mf2(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        __riscv_vsoxei32_v_i64m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vsoxei32_v_i64m1", sbuf, 256);
    }
    {
        int i;
        vuint32m1_t vv = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint32m1_t vi = __riscv_vle32_v_u32m1(xb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsoxei32_v_u32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsoxei32_v_u32m1", sbuf, 256);
    }
    {
        int i;
        vuint64m2_t vi = __riscv_vle64_v_u64m2(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vluxei64_v_f32m1(buff, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vluxei64_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m2_t vi = __riscv_vle64_v_u64m2(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vluxei64_v_i32m1(buf32, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vluxei64_v_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m1_t vi = __riscv_vle64_v_u64m1(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vluxei64_v_i64m1(buf64, vi, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vluxei64_v_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint64m2_t vi = __riscv_vle64_v_u64m2(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vluxei64_v_u32m1(buf32, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vluxei64_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m2_t vi = __riscv_vle64_v_u64m2(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vloxei64_v_f32m1(buff, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vloxei64_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m2_t vi = __riscv_vle64_v_u64m2(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vloxei64_v_i32m1(buf32, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vloxei64_v_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m1_t vi = __riscv_vle64_v_u64m1(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vloxei64_v_i64m1(buf64, vi, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vloxei64_v_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint64m2_t vi = __riscv_vle64_v_u64m2(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vloxei64_v_u32m1(buf32, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vloxei64_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t vv = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint64m2_t vi = __riscv_vle64_v_u64m2(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsuxei64_v_f32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsuxei64_v_f32m1", sbuf, 256);
    }
    {
        int i;
        vint32m1_t vv = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint64m2_t vi = __riscv_vle64_v_u64m2(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsuxei64_v_i32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsuxei64_v_i32m1", sbuf, 256);
    }
    {
        int i;
        vint64m1_t vv = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        memset(sbuf, 0, sizeof sbuf);
        vuint64m1_t vi = __riscv_vle64_v_u64m1(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        __riscv_vsuxei64_v_i64m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vsuxei64_v_i64m1", sbuf, 256);
    }
    {
        int i;
        vuint32m1_t vv = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint64m2_t vi = __riscv_vle64_v_u64m2(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsuxei64_v_u32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsuxei64_v_u32m1", sbuf, 256);
    }
    {
        int i;
        vfloat32m1_t vv = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint64m2_t vi = __riscv_vle64_v_u64m2(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsoxei64_v_f32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsoxei64_v_f32m1", sbuf, 256);
    }
    {
        int i;
        vint32m1_t vv = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint64m2_t vi = __riscv_vle64_v_u64m2(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsoxei64_v_i32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsoxei64_v_i32m1", sbuf, 256);
    }
    {
        int i;
        vint64m1_t vv = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        memset(sbuf, 0, sizeof sbuf);
        vuint64m1_t vi = __riscv_vle64_v_u64m1(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        __riscv_vsoxei64_v_i64m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vsoxei64_v_i64m1", sbuf, 256);
    }
    {
        int i;
        vuint32m1_t vv = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint64m2_t vi = __riscv_vle64_v_u64m2(xb64, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsoxei64_v_u32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsoxei64_v_u32m1", sbuf, 256);
    }
    {
        int i;
        vuint8mf4_t vi = __riscv_vle8_v_u8mf4(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vluxei8_v_f32m1(buff, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vluxei8_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint8mf2_t vi = __riscv_vle8_v_u8mf2(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vluxei8_v_i16m1(buf16, vi, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vluxei8_v_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint8mf2_t vi = __riscv_vle8_v_u8mf2(xb8, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t r = __riscv_vluxei8_v_i32m2(buf32, vi, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vluxei8_v_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint8m1_t vi = __riscv_vle8_v_u8m1(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vluxei8_v_i8m1(buf8, vi, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vluxei8_v_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint8m1_t vi = __riscv_vle8_v_u8m1(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vuint8m1_t r = __riscv_vluxei8_v_u8m1(buf8, vi, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_u8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vluxei8_v_u8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint8mf4_t vi = __riscv_vle8_v_u8mf4(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vloxei8_v_f32m1(buff, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vloxei8_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint8mf2_t vi = __riscv_vle8_v_u8mf2(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vloxei8_v_i16m1(buf16, vi, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vloxei8_v_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vuint8mf2_t vi = __riscv_vle8_v_u8mf2(xb8, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t r = __riscv_vloxei8_v_i32m2(buf32, vi, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vloxei8_v_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint8m1_t vi = __riscv_vle8_v_u8m1(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vloxei8_v_i8m1(buf8, vi, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vloxei8_v_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint8m1_t vi = __riscv_vle8_v_u8m1(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vuint8m1_t r = __riscv_vloxei8_v_u8m1(buf8, vi, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_u8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vloxei8_v_u8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vfloat32m1_t vv = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint8mf4_t vi = __riscv_vle8_v_u8mf4(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsuxei8_v_f32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsuxei8_v_f32m1", sbuf, 256);
    }
    {
        int i;
        vint16m1_t vv = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        memset(sbuf, 0, sizeof sbuf);
        vuint8mf2_t vi = __riscv_vle8_v_u8mf2(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        __riscv_vsuxei8_v_i16m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vsuxei8_v_i16m1", sbuf, 256);
    }
    {
        int i;
        vint32m2_t vv = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint8mf2_t vi = __riscv_vle8_v_u8mf2(xb8, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        __riscv_vsuxei8_v_i32m2(sbuf, vi, vv, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vsuxei8_v_i32m2", sbuf, 256);
    }
    {
        int i;
        vint8m1_t vv = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        memset(sbuf, 0, sizeof sbuf);
        vuint8m1_t vi = __riscv_vle8_v_u8m1(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        __riscv_vsuxei8_v_i8m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vsuxei8_v_i8m1", sbuf, 256);
    }
    {
        int i;
        vuint8m1_t vv = __riscv_vle8_v_u8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        memset(sbuf, 0, sizeof sbuf);
        vuint8m1_t vi = __riscv_vle8_v_u8m1(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        __riscv_vsuxei8_v_u8m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vsuxei8_v_u8m1", sbuf, 256);
    }
    {
        int i;
        vfloat32m1_t vv = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint8mf4_t vi = __riscv_vle8_v_u8mf4(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        __riscv_vsoxei8_v_f32m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vsoxei8_v_f32m1", sbuf, 256);
    }
    {
        int i;
        vint16m1_t vv = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        memset(sbuf, 0, sizeof sbuf);
        vuint8mf2_t vi = __riscv_vle8_v_u8mf2(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        __riscv_vsoxei8_v_i16m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vsoxei8_v_i16m1", sbuf, 256);
    }
    {
        int i;
        vint32m2_t vv = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        memset(sbuf, 0, sizeof sbuf);
        vuint8mf2_t vi = __riscv_vle8_v_u8mf2(xb8, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        __riscv_vsoxei8_v_i32m2(sbuf, vi, vv, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vsoxei8_v_i32m2", sbuf, 256);
    }
    {
        int i;
        vint8m1_t vv = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        memset(sbuf, 0, sizeof sbuf);
        vuint8m1_t vi = __riscv_vle8_v_u8m1(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        __riscv_vsoxei8_v_i8m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vsoxei8_v_i8m1", sbuf, 256);
    }
    {
        int i;
        vuint8m1_t vv = __riscv_vle8_v_u8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        memset(sbuf, 0, sizeof sbuf);
        vuint8m1_t vi = __riscv_vle8_v_u8m1(xb8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        __riscv_vsoxei8_v_u8m1(sbuf, vi, vv, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vsoxei8_v_u8m1", sbuf, 256);
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfrec7_v_f32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfrec7_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m2_t va = __riscv_vle32_v_f32m2(buff, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vfloat32m2_t r = __riscv_vfrec7_v_f32m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_f32m2(sof, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vfrec7_v_f32m2", (unsigned char *)sof, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32mf2_t va = __riscv_vle32_v_f32mf2(buff, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vfloat32mf2_t r = __riscv_vfrec7_v_f32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_f32mf2(sof, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfrec7_v_f32mf2", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfrec7_v_f64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfrec7_v_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfrsqrt7_v_f32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfrsqrt7_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m2_t va = __riscv_vle32_v_f32m2(buff, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vfloat32m2_t r = __riscv_vfrsqrt7_v_f32m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_f32m2(sof, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vfrsqrt7_v_f32m2", (unsigned char *)sof, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32mf2_t va = __riscv_vle32_v_f32mf2(buff, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vfloat32mf2_t r = __riscv_vfrsqrt7_v_f32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_f32mf2(sof, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfrsqrt7_v_f32mf2", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfrsqrt7_v_f64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfrsqrt7_v_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vfcvt_x_f_v_i32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfcvt_x_f_v_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m2_t va = __riscv_vle32_v_f32m2(buff, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t r = __riscv_vfcvt_x_f_v_i32m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vfcvt_x_f_v_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32mf2_t va = __riscv_vle32_v_f32mf2(buff, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint32mf2_t r = __riscv_vfcvt_x_f_v_i32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_i32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfcvt_x_f_v_i32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vfcvt_x_f_v_i64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfcvt_x_f_v_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vfcvt_xu_f_v_u32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfcvt_xu_f_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m2_t va = __riscv_vle32_v_f32m2(buff, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vuint32m2_t r = __riscv_vfcvt_xu_f_v_u32m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_u32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vfcvt_xu_f_v_u32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32mf2_t va = __riscv_vle32_v_f32mf2(buff, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint32mf2_t r = __riscv_vfcvt_xu_f_v_u32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_u32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfcvt_xu_f_v_u32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint64m1_t r = __riscv_vfcvt_xu_f_v_u64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_u64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfcvt_xu_f_v_u64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vfcvt_rtz_x_f_v_i32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfcvt_rtz_x_f_v_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m2_t va = __riscv_vle32_v_f32m2(buff, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vint32m2_t r = __riscv_vfcvt_rtz_x_f_v_i32m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_i32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vfcvt_rtz_x_f_v_i32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32mf2_t va = __riscv_vle32_v_f32mf2(buff, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint32mf2_t r = __riscv_vfcvt_rtz_x_f_v_i32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_i32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfcvt_rtz_x_f_v_i32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vfcvt_rtz_x_f_v_i64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfcvt_rtz_x_f_v_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vfcvt_rtz_xu_f_v_u32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfcvt_rtz_xu_f_v_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m2_t va = __riscv_vle32_v_f32m2(buff, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vuint32m2_t r = __riscv_vfcvt_rtz_xu_f_v_u32m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_u32m2(so32, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vfcvt_rtz_xu_f_v_u32m2", (unsigned char *)so32, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32mf2_t va = __riscv_vle32_v_f32mf2(buff, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint32mf2_t r = __riscv_vfcvt_rtz_xu_f_v_u32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_u32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfcvt_rtz_xu_f_v_u32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint64m1_t r = __riscv_vfcvt_rtz_xu_f_v_u64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_u64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfcvt_rtz_xu_f_v_u64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfcvt_f_x_v_f32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfcvt_f_x_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m2_t va = __riscv_vle32_v_i32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vfloat32m2_t r = __riscv_vfcvt_f_x_v_f32m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_f32m2(sof, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vfcvt_f_x_v_f32m2", (unsigned char *)sof, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32mf2_t va = __riscv_vle32_v_i32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vfloat32mf2_t r = __riscv_vfcvt_f_x_v_f32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_f32mf2(sof, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfcvt_f_x_v_f32mf2", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfcvt_f_x_v_f64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfcvt_f_x_v_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfcvt_f_xu_v_f32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfcvt_f_xu_v_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m2_t va = __riscv_vle32_v_u32m2(buf32, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        vfloat32m2_t r = __riscv_vfcvt_f_xu_v_f32m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_f32m2(sof, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vfcvt_f_xu_v_f32m2", (unsigned char *)sof, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32mf2_t va = __riscv_vle32_v_u32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vfloat32mf2_t r = __riscv_vfcvt_f_xu_v_f32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_f32mf2(sof, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfcvt_f_xu_v_f32mf2", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vuint64m1_t va = __riscv_vle64_v_u64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfcvt_f_xu_v_f64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfcvt_f_xu_v_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32mf2_t va = __riscv_vle32_v_f32mf2(buff, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vint64m1_t r = __riscv_vfwcvt_x_f_v_i64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfwcvt_x_f_v_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint64m2_t r = __riscv_vfwcvt_x_f_v_i64m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_i64m2(so64, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwcvt_x_f_v_i64m2", (unsigned char *)so64, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32mf2_t va = __riscv_vle32_v_f32mf2(buff, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vuint64m1_t r = __riscv_vfwcvt_xu_f_v_u64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_u64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfwcvt_xu_f_v_u64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint64m2_t r = __riscv_vfwcvt_xu_f_v_u64m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_u64m2(so64, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwcvt_xu_f_v_u64m2", (unsigned char *)so64, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint32mf2_t va = __riscv_vle32_v_i32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vfloat64m1_t r = __riscv_vfwcvt_f_x_v_f64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfwcvt_f_x_v_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwcvt_f_x_v_f64m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwcvt_f_x_v_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint32mf2_t va = __riscv_vle32_v_u32mf2(buf32, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vfloat64m1_t r = __riscv_vfwcvt_f_xu_v_f64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfwcvt_f_xu_v_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwcvt_f_xu_v_f64m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwcvt_f_xu_v_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32mf2_t va = __riscv_vle32_v_f32mf2(buff, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        vfloat64m1_t r = __riscv_vfwcvt_f_f_v_f64m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfwcvt_f_f_v_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwcvt_f_f_v_f64m2(va, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwcvt_f_f_v_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat64m2_t va = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vint32m1_t r = __riscv_vfncvt_x_f_w_i32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfncvt_x_f_w_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint32mf2_t r = __riscv_vfncvt_x_f_w_i32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_i32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfncvt_x_f_w_i32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat64m2_t va = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vuint32m1_t r = __riscv_vfncvt_xu_f_w_u32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfncvt_xu_f_w_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint32mf2_t r = __riscv_vfncvt_xu_f_w_u32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_u32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfncvt_xu_f_w_u32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vint64m2_t va = __riscv_vle64_v_i64m2(buf64, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vfloat32m1_t r = __riscv_vfncvt_f_x_w_f32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfncvt_f_x_w_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat32mf2_t r = __riscv_vfncvt_f_x_w_f32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_f32mf2(sof, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfncvt_f_x_w_f32mf2", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vuint64m2_t va = __riscv_vle64_v_u64m2(buf64, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vfloat32m1_t r = __riscv_vfncvt_f_xu_w_f32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfncvt_f_xu_w_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint64m1_t va = __riscv_vle64_v_u64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat32mf2_t r = __riscv_vfncvt_f_xu_w_f32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_f32mf2(sof, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfncvt_f_xu_w_f32mf2", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat64m2_t va = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vfloat32m1_t r = __riscv_vfncvt_f_f_w_f32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfncvt_f_f_w_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat32mf2_t r = __riscv_vfncvt_f_f_w_f32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_f32mf2(sof, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfncvt_f_f_w_f32mf2", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat64m2_t va = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vint32m1_t r = __riscv_vfncvt_rtz_x_f_w_i32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfncvt_rtz_x_f_w_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint32mf2_t r = __riscv_vfncvt_rtz_x_f_w_i32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_i32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfncvt_rtz_x_f_w_i32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat64m2_t va = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vuint32m1_t r = __riscv_vfncvt_rtz_xu_f_w_u32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfncvt_rtz_xu_f_w_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint32mf2_t r = __riscv_vfncvt_rtz_xu_f_w_u32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_u32mf2(so32, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfncvt_rtz_xu_f_w_u32mf2", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat64m2_t va = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vfloat32m1_t r = __riscv_vfncvt_rod_f_f_w_f32m1(va, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfncvt_rod_f_f_w_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat32mf2_t r = __riscv_vfncvt_rod_f_f_w_f32mf2(va, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_f32mf2(sof, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfncvt_rod_f_f_w_f32mf2", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t vd = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vslideup_vx_f32m1(vd, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vslideup_vx_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t vd = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t va = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vslideup_vx_f64m1(vd, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vslideup_vx_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint16m1_t vd = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t va = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vslideup_vx_i16m1(vd, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vslideup_vx_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint32m1_t vd = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t va = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vslideup_vx_i32m1(vd, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vslideup_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t vd = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t va = __riscv_vle64_v_i64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vslideup_vx_i64m1(vd, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vslideup_vx_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t vd = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t va = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vslideup_vx_i8m1(vd, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vslideup_vx_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t vd = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t va = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vslideup_vx_u32m1(vd, va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vslideup_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vslidedown_vx_f32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vslidedown_vx_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vslidedown_vx_f64m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vslidedown_vx_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vslidedown_vx_i16m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vslidedown_vx_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vslidedown_vx_i32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vslidedown_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vslidedown_vx_i64m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vslidedown_vx_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vslidedown_vx_i8m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vslidedown_vx_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vslidedown_vx_u32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vslidedown_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vslide1up_vx_i16m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vslide1up_vx_i16m1", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vslide1up_vx_i32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vslide1up_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vslide1up_vx_i64m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vslide1up_vx_i64m1", (unsigned char *)so64, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vslide1up_vx_i8m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vslide1up_vx_i8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vslide1up_vx_u32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vslide1up_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vslide1down_vx_i16m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vslide1down_vx_i16m1", (unsigned char *)so16, (((__riscv_vlenb() * 1 * 8 / (1 * 16)) - 1) * 2));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vslide1down_vx_i32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vslide1down_vx_i32m1", (unsigned char *)so32, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) - 1) * 4));
    }
    {
        int i;
        vint64m1_t va = __riscv_vle64_v_i64m1(buf64, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vslide1down_vx_i64m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vslide1down_vx_i64m1", (unsigned char *)so64, (((__riscv_vlenb() * 1 * 8 / (1 * 64)) - 1) * 8));
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vslide1down_vx_i8m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vslide1down_vx_i8m1", (unsigned char *)so8, (((__riscv_vlenb() * 1 * 8 / (1 * 8)) - 1) * 1));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vslide1down_vx_u32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vslide1down_vx_u32m1", (unsigned char *)so32, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) - 1) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfslide1up_vf_f32m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfslide1up_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfslide1up_vf_f64m1(va, 0.5, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfslide1up_vf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfslide1down_vf_f32m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfslide1down_vf_f32m1", (unsigned char *)sof, (((__riscv_vlenb() * 1 * 8 / (1 * 32)) - 1) * 4));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfslide1down_vf_f64m1(va, 0.5, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfslide1down_vf_f64m1", (unsigned char *)sod, (((__riscv_vlenb() * 1 * 8 / (1 * 64)) - 1) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vi = __riscv_vle32_v_u32m1(gb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vrgather_vv_f32m1(va, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrgather_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vi = __riscv_vle32_v_u32m1(gb32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrgather_vv_i32m1(va, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrgather_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint8m1_t va = __riscv_vle8_v_u8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vuint8m1_t vi = __riscv_vle8_v_u8m1(gb8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vuint8m1_t r = __riscv_vrgather_vv_u8m1(va, vi, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_u8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vrgather_vv_u8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vrgather_vx_f32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrgather_vx_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrgather_vx_i32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrgather_vx_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vrgather_vx_u32m1(va, 1, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrgather_vx_u32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(gb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vrgatherei16_vv_f32m1(va, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrgatherei16_vv_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint16mf2_t vi = __riscv_vle16_v_u16mf2(gb16, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vrgatherei16_vv_i32m1(va, vi, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vrgatherei16_vv_i32m1", (unsigned char *)so32, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vuint8m1_t va = __riscv_vle8_v_u8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vuint16m2_t vi = __riscv_vle16_v_u16m2(gb16, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vuint8m1_t r = __riscv_vrgatherei16_vv_u8m1(va, vi, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_u8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vrgatherei16_vv_u8m1", (unsigned char *)so8, ((__riscv_vlenb() * 1 * 8 / (1 * 8)) * 1));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwadd_vv_f64m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwadd_vv_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat64m2_t va = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwadd_wv_f64m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwadd_wv_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwadd_vf_f64m2(va, 0.5f, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwadd_vf_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfwadd_wf_f64m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfwadd_wf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat64m1_t va = __riscv_vle64_v_f64m1(bufd, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfwsub_wf_f64m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        pb("__riscv_vfwsub_wf_f64m1", (unsigned char *)sod, ((__riscv_vlenb() * 1 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfrsub_vf_f32m1(va, 0.5f, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfrsub_vf_f32m1", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vbool8_t va = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t vb = __riscv_vlm_v_b8(mbuf, (__riscv_vlenb() * 8 / 8));
        vbool8_t r = __riscv_vmnor_mm_b8(va, vb, (__riscv_vlenb() * 8 / 8));
__riscv_vsm_v_b8(obuf, r, (__riscv_vlenb() * 8 / 8));
if ((__riscv_vlenb() * 8 / 8) % 8) obuf[((__riscv_vlenb() * 8 / 8) + 7) / 8 - 1] &= (unsigned char)((1u << ((__riscv_vlenb() * 8 / 8) % 8)) - 1);
        pb("__riscv_vmnor_mm_b8", (unsigned char *)obuf, (((__riscv_vlenb() * 8 / 8) + 7) / 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwsub_vv_f64m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwsub_vv_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat64m2_t va = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwsub_wv_f64m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwsub_wv_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwsub_vf_f64m2(va, 0.5f, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwsub_vf_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwmul_vv_f64m2(va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwmul_vv_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwmul_vf_f64m2(va, 0.5f, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwmul_vf_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat64m2_t vd = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwmacc_vv_f64m2(vd, va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwmacc_vv_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat64m2_t vd = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwmacc_vf_f64m2(vd, 0.5f, vb, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwmacc_vf_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat64m2_t vd = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwmsac_vv_f64m2(vd, va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwmsac_vv_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat64m2_t vd = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwmsac_vf_f64m2(vd, 0.5f, vb, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwmsac_vf_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat64m2_t vd = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwnmsac_vv_f64m2(vd, va, vb, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwnmsac_vv_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vfloat64m2_t vd = __riscv_vle64_v_f64m2(bufd, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        vfloat32m1_t vb = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m2_t r = __riscv_vfwnmsac_vf_f64m2(vd, 0.5f, vb, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfwnmsac_vf_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t vs = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vredsum_vs_i16m1_i16m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vredsum_vs_i16m1_i16m1", (unsigned char *)so16, 2);
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vs = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vredsum_vs_i32m1_i32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredsum_vs_i32m1_i32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vs = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vredsum_vs_i8m1_i8m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vredsum_vs_i8m1_i8m1", (unsigned char *)so8, 1);
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vs = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vredsum_vs_u32m1_u32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredsum_vs_u32m1_u32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t vs = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vredand_vs_i16m1_i16m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vredand_vs_i16m1_i16m1", (unsigned char *)so16, 2);
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vs = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vredand_vs_i32m1_i32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredand_vs_i32m1_i32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vs = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vredand_vs_i8m1_i8m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vredand_vs_i8m1_i8m1", (unsigned char *)so8, 1);
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vs = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vredand_vs_u32m1_u32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredand_vs_u32m1_u32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t vs = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vredor_vs_i16m1_i16m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vredor_vs_i16m1_i16m1", (unsigned char *)so16, 2);
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vs = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vredor_vs_i32m1_i32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredor_vs_i32m1_i32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vs = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vredor_vs_i8m1_i8m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vredor_vs_i8m1_i8m1", (unsigned char *)so8, 1);
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vs = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vredor_vs_u32m1_u32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredor_vs_u32m1_u32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t vs = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vredxor_vs_i16m1_i16m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vredxor_vs_i16m1_i16m1", (unsigned char *)so16, 2);
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vs = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vredxor_vs_i32m1_i32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredxor_vs_i32m1_i32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vs = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vredxor_vs_i8m1_i8m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vredxor_vs_i8m1_i8m1", (unsigned char *)so8, 1);
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vs = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vredxor_vs_u32m1_u32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredxor_vs_u32m1_u32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t vs = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vredminu_vs_i16m1_i16m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vredminu_vs_i16m1_i16m1", (unsigned char *)so16, 2);
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vs = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vredminu_vs_i32m1_i32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredminu_vs_i32m1_i32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vs = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vredminu_vs_i8m1_i8m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vredminu_vs_i8m1_i8m1", (unsigned char *)so8, 1);
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vs = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vredminu_vs_u32m1_u32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredminu_vs_u32m1_u32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t vs = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vredmin_vs_i16m1_i16m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vredmin_vs_i16m1_i16m1", (unsigned char *)so16, 2);
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vs = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vredmin_vs_i32m1_i32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredmin_vs_i32m1_i32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vs = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vredmin_vs_i8m1_i8m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vredmin_vs_i8m1_i8m1", (unsigned char *)so8, 1);
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vs = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vredmin_vs_u32m1_u32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredmin_vs_u32m1_u32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t vs = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vredmaxu_vs_i16m1_i16m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vredmaxu_vs_i16m1_i16m1", (unsigned char *)so16, 2);
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vs = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vredmaxu_vs_i32m1_i32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredmaxu_vs_i32m1_i32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vs = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vredmaxu_vs_i8m1_i8m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vredmaxu_vs_i8m1_i8m1", (unsigned char *)so8, 1);
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vs = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vredmaxu_vs_u32m1_u32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredmaxu_vs_u32m1_u32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint16m1_t va = __riscv_vle16_v_i16m1(buf16, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t vs = __riscv_vle16_v_i16m1((buf16 + (__riscv_vlenb() * 1 * 8 / (1 * 16))), (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        vint16m1_t r = __riscv_vredmax_vs_i16m1_i16m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
__riscv_vse16_v_i16m1(so16, r, (__riscv_vlenb() * 1 * 8 / (1 * 16)));
        pb("__riscv_vredmax_vs_i16m1_i16m1", (unsigned char *)so16, 2);
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t vs = __riscv_vle32_v_i32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint32m1_t r = __riscv_vredmax_vs_i32m1_i32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_i32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredmax_vs_i32m1_i32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vint8m1_t va = __riscv_vle8_v_i8m1(buf8, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t vs = __riscv_vle8_v_i8m1((buf8 + (__riscv_vlenb() * 1 * 8 / (1 * 8))), (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        vint8m1_t r = __riscv_vredmax_vs_i8m1_i8m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
__riscv_vse8_v_i8m1(so8, r, (__riscv_vlenb() * 1 * 8 / (1 * 8)));
        pb("__riscv_vredmax_vs_i8m1_i8m1", (unsigned char *)so8, 1);
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t vs = __riscv_vle32_v_u32m1((buf32 + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint32m1_t r = __riscv_vredmax_vs_u32m1_u32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_u32m1(so32, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vredmax_vs_u32m1_u32m1", (unsigned char *)so32, 4);
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vs = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfredusum_vs_f32m1_f32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfredusum_vs_f32m1_f32m1", (unsigned char *)sof, 4);
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vs = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfredosum_vs_f32m1_f32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfredosum_vs_f32m1_f32m1", (unsigned char *)sof, 4);
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vs = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfredmin_vs_f32m1_f32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfredmin_vs_f32m1_f32m1", (unsigned char *)sof, 4);
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t vs = __riscv_vle32_v_f32m1((buff + (__riscv_vlenb() * 1 * 8 / (1 * 32))), (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat32m1_t r = __riscv_vfredmax_vs_f32m1_f32m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse32_v_f32m1(sof, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfredmax_vs_f32m1_f32m1", (unsigned char *)sof, 4);
    }
    {
        int i;
        vint32m1_t va = __riscv_vle32_v_i32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vint64m1_t vs = __riscv_vle64_v_i64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vint64m1_t r = __riscv_vwredsum_vs_i32m1_i64m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse64_v_i64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwredsum_vs_i32m1_i64m1", (unsigned char *)so64, 8);
    }
    {
        int i;
        vuint32m1_t va = __riscv_vle32_v_u32m1(buf32, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vuint64m1_t vs = __riscv_vle64_v_u64m1((buf64 + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vuint64m1_t r = __riscv_vwredsumu_vs_u32m1_u64m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse64_v_u64m1(so64, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vwredsumu_vs_u32m1_u64m1", (unsigned char *)so64, 8);
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m1_t vs = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfwredusum_vs_f32m1_f64m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfwredusum_vs_f32m1_f64m1", (unsigned char *)sod, 8);
    }
    {
        int i;
        vfloat32m1_t va = __riscv_vle32_v_f32m1(buff, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        vfloat64m1_t vs = __riscv_vle64_v_f64m1((bufd + (__riscv_vlenb() * 1 * 8 / (1 * 64))), (__riscv_vlenb() * 1 * 8 / (1 * 64)));
        vfloat64m1_t r = __riscv_vfwredosum_vs_f32m1_f64m1(va, vs, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
__riscv_vse64_v_f64m1(sod, r, (__riscv_vlenb() * 1 * 8 / (1 * 32)));
        pb("__riscv_vfwredosum_vs_f32m1_f64m1", (unsigned char *)sod, 8);
    }
    {
        int i;
        vfloat32m2_t r = __riscv_vfmv_v_f_f32m2(0.5f, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
__riscv_vse32_v_f32m2(sof, r, (__riscv_vlenb() * 2 * 8 / (1 * 32)));
        pb("__riscv_vfmv_v_f_f32m2", (unsigned char *)sof, ((__riscv_vlenb() * 2 * 8 / (1 * 32)) * 4));
    }
    {
        int i;
        vfloat32mf2_t r = __riscv_vfmv_v_f_f32mf2(0.5f, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
__riscv_vse32_v_f32mf2(sof, r, (__riscv_vlenb() * 1 * 8 / (2 * 32)));
        pb("__riscv_vfmv_v_f_f32mf2", (unsigned char *)sof, ((__riscv_vlenb() * 1 * 8 / (2 * 32)) * 4));
    }
    {
        int i;
        vfloat64m2_t r = __riscv_vfmv_v_f_f64m2(0.5, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
__riscv_vse64_v_f64m2(sod, r, (__riscv_vlenb() * 2 * 8 / (1 * 64)));
        pb("__riscv_vfmv_v_f_f64m2", (unsigned char *)sod, ((__riscv_vlenb() * 2 * 8 / (1 * 64)) * 8));
    }
    {
        int i;
        vint16m2_t r = __riscv_vmv_v_x_i16m2(3, (__riscv_vlenb() * 2 * 8 / (1 * 16)));
__riscv_vse16_v_i16m2(so16, r, (__riscv_vlenb() * 2 * 8 / (1 * 16)));
        pb("__riscv_vmv_v_x_i16m2", (unsigned char *)so16, ((__riscv_vlenb() * 2 * 8 / (1 * 16)) * 2));
    }
    {
        int i;
        vint16mf2_t r = __riscv_vmv_v_x_i16mf2(3, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
__riscv_vse16_v_i16mf2(so16, r, (__riscv_vlenb() * 1 * 8 / (2 * 16)));
        pb("__riscv_vmv_v_x_i16mf2", (unsigned char *)so16, ((__riscv_vlenb() * 1 * 8 / (2 * 16)) * 2));
    }
    return 0;
}
