/* the feature-gating pair for the aarch64 dot product builtin:
   at the default armv8-a level the compile must fail naming
   DOTPROD; with -march=armv8.2-a+dotprod it must compile and the
   program must verify the numeric result (exit code 0) */
#include <arm_neon.h>
int32x4_t gres;
int main(void)
{
    int32x4_t acc = { 1, 2, 3, 4 };
    uint8x16_t a = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    uint8x16_t b = { 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    int32x4_t r = vdotq_u32(acc, a, b);
    gres = r;
    int *p = (int *)&gres;
    /* lanes: 141, 270, 271, 144 -> sum 826 */
    return p[0] + p[1] + p[2] + p[3] - 826;
}
