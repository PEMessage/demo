#ifndef NODE_DEVICE_H
#define NODE_DEVICE_H

#include <memory>
#include <variant>
#include <optional>
#include <string>
#include <vector>
#include <list>
#include <mutex>

#include "timer.h"
#include "nocopyable.h"
#include "misc.h"

using namespace OHOS;

class NodeHandle;

class NodeDevice {
public:
    NodeDevice(std::string path, Utils::Timer &timer);
    ~NodeDevice();
    
    NodeHandle& createNode(const Config& config);
    void deleteNode(NodeHandle &node);
    void applyNode();

private:
    std::recursive_mutex m;
    State state_;
    std::list<NodeHandle> nodes;
    Utils::Timer &timer_;
    std::optional<uint32_t> timerid_;

    void stopTimer();
};

#endif // NODE_DEVICE_H
