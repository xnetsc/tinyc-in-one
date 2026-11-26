/* test-datareloc-side.c - dylink side module whose data segment is
   full of pointer initializers; check_data() verifies them from
   inside the module after the loader ran __tinyc_dylink_init
   (which adds the process bases to the relocated slots). */
char *s = "hi";
int gv = 77;
int *p = &gv;
char *arr[3] = { "a", "bc", "def" };
static int seven(void) { return 7; }
static int nine(void) { return 9; }
int (*tab[2])(void) = { seven, nine };
char *get_s(void) { return s; }
int check_data(void)
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
    return bad;
}
