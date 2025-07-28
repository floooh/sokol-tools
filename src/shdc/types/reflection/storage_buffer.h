#pragma once
#include <string>
#include "fmt/format.h"
#include "../shader_stage.h"
#include "type.h"

namespace shdc::refl {

struct StorageBuffer {
    ShaderStage::Enum stage = ShaderStage::Invalid;
    int sokol_slot = -1;
    int hlsl_register_t_n = -1;
    int hlsl_register_u_n = -1;
    int msl_buffer_n = -1;
    int wgsl_group1_binding_n = -1;
    int glsl_binding_n = -1;
    std::string name;   // shortcut for struct_info.name
    std::string inst_name;
    bool readonly;
    Type struct_info;

    bool equals(const StorageBuffer& other) const;
    void dump_debug(const std::string& indent) const;
};

inline bool StorageBuffer::equals(const StorageBuffer& other) const {
    return (stage == other.stage)
        && (sokol_slot == other.sokol_slot)
        && (name == other.name)
        && (readonly == other.readonly)
        && (struct_info.equals(other.struct_info));
}

inline void StorageBuffer::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}stage: {}\n", indent2, ShaderStage::to_str(stage));
    fmt::print(stderr, "{}sokol_slot: {}\n", indent2, sokol_slot);
    fmt::print(stderr, "{}hlsl_register_t_n: {}\n", indent2, hlsl_register_t_n);
    fmt::print(stderr, "{}msl_buffer_n: {}\n", indent2, msl_buffer_n);
    fmt::print(stderr, "{}wgsl_group1_binding_n: {}\n", indent2, wgsl_group1_binding_n);
    fmt::print(stderr, "{}glsl_binding_n: {}\n", indent2, glsl_binding_n);
    fmt::print(stderr, "{}inst_name: {}\n", indent2, inst_name);
    fmt::print(stderr, "{}readonly: {}\n", indent2, readonly);
    fmt::print(stderr, "{}struct:\n", indent2);
    struct_info.dump_debug(indent2);
}

} // namespace
