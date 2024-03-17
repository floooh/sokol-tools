#pragma once
#include <string>
#include "shader_stage.h"
#include "image_type.h"
#include "image_sample_type.h"

namespace shdc::refl {

struct Image {
    static const int NUM = 12;        // must be identical with SG_MAX_SHADERSTAGE_IMAGES
    ShaderStage::Enum stage = ShaderStage::INVALID;
    int slot = -1;
    std::string name;
    ImageType::Enum type = ImageType::INVALID;
    ImageSampleType::Enum sample_type = ImageSampleType::INVALID;
    bool multisampled = false;

    bool equals(const Image& other) const;
};

inline bool Image::equals(const Image& other) const {
    return (stage == other.stage)
        && (slot == other.slot)
        && (name == other.name)
        && (type == other.type)
        && (sample_type == other.sample_type)
        && (multisampled == other.multisampled);
}

} // namespace
