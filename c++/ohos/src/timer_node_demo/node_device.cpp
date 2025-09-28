#include "node_device.h"
#include "node_handle.h"
#include <iostream>
#include <tuple>

NodeDevice::NodeDevice(std::string path, Utils::Timer &timer):
    state_(path, false), timer_(timer) {
}

NodeDevice::~NodeDevice() {
    stopTimer();
}

NodeHandle& NodeDevice::createHandle(const Config& config) {
    std::lock_guard<std::recursive_mutex> lk(m);

    NodeHandle& node = nodes.emplace_back(this, config);

    update();
    return node;
}

void NodeDevice::deleteHandle(NodeHandle &node) {
    std::lock_guard<std::recursive_mutex> lk(m);

    auto it = std::find_if(nodes.begin(), nodes.end(),
                      [&](const NodeHandle& n) { return &n == &node; });
    if (it != nodes.end()) {
        nodes.erase(it);
    }

    update();
}

void NodeDevice::update() {
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

void NodeDevice::stopTimer() {
    if(!timerid_) { return; };
    timer_.Unregister(*timerid_);
    timerid_.reset();
}
