#pragma once
#include <string>
#include "fmt/format.h"
#include "sampler_type.h"
#include "shader_stage.h"

namespace shdc::refl {

struct Sampler {
    static const int Num = 12;      // must be identical with SG_MAX_SHADERSTAGE_SAMPLERS
    ShaderStage::Enum stage = ShaderStage::Invalid;
    int slot = -1;
    std::string name;
    SamplerType::Enum type = SamplerType::INVALID;

    bool equals(const Sampler& other) const;
    void dump_debug(const std::string& indent) const;
};

inline bool Sampler::equals(const Sampler& other) const {
    return (stage == other.stage)
        && (slot == other.slot)
        && (name == other.name)
        && (type == other.type);
}

inline void Sampler::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}stage: {}\n", indent2, ShaderStage::to_str(stage));
    fmt::print(stderr, "{}slot: {}\n", indent2, slot);
    fmt::print(stderr, "{}name: {}\n", indent2, name);
    fmt::print(stderr, "{}type: {}\n", indent2, SamplerType::to_str(type));
}

} // namespace
