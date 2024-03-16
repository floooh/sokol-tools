#pragma once
#include <string>
#include "image_sample_type.h"

namespace shdc {

struct ImageSampleTypeTag {
    std::string tex_name;
    image_sample_type_t::type_t type = image_sample_type_t::INVALID;
    int line_index = 0;
    ImageSampleTypeTag();
    ImageSampleTypeTag(const std::string& n, image_sample_type_t::type_t t, int l);
};

inline ImageSampleTypeTag::ImageSampleTypeTag() { };

inline ImageSampleTypeTag::ImageSampleTypeTag(const std::string& n, image_sample_type_t::type_t t, int l):
    tex_name(n),
    type(t),
    line_index(l)
{ };
} // namespace shdc
