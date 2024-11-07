#pragma once
#include <string>
#include "fmt/format.h"
#include "shader_stage.h"

namespace shdc::refl {

// special combined-image-samplers for GLSL output with GL semantics
struct ImageSampler {
    ShaderStage::Enum stage = ShaderStage::Invalid;
    int sokol_slot = -1;
    std::string name;
    std::string image_name;
    std::string sampler_name;

    bool equals(const ImageSampler& other) const;
    void dump_debug(const std::string& indent) const;
};

inline bool ImageSampler::equals(const ImageSampler& other) const {
    return (stage == other.stage)
        && (sokol_slot == other.sokol_slot)
        && (name == other.name)
        && (image_name == other.image_name)
        && (sampler_name == other.sampler_name);
}

inline void ImageSampler::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}stage: {}\n", indent2, ShaderStage::to_str(stage));
    fmt::print(stderr, "{}sokol_slot: {}\n", indent2, sokol_slot);
    fmt::print(stderr, "{}name: {}\n", indent2, name);
    fmt::print(stderr, "{}image_name: {}\n", indent2, image_name);
    fmt::print(stderr, "{}sampler_name: {}\n", indent2, sampler_name);
}

} // namespace
