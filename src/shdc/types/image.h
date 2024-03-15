#pragma once
#include <string>
#include "image_type.h"
#include "image_sample_type.h"

namespace shdc {

struct image_t {
    static const int NUM = 12;        // must be identical with SG_MAX_SHADERSTAGE_IMAGES
    int slot = -1;
    std::string name;
    image_type_t::type_t type = image_type_t::INVALID;
    image_sample_type_t::type_t sample_type = image_sample_type_t::INVALID;
    bool multisampled = false;
    int unique_index = -1;      // index into spirvcross_t.unique_images

    bool equals(const image_t& other);
};

inline bool image_t::equals(const image_t& other) {
    return (slot == other.slot)
        && (name == other.name)
        && (type == other.type)
        && (sample_type == other.sample_type)
        && (multisampled == other.multisampled);
}

} // namespace shdc
