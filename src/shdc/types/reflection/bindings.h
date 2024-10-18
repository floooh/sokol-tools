#pragma once
#include "uniform_block.h"
#include "image.h"
#include "sampler.h"
#include "image_sampler.h"
#include "storage_buffer.h"

namespace shdc::refl {

struct Bindings {
    // keep these in sync with:
    //  - SG_MAX_UNIFORMBLOCK_BINDSLOTS
    //  - SG_MAX_IMAGE_BINDSLOTS
    //  - SG_MAX_SAMPLER_BINDSLOTS
    //  - SG_MAX_STORAGEBUFFER_BINDSLOTS
    //  - SG_MAX_IMAGE_SAMPLERS_PAIRS
    //
    inline static const int MaxUniformBlocks = 8;
    inline static const int MaxImages = 16;
    inline static const int MaxSamplers = 16;
    inline static const int MaxStorageBuffers = 8;
    inline static const int MaxImageSamplers = 16;

    enum Type {
        UNIFORM_BLOCK,
        IMAGE,
        SAMPLER,
        STORAGE_BUFFER,
        IMAGE_SAMPLER,
    };

    std::vector<UniformBlock> uniform_blocks;
    std::vector<StorageBuffer> storage_buffers;
    std::vector<Image> images;
    std::vector<Sampler> samplers;
    std::vector<ImageSampler> image_samplers;

    static uint32_t base_slot(Slang::Enum slang, ShaderStage::Enum stage, Type type);

    const UniformBlock* find_uniform_block_by_sokol_slot(int slot) const;
    const StorageBuffer* find_storage_buffer_by_sokol_slot(int slot) const;
    const Image* find_image_by_sokol_slot(int slot) const;
    const Sampler* find_sampler_by_sokol_slot(int slot) const;
    const ImageSampler* find_image_sampler_by_sokol_slot(int slot) const;

    const UniformBlock* find_uniform_block_by_name(const std::string& name) const;
    const StorageBuffer* find_storage_buffer_by_name(const std::string& name) const;
    const Image* find_image_by_name(const std::string& name) const;
    const Sampler* find_sampler_by_name(const std::string& name) const;
    const ImageSampler* find_image_sampler_by_name(const std::string& name) const;

    void dump_debug(const std::string& indent) const;
};

// returns the 3D API specific base-binding slot for a shader dialect, stage and resource type
// NOTE: the special Slang::REFLECTION always returns zero, this can be used
// to figure out the sokol-gfx bindslots
inline uint32_t Bindings::base_slot(Slang::Enum slang, ShaderStage::Enum stage, Type type) {
    int res = 0;
    switch (type) {
        case Type::UNIFORM_BLOCK:
            if (Slang::is_wgsl(slang)) {
                res = ShaderStage::is_vs(stage) ? 0 : MaxUniformBlocks;
            }
            break;
        case Type::IMAGE_SAMPLER:
            if (Slang::is_glsl(slang)) {
                res = ShaderStage::is_vs(stage) ? 0 : MaxImageSamplers;
            }
            break;
        case Type::IMAGE:
            if (Slang::is_wgsl(slang)) {
                if (ShaderStage::is_fs(stage)) {
                    res += 64;
                }
            }
            break;
        case Type::SAMPLER:
            if (Slang::is_wgsl(slang)) {
                res = MaxImages;
                if (ShaderStage::is_fs(stage)) {
                    res += 64;
                }
            }
            break;
        case Type::STORAGE_BUFFER:
            if (Slang::is_msl(slang)) {
                res = MaxUniformBlocks;
            } else if (Slang::is_hlsl(slang)) {
                res = MaxImages;
            } else if (Slang::is_glsl(slang)) {
                if (ShaderStage::is_fs(stage)) {
                    res = MaxStorageBuffers;
                }
            } else if (Slang::is_wgsl(slang)) {
                res = MaxImages + MaxSamplers;
                if (ShaderStage::is_fs(stage)) {
                    res += 64;
                }
            }
            break;
    }
    return res;
}

inline const UniformBlock* Bindings::find_uniform_block_by_sokol_slot(int slot) const {
    for (const UniformBlock& ub: uniform_blocks) {
        if (ub.sokol_slot == slot) {
            return &ub;
        }
    }
    return nullptr;
}

inline const StorageBuffer* Bindings::find_storage_buffer_by_sokol_slot(int slot) const {
    for (const StorageBuffer& sbuf: storage_buffers) {
        if (sbuf.sokol_slot == slot) {
            return &sbuf;
        }
    }
    return nullptr;
}

inline const Image* Bindings::find_image_by_sokol_slot(int slot) const {
    for (const Image& img: images) {
        if (img.sokol_slot == slot) {
            return &img;
        }
    }
    return nullptr;
}

inline const Sampler* Bindings::find_sampler_by_sokol_slot(int slot) const {
    for (const Sampler& smp: samplers) {
        if (smp.sokol_slot == slot) {
            return &smp;
        }
    }
    return nullptr;
}

inline const ImageSampler* Bindings::find_image_sampler_by_sokol_slot(int slot) const {
    for (const ImageSampler& img_smp: image_samplers) {
        if (img_smp.sokol_slot == slot) {
            return &img_smp;
        }
    }
    return nullptr;
}

inline const UniformBlock* Bindings::find_uniform_block_by_name(const std::string& name) const {
    for (const UniformBlock& ub: uniform_blocks) {
        if (ub.name == name) {
            return &ub;
        }
    }
    return nullptr;
}

inline const StorageBuffer* Bindings::find_storage_buffer_by_name(const std::string& name) const {
    for (const StorageBuffer& sbuf: storage_buffers) {
        if (sbuf.name == name) {
            return &sbuf;
        }
    }
    return nullptr;
}

inline const Image* Bindings::find_image_by_name(const std::string& name) const {
    for (const Image& img: images) {
        if (img.name == name) {
            return &img;
        }
    }
    return nullptr;
}

inline const Sampler* Bindings::find_sampler_by_name(const std::string& name) const {
    for (const Sampler& smp: samplers) {
        if (smp.name == name) {
            return &smp;
        }
    }
    return nullptr;
}

inline const ImageSampler* Bindings::find_image_sampler_by_name(const std::string& name) const {
    for (const ImageSampler& img_smp: image_samplers) {
        if (img_smp.name == name) {
            return &img_smp;
        }
    }
    return nullptr;
}

inline void Bindings::dump_debug(const std::string& indent) const {
    fmt::print(stderr, "{}uniform_blocks:\n", indent);
    for (const auto& ub: uniform_blocks) {
        ub.dump_debug(indent);
    }
    fmt::print(stderr, "{}storage_buffers:\n", indent);
    for (const auto& sbuf: storage_buffers) {
        sbuf.dump_debug(indent);
    }
    fmt::print(stderr, "{}images:\n", indent);
    for (const auto& img: images) {
        img.dump_debug(indent);
    }
    fmt::print(stderr, "{}samplers:\n", indent);
    for (const auto& smp: samplers) {
        smp.dump_debug(indent);
    }
    fmt::print(stderr, "{}image_samplers:\n", indent);
    for (const auto& img_smp: image_samplers) {
        img_smp.dump_debug(indent);
    }
}

} // namespace
