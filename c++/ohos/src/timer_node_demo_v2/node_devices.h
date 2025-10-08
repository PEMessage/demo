#ifndef NODE_DEVICES_H
#define NODE_DEVICES_H

#include "node_device.h"
using namespace OHOS;

class NodeDevices {
DISALLOW_COPY_AND_MOVE(NodeDevices);

public:
    using Item = std::pair<NodeDevice, Mode>;
    using Container = std::list<Item>;

    struct InitOpts {
        NodeDevice::InitOpts devopts; // device init opts
        Mode mode;
    };

    std::vector<std::reference_wrapper<NodeDevice>> allDevices() {
        std::vector<std::reference_wrapper<NodeDevice>> result;
        for (auto& item : container_) {
            result.push_back(std::ref(item.first));
        }
        return result;
    }
    Container& getContainer() { return container_; }

    NodeDevices(const std::initializer_list<InitOpts>& opts) : timer_("TimerDaemon"), container_() {
        timer_.Setup();

        for (InitOpts opt : opts) {
            Item &item = container_.emplace_back(std::piecewise_construct,
                                    std::forward_as_tuple(timer_, opt.devopts),
                                    std::forward_as_tuple(opt.mode)
            );
            item.first.set(opt.mode);
        }

        for (auto dev_ref : allDevices()) {
            NodeDevice &dev = dev_ref.get();
            dev.update();
        }

    }

    ~NodeDevices() {
        timer_.Shutdown();
    }

private:
    Utils::Timer timer_;
    Container container_;
};


#endif // NODE_DEVICES_H
