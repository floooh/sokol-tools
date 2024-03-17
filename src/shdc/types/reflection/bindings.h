#pragma once
#include "uniform_block.h"
#include "image.h"
#include "sampler.h"
#include "image_sampler.h"

namespace shdc::refl {

struct Bindings {
    std::vector<UniformBlock> uniform_blocks;
    std::vector<Image> images;
    std::vector<Sampler> samplers;
    std::vector<ImageSampler> image_samplers;

    const UniformBlock* find_uniform_block_by_slot(int slot) const;
    const Image* find_image_by_slot(int slot) const;
    const Sampler* find_sampler_by_slot(int slot) const;
    const ImageSampler* find_image_sampler_by_slot(int slot) const;

    const UniformBlock* find_uniform_block_by_name(const std::string& name) const;
    const Image* find_image_by_name(const std::string& name) const;
    const Sampler* find_sampler_by_name(const std::string& name) const;
    const ImageSampler* find_image_sampler_by_name(const std::string& name) const;
};

inline const UniformBlock* Bindings::find_uniform_block_by_slot(int slot) const {
    for (const UniformBlock& ub: this->uniform_blocks) {
        if (ub.slot == slot) {
            return &ub;
        }
    }
    return nullptr;
}

inline const Image* Bindings::find_image_by_slot(int slot) const {
    for (const Image& img: this->images) {
        if (img.slot == slot) {
            return &img;
        }
    }
    return nullptr;
}

inline const Sampler* Bindings::find_sampler_by_slot(int slot) const {
    for (const Sampler& smp: this->samplers) {
        if (smp.slot == slot) {
            return &smp;
        }
    }
    return nullptr;
}

inline const ImageSampler* Bindings::find_image_sampler_by_slot(int slot) const {
    for (const ImageSampler& img_smp: this->image_samplers) {
        if (img_smp.slot == slot) {
            return &img_smp;
        }
    }
    return nullptr;
}

inline const UniformBlock* Bindings::find_uniform_block_by_name(const std::string& name) const {
    for (const UniformBlock& ub: this->uniform_blocks) {
        if (ub.struct_name == name) {
            return &ub;
        }
    }
    return nullptr;
}

inline const Image* Bindings::find_image_by_name(const std::string& name) const {
    for (const Image& img: this->images) {
        if (img.name == name) {
            return &img;
        }
    }
    return nullptr;
}

inline const Sampler* Bindings::find_sampler_by_name(const std::string& name) const {
    for (const Sampler& smp: this->samplers) {
        if (smp.name == name) {
            return &smp;
        }
    }
    return nullptr;
}

inline const ImageSampler* Bindings::find_image_sampler_by_name(const std::string& name) const {
    for (const ImageSampler& img_smp: this->image_samplers) {
        if (img_smp.name == name) {
            return &img_smp;
        }
    }
    return nullptr;
}

} // namespace
