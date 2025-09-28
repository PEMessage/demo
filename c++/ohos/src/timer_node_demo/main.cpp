#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>

#include "misc.h"
#include "node_handle.h"
#include "node_device.h"
#include "timer.h"

int testNodePreemptive() {
    // Create a timer for the NodeManager
    Utils::Timer timer("Timer");
    timer.Setup();

    {
        // Create NodeManager with a file path (using console output since file may not exist)
        NodeDevice device("MockPath", timer);

        // Create different nodes with different configurations
        NodeHandle& node1 = device.createHandle(Config{BlinkMode{200}});
        std::this_thread::sleep_for(std::chrono::seconds(1));

        NodeHandle& node2 = device.createHandle(Config{SwitchMode{true}});
        std::this_thread::sleep_for(std::chrono::seconds(1));
        assert(node2.device_->read() == true);

        NodeHandle& node3 = device.createHandle(Config{DutyMode{500, 90}});
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    timer.Shutdown();

    return 0;
}

int main() {
    testNodePreemptive();
}
