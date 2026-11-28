/* test-deadlit.c - string literals whose only references sit in
 * compile-time-dead branches (if (0), a constant-folded ||, && or
 * ?:) are parsed by the frontend with code generation suppressed,
 * which suppresses their symbol definition and their data as
 * well; the backend still records the address materialisations.
 * Those references must not turn into GOT.mem imports in dylink
 * output or unresolved addresses in standalone output, and the
 * live literals around them must keep their exact contents.
 * The translation unit defines no static functions. */
#include <stdio.h>
#include <string.h>

int fails;
#define CHECK(cond) do { if (!(cond)) { \
	printf("deadlit: FAIL line %d: %s\n", __LINE__, #cond); \
	fails++; } } while (0)

const char *name(void) { return "C"; }

int main(void)
{
	/* dead branches naming literals */
	if (0)
		printf("dead-if-literal\n");
	(void)(1 || printf("dead-or-literal\n"));
	(void)(0 && printf("dead-and-literal\n"));
	1 ? (void)0 : printf("dead-cond-literal\n");

	/* live literals, contents checked byte for byte */
	CHECK(strcmp(name(), "C") == 0);
	CHECK(strcmp("prefix-" "joined", "prefix-joined") == 0);
	{
		const char *s = name();
		CHECK(s[0] == 'C' && s[1] == '\0');
	}
	if (fails)
		return 1;
	printf("DEADLIT-RESULT: PASS\n");
	return 0;
}
