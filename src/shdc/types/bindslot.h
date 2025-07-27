#pragma once
#include <string>
#include "consts.h"

namespace shdc {

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
    Type type = Invalid;
    bool readonly = false;
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

    bool empty() const;
    bool equals(const BindSlot& other) const;
};

inline bool BindSlot::empty() const {
    return type == Invalid;
}

inline bool BindSlot::equals(const BindSlot& other) const {
    return (name == other.name)
        && (type == other.type)
        && (readonly == other.readonly)
        && (hlsl.register_b_n == other.hlsl.register_b_n)
        && (hlsl.register_t_n == other.hlsl.register_t_n)
        && (hlsl.register_u_n == other.hlsl.register_u_n)
        && (hlsl.register_s_n == other.hlsl.register_s_n)
        && (msl.buffer_n == other.msl.buffer_n)
        && (msl.texture_n == other.msl.texture_n)
        && (msl.sampler_n == other.msl.sampler_n)
        && (wgsl.group0_binding_n == other.wgsl.group0_binding_n)
        && (wgsl.group1_binding_n == other.wgsl.group1_binding_n);
}

} // namespace shdc

