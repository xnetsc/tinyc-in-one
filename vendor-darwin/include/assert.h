/* assert.h - diagnostics, Darwin.
 *
 * Written for this package from the C99 <assert.h> requirements.
 * A failed assertion calls libSystem's __assert_rtn, the entry
 * point Darwin uses (it takes the function name as its first
 * argument).  Like the standard header, this one is not guarded:
 * assert is (re)defined on every inclusion according to NDEBUG.
 */

#undef assert

#ifdef NDEBUG
#  define assert(e)	((void)0)
#else
void __assert_rtn(const char *func, const char *file, int line,
		  const char *expr);
#  define assert(e) \
	((e) ? (void)0 \
	     : __assert_rtn(__func__, __FILE__, __LINE__, #e))
#endif

#define _assert(e)	assert(e)

#ifndef __cplusplus
#  define static_assert	_Static_assert
#endif
