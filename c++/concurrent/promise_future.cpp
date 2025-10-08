#include <iostream>
#include <future>
#include <thread>
#include <chrono>

// Furture: 
//  compare to optional, has `get()` to block until value set
//  using promise set the value of `furture`
int main() {
    // Create promise and future Pair
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();

    // Launch worker thread to set the value
    std::thread worker([&prom]() {
        std::cout << "Worker thread: Sleeping for 1 second...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Worker thread: Setting value to 42\n";
        prom.set_value(42);  // Asynchronously set the value!
    });

    // Main thread waits for the result
    std::cout << "Main thread: Waiting for result...\n";
    int result = fut.get();  // Blocks until set_value is called
    std::cout << "Main thread: Received result: " << result << "\n";

    // Clean up
    worker.join();
    std::cout << "Main thread: Worker thread joined successfully\n";

    return 0;
}
