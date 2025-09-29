#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>

#include "misc.h"
#include "node_handle.h"
#include "node_device.h"
#include "timer.h"
using namespace std;

class LogStream {
private:
    bool after_endl;
public:
    LogStream() : after_endl(true) {}
    template<typename T>
    LogStream& operator<<(const T& value) {
        if (after_endl) { cout << "\033[0m" "\033[1;34m" << "---- " ; }

        after_endl = false;
        cout << value;
        return *this;
    }
    LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
        if (after_endl) { cout << "\033[0m" "\033[1;34m" << "---- " ; }
        manip(cout);
        if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::endl)) {
            after_endl = true;
            cout << "\033[0m";
        } else {
            after_endl = false;
        }
        return *this;
    }
};

LogStream logger;

int testNodePreemptive() {
    logger << "===================" << endl;
    logger << " testNodePreemptive" << endl;
    logger << "===================" << endl;
    Utils::Timer timer("Timer");
    timer.Setup();

    {
        NodeDevice device(timer, "MockPath");

        logger << "Now create handle1, with blink mode" << endl;
        NodeHandle& node1 = device.createHandle(Config{true,BlinkMode{200}});
        std::this_thread::sleep_for(std::chrono::seconds(1));

        logger << "Now create handle2, with const mode" << endl;
        NodeHandle& node2 = device.createHandle(Config{true, ConstMode{}});
        std::this_thread::sleep_for(std::chrono::seconds(1));
        assert(node2.device_->read() == true);

        logger << "Now create handle3, with duty mode" << endl;
        NodeHandle& node3 = device.createHandle(Config{true, DutyMode{500, 70}});
        std::this_thread::sleep_for(std::chrono::seconds(3));

        logger << "Now delete handle2, should still handle3" << endl;
        device.deleteHandle(node2);
        std::this_thread::sleep_for(std::chrono::seconds(3));

        logger << "Now delete handle3, should be handle1 blink mode" << endl;
        device.deleteHandle(node3);
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    timer.Shutdown();

    return 0;
}

void testConfigSaveRestore() {
    logger << "==== testConfigSaveRestore" << endl;
    Utils::Timer timer("Timer");
    timer.Setup();
    {
        // use `watch --interval 0.1 cat MockPath`
        NodeDevice device(timer, "MockPath");

        logger << "Now use blink to init handle" << endl;
        NodeHandle& node1 = device.createHandle(Config{true,BlinkMode{500}});
        std::this_thread::sleep_for(std::chrono::seconds(3));

        logger << "Now dark handle" << endl;
        node1.dark();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        logger << "Now light handle" << endl;
        node1.light();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        logger << "Now dark handle, and assert" << endl;
        node1.dark();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        assert(node1.device_->read() == false);

        logger << "Now using user: set user light, and system dark" << endl;
        node1.light(NodeHandle::USER);
        node1.switchSlot(NodeHandle::USER);
        node1.dark();
        assert(node1.device_->read() == true);

        logger << "Now using system" << endl;
        node1.switchSlot(NodeHandle::SYSTEM);
        assert(node1.device_->read() == false);
    }
    timer.Shutdown();
}

int main() {
    // testConfigSaveRestore();
    testNodePreemptive();
}
