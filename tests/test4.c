int loop(int a)
{
    int c;
    c = a;

    while (c > 0) {
        a = a + c;
        c = c - 1;
    }

    return a;
}

int main()
{
    int x;

    x = 10;

    x = loop(x);

    return x;
}