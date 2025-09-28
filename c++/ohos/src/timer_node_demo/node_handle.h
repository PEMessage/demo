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
    NodeDevice *manager_;

public:
    NodeHandle(NodeDevice *manager, Config config);
    
    void save(Config config);
    void restore(Config config);
    void light();
    void dark();
    void blink(uint32_t interval);
};

#endif // NODE_HANDLE_H
