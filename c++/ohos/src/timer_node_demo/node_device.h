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
DISALLOW_COPY_AND_MOVE(NodeDevice);
public:
    NodeDevice(const std::string& path, Utils::Timer &timer);
    ~NodeDevice();
    
    NodeHandle& createHandle(const Config& handle_defconfig);
    void deleteHandle(NodeHandle &node);
    void update();
    bool read();

private:
    std::recursive_mutex m;
    State state_;
    std::list<NodeHandle> nodes;
    Utils::Timer &timer_;
    std::optional<uint32_t> timerid_;

    std::function<void()> callback_;

    void stopTimer();
};

#endif // NODE_DEVICE_H
