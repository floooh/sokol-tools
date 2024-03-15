#pragma once

namespace shdc {

// a shader stage
struct stage_t {
    enum type_t {
        INVALID,
        VS,
        FS,
    };
    static const char* to_str(type_t t);
    static bool is_vs(type_t t);
    static bool is_fs(type_t t);
};

inline const char* stage_t::to_str(stage_t::type_t t) {
    switch (t) {
        case VS: return "VS";
        case FS: return "FS";
        default: return "INVALID";
    }
}

inline bool stage_t::is_vs(stage_t::type_t t) {
    return VS == t;
}

inline bool stage_t::is_fs(stage_t::type_t t) {
    return FS == t;
}

} // namespace shdc