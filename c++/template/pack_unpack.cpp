#include <cstring> // for memcpy
#include <stdexcept> // for runtime_error
#include <iostream>
#include <string>

class Buffer {
private:
    char* buffer_;      // Pointer to the buffer
    size_t size_;       // Current size of data in buffer
    size_t capacity_;   // Total capacity of the buffer

public:
    // Constructor
    Buffer(char* buf, size_t size, size_t capacity)
        : buffer_(buf), size_(size), capacity_(capacity) {}

    // Pack an object into the buffer
    // 设计考虑： 使用异常，而非返回值，可以使得我们不用每一行都写if (buffer.pack) {...}
    template<typename T>
    void pack(const T& obj) {
        if (size_ + sizeof(T) > capacity_) {
            throw std::runtime_error("Buffer capacity exceeded");
        }
        memcpy(buffer_ + size_, &obj, sizeof(T));
        size_ += sizeof(T);
    }

    // Pack arbitrary data into the buffer
    void pack_buffer(const void* data, size_t data_size) {
        if (size_ + data_size > capacity_) {
            throw std::runtime_error("Buffer capacity exceeded");
        }
        memcpy(buffer_ + size_, data, data_size);
        size_ += data_size;
    }

    // Unpack an object from the buffer
    template<typename T>
    void unpack(T& obj) {
        if (size_ < sizeof(T)) {
            throw std::runtime_error("Not enough data in buffer");
        }
        memcpy(&obj, buffer_, sizeof(T));
        buffer_ += sizeof(T);
        size_ -= sizeof(T);
    }

    // Unpack arbitrary data from the buffer
    void unpack_buffer(void* dest, size_t data_size) {
        if (size_ < data_size) {
            throw std::runtime_error("Not enough data in buffer");
        }
        memcpy(dest, buffer_, data_size);
        buffer_ += data_size;
        size_ -= data_size;
    }

    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    size_t remaining() const { return capacity_ - size_; }
    char* data() const { return buffer_; }
};



int main() {
    char raw_buffer[1024]; // Underlying storage
    Buffer buffer(raw_buffer, 0, sizeof(raw_buffer));

    // Pack data using different methods
    int a = 42;
    double b = 3.14159;
    std::string str = "Hello World";

    buffer.pack(a);  // Using template method
    buffer.pack_buffer(&b, sizeof(b));  // Using pack_buffer
    buffer.pack_buffer(str.data(), str.size());  // Packing string data

    std::cout << "Buffer size after packing: " << buffer.size() << std::endl;

    // Unpack the data
    int unpacked_a;
    double unpacked_b;
    char unpacked_str[20] = {0};

    // buffer.reset(); // Reset to start of buffer for reading
    buffer.unpack(unpacked_a);  // Using template method
    buffer.unpack_buffer(&unpacked_b, sizeof(unpacked_b));  // Using unpack_buffer
    buffer.unpack_buffer(unpacked_str, str.size());  // Unpacking string data

    std::cout << "Unpacked values: " << unpacked_a << ", " << unpacked_b
              << ", " << unpacked_str << std::endl;
    std::cout << "Buffer size after unpacking: " << buffer.size() << std::endl;

    return 0;
}

