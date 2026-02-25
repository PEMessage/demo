#include <iostream>
#include <mutex>
#include <thread>

std::mutex mtx;

void test_goto() {
    {
        std::cout << "Before locking" << std::endl;
        std::unique_lock<std::mutex> lock(mtx);
        std::cout << "Lock acquired" << std::endl;

        std::cout << "Preparing to goto" << std::endl;
        goto EXIT;  // Jump out of scope

        std::cout << "This line will not execute" << std::endl;
    }  // unique_lock destructor is called here (due to goto jump)

EXIT:
    std::cout << "Reached EXIT label" << std::endl;
    // Attempt to lock again to verify if the previous lock was released
    std::unique_lock<std::mutex> new_lock(mtx);  // If previous lock wasn't released, this will block
    std::cout << "Lock acquired again, indicating previous lock was released" << std::endl;
}

int main() {
    test_goto();
    return 0;
}
