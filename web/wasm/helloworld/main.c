extern void console_log(const char* str);

#define UNUSED(x) (void)(x)
#define KEEP __attribute__((used, retain))


#define EXPORT(name) __attribute__((export_name(name)))

// TODO: move this part to js
// align RangeError: start offset of Uint32Array should be a multiple of 4
KEEP char jsruntime[1024];

EXPORT("main") int main(int argc, char *argv[]) {
    // UNUSED(argc);
    // UNUSED(argv);
    console_log("Hello from WebAssembly!");

    // Print all arguments
    for (int i = 0; i < argc; i++) {
        console_log(argv[i]);
    }

    return 0;
}
