#include "node_handles.h"

NodeHandle& NodeHandles::createHandle(const Mode& defmode) {
    NodeHandle& handle = handles_.emplace_back(this, defmode);

    update();

    return handle;
}

void NodeHandles::deleteHandle(NodeHandle& handle) {
    auto it = std::find_if(handles_.begin(), handles_.end(),
                           [&](const NodeHandle& n) { return &n == &handle; });
    if (it != handles_.end()) {
        handles_.erase(it);
    }

    update();
}

void NodeHandles::update() {
    if (handles_.empty()) {
        dev_.set(Mode{}); // reset to default `Mode{enabled=0,submode=InvalidMode{}}`
        dev_.update();
        return;
    }

    Mode next = handles_.back().getCurrentMode();
    Mode current = dev_.getMode();

    if (current == next) {return;}

    dev_.set(next);
    dev_.update();
}





