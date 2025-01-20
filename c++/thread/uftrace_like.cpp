#include <iostream>
#include <thread>
#include <mutex>
#include <unistd.h> // For getpid()
// #include <sys/syscall.h>

// Prompt: In C++, using thread local,
//         in every function enter add “  ” to thread local global var INDENT,
//         and print function name with indent，


thread_local std::string INDENT;  // Thread-local variable for indentation
std::mutex print_mutex;           // Mutex for print eachline indepent, (not work perfect, why?)
                                  // not for INDENT

void print_with_indent(const std::string& function_name) {
    std::lock_guard<std::mutex> lock(print_mutex);
    std::cout << gettid()  << "||" <<  INDENT << function_name << std::endl;
}

#define FUNCTION_ENTER() \
    INDENT += "  "; \
    print_with_indent(__PRETTY_FUNCTION__);

#define FUNCTION_EXIT() \
    INDENT.erase(0, 2);  // Remove the last two spaces


void functionC() {
    FUNCTION_ENTER();
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    FUNCTION_EXIT();
}

void functionB() {
    FUNCTION_ENTER();
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    FUNCTION_EXIT();
}

void functionA() {
    FUNCTION_ENTER();
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    functionB();
    FUNCTION_EXIT();
}

void threadFunction() {
    functionA();
    functionC();
}

int main() {
    std::thread t1(threadFunction);
    std::thread t2(threadFunction);

    t1.join();
    t2.join();

    return 0;
}

