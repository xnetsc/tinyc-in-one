/* integer-only, straight-line code: the subset the skeleton implements */
int add(int a, int b)
{
    return a + b;
}

int triple(int x)
{
    return add(x, x) + x;
}

int poly(int x)
{
    int y = x * x;
    int z = triple(y);
    return y + z - add(3, 4);
}

int mix(int a, int b)
{
    int q = a / b;
    int r = a % b;
    return (q << 2) | (r & 7) ^ (a - b);
}
