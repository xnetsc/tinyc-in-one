/* test-datagot-varside.c - the variable half of the data-GOT
   mechanism: a static data initializer holds the address of an
   imported variable.  The slot is filled from GOT.mem at load
   time (which always binds eagerly), so after load it holds
   the variable's absolute address — the value dlsym returns —
   not the addend 0 the slot starts with. */
extern int ext_var;
int *p = &ext_var;
int read_p(void) { return *p; }
int *get_p(void) { return p; }
