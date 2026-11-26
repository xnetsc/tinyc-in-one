/* Atomic operations (threads design §8): the __atomic_* helper
   family, the C11 surface over it, the fences, lock-free queries,
   atomic_flag, and direct _Atomic variable access.  Every function
   is exported and checked against an exact expected value by
   run-atomics.mjs.  Single-threaded: this stage proves the emitted
   instruction sequences compute the right values; the two-thread
   contention cases live in the process thread suites. */

#include <stdatomic.h>
#include <stdbool.h>

static _Atomic int ai;
static _Atomic long long all;
static _Atomic unsigned char auc;
static _Atomic short ash;
static _Atomic float af;
static _Atomic double ad;
static atomic_flag flag = ATOMIC_FLAG_INIT;

/* ---- fetch / op-fetch on int ------------------------------------- */

int fa_seq(void)
{
    atomic_store(&ai, 10);
    int a = atomic_fetch_add(&ai, 5);   /* old 10 */
    int b = atomic_fetch_sub(&ai, 3);   /* old 15 */
    int c = __atomic_add_fetch(&ai, 100, __ATOMIC_SEQ_CST); /* new 112 */
    int d = __atomic_sub_fetch(&ai, 12, __ATOMIC_SEQ_CST);  /* new 100 */
    return a + b * 1000 + c * 100000 + d; /* 10 + 15000 + 11200000 + 100 */
}

int fa_bits(void)
{
    atomic_store(&ai, 0x30);
    int a = atomic_fetch_or(&ai, 0x05);   /* old 0x30 */
    int b = atomic_fetch_and(&ai, 0x14);  /* old 0x35 */
    int c = atomic_fetch_xor(&ai, 0x1f);  /* old 0x14 */
    int d = __atomic_or_fetch(&ai, 0x40, __ATOMIC_SEQ_CST);   /* new 0x4b */
    int e = __atomic_and_fetch(&ai, 0x4a, __ATOMIC_SEQ_CST);  /* new 0x4a */
    int f = __atomic_xor_fetch(&ai, 0xff, __ATOMIC_SEQ_CST);  /* new 0xb5 */
    return a + b + c + d + e + f + atomic_load(&ai);
}

int fa_nand(void)
{
    atomic_store(&ai, 0x3c);
    int a = __atomic_fetch_nand(&ai, 0x2a, __ATOMIC_SEQ_CST); /* old 0x3c; cell ~(0x3c&0x2a) */
    int cell = atomic_load(&ai);
    int b = __atomic_nand_fetch(&ai, 0x0f, __ATOMIC_SEQ_CST); /* new ~(cell&0x0f) */
    return a == 0x3c && cell == (int)~(0x3c & 0x2a) &&
           b == (int)~(cell & 0x0f) && atomic_load(&ai) == b;
}

/* ---- load / store / exchange ------------------------------------- */

int xchg(void)
{
    atomic_store(&ai, 7);
    int old = atomic_exchange(&ai, 42);
    return old * 1000 + atomic_load(&ai); /* 7042 */
}

int load_store_explicit(void)
{
    atomic_store_explicit(&ai, 123, memory_order_relaxed);
    return atomic_load_explicit(&ai, memory_order_acquire);
}

/* ---- compare-exchange -------------------------------------------- */

int cmpxchg_ok(void)
{
    atomic_store(&ai, 5);
    int expected = 5;
    bool ok = atomic_compare_exchange_strong(&ai, &expected, 9);
    return ok && atomic_load(&ai) == 9 && expected == 5;
}

int cmpxchg_fail(void)
{
    atomic_store(&ai, 5);
    int expected = 4;
    bool ok = atomic_compare_exchange_strong(&ai, &expected, 9);
    /* on failure *expected is replaced by the actual value */
    return !ok && atomic_load(&ai) == 5 && expected == 5;
}

int cmpxchg_weak(void)
{
    atomic_store(&ai, 3);
    int expected = 3;
    bool ok = atomic_compare_exchange_weak(&ai, &expected, 8);
    return ok && atomic_load(&ai) == 8;
}

/* ---- sub-word sizes ---------------------------------------------- */

int subword(void)
{
    atomic_store(&auc, 200);
    int a = atomic_fetch_add(&auc, 100); /* old 200, wraps to 44 */
    int cell = atomic_load(&auc);
    atomic_store(&ash, -30000);
    int b = atomic_fetch_sub(&ash, -30000); /* old -30000 */
    int scell = atomic_load(&ash);          /* 0 */
    /* a signed char store/load round-trips negative values */
    _Atomic signed char sc;
    atomic_store(&sc, -5);
    int c = atomic_load(&sc);
    int d = atomic_fetch_add(&sc, -10); /* old -5 */
    int e = atomic_load(&sc);           /* -15 */
    return a == 200 && cell == 44 && b == -30000 && scell == 0 &&
           c == -5 && d == -5 && e == -15;
}

/* ---- 8-byte atoms ------------------------------------------------- */

long long ll_seq(void)
{
    atomic_store(&all, 5000000000LL);
    long long a = atomic_fetch_add(&all, 3000000000LL); /* 5000000000 */
    long long b = __atomic_add_fetch(&all, 1, __ATOMIC_SEQ_CST);            /* 8000000001 */
    long long c = __atomic_fetch_nand(&all, 0x0f0f0f0fLL, __ATOMIC_SEQ_CST);
    long long cell = atomic_load(&all);
    return a == 5000000000LL && b == 8000000001LL &&
           c == 8000000001LL &&
           cell == ~(8000000001LL & 0x0f0f0f0fLL) ? b : -1;
}

int ll_cmpxchg(void)
{
    atomic_store(&all, 9000000000LL);
    long long expected = 9000000000LL;
    bool ok = atomic_compare_exchange_strong(&all, &expected, -42);
    if (!ok || atomic_load(&all) != -42) return 0;
    expected = 1;
    ok = atomic_compare_exchange_strong(&all, &expected, 7);
    return !ok && expected == -42 && atomic_load(&all) == -42;
}

/* ---- float atoms (generic builtins) ------------------------------- */

int float_ops(void)
{
    float f = 1.5f, out = 0.0f, nf = 2.5f;
    __atomic_store(&af, &f, __ATOMIC_SEQ_CST);
    __atomic_load(&af, &out, __ATOMIC_SEQ_CST);
    if (out != 1.5f) return 1;
    float old;
    __atomic_exchange(&af, &nf, &old, __ATOMIC_SEQ_CST);
    if (old != 1.5f) return 2;
    __atomic_load(&af, &out, __ATOMIC_SEQ_CST);
    if (out != 2.5f) return 3;
    float expected = 2.5f, desired = 4.25f;
    bool ok = __atomic_compare_exchange(&af, &expected, &desired,
                                        0, __ATOMIC_SEQ_CST,
                                        __ATOMIC_SEQ_CST);
    if (!ok) return 4;
    __atomic_load(&af, &out, __ATOMIC_SEQ_CST);
    if (out != 4.25f) return 5;
    double d = 3.25, dout = 0.0, nd = -1.5;
    __atomic_store(&ad, &d, __ATOMIC_SEQ_CST);
    __atomic_exchange(&ad, &nd, &dout, __ATOMIC_SEQ_CST);
    if (dout != 3.25) return 6;
    __atomic_load(&ad, &dout, __ATOMIC_SEQ_CST);
    if (dout != -1.5) return 7;
    return 0;
}

/* ---- atomic_flag ---------------------------------------------------- */

int flag_ops(void)
{
    if (atomic_flag_test_and_set(&flag)) return 1; /* was clear */
    if (!atomic_flag_test_and_set(&flag)) return 2; /* now set */
    atomic_flag_clear(&flag);
    if (atomic_flag_test_and_set(&flag)) return 3;
    atomic_flag_clear_explicit(&flag, memory_order_release);
    return atomic_flag_test_and_set_explicit(&flag,
                                             memory_order_acquire)
               ? 4
               : 0;
}

/* ---- fences and lock-free ------------------------------------------ */

int fence_and_lockfree(void)
{
    atomic_thread_fence(memory_order_seq_cst);
    atomic_signal_fence(memory_order_seq_cst);
    __atomic_thread_fence(__ATOMIC_ACQ_REL);
    if (!atomic_is_lock_free(&ai)) return 1;
    if (!__atomic_is_lock_free(8, &all)) return 2;
    if (__atomic_is_lock_free(16, &all)) return 3;
    if (!__atomic_always_lock_free(4, &ai)) return 4;
    if (__atomic_always_lock_free(32, &ai)) return 5;
    return 0;
}

/* ---- direct _Atomic access (the load/store hook) -------------------- */

static _Atomic int direct_i;
static _Atomic long long direct_ll;
static volatile int plain_v;

int direct_access(void)
{
    direct_i = 40;
    direct_i += 2;         /* load + store through the atomic form */
    int a = direct_i;      /* 42 */
    direct_ll = 6000000000LL;
    direct_ll += 1;
    long long b = direct_ll;
    plain_v = 5;           /* volatile takes the atomic form too */
    plain_v += 1;
    return a == 42 && b == 6000000001LL && plain_v == 6;
}

/* a helper-named function of the user's own is NOT intercepted:
   the interception applies only to undefined helpers */
static int __atomic_fetch_add_4(int *p, int v, int m)
{
    (void)m;
    int old = *p;
    *p = old + v * 10;
    return old;
}

int user_defined_helper(void)
{
    int x = 1;
    int old = __atomic_fetch_add_4(&x, 2, 0);
    return old == 1 && x == 21;
}
