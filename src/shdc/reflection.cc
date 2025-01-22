/*
    Code for reflection parsing.
*/
#include "reflection.h"
#include "spirvcross.h"
#include "types/reflection/bindings.h"

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

namespace shdc::refl {

static const Type::Enum bool_types[4][4] = {
    { Type::Bool,    Type::Bool2,   Type::Bool3,   Type::Bool4 },
    { Type::Invalid, Type::Invalid, Type::Invalid, Type::Invalid },
    { Type::Invalid, Type::Invalid, Type::Invalid, Type::Invalid },
    { Type::Invalid, Type::Invalid, Type::Invalid, Type::Invalid },
};
static const Type::Enum int_types[4][4] = {
    { Type::Int,     Type::Int2,    Type::Int3,    Type::Int4 },
    { Type::Invalid, Type::Invalid, Type::Invalid, Type::Invalid },
    { Type::Invalid, Type::Invalid, Type::Invalid, Type::Invalid },
    { Type::Invalid, Type::Invalid, Type::Invalid, Type::Invalid },
};
static const Type::Enum uint_types[4][4] = {
    { Type::UInt,    Type::UInt2,   Type::UInt3,   Type::UInt4 },
    { Type::Invalid, Type::Invalid, Type::Invalid, Type::Invalid },
    { Type::Invalid, Type::Invalid, Type::Invalid, Type::Invalid },
    { Type::Invalid, Type::Invalid, Type::Invalid, Type::Invalid },
};
static const Type::Enum float_types[4][4] = {
    { Type::Float,  Type::Float2, Type::Float3, Type::Float4 },
    { Type::Mat2x1, Type::Mat2x2, Type::Mat2x3, Type::Mat2x4 },
    { Type::Mat3x1, Type::Mat3x2, Type::Mat3x3, Type::Mat3x4 },
    { Type::Mat4x1, Type::Mat4x2, Type::Mat4x3, Type::Mat4x4 },
};


// check that a program's vertex shader outputs match the fragment shader inputs
// FIXME: this should also check the attribute's type
static ErrMsg validate_linking(const Input& inp, const Program& prog, const ProgramReflection& prog_refl) {
    for (int slot = 0; slot < StageAttr::Num; slot++) {
        const StageAttr& vs_out = prog_refl.vs().outputs[slot];
        const StageAttr& fs_inp = prog_refl.fs().inputs[slot];
        // ignore snippet-name for equality check
        if (!vs_out.equals(fs_inp)) {
            return inp.error(prog.line_index,
                fmt::format("outputs of vs '{}' don't match inputs of fs '{}' for attr #{} (vs={},fs={})\n",
                    prog_refl.vs_name(),
                    prog_refl.fs_name(),
                    slot,
                    vs_out.name,
                    fs_inp.name));
        }
    }
    return ErrMsg();
}

Reflection Reflection::build(const Args& args, const Input& inp, const std::array<Spirvcross,Slang::Num>& spirvcross_array) {
    Reflection res;
    ErrMsg err;

    // for each program, just pick the reflection info from the first compiled slang,
    // the reflection info is the same for each Slang because it has been generated
    // with the special Slang::REFLECTION, not the actual slang.
    std::vector<Bindings> prog_bindings;
    for (const auto& item: inp.programs) {
        const Program& prog = item.second;
        int vs_snippet_index = inp.snippet_map.at(prog.vs_name);
        int fs_snippet_index = inp.snippet_map.at(prog.fs_name);
        const Slang::Enum slang = Slang::first_valid(args.slang);
        const Spirvcross& spirvcross = spirvcross_array[slang];
        const SpirvcrossSource* vs_src = spirvcross.find_source_by_snippet_index(vs_snippet_index);
        const SpirvcrossSource* fs_src = spirvcross.find_source_by_snippet_index(fs_snippet_index);
        assert(vs_src && fs_src);
        ProgramReflection prog_refl;
        prog_refl.name = prog.name;
        prog_refl.stages[ShaderStage::Vertex] = vs_src->stage_refl;
        prog_refl.stages[ShaderStage::Fragment] = fs_src->stage_refl;
        prog_refl.bindings = merge_bindings({ vs_src->stage_refl.bindings, fs_src->stage_refl.bindings }, true, err);
        if (err.valid()) {
            res.error = inp.error(prog.line_index, err.msg);
            return res;
        }
        err = validate_program_bindings(prog_refl.bindings);
        if (err.valid()) {
            res.error = inp.error(prog.line_index, err.msg);
            return res;
        }
        prog_bindings.push_back(prog_refl.bindings);

        // check that the outputs of the vertex stage match the input stage
        res.error = validate_linking(inp, prog, prog_refl);
        if (res.error.valid()) {
            return res;
        }
        res.progs.push_back(prog_refl);
    }

    // create a merged set of resource bindings across all programs
    for (const auto& prog_refl: res.progs) {
        prog_bindings.push_back(prog_refl.bindings);
    }
    res.bindings = merge_bindings(prog_bindings, false, err);
    if (err.valid()) {
        res.error = inp.error(0, err.msg);
    }
    return res;
}

static ImageType::Enum spirtype_to_image_type(const SPIRType& type) {
    if (type.image.arrayed) {
        if (type.image.dim == spv::Dim2D) {
            return ImageType::ARRAY;
        }
    } else {
        switch (type.image.dim) {
            case spv::Dim2D:    return ImageType::_2D;
            case spv::DimCube:  return ImageType::CUBE;
            case spv::Dim3D:    return ImageType::_3D;
            default: break;
        }
    }
    // fallthrough: invalid type
    return ImageType::INVALID;
}

static ImageSampleType::Enum spirtype_to_image_sample_type(const SPIRType& type) {
    if (type.image.depth) {
        return ImageSampleType::DEPTH;
    } else {
        switch (type.basetype) {
            case SPIRType::Int:
            case SPIRType::Short:
            case SPIRType::SByte:
                return ImageSampleType::SINT;
            case SPIRType::UInt:
            case SPIRType::UShort:
            case SPIRType::UByte:
                return ImageSampleType::UINT;
            default:
                return ImageSampleType::FLOAT;
        }
    }
}

static bool spirtype_to_image_multisampled(const SPIRType& type) {
    return type.image.ms;
}

Type get_type_for_attribute(const Compiler& compiler, const Resource& res_attr) {
    const SPIRType& attr_type = compiler.get_type(res_attr.type_id);
    Type out;
    out.name = res_attr.name;
    out.type = Type::Invalid;
    uint32_t col_idx = attr_type.columns - 1;
    uint32_t vec_idx = attr_type.vecsize - 1;
    if ((col_idx < 4) && (vec_idx < 4)) {
        switch (attr_type.basetype) {
            case SPIRType::Boolean:
                out.type = bool_types[col_idx][vec_idx];
                break;
            case SPIRType::Int:
                out.type = int_types[col_idx][vec_idx];
                break;
            case SPIRType::UInt:
                out.type = uint_types[col_idx][vec_idx];
                break;
            case SPIRType::Float:
                out.type = float_types[col_idx][vec_idx];
                break;
            case SPIRType::Struct:
                out.type = Type::Struct;
                break;
            default:
                break;
        }
    }
    return out;
}

StageReflection Reflection::parse_snippet_reflection(const Compiler& compiler, const Snippet& snippet, const Input& inp, ErrMsg& out_error) {
    out_error = ErrMsg();
    StageReflection refl;

    assert(snippet.index >= 0);
    refl.snippet_index = snippet.index;
    refl.snippet_name = snippet.name;

    ShaderResources shd_resources = compiler.get_shader_resources();
    // shader stage
    switch (compiler.get_execution_model()) {
        case spv::ExecutionModelVertex:   refl.stage = ShaderStage::Vertex; break;
        case spv::ExecutionModelFragment: refl.stage = ShaderStage::Fragment; break;
        default: refl.stage = ShaderStage::Invalid; break;
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
        StageAttr refl_attr;
        refl_attr.slot = compiler.get_decoration(res_attr.id, spv::DecorationLocation);
        refl_attr.name = res_attr.name;
        refl_attr.sem_name = "TEXCOORD";
        refl_attr.sem_index = refl_attr.slot;
        refl_attr.type_info = get_type_for_attribute(compiler, res_attr);

        refl.inputs[refl_attr.slot] = refl_attr;
    }
    for (const Resource& res_attr: shd_resources.stage_outputs) {
        StageAttr refl_attr;
        refl_attr.slot = compiler.get_decoration(res_attr.id, spv::DecorationLocation);
        refl_attr.name = res_attr.name;
        refl_attr.sem_name = "TEXCOORD";
        refl_attr.sem_index = refl_attr.slot;
        refl_attr.type_info = get_type_for_attribute(compiler, res_attr);

        refl.outputs[refl_attr.slot] = refl_attr;
    }
    // uniform blocks
    for (const Resource& ub_res: shd_resources.uniform_buffers) {
        const int slot = compiler.get_decoration(ub_res.id, spv::DecorationBinding);
        const Bindings::Type res_type = Bindings::Type::UNIFORM_BLOCK;
        UniformBlock refl_ub;
        refl_ub.stage = refl.stage;
        refl_ub.inst_name = compiler.get_name(ub_res.id);
        if (refl_ub.inst_name.empty()) {
            refl_ub.inst_name = compiler.get_fallback_name(ub_res.id);
        }
        refl_ub.hlsl_register_b_n = slot + Bindings::base_slot(Slang::HLSL5, refl.stage, res_type);
        refl_ub.msl_buffer_n = slot + Bindings::base_slot(Slang::METAL_SIM, refl.stage, res_type);
        refl_ub.wgsl_group0_binding_n = slot + Bindings::base_slot(Slang::WGSL, refl.stage, res_type);
        refl_ub.flattened = Spirvcross::can_flatten_uniform_block(compiler, ub_res);
        refl_ub.struct_info = parse_toplevel_struct(compiler, ub_res, out_error);
        if (out_error.valid()) {
            return refl;
        }
        // uniform blocks always have 16 byte alignment
        refl_ub.struct_info.align = 16;
        refl_ub.name = refl_ub.struct_info.name;
        refl_ub.sokol_slot = inp.find_ub_slot(refl_ub.name);
        if (refl_ub.sokol_slot == -1) {
            out_error = inp.error(0, fmt::format("no binding found for uniformblock '{}' (might be unused in shader code?)\n", refl_ub.name));
            return refl;
        }
        refl.bindings.uniform_blocks.push_back(refl_ub);
    }
    // storage buffers
    for (const Resource& sbuf_res: shd_resources.storage_buffers) {
        const int slot = compiler.get_decoration(sbuf_res.id, spv::DecorationBinding);
        const Bindings::Type res_type = Bindings::Type::STORAGE_BUFFER;
        StorageBuffer refl_sbuf;
        refl_sbuf.inst_name = compiler.get_name(sbuf_res.id);
        if (refl_sbuf.inst_name.empty()) {
            refl_sbuf.inst_name = compiler.get_fallback_name(sbuf_res.id);
        }
        refl_sbuf.struct_info = parse_toplevel_struct(compiler, sbuf_res, out_error);
        if (out_error.valid()) {
            return refl;
        }
        refl_sbuf.name = refl_sbuf.struct_info.name;
        refl_sbuf.sokol_slot = inp.find_sbuf_slot(refl_sbuf.name);
        if (refl_sbuf.sokol_slot == -1) {
            out_error = inp.error(0, fmt::format("no binding found for storagebuffer '{}' (might be unused in shader code?)\n", refl_sbuf.name));
            return refl;
        }
        refl_sbuf.stage = refl.stage;
        refl_sbuf.hlsl_register_t_n = slot + Bindings::base_slot(Slang::HLSL5, refl.stage, res_type);
        refl_sbuf.msl_buffer_n = slot + Bindings::base_slot(Slang::METAL_SIM, refl.stage, res_type);
        refl_sbuf.wgsl_group1_binding_n = slot + Bindings::base_slot(Slang::WGSL, refl.stage, res_type);
        // SPECIAL CASE GL: the GL storage buffer bind slot is identical with the sokol-bind slot
        // since GL also has a common bindspace across shader stages for storage buffers
        refl_sbuf.glsl_binding_n = refl_sbuf.sokol_slot;
        refl_sbuf.readonly = compiler.get_buffer_block_flags(sbuf_res.id).get(spv::DecorationNonWritable);
        refl.bindings.storage_buffers.push_back(refl_sbuf);
    }

    // (separate) images
    for (const Resource& img_res: shd_resources.separate_images) {
        const int slot = compiler.get_decoration(img_res.id, spv::DecorationBinding);
        const Bindings::Type res_type = Bindings::Type::IMAGE;
        Image refl_img;
        refl_img.stage = refl.stage;
        refl_img.hlsl_register_t_n = slot + Bindings::base_slot(Slang::HLSL5, refl.stage, res_type);
        refl_img.msl_texture_n = slot + Bindings::base_slot(Slang::METAL_SIM, refl.stage, res_type);
        refl_img.wgsl_group1_binding_n = slot + Bindings::base_slot(Slang::WGSL, refl.stage, res_type);
        refl_img.name = img_res.name;
        const SPIRType& img_type = compiler.get_type(img_res.type_id);
        refl_img.type = spirtype_to_image_type(img_type);
        if (((UnprotectedCompiler*)&compiler)->is_used_as_depth_texture(img_type, img_res.id)) {
            refl_img.sample_type = ImageSampleType::DEPTH;
        } else {
            refl_img.sample_type = spirtype_to_image_sample_type(compiler.get_type(img_type.image.type));
        }
        refl_img.multisampled = spirtype_to_image_multisampled(img_type);
        refl_img.sokol_slot = inp.find_img_slot(refl_img.name);
        if (refl_img.sokol_slot == -1) {
            out_error = inp.error(0, fmt::format("no binding found for image '{}' (might be unused in shader code?)\n", refl_img.name));
            return refl;
        }
        refl.bindings.images.push_back(refl_img);
    }
    // (separate) samplers
    for (const Resource& smp_res: shd_resources.separate_samplers) {
        const int slot = compiler.get_decoration(smp_res.id, spv::DecorationBinding);
        const Bindings::Type res_type = Bindings::Type::SAMPLER;
        const SPIRType& smp_type = compiler.get_type(smp_res.type_id);
        Sampler refl_smp;
        refl_smp.stage = refl.stage;
        refl_smp.hlsl_register_s_n = slot + Bindings::base_slot(Slang::HLSL5, refl.stage, res_type);
        refl_smp.msl_sampler_n = slot + Bindings::base_slot(Slang::METAL_SIM, refl.stage, res_type);
        refl_smp.wgsl_group1_binding_n = slot + Bindings::base_slot(Slang::WGSL, refl.stage, res_type);
        refl_smp.name = smp_res.name;
        // HACK ALERT!
        if (((UnprotectedCompiler*)&compiler)->is_comparison_sampler(smp_type, smp_res.id)) {
            refl_smp.type = SamplerType::COMPARISON;
        } else {
            refl_smp.type = SamplerType::FILTERING;
        }
        refl_smp.sokol_slot = inp.find_smp_slot(refl_smp.name);
        if (refl_smp.sokol_slot == -1) {
            out_error = inp.error(0, fmt::format("no binding found for sampler '{}' (might be unused in shader code?)\n", refl_smp.name));
            return refl;
        }
        refl.bindings.samplers.push_back(refl_smp);
    }
    // combined image samplers
    for (auto& img_smp_res: compiler.get_combined_image_samplers()) {
        ImageSampler refl_img_smp;
        refl_img_smp.stage = refl.stage;
        refl_img_smp.sokol_slot = compiler.get_decoration(img_smp_res.combined_id, spv::DecorationBinding);
        refl_img_smp.name = compiler.get_name(img_smp_res.combined_id);
        refl_img_smp.image_name = compiler.get_name(img_smp_res.image_id);
        refl_img_smp.sampler_name = compiler.get_name(img_smp_res.sampler_id);
        refl.bindings.image_samplers.push_back(refl_img_smp);
    }
    // patch textures with overridden image-sample-types
    for (auto& img: refl.bindings.images) {
        const auto* tag = inp.find_image_sample_type_tag(img.name);
        if (tag) {
            img.sample_type = tag->type;
        }
    }
    // patch samplers with overridden sampler-types
    for (auto& smp: refl.bindings.samplers) {
        const auto* tag = inp.find_sampler_type_tag(smp.name);
        if (tag) {
            smp.type = tag->type;
        }
    }
    return refl;
}

Bindings Reflection::merge_bindings(const std::vector<Bindings>& in_bindings, bool to_prog_bindings, ErrMsg& out_error) {
    Bindings out_bindings;
    out_error = ErrMsg();
    for (const Bindings& src_bindings: in_bindings) {

        // merge identical uniform blocks
        for (const UniformBlock& ub: src_bindings.uniform_blocks) {
            const UniformBlock* other_ub = out_bindings.find_uniform_block_by_name(ub.name);
            if (other_ub) {
                // another uniform block of the same name exists, make sure it's identical
                if (!ub.equals(*other_ub)) {
                    out_error = ErrMsg::error(fmt::format("conflicting uniform block definitions found for '{}'", ub.name));
                    return Bindings();
                }
            } else {
                out_bindings.uniform_blocks.push_back(ub);
            }
        }

        // merge identical storage buffers
        for (const StorageBuffer& sbuf: src_bindings.storage_buffers) {
            const StorageBuffer* other_sbuf = out_bindings.find_storage_buffer_by_name(sbuf.name);
            if (other_sbuf) {
                // another storage buffer of the same name exists, make sure it's identical
                if (!sbuf.equals(*other_sbuf)) {
                    out_error = ErrMsg::error(fmt::format("conflicting storage buffer definitions found for '{}'", sbuf.name));
                    return Bindings();
                }
            } else {
                out_bindings.storage_buffers.push_back(sbuf);
            }
        }

        // merge identical images
        for (const Image& img: src_bindings.images) {
            const Image* other_img = out_bindings.find_image_by_name(img.name);
            if (other_img) {
                // another image of the same name exists, make sure it's identical
                if (!img.equals(*other_img)) {
                    out_error = ErrMsg::error(fmt::format("conflicting texture definitions found for '{}'", img.name));
                    return Bindings();
                }
            } else {
                out_bindings.images.push_back(img);
            }
        }

        // merge identical samplers
        for (const Sampler& smp: src_bindings.samplers) {
            const Sampler* other_smp = out_bindings.find_sampler_by_name(smp.name);
            if (other_smp) {
                // another sampler of the same name exists, make sure it's identical
                if (!smp.equals(*other_smp)) {
                    out_error = ErrMsg::error(fmt::format("conflicting sampler definitions found for '{}'", smp.name));
                    return Bindings();
                }
            } else {
                out_bindings.samplers.push_back(smp);
            }
        }

        // merge image samplers (only for prog bindings)
        // since image-samplers will have their bindings auto-assigned their slots won't
        // match across programs, but common image-sampler bindings across programs are also not required
        // anywhere (such bindings are only needed for generating the common slot constants)
        if (to_prog_bindings) {
            for (const ImageSampler& img_smp: src_bindings.image_samplers) {
                const ImageSampler* other_img_smp = out_bindings.find_image_sampler_by_name(img_smp.name);
                if (other_img_smp) {
                    // another image sampler of the same name exists, make sure it's identical
                    if (!img_smp.equals(*other_img_smp)) {
                        out_error = ErrMsg::error(fmt::format("conflicting image-sampler definition found for '{}'", img_smp.name));
                        return Bindings();
                    }
                } else {
                    out_bindings.image_samplers.push_back(img_smp);
                }
            }
        }
    }

    // if requested, assign new image-sampler slots which are unique across shader stages, this
    // is needed when merging the per-shader-stage bindings into per-program-bindings
    if (to_prog_bindings) {
        int sokol_slot = 0;
        for (ImageSampler& img_smp: out_bindings.image_samplers) {
            img_smp.sokol_slot = sokol_slot++;
        }
    }

    return out_bindings;
}

Type Reflection::parse_struct_item(const Compiler& compiler, const TypeID& type_id, const TypeID& base_type_id, uint32_t item_index, ErrMsg& out_error) {
    const SPIRType& base_type = compiler.get_type(base_type_id);
    const TypeID& item_base_type_id = base_type.member_types[item_index];
    const SPIRType& item_base_type = compiler.get_type(item_base_type_id);
    const SPIRType& type = compiler.get_type(type_id);
    const TypeID& item_type_id = type.member_types[item_index];
    Type out;
    out.name = compiler.get_member_name(base_type.self, item_index);
    out.type = Type::Invalid;
    uint32_t col_idx = item_base_type.columns - 1;
    uint32_t vec_idx = item_base_type.vecsize - 1;
    if ((col_idx < 4) && (vec_idx < 4)) {
        switch (item_base_type.basetype) {
            case SPIRType::Boolean:
                out.type = bool_types[col_idx][vec_idx];
                break;
            case SPIRType::Int:
                out.type = int_types[col_idx][vec_idx];
                break;
            case SPIRType::UInt:
                out.type = uint_types[col_idx][vec_idx];
                break;
            case SPIRType::Float:
                out.type = float_types[col_idx][vec_idx];
                break;
            case SPIRType::Struct:
                out.type = Type::Struct;
                break;
            default:
                break;
        }
    }
    if (out.type == Type::Struct) {
        out.struct_typename = compiler.get_name(compiler.get_type(item_type_id).self);
    }
    if (Type::Invalid == out.type) {
        out_error = ErrMsg::error(fmt::format("struct item {} has unsupported type", out.name));
    }
    if (out.type == Type::Struct) {
        out.size = (int) compiler.get_declared_struct_size_runtime_array(item_base_type, 1);
    } else {
        out.size = (int) compiler.get_declared_struct_member_size(base_type, item_index);
        if (item_base_type.vecsize == 3) {
            out.align = 4 * 4;
        } else {
            out.align = 4 * item_base_type.vecsize;
        }
    }
    out.is_matrix = item_base_type.columns > 1;
    if (out.is_matrix) {
        out.matrix_stride = compiler.type_struct_member_matrix_stride(base_type, item_index);
    }
    out.offset = compiler.type_struct_member_offset(base_type, item_index);
    if (item_base_type.array.size() == 0) {
        out.is_array = false;
    } else if (item_base_type.array.size() == 1) {
        out.is_array = true;
        out.array_count = item_base_type.array[0];   // NOTE: may be 0 for unbounded array!
        out.array_stride = compiler.type_struct_member_array_stride(base_type, item_index);
    } else {
        out_error = ErrMsg::error(fmt::format("arrays of arrays are not supported (struct item {})", out.name));
        return out;
    }
    if (out.type == Type::Struct) {
        for (uint32_t nested_item_index = 0; nested_item_index < item_base_type.member_types.size(); nested_item_index++) {
            const Type nested_type = parse_struct_item(compiler, item_type_id, item_base_type_id, nested_item_index, out_error);
            if (out_error.valid()) {
                return out;
            }
            if (nested_type.align > out.align) {
                out.align = nested_type.align;
            }
            out.struct_items.push_back(nested_type);
        }
    }
    return out;
}

ErrMsg Reflection::validate_program_bindings(const Bindings& bindings) {
    {
        std::array<std::string, Bindings::MaxUniformBlocks> ub_slots;
        for (const auto& ub: bindings.uniform_blocks) {
            const int slot = ub.sokol_slot;
            if ((slot < 0) || (slot >= Bindings::MaxUniformBlocks)) {
                return ErrMsg::error(fmt::format("binding {} out of range for uniform block '{}' (must be 0..{})",
                    slot,
                    ub.name,
                    Bindings::MaxUniformBlocks - 1));
            }
            if (!ub_slots[slot].empty()) {
                return ErrMsg::error(fmt::format("uniform blocks {} and {} cannot use the same binding {}",
                    ub_slots[slot],
                    ub.name,
                    slot));
            }
            ub_slots[slot] = ub.name;
        }
    }
    {
        std::array<std::string, Bindings::MaxStorageBuffers> sbuf_slots;
        for (const auto& sbuf: bindings.storage_buffers) {
            const int slot = sbuf.sokol_slot;
            if ((slot < 0) || (slot >= Bindings::MaxStorageBuffers)) {
                return ErrMsg::error(fmt::format("binding {} out of range for storage buffer '{}' (must be 0..{})",
                    slot,
                    sbuf.name,
                    Bindings::MaxStorageBuffers - 1));
            }
            if (!sbuf_slots[slot].empty()) {
                return ErrMsg::error(fmt::format("storage buffer '{}' and '{}' cannot use the same binding {}",
                    sbuf_slots[slot],
                    sbuf.name,
                    slot));
            }
            sbuf_slots[slot] = sbuf.name;
        }
    }
    {
        std::array<std::string, Bindings::MaxImages> img_slots;
        for (const auto& img: bindings.images) {
            const int slot = img.sokol_slot;
            if ((slot < 0) || (slot > Bindings::MaxImages)) {
                return ErrMsg::error(fmt::format("binding {} out of range for image '{}' (must be 0..{})",
                    slot,
                    img.name,
                    Bindings::MaxImages - 1));
            }
            if (!img_slots[slot].empty()) {
                return ErrMsg::error(fmt::format("images '{}' and '{}' cannot use the same binding {}",
                    img_slots[slot],
                    img.name,
                    slot));
            }
            img_slots[slot] = img.name;
        }
    }
    {
        std::array<std::string, Bindings::MaxSamplers> smp_slots;
        for (const auto& smp: bindings.samplers) {
            const int slot = smp.sokol_slot;
            if ((slot < 0) || (slot > Bindings::MaxSamplers)) {
                return ErrMsg::error(fmt::format("binding {} out of range for sampler '{}' (must be 0..{})",
                    slot,
                    smp.name,
                    Bindings::MaxSamplers - 1));
            }
            if (!smp_slots[slot].empty()) {
                    return ErrMsg::error(fmt::format("samplers '{}' and '{}' cannot use the same binding {}",
                        smp_slots[slot],
                        smp.name,
                        slot));
            }
            smp_slots[slot] = smp.name;
        }
    }
    return ErrMsg();
}

Type Reflection::parse_toplevel_struct(const Compiler& compiler, const Resource& res, ErrMsg& out_error) {
    Type out;
    out.name = res.name;
    const SPIRType& struct_type = compiler.get_type(res.base_type_id);
    if (struct_type.basetype != SPIRType::Struct) {
        out_error = ErrMsg::error(fmt::format("toplevel item {} is not a struct", out.name));
        return out;
    }
    out.type = Type::Struct;
    out.struct_typename = compiler.get_name(compiler.get_type(res.type_id).self);
    out.size = (int) compiler.get_declared_struct_size_runtime_array(struct_type, 1);
    for (uint32_t item_index = 0; item_index < struct_type.member_types.size(); item_index++) {
        const Type item_type = parse_struct_item(compiler, res.type_id, res.base_type_id, item_index, out_error);
        if (out_error.valid()) {
            return out;
        }
        if (item_type.align > out.align) {
            out.align = item_type.align;
        }
        out.struct_items.push_back(item_type);
    }
    return out;
}

void Reflection::dump_debug(ErrMsg::Format err_fmt) const {
    const std::string indent = "  ";
    const std::string indent2 = indent + "  ";
    fmt::print(stderr, "Reflection:\n");
    if (error.valid()) {
        fmt::print(stderr, "{}error: {}\n", indent, error.as_string(err_fmt));
    } else {
        fmt::print(stderr, "{}error: not set\n", indent);
    }
    fmt::print(stderr, "{}merged bindings:\n", indent);
    bindings.dump_debug(indent2);
    fmt::print(stderr, "{}programs:\n", indent);
    for (const auto& prog: progs) {
        prog.dump_debug(indent2);
    }
}

} // namespace
