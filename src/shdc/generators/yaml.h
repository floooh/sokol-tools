#pragma once
#include "bare.h"

namespace shdc::gen {

class YamlGenerator: public BareGenerator {
public:
    virtual ErrMsg generate(const GenInput& gen);
protected:
    virtual std::string shader_stage(ShaderStage::Enum e);
    virtual std::string attr_basetype(refl::Type::Enum e);
    virtual std::string uniform_type(refl::Type::Enum e);
    virtual std::string flattened_uniform_type(refl::Type::Enum e);
    virtual std::string image_type(refl::ImageType::Enum e);
    virtual std::string image_sample_type(refl::ImageSampleType::Enum e);
    virtual std::string sampler_type(refl::SamplerType::Enum e);
    virtual std::string storage_pixel_format(refl::StoragePixelFormat::Enum e);
private:
    void gen_attr(const GenInput& gen, const refl::StageAttr& attr, Slang::Enum slang);
    void gen_uniform_block(const GenInput& gen, const refl::UniformBlock& ub, Slang::Enum slang);
    void gen_uniform_block_refl(const refl::UniformBlock& ub);
    void gen_storage_buffer(const refl::StorageBuffer& sbuf, Slang::Enum slang);
    void gen_storage_image(const refl::StorageImage& simg, Slang::Enum slang);
    void gen_texture(const refl::Texture& tex, Slang::Enum slang);
    void gen_sampler(const refl::Sampler& smp, Slang::Enum slang);
    void gen_texture_sampler(const refl::Bindings& bindings, const refl::TextureSampler& tex_smp, Slang::Enum slang);
};

} // namespace
