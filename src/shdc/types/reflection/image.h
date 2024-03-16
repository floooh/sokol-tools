#pragma once
#include <string>
#include "image_type.h"
#include "image_sample_type.h"

namespace shdc::refl {

struct Image {
    static const int NUM = 12;        // must be identical with SG_MAX_SHADERSTAGE_IMAGES
    int slot = -1;
    std::string name;
    ImageType::Enum type = ImageType::INVALID;
    ImageSampleType::Enum sample_type = ImageSampleType::INVALID;
    bool multisampled = false;
    int unique_index = -1;      // index into Spirvcross.unique_images

    bool equals(const Image& other);
};

inline bool Image::equals(const Image& other) {
    return (slot == other.slot)
        && (name == other.name)
        && (type == other.type)
        && (sample_type == other.sample_type)
        && (multisampled == other.multisampled);
}

} // namespace