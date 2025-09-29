#include "node_handle.h"
#include "node_device.h"
#include <assert.h>

NodeHandle::NodeHandle(NodeDevice *device, Config config): device_(device),
    configs_(), current_(SlotId::SYSTEM) {
    configs_[SlotId::SYSTEM] = config;
}

Config& NodeHandle::getConfig(SlotId id) { return configs_[id]; }
Config& NodeHandle::getCurrentConfig() {
    Config& current = getConfig(current_);
    if (current.mode != Mode{InvalidMode{}}) {
        return current;
    } else {
        return getConfig(SlotId::SYSTEM);
    }
}

void NodeHandle::switchSlot(SlotId id) {
    current_ = id;
    device_->update();
}


void NodeHandle::light(SlotId id) {
    Config next = Config {
        .enabled = true,
        .mode = ConstMode{},
    };
    Config& current = getConfig(id);

    if (next == current) { return; }
    current = next;
    
    device_->update();
}

void NodeHandle::dark(SlotId id) {
    Config next = Config {
        .enabled = false,
        .mode = ConstMode{},
    };
    Config& current = getConfig(id);

    if (next == current) { return; }
    current = next;

    device_->update();
}

void NodeHandle::blink(uint32_t interval, SlotId id) {
    Config next = Config {
        .enabled = true,
        .mode = BlinkMode{interval}
    };
    Config& current = getConfig(id);

    if (next == current) { return; }
    current = next;

    device_->update();
}
