#pragma once
#include <string>
#include "sampler_type.h"
#include "shader_stage.h"

namespace shdc::refl {

struct Sampler {
    static const int NUM = 12;      // must be identical with SG_MAX_SHADERSTAGE_SAMPLERS
    ShaderStage::Enum stage = ShaderStage::INVALID;
    int slot = -1;
    std::string name;
    SamplerType::Enum type = SamplerType::INVALID;

    bool equals(const Sampler& other) const;
};

inline bool Sampler::equals(const Sampler& other) const {
    return (stage == other.stage)
        && (slot == other.slot)
        && (name == other.name)
        && (type == other.type);
}

} // namespace
