#include "node_manager.h"
#include "node.h"
#include <iostream>
#include <tuple>

NodeManager::NodeManager(std::string path, Utils::Timer &timer):
    state_(path, false), timer_(timer) {
}

NodeManager::~NodeManager() {
    stopTimer();
}

Node& NodeManager::createNode(const Config& config) {
   Node& node = nodes.emplace_back(this, config);
   return node;
}

void NodeManager::deleteNode(Node &node) {
    auto it = std::find_if(nodes.begin(), nodes.end(),
                      [&](const Node& n) { return &n == &node; });
    if (it != nodes.end()) {
        nodes.erase(it);
    }
}

void NodeManager::applyNode() {
    std::lock_guard<std::recursive_mutex> lk(m);

    if (nodes.empty()) {
        return;
    }

    Config &config = nodes.back().config_;

    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, SwitchMode>) {
            stopTimer();
            state_.write(arg.isOn);
        } else if constexpr (std::is_same_v<T, BlinkMode>) {
            stopTimer();
            timerid_ = timer_.Register([&]() {
                std::lock_guard<std::recursive_mutex> lk(m);
                state_.toggle();
            }, arg.interval, false);
        } else {
            std::cout << "Unknow Mode" << std::endl;
        }
    }, config.mode);
}

void NodeManager::stopTimer() {
    if(!timerid_) { return; };
    timer_.Unregister(*timerid_);
    timerid_.reset();
}
