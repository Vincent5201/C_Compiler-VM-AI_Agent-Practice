int add(int a, int b)
{
    int c;

    c = a + b;

    if (c > 5) {
        return 0;
    }

    return c;
}

int main()
{
    int x;

    x = add(10, 20);

    return x;
}