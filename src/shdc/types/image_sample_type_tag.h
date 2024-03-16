#pragma once
#include <string>
#include "reflection/image_sample_type.h"

namespace shdc {

struct ImageSampleTypeTag {
    std::string tex_name;
    refl::ImageSampleType::Enum type = refl::ImageSampleType::INVALID;
    int line_index = 0;
    ImageSampleTypeTag();
    ImageSampleTypeTag(const std::string& n, refl::ImageSampleType::Enum t, int l);
};

inline ImageSampleTypeTag::ImageSampleTypeTag() { };

inline ImageSampleTypeTag::ImageSampleTypeTag(const std::string& n, refl::ImageSampleType::Enum t, int l):
    tex_name(n),
    type(t),
    line_index(l)
{ };
} // namespace shdc
