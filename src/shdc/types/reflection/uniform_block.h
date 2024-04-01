#pragma once
#include <string>
#include <vector>
#include "fmt/format.h"
#include "uniform.h"
#include "shader_stage.h"
#include "type.h"

namespace shdc::refl {

struct UniformBlock {
    static const int Num = 4;     // must be identical with SG_MAX_SHADERSTAGE_UBS
    ShaderStage::Enum stage = ShaderStage::Invalid;
    int slot = -1;
    int size = 0;
    std::string struct_name;
    std::string inst_name;
    std::vector<Uniform> uniforms;
    bool flattened = false;
    Type struct_refl;      // FIXME: replace struct_name and uniforms with this

    bool equals(const UniformBlock& other) const;
    void dump_debug(const std::string& indent) const;
};

// FIXME: hmm is this correct??
inline bool UniformBlock::equals(const UniformBlock& other) const {
    if ((stage != other.stage) ||
        (slot != other.slot) ||
        (size != other.size) ||
        (struct_name != other.struct_name) ||
        (uniforms.size() != other.uniforms.size()) ||
        (flattened != other.flattened))
    {
        return false;
    }
    for (int i = 0; i < (int)uniforms.size(); i++) {
        if (!uniforms[i].equals(other.uniforms[i])) {
            return false;
        }
    }
    return true;
}

inline void UniformBlock::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}stage: {}\n", indent2, ShaderStage::to_str(stage));
    fmt::print(stderr, "{}slot: {}\n", indent2, slot);
    fmt::print(stderr, "{}inst_name: {}\n", indent2, inst_name);
    fmt::print(stderr, "{}flattened: {}\n", indent2, flattened);
    fmt::print(stderr, "{}struct:\n", indent2);
    struct_refl.dump_debug(indent2);
}

} // namespace
