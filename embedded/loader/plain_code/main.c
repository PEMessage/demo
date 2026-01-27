

int global_with_initial = 0xaa;
int global_without_initial;

int func1(int arg)
{
    global_with_initial = arg;
    global_without_initial = 3;
    return global_without_initial;
}

int func2(int arg)
{
    global_with_initial = 1;
    global_without_initial = func1(2);
    return 0;
}
