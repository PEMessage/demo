
#include <iostream>
#include "ashmem.h"
#include <string.h>
#include <sys/mman.h>

using namespace OHOS;

int main() {
    const char* name = "DemoAshmem";
    const int32_t size = 4096; // 4KB

    // 1. Create an Ashmem region
    sptr<Ashmem> ashmem = Ashmem::CreateAshmem(name, size);
    if (ashmem == nullptr) {
        std::cerr << "Failed to create ashmem" << std::endl;
        return -1;
    }

    std::cout << "Ashmem created successfully. FD: " << ashmem->GetAshmemFd()
              << ", Size: " << ashmem->GetAshmemSize() << std::endl;

    // 2. Set protection to read-write
    if (!ashmem->SetProtection(PROT_READ | PROT_WRITE)) {
        std::cerr << "Failed to set protection" << std::endl;
        ashmem->CloseAshmem();
        return -1;
    }

    // 3. Map the ashmem for read-write access
    if (!ashmem->MapReadAndWriteAshmem()) {
        std::cerr << "Failed to map ashmem" << std::endl;
        ashmem->CloseAshmem();
        return -1;
    }

    // 4. Write data to ashmem
    const char* testData = "Hello, Ashmem!";
    int dataSize = strlen(testData) + 1;
    if (!ashmem->WriteToAshmem(testData, dataSize, 0)) {
        std::cerr << "Failed to write to ashmem" << std::endl;
        ashmem->UnmapAshmem();
        ashmem->CloseAshmem();
        return -1;
    }

    // 5. Read data from ashmem
    const void* readData = ashmem->ReadFromAshmem(dataSize, 0);
    if (readData == nullptr) {
        std::cerr << "Failed to read from ashmem" << std::endl;
        ashmem->UnmapAshmem();
        ashmem->CloseAshmem();
        return -1;
    }

    std::cout << "Read from ashmem: " << (const char*)readData << std::endl;

    // 6. Clean up
    ashmem->UnmapAshmem();
    ashmem->CloseAshmem();

    std::cout << "Ashmem demo completed successfully" << std::endl;
    return 0;
}
