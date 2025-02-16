#pragma once
#include <string>
#include <array>
#include "fmt/format.h"
#include "shader_stage.h"
#include "stage_attr.h"
#include "bindings.h"

namespace shdc::refl {

struct StageReflection {
    int snippet_index = -1;
    std::string snippet_name;
    ShaderStage::Enum stage = ShaderStage::Invalid;
    std::string entry_point;
    std::array<StageAttr, StageAttr::Num> inputs;       // index == attribute slot
    std::array<StageAttr, StageAttr::Num> outputs;      // index == attribute slot
    Bindings bindings;
    int cs_workgroup_size[3];  // layout(local_size_x=x, local_size_y=y, local_size_z=z)

    size_t num_inputs() const;
    size_t num_outputs() const;
    std::string entry_point_by_slang(Slang::Enum slang) const;
    void dump_debug(const std::string& indent) const;
};

inline size_t StageReflection::num_inputs() const {
    size_t num = 0;
    for (const auto& attr: inputs) {
        if (attr.slot >= 0) {
            num += 1;
        }
    }
    return num;
}

inline size_t StageReflection::num_outputs() const {
    size_t num = 0;
    for (const auto& attr: outputs) {
        if (attr.slot >= 0) {
            num += 1;
        }
    }
    return num;
}

inline std::string StageReflection::entry_point_by_slang(Slang::Enum slang) const {
    if (Slang::is_msl(slang)) {
        return entry_point + "0";
    } else {
        return entry_point;
    }
}

inline void StageReflection::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}stage: {}\n", indent2, ShaderStage::to_str(stage));
    fmt::print(stderr, "{}snippet_index: {}\n", indent2, snippet_index);
    fmt::print(stderr, "{}snippet_name: {}\n", indent2, snippet_name);
    fmt::print(stderr, "{}entry_point: {}\n", indent2, entry_point);
    fmt::print(stderr, "{}workgroup_size: {} {} {}\n", indent2, cs_workgroup_size[0], cs_workgroup_size[1], cs_workgroup_size[2]);
    fmt::print(stderr, "{}inputs:\n", indent2);
    for (const auto& input: inputs) {
        if (input.slot != -1) {
            input.dump_debug(indent2);
        }
    }
    fmt::print(stderr, "{}outputs:\n", indent2);
    for (const auto& output: outputs) {
        if (output.slot != -1) {
            output.dump_debug(indent2);
        }
    }
    fmt::print(stderr, "{}bindings:\n", indent2);
    bindings.dump_debug(indent2);
}

} // namespace
