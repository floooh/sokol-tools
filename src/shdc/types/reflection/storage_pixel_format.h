#pragma once
#include <string.h>

namespace shdc::refl {

// subset of pixel formats valid for storage textures
struct StoragePixelFormat {
    enum Enum {
        INVALID,
        R8,                 // => SG_PIXELFORMAT_R8
        R8SN,               // => SG_PIXELFORMAT_R8SN
        R8UI,               // => SG_PIXELFORMAT_R8UI
        R8SI,               // => SG_PIXELFORMAT_R8SI
        R16,                // => SG_PIXELFORMAT_R16
        R16SN,              // => SG_PIXELFORMAT_R16SN
        R16UI,              // => SG_PIXELFORMAT_R16UI
        R16SI,              // => SG_PIXELFORMAT_R16SI
        R16F,               // => SG_PIXELFORMAT_R16F
        RG8,                // => SG_PIXELFORMAT_RG8
        RG8SN,              // => SG_PIXELFORMAT_RG8SN
        RG8UI,              // => SG_PIXELFORMAT_RG8UI
        RG8SI,              // => SG_PIXELFORMAT_RG8SI
        R32UI,              // => SG_PIXELFORMAT_R32UI
        R32SI,              // => SG_PIXELFORMAT_R32SI
        R32F,               // => SG_PIXELFORMAT_R32F
        RG16,               // => SG_PIXELFORMAT_RG16
        RG16SN,             // => SG_PIXELFORMAT_RG16SN
        RG16UI,             // => SG_PIXELFORMAT_RG16UI
        RG16SI,             // => SG_PIXELFORMAT_RG16SI
        RG16F,              // => SG_PIXELFORMAT_RG16F
        RGBA8,              // => SG_PIXELFORMAT_RGBA8
        RGBA8SN,            // => SG_PIXELFORMAT_RGBA8SN
        RGBA8UI,            // => SG_PIXELFORMAT_RGBA8UI
        RGBA8SI,            // => SG_PIXELFORMAT_RGBA8SI
        RGB10A2,            // => SG_PIXELFORMAT_RGB10A2
        RG11B10F,           // => SG_PIXELFORMAT_RG11B10F
        RG32UI,             // => SG_PIXELFORMAT_RG32UI
        RG32SI,             // => SG_PIXELFORMAT_RG32SI
        RG32F,              // => SG_PIXELFORMAT_RG32F
        RGBA16,             // => SG_PIXELFORMAT_RGBA16
        RGBA16SN,           // => SG_PIXELFORMAT_RGBA16SN
        RGBA16UI,           // => SG_PIXELFORMAT_RGBA16UI
        RGBA16SI,           // => SG_PIXELFORMAT_RGBA16SI
        RGBA16F,            // => SG_PIXELFORMAT_RGBA16F
        RGBA32UI,           // => SG_PIXELFORMAT_RGBA32UI
        RGBA32SI,           // => SG_PIXELFORMAT_RGBA32SI
        RGBA32F,            // => SG_PIXELFORMAT_RGBA32F
    };
    static const char* to_str(Enum e);
};

inline const char* StoragePixelFormat::to_str(Enum e) {
    switch (e) {
        case R8: return "R8";
        case R8SN: return "R8SN";
        case R8UI: return "R8UI";
        case R8SI: return "R8SI";
        case R16: return "R16";
        case R16SN: return "R16SN";
        case R16UI: return "R16UI";
        case R16SI: return "R16SI";
        case R16F: return "R16F";
        case RG8: return "RG8";
        case RG8SN: return "RG8SN";
        case RG8UI: return "RG8UI";
        case RG8SI: return "RG8SI";
        case R32UI: return "R32UI";
        case R32SI: return "R32SI";
        case R32F: return "R32F";
        case RG16: return "RG16";
        case RG16SN: return "RG16SN";
        case RG16UI: return "RG16UI";
        case RG16SI: return "RG16SI";
        case RG16F: return "RG16F";
        case RGBA8: return "RGBA8";
        case RGBA8SN: return "RGBA8SN";
        case RGBA8UI: return "RGBA8UI";
        case RGBA8SI: return "RGBA8SI";
        case RGB10A2: return "RGB10A2";
        case RG11B10F: return "RG11B10F";
        case RG32UI: return "RG32UI";
        case RG32SI: return "RG32SI";
        case RG32F: return "RG32F";
        case RGBA16: return "RGBA16";
        case RGBA16SN: return "RGBA16SN";
        case RGBA16UI: return "RGBA16UI";
        case RGBA16SI: return "RGBA16SI";
        case RGBA16F: return "RGBA16F";
        case RGBA32UI: return "RGBA32UI";
        case RGBA32SI: return "RGBA32SI";
        case RGBA32F: return "RGBA32F";
        default: return "INVALID";
    }
}

} // namespace
