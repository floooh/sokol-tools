/*
    translate SPIRV bytecode to shader sources and generate
    uniform block reflection information, wrapper around
    https://github.com/KhronosGroup/SPIRV-Cross
*/
#include "shdc.h"
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

// workaround for Compiler.comparison_ids being protected
class UnprotectedCompiler: spirv_cross::Compiler {
public:
    bool is_comparison_sampler(const spirv_cross::SPIRType &type, uint32_t id) {
        if (type.basetype == spirv_cross::SPIRType::Sampler) {
            return comparison_ids.count(id) > 0;
        }
        return 0;
    }
    bool is_used_as_depth_texture(const spirv_cross::SPIRType &type, uint32_t id) {
        if (type.basetype == spirv_cross::SPIRType::Image) {
            return comparison_ids.count(id) > 0;
        }
        return 0;
    }
};

using namespace spirv_cross;

namespace shdc {

int spirvcross_t::find_source_by_snippet_index(int snippet_index) const {
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

static void fix_bind_slots(Compiler& compiler, snippet_t::type_t type, slang_t::type_t slang) {
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
static spirvcross_wgsl_symbol_table_t wgsl_patch_bind_slots(Compiler& compiler, snippet_t::type_t type, std::vector<uint32_t>& inout_bytecode) {
    ShaderResources shader_resources = compiler.get_shader_resources();

    // while at it, also create a name lookup table by bindings, because
    // Tint's inspector doesn't provide this information
    spirvcross_wgsl_symbol_table_t symbols;

    // first add input and output names to symbol table
    for (const Resource& input: shader_resources.stage_inputs) {
        const uint32_t slot = compiler.get_decoration(input.id, spv::DecorationLocation);
        symbols.input_names[slot] = input.name;
    }
    for (const Resource& output: shader_resources.stage_outputs) {
        const uint32_t slot = compiler.get_decoration(output.id, spv::DecorationLocation);
        symbols.output_names[slot] = output.name;
    }

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
    uint32_t cur_ub_binding = snippet_t::is_vs(type) ? wgsl_vs_ub_bind_offset : wgsl_fs_ub_bind_offset;
    uint32_t cur_image_binding = snippet_t::is_vs(type) ? wgsl_vs_img_bind_offset : wgsl_fs_img_bind_offset;
    uint32_t cur_sampler_binding = snippet_t::is_vs(type) ? wgsl_vs_smp_bind_offset : wgsl_fs_smp_bind_offset;

    // uniform buffers
    {
        for (const Resource& res: shader_resources.uniform_buffers) {
            symbols.uniform_block_struct_names[cur_ub_binding] = res.name;
            std::string inst_name = compiler.get_name(res.id);
            if (inst_name.empty()) {
                inst_name = compiler.get_fallback_name(res.id);
            }
            symbols.uniform_block_inst_names[cur_ub_binding] = inst_name;
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
            symbols.image_names[cur_image_binding] = res.name;
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
            symbols.sampler_names[cur_sampler_binding] = res.name;
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
    return symbols;
}

static errmsg_t validate_uniform_blocks_and_separate_image_samplers(const input_t& inp, const spirv_blob_t& blob) {
    CompilerGLSL compiler(blob.bytecode);
    ShaderResources res = compiler.get_shader_resources();
    for (const Resource& ub_res: res.uniform_buffers) {
        const SPIRType& ub_type = compiler.get_type(ub_res.base_type_id);
        for (int m_index = 0; m_index < (int)ub_type.member_types.size(); m_index++) {
            const SPIRType& m_type = compiler.get_type(ub_type.member_types[m_index]);
            if ((m_type.basetype != SPIRType::Float) && (m_type.basetype != SPIRType::Int)) {
                return errmsg_t::error(inp.base_path, 0, fmt::format("uniform block '{}': uniform blocks can only contain float or int base types", ub_res.name));
            }
            if (m_type.array.size() > 0) {
                if (m_type.vecsize != 4) {
                    return errmsg_t::error(inp.base_path, 0, fmt::format("uniform block '{}': arrays must be of type vec4[], ivec4[] or mat4[]", ub_res.name));
                }
                if (m_type.array.size() > 1) {
                    return errmsg_t::error(inp.base_path, 0, fmt::format("uniform block '{}': arrays must be 1-dimensional", ub_res.name));
                }
            }
        }
    }
    if (res.sampled_images.size() > 0) {
        return errmsg_t::error(inp.base_path, 0, fmt::format("combined image sampler '{}' detected, please use separate textures and samplers", res.sampled_images[0].name));
    }
    return errmsg_t();
}

static bool can_flatten_uniform_block(const Compiler& compiler, const Resource& ub_res) {
    const SPIRType& ub_type = compiler.get_type(ub_res.base_type_id);
    SPIRType::BaseType basic_type = SPIRType::Unknown;
    for (int m_index = 0; m_index < (int)ub_type.member_types.size(); m_index++) {
        const SPIRType& m_type = compiler.get_type(ub_type.member_types[m_index]);
        if (basic_type == SPIRType::Unknown) {
            basic_type = m_type.basetype;
            if ((basic_type != SPIRType::Float) && (basic_type != SPIRType::Int)) {
                return false;
            }
        }
        else if (basic_type != m_type.basetype) {
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
        if (can_flatten_uniform_block(compiler, ub_res)) {
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

static uniform_t::type_t spirtype_to_uniform_type(const SPIRType& type) {
    switch (type.basetype) {
        case SPIRType::Float:
            if (type.columns == 1) {
                // scalar or vec
                switch (type.vecsize) {
                    case 1: return uniform_t::FLOAT;
                    case 2: return uniform_t::FLOAT2;
                    case 3: return uniform_t::FLOAT3;
                    case 4: return uniform_t::FLOAT4;
                }
            }
            else {
                // a matrix
                if ((type.vecsize == 4) && (type.columns == 4)) {
                    return uniform_t::MAT4;
                }
            }
            break;
        case SPIRType::Int:
            if (type.columns == 1) {
                switch (type.vecsize) {
                    case 1: return uniform_t::INT;
                    case 2: return uniform_t::INT2;
                    case 3: return uniform_t::INT3;
                    case 4: return uniform_t::INT4;
                }
            }
            break;
        default: break;
    }
    // fallthrough: invalid type
    return uniform_t::INVALID;
}

static image_type_t::type_t spirtype_to_image_type(const SPIRType& type) {
    if (type.image.arrayed) {
        if (type.image.dim == spv::Dim2D) {
            return image_type_t::ARRAY;
        }
    } else {
        switch (type.image.dim) {
            case spv::Dim2D:    return image_type_t::_2D;
            case spv::DimCube:  return image_type_t::CUBE;
            case spv::Dim3D:    return image_type_t::_3D;
            default: break;
        }
    }
    // fallthrough: invalid type
    return image_type_t::INVALID;
}

static image_sample_type_t::type_t spirtype_to_image_sample_type(const SPIRType& type) {
    if (type.image.depth) {
        return image_sample_type_t::DEPTH;
    } else {
        switch (type.basetype) {
            case SPIRType::Int:
            case SPIRType::Short:
            case SPIRType::SByte:
                return image_sample_type_t::SINT;
            case SPIRType::UInt:
            case SPIRType::UShort:
            case SPIRType::UByte:
                return image_sample_type_t::UINT;
            default:
                return image_sample_type_t::FLOAT;
        }
    }
}

static bool spirvtype_to_image_multisampled(const SPIRType& type) {
    return type.image.ms;
}

static spirvcross_refl_t parse_reflection(const Compiler& compiler, const snippet_t& snippet, slang_t::type_t slang) {
    spirvcross_refl_t refl;

    ShaderResources shd_resources = compiler.get_shader_resources();
    // shader stage
    switch (compiler.get_execution_model()) {
        case spv::ExecutionModelVertex:   refl.stage = stage_t::VS; break;
        case spv::ExecutionModelFragment: refl.stage = stage_t::FS; break;
        default: refl.stage = stage_t::INVALID; break;
    }

    int ub_bind_offset = 0;
    int img_bind_offset = 0;
    int smp_bind_offset = 0;
    if (slang_t::is_wgsl(slang)) {
        if (stage_t::is_vs(refl.stage)) {
            ub_bind_offset = wgsl_vs_ub_bind_offset;
            img_bind_offset = wgsl_vs_img_bind_offset;
            smp_bind_offset = wgsl_vs_smp_bind_offset;
        } else {
            ub_bind_offset = wgsl_fs_ub_bind_offset;
            img_bind_offset = wgsl_fs_img_bind_offset;
            smp_bind_offset = wgsl_fs_smp_bind_offset;
        }
    }

    // find entry point
    const auto entry_points = compiler.get_entry_points_and_stages();
    for (const auto& item: entry_points) {
        if (compiler.get_execution_model() == item.execution_model) {
            refl.entry_point = item.name;
            break;
        }
    }
    // stage inputs and outputs
    for (const Resource& res_attr: shd_resources.stage_inputs) {
        attr_t refl_attr;
        refl_attr.slot = compiler.get_decoration(res_attr.id, spv::DecorationLocation);
        refl_attr.name = res_attr.name;
        refl_attr.sem_name = "TEXCOORD";
        refl_attr.sem_index = refl_attr.slot;
        refl.inputs[refl_attr.slot] = refl_attr;
    }
    for (const Resource& res_attr: shd_resources.stage_outputs) {
        attr_t refl_attr;
        refl_attr.slot = compiler.get_decoration(res_attr.id, spv::DecorationLocation);
        refl_attr.name = res_attr.name;
        refl_attr.sem_name = "TEXCOORD";
        refl_attr.sem_index = refl_attr.slot;
        refl.outputs[refl_attr.slot] = refl_attr;
    }
    // uniform blocks
    for (const Resource& ub_res: shd_resources.uniform_buffers) {
        std::string n = compiler.get_name(ub_res.id);
        uniform_block_t refl_ub;
        const SPIRType& ub_type = compiler.get_type(ub_res.base_type_id);
        refl_ub.slot = compiler.get_decoration(ub_res.id, spv::DecorationBinding) - ub_bind_offset;
        refl_ub.size = (int) compiler.get_declared_struct_size(ub_type);
        refl_ub.struct_name = ub_res.name;
        refl_ub.inst_name = compiler.get_name(ub_res.id);
        if (refl_ub.inst_name.empty()) {
            refl_ub.inst_name = compiler.get_fallback_name(ub_res.id);
        }
        refl_ub.flattened = can_flatten_uniform_block(compiler, ub_res);
        for (int m_index = 0; m_index < (int)ub_type.member_types.size(); m_index++) {
            uniform_t refl_uniform;
            refl_uniform.name = compiler.get_member_name(ub_res.base_type_id, m_index);
            const SPIRType& m_type = compiler.get_type(ub_type.member_types[m_index]);
            refl_uniform.type = spirtype_to_uniform_type(m_type);
            if (m_type.array.size() > 0) {
                refl_uniform.array_count = m_type.array[0];
            }
            refl_uniform.offset = compiler.type_struct_member_offset(ub_type, m_index);
            refl_ub.uniforms.push_back(refl_uniform);
        }
        refl.uniform_blocks.push_back(refl_ub);
    }
    // (separate) images
    // FIXME: override from @image_sample_type
    for (const Resource& img_res: shd_resources.separate_images) {
        image_t refl_img;
        refl_img.slot = compiler.get_decoration(img_res.id, spv::DecorationBinding) - img_bind_offset;
        refl_img.name = img_res.name;
        const SPIRType& img_type = compiler.get_type(img_res.type_id);
        refl_img.type = spirtype_to_image_type(img_type);
        if (((UnprotectedCompiler*)&compiler)->is_used_as_depth_texture(img_type, img_res.id)) {
            refl_img.sample_type = image_sample_type_t::DEPTH;
        } else {
            refl_img.sample_type = spirtype_to_image_sample_type(compiler.get_type(img_type.image.type));
        }
        refl_img.multisampled = spirvtype_to_image_multisampled(img_type);
        refl.images.push_back(refl_img);
    }
    // (separate) samplers
    for (const Resource& smp_res: shd_resources.separate_samplers) {
        // ignore generated dummy samplers
        if (pystring::find(smp_res.name, "SPIRV_Cross") != -1) {
            continue;
        }
        const SPIRType& smp_type = compiler.get_type(smp_res.type_id);
        sampler_t refl_smp;
        refl_smp.slot = compiler.get_decoration(smp_res.id, spv::DecorationBinding) - smp_bind_offset;
        refl_smp.name = smp_res.name;
        // HACK ALERT!
        if (((UnprotectedCompiler*)&compiler)->is_comparison_sampler(smp_type, smp_res.id)) {
            refl_smp.type = sampler_type_t::COMPARISON;
        } else {
            refl_smp.type = sampler_type_t::FILTERING;
        }
        refl.samplers.push_back(refl_smp);
    }
    // combined image samplers
    for (auto& img_smp_res: compiler.get_combined_image_samplers()) {
        // ignore any combined image samplers which involve dummy samplers
        // FIXME: this will probably bite us in the ass later (GLSL texelFetch?)
        if (pystring::find(compiler.get_name(img_smp_res.combined_id), "SPIRV_Cross") != -1) {
            continue;
        }
        image_sampler_t refl_img_smp;
        refl_img_smp.slot = compiler.get_decoration(img_smp_res.combined_id, spv::DecorationBinding);
        refl_img_smp.name = compiler.get_name(img_smp_res.combined_id);
        refl_img_smp.image_name = compiler.get_name(img_smp_res.image_id);
        refl_img_smp.sampler_name = compiler.get_name(img_smp_res.sampler_id);
        refl.image_samplers.push_back(refl_img_smp);
    }
    // patch textures with overridden image-sample-types
    for (auto& img: refl.images) {
        const auto* tag = snippet.lookup_image_sample_type_tag(img.name);
        if (tag) {
            img.sample_type = tag->type;
        }
    }
    // patch samplers with overridden sampler-types
    for (auto& smp: refl.samplers) {
        const auto* tag = snippet.lookup_sampler_type_tag(smp.name);
        if (tag) {
            smp.type = tag->type;
        }
    }
    return refl;
}

/*
static spirvcross_refl_t wgsl_parse_reflection(const tint::Program* program, spirvcross_wgsl_symbol_table_t& symbols) {
    spirvcross_refl_t refl;

    auto inspector = tint::inspector::Inspector(program);
    const auto entry_points = inspector.GetEntryPoints();
    assert(entry_points.size() == 1);
    const auto& entry_point = entry_points[0];

    switch (entry_point.stage) {
        case tint::inspector::PipelineStage::kVertex:
            refl.stage = stage_t::VS;
            break;
        case tint::inspector::PipelineStage::kFragment:
            refl.stage = stage_t::FS;
            break;
        default:
            break;
    }
    refl.entry_point = entry_point.name;

    for (const auto& input: entry_point.input_variables) {
        assert(input.has_location_attribute);
        attr_t refl_attr;
        refl_attr.name = symbols.input_names[input.location_attribute];
        refl_attr.slot = (int)input.location_attribute;
        refl.inputs[refl_attr.slot] = refl_attr;
    }
    for (const auto& output: entry_point.output_variables) {
        assert(output.has_location_attribute);
        attr_t refl_attr;
        refl_attr.name = symbols.output_names[output.location_attribute];
        refl_attr.slot = (int)output.location_attribute;
        refl.outputs[refl_attr.slot] = refl_attr;
    }

    // uniform blocks
    for (const auto& ub: inspector.GetUniformBufferResourceBindings(entry_point.name)) {
        uniform_block_t refl_ub;
        refl_ub.slot = ub.binding - (refl.stage == stage_t::VS ? 0 : 4);
        refl_ub.size = (int)ub.size;
        refl_ub.struct_name = symbols.uniform_block_struct_names[ub.binding];
        refl_ub.inst_name = symbols.uniform_block_inst_names[ub.binding];
        refl.uniform_blocks.push_back(refl_ub);
    }

    // image bindings
    // FIXME: sampled vs comparison vs multisampled
    for (const auto& img: inspector.GetSampledTextureResourceBindings(entry_point.name)) {
        image_t refl_img;
        refl_img.slot = img.binding;
        refl_img.name = symbols.image_names[img.binding];
        switch (img.dim) {
            case tint::inspector::ResourceBinding::TextureDimension::k2d:
                refl_img.type = image_t::IMAGE_TYPE_2D;
                break;
            case tint::inspector::ResourceBinding::TextureDimension::kCube:
                refl_img.type = image_t::IMAGE_TYPE_CUBE;
                break;
            case tint::inspector::ResourceBinding::TextureDimension::k3d:
                refl_img.type = image_t::IMAGE_TYPE_3D;
                break;
            case tint::inspector::ResourceBinding::TextureDimension::k2dArray:
                refl_img.type = image_t::IMAGE_TYPE_ARRAY;
                break;
            default:
                break;
        }
        switch (img.sampled_kind) {
            case tint::inspector::ResourceBinding::SampledKind::kFloat:
                refl_img.sample_type = image_t::IMAGE_SAMPLETYPE_FLOAT;
                break;
            case tint::inspector::ResourceBinding::SampledKind::kUInt:
                refl_img.sample_type = image_t::IMAGE_SAMPLETYPE_UINT;
                break;
            case tint::inspector::ResourceBinding::SampledKind::kSInt:
                refl_img.sample_type = image_t::IMAGE_SAMPLETYPE_SINT;
                break;
            default:
                break;
        }
        refl.images.push_back(refl_img);
    }

    // sampler bindings
    for (const auto& smp: inspector.GetSamplerResourceBindings(entry_point.name)) {
        sampler_t refl_smp;
        refl_smp.slot = smp.binding;
        refl_smp.name = symbols.sampler_names[smp.binding];
        // FIXME!
        refl_smp.type = sampler_t::SAMPLER_TYPE_SAMPLE;
        refl.samplers.push_back(refl_smp);
    }

    // FIXME: image-sampler-pairs

    return refl;
}
*/

static spirvcross_source_t to_glsl(const input_t& inp, const spirv_blob_t& blob, slang_t::type_t slang, uint32_t opt_mask, const snippet_t& snippet) {
    CompilerGLSL compiler(blob.bytecode);
    CompilerGLSL::Options options;
    options.emit_line_directives = false;
    switch (slang) {
        case slang_t::GLSL100:
            options.version = 100;
            options.es = true;
            break;
        case slang_t::GLSL300ES:
            options.version = 300;
            options.es = true;
            break;
        default:
            options.version = 330;
            options.es = false;
            break;
    }
    options.vulkan_semantics = false;
    options.enable_420pack_extension = false;
    options.emit_uniform_buffer_as_plain_uniforms = true;
    options.vertex.fixup_clipspace = (0 != (opt_mask & option_t::FIXUP_CLIPSPACE));
    options.vertex.flip_vert_y = (0 != (opt_mask & option_t::FLIP_VERT_Y));
    compiler.set_common_options(options);
    flatten_uniform_blocks(compiler);
    to_combined_image_samplers(compiler);
    fix_bind_slots(compiler, snippet.type, slang);
    fix_ub_matrix_force_colmajor(compiler);
    std::string src = compiler.compile();
    spirvcross_source_t res;
    res.snippet_index = blob.snippet_index;
    if (!src.empty()) {
        res.valid = true;
        res.source_code = std::move(src);
        res.refl = parse_reflection(compiler, snippet, slang);
    }
    return res;
}

static spirvcross_source_t to_hlsl(const input_t& inp, const spirv_blob_t& blob, slang_t::type_t slang, uint32_t opt_mask, const snippet_t& snippet) {
    CompilerHLSL compiler(blob.bytecode);
    CompilerGLSL::Options commonOptions;
    commonOptions.emit_line_directives = false;
    commonOptions.vertex.fixup_clipspace = (0 != (opt_mask & option_t::FIXUP_CLIPSPACE));
    commonOptions.vertex.flip_vert_y = (0 != (opt_mask & option_t::FLIP_VERT_Y));
    compiler.set_common_options(commonOptions);
    CompilerHLSL::Options hlslOptions;
    switch (slang) {
        case slang_t::HLSL4:
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
    spirvcross_source_t res;
    res.snippet_index = blob.snippet_index;
    if (!src.empty()) {
        res.valid = true;
        res.source_code = std::move(src);
        compiler.build_dummy_sampler_for_combined_images();
        to_combined_image_samplers(compiler);
        res.refl = parse_reflection(compiler, snippet, slang);
    }
    return res;
}

static spirvcross_source_t to_msl(const input_t& inp, const spirv_blob_t& blob, slang_t::type_t slang, uint32_t opt_mask, const snippet_t& snippet) {
    CompilerMSL compiler(blob.bytecode);
    CompilerGLSL::Options commonOptions;
    commonOptions.emit_line_directives = false;
    commonOptions.vertex.fixup_clipspace = (0 != (opt_mask & option_t::FIXUP_CLIPSPACE));
    commonOptions.vertex.flip_vert_y = (0 != (opt_mask & option_t::FLIP_VERT_Y));
    compiler.set_common_options(commonOptions);
    CompilerMSL::Options mslOptions;
    switch (slang) {
        case slang_t::METAL_MACOS:
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
    spirvcross_source_t res;
    res.snippet_index = blob.snippet_index;
    if (!src.empty()) {
        res.valid = true;
        res.source_code = std::move(src);
        compiler.build_dummy_sampler_for_combined_images();
        to_combined_image_samplers(compiler);
        res.refl = parse_reflection(compiler, snippet, slang);
        // Metal's entry point function are called main0() because main() is reserved
        res.refl.entry_point += "0";
    }
    return res;
}

static spirvcross_source_t to_wgsl(const input_t& inp, const spirv_blob_t& blob, slang_t::type_t slang, uint32_t opt_mask, const snippet_t& snippet) {
    std::vector<uint32_t> patched_bytecode = blob.bytecode;
    CompilerGLSL compiler_temp(blob.bytecode);
    fix_bind_slots(compiler_temp, snippet.type, slang);
    spirvcross_wgsl_symbol_table_t symbols = wgsl_patch_bind_slots(compiler_temp, snippet.type, patched_bytecode);
    spirvcross_source_t res;
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

            // reactivate this idea at some point
            //res.refl = wgsl_parse_reflection(&program, symbols);

            // NOTE: in order to get correct reflection info out of SPIRVCross (also for depth images etc...)
            // we need to do a dummy compile
            CompilerHLSL compiler_refl(patched_bytecode);
            CompilerGLSL::Options commonOptions;
            commonOptions.emit_line_directives = false;
            commonOptions.vertex.fixup_clipspace = (0 != (opt_mask & option_t::FIXUP_CLIPSPACE));
            commonOptions.vertex.flip_vert_y = (0 != (opt_mask & option_t::FLIP_VERT_Y));
            compiler_refl.set_common_options(commonOptions);
            CompilerHLSL::Options hlslOptions;
            hlslOptions.shader_model = 40;
            hlslOptions.point_size_compat = true;
            compiler_refl.set_hlsl_options(hlslOptions);
            fix_ub_matrix_force_colmajor(compiler_refl);
            compiler_refl.compile();
            compiler_refl.build_dummy_sampler_for_combined_images();
            to_combined_image_samplers(compiler_refl);
            res.refl = parse_reflection(compiler_refl, snippet, slang);
        } else {
            res.error = inp.error(blob.snippet_index, result.error);
        }
    } else {
        res.error = inp.error(blob.snippet_index, program.Diagnostics().str());
    }
    return res;
}

static int find_unique_uniform_block_by_name(const spirvcross_t& spv_cross, const std::string& name) {
    for (int i = 0; i < (int)spv_cross.unique_uniform_blocks.size(); i++) {
        if (spv_cross.unique_uniform_blocks[i].struct_name == name) {
            return i;
        }
    }
    return -1;
}

static int find_unique_image_by_name(const spirvcross_t& spv_cross, const std::string& name) {
    for (int i = 0; i < (int)spv_cross.unique_images.size(); i++) {
        if (spv_cross.unique_images[i].name == name) {
            return i;
        }
    }
    return -1;
}

static int find_unique_sampler_by_name(const spirvcross_t& spv_cross, const std::string& name) {
    for (int i = 0; i < (int)spv_cross.unique_samplers.size(); i++) {
        if (spv_cross.unique_samplers[i].name == name) {
            return i;
        }
    }
    return -1;
}

// find all identical uniform blocks across all shaders, and check for collisions
static bool gather_unique_uniform_blocks(const input_t& inp, spirvcross_t& spv_cross) {
    for (spirvcross_source_t& src: spv_cross.sources) {
        for (uniform_block_t& ub: src.refl.uniform_blocks) {
            int other_ub_index = find_unique_uniform_block_by_name(spv_cross, ub.struct_name);
            if (other_ub_index >= 0) {
                if (ub.equals(spv_cross.unique_uniform_blocks[other_ub_index])) {
                    // identical uniform block already exists, take note of the index
                    ub.unique_index = other_ub_index;
                }
                else {
                    spv_cross.error = errmsg_t::error(inp.base_path, 0, fmt::format("conflicting uniform block definitions found for '{}'", ub.struct_name));
                    return false;
                }
            }
            else {
                // a new unique uniform block
                ub.unique_index = (int) spv_cross.unique_uniform_blocks.size();
                spv_cross.unique_uniform_blocks.push_back(ub);
            }
        }
    }
    return true;
}

// find all identical images across all shaders, and check for collisions
static bool gather_unique_images(const input_t& inp, spirvcross_t& spv_cross) {
    for (spirvcross_source_t& src: spv_cross.sources) {
        for (image_t& img: src.refl.images) {
            int other_img_index = find_unique_image_by_name(spv_cross, img.name);
            if (other_img_index >= 0) {
                if (img.equals(spv_cross.unique_images[other_img_index])) {
                    // identical image already exists, take note of the index
                    img.unique_index = other_img_index;
                } else {
                    spv_cross.error = errmsg_t::error(inp.base_path, 0, fmt::format("conflicting texture definitions found for '{}'", img.name));
                    return false;
                }
            } else {
                // new unique image
                img.unique_index = (int) spv_cross.unique_images.size();
                spv_cross.unique_images.push_back(img);
            }
        }
    }
    return true;
}

// find all identical samplers across all shaders, and check for collisions
static bool gather_unique_samplers(const input_t& inp, spirvcross_t& spv_cross) {
    for (spirvcross_source_t& src: spv_cross.sources) {
        for (sampler_t& smp: src.refl.samplers) {
            int other_smp_index = find_unique_sampler_by_name(spv_cross, smp.name);
            if (other_smp_index >= 0) {
                if (smp.equals(spv_cross.unique_samplers[other_smp_index])) {
                    // identical sampler already exists, take note of the index
                    smp.unique_index = other_smp_index;
                } else {
                    spv_cross.error = errmsg_t::error(inp.base_path, 0, fmt::format("conflicting sampler definitions found for '{}'", smp.name));
                    return false;
                }
            } else {
                // new unique sampler
                smp.unique_index = (int) spv_cross.unique_samplers.size();
                spv_cross.unique_samplers.push_back(smp);
            }
        }
    }
    return true;
}

// check that the vertex shader outputs match the fragment shader inputs for each program
// FIXME: this should also check the attribute's type
static errmsg_t validate_linking(const input_t& inp, const spirvcross_t& spv_cross) {
    for (const auto& prog_item: inp.programs) {
        const program_t& prog = prog_item.second;
        int vs_snippet_index = inp.vs_map.at(prog.vs_name);
        int fs_snippet_index = inp.fs_map.at(prog.fs_name);
        int vs_src_index = spv_cross.find_source_by_snippet_index(vs_snippet_index);
        int fs_src_index = spv_cross.find_source_by_snippet_index(fs_snippet_index);
        assert((vs_src_index >= 0) && (fs_src_index >= 0));
        const spirvcross_source_t& vs_src = spv_cross.sources[vs_src_index];
        const spirvcross_source_t& fs_src = spv_cross.sources[fs_src_index];
        assert(vs_snippet_index == vs_src.snippet_index);
        assert(fs_snippet_index == fs_src.snippet_index);
        for (int i = 0; i < attr_t::NUM; i++) {
            const attr_t& vs_out = vs_src.refl.outputs[i];
            const attr_t& fs_inp = fs_src.refl.inputs[i];
            if (!vs_out.equals(fs_inp)) {
                return inp.error(prog.line_index,
                    fmt::format("outputs of vs '{}' don't match inputs of fs '{}' for attr #{} (vs={},fs={})\n",
                    prog.vs_name, prog.fs_name, i, vs_out.name, fs_inp.name));
            }
        }
    }
    return errmsg_t();
}

static errmsg_t validate_image_sample_type_tags(const input_t& inp, const spirvcross_t& spv_cross) {
    for (const auto& prog_item: inp.programs) {
        const program_t& prog = prog_item.second;
        int vs_snippet_index = inp.vs_map.at(prog.vs_name);
        int fs_snippet_index = inp.fs_map.at(prog.fs_name);
        int vs_src_index = spv_cross.find_source_by_snippet_index(vs_snippet_index);
        int fs_src_index = spv_cross.find_source_by_snippet_index(fs_snippet_index);
        assert((vs_src_index >= 0) && (fs_src_index >= 0));
        const spirvcross_source_t& vs_src = spv_cross.sources[vs_src_index];
        const spirvcross_source_t& fs_src = spv_cross.sources[fs_src_index];
        assert(vs_snippet_index == vs_src.snippet_index);
        assert(fs_snippet_index == fs_src.snippet_index);
        const snippet_t& vs_snippet = inp.snippets[vs_snippet_index];
        const snippet_t& fs_snippet = inp.snippets[fs_snippet_index];
        for (const auto& kvp: vs_snippet.image_sample_type_tags) {
            const auto& tag = kvp.second;
            if (nullptr == util::find_image_by_name(vs_src.refl, tag.tex_name)) {
                return inp.error(tag.line_index, fmt::format("unknown texture name '{}' in @image_sample_type tag\n", tag.tex_name));
            }
        }
        for (const auto& kvp: fs_snippet.image_sample_type_tags) {
            const auto& tag = kvp.second;
            if (nullptr == util::find_image_by_name(fs_src.refl, tag.tex_name)) {
                return inp.error(tag.line_index, fmt::format("unknown texture name '{}' in @image_sample_type tag\n", tag.tex_name));
            }
        }
    }
    return errmsg_t();
}

spirvcross_t spirvcross_t::translate(const input_t& inp, const spirv_t& spirv, slang_t::type_t slang) {
    spirvcross_t spv_cross;
    for (const auto& blob: spirv.blobs) {
        spirvcross_source_t src;
        uint32_t opt_mask = inp.snippets[blob.snippet_index].options[(int)slang];
        const snippet_t& snippet = inp.snippets[blob.snippet_index];
        assert((snippet.type == snippet_t::VS) || (snippet.type == snippet_t::FS));
        spv_cross.error = validate_uniform_blocks_and_separate_image_samplers(inp, blob);
        if (spv_cross.error.valid) {
            return spv_cross;
        }
        switch (slang) {
            case slang_t::GLSL330:
            case slang_t::GLSL100:
            case slang_t::GLSL300ES:
                src = to_glsl(inp, blob, slang, opt_mask, snippet);
                break;
            case slang_t::HLSL4:
            case slang_t::HLSL5:
                src = to_hlsl(inp, blob, slang, opt_mask, snippet);
                break;
            case slang_t::METAL_MACOS:
            case slang_t::METAL_IOS:
            case slang_t::METAL_SIM:
                src = to_msl(inp, blob, slang, opt_mask, snippet);
                break;
            case slang_t::WGSL:
                src = to_wgsl(inp, blob, slang, opt_mask, snippet);
                break;
            default: break;
        }
        if (src.valid) {
            assert(src.snippet_index == blob.snippet_index);
            spv_cross.sources.push_back(std::move(src));
        }
        else {
            const int line_index = snippet.lines[0];
            std::string err_msg;
            if (src.error.valid) {
                err_msg = fmt::format("Failed to cross-compile to {} with:\n{}\n", slang_t::to_str(slang), src.error.msg);
            } else {
                err_msg = fmt::format("Failed to cross-compile to {}\n", slang_t::to_str(slang));
            }
            spv_cross.error = inp.error(line_index, err_msg);
            return spv_cross;
        }
        if (!gather_unique_uniform_blocks(inp, spv_cross)) {
            // error has been set in spv_cross.error
            return spv_cross;
        }
        if (!gather_unique_images(inp, spv_cross)) {
            // error has been set in spv_cross.error
            return spv_cross;
        }
        if (!gather_unique_samplers(inp, spv_cross)) {
            // error has been set in spv_cross.error
            return spv_cross;
        }
    }
    // check that vertex-shader outputs match their fragment shader inputs
    errmsg_t err;
    err = validate_linking(inp, spv_cross);
    if (err.valid) {
        spv_cross.error = err;
        return spv_cross;
    }
    // check that explicit image-sampler-type-tags and sampler-type-tags use existing image and sampler names
    err = validate_image_sample_type_tags(inp, spv_cross);
    if (err.valid) {
        spv_cross.error = err;
        return spv_cross;
    }
    /* FIXME
    err = validate_sampler_type_tags(inp, spv_cross);
    if (err.valid) {
        spv_cross.error = err;
        return spv_cross;
    }
    */
    return spv_cross;
}

void spirvcross_t::dump_debug(errmsg_t::msg_format_t err_fmt, slang_t::type_t slang) const {
    fmt::print(stderr, "spirvcross_t ({}):\n", slang_t::to_str(slang));
    if (error.valid) {
        fmt::print(stderr, "  error: {}\n", error.as_string(err_fmt));
    }
    else {
        fmt::print(stderr, "  error: not set\n");
    }
    std::vector<std::string> lines;
    for (const spirvcross_source_t& source: sources) {
        fmt::print(stderr, "    source for snippet {}:\n", source.snippet_index);
        pystring::splitlines(source.source_code, lines);
        for (const std::string& line: lines) {
            fmt::print(stderr, "      {}\n", line);
        }
        fmt::print(stderr, "    reflection for snippet {}:\n", source.snippet_index);
        fmt::print(stderr, "      stage: {}\n", stage_t::to_str(source.refl.stage));
        fmt::print(stderr, "      entry: {}\n", source.refl.entry_point);
        fmt::print(stderr, "      inputs:\n");
        for (const attr_t& attr: source.refl.inputs) {
            if (attr.slot >= 0) {
                fmt::print(stderr, "        {}: slot={}, sem_name={}, sem_index={}\n", attr.name, attr.slot, attr.sem_name, attr.sem_index);
            }
        }
        fmt::print(stderr, "      outputs:\n");
        for (const attr_t& attr: source.refl.outputs) {
            if (attr.slot >= 0) {
                fmt::print(stderr, "        {}: slot={}, sem_name={}, sem_index={}\n", attr.name, attr.slot, attr.sem_name, attr.sem_index);
            }
        }
        for (const uniform_block_t& ub: source.refl.uniform_blocks) {
            fmt::print(stderr, "      uniform block: {}, slot: {}, size: {}\n", ub.struct_name, ub.slot, ub.size);
            for (const uniform_t& uniform: ub.uniforms) {
                fmt::print(stderr, "          member: {}, type: {}, array_count: {}, offset: {}\n",
                    uniform.name,
                    uniform_t::type_to_str(uniform.type),
                    uniform.array_count,
                    uniform.offset);
            }
        }
        for (const image_t& img: source.refl.images) {
            fmt::print(stderr, "      image: {}, slot: {}, type: {}, sampletype: {}\n",
                img.name, img.slot, image_type_t::to_str(img.type), image_sample_type_t::to_str(img.sample_type));
        }
        for (const sampler_t& smp: source.refl.samplers) {
            fmt::print(stderr, "      sampler: {}, slot: {}\n", smp.name, smp.slot);
        }
        for (const image_sampler_t& img_smp: source.refl.image_samplers) {
            fmt::print(stderr, "      image sampler: {}, slot: {}, image: {}, sampler: {}\n",
                img_smp.name, img_smp.slot, img_smp.image_name, img_smp.sampler_name);
        }
        fmt::print(stderr, "\n");
    }
    fmt::print(stderr, "\n");
}

} // namespace shdc
