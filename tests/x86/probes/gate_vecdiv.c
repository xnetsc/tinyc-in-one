/* vector integer division has no x86 SIMD instruction: the
   operator must be rejected by name at any level */
typedef int v4si __attribute__((__vector_size__(16)));

int main(void)
{
    v4si a = { 100, 200, 300, 400 };
    v4si b = { 3, 5, 7, 9 };
    v4si r = a / b;
    return r[0];
}
