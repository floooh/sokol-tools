#pragma once
#include <string>
#include "consts.h"
#include "shader_stage.h"

namespace shdc::refl {

struct BindSlot {
    enum Type {
        Invalid,
        UniformBlock,
        Texture,
        StorageBuffer,
        StorageImage,
        Sampler,
        TextureSampler,
    };
    std::string name;
    ShaderStage::Enum stage = ShaderStage::Invalid;
    Type type = Invalid;
    struct {
        int binding_n = -1; // storage buffer/image
    } glsl;
    struct {
        int register_b_n = -1;  // uniform block
        int register_t_n = -1;  // texture, readonly storage buffer/image
        int register_u_n = -1;  // read/write storage buffer/image
        int register_s_n = -1;  // sampler
    } hlsl;
    struct {
        int buffer_n = -1;      // uniform block, storage buffer
        int texture_n = -1;     // texture, storage image
        int sampler_n = -1;     // sampler
    } msl;
    struct {
        int group0_binding_n = -1;  // uniform block
        int group1_binding_n = -1;  // texture, storage buffer/image, sampler
    } wgsl;
};

} // namespace shdc::refl

