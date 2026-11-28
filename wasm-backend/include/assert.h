/* assert.h — diagnostics for the wasm32 sysroot.
 *
 * A failed assertion goes through the shim layer's
 * __assert_fail, which writes the message to stderr and aborts;
 * both are supplied by the runtime.  As the standard requires,
 * the assert macro follows the state of NDEBUG at each
 * inclusion point, so this header deliberately has no include
 * guard around the macro: including it again with a different
 * NDEBUG state redefines assert.
 */

#undef assert
#undef static_assert

#define static_assert	_Static_assert

void __assert_fail(const char *assertion, const char *file, int line);

#ifdef NDEBUG
# define assert(expr)	((void)0)
#else
# define assert(expr) \
	((void)((expr) || (__assert_fail(#expr, __FILE__, __LINE__), 0)))
#endif
