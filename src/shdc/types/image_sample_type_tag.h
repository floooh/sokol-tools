#pragma once
#include <string>
#include "image_sample_type.h"

namespace shdc {

struct ImageSampleTypeTag {
    std::string tex_name;
    ImageSampleType::Enum type = ImageSampleType::INVALID;
    int line_index = 0;
    ImageSampleTypeTag();
    ImageSampleTypeTag(const std::string& n, ImageSampleType::Enum t, int l);
};

inline ImageSampleTypeTag::ImageSampleTypeTag() { };

inline ImageSampleTypeTag::ImageSampleTypeTag(const std::string& n, ImageSampleType::Enum t, int l):
    tex_name(n),
    type(t),
    line_index(l)
{ };
} // namespace shdc
