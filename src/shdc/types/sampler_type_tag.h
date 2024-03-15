#pragma once
#include <string>
#include "sampler_type.h"

namespace shdc {

struct sampler_type_tag_t {
    std::string smp_name;
    sampler_type_t::type_t type = sampler_type_t::INVALID;
    int line_index = 0;
    sampler_type_tag_t();
    sampler_type_tag_t(const std::string& n, sampler_type_t::type_t t, int l);
};

inline sampler_type_tag_t::sampler_type_tag_t() { };

inline sampler_type_tag_t::sampler_type_tag_t(const std::string& n, sampler_type_t::type_t t, int l):
    smp_name(n),
    type(t),
    line_index(l)
{ };

} // namespace shdc
