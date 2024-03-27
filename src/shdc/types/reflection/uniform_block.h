#pragma once
#include <string>
#include <vector>
#include "uniform.h"
#include "shader_stage.h"

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

    bool equals(const UniformBlock& other) const;
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

} // namespace
