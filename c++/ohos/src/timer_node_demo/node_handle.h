#ifndef NODE_HANDLE_H
#define NODE_HANDLE_H

#include "misc.h"
#include "nocopyable.h"
#include <optional>

class NodeDevice;

class NodeHandle {
    DISALLOW_COPY_AND_MOVE(NodeHandle);
    friend class NodeDevice;
    
private:
    Config config_;
    std::optional<Config> savedconfig_;

    Config& getConfig(bool isSystem);

public:
    NodeDevice *device_;
    NodeHandle(NodeDevice *device, Config config);
    
    void save(Config config);
    void save();
    void restore();
    void light(bool isSystem = false);
    void dark(bool isSystem = false);
    void blink(uint32_t interval, bool isSystem = false);
};

#endif // NODE_HANDLE_H
