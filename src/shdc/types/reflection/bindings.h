#pragma once
#include "uniform_block.h"
#include "image.h"
#include "sampler.h"
#include "image_sampler.h"
#include "storage_buffer.h"

namespace shdc::refl {

struct Bindings {
    std::vector<UniformBlock> uniform_blocks;
    std::vector<StorageBuffer> storage_buffers;
    std::vector<Image> images;
    std::vector<Sampler> samplers;
    std::vector<ImageSampler> image_samplers;

    const UniformBlock* find_uniform_block_by_slot(int slot) const;
    const StorageBuffer* find_storage_buffer_by_slot(int slot) const;
    const Image* find_image_by_slot(int slot) const;
    const Sampler* find_sampler_by_slot(int slot) const;
    const ImageSampler* find_image_sampler_by_slot(int slot) const;

    const UniformBlock* find_uniform_block_by_name(const std::string& name) const;
    const StorageBuffer* find_storage_buffer_by_name(const std::string& name) const;
    const Image* find_image_by_name(const std::string& name) const;
    const Sampler* find_sampler_by_name(const std::string& name) const;
    const ImageSampler* find_image_sampler_by_name(const std::string& name) const;

    void dump_debug(const std::string& indent) const;
};

inline const UniformBlock* Bindings::find_uniform_block_by_slot(int slot) const {
    for (const UniformBlock& ub: uniform_blocks) {
        if (ub.slot == slot) {
            return &ub;
        }
    }
    return nullptr;
}

inline const StorageBuffer* Bindings::find_storage_buffer_by_slot(int slot) const {
    for (const StorageBuffer& sbuf: storage_buffers) {
        if (sbuf.slot == slot) {
            return &sbuf;
        }
    }
    return nullptr;
}

inline const Image* Bindings::find_image_by_slot(int slot) const {
    for (const Image& img: images) {
        if (img.slot == slot) {
            return &img;
        }
    }
    return nullptr;
}

inline const Sampler* Bindings::find_sampler_by_slot(int slot) const {
    for (const Sampler& smp: samplers) {
        if (smp.slot == slot) {
            return &smp;
        }
    }
    return nullptr;
}

inline const ImageSampler* Bindings::find_image_sampler_by_slot(int slot) const {
    for (const ImageSampler& img_smp: image_samplers) {
        if (img_smp.slot == slot) {
            return &img_smp;
        }
    }
    return nullptr;
}

inline const UniformBlock* Bindings::find_uniform_block_by_name(const std::string& name) const {
    for (const UniformBlock& ub: uniform_blocks) {
        if (ub.struct_info.name == name) {
            return &ub;
        }
    }
    return nullptr;
}

inline const StorageBuffer* Bindings::find_storage_buffer_by_name(const std::string& name) const {
    for (const StorageBuffer& sbuf: storage_buffers) {
        if (sbuf.struct_info.name == name) {
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
