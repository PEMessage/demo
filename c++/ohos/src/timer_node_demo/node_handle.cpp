#include "node_handle.h"
#include "node_device.h"

NodeHandle::NodeHandle(NodeDevice *device, Config config): device_(device),
    config_(config), savedconfig_() {}

void NodeHandle::save(Config config) {
    if (!savedconfig_) {
        savedconfig_ = config_;
    }
    config_ = config;

    device_->update();
}


void NodeHandle::save() {
    save(config_);
}


void NodeHandle::restore() {
    if (savedconfig_) {
        config_ = *savedconfig_;
        savedconfig_.reset();
    }

    device_->update();
}

Config& NodeHandle::getConfig(bool isSystem) {
    if (isSystem && savedconfig_) {
        return *savedconfig_;
    } else {
        return config_;
    }
}

void NodeHandle::light(bool isSystem) {
    Config next = Config {
        .mode = SwitchMode{true}
    };
    Config& current = getConfig(isSystem);

    if (next == current) { return; }
    current = next;
    
    device_->update();
}

void NodeHandle::dark(bool isSystem) {
    Config next = Config {
        .mode = SwitchMode{false}
    };
    Config& current = getConfig(isSystem);

    if (next == current) { return; }
    current = next;

    device_->update();
}

void NodeHandle::blink(uint32_t interval, bool isSystem) {
    Config next = Config {
        .mode = BlinkMode{interval}
    };
    Config& current = getConfig(isSystem);

    if (next == current) { return; }
    current = next;

    device_->update();
}
