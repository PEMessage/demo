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
    cout << "==== testNodePreemptive" << endl;
    Utils::Timer timer("Timer");
    timer.Setup();

    {
        NodeDevice device(timer, "MockPath");

        cout << "Now create handle1, with blink mode" << endl;
        NodeHandle& node1 = device.createHandle(Config{true,BlinkMode{200}});
        std::this_thread::sleep_for(std::chrono::seconds(1));

        cout << "Now create handle2, with const mode" << endl;
        NodeHandle& node2 = device.createHandle(Config{true, ConstMode{}});
        std::this_thread::sleep_for(std::chrono::seconds(1));
        assert(node2.device_->read() == true);

        cout << "Now create handle3, with duty mode" << endl;
        NodeHandle& node3 = device.createHandle(Config{true, DutyMode{500, 70}});
        std::this_thread::sleep_for(std::chrono::seconds(3));

        cout << "Now delete handle2, should still handle3" << endl;
        device.deleteHandle(node2);
        std::this_thread::sleep_for(std::chrono::seconds(3));

        cout << "Now delete handle3, should be handle1 blink mode" << endl;
        device.deleteHandle(node3);
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    timer.Shutdown();

    return 0;
}

void testConfigSaveRestore() {
    cout << "==== testConfigSaveRestore" << endl;
    Utils::Timer timer("Timer");
    timer.Setup();
    {
        // use `watch --interval 0.1 cat MockPath`
        NodeDevice device(timer, "MockPath");

        cout << "Now use blink to init handle" << endl;
        NodeHandle& node1 = device.createHandle(Config{true,BlinkMode{500}});
        std::this_thread::sleep_for(std::chrono::seconds(3));

        cout << "Now dark handle" << endl;
        node1.dark();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        cout << "Now light handle" << endl;
        node1.light();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        cout << "Now dark handle, and assert" << endl;
        node1.dark();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        assert(node1.device_->read() == false);

        cout << "Now using user: set user light, and system dark" << endl;
        node1.light(NodeHandle::USER);
        node1.switchSlot(NodeHandle::USER);
        node1.dark();
        assert(node1.device_->read() == true);

        cout << "Now using system" << endl;
        node1.switchSlot(NodeHandle::SYSTEM);
        assert(node1.device_->read() == false);
    }
    timer.Shutdown();
}

int main() {
    // testConfigSaveRestore();
    testNodePreemptive();
}
