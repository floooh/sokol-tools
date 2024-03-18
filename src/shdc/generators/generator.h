#pragma once
#include <string>
#include "types/gen/gen_input.h"

namespace shdc::gen {

// a base class for code generators
class Generator {
public:
    virtual ~Generator() {};
    virtual ErrMsg generate(const GenInput& gen);

protected:
    std::string content;
    std::string mod_prefix;

    // add a line to content
    template<typename... T> void l(fmt::format_string<T...> fmt, T&&... args) {
        content.append(fmt::vformat(fmt, fmt::make_format_args(args...)));
    }
    // add a comment block line to content
    template<typename... T> void cbl(fmt::format_string<T...> fmt, T&&... args) {
        content.append(comment_block_line(fmt::vformat(fmt, fmt::make_format_args(args...))));
    }

    // highlevel code generation methods
    virtual ErrMsg begin(const GenInput& gen);
    virtual void gen_header(const GenInput& gen, Slang::Enum slang);
    virtual void gen_vertex_attrs(const GenInput& gen, Slang::Enum slang);
    virtual void gen_bind_slots(const GenInput& gen);
    virtual void gen_uniform_blocks(const GenInput& gen);
    virtual ErrMsg end(const GenInput& gen);

    virtual void gen_vertex_shader_info(const GenInput& gen, const Program& prog, const SpirvcrossSource& vs_src);
    virtual void gen_fragment_shader_info(const GenInput& gen, const Program& prog, const SpirvcrossSource& fs_src);
    virtual void gen_bindings_info(const GenInput& gen, const refl::Bindings& bindings);

    // override at least those methods in a concrete generator subclass
    virtual void gen_prerequisites(const GenInput& gen) { assert(false && "implement me"); };
    virtual void gen_prolog(const GenInput& gen) { assert(false && "implement me"); };
    virtual void gen_epilog(const GenInput& gen) { assert(false && "implement me"); };
    virtual void gen_uniform_block_decl(const GenInput& gen, const refl::UniformBlock& ub) { assert(false && "implement me"); };

    virtual std::string comment_block_start() { assert(false && "implement me"); return ""; };
    virtual std::string comment_block_line(const std::string& l) { assert(false && "implement me"); return ""; };
    virtual std::string comment_block_end() { assert(false && "implement me"); return ""; };

    virtual std::string lang_name() { assert(false && "implement me"); return ""; };
    virtual std::string get_shader_desc_help(const std::string& prog_name) { assert(false && "implement me"); return ""; };

    virtual std::string to_uniform_type(refl::Uniform::Type t) { assert(false && "implement me"); return ""; };
    virtual std::string to_flattened_uniform_type(refl::Uniform::Type t) { assert(false && "implement me"); return ""; };
    virtual std::string to_image_type(refl::ImageType::Enum e) { assert(false && "implement me"); return ""; };
    virtual std::string to_image_sample_type(refl::ImageSampleType::Enum e) { assert(false && "implement me"); return ""; };
    virtual std::string to_sampler_type(refl::SamplerType::Enum e) { assert(false && "implement me"); return ""; };
    virtual std::string to_backend(Slang::Enum e) { assert(false && "implement me"); return ""; };

    virtual std::string to_struct_name(const std::string& name) { assert(false && "implement me"); return ""; };
    virtual std::string to_vertex_attr_name(const std::string& snippet_name, const refl::VertexAttr& attr) { assert(false && "implement me"); return ""; };
    virtual std::string to_image_bind_slot_name(const refl::Image& img) { assert(false && "implement me"); return ""; };
    virtual std::string to_sampler_bind_slot_name(const refl::Sampler& smp) { assert(false && "implement me"); return ""; };
    virtual std::string to_uniform_block_bind_slot_name(const refl::UniformBlock& ub) { assert(false && "implement me"); return ""; };

    virtual std::string to_vertex_attr_definition(const std::string& snippet_name, const refl::VertexAttr& attr) { assert(false && "implement me"); return ""; };
    virtual std::string to_image_bind_slot_definition(const refl::Image& img) { assert(false && "implement me"); return ""; };
    virtual std::string to_sampler_bind_slot_definition(const refl::Sampler& smp) { assert(false && "implement me"); return ""; };
    virtual std::string to_uniform_block_bind_slot_definition(const refl::UniformBlock& ub) { assert(false && "implement me"); return ""; };

    // utility methods
    static ErrMsg check_errors(const GenInput& gen);
};

} // namespace
