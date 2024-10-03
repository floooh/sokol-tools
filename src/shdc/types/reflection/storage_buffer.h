#pragma once
#include <string>
#include "fmt/format.h"
#include "shader_stage.h"
#include "type.h"

namespace shdc::refl {

struct StorageBuffer {
    static const int Num = 8; // must be identical with SG_MAX_STORAGEBUFFER_BINDSLOTS
    ShaderStage::Enum stage = ShaderStage::Invalid;
    int slot = -1;
    std::string inst_name;
    bool readonly;
    Type struct_info;

    bool equals(const StorageBuffer& other) const;
    void dump_debug(const std::string& indent) const;
};

inline bool StorageBuffer::equals(const StorageBuffer& other) const {
    return (stage == other.stage)
        && (slot == other.slot)
        && (inst_name == other.inst_name)
        && (readonly == other.readonly)
        && (struct_info.equals(other.struct_info));
}

inline void StorageBuffer::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}stage: {}\n", indent2, ShaderStage::to_str(stage));
    fmt::print(stderr, "{}slot: {}\n", indent2, slot);
    fmt::print(stderr, "{}inst_name: {}\n", indent2, inst_name);
    fmt::print(stderr, "{}readonly: {}\n", indent2, readonly);
    fmt::print(stderr, "{}struct:\n", indent2);
    struct_info.dump_debug(indent2);
}

} // namespace
