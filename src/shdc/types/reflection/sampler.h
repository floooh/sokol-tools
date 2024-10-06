#pragma once
#include <string>
#include "fmt/format.h"
#include "sampler_type.h"
#include "shader_stage.h"

namespace shdc::refl {

struct Sampler {
    ShaderStage::Enum stage = ShaderStage::Invalid;
    int sokol_slot = -1;
    int hlsl_register_s_n = -1;
    int msl_sampler_n = -1;
    int wgsl_group1_binding_n = -1;
    std::string name;
    SamplerType::Enum type = SamplerType::INVALID;

    bool equals(const Sampler& other) const;
    void dump_debug(const std::string& indent) const;
};

inline bool Sampler::equals(const Sampler& other) const {
    return (stage == other.stage)
        && (name == other.name)
        && (type == other.type);
}

inline void Sampler::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}stage: {}\n", indent2, ShaderStage::to_str(stage));
    fmt::print(stderr, "{}sokol_slot: {}\n", indent2, sokol_slot);
    fmt::print(stderr, "{}hlsl_register_s_n: {}\n", indent2, hlsl_register_s_n);
    fmt::print(stderr, "{}msl_sampler_n: {}\n", indent2, msl_sampler_n);
    fmt::print(stderr, "{}wgsl_group1_binding_n: {}\n", indent2, wgsl_group1_binding_n);
    fmt::print(stderr, "{}name: {}\n", indent2, name);
    fmt::print(stderr, "{}type: {}\n", indent2, SamplerType::to_str(type));
}

} // namespace
