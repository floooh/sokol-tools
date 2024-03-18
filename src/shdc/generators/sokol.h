#pragma once
#include "generator.h"

namespace shdc::gen {

class SokolGenerator: public Generator {
protected:
    virtual void gen_prolog(const GenInput& gen);
    virtual void gen_epilog(const GenInput& gen);
    virtual std::string comment_block_start();
    virtual std::string comment_block_line(const std::string& l);
    virtual std::string comment_block_end();
    virtual std::string lang_name();
    virtual std::string get_shader_desc_help(const std::string& mod_prefix, const std::string& prog_name);
    virtual std::string to_uniform_type(refl::Uniform::Type t);
    virtual std::string to_flattened_uniform_type(refl::Uniform::Type t);
    virtual std::string to_image_type(refl::ImageType::Enum e);
    virtual std::string to_image_sample_type(refl::ImageSampleType::Enum e);
    virtual std::string to_sampler_type(refl::SamplerType::Enum e);
    virtual std::string to_backend(Slang::Enum e);
    virtual std::string to_struct_name(const std::string& mod_prefix, const std::string& name);
    virtual std::string to_vertex_attr_name(const std::string& mod_prefix, const std::string& snippet_name, const refl::VertexAttr& attr);
    virtual std::string to_image_bind_slot_name(const std::string& mod_prefix, const refl::Image& img);
    virtual std::string to_sampler_bind_slot_name(const std::string& mod_prefix, const refl::Sampler& smp);
    virtual std::string to_uniform_block_bind_slot_name(const std::string& mod_prefix, const refl::UniformBlock& ub);
    virtual std::string to_vertex_attr_definition(const std::string& mod_prefix, const std::string& snippet_name, const refl::VertexAttr& attr);
    virtual std::string to_image_bind_slot_definition(const std::string& mod_prefix, const refl::Image& img);
    virtual std::string to_sampler_bind_slot_definition(const std::string& mod_prefix, const refl::Sampler& smp);
    virtual std::string to_uniform_block_bind_slot_definition(const std::string& mod_prefix, const refl::UniformBlock& ub);
};

} // namespace
