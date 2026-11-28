/* getenv reads the environment the process was created with;
 * system has no command processor to talk to. The runner
 * creates the process with CST2_GREETING=hello-42 in its
 * environment. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "guest_check.h"

int main(void) {
  const char *v = getenv("CST2_GREETING");
  CK("getenv_present", v != NULL && strcmp(v, "hello-42") == 0);
  CK("getenv_absent", getenv("CST2_NOT_PRESENT_XYZ") == NULL);
  CK("getenv_stable", getenv("CST2_GREETING") == v);
  CK("system_null", system(NULL) == 0);
  errno = 0;
  int rc = system("echo hi");
  CK("system_cmd_fails", rc == -1);
  CK("system_errno", errno == ENOSYS);
  DONE();
}
