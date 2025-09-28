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
        NodeDevice manager("MockPath", timer);

        // Create different nodes with different configurations
        NodeHandle& node1 = manager.createNode(Config{BlinkMode{200}});
        manager.applyNode();
        std::this_thread::sleep_for(std::chrono::seconds(2));


        NodeHandle& node2 = manager.createNode(Config{SwitchMode{true}});
        manager.applyNode();
        std::this_thread::sleep_for(std::chrono::seconds(2));

        manager.deleteNode(node2);
        manager.applyNode();
        std::this_thread::sleep_for(std::chrono::seconds(2));


    }

    timer.Shutdown();

    return 0;
}
