#ifndef MISC_H
#define MISC_H

#include <optional>
#include <variant>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <unistd.h>

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

struct DutyMode {
    uint32_t interval = 0;
    uint32_t duty = 0; // should be 0-100

    bool operator==(const DutyMode& other) const {
        return interval == other.interval && duty == other.duty;
    }
};

using Mode = std::variant<SwitchMode, BlinkMode, DutyMode>;

struct Config {
    Mode mode;

    bool operator==(const Config& other) const {
        return mode == other.mode;
    }
};

struct State {
    std::string path_;
    std::optional<std::ofstream> file_;

    bool on_;

    State(std::string path, bool on) : path_(path), file_(), on_(on) {
        if(access(path_.c_str(), F_OK | W_OK) == 0)  {
            file_ = std::ofstream(path_);
        }

    }

    void write() {
        if (file_ && file_->is_open()) {
            file_->seekp(0);
            *file_ << (on_ ? "1": "0");
            file_->flush();

            std::cout << "[F] " << path_ << ":" << on_ << std::endl;
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

#endif // MISC_H
