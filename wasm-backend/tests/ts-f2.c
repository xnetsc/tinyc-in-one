/* second translation unit of the multi-file scenario: calls the
 * first unit's function, so the module is only correct if the
 * registry kept both units of the one program */
int helper(void);
int entry(void) { return helper() + 1; }
