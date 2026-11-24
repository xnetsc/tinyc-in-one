/* x86-64 feature macro gating: the exit code reports the level
   the predefined macros describe */
#if defined(__AVX512F__) && defined(__AVX512BW__) && defined(__AVX512VL__)
int level = 4;
#elif defined(__AVX2__) && defined(__AVX__) && defined(__SSE4_2__) && defined(__FMA__)
int level = 3;
#elif defined(__SSE2__) && defined(__SSE__) && !defined(__SSSE3__)
int level = 0;
#else
#error unexpected macro set
#endif
int main(void) { return level; }
