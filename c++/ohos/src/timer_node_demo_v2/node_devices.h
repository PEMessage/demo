#ifndef NODE_DEVICES_H
#define NODE_DEVICES_H

#include "node_device.h"
using namespace OHOS;

class NodeDevices {
DISALLOW_COPY_AND_MOVE(NodeDevices);

public:
    using Container = std::list<std::pair<NodeDevice, bool>>;

    struct InitOpts {
        NodeDevice::InitOpts devopts; // device init opts
        bool reserved;
    };

    NodeDevices(const std::initializer_list<InitOpts>& opts) : timer_("TimerDaemon"), container_() {
        for (InitOpts opt : opts) {
            container_.emplace_back(std::piecewise_construct,
                                    std::forward_as_tuple(timer_, opt.devopts),
                                    std::forward_as_tuple(opt.reserved)
            );

        }
    }
    Container& getContainer() { return container_; }

private:
    Utils::Timer timer_;
    Container container_;
};


#endif // NODE_DEVICES_H
