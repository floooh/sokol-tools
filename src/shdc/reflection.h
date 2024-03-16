#pragma once
#include <string>
#include <array>
#include <vector>
#include "spirv_cross.hpp"
#include "types/slang.h"
#include "types/snippet.h"
#include "types/stage.h"
#include "types/vertex_attr.h"
#include "types/uniform_block.h"
#include "types/image.h"
#include "types/sampler.h"
#include "types/image_sampler.h"

namespace shdc {

struct reflection_t {
    stage_t::type_t stage = stage_t::INVALID;
    std::string entry_point;
    std::array<VertexAttr, VertexAttr::NUM> inputs;
    std::array<VertexAttr, VertexAttr::NUM> outputs;
    std::vector<uniform_block_t> uniform_blocks;
    std::vector<Image> images;
    std::vector<Sampler> samplers;
    std::vector<ImageSampler> image_samplers;

    static reflection_t parse(const spirv_cross::Compiler& compiler, const snippet_t& snippet, Slang::type_t slang);

    const uniform_block_t* find_uniform_block_by_slot(int slot) const;
    const Image* find_image_by_slot(int slot) const;
    const Image* find_image_by_name(const std::string& name) const;
    const Sampler* find_sampler_by_slot(int slot) const;
    const Sampler* find_sampler_by_name(const std::string& name) const;
    const ImageSampler* find_image_sampler_by_slot(int slot) const;
};

} // namespace reflection
