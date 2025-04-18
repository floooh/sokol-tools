#pragma once
#include <string>
#include "fmt/format.h"
#include "shader_stage.h"
#include "image_type.h"
#include "storage_pixel_format.h"
#include "type.h"

namespace shdc::refl {

struct StorageImage {
    ShaderStage::Enum stage = ShaderStage::Invalid;
    int sokol_slot = -1;
    int hlsl_register_u_n = -1;
    int msl_texture_n = -1;
    int wgsl_group2_binding_n = -1;
    int glsl_binding_n = -1;
    std::string name;   // shortcut for struct_info.name
    ImageType::Enum type = ImageType::INVALID;
    StoragePixelFormat::Enum access_format = StoragePixelFormat::INVALID;

    bool equals(const StorageImage& other) const;
    void dump_debug(const std::string& indent) const;
};

inline bool StorageImage::equals(const StorageImage& other) const {
    return (stage == other.stage)
        && (sokol_slot == other.sokol_slot)
        && (name == other.name)
        && (type == other.type)
        && (access_format == other.access_format);
}

inline void StorageImage::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent2);
    fmt::print(stderr, "{}stage: {}\n", indent2, ShaderStage::to_str(stage));
    fmt::print(stderr, "{}sokol_slot: {}\n", indent2, sokol_slot);
    fmt::print(stderr, "{}hlsl_register_u_n: {}\n", indent2, hlsl_register_u_n);
    fmt::print(stderr, "{}msl_texture_n: {}\n", indent2, msl_texture_n);
    fmt::print(stderr, "{}wgsl_group2_binding_n: {}\n", indent2, wgsl_group2_binding_n);
    fmt::print(stderr, "{}name: {}\n", indent2, name);
    fmt::print(stderr, "{}type: {}\n", indent2, ImageType::to_str(type));
    fmt::print(stderr, "{}access_format: {}\n", indent2, StoragePixelFormat::to_str(access_format));
}

} // namespace
