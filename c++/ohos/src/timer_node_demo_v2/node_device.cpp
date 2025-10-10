#include "node_device.h"

namespace Node {

void NodeDevice::stopTimer() {
    if(!timerid_) { return; };
    timer_.Unregister(*timerid_);
    timerid_.reset();
}

NodeDevice::NodeDevice(Utils::Timer &timer, InitOpts opts)
    : name_(opts.name),
      m(),
      state_(opts.path, false, opts.enabled),
      mode_(Mode{.enabled = true, .submode = InvalidMode{}}),
      timer_(timer),
      timerid_(),
      callback_()
{}

NodeDevice::~NodeDevice() {
    stopTimer();
}

Mode& NodeDevice::getMode() {
    return mode_;
}

void NodeDevice::setMode(Mode mode) {
    mode_ = mode;
}

void NodeDevice::update() {
    std::lock_guard<std::recursive_mutex> lk(m);

    // std::cout << mode_ << std::endl;
    if (mode_.enabled == false) {
        stopTimer();
        state_.write(false);
        return;
    }

    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ConstMode>) {
            stopTimer();
            state_.write(true);
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
    }, mode_.submode);


}

} // namespace Node