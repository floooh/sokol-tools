#pragma once
#include <string>
#include "sampler_type.h"

namespace shdc::refl {

struct Sampler {
    static const int NUM = 12;      // must be identical with SG_MAX_SHADERSTAGE_SAMPLERS
    int slot = -1;
    std::string name;
    SamplerType::Enum type = SamplerType::INVALID;
    int unique_index = -1;          // index into Spirvcross.unique_samplers

    bool equals(const Sampler& other);
};

inline bool Sampler::equals(const Sampler& other) {
    return (slot == other.slot)
        && (name == other.name)
        && (type == other.type);
}

} // namespace
