/* test-fpcall.c - calls through function-pointer lvalues.
 *
 * A call whose callee expression is a global function-pointer
 * variable (or an element of a function-pointer array) has an
 * SValue of VT_SYM|VT_LVAL: it names a memory cell whose CONTENTS
 * are the callee.  The call must load the pointer and go indirect.
 * Compiling it as a direct call to the cell's symbol instead calls
 * whatever function bears the cell's name - or, with no such
 * function, invents an import named after the variable.
 * probe() returns a bitmask of failures (0 = all good):
 *   bit0: global var -> differently-named function, called directly
 *   bit1: global var retargeted at runtime, call follows the cell
 *   bit2: array elements tab[0] / tab[1] called directly
 *   bit3: array element retargeted at runtime
 *   bit4: static local function-pointer variable
 *   bit5: control - local copy (always went indirect)
 *   bit6: control - plain direct call unaffected
 */
static int plus1(int x)   { return x + 1; }
static int plus100(int x) { return x + 100; }
static int plus7(int x)   { return x + 7; }

int (*gfp)(int) = plus100;
int (*tab[2])(int) = { plus1, plus100 };

static int via_static_local(int x)
{
    static int (*lfp)(int) = plus7;
    return lfp(x);
}

int probe(void)
{
    int bad = 0;
    int (*l)(int);
    if (gfp(1) != 101) bad |= 1;
    gfp = plus7;
    if (gfp(1) != 8) bad |= 2;
    gfp = plus100;
    if (tab[0](1) != 2) bad |= 4;
    if (tab[1](1) != 101) bad |= 8;
    tab[0] = plus100;
    if (tab[0](1) != 101) bad |= 16;
    if (via_static_local(1) != 8) bad |= 32;
    l = gfp;
    if (l(1) != 101) bad |= 64;
    if (plus100(1) != 101) bad |= 128;
    return bad;
}
