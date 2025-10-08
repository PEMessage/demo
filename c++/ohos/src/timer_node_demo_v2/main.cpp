#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>

#include "misc.h"
#include "node_device.h"
#include "node_devices.h"
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



void testDevice() {
    logger << "==== testDevice" << endl;
    Utils::Timer timer("Timer");
    timer.Setup();

    NodeDevice dev {timer, NodeDevice::InitOpts{.name = "MockName" , .path = "MockPath", .enabled = false }};
    {
        dev.set(Mode{.enabled = true, .submode = BlinkMode{.interval = 500}}); 
        dev.update();
        std::this_thread::sleep_for(std::chrono::seconds(5));

        dev.set(Mode{.enabled = true, .submode = DutyMode{.interval = 300, .duty = 90}}); 
        dev.update();
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    timer.Shutdown();
}

void testDevices() {
    logger << "==== testDevices" << endl;
    NodeDevices devs {
        NodeDevices::InitOpts{
            .devopts = {"MockName1", "MockPath1", false},
            .mode = Mode{true, BlinkMode{.interval = 500}}
        },
        NodeDevices::InitOpts{
            .devopts = {"MockName2", "MockPath2", false},
            .mode = Mode{true, DutyMode{.interval = 500, .duty = 90}}
        },
    };
    std::this_thread::sleep_for(std::chrono::seconds(3));

    logger << "ConstMode on all devs" << endl;
    for (auto dev_ref : devs.allDevices()) {
        NodeDevice &dev = dev_ref.get();
        dev.set(Mode{true, ConstMode{}});
        dev.update();
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));


    logger << "ConstMode off all devs" << endl;
    for (auto dev_ref : devs.allDevices()) {
        NodeDevice &dev = dev_ref.get();
        dev.set(Mode{false, ConstMode{}});
        dev.update();
    }

}

int main() {
    // testDevice();
    testDevices();
}
