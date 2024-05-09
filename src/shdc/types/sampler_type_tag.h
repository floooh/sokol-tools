#pragma once
#include <string>
#include "reflection/sampler_type.h"

namespace shdc {

struct SamplerTypeTag {
    std::string smp_name;
    refl::SamplerType::Enum type = refl::SamplerType::INVALID;
    int line_index = 0;
    SamplerTypeTag();
    SamplerTypeTag(const std::string& n, refl::SamplerType::Enum t, int l);
};

inline SamplerTypeTag::SamplerTypeTag() { };

inline SamplerTypeTag::SamplerTypeTag(const std::string& n, refl::SamplerType::Enum t, int l):
    smp_name(n),
    type(t),
    line_index(l)
{ };

} // namespace shdc
