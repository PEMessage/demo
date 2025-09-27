#include <iostream>
#include <unistd.h>
#include "thread_ex.h"

using namespace OHOS;

// Custom thread class that inherits from Thread
class DemoThread : public Thread {
protected:
    bool Run() override {
        static int counter = 0;

        std::cout << "DemoThread running, counter: " << counter++ << std::endl;

        // Simulate some work
        sleep(1);

        // Continue running until counter reaches 5
        return (counter < 5);
    }

public:
    bool ReadyToWork() override {
        std::cout << "DemoThread is ready to work!" << std::endl;
        return true;
    }
};

int main() {
    std::cout << "=== OHOS ThreadEx Demo ===" << std::endl;

    // Demo 1: Self-terminating thread
    {
        std::cout << "\n1. Self-terminating thread demo:" << std::endl;
        DemoThread thread1;

        // Start the thread
        ThreadStatus status = thread1.Start("DemoThread");
        if (status != ThreadStatus::OK) {
            std::cout << "Failed to start DemoThread: " << static_cast<int>(status) << std::endl;
            return -1;
        }

        std::cout << "Thread started, waiting for it to finish..." << std::endl;

        // Wait for the thread to finish naturally (after 5 iterations)
        while (thread1.IsRunning()) {
            sleep(1);
        }

        std::cout << "DemoThread finished naturally" << std::endl;
    }


    std::cout << "\n=== Demo completed ===" << std::endl;
    return 0;
}
