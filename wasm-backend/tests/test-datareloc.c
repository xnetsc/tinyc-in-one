/* test-datareloc.c - data-section pointer initializers.
 *
 * Initialised pointers (string literals, addresses of globals,
 * pointer arrays, function-pointer tables) are recorded by the
 * frontend as R_DATA_PTR relocations in the section's reloc array;
 * the output stage must apply them against the module layout.
 * probe() returns a bitmask of failures (0 = all good):
 *   bit0: char *s = "hi"          bit3: function-pointer table
 *   bit1: int *p = &gv            bit4: call through the table
 *   bit2: pointer array           bit5: int *q = &arr[1][0] style
 *                                   cross-section reference via p2
 */
char *s = "hi";
int gv = 77;
int *p = &gv;
char *arr[3] = { "a", "bc", "def" };
char **p2 = &arr[1];
static int seven(void) { return 7; }
static int nine(void) { return 9; }
int (*tab[2])(void) = { seven, nine };
int (*fp)(void) = seven;

int probe(void)
{
    int bad = 0;
    int (*f)(void);
    if (!s || s[0] != 'h' || s[1] != 'i' || s[2] != 0) bad |= 1;
    if (!p || *p != 77) bad |= 2;
    if (!arr[0] || arr[0][0] != 'a' || !arr[1] || arr[1][1] != 'c' ||
        !arr[2] || arr[2][2] != 'f') bad |= 4;
    if (tab[0] != seven || tab[1] != nine) bad |= 8;
    f = tab[1];
    if (f() != 9) bad |= 16;
    if (fp != seven) bad |= 32;
    if (!p2 || *p2 != arr[1] || (*p2)[0] != 'b') bad |= 64;
    return bad;
}
