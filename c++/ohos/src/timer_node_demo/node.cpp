#include "node.h"
#include "node_manager.h"

Node::Node(NodeManager *manager, Config config): manager_(manager), config_(config) {}

void Node::save(Config config) {
    if (!savedconfig_) {
        savedconfig_ = config_;
    }
    config_ = config;
}

void Node::restore(Config config) {
    if (savedconfig_) {
        config_ = *savedconfig_;
        savedconfig_.reset();
    }
}

void Node::light() {
    Config expected = Config {
        .mode = SwitchMode{true}
    };

    if (expected == config_) { return; }
    manager_->applyNode();
}

void Node::dark() {
    Config expected = Config {
        .mode = SwitchMode{false}
    };

    if (expected == config_) { return; }
    manager_->applyNode();
}

void Node::blink(uint32_t interval) {
    Config expected = Config {
        .mode = BlinkMode{interval}
    };
    if (expected == config_) { return; }
    manager_->applyNode();
}
