
// See: https://zhuanlan.zhihu.com/c_1707545619290316800
// .\main.exe "int f(int a, int b){ return a + b; }" 1 2
#include <fstream>
#include <iostream>
#include <string>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#define Alloc(size) VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)
#elif __linux__
#include <sys/mman.h>
#define Alloc(size) mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
// MAP_PRIVATE：创建私有写时复制映射，更新对其他进程不可见，也不同步到文件，mmap 后文件的修改是否可见未指定
// MAP_ANONYMOUS/MAP_ANON：匿名映射，不基于文件，内容初始化为 0，需确保 fd 为 - 1 且 offset 为 0
#endif

int main(int argc, char* argv[])
{
    std::ofstream("source.c") << argv[1];
    system("gcc -c source.c && objcopy -O binary -j .text source.o source.bin");

    std::ifstream file("source.bin", std::ios::binary);
    std::string source((std::istreambuf_iterator<char>(file)), {});

    auto p = Alloc(source.size());
    memcpy(p, source.c_str(), source.size());

    using Fn = int (*)(int, int);
    std::cout << reinterpret_cast<Fn>(p)(std::stoi(argv[2]), std::stoi(argv[3])) << std::endl;

    return 0;
}
