#ifndef NODE_HANDLE_H
#define NODE_HANDLE_H

#include <optional>
#include <vector>
#include <array>

#include "misc.h"
#include "nocopyable.h"

class NodeDevice;

class NodeHandle {
DISALLOW_COPY_AND_MOVE(NodeHandle);
friend class NodeDevice;
    
public:
    enum SlotId {
        SYSTEM = 0,
        USER,
        MAX_NUM,
    };

private:
    std::array<Config, SlotId::MAX_NUM> configs_;
    SlotId current_;

    Config& getCurrentConfig();
    Config& getConfig(SlotId id);

public:
    NodeDevice *device_;
    NodeHandle(NodeDevice *device, Config config);

    void light(SlotId id = SlotId::SYSTEM);
    void dark(SlotId id = SlotId::SYSTEM);
    void blink(uint32_t interval, SlotId id = SlotId::SYSTEM);
};

#endif // NODE_HANDLE_H
