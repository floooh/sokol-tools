/*
    translate SPIRV bytecode to shader sources and generate
    uniform block reflection information, wrapper around
    https://github.com/KhronosGroup/SPIRV-Cross
*/
#include "spirvcross.h"
#include "reflection.h"
#include "types/option.h"
#include "fmt/format.h"
#include "pystring.h"
#include "spirv_hlsl.hpp"
#include "spirv_msl.hpp"
#include "tint/tint.h"

#include "spirv_glsl.hpp"

static const int wgsl_vs_ub_bind_offset = 0;
static const int wgsl_fs_ub_bind_offset = 4;
static const int wgsl_vs_img_bind_offset = 0;
static const int wgsl_vs_smp_bind_offset = 16;
static const int wgsl_fs_img_bind_offset = 32;
static const int wgsl_fs_smp_bind_offset = 48;

using namespace spirv_cross;

namespace shdc {

using namespace refl;

int Spirvcross::find_source_by_snippet_index(int snippet_index) const {
    for (int i = 0; i < (int)sources.size(); i++) {
        if (sources[i].snippet_index == snippet_index) {
            return i;
        }
    }
    return -1;
}

static void fix_ub_matrix_force_colmajor(Compiler& compiler) {
    /* go though all uniform block matrixes and decorate them with
        column-major, this is needed in the HLSL backend to fix the
        multiplication order
    */
    ShaderResources res = compiler.get_shader_resources();
    for (const Resource& ub_res: res.uniform_buffers) {
        const SPIRType& ub_type = compiler.get_type(ub_res.base_type_id);
        for (int m_index = 0; m_index < (int)ub_type.member_types.size(); m_index++) {
            const SPIRType& m_type = compiler.get_type(ub_type.member_types[m_index]);
            if ((m_type.basetype == SPIRType::Float) && (m_type.vecsize > 1) && (m_type.columns > 1)) {
                compiler.set_member_decoration(ub_res.base_type_id, m_index, spv::DecorationColMajor);
            }
        }
    }
}

static void fix_bind_slots(Compiler& compiler, Snippet::Type type, Slang::Enum slang) {
    ShaderResources shader_resources = compiler.get_shader_resources();

    // uniform buffers
    {
        uint32_t binding = 0;
        for (const Resource& res: shader_resources.uniform_buffers) {
            compiler.set_decoration(res.id, spv::DecorationDescriptorSet, 0);
            compiler.set_decoration(res.id, spv::DecorationBinding, binding++);
        }
    }

    // combined image samplers
    {
        uint32_t binding = 0;
        for (const Resource& res: shader_resources.sampled_images) {
            compiler.set_decoration(res.id, spv::DecorationDescriptorSet, 0);
            compiler.set_decoration(res.id, spv::DecorationBinding, binding++);
        }
    }

    // separate images
    {
        uint32_t binding = 0;
        for (const Resource& res: shader_resources.separate_images) {
            compiler.set_decoration(res.id, spv::DecorationDescriptorSet, 0);
            compiler.set_decoration(res.id, spv::DecorationBinding, binding++);
        }
    }

    // separate samplers
    {
        uint32_t binding = 0;
        for (const Resource& res: shader_resources.separate_samplers) {
            compiler.set_decoration(res.id, spv::DecorationDescriptorSet, 0);
            compiler.set_decoration(res.id, spv::DecorationBinding, binding++);
        }
    }
}

// This directly patches the descriptor set and bindslot decorators in the input SPIRV
// via SPIRVCross helper functions. This patched SPIRV is then used as input to Tint
// for the SPIRV-to-WGSL translation.
static void wgsl_patch_bind_slots(Compiler& compiler, Snippet::Type type, std::vector<uint32_t>& inout_bytecode) {
    ShaderResources shader_resources = compiler.get_shader_resources();

    // WGPU bindgroups and binding offsets are hardwired:
    //  - bindgroup 0 for uniform buffer bindings
    //      - vertex stage bindings start at 0
    //      - fragment stage bindings start at 4
    //  - bindgroup 1 for all images and samplers
    //      - vertex stage image bindings start at 0
    //      - vertex stage sampler bindings start at 16
    //      - fragment stage image bindings start at 32
    //      - fragment stage sampler bindings start at 48
    const uint32_t ub_bindgroup = 0;
    const uint32_t res_bindgroup = 1;
    uint32_t cur_ub_binding = Snippet::is_vs(type) ? wgsl_vs_ub_bind_offset : wgsl_fs_ub_bind_offset;
    uint32_t cur_image_binding = Snippet::is_vs(type) ? wgsl_vs_img_bind_offset : wgsl_fs_img_bind_offset;
    uint32_t cur_sampler_binding = Snippet::is_vs(type) ? wgsl_vs_smp_bind_offset : wgsl_fs_smp_bind_offset;

    // uniform buffers
    {
        for (const Resource& res: shader_resources.uniform_buffers) {
            uint32_t out_offset = 0;
            if (compiler.get_binary_offset_for_decoration(res.id, spv::DecorationDescriptorSet, out_offset)) {
                inout_bytecode[out_offset] = ub_bindgroup;
            } else {
                // FIXME handle error
            }
            if (compiler.get_binary_offset_for_decoration(res.id, spv::DecorationBinding, out_offset)) {
                inout_bytecode[out_offset] = cur_ub_binding;
                cur_ub_binding += 1;
            } else {
                // FIXME: handle error
            }
        }
    }

    // separate images
    {
        for (const Resource& res: shader_resources.separate_images) {
            uint32_t out_offset = 0;
            if (compiler.get_binary_offset_for_decoration(res.id, spv::DecorationDescriptorSet, out_offset)) {
                inout_bytecode[out_offset] = res_bindgroup;
            } else {
                // FIXME: handle error
            }
            if (compiler.get_binary_offset_for_decoration(res.id, spv::DecorationBinding, out_offset)) {
                inout_bytecode[out_offset] = cur_image_binding;
                cur_image_binding += 1;
            } else {
                // FIXME: handle error
            }
        }
    }

    // separate samplers
    {
        for (const Resource& res: shader_resources.separate_samplers) {
            uint32_t out_offset = 0;
            if (compiler.get_binary_offset_for_decoration(res.id, spv::DecorationDescriptorSet, out_offset)) {
                inout_bytecode[out_offset] = res_bindgroup;
            } else {
                // FIXME: handle error
            }
            if (compiler.get_binary_offset_for_decoration(res.id, spv::DecorationBinding, out_offset)) {
                inout_bytecode[out_offset] = cur_sampler_binding;
                cur_sampler_binding += 1;
            } else {
                // FIXME: handle error
            }
        }
    }
}

static ErrMsg validate_uniform_blocks_and_separate_image_samplers(const Input& inp, const SpirvBlob& blob) {
    CompilerGLSL compiler(blob.bytecode);
    ShaderResources res = compiler.get_shader_resources();
    for (const Resource& ub_res: res.uniform_buffers) {
        const SPIRType& ub_type = compiler.get_type(ub_res.base_type_id);
        for (int m_index = 0; m_index < (int)ub_type.member_types.size(); m_index++) {
            const SPIRType& m_type = compiler.get_type(ub_type.member_types[m_index]);
            if ((m_type.basetype != SPIRType::Float) && (m_type.basetype != SPIRType::Int)) {
                return ErrMsg::error(inp.base_path, 0, fmt::format("uniform block '{}': uniform blocks can only contain float or int base types", ub_res.name));
            }
            if (m_type.array.size() > 0) {
                if (m_type.vecsize != 4) {
                    return ErrMsg::error(inp.base_path, 0, fmt::format("uniform block '{}': arrays must be of type vec4[], ivec4[] or mat4[]", ub_res.name));
                }
                if (m_type.array.size() > 1) {
                    return ErrMsg::error(inp.base_path, 0, fmt::format("uniform block '{}': arrays must be 1-dimensional", ub_res.name));
                }
            }
        }
    }
    if (res.sampled_images.size() > 0) {
        return ErrMsg::error(inp.base_path, 0, fmt::format("combined image sampler '{}' detected, please use separate textures and samplers", res.sampled_images[0].name));
    }
    return ErrMsg();
}

bool Spirvcross::can_flatten_uniform_block(const Compiler& compiler, const Resource& ub_res) {
    const SPIRType& ub_type = compiler.get_type(ub_res.base_type_id);
    SPIRType::BaseType basic_type = SPIRType::Unknown;
    for (int m_index = 0; m_index < (int)ub_type.member_types.size(); m_index++) {
        const SPIRType& m_type = compiler.get_type(ub_type.member_types[m_index]);
        if (basic_type == SPIRType::Unknown) {
            basic_type = m_type.basetype;
            if ((basic_type != SPIRType::Float) && (basic_type != SPIRType::Int)) {
                return false;
            }
        } else if (basic_type != m_type.basetype) {
            return false;
        }
    }
    return true;
}

static void flatten_uniform_blocks(CompilerGLSL& compiler) {
    /* this flattens each uniform block into a vec4 array, in WebGL/GLES2 this
        allows more efficient uniform updates
    */
    ShaderResources res = compiler.get_shader_resources();
    for (const Resource& ub_res: res.uniform_buffers) {
        if (Spirvcross::can_flatten_uniform_block(compiler, ub_res)) {
            compiler.flatten_buffer_block(ub_res.id);
        }
    }
}

static void to_combined_image_samplers(CompilerGLSL& compiler) {
    compiler.build_combined_image_samplers();
    // give the combined samplers new names
    uint32_t binding = 0;
    for (auto& remap: compiler.get_combined_image_samplers()) {
        const std::string img_name = compiler.get_name(remap.image_id);
        const std::string smp_name = compiler.get_name(remap.sampler_id);
        compiler.set_name(remap.combined_id, pystring::join("_", { img_name, smp_name }));
        compiler.set_decoration(remap.combined_id, spv::DecorationBinding, binding++);
    }
}

static StageReflection to_glsl_and_parse_reflection(const std::vector<uint32_t>& bytecode, const Snippet& snippet, Slang::Enum slang) {
    // use a separate CompilerGLSL instance to parse reflection, this is used
    // for HLSL, MSL and WGSL output and avoids the generation of dummy samplers
    CompilerGLSL compiler(bytecode);
    CompilerGLSL::Options options;
    options.emit_line_directives = false;
    options.version = 430;
    options.es = false;
    options.vulkan_semantics = false;
    options.enable_420pack_extension = false;
    options.emit_uniform_buffer_as_plain_uniforms = true;
    compiler.set_common_options(options);
    flatten_uniform_blocks(compiler);
    to_combined_image_samplers(compiler);
    fix_bind_slots(compiler, snippet.type, slang);
    fix_ub_matrix_force_colmajor(compiler);
    // NOTE: we need to compile here, otherwise the reflection won't be
    // able to detect depth-textures and comparison-samplers!
    compiler.compile();
    return Reflection::parse_snippet_reflection(compiler, snippet, slang);
}

static SpirvcrossSource to_glsl(const Input& inp, const SpirvBlob& blob, Slang::Enum slang, uint32_t opt_mask, const Snippet& snippet) {
    CompilerGLSL compiler(blob.bytecode);
    CompilerGLSL::Options options;
    options.emit_line_directives = false;
    switch (slang) {
        case Slang::GLSL410:
            options.version = 410;
            options.es = false;
            break;
        case Slang::GLSL430:
            options.version = 430;
            options.es = false;
            break;
        case Slang::GLSL300ES:
            options.version = 300;
            options.es = true;
            break;
        default:
            // can't happen
            assert(false);
            break;
    }
    options.vulkan_semantics = false;
    options.enable_420pack_extension = false;
    options.emit_uniform_buffer_as_plain_uniforms = true;
    options.vertex.fixup_clipspace = (0 != (opt_mask & Option::FIXUP_CLIPSPACE));
    options.vertex.flip_vert_y = (0 != (opt_mask & Option::FLIP_VERT_Y));
    compiler.set_common_options(options);
    flatten_uniform_blocks(compiler);
    to_combined_image_samplers(compiler);
    fix_bind_slots(compiler, snippet.type, slang);
    fix_ub_matrix_force_colmajor(compiler);
    std::string src = compiler.compile();
    SpirvcrossSource res;
    res.snippet_index = blob.snippet_index;
    if (!src.empty()) {
        res.valid = true;
        res.source_code = std::move(src);
        res.stage_refl = Reflection::parse_snippet_reflection(compiler, snippet, slang);
    }
    return res;
}

static SpirvcrossSource to_hlsl(const Input& inp, const SpirvBlob& blob, Slang::Enum slang, uint32_t opt_mask, const Snippet& snippet) {
    CompilerHLSL compiler(blob.bytecode);
    CompilerGLSL::Options commonOptions;
    commonOptions.emit_line_directives = false;
    commonOptions.vertex.fixup_clipspace = (0 != (opt_mask & Option::FIXUP_CLIPSPACE));
    commonOptions.vertex.flip_vert_y = (0 != (opt_mask & Option::FLIP_VERT_Y));
    compiler.set_common_options(commonOptions);
    CompilerHLSL::Options hlslOptions;
    switch (slang) {
        case Slang::HLSL4:
            hlslOptions.shader_model = 40;
            break;
        default:
            hlslOptions.shader_model = 50;
            break;
    }
    hlslOptions.point_size_compat = true;
    compiler.set_hlsl_options(hlslOptions);
    fix_bind_slots(compiler, snippet.type, slang);
    fix_ub_matrix_force_colmajor(compiler);
    std::string src = compiler.compile();
    SpirvcrossSource res;
    res.snippet_index = blob.snippet_index;
    if (!src.empty()) {
        res.valid = true;
        res.source_code = std::move(src);
        res.stage_refl = to_glsl_and_parse_reflection(blob.bytecode, snippet, slang);
    }
    return res;
}

static SpirvcrossSource to_msl(const Input& inp, const SpirvBlob& blob, Slang::Enum slang, uint32_t opt_mask, const Snippet& snippet) {
    CompilerMSL compiler(blob.bytecode);
    CompilerGLSL::Options commonOptions;
    commonOptions.emit_line_directives = false;
    commonOptions.vertex.fixup_clipspace = (0 != (opt_mask & Option::FIXUP_CLIPSPACE));
    commonOptions.vertex.flip_vert_y = (0 != (opt_mask & Option::FLIP_VERT_Y));
    compiler.set_common_options(commonOptions);
    CompilerMSL::Options mslOptions;
    switch (slang) {
        case Slang::METAL_MACOS:
            mslOptions.platform = CompilerMSL::Options::macOS;
            break;
        default:
            mslOptions.platform = CompilerMSL::Options::iOS;
            break;
    }
    mslOptions.enable_decoration_binding = true;
    compiler.set_msl_options(mslOptions);
    fix_bind_slots(compiler, snippet.type, slang);
    std::string src = compiler.compile();
    SpirvcrossSource res;
    res.snippet_index = blob.snippet_index;
    if (!src.empty()) {
        res.valid = true;
        res.source_code = std::move(src);
        res.stage_refl = to_glsl_and_parse_reflection(blob.bytecode, snippet, slang);
        // Metal's entry point function are called main0() because main() is reserved
        res.stage_refl.entry_point += "0";
    }
    return res;
}

static SpirvcrossSource to_wgsl(const Input& inp, const SpirvBlob& blob, Slang::Enum slang, uint32_t opt_mask, const Snippet& snippet) {
    std::vector<uint32_t> patched_bytecode = blob.bytecode;
    CompilerGLSL compiler_temp(blob.bytecode);
    fix_bind_slots(compiler_temp, snippet.type, slang);
    wgsl_patch_bind_slots(compiler_temp, snippet.type, patched_bytecode);
    SpirvcrossSource res;
    res.snippet_index = blob.snippet_index;
    tint::reader::spirv::Options spirv_options;
    spirv_options.allow_non_uniform_derivatives = true; // FIXME? => this allow texture sample calls inside dynamic if blocks
    tint::Program program = tint::reader::spirv::Parse(patched_bytecode, spirv_options);
    if (!program.Diagnostics().contains_errors()) {
        const tint::writer::wgsl::Options wgsl_options;
        tint::writer::wgsl::Result result = tint::writer::wgsl::Generate(&program, wgsl_options);
        if (result.success) {
            res.valid = true;
            res.source_code = result.wgsl;
            res.stage_refl = to_glsl_and_parse_reflection(blob.bytecode, snippet, slang);
        } else {
            res.error = inp.error(blob.snippet_index, result.error);
        }
    } else {
        res.error = inp.error(blob.snippet_index, program.Diagnostics().str());
    }
    return res;
}

struct SnippetRefls {
    const Snippet& vs_snippet;
    const Snippet& fs_snippet;
    const StageReflection vs_refl;
    const StageReflection fs_refl;
};

Spirvcross Spirvcross::translate(const Input& inp, const Spirv& spirv, Slang::Enum slang) {
    Spirvcross spv_cross;
    for (const auto& blob: spirv.blobs) {
        SpirvcrossSource src;
        uint32_t opt_mask = inp.snippets[blob.snippet_index].options[(int)slang];
        const Snippet& snippet = inp.snippets[blob.snippet_index];
        assert((snippet.type == Snippet::VS) || (snippet.type == Snippet::FS));
        spv_cross.error = validate_uniform_blocks_and_separate_image_samplers(inp, blob);
        if (spv_cross.error.valid()) {
            return spv_cross;
        }
        switch (slang) {
            case Slang::GLSL410:
            case Slang::GLSL430:
            case Slang::GLSL300ES:
                src = to_glsl(inp, blob, slang, opt_mask, snippet);
                break;
            case Slang::HLSL4:
            case Slang::HLSL5:
                src = to_hlsl(inp, blob, slang, opt_mask, snippet);
                break;
            case Slang::METAL_MACOS:
            case Slang::METAL_IOS:
            case Slang::METAL_SIM:
                src = to_msl(inp, blob, slang, opt_mask, snippet);
                break;
            case Slang::WGSL:
                src = to_wgsl(inp, blob, slang, opt_mask, snippet);
                break;
            default: break;
        }
        if (src.valid) {
            assert(src.snippet_index == blob.snippet_index);
            spv_cross.sources.push_back(std::move(src));
        } else {
            const int line_index = snippet.lines[0];
            std::string err_msg;
            if (src.error.valid()) {
                err_msg = fmt::format("Failed to cross-compile to {} with:\n{}\n", Slang::to_str(slang), src.error.msg);
            } else {
                err_msg = fmt::format("Failed to cross-compile to {}\n", Slang::to_str(slang));
            }
            spv_cross.error = inp.error(line_index, err_msg);
            return spv_cross;
        }
    }
    return spv_cross;
}

// FIXME: most of this should go into Reflection::dump_debug()
void Spirvcross::dump_debug(ErrMsg::Format err_fmt, Slang::Enum slang) const {
    fmt::print(stderr, "Spirvcross ({}):\n", Slang::to_str(slang));
    if (error.valid()) {
        fmt::print(stderr, "  error: {}\n", error.as_string(err_fmt));
    } else {
        fmt::print(stderr, "  error: not set\n");
    }
    std::vector<std::string> lines;
    for (const SpirvcrossSource& source: sources) {
        fmt::print(stderr, "    source for snippet {}:\n", source.snippet_index);
        pystring::splitlines(source.source_code, lines);
        for (const std::string& line: lines) {
            fmt::print(stderr, "      {}\n", line);
        }
        fmt::print(stderr, "    reflection for snippet {}:\n", source.snippet_index);
        fmt::print(stderr, "      stage: {}\n", source.stage_refl.stage_name);
        fmt::print(stderr, "      entry: {}\n", source.stage_refl.entry_point);
        fmt::print(stderr, "      inputs:\n");
        for (const StageAttr& attr: source.stage_refl.inputs) {
            if (attr.slot >= 0) {
                fmt::print(stderr, "        {}: slot={}, sem_name={}, sem_index={}\n", attr.name, attr.slot, attr.sem_name, attr.sem_index);
            }
        }
        fmt::print(stderr, "      outputs:\n");
        for (const StageAttr& attr: source.stage_refl.outputs) {
            if (attr.slot >= 0) {
                fmt::print(stderr, "        {}: slot={}, sem_name={}, sem_index={}\n", attr.name, attr.slot, attr.sem_name, attr.sem_index);
            }
        }
        for (const UniformBlock& ub: source.stage_refl.bindings.uniform_blocks) {
            fmt::print(stderr, "      uniform block: {}, slot: {}, size: {}\n", ub.struct_name, ub.slot, ub.size);
            for (const Uniform& uniform: ub.uniforms) {
                fmt::print(stderr, "          member: {}, type: {}, array_count: {}, offset: {}\n",
                    uniform.name,
                    Uniform::type_to_str(uniform.type),
                    uniform.array_count,
                    uniform.offset);
            }
        }
        for (const Image& img: source.stage_refl.bindings.images) {
            fmt::print(stderr, "      image: {}, slot: {}, type: {}, sampletype: {}\n",
                img.name, img.slot, ImageType::to_str(img.type), ImageSampleType::to_str(img.sample_type));
        }
        for (const Sampler& smp: source.stage_refl.bindings.samplers) {
            fmt::print(stderr, "      sampler: {}, slot: {}\n", smp.name, smp.slot);
        }
        for (const ImageSampler& img_smp: source.stage_refl.bindings.image_samplers) {
            fmt::print(stderr, "      image sampler: {}, slot: {}, image: {}, sampler: {}\n",
                img_smp.name, img_smp.slot, img_smp.image_name, img_smp.sampler_name);
        }
        fmt::print(stderr, "\n");
    }
    fmt::print(stderr, "\n");
}

} // namespace shdc
