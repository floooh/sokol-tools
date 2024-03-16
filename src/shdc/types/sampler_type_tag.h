#pragma once
#include <string>
#include "sampler_type.h"

namespace shdc {

struct SamplerTypeTag {
    std::string smp_name;
    sampler_type_t::type_t type = sampler_type_t::INVALID;
    int line_index = 0;
    SamplerTypeTag();
    SamplerTypeTag(const std::string& n, sampler_type_t::type_t t, int l);
};

inline SamplerTypeTag::SamplerTypeTag() { };

inline SamplerTypeTag::SamplerTypeTag(const std::string& n, sampler_type_t::type_t t, int l):
    smp_name(n),
    type(t),
    line_index(l)
{ };

} // namespace shdc
