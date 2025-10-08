#ifndef NODE_DEVICES_H
#define NODE_DEVICES_H

#include "node_device.h"
#include "node_handles.h"
#include "node_handle.h"
using namespace OHOS;

class NodeDevices {
DISALLOW_COPY_AND_MOVE(NodeDevices);

public:
    struct Item {
        NodeDevice dev_;
        NodeHandles handles_;
        Mode defmode_;

        Item(Utils::Timer &timer, NodeDevice::InitOpts opt, Mode defmode):
            dev_(timer, opt),
            handles_(dev_),
            defmode_(defmode)
        {}
    };

    using Items = std::list<Item>;

    struct InitOpts {
        NodeDevice::InitOpts devopts; // device init opts
        Mode mode;
    };

    std::vector<std::reference_wrapper<NodeDevice>> allDevices() {
        std::vector<std::reference_wrapper<NodeDevice>> result;
        for (auto& item : itmes_) {
            result.push_back(std::ref(item.dev_));
        }
        return result;
    }

    Items& allItems() { return itmes_; }

    NodeDevices(const std::initializer_list<InitOpts>& opts) : timer_("TimerDaemon"), itmes_() {
        timer_.Setup();

        for (InitOpts opt : opts) {
            Item &item = itmes_.emplace_back(timer_, opt.devopts, opt.mode);
        }

    }

    ~NodeDevices() {
        timer_.Shutdown();
    }

private:
    Utils::Timer timer_;
    Items itmes_;
};


#endif // NODE_DEVICES_H
