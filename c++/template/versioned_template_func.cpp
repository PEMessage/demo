#include <iostream>

#define LEN 1024
struct Data {
    char buf[LEN];
};

#define LEN_V2 2048
struct DataV2 {
    char buf[LEN_V2];
};

template<typename T>
void func(T *data) {
    std::cout << sizeof(data->buf) << std::endl;
}

int main() {
    Data v1 = {0};
    DataV2 v2 = {0};

    func<Data>(&v1);    // 输出 1024
    func<DataV2>(&v2);  // 输出 2048
    return 0;
}
