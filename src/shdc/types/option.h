#pragma once
#include <string>

namespace shdc {

// per-shader compile options
struct option_t {
    enum type_t {
        INVALID = 0,
        FIXUP_CLIPSPACE = (1<<0),
        FLIP_VERT_Y = (1<<1),
    };
    static type_t from_string(const std::string& str);
};

inline option_t::type_t option_t::from_string(const std::string& str) {
    if (str == "fixup_clipspace") {
        return FIXUP_CLIPSPACE;
    } else if (str == "flip_vert_y") {
        return FLIP_VERT_Y;
    } else {
        return INVALID;
    }
}

} // namespace shdc
