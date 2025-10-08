#ifndef MISC_H
#define MISC_H

#include <optional>
#include <variant>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <unistd.h>

struct InvalidMode {
    // All InvalidMode equal to each other in value
    bool operator==(const InvalidMode& other) const {
        return true;
    }
    bool operator!=(const InvalidMode& other) const { return !(*this == other); }
};

struct ConstMode {
    bool operator==(const ConstMode& other) const {
        return true;
    }
    bool operator!=(const ConstMode& other) const { return !(*this == other); }
};

struct BlinkMode {
    uint32_t interval = 0;

    bool operator==(const BlinkMode& other) const {
        return interval == other.interval;
    }
    bool operator!=(const BlinkMode& other) const { return !(*this == other); }
};

struct DutyMode {
    uint32_t interval = 0;
    uint32_t duty = 0; // should be 0-100

    bool operator==(const DutyMode& other) const {
        return interval == other.interval && duty == other.duty;
    }
    bool operator!=(const DutyMode& other) const { return !(*this == other); }
};

using SubMode = std::variant<InvalidMode, ConstMode, BlinkMode, DutyMode>;

struct Mode {
    bool enabled;
    SubMode submode;

    bool operator==(const Mode& other) const {
        return submode == other.submode && enabled == other.enabled;
    }
    bool operator!=(const Mode& other) const {
        return !(*this == other);
    }
};

struct State {
    std::string path_;
    std::optional<std::ofstream> file_;

    bool on_;
    bool enabled_;

    State(std::string path, bool on, bool enabled) : path_(path), file_(), on_(on), enabled_(enabled) {
        if(access(path_.c_str(), F_OK | W_OK) == 0 && enabled_)  {
            file_ = std::ofstream(path_);
            file_->setf(std::ios::unitbuf);
        }

    }

    void write() {
        if (file_ && file_->is_open()) {
            file_->seekp(0);
            *file_ << (on_ ? "1": "0");
            file_->flush();

            std::cout << "[*] " << path_ << ":" << on_ << std::endl;
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
