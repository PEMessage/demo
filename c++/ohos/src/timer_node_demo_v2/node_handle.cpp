#include "node_handle.h"
#include "node_handles.h"

NodeHandle::NodeHandle(NodeHandles* handles, const InitOpts& opts)
    : handles_(handles), modes_(), current_(SlotId::SYSTEM)
{
    const Mode& mode = opts; // at now, Config same as InitOpts
    modes_[current_] = mode;
}

Mode& NodeHandle::getCurrentMode() {
    // std::cout << "Inside Curr: " <<  current_ <<  modes_[current_] << std::endl;
    return modes_[current_];
}

Mode& NodeHandle::getMode(SlotId id) {
    return modes_[id];
}

void NodeHandle::switchSlot(SlotId id) {
    current_ = id;
    // std::cout << "Switch Curr" <<  getCurrentMode() << std::endl;
    handles_->update();

}

void NodeHandle::setMode(SlotId id, const Mode& mode) {
    modes_[id] = mode;
    handles_->update();
}


NodeHandles* NodeHandle::getHandles() {
    return handles_;
}
