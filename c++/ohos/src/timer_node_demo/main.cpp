#include <algorithm>
#include <memory>
#include <variant>
#include <optional>
#include <string>
#include <vector>
#include <fstream>
#include <stack>
#include <iostream>

#include "timer.h"
#include "nocopyable.h"

using namespace OHOS;

class NodeManager;

struct SwitchMode {
    bool isOn = false;
    
    bool operator==(const SwitchMode& other) const {
        return isOn == other.isOn;
    }
};

struct BlinkMode {
    uint32_t interval = 0;
    
    bool operator==(const BlinkMode& other) const {
        return interval == other.interval;
    }
};

using Mode = std::variant<SwitchMode, BlinkMode>;

struct Config {
    Mode mode;

    bool operator==(const Config& other) const {
        return mode == other.mode;
    }
};


struct State {
    std::string path_;
    std::ofstream file_;

    bool on_;

    State(std::string path, bool on) : path_(path), file_(path), on_(on) {}

    void write() {
        if (file_.is_open()) {
            file_ << (on_ ? "1": "0") << std::endl;
        } else {
            std::cout << path_ << ":" << on_ << std::endl;
        }
    }
    void write(bool on) { 
        on_ = on;
        write();
    }
    void toggle() {
        on_ = !on_;
        write();
    }
    bool read() { return on_; }
};


class Node {
    DISALLOW_COPY_AND_MOVE(Node);
    friend class NodeManager;
private:
    Config  config_;
    std::optional<Config>  savedconfig_;

    NodeManager *manager_;

public:
    Node(NodeManager * manager, Config config): manager_(manager) , config_(config) {};

    void save(Config config) {
        if (!savedconfig_) {
            savedconfig_ = config_;
        }
        config_ = config;

    }

    void restore(Config config) {
        if (savedconfig_) {
            config_ = *savedconfig_;
            savedconfig_.reset();
        }
    }

    void light() {
        Config expected = Config {
            .mode = SwitchMode{true}
        };

        if (expected == config_) { return; }
        manager_->applyNode();
    }

    void dark() {
        Config expected = Config {
            .mode = SwitchMode{false}
        };

        if (expected == config_) { return; }
        manager_->applyNode();
    }

    void blink(uint32_t interval) {
        Config expected = Config {
            .mode = BlinkMode{interval}
        };
        if (expected == config_) { return; }
        manager_->applyNode();

    }

};


class NodeManager {
public:
    Node& createNode(const Config& config) {
       return nodes.emplace_back(this, config);
    }

    NodeManager(std::string path, Utils::Timer &timer):
        state_(path, false), timer_(timer) {
    }

    ~NodeManager() {
        stopTimer();
    }

    void applyNode() {
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
                timer_.Register([&]() {
                    std::lock_guard<std::recursive_mutex> lk(m);
                    state_.toggle();
                }, arg.interval, false);
            } else {
                std::cout << "Unknow Mode" << std::endl;
            }
        }, config.mode);
        
    }

private:
    std::recursive_mutex m;

    State state_;
    std::list<Node> nodes;
    Utils::Timer &timer_;
    std::optional<uint32_t> timerid_;

private:
    void stopTimer() {
        if(!timerid_) { return; };
        timer_.Unregister(*timerid_);
        timerid_.reset();
    }

};
