#pragma once

namespace shdc {

// image-type (see sg_image_type)
struct image_type_t {
    enum type_t {
        INVALID,
        _2D,
        CUBE,
        _3D,
        ARRAY
    };
    static const char* to_str(type_t t);
};

inline const char* image_type_t::to_str(type_t t) {
    switch (t) {
        case _2D:   return "2d";
        case CUBE:  return "cube";
        case _3D:   return "3d";
        case ARRAY: return "array";
        default:    return "invalid";
    }
}

} // namespace shdc