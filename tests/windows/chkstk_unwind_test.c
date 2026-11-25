/* Unwind evidence for the __chkstk prolog (PE, x86_64).
 *
 * A function whose frame is 4096 bytes or larger takes the __chkstk
 * prolog (mov eax, size; call __chkstk; nop) instead of the small-frame
 * prolog (push rbp; mov rbp, rsp; sub rsp, size), so its frame pointer
 * is only established when the call returns, at prolog offset 10.
 * The function's UNWIND_INFO must say so: the SET_FPREG and
 * PUSH_NONVOL codes carry offset 10, not the small-frame offsets 4
 * and 1.  This test checks that, two ways, for a big frame with
 * register-cache saves and for one without, against a small-frame
 * control:
 *
 *   1. metadata: read the UNWIND_INFO through RtlLookupFunctionEntry
 *      and assert the frame setup codes' offsets;
 *   2. behaviour: fabricate a CONTEXT whose Rip points inside the
 *      prolog (at the call to __chkstk) and unwind one frame with
 *      RtlVirtualUnwind -- the same engine SEH uses -- asserting the
 *      unwound Rsp/Rip are the values on entry to the function;
 *   3. behaviour: the same unwind from inside the function body,
 *      which exercises the ordinary path (saves, frame pointer,
 *      return address) and guards the fix against breaking it.
 *
 * The test is compiled by the tinyc.exe under test and run natively
 * on the Windows runner.  RtlVirtualUnwind is not declared in TCC's
 * winnt.h, so both ntdll entry points are resolved at run time.
 */
#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef unsigned long long u64;
typedef unsigned int u32;

typedef u64 (__stdcall *RtlVirtualUnwindFn)(u32 HandlerType, u64 ImageBase,
    u64 ControlPc, PRUNTIME_FUNCTION FunctionEntry, PCONTEXT ContextRecord,
    void **HandlerData, u64 *EstablisherFrame, void *ContextPointers);
typedef PRUNTIME_FUNCTION (__stdcall *RtlLookupFunctionEntryFn)(
    u64 ControlPc, u64 *ImageBase, void *HistoryTable);

static int failures = 0;

static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "ok" : "FAIL", what);
    if (!ok)
        failures++;
}

/* --- functions under test (TCC never inlines, so plain definitions do) --- */

int bigframe(int x)
{
    char buf[8192];
    int s = 0;
    for (int i = 0; i < 8192; i++) { buf[i] = (char)(i + x); s += buf[i]; }
    return s;
}

int bigframe_plain(void)
{
    char buf[8192];
    buf[0] = 1;
    buf[8191] = 2;
    return buf[0] + buf[8191];
}

int smallframe(int x)
{
    int a[100];
    int s = 0;
    for (int i = 0; i < 100; i++) { a[i] = i + x; s += a[i]; }
    return s;
}

/* --- unwind-info parsing --- */

/* Unwind code ops (x64): op is the low nibble of the second byte. */
#define UWOP_PUSH_NONVOL   0
#define UWOP_ALLOC_SMALL   1
#define UWOP_ALLOC_LARGE   2
#define UWOP_SET_FPREG     3
#define UWOP_SAVE_NONVOL   4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_SAVE_XMM128   8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME 10

/* Read the frame setup codes' offsets out of a function's UNWIND_INFO.
   Returns 0 on success, filling *fpreg_off and *push_off. */
static int frame_code_offsets(u64 func, RtlLookupFunctionEntryFn lookup,
                              int *fpreg_off, int *push_off)
{
    u64 image_base = 0;
    PRUNTIME_FUNCTION rf = lookup(func, &image_base, NULL);
    unsigned char *info;
    int count, i, slots;
    if (!rf)
        return -1;
    info = (unsigned char *)(image_base + rf->UnwindData);
    if ((info[0] & 7) != 1) /* version */
        return -1;
    count = info[2];
    *fpreg_off = -1;
    *push_off = -1;
    i = 0;
    slots = 0;
    while (slots < count) {
        int off = info[4 + 2 * i];
        int op = info[4 + 2 * i + 1] & 0xf;
        int inf = info[4 + 2 * i + 1] >> 4;
        int extra = 0;
        if (op == UWOP_SET_FPREG)
            *fpreg_off = off;
        else if (op == UWOP_PUSH_NONVOL)
            *push_off = off;
        if (op == UWOP_ALLOC_LARGE)
            extra = (inf == 0) ? 2 : 3;
        else if (op == UWOP_SAVE_NONVOL_FAR || op == UWOP_SAVE_XMM128_FAR)
            extra = 2;
        else if (op == UWOP_SAVE_NONVOL || op == UWOP_SAVE_XMM128)
            extra = 1;
        else if (op == UWOP_PUSH_MACHFRAME)
            extra = 1;
        slots += 1 + extra;
        i += 1 + extra;
    }
    return (*fpreg_off >= 0 && *push_off >= 0) ? 0 : -1;
}

static void check_metadata(const char *name, u64 func,
                           int want_fpreg, int want_push,
                           RtlLookupFunctionEntryFn lookup)
{
    int fpreg = -1, push = -1;
    char msg[128];
    if (frame_code_offsets(func, lookup, &fpreg, &push) != 0) {
        snprintf(msg, sizeof msg, "metadata %s: UNWIND_INFO unreadable", name);
        check(0, msg);
        return;
    }
    snprintf(msg, sizeof msg,
             "metadata %s: SET_FPREG@%d PUSH_NONVOL@%d (want %d/%d)",
             name, fpreg, push, want_fpreg, want_push);
    check(fpreg == want_fpreg && push == want_push, msg);
}

/* --- behavioural unwind --- */

static char fakestack[65536];
#define PLANTED_RIP 0x4010000000000000ULL
#define PLANTED_RBP 0x2222222222222222ULL

/* Unwind one frame from a fabricated context and check the result.
   off is the offset of Rip inside the function; want_rsp/want_rip/
   want_rbp describe the expected unwound state (-1 = do not check). */
static void check_unwind(const char *name, u64 func, unsigned off,
                         u64 want_rsp, u64 want_rip, u64 want_rbp,
                         RtlVirtualUnwindFn unwind,
                         RtlLookupFunctionEntryFn lookup)
{
    CONTEXT ctx;
    u64 image_base = 0, establisher = 0;
    PRUNTIME_FUNCTION rf;
    char msg[160];
    memset(&ctx, 0, sizeof ctx);
    ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    ctx.Rip = func + off;
    ctx.Rsp = (u64)(fakestack + 8192);
    ctx.Rbp = (u64)(fakestack + 4096);
    *(u64 *)(fakestack + 8192) = PLANTED_RIP;
    *(u64 *)(fakestack + 4096) = PLANTED_RBP;
    *(u64 *)(fakestack + 4104) = PLANTED_RIP;
    rf = lookup(func + off, &image_base, NULL);
    if (!rf) {
        snprintf(msg, sizeof msg, "unwind %s: lookup failed", name);
        check(0, msg);
        return;
    }
    unwind(0 /* UNW_FLAG_NHANDLER */, image_base, func + off, rf,
           &ctx, NULL, &establisher, NULL);
    snprintf(msg, sizeof msg,
             "unwind %s off=%u: Rip=%llx (want %llx) Rsp=%llx (want %llx)",
             name, off, ctx.Rip, want_rip, ctx.Rsp, want_rsp);
    check(ctx.Rip == want_rip && ctx.Rsp == want_rsp &&
          (want_rbp == (u64)-1 || ctx.Rbp == want_rbp), msg);
}

int main(void)
{
    void *ntdll = GetModuleHandleA("ntdll.dll");
    RtlVirtualUnwindFn unwind;
    RtlLookupFunctionEntryFn lookup;
    u64 big = (u64)bigframe;
    u64 big_plain = (u64)bigframe_plain;
    u64 small = (u64)smallframe;

    if (!ntdll) {
        check(0, "GetModuleHandleA(ntdll.dll)");
        return 1;
    }
    unwind = (RtlVirtualUnwindFn)GetProcAddress(ntdll, "RtlVirtualUnwind");
    lookup = (RtlLookupFunctionEntryFn)GetProcAddress(ntdll, "RtlLookupFunctionEntry");
    if (!unwind || !lookup) {
        check(0, "resolve RtlVirtualUnwind/RtlLookupFunctionEntry");
        return 1;
    }

    /* exercise the functions so their code is really there */
    printf("big=%d plain=%d small=%d\n",
           bigframe(2), bigframe_plain(), smallframe(1));

    /* 1. metadata: the __chkstk call ends at prolog offset 10, so
       both frame setup codes sit at 10; the small frame keeps the
       push at 1 and the frame-pointer setup at 4. */
    check_metadata("bigframe", big, 10, 10, lookup);
    check_metadata("bigframe_plain", big_plain, 10, 10, lookup);
    check_metadata("smallframe", small, 4, 1, lookup);

    /* 2. unwind from inside the __chkstk call (prolog offset 5):
       no unwind code has executed yet, so the unwound state is the
       caller's state just before the call: Rip is the planted
       return address at the stack top, and Rsp is the stack top
       plus 8 -- RtlVirtualUnwind pops the return-address slot. */
    check_unwind("bigframe prolog", big, 5,
                 (u64)(fakestack + 8200), PLANTED_RIP, (u64)-1,
                 unwind, lookup);
    check_unwind("bigframe_plain prolog", big_plain, 5,
                 (u64)(fakestack + 8200), PLANTED_RIP, (u64)-1,
                 unwind, lookup);

    /* 3. unwind from inside the body (prolog offset 60): the frame
       pointer is established, so Rbp comes back from the slot it
       points at, Rip from the return-address slot 8 bytes above
       it, and Rsp from Rbp + 16 -- past both the saved-RBP slot
       and the return-address slot. */
    check_unwind("bigframe body", big, 60,
                 (u64)(fakestack + 4112), PLANTED_RIP, PLANTED_RBP,
                 unwind, lookup);

    if (failures) {
        printf("chkstk unwind: %d FAILURES\n", failures);
        return 1;
    }
    printf("chkstk unwind: all checks passed\n");
    return 0;
}
