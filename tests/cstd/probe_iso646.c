/* probe_iso646.c - ordinary use of <iso646.h>: the alternative
 * spellings as the only operator tokens in the program. */
#include <iso646.h>
#include <stdio.h>

int main(void)
{
	int a = 6, b = 3;
	if (a > 0 and b > 0 and not (a == b))
		printf("and/not ok\n");
	if (a == 0 or b == 3)
		printf("or ok\n");
	if ((a bitand b) == 2 and (a bitor b) == 7 and (a xor b) == 5)
		printf("bitand/bitor/xor ok\n");
	if (compl a == ~6 and (a not_eq b))
		printf("compl/not_eq ok\n");
	a and_eq b;	/* a = 2 */
	a or_eq 8;	/* a = 10 */
	a xor_eq 3;	/* a = 9 */
	if (a == 9)
		printf("iso646 probe: OK\n");
	return a == 9 ? 0 : 1;
}
