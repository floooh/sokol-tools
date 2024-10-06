#pragma once
#include <string>
#include <vector>
#include "fmt/format.h"
#include "shader_stage.h"
#include "type.h"

namespace shdc::refl {

struct UniformBlock {
    ShaderStage::Enum stage = ShaderStage::Invalid;
    int sokol_slot = -1;
    int hlsl_register_b_n = -1;
    int msl_buffer_n = -1;
    int wgsl_group0_binding_n = -1;
    std::string inst_name;
    bool flattened = false;
    Type struct_info;

    bool equals(const UniformBlock& other) const;
    void dump_debug(const std::string& indent) const;
};

inline bool UniformBlock::equals(const UniformBlock& other) const {
    return (stage == other.stage)
        // NOTE: ignore inst_name
        && (flattened == other.flattened)
        && struct_info.equals(other.struct_info);
}

inline void UniformBlock::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}stage: {}\n", indent2, ShaderStage::to_str(stage));
    fmt::print(stderr, "{}sokol_slot: {}\n", indent2, sokol_slot);
    fmt::print(stderr, "{}hlsl_register_b_n: {}\n", indent2, hlsl_register_b_n);
    fmt::print(stderr, "{}msl_buffer_n: {}\n", indent2, msl_buffer_n);
    fmt::print(stderr, "{}wgsl_group0_binding_n: {}\n", indent2, wgsl_group0_binding_n);
    fmt::print(stderr, "{}inst_name: {}\n", indent2, inst_name);
    fmt::print(stderr, "{}flattened: {}\n", indent2, flattened);
    fmt::print(stderr, "{}struct:\n", indent2);
    struct_info.dump_debug(indent2);
}

} // namespace
