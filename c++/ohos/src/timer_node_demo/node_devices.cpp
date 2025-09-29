#include "node_device.h"
#include "node_handle.h"
#include "misc.h"
#include "timer.h"
#include <vector>

using namespace OHOS;



class NodeDevices {
DISALLOW_COPY_AND_MOVE(NodeDevices);



public:
    using Container = std::list<std::pair<NodeDevice, NodeHandle::InitOpts>>;

    struct InitOpts {
        NodeDevice::InitOpts devopts; // device init opts
        NodeHandle::InitOpts hopts; // handle init opts
    };

    NodeDevices(const std::initializer_list<InitOpts>& opts) : timer_("TimerDaemon"), container_() {
        for (InitOpts opt : opts) {
            container_.emplace_back(std::piecewise_construct,
                                    std::forward_as_tuple(timer_, opt.devopts),
                                    std::forward_as_tuple(opt.hopts)
            );

        }
    }
    Container& getContainer() { return container_; }

private:
    Utils::Timer timer_;
    Container container_;
};

class NodeHandles {

public:
    NodeHandles(NodeDevices& devices) : container_() {
        for (auto& [device, opts]  : devices.getContainer()) {
            container_.emplace_back(& device.createHandle(opts));
        }
    }

    ~NodeHandles() {
        for (NodeHandle *handle : container_) {
            handle->device_->deleteHandle(*handle);
        }
    }
private:
    std::vector<NodeHandle *> container_;
};
