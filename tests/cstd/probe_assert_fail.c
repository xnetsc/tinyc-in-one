/* probe_assert_fail.c - a failing assertion must print the
 * assertion text to stderr and abort; the runner expects a
 * nonzero outcome and the message. */
#include <assert.h>
#include <stdio.h>

int main(void)
{
	printf("before the assertion\n");
	assert(1 == 2);
	printf("after the assertion (must not print)\n");
	return 0;
}
