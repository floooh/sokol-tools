#pragma once
#include <string>
#include <vector>
#include "fmt/format.h"
#include "shader_stage.h"
#include "type.h"
#include "uniform.h"

namespace shdc::refl {

struct UniformBlock {
    static const int Num = 4;     // must be identical with SG_MAX_SHADERSTAGE_UBS
    ShaderStage::Enum stage = ShaderStage::Invalid;
    int slot = -1;
    std::string inst_name;
    bool flattened = false;
    Type struct_info;

    // FIXME: remove
    std::vector<Uniform> uniforms;

    bool equals(const UniformBlock& other) const;
    void dump_debug(const std::string& indent) const;
};

inline bool UniformBlock::equals(const UniformBlock& other) const {
    return (stage == other.stage)
        && (slot == other.slot)
        // NOTE: ignore inst_name
        && (flattened == other.flattened)
        && struct_info.equals(other.struct_info);
}

inline void UniformBlock::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}stage: {}\n", indent2, ShaderStage::to_str(stage));
    fmt::print(stderr, "{}slot: {}\n", indent2, slot);
    fmt::print(stderr, "{}inst_name: {}\n", indent2, inst_name);
    fmt::print(stderr, "{}flattened: {}\n", indent2, flattened);
    fmt::print(stderr, "{}struct:\n", indent2);
    struct_info.dump_debug(indent2);
}

} // namespace
