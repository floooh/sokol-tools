#pragma once
#include <string>

namespace shdc {

// the output format
struct Format	{
    enum Enum {
        SOKOL = 0,
        SOKOL_IMPL,
        SOKOL_ZIG,
        SOKOL_NIM,
        SOKOL_ODIN,
        SOKOL_RUST,
        SOKOL_D,
        SOKOL_JAI,
        SOKOL_C2,
        SOKOL_C3,
        BARE,
        BARE_YAML,
        NUM,
        INVALID,
    };

    static const char* to_str(Enum f);
    static Enum from_str(const std::string& str);
};

inline const char* Format::to_str(Enum f) {
    switch (f) {
        case SOKOL:         return "sokol";
        case SOKOL_IMPL:    return "sokol_impl";
        case SOKOL_ZIG:     return "sokol_zig";
        case SOKOL_NIM:     return "sokol_nim";
        case SOKOL_ODIN:    return "sokol_odin";
        case SOKOL_RUST:    return "sokol_rust";
        case SOKOL_D:       return "sokol_d";
        case SOKOL_JAI:     return "sokol_jai";
        case SOKOL_C2:      return "sokol_c2";
        case SOKOL_C3:      return "sokol_c3";
        case BARE:          return "bare";
        case BARE_YAML:     return "bare_yaml";
        default:            return "<invalid>";
    }
}

inline Format::Enum Format::from_str(const std::string& str) {
    if (str == "sokol") {
        return SOKOL;
    } else if (str == "sokol_impl") {
        return SOKOL_IMPL;
    } else if (str == "sokol_zig") {
        return SOKOL_ZIG;
    } else if (str == "sokol_nim") {
        return SOKOL_NIM;
    } else if (str == "sokol_odin") {
        return SOKOL_ODIN;
    } else if (str == "sokol_rust") {
        return SOKOL_RUST;
    } else if (str == "sokol_d") {
        return SOKOL_D;
    } else if (str == "sokol_jai") {
        return SOKOL_JAI;
    } else if (str == "sokol_c2") {
        return SOKOL_C2;
    } else if (str == "sokol_c3") {
        return SOKOL_C3;
    } else if (str == "bare") {
        return BARE;
    } else if (str == "bare_yaml") {
        return BARE_YAML;
    } else {
        return INVALID;
    }
}

} // namespace shdc
