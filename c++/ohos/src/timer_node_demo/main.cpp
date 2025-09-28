#include <iostream>
#include <thread>
#include <chrono>

#include "misc.h"
#include "node_handle.h"
#include "node_device.h"
#include "timer.h"

int main() {
    // Create a timer for the NodeManager
    Utils::Timer timer("Timer");
    timer.Setup();

    {
        // Create NodeManager with a file path (using console output since file may not exist)
        NodeDevice device("MockPath", timer);

        // Create different nodes with different configurations
        NodeHandle& node1 = device.createHandle(Config{BlinkMode{200}});
        device.update();
        std::this_thread::sleep_for(std::chrono::seconds(2));


        NodeHandle& node2 = device.createHandle(Config{SwitchMode{true}});
        device.update();
        std::this_thread::sleep_for(std::chrono::seconds(2));

        device.deleteHandle(node2);
        device.update();
        std::this_thread::sleep_for(std::chrono::seconds(2));


    }

    timer.Shutdown();

    return 0;
}
