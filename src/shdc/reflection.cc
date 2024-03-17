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

Reflection Reflection::parse(const Compiler& compiler, const Snippet& snippet, Slang::Enum slang) {
    Reflection refl;

    ShaderResources shd_resources = compiler.get_shader_resources();
    // shader stage
    switch (compiler.get_execution_model()) {
        case spv::ExecutionModelVertex:   refl.stage = ShaderStage::VS; break;
        case spv::ExecutionModelFragment: refl.stage = ShaderStage::FS; break;
        default: refl.stage = ShaderStage::INVALID; break;
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
        VertexAttr refl_attr;
        refl_attr.slot = compiler.get_decoration(res_attr.id, spv::DecorationLocation);
        refl_attr.name = res_attr.name;
        refl_attr.sem_name = "TEXCOORD";
        refl_attr.sem_index = refl_attr.slot;
        refl.inputs[refl_attr.slot] = refl_attr;
    }
    for (const Resource& res_attr: shd_resources.stage_outputs) {
        VertexAttr refl_attr;
        refl_attr.slot = compiler.get_decoration(res_attr.id, spv::DecorationLocation);
        refl_attr.name = res_attr.name;
        refl_attr.sem_name = "TEXCOORD";
        refl_attr.sem_index = refl_attr.slot;
        refl.outputs[refl_attr.slot] = refl_attr;
    }
    // uniform blocks
    for (const Resource& ub_res: shd_resources.uniform_buffers) {
        std::string n = compiler.get_name(ub_res.id);
        UniformBlock refl_ub;
        const SPIRType& ub_type = compiler.get_type(ub_res.base_type_id);
        refl_ub.stage = refl.stage;
        refl_ub.slot = compiler.get_decoration(ub_res.id, spv::DecorationBinding);
        refl_ub.size = (int) compiler.get_declared_struct_size(ub_type);
        refl_ub.struct_name = ub_res.name;
        refl_ub.inst_name = compiler.get_name(ub_res.id);
        if (refl_ub.inst_name.empty()) {
            refl_ub.inst_name = compiler.get_fallback_name(ub_res.id);
        }
        refl_ub.flattened = Spirvcross::can_flatten_uniform_block(compiler, ub_res);
        for (int m_index = 0; m_index < (int)ub_type.member_types.size(); m_index++) {
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

bool Reflection::merge_bindings(const std::vector<Bindings>& in_bindings, const std::string& inp_base_path, Bindings& out_bindings, ErrMsg& out_error) {
    out_bindings = Bindings();
    out_error = ErrMsg();
    for (const Bindings& src_bindings: in_bindings) {

        // merge identical uniform blocks
        for (const UniformBlock& ub: src_bindings.uniform_blocks) {
            const UniformBlock* other_ub = out_bindings.find_uniform_block_by_name(ub.struct_name);
            if (other_ub) {
                // another uniform block of the same name exists, make sure it's identical
                if (!ub.equals(*other_ub)) {
                    out_error = ErrMsg::error(inp_base_path, 0, fmt::format("conflicting uniform block definitions found for '{}'", ub.struct_name));
                    return false;
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
                    return false;
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
                    return false;
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
                    return false;
                }
            } else {
                out_bindings.image_samplers.push_back(img_smp);
            }
        }
    }
    return true;
}

} // namespace
