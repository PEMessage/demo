#include "node_handle.h"
#include "node_device.h"
#include <assert.h>

NodeHandle::NodeHandle(NodeDevice *device, Config config): device_(device),
    config_(config), savedconfig_() {}

void NodeHandle::save(Config config) {
    if (!savedconfig_) {
        savedconfig_ = config_;
    } else {
        assert(false); // double save
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
    } else {
        assert(false); // double restore
    }

    device_->update();
}

Config& NodeHandle::getConfig(bool isUser) {
    if (!isUser && savedconfig_) {
        return *savedconfig_;
    } else {
        return config_;
    }
}

void NodeHandle::light(bool isUser) {
    Config next = Config {
        .mode = SwitchMode{true}
    };
    Config& current = getConfig(isUser);

    if (next == current) { return; }
    current = next;
    
    device_->update();
}

void NodeHandle::dark(bool isUser) {
    Config next = Config {
        .mode = SwitchMode{false}
    };
    Config& current = getConfig(isUser);

    if (next == current) { return; }
    current = next;

    device_->update();
}

void NodeHandle::blink(uint32_t interval, bool isUser) {
    Config next = Config {
        .mode = BlinkMode{interval}
    };
    Config& current = getConfig(isUser);

    if (next == current) { return; }
    current = next;

    device_->update();
}
