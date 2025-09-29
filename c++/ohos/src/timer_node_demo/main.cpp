#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>

#include "misc.h"
#include "node_handle.h"
#include "node_device.h"
#include "timer.h"
using namespace std;

int testNodePreemptive() {
    // Create a timer for the NodeDevice
    Utils::Timer timer("Timer");
    timer.Setup();

    {
        NodeDevice device("MockPath", timer);

        // Create different nodes with different configurations
        NodeHandle& node1 = device.createHandle(Config{true,BlinkMode{200}});
        std::this_thread::sleep_for(std::chrono::seconds(1));

        NodeHandle& node2 = device.createHandle(Config{true, ConstMode{}});
        std::this_thread::sleep_for(std::chrono::seconds(1));
        assert(node2.device_->read() == true);

        NodeHandle& node3 = device.createHandle(Config{true, DutyMode{500, 70}});
        std::this_thread::sleep_for(std::chrono::seconds(3));

        device.deleteHandle(node2);
        std::this_thread::sleep_for(std::chrono::seconds(3));

        device.deleteHandle(node3);
        std::this_thread::sleep_for(std::chrono::seconds(3));

        node1.dark();
        std::this_thread::sleep_for(std::chrono::seconds(3));

        node1.light();
        std::this_thread::sleep_for(std::chrono::seconds(3));

        node1.blink(300);
        std::this_thread::sleep_for(std::chrono::seconds(3));

    }

    timer.Shutdown();

    return 0;
}

void testConfigSaveRestore() {
    Utils::Timer timer("Timer");
    timer.Setup();
    {
        // use `watch --interval 0.1 cat MockPath`
        NodeDevice device("MockPath", timer);

        NodeHandle& node1 = device.createHandle(Config{true,BlinkMode{500}});
        std::this_thread::sleep_for(std::chrono::seconds(3));

        node1.dark();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        node1.light();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        node1.dark();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        assert(node1.device_->read() == false);

        cout << "Now using user" << endl;
        node1.light(NodeHandle::USER);
        node1.switchSlot(NodeHandle::USER);
        node1.dark();
        assert(node1.device_->read() == true);

        node1.switchSlot(NodeHandle::SYSTEM);
        assert(node1.device_->read() == false);
    }
    timer.Shutdown();
}

int main() {
    testConfigSaveRestore();
    // testNodePreemptive();
}
