#pragma once
#include <string.h>

namespace shdc::refl {

// subset of pixel formats valid for storage textures
struct StoragePixelFormat {
    enum Enum {
        INVALID,
        RGBA8_UNORM,        // => SG_PIXELFORMAT_RGBA8
        RGBA8_SNORM,        // => SG_PIXELFORMAT_RGBA8SN
        RGBA8_UINT,         // => SG_PIXELFORMAT_RGBA8UI
        RGBA8_SINT,         // => SG_PIXELFORMAT_RGBA8SI
        RGBA16_UINT,        // => SG_PIXELFORMAT_RGBA16UI
        RGBA16_SINT,        // => SG_PIXELFORMAT_RGBA16SI
        RGBA16_FLOAT,       // => SG_PIXELFORMAT_RGBA16F
        R32_UINT,           // => SG_PIXELFORMAT_R32UI
        R32_SINT,           // => SG_PIXELFORMAT_R32SI
        R32_FLOAT,          // => SG_PIXELFORMAT_R32F
        RG32_UINT,          // => SG_PIXELFORMAT_RG32UI
        RG32_SINT,          // => SG_PIXELFORMAT_RG32SI
        RG32_FLOAT,         // => SG_PIXELFORMAT_RG32F
        RGBA32_UINT,        // => SG_PIXELFORMAT_RGBA32UI
        RGBA32_SINT,        // => SG_PIXELFORMAT_RGBA32SI
        RGBA32_FLOAT,       // => SG_PIXELFORMAT_RGBA32F
    };
    static const char* to_str(Enum e);
};

inline const char* StoragePixelFormat::to_str(Enum e) {
    switch (e) {
        case RGBA8_UNORM: return "rgba8_unorm";
        case RGBA8_SNORM: return "rgba8_snorm";
        case RGBA8_UINT: return "rgba8_uint";
        case RGBA8_SINT: return "rgba8_sint";
        case RGBA16_UINT: return "rgba16_uint";
        case RGBA16_SINT: return "rgba16_sint";
        case RGBA16_FLOAT: return "rgba16_float";
        case R32_UINT: return "r32_uint";
        case R32_SINT: return "r32_sint";
        case R32_FLOAT: return "r32_float";
        case RG32_UINT: return "r32_uint";
        case RG32_SINT: return "r32_sint";
        case RG32_FLOAT: return "r32_float";
        case RGBA32_UINT: return "rgba32_uint";
        case RGBA32_SINT: return "rgba32_sint";
        case RGBA32_FLOAT: return "rgba32_float";
        default: return "invalid";
    }
}

} // namespace
