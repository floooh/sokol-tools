#pragma once
#include <string>
#include "fmt/format.h"
#include "../shader_stage.h"
#include "image_type.h"
#include "image_sample_type.h"

namespace shdc::refl {

struct Texture {
    ShaderStage::Enum stage = ShaderStage::Invalid;
    int sokol_slot = -1;
    int hlsl_register_t_n = -1;
    int msl_texture_n = -1;
    int wgsl_group1_binding_n = -1;
    int spirv_set1_binding_n = -1;
    std::string name;
    ImageType::Enum type = ImageType::INVALID;
    ImageSampleType::Enum sample_type = ImageSampleType::INVALID;
    bool multisampled = false;

    bool equals(const Texture& other) const;
    void dump_debug(const std::string& indent) const;
};

inline bool Texture::equals(const Texture& other) const {
    return (stage == other.stage)
        && (sokol_slot == other.sokol_slot)
        && (name == other.name)
        && (type == other.type)
        && (sample_type == other.sample_type)
        && (multisampled == other.multisampled);
}

inline void Texture::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}stage: {}\n", indent2, ShaderStage::to_str(stage));
    fmt::print(stderr, "{}sokol_slot: {}\n", indent2, sokol_slot);
    fmt::print(stderr, "{}hlsl_register_t_n: {}\n", indent2, hlsl_register_t_n);
    fmt::print(stderr, "{}msl_texture_n: {}\n", indent2, msl_texture_n);
    fmt::print(stderr, "{}wgsl_group1_binding_n: {}\n", indent2, wgsl_group1_binding_n);
    fmt::print(stderr, "{}spirv_set1_binding_n: {}\n", indent2, spirv_set1_binding_n);
    fmt::print(stderr, "{}name: {}\n", indent2, name);
    fmt::print(stderr, "{}type: {}\n", indent2, ImageType::to_str(type));
    fmt::print(stderr, "{}sample_type: {}\n", indent2, ImageSampleType::to_str(sample_type));
    fmt::print(stderr, "{}multisampled: {}\n", indent2, multisampled);
}

} // namespace
