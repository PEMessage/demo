#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>

#include "misc.h"
#include "node_device.h"
#include "node_manager.h"
#include "node_handles.h"
#include "timer.h"

using namespace std;
using namespace OHOS;
using namespace Node;

int main();

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

    Node::NodeDevice dev {timer, Node::NodeDevice::InitOpts{.name = "MockName" , .path = "MockPath", .enabled = false }};
    {
        dev.setMode(Node::Mode{.enabled = true, .submode = Node::BlinkMode{.interval = 500}}); 
        dev.update();
        std::this_thread::sleep_for(std::chrono::seconds(5));

        dev.setMode(Node::Mode{.enabled = true, .submode = Node::DutyMode{.interval = 300, .duty = 90}}); 
        dev.update();
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    timer.Shutdown();
}

void testHandles() {
    logger << "==== testHandles" << endl;
    Utils::Timer timer("Timer");
    timer.Setup();

    Node::NodeDevice dev {timer, Node::NodeDevice::InitOpts{.name = "MockName" , .path = "MockPath", .enabled = false }};
    {
        logger << "Handle init with blink" << endl;
        Node::NodeHandles handles {dev};
        Node::NodeHandle& handle1 = handles.createHandle(Node::Mode{.enabled = true, .submode = Node::BlinkMode{.interval = 500}});
        std::this_thread::sleep_for(std::chrono::seconds(3));

        logger << "Handle set user slot to Const" << endl;
        handle1.setMode(Node::NodeHandle::USER, Node::Mode{.enabled = true, .submode = Node::ConstMode{}});
        std::this_thread::sleep_for(std::chrono::seconds(2));

        logger << "Handle switch Slot" << endl;
        handle1.switchSlot(Node::NodeHandle::USER);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        logger << "Handle enable, should no change" << endl;
        handle1.enable(Node::NodeHandle::USER);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        logger << "Handle disable System, should do nothing" << endl;
        handle1.disable(Node::NodeHandle::SYSTEM);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        logger << "Handle switch System, should do disable blink" << endl;
        handle1.switchSlot(Node::NodeHandle::SYSTEM);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        logger << "Handle enable System, should do enable blink" << endl;
        handle1.enable(Node::NodeHandle::SYSTEM);
        std::this_thread::sleep_for(std::chrono::seconds(3));

        logger << "Handle delete" << endl;
        handles.deleteHandle(handle1);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    timer.Shutdown();
}

void testDevices() {
    logger << "==== testDevices" << endl;
    Node::NodeManager devs {
        Node::NodeManager::InitOpts{
            .devopts = {"MockName1", "MockPath1", false},
            .mode = Node::Mode{true, Node::BlinkMode{.interval = 500}}
        },
        Node::NodeManager::InitOpts{
            .devopts = {"MockName2", "MockPath2", false},
            .mode = Node::Mode{true, Node::DutyMode{.interval = 500, .duty = 90}}
        },
    };
    std::this_thread::sleep_for(std::chrono::seconds(1));

    logger << "ConstMode on all devs" << endl;
    for (auto dev_ref : devs.allDevices()) {
        Node::NodeDevice &dev = dev_ref.get();
        dev.setMode(Node::Mode{true, Node::ConstMode{}});
        dev.update();
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));


    logger << "ConstMode off all devs" << endl;
    for (auto dev_ref : devs.allDevices()) {
        Node::NodeDevice &dev = dev_ref.get();
        dev.setMode(Node::Mode{false, Node::ConstMode{}});
        dev.update();
    }

    logger << "Create handles group" << endl;
    std::list<std::reference_wrapper<Node::NodeHandle>> handles_group1 {};
    for (auto& item : devs.allItems()) {
        handles_group1.emplace_back(item.createHandle());
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));

    logger << "Only Keep MockName1, and disable another" << endl;
    for (auto handle : handles_group1) {
        if (handle.get().name() == "MockName1") {
            handle.get().enable(Node::NodeHandle::SYSTEM);
        } else {
            handle.get().disable(Node::NodeHandle::SYSTEM);
        }
    }
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

std::initializer_list<Node::NodeManager::InitOpts> generateLightOpts() {
    // std::initializer_list is not value type, Do not return on stack one
    auto res =  std::initializer_list<Node::NodeManager::InitOpts>{
        Node::NodeManager::InitOpts{
            .devopts = {"MockName1", "MockPath1", false},
            .mode = Node::Mode{true, Node::BlinkMode{.interval = 500}}
        },
    };
    return res;
}

void testErrorInitWay() {
    Node::NodeManager devs(generateLightOpts()); // THIS WILL CASUSE ERROR
}


int main() {
    testDevice();
    testHandles();
    testDevices();
    // testErrorInitWay();
    return 0;
}
