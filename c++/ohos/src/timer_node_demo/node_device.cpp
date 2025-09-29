#include "node_device.h"
#include "node_handle.h"
#include <iostream>
#include <tuple>

NodeDevice::NodeDevice(const std::string& path, Utils::Timer &timer):
    state_(path, false), timer_(timer) {
}

NodeDevice::~NodeDevice() {
    stopTimer();
}

NodeHandle& NodeDevice::createHandle(const Config& handle_defconfig) {
    std::lock_guard<std::recursive_mutex> lk(m);

    NodeHandle& node = nodes.emplace_back(this, handle_defconfig);

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


bool NodeDevice::read() {
    std::lock_guard<std::recursive_mutex> lk(m);
    return state_.read();
}

void NodeDevice::update() {
    std::lock_guard<std::recursive_mutex> lk(m);

    if (nodes.empty()) {
        return;
    }

    Config &config = nodes.back().getCurrentConfig();

    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, SwitchMode>) {
            stopTimer();
            state_.write(arg.isOn);
        } else if constexpr (std::is_same_v<T, BlinkMode>) {
            stopTimer();
            callback_ = [&]() {
                std::lock_guard<std::recursive_mutex> lk(m);
                // Unregister not ensure that happen after all callback, extra check is need.
                // See: https://gitee.com/openharmony/commonlibrary_c_utils/blob/master/docs/zh-cn/c_utils_timer.md#典型案例
                if(!timerid_) { return; }

                state_.toggle();
            };
            timerid_ = timer_.Register(callback_, arg.interval, false);
        } else if constexpr (std::is_same_v<T, DutyMode>) {
            stopTimer();
            uint32_t first_interval = arg.interval * arg.duty / 100;
            uint32_t second_interval = arg.interval - first_interval;
            callback_ = [&, first_interval, second_interval]() mutable {
                std::lock_guard<std::recursive_mutex> lk(m);
                if(!timerid_) { return; }

                state_.toggle();

                stopTimer();
                if(state_.read()) {
                    timerid_ = timer_.Register(callback_, first_interval, true);
                } else {
                    timerid_ = timer_.Register(callback_, second_interval, true);
                }
            };
            timerid_ = timer_.Register(callback_, first_interval, true); // first kick
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
