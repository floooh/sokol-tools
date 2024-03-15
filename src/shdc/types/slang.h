#pragma once
#include <stdint.h>
#include <string>

namespace shdc {

// the output shader languages to create
struct slang_t {
    enum type_t {
        GLSL410 = 0,
        GLSL430,
        GLSL300ES,
        HLSL4,
        HLSL5,
        METAL_MACOS,
        METAL_IOS,
        METAL_SIM,
        WGSL,
        NUM
    };

    static uint32_t bit(type_t c);
    static const char* to_str(type_t c);
    static std::string bits_to_str(uint32_t mask, const std::string& delim);
    static bool is_glsl(type_t c);
    static bool is_hlsl(type_t c);
    static bool is_msl(type_t c);
    static bool is_wgsl(type_t c);
    static slang_t::type_t first_valid(uint32_t mask);
};

inline uint32_t slang_t::bit(type_t c) {
    return (1<<c);
}

inline const char* slang_t::to_str(type_t c) {
    switch (c) {
        case GLSL410:       return "glsl410";
        case GLSL430:       return "glsl430";
        case GLSL300ES:     return "glsl300es";
        case HLSL4:         return "hlsl4";
        case HLSL5:         return "hlsl5";
        case METAL_MACOS:   return "metal_macos";
        case METAL_IOS:     return "metal_ios";
        case METAL_SIM:     return "metal_sim";
        case WGSL:          return "wgsl";
        default:            return "<invalid>";
    }
}

inline std::string slang_t::bits_to_str(uint32_t mask, const std::string& delim) {
    std::string res;
    bool sep = false;
    for (int i = 0; i < slang_t::NUM; i++) {
        if (mask & slang_t::bit((type_t)i)) {
            if (sep) {
                res += delim;
            }
            res += to_str((type_t)i);
            sep = true;
        }
    }
    return res;
}

inline bool slang_t::is_glsl(type_t c) {
    switch (c) {
        case GLSL410:
        case GLSL430:
        case GLSL300ES:
            return true;
        default:
            return false;
    }
}

inline bool slang_t::is_hlsl(type_t c) {
    switch (c) {
        case HLSL4:
        case HLSL5:
            return true;
        default:
            return false;
    }
}

inline bool slang_t::is_msl(type_t c) {
    switch (c) {
        case METAL_MACOS:
        case METAL_IOS:
        case METAL_SIM:
            return true;
        default:
            return false;
    }
}

inline bool slang_t::is_wgsl(type_t c) {
    return WGSL == c;
}

inline slang_t::type_t slang_t::first_valid(uint32_t mask) {
    int i = 0;
    for (i = 0; i < NUM; i++) {
        if (0 != (mask & (1<<i))) {
            break;
        }
    }
    return (slang_t::type_t)i;
}

} // namespace shdc
