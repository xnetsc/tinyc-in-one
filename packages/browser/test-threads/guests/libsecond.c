/* Side module for dlopen_thread: loaded by the main thread
 * after the worker thread already exists, which is what makes
 * the worker call into it afterwards a check of the loader's
 * per-thread region backfill. */
int second_answer(void) { return 43; }
