#pragma once
#include <string>
#include <array>
#include <vector>
#include "spirv_cross.hpp"
#include "types/slang.h"
#include "types/snippet.h"
#include "types/reflection/shader_stage.h"
#include "types/reflection/vertex_attr.h"
#include "types/reflection/uniform_block.h"
#include "types/reflection/image.h"
#include "types/reflection/sampler.h"
#include "types/reflection/image_sampler.h"

namespace shdc::refl {

struct Reflection {
    ShaderStage::Enum stage = ShaderStage::INVALID;
    std::string entry_point;
    std::array<VertexAttr, VertexAttr::NUM> inputs;
    std::array<VertexAttr, VertexAttr::NUM> outputs;
    std::vector<UniformBlock> uniform_blocks;
    std::vector<Image> images;
    std::vector<Sampler> samplers;
    std::vector<ImageSampler> image_samplers;

    static Reflection parse(const spirv_cross::Compiler& compiler, const Snippet& snippet, Slang::Enum slang);

    const UniformBlock* find_uniform_block_by_slot(int slot) const;
    const Image* find_image_by_slot(int slot) const;
    const Image* find_image_by_name(const std::string& name) const;
    const Sampler* find_sampler_by_slot(int slot) const;
    const Sampler* find_sampler_by_name(const std::string& name) const;
    const ImageSampler* find_image_sampler_by_slot(int slot) const;
};

} // namespace reflection
