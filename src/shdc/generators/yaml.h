#pragma once
#include "bare.h"

namespace shdc::gen {

class YamlGenerator: public BareGenerator {
public:
    virtual ErrMsg generate(const GenInput& gen);
protected:
    virtual std::string uniform_type(refl::Type::Enum e);
    virtual std::string flattened_uniform_type(refl::Type::Enum e);
    virtual std::string image_type(refl::ImageType::Enum e);
    virtual std::string image_sample_type(refl::ImageSampleType::Enum e);
    virtual std::string sampler_type(refl::SamplerType::Enum e);
private:
    void gen_attr(const refl::StageAttr& attr);
    void gen_uniform_block(const GenInput& gen, const refl::UniformBlock& ub);
    void gen_uniform_block_refl(const refl::UniformBlock& ub);
    void gen_storage_buffer(const refl::StorageBuffer& sbuf);
    void gen_image(const refl::Image& img);
    void gen_sampler(const refl::Sampler& smp);
    void gen_image_sampler(const refl::ImageSampler& img_smp);
};

} // namespace
