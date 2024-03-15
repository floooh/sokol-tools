#pragma once
#include <string>
#include "fmt/format.h"

namespace shdc {

// an error message object with filename, line number and message
struct errmsg_t {
    enum type_t {
        ERROR,
        WARNING,
    };
    type_t type = ERROR;
    std::string file;
    std::string msg;
    int line_index = -1;      // line_index is zero-based!
    bool valid = false;

    enum msg_format_t {
        GCC,
        MSVC
    };

    static errmsg_t error(const std::string& file, int line, const std::string& msg);
    static errmsg_t warning(const std::string& file, int line, const std::string& msg);
    std::string as_string(msg_format_t fmt) const;
    void print(msg_format_t fmt) const;
    static const char* msg_format_to_str(msg_format_t fmt);
};


inline errmsg_t errmsg_t::error(const std::string& file, int line, const std::string& msg) {
    errmsg_t err;
    err.type = ERROR;
    err.file = file;
    err.msg = msg;
    err.line_index = line;
    err.valid = true;
    return err;
}

inline errmsg_t errmsg_t::warning(const std::string& file, int line, const std::string& msg) {
    errmsg_t err;
    err.type = WARNING;
    err.file = file;
    err.msg = msg;
    err.line_index = line;
    err.valid = true;
    return err;
}

inline std::string errmsg_t::as_string(msg_format_t fmt) const {
    if (fmt == MSVC) {
        return fmt::format("{}({}): {}: {}", file, line_index, (type==ERROR)?"error":"warning", msg);
    } else {
        return fmt::format("{}:{}:0: {}: {}", file, line_index, (type==ERROR)?"error":"warning", msg);
    }
}

inline void errmsg_t::print(msg_format_t fmt) const {
    fmt::print("{}\n", as_string(fmt));
}

inline const char* errmsg_t::msg_format_to_str(msg_format_t fmt) {
    switch (fmt) {
        case GCC: return "gcc";
        case MSVC: return "msvc";
        default: return "<invalid>";
    }
}

} // namespace shdc
