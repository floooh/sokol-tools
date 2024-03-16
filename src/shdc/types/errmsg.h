#pragma once
#include <string>
#include "fmt/format.h"

namespace shdc {

// an error message object with filename, line number and message
struct ErrMsg {
    enum Type {
        ERROR,
        WARNING,
    };
    Type type = ERROR;
    std::string file;
    std::string msg;
    int line_index = -1;      // line_index is zero-based!
    bool has_error = false;

    enum Format {
        GCC,
        MSVC
    };

    static ErrMsg error(const std::string& file, int line, const std::string& msg);
    static ErrMsg warning(const std::string& file, int line, const std::string& msg);
    std::string as_string(Format fmt) const;
    void print(Format fmt) const;
    static const char* format_to_str(Format fmt);
};


inline ErrMsg ErrMsg::error(const std::string& file, int line, const std::string& msg) {
    ErrMsg err;
    err.type = ERROR;
    err.file = file;
    err.msg = msg;
    err.line_index = line;
    err.has_error = true;
    return err;
}

inline ErrMsg ErrMsg::warning(const std::string& file, int line, const std::string& msg) {
    ErrMsg err;
    err.type = WARNING;
    err.file = file;
    err.msg = msg;
    err.line_index = line;
    err.has_error = true;
    return err;
}

inline std::string ErrMsg::as_string(Format fmt) const {
    if (fmt == MSVC) {
        return fmt::format("{}({}): {}: {}", file, line_index, (type==ERROR)?"error":"warning", msg);
    } else {
        return fmt::format("{}:{}:0: {}: {}", file, line_index, (type==ERROR)?"error":"warning", msg);
    }
}

inline void ErrMsg::print(Format fmt) const {
    fmt::print("{}\n", as_string(fmt));
}

inline const char* ErrMsg::format_to_str(Format fmt) {
    switch (fmt) {
        case GCC: return "gcc";
        case MSVC: return "msvc";
        default: return "<invalid>";
    }
}

} // namespace shdc
