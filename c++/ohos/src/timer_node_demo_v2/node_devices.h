#ifndef NODE_DEVICES_H
#define NODE_DEVICES_H

#include "node_device.h"
#include "node_handles.h"
#include "node_handle.h"
using namespace OHOS;

class NodeManager {
DISALLOW_COPY_AND_MOVE(NodeManager);

public:
    struct Item {
        DISALLOW_COPY_AND_MOVE(Item);

        NodeDevice dev_;
        NodeHandles handles_;
        Mode defmode_;

        Item(Utils::Timer &timer, const NodeDevice::InitOpts& opt, const Mode& defmode):
            dev_(timer, opt),
            handles_(dev_),
            defmode_(defmode)
        {}

        NodeHandle& createHandle() {
            return handles_.createHandle(defmode_);
        }

        void deleteHandle(NodeHandle& handle) {
            handles_.deleteHandle(handle);
        }
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



    NodeManager(const std::initializer_list<InitOpts>& opts) : timer_("TimerDaemon"), itmes_() {
        timer_.Setup();

        for (InitOpts opt : opts) {
            Item &item = itmes_.emplace_back(timer_, opt.devopts, opt.mode);
        }

    }

    ~NodeManager() {
        timer_.Shutdown();
    }

private:
    Utils::Timer timer_;
    Items itmes_;
};


#endif // NODE_DEVICES_H
