#pragma once
#include <string>
#include "consts.h"
#include "fmt/format.h"
#include "slang.h"

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
    enum Qualifier {
        ReadOnly = (1<<0),
        WriteOnly = (1<<1),
    };
    int binding = 0;
    std::string name;
    Type type = Invalid;
    int qualifiers = 0;
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
    struct {
        int set0_binding_n = -1;    // uniform block
        int set1_binding_n = -1;    // textures, storage buffers/images, samplers
    } spirv;

    BindSlot() {};
    BindSlot(int binding, const std::string& name, Type type, int qualifiers = 0);
    static const char* type_to_str(Type t);
    bool empty() const;
    bool readonly() const;
    bool writeonly() const;
    void dump_debug() const;
    int get_slot_by_slang(Slang::Enum slang) const;
};

inline BindSlot::BindSlot(int _binding, const std::string& _name, Type _type, int _qualifiers):
    binding(_binding),
    name(_name),
    type(_type),
    qualifiers(_qualifiers)
{ }

inline const char* BindSlot::type_to_str(Type t) {
    switch (t) {
        case UniformBlock: return "UniformBlock";
        case Texture: return "Texture";
        case StorageBuffer: return "StorageBuffer";
        case StorageImage: return "StorageImage";
        case Sampler: return "Sampler";
        case TextureSampler: return "TextureSampler";
        default: return "Invalid";
    }
}

inline bool BindSlot::empty() const {
    return type == Invalid;
}

inline bool BindSlot::readonly() const {
    return 0 != (qualifiers & ReadOnly);
}

inline bool BindSlot::writeonly() const {
    return 0 != (qualifiers & WriteOnly);
}

inline void BindSlot::dump_debug() const {
    if (empty()) {
        return;
    }
    fmt::print("    - name: {}\n", name);
    fmt::print("      type: {}\n", type_to_str(type));
    fmt::print("      binding: {}\n", binding);
    fmt::print("      readonly: {}\n", readonly());
    fmt::print("      writeonly: {}\n", writeonly());
    if (glsl.binding_n != -1) {
        fmt::print("      glsl.binding_n: {}\n", glsl.binding_n);
    }
    if (hlsl.register_b_n != -1) {
        fmt::print("      hlsl.register_b_n: {}\n", hlsl.register_b_n);
    }
    if (hlsl.register_t_n != -1) {
        fmt::print("      hlsl.register_t_n: {}\n", hlsl.register_t_n);
    }
    if (hlsl.register_u_n != -1) {
        fmt::print("      hlsl.register_u_n: {}\n", hlsl.register_u_n);
    }
    if (hlsl.register_s_n != -1) {
        fmt::print("      hlsl.register_s_n: {}\n", hlsl.register_s_n);
    }
    if (msl.buffer_n != -1) {
        fmt::print("      msl.buffer_n: {}\n", msl.buffer_n);
    }
    if (msl.texture_n != -1) {
        fmt::print("      msl.texture_n: {}\n", msl.texture_n);
    }
    if (msl.sampler_n != -1) {
        fmt::print("      msl.sampler_n: {}\n", msl.sampler_n);
    }
    if (wgsl.group0_binding_n != -1) {
        fmt::print("      wgsl.group0_binding_n: {}\n", wgsl.group0_binding_n);
    }
    if (wgsl.group1_binding_n != -1) {
        fmt::print("      wgsl.group1_binding_b: {}\n", wgsl.group1_binding_n);
    }
    if (spirv.set0_binding_n != -1) {
        fmt::print("      spirv.set0_binding_n: {}\n", spirv.set0_binding_n);
    }
    if (spirv.set1_binding_n != -1) {
        fmt::print("      spirv.set1_binding_n: {}\n", spirv.set1_binding_n);
    }
}

inline int BindSlot::get_slot_by_slang(Slang::Enum slang) const {
    if (Slang::is_hlsl(slang)) {
        switch (type) {
            case BindSlot::Type::UniformBlock:
                return hlsl.register_b_n;
            case BindSlot::Type::Texture:
                return hlsl.register_t_n;
            case BindSlot::Type::StorageBuffer:
                return readonly() ? hlsl.register_t_n : hlsl.register_u_n;
            case BindSlot::Type::StorageImage:
                return hlsl.register_u_n;
            case BindSlot::Type::Sampler:
                return hlsl.register_s_n;
            default:
                return -1;
        }
    } else if (Slang::is_msl(slang)) {
        switch (type) {
            case BindSlot::Type::UniformBlock:
            case BindSlot::Type::StorageBuffer:
                return msl.buffer_n;
            case BindSlot::Type::Texture:
            case BindSlot::Type::StorageImage:
                return msl.texture_n;
            case BindSlot::Type::Sampler:
                return msl.sampler_n;
            default:
                return -1;
        }
    } else if (Slang::is_wgsl(slang)) {
        switch (type) {
            case BindSlot::Type::UniformBlock:
                return wgsl.group0_binding_n;
            default:
                return wgsl.group1_binding_n;
        }
    } else if (Slang::is_spirv(slang)) {
        switch (type) {
            case BindSlot::Type::UniformBlock:
                return spirv.set0_binding_n;
            default:
                return spirv.set1_binding_n;
        }
    } else if (Slang::is_glsl(slang)) {
        return glsl.binding_n;
    } else {
        return -1;
    }
}

} // namespace shdc
