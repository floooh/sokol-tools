#pragma once
#include <string.h>

namespace shdc::refl {

// subset of pixel formats valid for storage textures
struct StoragePixelFormat {
    enum Enum {
        INVALID,
        RGBA8,              // => SG_PIXELFORMAT_RGBA8
        RGBA8SN,            // => SG_PIXELFORMAT_RGBA8SN
        RGBA8UI,            // => SG_PIXELFORMAT_RGBA8UI
        RGBA8SI,            // => SG_PIXELFORMAT_RGBA8SI
        RGBA16UI,           // => SG_PIXELFORMAT_RGBA16UI
        RGBA16SI,           // => SG_PIXELFORMAT_RGBA16SI
        RGBA16F,            // => SG_PIXELFORMAT_RGBA16F
        R32UI,              // => SG_PIXELFORMAT_R32UI
        R32SI,              // => SG_PIXELFORMAT_R32SI
        R32F,               // => SG_PIXELFORMAT_R32F
        RG32UI,             // => SG_PIXELFORMAT_RG32UI
        RG32SI,             // => SG_PIXELFORMAT_RG32SI
        RG32F,              // => SG_PIXELFORMAT_RG32F
        RGBA32UI,           // => SG_PIXELFORMAT_RGBA32UI
        RGBA32SI,           // => SG_PIXELFORMAT_RGBA32SI
        RGBA32F,            // => SG_PIXELFORMAT_RGBA32F
    };
    static const char* to_str(Enum e);
};

inline const char* StoragePixelFormat::to_str(Enum e) {
    switch (e) {
        case RGBA8: return "RGBA8";
        case RGBA8SN: return "RGBA8SN";
        case RGBA8UI: return "RGBA8UI";
        case RGBA8SI: return "RGBA8SI";
        case RGBA16UI: return "RGBA16UI";
        case RGBA16SI: return "RGBA16SI";
        case RGBA16F: return "RGBA16F";
        case R32UI: return "R32UI";
        case R32SI: return "R32SI";
        case R32F: return "R32F";
        case RG32UI: return "RG32UI";
        case RG32SI: return "RG32SI";
        case RG32F: return "RG32F";
        case RGBA32UI: return "RGBA32UI";
        case RGBA32SI: return "RGBA32SI";
        case RGBA32F: return "RGBA32F";
        default: return "INVALID";
    }
}

} // namespace
