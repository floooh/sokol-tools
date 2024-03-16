#pragma once
#include <string>
#include "sampler_type.h"

namespace shdc {

struct SamplerTypeTag {
    std::string smp_name;
    SamplerType::Enum type = SamplerType::INVALID;
    int line_index = 0;
    SamplerTypeTag();
    SamplerTypeTag(const std::string& n, SamplerType::Enum t, int l);
};

inline SamplerTypeTag::SamplerTypeTag() { };

inline SamplerTypeTag::SamplerTypeTag(const std::string& n, SamplerType::Enum t, int l):
    smp_name(n),
    type(t),
    line_index(l)
{ };

} // namespace shdc
