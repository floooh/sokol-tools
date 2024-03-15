#pragma once
#include <string>

namespace shdc {

// special combined-image-samplers for GLSL output with GL semantics
struct image_sampler_t {
    static const int NUM = 12;      // must be identical with SG_MAX_SHADERSTAGE_IMAGES
    int slot = -1;
    std::string name;
    std::string image_name;
    std::string sampler_name;
    int unique_index = -1;

    bool equals(const image_sampler_t& other);
};

inline bool image_sampler_t::equals(const image_sampler_t& other) {
    return (slot == other.slot)
        && (name == other.name)
        && (image_name == other.image_name)
        && (sampler_name == other.sampler_name);
}

} // namespace shdc