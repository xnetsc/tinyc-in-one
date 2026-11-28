/* test-deadcode.c - expression-level compile-time dead code must
   not execute: the frontend parses a constant-folded &&, || or ?:
   dead arm with nocode_wanted set, and the backend must record
   none of it.  Side effects in dead arms (a counter bump, a call
   to the imported exit) stay unexecuted; statement-level dead
   code behaves as before; the live arms around them are exact. */
#include <stdio.h>
#include <stdlib.h>

static int counter;
static int bump(void) { counter++; return 1; }

int main(void)
{
    int r = 0;
    r = 0 && bump();          /* dead RHS of && */
    if (r != 0) return 1;
    r = 1 || bump();          /* dead RHS of || */
    if (r != 1) return 2;
    r = 1 ? 42 : bump();      /* dead arm of ?: */
    if (r != 42) return 3;
    r = 0 ? bump() : 43;      /* dead arm of ?: */
    if (r != 43) return 4;
    if (0) { bump(); }        /* statement level */
    while (0) { bump(); }     /* statement level */
    if (counter != 0) { printf("counter=%d\n", counter); return 5; }
    r = 1 || (exit(3), 1);    /* dead arm calls exit(3) */
    if (r != 1) return 6;
    printf("DEADCODE-RESULT: PASS\n");
    return 0;
}
