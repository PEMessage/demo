#ifndef MISC_H
#define MISC_H

#include <variant>
#include <cstdint>
#include <iostream>
#include <fstream>

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
        // if (file_.is_open()) {
        //     file_ << (on_ ? "1": "0") << std::endl;
        // } else {
        // }
        std::cout << path_ << ":" << on_ << std::endl;
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

#endif // MISC_H
