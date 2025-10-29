struct A {
    int a;
    int b;
};


// Thanks to: https://stackoverflow.com/questions/20979565/how-can-i-print-the-result-of-sizeof-at-compile-time-in-c
int main(int argc, char *argv[])
{
    char (*__kaboom)[sizeof( struct A )] = 1;
    return 0;
}
