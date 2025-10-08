#ifndef NODE_HANDLES_H
#define NODE_HANDLES_H

#include "node_device.h"
#include "node_handle.h"

class NodeHandles {
DISALLOW_COPY_AND_MOVE(NodeHandles);
private:
    NodeDevice& dev_;
    std::list<NodeHandle> handles_;
public:
    NodeHandles(NodeDevice& dev):
        dev_(dev),
        handles_()
    {}

    NodeHandle& createHandle(const Mode& defmode);
    void deleteHandle(NodeHandle &node);
    void update();
};


#endif
