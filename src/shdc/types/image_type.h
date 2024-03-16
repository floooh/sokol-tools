#pragma once

namespace shdc {

// image-type (see sg_image_type)
struct ImageType {
    enum Enum {
        INVALID,
        _2D,
        CUBE,
        _3D,
        ARRAY
    };
    static const char* to_str(Enum e);
};

inline const char* ImageType::to_str(Enum e) {
    switch (e) {
        case _2D:   return "2d";
        case CUBE:  return "cube";
        case _3D:   return "3d";
        case ARRAY: return "array";
        default:    return "invalid";
    }
}

} // namespace shdc
