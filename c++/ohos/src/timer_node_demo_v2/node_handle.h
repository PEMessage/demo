#ifndef NODE_HANDLE_H
#define NODE_HANDLE_H

#include <array>

#include "nocopyable.h"
#include "misc.h"

class NodeHandles;

class NodeHandle {
DISALLOW_COPY_AND_MOVE(NodeHandle);

public:
    using InitOpts = Mode;
    enum SlotId {
        SYSTEM = 0,
        USER,
        MAX_NUM,
    };

private:
    NodeHandles* handles_;
    std::array<Mode, SlotId::MAX_NUM> modes_;
    SlotId current_;

public:
    NodeHandle(NodeHandles* handles, const InitOpts& opts);
    
    Mode& getCurrentMode();
    Mode& getMode(SlotId id);
    void switchSlot(SlotId id);
    void setMode(SlotId id, const Mode& mode);
};

#endif
