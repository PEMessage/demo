#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cstring>

// Sequential approach (simple but slower)
void write_sequential(const std::vector<std::string>& paths, const std::string& content) {
    for (const auto& path : paths) {
        std::ofstream file(path, std::ios::out | std::ios::trunc);
        file << content;
    }
}

// Parallel approach (faster for many files)
void write_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    file << content;
}

void write_parallel(const std::vector<std::string>& paths, const std::string& content) {
    std::vector<std::thread> threads;
    threads.reserve(paths.size());
    
    for (const auto& path : paths) {
        threads.emplace_back(write_file, path, content);
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// Using sendfile() system call (fastest for large files)
void write_sendfile(const std::vector<std::string>& paths, const std::string& content) {
    if (paths.empty()) return;
    
    // Write to first file normally
    int first_fd = open(paths[0].c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (first_fd != -1) {
        write(first_fd, content.data(), content.size());
    }
    
    // Use first file as source for others
    int src_fd = first_fd;
    size_t filesize = lseek(src_fd, 0, SEEK_END);
    
    off_t offset = 0;
    for (size_t i = 1; i < paths.size(); ++i) {
        int dest_fd = open(paths[i].c_str(), O_WRONLY | O_CREAT , 0644);
        int ret = sendfile(dest_fd, src_fd, &offset, filesize);
        offset = 0;
        close(dest_fd);
    }
    
    close(src_fd);
}


// Memory-mapped approach (fast for medium files)
void write_mmap(const std::vector<std::string>& paths, const std::string& content) {
    for (const auto& path : paths) {
        int fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
        ftruncate(fd, content.size());
        void* mem = mmap(NULL, content.size(), PROT_WRITE, MAP_SHARED, fd, 0);
        memcpy(mem, content.data(), content.size());
        munmap(mem, content.size());
        close(fd);
    }
}

// Benchmark helper function
template<typename Func>
void benchmark(const std::string& name, Func func, 
               const std::vector<std::string>& paths, 
               const std::string& content) {
    auto start = std::chrono::high_resolution_clock::now();
    func(paths, content);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << name << " took " << duration.count() << " ms\n";
}

// Test function to verify all files were written correctly
bool verify_files(const std::vector<std::string>& paths, const std::string& expected) {
    for (const auto& path : paths) {
        std::ifstream file(path);
        std::string content((std::istreambuf_iterator<char>(file)), 
                            std::istreambuf_iterator<char>());
        if (content != expected) {
            std::cerr << "Verification failed for file: " << path << "\n";
            return false;
        }
    }
    return true;
}

int main() {
    // Test parameters
    const int num_files = 2000;
    const int file_size = 2048;
    const std::string contentX(file_size, 'X');
    const std::string contentY(file_size, 'Y');
    const std::string contentZ(file_size, 'Z');
    const std::string base_path = "test_file_";
    
    // Prepare test files
    std::vector<std::string> paths;
    for (int i = 0; i < num_files; ++i) {
        paths.push_back(base_path + std::to_string(i));
    }
    
    // Run benchmarks
    std::cout << "Testing with " << num_files << " files of " << file_size << " bytes each\n";
    
    benchmark("Sequential", write_sequential, paths, contentX);
    if (!verify_files(paths, contentX)) return 1;
    
    benchmark("Sendfile  ", write_sendfile, paths, contentY);
    if (!verify_files(paths, contentY)) return 1;
    
    benchmark("MMap      ", write_mmap, paths, contentZ);
    if (!verify_files(paths, contentZ)) return 1;
    
    // Clean up
    for (const auto& path : paths) {
        unlink(path.c_str());
    }
    
    return 0;
}
