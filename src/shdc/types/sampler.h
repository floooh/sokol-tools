#pragma once
#include <string>
#include "sampler_type.h"

namespace shdc {

struct sampler_t {
    static const int NUM = 12;      // must be identical with SG_MAX_SHADERSTAGE_SAMPLERS
    int slot = -1;
    std::string name;
    SamplerType::type_t type = SamplerType::INVALID;
    int unique_index = -1;          // index into spirvcross_t.unique_samplers

    bool equals(const sampler_t& other);
};

inline bool sampler_t::equals(const sampler_t& other) {
    return (slot == other.slot)
        && (name == other.name)
        && (type == other.type);
}

} // namespace shdc
