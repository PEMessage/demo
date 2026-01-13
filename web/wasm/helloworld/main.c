extern void console_log(const char* str);

#define UNUSED(x) (void)(x)

__attribute__((export_name("main")))
int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    console_log("Hello from WebAssembly!");
    return 0;
}
