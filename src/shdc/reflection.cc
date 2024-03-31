/*
    Code for reflection parsing.
*/
#include "reflection.h"
#include "spirvcross.h"

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

// check that a program's vertex shader outputs match the fragment shader inputs
// FIXME: this should also check the attribute's type
// FIXME: move all reflection validations into Reflection
static ErrMsg validate_linking(const Input& inp, const Program& prog, const ProgramReflection& prog_refl) {
    for (int slot = 0; slot < StageAttr::Num; slot++) {
        const StageAttr& vs_out = prog_refl.vs().outputs[slot];
        const StageAttr& fs_inp = prog_refl.fs().inputs[slot];
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

    // for each program, just pick the reflection info from the first compiled slang
    // FIXME: we should check whether the reflection info of all compiled slangs actually matches
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

        // check that the outputs of the vertex stage match the input stage
        res.error = validate_linking(inp, prog, prog_refl);
        if (res.error.valid()) {
            return res;
        }
        res.progs.push_back(prog_refl);
    }

    // create a merged set of resource bindings
    std::vector<Bindings> snippet_bindings;
    for (int i = 0; i < Slang::Num; i++) {
        Slang::Enum slang = Slang::from_index(i);
        if (args.slang & Slang::bit(slang)) {
            for (const SpirvcrossSource& src: spirvcross_array[i].sources) {
                snippet_bindings.push_back(src.stage_refl.bindings);
            }
        }
    }
    res.bindings = merge_bindings(snippet_bindings, inp.base_path, res.error);
    return res;
}

static Uniform::Type spirtype_to_uniform_type(const SPIRType& type) {
    switch (type.basetype) {
        case SPIRType::Float:
            if (type.columns == 1) {
                // scalar or vec
                switch (type.vecsize) {
                    case 1: return Uniform::FLOAT;
                    case 2: return Uniform::FLOAT2;
                    case 3: return Uniform::FLOAT3;
                    case 4: return Uniform::FLOAT4;
                }
            } else {
                // a matrix
                if ((type.vecsize == 4) && (type.columns == 4)) {
                    return Uniform::MAT4;
                }
            }
            break;
        case SPIRType::Int:
            if (type.columns == 1) {
                switch (type.vecsize) {
                    case 1: return Uniform::INT;
                    case 2: return Uniform::INT2;
                    case 3: return Uniform::INT3;
                    case 4: return Uniform::INT4;
                }
            }
            break;
        default: break;
    }
    // fallthrough: invalid type
    return Uniform::INVALID;
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

StageReflection Reflection::parse_snippet_reflection(const Compiler& compiler, const Snippet& snippet, Slang::Enum slang, ErrMsg& out_error) {
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
    refl.stage_name = ShaderStage::to_str(refl.stage);

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
        refl.inputs[refl_attr.slot] = refl_attr;
    }
    for (const Resource& res_attr: shd_resources.stage_outputs) {
        StageAttr refl_attr;
        refl_attr.slot = compiler.get_decoration(res_attr.id, spv::DecorationLocation);
        refl_attr.name = res_attr.name;
        refl_attr.sem_name = "TEXCOORD";
        refl_attr.sem_index = refl_attr.slot;
        refl.outputs[refl_attr.slot] = refl_attr;
    }
    // uniform blocks
    for (const Resource& ub_res: shd_resources.uniform_buffers) {
        UniformBlock refl_ub;
        const SPIRType& ub_type = compiler.get_type(ub_res.base_type_id);
        refl_ub.stage = refl.stage;
        refl_ub.slot = compiler.get_decoration(ub_res.id, spv::DecorationBinding);
        refl_ub.size = (int) compiler.get_declared_struct_size(ub_type);
        // FIXME: struct_name is obsolete
        refl_ub.struct_name = ub_res.name;
        refl_ub.inst_name = compiler.get_name(ub_res.id);
        if (refl_ub.inst_name.empty()) {
            refl_ub.inst_name = compiler.get_fallback_name(ub_res.id);
        }
        refl_ub.flattened = Spirvcross::can_flatten_uniform_block(compiler, ub_res);
        // FIXME uniforms array is obsolete
        refl_ub.struct_refl = parse_toplevel_struct(compiler, ub_res.base_type_id, ub_res.name, out_error);
        if (out_error.valid()) {
            return refl;
        }
        for (size_t m_index = 0; m_index < ub_type.member_types.size(); m_index++) {
            Uniform refl_uniform;
            refl_uniform.name = compiler.get_member_name(ub_res.base_type_id, m_index);
            const SPIRType& m_type = compiler.get_type(ub_type.member_types[m_index]);
            refl_uniform.type = spirtype_to_uniform_type(m_type);
            if (m_type.array.size() > 0) {
                refl_uniform.array_count = m_type.array[0];
            }
            refl_uniform.offset = compiler.type_struct_member_offset(ub_type, m_index);
            refl_ub.uniforms.push_back(refl_uniform);
        }
        refl.bindings.uniform_blocks.push_back(refl_ub);
    }
    // storage buffers
    for (const Resource& sbuf_res: shd_resources.storage_buffers) {
        StorageBuffer refl_sbuf;
        refl_sbuf.stage = refl.stage;
        refl_sbuf.slot = compiler.get_decoration(sbuf_res.id, spv::DecorationBinding);
        refl_sbuf.inst_name = compiler.get_name(sbuf_res.id);
        if (refl_sbuf.inst_name.empty()) {
            refl_sbuf.inst_name = compiler.get_fallback_name(sbuf_res.id);
        }
        refl_sbuf.struct_refl = parse_toplevel_struct(compiler, sbuf_res.base_type_id, sbuf_res.name, out_error);
        if (out_error.valid()) {
            return refl;
        }
        refl.bindings.storage_buffers.push_back(refl_sbuf);
    }

    // (separate) images
    for (const Resource& img_res: shd_resources.separate_images) {
        Image refl_img;
        refl_img.stage = refl.stage;
        refl_img.slot = compiler.get_decoration(img_res.id, spv::DecorationBinding);
        refl_img.name = img_res.name;
        const SPIRType& img_type = compiler.get_type(img_res.type_id);
        refl_img.type = spirtype_to_image_type(img_type);
        if (((UnprotectedCompiler*)&compiler)->is_used_as_depth_texture(img_type, img_res.id)) {
            refl_img.sample_type = ImageSampleType::DEPTH;
        } else {
            refl_img.sample_type = spirtype_to_image_sample_type(compiler.get_type(img_type.image.type));
        }
        refl_img.multisampled = spirtype_to_image_multisampled(img_type);
        refl.bindings.images.push_back(refl_img);
    }
    // (separate) samplers
    for (const Resource& smp_res: shd_resources.separate_samplers) {
        const SPIRType& smp_type = compiler.get_type(smp_res.type_id);
        Sampler refl_smp;
        refl_smp.stage = refl.stage;
        refl_smp.slot = compiler.get_decoration(smp_res.id, spv::DecorationBinding);
        refl_smp.name = smp_res.name;
        // HACK ALERT!
        if (((UnprotectedCompiler*)&compiler)->is_comparison_sampler(smp_type, smp_res.id)) {
            refl_smp.type = SamplerType::COMPARISON;
        } else {
            refl_smp.type = SamplerType::FILTERING;
        }
        refl.bindings.samplers.push_back(refl_smp);
    }
    // combined image samplers
    for (auto& img_smp_res: compiler.get_combined_image_samplers()) {
        ImageSampler refl_img_smp;
        refl_img_smp.stage = refl.stage;
        refl_img_smp.slot = compiler.get_decoration(img_smp_res.combined_id, spv::DecorationBinding);
        refl_img_smp.name = compiler.get_name(img_smp_res.combined_id);
        refl_img_smp.image_name = compiler.get_name(img_smp_res.image_id);
        refl_img_smp.sampler_name = compiler.get_name(img_smp_res.sampler_id);
        refl.bindings.image_samplers.push_back(refl_img_smp);
    }
    // patch textures with overridden image-sample-types
    for (auto& img: refl.bindings.images) {
        const auto* tag = snippet.lookup_image_sample_type_tag(img.name);
        if (tag) {
            img.sample_type = tag->type;
        }
    }
    // patch samplers with overridden sampler-types
    for (auto& smp: refl.bindings.samplers) {
        const auto* tag = snippet.lookup_sampler_type_tag(smp.name);
        if (tag) {
            smp.type = tag->type;
        }
    }
    return refl;
}

Bindings Reflection::merge_bindings(const std::vector<Bindings>& in_bindings, const std::string& inp_base_path, ErrMsg& out_error) {
    Bindings out_bindings;
    out_error = ErrMsg();
    for (const Bindings& src_bindings: in_bindings) {

        // merge identical uniform blocks
        for (const UniformBlock& ub: src_bindings.uniform_blocks) {
            const UniformBlock* other_ub = out_bindings.find_uniform_block_by_name(ub.struct_name);
            if (other_ub) {
                // another uniform block of the same name exists, make sure it's identical
                if (!ub.equals(*other_ub)) {
                    out_error = ErrMsg::error(inp_base_path, 0, fmt::format("conflicting uniform block definitions found for '{}'", ub.struct_name));
                    return Bindings();
                }
            } else {
                out_bindings.uniform_blocks.push_back(ub);
            }
        }

        // merge identical images
        for (const Image& img: src_bindings.images) {
            const Image* other_img = out_bindings.find_image_by_name(img.name);
            if (other_img) {
                // another image of the same name exists, make sure it's identical
                if (!img.equals(*other_img)) {
                    out_error = ErrMsg::error(inp_base_path, 0, fmt::format("conflicting texture definitions found for '{}'", img.name));
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
                    out_error = ErrMsg::error(inp_base_path, 0, fmt::format("conflicting sampler definitions found for '{}'", smp.name));
                    return Bindings();
                }
            } else {
                out_bindings.samplers.push_back(smp);
            }
        }

        // merge image samplers
        for (const ImageSampler& img_smp: src_bindings.image_samplers) {
            const ImageSampler* other_img_smp = out_bindings.find_image_sampler_by_name(img_smp.name);
            if (other_img_smp) {
                // another image sampler of the same name exists, make sure it's identical
                if (!img_smp.equals(*other_img_smp)) {
                    out_error = ErrMsg::error(inp_base_path, 0, fmt::format("conflicting image-sampler definition found for '{}'", img_smp.name));
                    return Bindings();
                }
            } else {
                out_bindings.image_samplers.push_back(img_smp);
            }
        }
    }
    return out_bindings;
}

Type Reflection::parse_struct_item(const Compiler& compiler, const TypeID& struct_type_id, uint32_t item_index, ErrMsg& out_error) {
    const SPIRType& struct_type = compiler.get_type(struct_type_id);
    const TypeID& item_type_id = struct_type.member_types[item_index];
    const SPIRType& item_type = compiler.get_type(item_type_id);
    Type out;
    out.name = compiler.get_member_name(struct_type.self, item_index);
    switch (item_type.basetype) {
        case SPIRType::Boolean:
            out.base_type = Type::Bool;
            break;
        case SPIRType::Int:
            out.base_type = Type::Int;
            break;
        case SPIRType::UInt:
            out.base_type = Type::UInt;
            break;
        case SPIRType::Float:
            out.base_type = Type::Float;
            break;
        case SPIRType::Half:
            out.base_type = Type::Half;
            break;
        case SPIRType::Struct:
            out.base_type = Type::Struct;
            break;
        default:
            out_error = ErrMsg::error(fmt::format("struct item {} has unsupported type", out.name));
            return out;
    }
    if (out.base_type == Type::Struct) {
        out.size = compiler.get_declared_struct_size_runtime_array(item_type, 1);
    } else {
        out.size = compiler.get_declared_struct_member_size(struct_type, item_index);
    }
    out.vecsize = item_type.vecsize;
    out.columns = item_type.columns;
    out.offset = compiler.type_struct_member_offset(struct_type, item_index);
    if (item_type.array.size() == 0) {
        out.is_array = false;
    } else if (item_type.array.size() == 1) {
        out.is_array = true;
        out.array_count = item_type.array[0];   // NOTE: may be 0 for unbounded array
        out.array_stride = compiler.type_struct_member_array_stride(struct_type, item_index);
    } else {
        out_error = ErrMsg::error(fmt::format("arrays of arrays are not supported (struct item {})", out.name));
        return out;
    }
    if (out.base_type == Type::Struct) {
        for (size_t nested_item_index = 0; nested_item_index < item_type.member_types.size(); nested_item_index++) {
            const Type nested_type = parse_struct_item(compiler, item_type_id, nested_item_index, out_error);
            if (out_error.valid()) {
                return out;
            }
            out.struct_items.push_back(nested_type);
        }
    }
    return out;
}

Type Reflection::parse_toplevel_struct(const Compiler& compiler, const TypeID& struct_type_id, const std::string& name, ErrMsg& out_error) {
    Type out;
    out.name = name;
    const SPIRType& struct_type = compiler.get_type(struct_type_id);
    if (struct_type.basetype != SPIRType::Struct) {
        out_error = ErrMsg::error(fmt::format("toplevel item {} is not a struct", name));
        return out;
    }
    out.base_type = Type::Struct;
    out.size = (int) compiler.get_declared_struct_size_runtime_array(struct_type, 1);
    for (size_t item_index = 0; item_index < struct_type.member_types.size(); item_index++) {
        const Type item_type = parse_struct_item(compiler, struct_type_id, item_index, out_error);
        if (out_error.valid()) {
            return out;
        }
        out.struct_items.push_back(item_type);
    }
    return out;
}

} // namespace
