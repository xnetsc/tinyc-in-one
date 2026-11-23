#include <stdio.h>

static int add(int a, int b) { return a + b; }

int main(void)
{
    printf("hello from tinyc: %d\n", add(19, 23));
    return 0;
}
