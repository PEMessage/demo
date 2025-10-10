#include "node_handle.h"
#include "node_manager.h"

namespace Node {

NodeManager::Item::Item(Utils::Timer &timer, const NodeDevice::InitOpts& opt, const Mode& defmode)
    : dev_(timer, opt),
      handles_(dev_),
      defmode_(defmode)
{}

NodeHandle& NodeManager::Item::createHandle() {
    return handles_.createHandle(defmode_);
}

void NodeManager::Item::deleteHandle(NodeHandle& handle) {
    handles_.deleteHandle(handle);
}

std::vector<std::reference_wrapper<NodeDevice>> NodeManager::allDevices() {
    std::vector<std::reference_wrapper<NodeDevice>> result;
    for (auto& item : items_) {
        result.push_back(std::ref(item.dev_));
    }
    return result;
}

NodeManager::Items& NodeManager::allItems() { 
    return items_; 
}

NodeManager::NodeManager(const std::initializer_list<InitOpts>& opts) : timer_("TimerDaemon"), items_() {
    timer_.Setup();

    for (InitOpts opt : opts) {
        Item &item = items_.emplace_back(timer_, opt.devopts, opt.mode);
    }

}

NodeManager::~NodeManager() {
    timer_.Shutdown();
}

} // namespace Node
