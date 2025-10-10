#ifndef NODE_MANAGER_H
#define NODE_MANAGER_H

#include "node_device.h"
#include "node_handles.h"
#include "node_handle.h"
#include "misc.h"

namespace Node {

using namespace OHOS;

class NodeManager {
DISALLOW_COPY_AND_MOVE(NodeManager);

public:
    struct Item {
        DISALLOW_COPY_AND_MOVE(Item);

        NodeDevice dev_;
        NodeHandles handles_;
        Mode defmode_;

        Item(Utils::Timer &timer, const NodeDevice::InitOpts& opt, const Mode& defmode);

        NodeHandle& createHandle();

        void deleteHandle(NodeHandle& handle);
    };



    using Items = std::list<Item>;

    struct InitOpts {
        NodeDevice::InitOpts devopts; // device init opts
        Mode mode;
    };

    std::vector<std::reference_wrapper<NodeDevice>> allDevices();
    Items& allItems();



    NodeManager(const std::initializer_list<InitOpts>& opts);

    ~NodeManager();

private:
    Utils::Timer timer_;
    Items items_;
};

} // namespace Node

#endif // NODE_DEVICES_H
