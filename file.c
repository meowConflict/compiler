void plus(int x1, int x2, int x3, int x4, int y1, int y2, int y3, int y4, int *a, int *b, int *c, int *d)
{
    *a = x1*y1+x2*y3;
    *b = x1*y2+x2*y4;
    *c = x3*y1+x4*y3;
    *d = x3*y2+x4*y4;
}
void f(int n, int *a, int *b, int *c, int *d)
{
    if (n == 1) {
        *a = 1;
        *b = 1;
        *c = 1;
        *d = 0;
        return;
    }
    int aa, bb, cc, dd;
    f(n/2, &aa, &bb, &cc, &dd);
    plus(aa, bb, cc, dd, aa, bb, cc, dd, &aa, &bb, &cc, &dd);
    if (n % 2)
    {
        plus(1, 1, 1, 0, aa, bb, cc, dd, &aa, &bb, &cc, &dd);    
    }
    *a = aa;
    *b = bb;
    *c = cc;
    *d = dd;
}
int main()
{
    int a, b, c, d;
    f(200000000, &a, &b, &c, &d);
    printf("%d\n", b);
}
