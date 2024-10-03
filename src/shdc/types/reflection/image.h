#pragma once
#include <string>
#include "fmt/format.h"
#include "shader_stage.h"
#include "image_type.h"
#include "image_sample_type.h"

namespace shdc::refl {

struct Image {
    static const int Num = 16; // must be identical with SG_MAX_IMAGE_BINDSLOTS
    ShaderStage::Enum stage = ShaderStage::Invalid;
    int slot = -1;
    std::string name;
    ImageType::Enum type = ImageType::INVALID;
    ImageSampleType::Enum sample_type = ImageSampleType::INVALID;
    bool multisampled = false;

    bool equals(const Image& other) const;
    void dump_debug(const std::string& indent) const;
};

inline bool Image::equals(const Image& other) const {
    return (stage == other.stage)
        && (slot == other.slot)
        && (name == other.name)
        && (type == other.type)
        && (sample_type == other.sample_type)
        && (multisampled == other.multisampled);
}

inline void Image::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}stage: {}\n", indent2, ShaderStage::to_str(stage));
    fmt::print(stderr, "{}slot: {}\n", indent2, slot);
    fmt::print(stderr, "{}name: {}\n", indent2, name);
    fmt::print(stderr, "{}type: {}\n", indent2, ImageType::to_str(type));
    fmt::print(stderr, "{}sample_type: {}\n", indent2, ImageSampleType::to_str(sample_type));
    fmt::print(stderr, "{}multisampled: {}\n", indent2, multisampled);
}

} // namespace
