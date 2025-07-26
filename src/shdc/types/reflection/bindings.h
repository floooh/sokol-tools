#pragma once
#include "uniform_block.h"
#include "texture.h"
#include "sampler.h"
#include "texture_sampler.h"
#include "storage_buffer.h"
#include "storage_image.h"

namespace shdc::refl {

struct Bindings {
    // keep these in sync with:
    //  - SG_MAX_VIEW_BINDSLOTS
    //  - SG_MAX_UNIFORMBLOCKS
    //  - SG_MAX_TEXTURE_BINDINGS_PER_STAGE
    //  - SG_MAX_STORAGEBUFFER_BINDINGS_PER_STAGE
    //  - SG_MAX_STORAGEIMAGE_BINDINGS_PER_STAGE
    //  - SG_MAX_SAMPLER_BINDSLOTS
    //  - SG_MAX_TEXTURE_SAMPLERS_PAIRS
    //
    inline static const int MaxUniformBlocks = 8;
    inline static const int MaxSamplers = 16;
    inline static const int MaxTextureBindingsPerStage = 16;
    inline static const int MaxStorageBufferBindingsPerStage = 8;
    inline static const int MaxStorageImageBindingsPerStage = 4;
    inline static const int MaxViews = 28;
    inline static const int MaxTextureSamplers = 16;
    static_assert(MaxViews == MaxTextureBindingsPerStage + MaxStorageBufferBindingsPerStage + MaxStorageImageBindingsPerStage);

    enum Type {
        INVALID,
        UNIFORM_BLOCK,
        TEXTURE,
        SAMPLER,
        STORAGE_BUFFER,
        STORAGE_IMAGE,
        TEXTURE_SAMPLER,
    };

    struct View {
        Type type = INVALID;
        struct Texture texture;
        struct StorageBuffer storage_buffer;
        struct StorageImage storage_image;
    };

    std::vector<UniformBlock> uniform_blocks;
    std::vector<StorageBuffer> storage_buffers;
    std::vector<StorageImage> storage_images;
    std::vector<Texture> textures;
    std::vector<Sampler> samplers;
    std::vector<TextureSampler> texture_samplers;

    static uint32_t base_slot(Slang::Enum slang, ShaderStage::Enum stage, Type type, bool readonly=true);

    View get_view_by_sokol_slot(int slot) const;

    const UniformBlock* find_uniform_block_by_sokol_slot(int slot) const;
    const StorageBuffer* find_storage_buffer_by_sokol_slot(int slot) const;
    const StorageImage* find_storage_image_by_sokol_slot(int slot) const;
    const Texture* find_texture_by_sokol_slot(int slot) const;
    const Sampler* find_sampler_by_sokol_slot(int slot) const;
    const TextureSampler* find_texture_sampler_by_sokol_slot(int slot) const;

    const UniformBlock* find_uniform_block_by_name(const std::string& name) const;
    const StorageBuffer* find_storage_buffer_by_name(const std::string& name) const;
    const StorageImage* find_storage_image_by_name(const std::string& name) const;
    const Texture* find_texture_by_name(const std::string& name) const;
    const Sampler* find_sampler_by_name(const std::string& name) const;
    const TextureSampler* find_texture_sampler_by_name(const std::string& name) const;

    void dump_debug(const std::string& indent) const;
};

// returns the 3D API specific base-binding slot for a shader dialect, stage and resource type
// NOTE: the special Slang::REFLECTION always returns zero, this can be used
// to figure out the sokol-gfx bindslots
inline uint32_t Bindings::base_slot(Slang::Enum slang, ShaderStage::Enum stage, Type type, bool readonly) {
    int res = 0;
    switch (type) {
        case Type::UNIFORM_BLOCK:
            if (Slang::is_wgsl(slang)) {
                res = ShaderStage::is_fs(stage) ? MaxUniformBlocks: 0;
            }
            break;
        case Type::TEXTURE_SAMPLER:
            if (Slang::is_glsl(slang)) {
                res = ShaderStage::is_fs(stage) ? MaxTextureSamplers: 0;
            }
            break;
        case Type::TEXTURE:
            if (Slang::is_wgsl(slang)) {
                if (ShaderStage::is_fs(stage)) {
                    res += 64;
                }
            }
            break;
        case Type::SAMPLER:
            if (Slang::is_wgsl(slang)) {
                res = 32;
                if (ShaderStage::is_fs(stage)) {
                    res += 64;
                }
            }
            break;
        case Type::STORAGE_BUFFER:
            if (Slang::is_msl(slang)) {
                res = MaxUniformBlocks;
            } else if (Slang::is_hlsl(slang)) {
                if (readonly) {
                    // read-only storage buffers are bound as SRV
                    res = MaxTextureBindingsPerStage;
                } else {
                    // read/write storage buffers are bound as UAV
                    res = 0;
                }
            } else if (Slang::is_wgsl(slang)) {
                if (ShaderStage::is_fs(stage)) {
                    res += 64;
                }
            }
            break;
        case Type::STORAGE_IMAGE:
            // HLSL: assume D3D11.1, which allows for more than 8 UAV slots
            // MSL: uses texture bindslot, but enough bindslot space available to move behind texture bindings
            if (Slang::is_hlsl(slang)) {
                res = MaxStorageBufferBindingsPerStage;
            } else if (Slang::is_msl(slang)) {
                res = MaxTextureBindingsPerStage;
            }
            break;
        default:
            assert(false);
            break;
    }
    return res;
}

inline Bindings::View Bindings::get_view_by_sokol_slot(int slot) const {
    View view;
    const Texture* tex = find_texture_by_sokol_slot(slot);
    const StorageBuffer* sbuf = find_storage_buffer_by_sokol_slot(slot);
    const StorageImage* simg = find_storage_image_by_sokol_slot(slot);
    if (tex) {
        assert((sbuf == nullptr) && (simg == nullptr));
        view.type = TEXTURE;
        view.texture = *tex;
    } else if (sbuf) {
        assert((tex == nullptr) && (simg == nullptr));
        view.type = STORAGE_BUFFER;
        view.storage_buffer = *sbuf;
    } else if (simg) {
        assert((tex == nullptr) && (sbuf == nullptr));
        view.type = STORAGE_IMAGE;
        view.storage_image = *simg;
    }
    return view;
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

inline const StorageImage* Bindings::find_storage_image_by_sokol_slot(int slot) const {
    for (const StorageImage& simg: storage_images) {
        if (simg.sokol_slot == slot) {
            return &simg;
        }
    }
    return nullptr;
}

inline const Texture* Bindings::find_texture_by_sokol_slot(int slot) const {
    for (const Texture& tex: textures) {
        if (tex.sokol_slot == slot) {
            return &tex;
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

inline const TextureSampler* Bindings::find_texture_sampler_by_sokol_slot(int slot) const {
    for (const TextureSampler& tex_smp: texture_samplers) {
        if (tex_smp.sokol_slot == slot) {
            return &tex_smp;
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

inline const StorageImage* Bindings::find_storage_image_by_name(const std::string& name) const {
    for (const StorageImage& simg: storage_images) {
        if (simg.name == name) {
            return &simg;
        }
    }
    return nullptr;
}

inline const Texture* Bindings::find_texture_by_name(const std::string& name) const {
    for (const Texture& tex: textures) {
        if (tex.name == name) {
            return &tex;
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

inline const TextureSampler* Bindings::find_texture_sampler_by_name(const std::string& name) const {
    for (const TextureSampler& tex_smp: texture_samplers) {
        if (tex_smp.name == name) {
            return &tex_smp;
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
    fmt::print(stderr, "{}storage_images:\n", indent);
    for (const auto& simg: storage_images) {
        simg.dump_debug(indent);
    }
    fmt::print(stderr, "{}textures:\n", indent);
    for (const auto& tex: textures) {
        tex.dump_debug(indent);
    }
    fmt::print(stderr, "{}samplers:\n", indent);
    for (const auto& smp: samplers) {
        smp.dump_debug(indent);
    }
    fmt::print(stderr, "{}texture_samplers:\n", indent);
    for (const auto& tex_smp: texture_samplers) {
        tex_smp.dump_debug(indent);
    }
}

} // namespace
