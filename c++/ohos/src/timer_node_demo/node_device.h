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
    struct InitOpts {
        std::string path;
        bool enabled;
    };
    NodeDevice(Utils::Timer &timer, const InitOpts& opts) : NodeDevice(timer, opts.path, opts.enabled) {};
    NodeDevice(Utils::Timer &timer, const std::string& path, bool enabled = true);
    ~NodeDevice();

    NodeHandle& createHandle(const Config& handle_defconfig);
    void deleteHandle(NodeHandle &node);
    void update();
    bool read();

private:
    State state_;
    Utils::Timer &timer_;

    std::recursive_mutex m;
    std::list<NodeHandle> nodes;
    std::optional<uint32_t> timerid_;
    std::function<void()> callback_;

    void stopTimer();
};

#endif // NODE_DEVICE_H
