#pragma once
#include <string>
#include <array>
#include "consts.h"
#include "bindslot.h"

namespace shdc {

// sokol-to-backend bind slot mappings for one shader stage
struct BindSlotMap {
    std::array<BindSlot, MaxUniformBlocks> uniform_blocks;
    std::array<BindSlot, MaxViews> views;
    std::array<BindSlot, MaxSamplers> samplers;

    // return -1 if not found
    int find_ub_slot(const std::string& name) const;
    int find_img_slot(const std::string& name) const;
    int find_smp_slot(const std::string& name) const;
    int find_sbuf_slot(const std::string& name) const;
    int find_simg_slot(const std::string& name) const;
};

} // namespace shdc

