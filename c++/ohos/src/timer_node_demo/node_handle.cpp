#include "node_handle.h"
#include "node_device.h"

NodeHandle::NodeHandle(NodeDevice *manager, Config config): manager_(manager), config_(config) {}

void NodeHandle::save(Config config) {
    if (!savedconfig_) {
        savedconfig_ = config_;
    }
    config_ = config;
}

void NodeHandle::restore(Config config) {
    if (savedconfig_) {
        config_ = *savedconfig_;
        savedconfig_.reset();
    }
}

void NodeHandle::light() {
    Config expected = Config {
        .mode = SwitchMode{true}
    };

    if (expected == config_) { return; }
    manager_->applyNode();
}

void NodeHandle::dark() {
    Config expected = Config {
        .mode = SwitchMode{false}
    };

    if (expected == config_) { return; }
    manager_->applyNode();
}

void NodeHandle::blink(uint32_t interval) {
    Config expected = Config {
        .mode = BlinkMode{interval}
    };
    if (expected == config_) { return; }
    manager_->applyNode();
}
