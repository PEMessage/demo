#ifndef NODE_DEVICE_H
#define NODE_DEVICE_H

#include <mutex>
#include <memory>

#include "timer.h"
#include "nocopyable.h"
#include "misc.h"


namespace Node {

using namespace OHOS;

class NodeDevice {
    DISALLOW_COPY(NodeDevice);

public: 
    const std::string name_;

private:
    std::recursive_mutex m;

    State state_;
    Mode mode_;

    Utils::Timer &timer_;
    std::optional<uint32_t> timerid_;
    std::function<void()> callback_;
    void stopTimer();

public:
    struct InitOpts {
        std::string name;
        std::string path;
        bool enabled;
    };

    NodeDevice(Utils::Timer &timer, InitOpts opts);
    ~NodeDevice();

    Mode& getMode();
    void setMode(Mode mode);
    void update();
};

} // namespace Node

#endif // NODE_DEVICE_H