#ifndef NODE_H
#define NODE_H

#include "misc.h"
#include "nocopyable.h"
#include <optional>

class NodeManager;

class Node {
    DISALLOW_COPY_AND_MOVE(Node);
    friend class NodeManager;
    
private:
    Config config_;
    std::optional<Config> savedconfig_;
    NodeManager *manager_;

public:
    Node(NodeManager *manager, Config config);
    
    void save(Config config);
    void restore(Config config);
    void light();
    void dark();
    void blink(uint32_t interval);
};

#endif // NODE_H
