#pragma once
#include <string>

namespace shdc::refl {

// special combined-image-samplers for GLSL output with GL semantics
struct ImageSampler {
    static const int NUM = 12;      // must be identical with SG_MAX_SHADERSTAGE_IMAGES
    int slot = -1;
    std::string name;
    std::string image_name;
    std::string sampler_name;

    bool equals(const ImageSampler& other) const;
};

inline bool ImageSampler::equals(const ImageSampler& other) const {
    return (slot == other.slot)
        && (name == other.name)
        && (image_name == other.image_name)
        && (sampler_name == other.sampler_name);
}

} // namespace
