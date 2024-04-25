#pragma once
#include "generator.h"

namespace shdc::gen {

class SokolNimGenerator: public Generator {
protected:
    virtual void gen_shader_arrays(const GenInput& gen);

    virtual void gen_prolog(const GenInput& gen);
    virtual void gen_epilog(const GenInput& gen);
    virtual void gen_prerequisites(const GenInput& gen);
    virtual void gen_uniform_block_decl(const GenInput& gen, const refl::UniformBlock& ub);
    virtual void gen_storage_buffer_decl(const GenInput& gen, const refl::StorageBuffer& sbuf);
    virtual void gen_shader_array_start(const GenInput& gen, const std::string& array_name, size_t num_bytes, Slang::Enum slang);
    virtual void gen_shader_array_end(const GenInput& gen);
    virtual void gen_shader_desc_func(const GenInput& gen, const refl::ProgramReflection& prog);
    virtual void gen_attr_slot_refl_func(const GenInput& gen, const refl::ProgramReflection& prog);
    virtual void gen_image_slot_refl_func(const GenInput& gen, const refl::ProgramReflection& prog);
    virtual void gen_sampler_slot_refl_func(const GenInput& gen, const refl::ProgramReflection& progm);
    virtual void gen_uniform_block_slot_refl_func(const GenInput& gen, const refl::ProgramReflection& prog);
    virtual void gen_uniform_block_size_refl_func(const GenInput& gen, const refl::ProgramReflection& prog);
    virtual void gen_uniform_offset_refl_func(const GenInput& gen, const refl::ProgramReflection& prog);
    virtual void gen_uniform_desc_refl_func(const GenInput& gen, const refl::ProgramReflection& prog);
    virtual std::string lang_name();
    virtual std::string comment_block_start();
    virtual std::string comment_block_line_prefix();
    virtual std::string comment_block_end();
    virtual std::string shader_bytecode_array_name(const std::string& snippet_name, Slang::Enum slang);
    virtual std::string shader_source_array_name(const std::string& snippet_name, Slang::Enum slang);
    virtual std::string get_shader_desc_help(const std::string& prog_name);
    virtual std::string uniform_type(refl::Type::Enum e);
    virtual std::string flattened_uniform_type(refl::Type::Enum e);
    virtual std::string image_type(refl::ImageType::Enum e);
    virtual std::string image_sample_type(refl::ImageSampleType::Enum e);
    virtual std::string sampler_type(refl::SamplerType::Enum e);
    virtual std::string backend(Slang::Enum e);
    virtual std::string struct_name(const std::string& name);
    virtual std::string vertex_attr_name(const std::string& snippet_name, const refl::StageAttr& attr);
    virtual std::string image_bind_slot_name(const refl::Image& img);
    virtual std::string sampler_bind_slot_name(const refl::Sampler& smp);
    virtual std::string uniform_block_bind_slot_name(const refl::UniformBlock& ub);
    virtual std::string storage_buffer_bind_slot_name(const refl::StorageBuffer& sbuf);
    virtual std::string vertex_attr_definition(const std::string& snippet_name, const refl::StageAttr& attr);
    virtual std::string image_bind_slot_definition(const refl::Image& img);
    virtual std::string sampler_bind_slot_definition(const refl::Sampler& smp);
    virtual std::string uniform_block_bind_slot_definition(const refl::UniformBlock& ub);
    virtual std::string storage_buffer_bind_slot_definition(const refl::StorageBuffer& sbuf);
private:
    virtual void gen_struct_interior_decl_std430(const GenInput& gen, const refl::Type& struc, const std::string& name, int alignment, int pad_to_size);
    virtual void recurse_unfold_structs(const GenInput& gen, const refl::Type& struc, const std::string& name, int alignment, int pad_to_size);
};

} // namespace
