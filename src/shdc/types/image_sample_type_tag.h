#pragma once
#include <string>
#include "image_sample_type.h"

namespace shdc {

struct image_sample_type_tag_t {
    std::string tex_name;
    image_sample_type_t::type_t type = image_sample_type_t::INVALID;
    int line_index = 0;
    image_sample_type_tag_t();
    image_sample_type_tag_t(const std::string& n, image_sample_type_t::type_t t, int l);
};

inline image_sample_type_tag_t::image_sample_type_tag_t() { };

inline image_sample_type_tag_t::image_sample_type_tag_t(const std::string& n, image_sample_type_t::type_t t, int l):
    tex_name(n),
    type(t),
    line_index(l)
{ };
} // namespace shdc
