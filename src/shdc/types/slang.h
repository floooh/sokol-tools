#pragma once
#include <stdint.h>
#include <string>
#include <assert.h>

namespace shdc {

// the output shader languages to create
struct Slang {
    enum Enum {
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

    static uint32_t bit(Enum c);
    static Slang::Enum from_index(int idx);
    static const char* to_str(Enum c);
    static std::string bits_to_str(uint32_t mask, const std::string& delim);
    static bool is_glsl(Enum c);
    static bool is_hlsl(Enum c);
    static bool is_msl(Enum c);
    static bool is_wgsl(Enum c);
    static Slang::Enum first_valid(uint32_t mask);
};

inline uint32_t Slang::bit(Enum c) {
    return (1<<c);
}

inline Slang::Enum Slang::from_index(int idx) {
    assert((idx >= 0) && (idx < NUM));
    return (Slang::Enum)idx;
}

inline const char* Slang::to_str(Enum c) {
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

inline std::string Slang::bits_to_str(uint32_t mask, const std::string& delim) {
    std::string res;
    bool sep = false;
    for (int i = 0; i < Slang::NUM; i++) {
        if (mask & Slang::bit((Enum)i)) {
            if (sep) {
                res += delim;
            }
            res += to_str((Enum)i);
            sep = true;
        }
    }
    return res;
}

inline bool Slang::is_glsl(Enum c) {
    switch (c) {
        case GLSL410:
        case GLSL430:
        case GLSL300ES:
            return true;
        default:
            return false;
    }
}

inline bool Slang::is_hlsl(Enum c) {
    switch (c) {
        case HLSL4:
        case HLSL5:
            return true;
        default:
            return false;
    }
}

inline bool Slang::is_msl(Enum c) {
    switch (c) {
        case METAL_MACOS:
        case METAL_IOS:
        case METAL_SIM:
            return true;
        default:
            return false;
    }
}

inline bool Slang::is_wgsl(Enum c) {
    return WGSL == c;
}

inline Slang::Enum Slang::first_valid(uint32_t mask) {
    int i = 0;
    for (i = 0; i < NUM; i++) {
        if (0 != (mask & (1<<i))) {
            break;
        }
    }
    return (Slang::Enum)i;
}

} // namespace shdc
