/* Hand-written probes for the calling convention: vector
   arguments and results, homogeneous aggregates, the structured
   load result types, mixed scalar/vector signatures and va_arg. */
#include "neon_probe.h"
#include <stdarg.h>

static int32x4_t add4(int32x4_t a, int32x4_t b)
{
    return vaddq_s32(a, b);
}

static int32x4_t many(int32x4_t a, int32x4_t b, int32x4_t c,
                      int32x4_t d, int32x4_t e, int32x4_t f)
{
    /* the sixth vector argument arrives on the stack */
    return vaddq_s32(vaddq_s32(vaddq_s32(a, b), vaddq_s32(c, d)),
                     vaddq_s32(e, f));
}

static float32x4_t mix(int k, float32x4_t a, double w, float32x4_t b)
{
    float32x4_t r = vaddq_f32(a, b);
    if (k > 0)
        r = vmulq_n_f32(r, (float)w);
    return r;
}

struct hfa2 { float32x4_t a, b; };
struct hfa3 { float32x2_t a, b, c; };
struct mixed { int32x4_t v; int n; };

static struct hfa2 hfa_swap(struct hfa2 x)
{
    struct hfa2 r = { x.b, x.a };
    return r;
}

static struct hfa3 hfa_rot(struct hfa3 x)
{
    struct hfa3 r = { x.c, x.a, x.b };
    return r;
}

static struct mixed mixed_bump(struct mixed m, int32x4_t extra)
{
    m.v = vaddq_s32(m.v, extra);
    m.n += 1;
    return m;
}

static int8x16x2_t deint(int8x16x2_t p)
{
    int8x16x2_t r;
    r.val[0] = vaddq_s8(p.val[0], p.val[1]);
    r.val[1] = vsubq_s8(p.val[0], p.val[1]);
    return r;
}

static int32x4_t vsum(int n, ...)
{
    va_list ap;
    int32x4_t acc = vdupq_n_s32(0);
    int i;
    va_start(ap, n);
    for (i = 0; i < n; i++)
        acc = vaddq_s32(acc, va_arg(ap, int32x4_t));
    va_end(ap);
    return acc;
}

static double dsum(int n, ...)
{
    va_list ap;
    double acc = 0;
    int i;
    va_start(ap, n);
    for (i = 0; i < n; i++)
        acc += va_arg(ap, double);
    va_end(ap);
    return acc;
}

typedef int32x4_t (*binop)(int32x4_t, int32x4_t);

int main(void)
{
    int32x4_t a = { 1, 2, 3, 4 }, b = { 10, 20, 30, 40 };
    int32x4_t r = add4(a, b);
    int32x4_t m = many(a, b, a, b, a, b);
    float32x4_t f = mix(2, (float32x4_t){ 1, 2, 3, 4 }, 2.0,
                        (float32x4_t){ 5, 6, 7, 8 });
    struct hfa2 h = { (float32x4_t){ 1, 2, 3, 4 },
                      (float32x4_t){ 5, 6, 7, 8 } };
    struct hfa2 hs = hfa_swap(h);
    struct hfa3 t = { (float32x2_t){ 1, 2 }, (float32x2_t){ 3, 4 },
                      (float32x2_t){ 5, 6 } };
    struct hfa3 tr = hfa_rot(t);
    struct mixed mm = { a, 41 };
    struct mixed mr = mixed_bump(mm, b);
    int8x16_t x = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    int8x16_t y = { 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    int8x16x2_t p = { { x, y } };
    int8x16x2_t dr = deint(p);
    int32x4_t vs = vsum(3, a, b, a);
    binop fp = add4;
    int32x4_t fr = fp(b, b);
    print_hex(1, &r, sizeof r);
    print_hex(2, &m, sizeof m);
    print_hex(3, &f, sizeof f);
    print_hex(4, &hs, sizeof hs);
    print_hex(5, &tr, sizeof tr);
    print_hex(6, &mr, sizeof mr);
    print_hex(7, &dr, sizeof dr);
    print_hex(8, &vs, sizeof vs);
    print_hex(9, &fr, sizeof fr);
    if (dsum(2, 1.5, 2.25) != 3.75)
        mismatch("dsum");
    return fails != 0;
}
