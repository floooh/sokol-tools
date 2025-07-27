#pragma once
#include <string>
#include <array>
#include "consts.h"
#include "bindslot.h"
#include "shader_stage.h"

namespace shdc::refl {

// sokol-to-backend bind slot mappings for one shader stage
struct BindslotMap {
    std::array<BindSlot, MaxUniformBlocks> uniform_blocks;
    std::array<BindSlot, MaxViews> views;
    std::array<BindSlot, MaxSamplers> samplers;
};

} // namespace shdc

