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
    std::string stage_name;                             // same as ShaderStage::to_str(stage)
    std::string entry_point;
    std::array<StageAttr, StageAttr::Num> inputs;       // index == attribute slot
    std::array<StageAttr, StageAttr::Num> outputs;      // index == attribute slot
    Bindings bindings;

    void dump_debug(const std::string& indent) const;
};

inline void StageReflection::dump_debug(const std::string& indent) const {
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "{}-\n", indent);
    fmt::print(stderr, "{}name: {}\n", indent2, stage_name);
    fmt::print(stderr, "{}snippet_index: {}\n", indent2, snippet_index);
    fmt::print(stderr, "{}snippet_name: {}\n", indent2, snippet_name);
    fmt::print(stderr, "{}entry_point: {}\n", indent2, entry_point);
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
