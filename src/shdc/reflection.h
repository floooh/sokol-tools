#pragma once
#include <string>
#include <array>
#include <vector>
#include "spirv_cross.hpp"
#include "types/slang.h"
#include "types/snippet.h"
#include "types/shader_stage.h"
#include "types/vertex_attr.h"
#include "types/uniform_block.h"
#include "types/image.h"
#include "types/sampler.h"
#include "types/image_sampler.h"

namespace shdc {

struct Reflection {
    ShaderStage::type_t stage = ShaderStage::INVALID;
    std::string entry_point;
    std::array<VertexAttr, VertexAttr::NUM> inputs;
    std::array<VertexAttr, VertexAttr::NUM> outputs;
    std::vector<UniformBlock> uniform_blocks;
    std::vector<Image> images;
    std::vector<Sampler> samplers;
    std::vector<ImageSampler> image_samplers;

    static Reflection parse(const spirv_cross::Compiler& compiler, const Snippet& snippet, Slang::type_t slang);

    const UniformBlock* find_uniform_block_by_slot(int slot) const;
    const Image* find_image_by_slot(int slot) const;
    const Image* find_image_by_name(const std::string& name) const;
    const Sampler* find_sampler_by_slot(int slot) const;
    const Sampler* find_sampler_by_name(const std::string& name) const;
    const ImageSampler* find_image_sampler_by_slot(int slot) const;
};

} // namespace reflection
