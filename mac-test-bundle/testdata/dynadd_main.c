#include <stdio.h>
#include "mylib.h"

int main(void)
{
    printf("dynadd %s: %d\n", MYLIB_TAG, mylib_answer());
    return 0;
}
