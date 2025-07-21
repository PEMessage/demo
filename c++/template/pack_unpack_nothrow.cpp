#include <string.h>
#include <iostream>
#include <string>


class Buffer {
private:
    char* buffer_;      // Pointer to the buffer
    size_t size_;       // Current size of data in buffer
    size_t capacity_;   // Total capacity of the buffer

public:
    enum class Error {
        Ok,
        OutOfCapacity,
        NotEnoughData,
    };

    Buffer(char* buf, size_t size, size_t capacity)
        : buffer_(buf), size_(size), capacity_(capacity) {}

    inline Error pack(const void* data, size_t data_size) {
        if (size_ + data_size > capacity_) {
            return Error::OutOfCapacity;
        }
        memcpy(buffer_ + size_, data, data_size);
        size_ += data_size;
        return Error::Ok;
    }

    template<typename T>
    Error pack(const T& obj) {
        return pack(&obj, sizeof(obj));
    }


    inline Error unpack(void* dest, size_t data_size) {
        if (size_ < data_size) {
            return Error::NotEnoughData;
        }
        memcpy(dest, buffer_, data_size);
        buffer_ += data_size;
        size_ -= data_size;
        return Error::Ok;
    }

    template<typename T>
    Error unpack(T& obj) {
        return unpack(&obj, sizeof(obj));
    }

    inline size_t size() const { return size_; }
    inline size_t capacity() const { return capacity_; }
    inline size_t remaining() const { return capacity_ - size_; }
    inline char* data() const { return buffer_; }
};


int main() {
    char raw_buffer[1024]; // Underlying storage
    Buffer buffer(raw_buffer, 0, sizeof(raw_buffer));

    // Pack data using different methods
    int a = 42;
    double b = 3.14159;
    std::string str = "Hello World";

    buffer.pack(a);  // Using template method
    buffer.pack(&b, sizeof(b));  // Using pack_buffer
    buffer.pack(str.data(), str.size());  // Packing string data

    std::cout << "Buffer size after packing: " << buffer.size() << std::endl;

    // Unpack the data
    int unpacked_a;
    double unpacked_b;
    char unpacked_str[20] = {0};

    // buffer.reset(); // Reset to start of buffer for reading
    buffer.unpack(unpacked_a);  // Using template method
    buffer.unpack(&unpacked_b, sizeof(unpacked_b));  // Using unpack_buffer
    buffer.unpack(unpacked_str, str.size());  // Unpacking string data

    std::cout << "Unpacked values: " << unpacked_a << ", " << unpacked_b
              << ", " << unpacked_str << std::endl;
    std::cout << "Buffer size after unpacking: " << buffer.size() << std::endl;

    return 0;
}

