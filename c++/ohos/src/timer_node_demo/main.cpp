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

    std::recursive_mutex m;


    State(std::string path, bool on) : path_(path), file_(path), on_(on) {}

    void write() {
        std::lock_guard<std::recursive_mutex> lk(m);
        if (file_.is_open()) {
            file_ << (on_ ? "1": "0") << std::endl;
        } else {
            std::cout << path_ << ":" << on_ << std::endl;
        }
    }
    void write(bool on) { 
        std::lock_guard<std::recursive_mutex> lk(m);
        on_ = on;
        write();
    }
    void toggle() {
        std::lock_guard<std::recursive_mutex> lk(m);
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
    NodeManager *manager_;

public:
    Node(NodeManager * manager, Config config): manager_(manager) , config_(config) {};
};


class NodeManager {
public:
    Node& createNode(const std::string& name, const Config& config) {
       return nodes.emplace_back(this, config);
    }

    NodeManager(std::string path):
        state_(path, false) {
        timer_.Setup();
    }

    ~NodeManager() {
        stopTimer();
        timer_.Shutdown();
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
    Utils::Timer timer_ {"Node"};
    std::optional<uint32_t> timerid_;
    


private:
    void stopTimer() {
        std::lock_guard<std::recursive_mutex> lk(m);

        if(!timerid_) { return; };
        timer_.Unregister(*timerid_);
        timerid_.reset();
    }

};
