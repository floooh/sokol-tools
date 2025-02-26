/*
    Generate bare output in text or binary format
*/
#include "yaml.h"
#include "bare.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc::gen {

using namespace refl;

ErrMsg YamlGenerator::generate(const GenInput& gen) {
    tab_width = 2;
    // first run the BareGenerator to generate shader source/blob files
    ErrMsg err = BareGenerator::generate(gen);
    if (err.valid()) {
        return err;
    }
    // next generate a YAML file with reflection info
    content.clear();
    l_open("shaders:\n");
    for (int slang_idx = 0; slang_idx < Slang::Num; slang_idx++) {
        Slang::Enum slang = Slang::from_index(slang_idx);
        if (gen.args.slang & Slang::bit(slang)) {
            l_open("-\n");
            l("slang: {}\n", Slang::to_str(slang));
            l_open("programs:\n");
            for (const ProgramReflection& prog: gen.refl.progs) {
                l_open("-\n");
                l("name: {}\n", prog.name);
                for (int stage_index = 0; stage_index < ShaderStage::Num; stage_index++) {
                    const ShaderStageArrayInfo& info = shader_stage_array_info(gen, prog, ShaderStage::from_index(stage_index), slang);
                    if (info.stage == ShaderStage::Invalid) {
                        continue;
                    }
                    std::string func;
                    switch (info.stage) {
                        case ShaderStage::Vertex: func = "vertex_func"; break;
                        case ShaderStage::Fragment: func = "fragment_func"; break;
                        case ShaderStage::Compute: func = "compute_func"; break;
                        default: func = "INVALID"; break;
                    }
                    l_open("{}:\n", func);
                    const StageReflection& refl = prog.stages[stage_index];
                    const std::string file_path = shader_file_path(gen, prog.name, ShaderStage::to_str(info.stage), slang, info.has_bytecode);
                    l("path: {}\n", file_path);
                    l("is_binary: {}\n", info.has_bytecode);
                    l("entry_point: {}\n", refl.entry_point_by_slang(slang));
                    const char* d3d11_tgt = hlsl_target(slang, info.stage);
                    if (d3d11_tgt) {
                        l("d3d11_target: {}\n", d3d11_tgt);
                    }
                    l_close();
                }
                if (Slang::is_msl(slang) && prog.has_cs()) {
                    l_open("mtl_threads_per_threadgroup:\n");
                    l("x: {}\n", prog.cs().cs_workgroup_size[0]);
                    l("y: {}\n", prog.cs().cs_workgroup_size[1]);
                    l("z: {}\n", prog.cs().cs_workgroup_size[2]);
                    l_close();
                }
                if (prog.has_vs() && prog.vs().num_inputs() > 0) {
                    l_open("attrs:\n");
                    for (const auto& attr: prog.vs().inputs) {
                        if (attr.slot >= 0) {
                            gen_attr(attr, slang);
                        }
                    }
                    l_close();
                }
                if (prog.bindings.uniform_blocks.size() > 0) {
                    l_open("uniform_blocks:\n");
                    for (const auto& uniform_block: prog.bindings.uniform_blocks) {
                        gen_uniform_block(gen, uniform_block, slang);
                    }
                    l_close();
                }
                if (prog.bindings.storage_buffers.size() > 0) {
                    l_open("storage_buffers:\n");
                    for (const auto& sbuf: prog.bindings.storage_buffers) {
                        gen_storage_buffer(sbuf, slang);
                    }
                    l_close();
                }
                if (prog.bindings.images.size() > 0) {
                    l_open("images:\n");
                    for (const auto& image: prog.bindings.images) {
                        gen_image(image, slang);
                    }
                    l_close();
                }
                if (prog.bindings.samplers.size() > 0) {
                    l_open("samplers:\n");
                    for (const auto& sampler: prog.bindings.samplers) {
                        gen_sampler(sampler, slang);
                    }
                    l_close();
                }
                if (prog.bindings.image_samplers.size() > 0) {
                    l_open("image_sampler_pairs:\n");
                    for (const auto& image_sampler: prog.bindings.image_samplers) {
                        gen_image_sampler(image_sampler, slang);
                    }
                    l_close();
                }
                l_close();
            }
            l_close();
            l_close();
        }
    }
    l_close();

    // write result into output file
    const std::string file_path = fmt::format("{}_{}reflection.yaml", gen.args.output, mod_prefix);
    FILE* f = fopen(file_path.c_str(), "w");
    if (!f) {
        return ErrMsg::error(gen.inp.base_path, 0, fmt::format("failed to open output file '{}'", file_path));
    }
    fwrite(content.c_str(), content.length(), 1, f);
    fclose(f);
    return ErrMsg();
}

void YamlGenerator::gen_attr(const StageAttr& attr, Slang::Enum slang) {
    l_open("-\n");
    l("slot: {}\n", attr.slot);
    l("type: {}\n", uniform_type(attr.type_info.type));
    if (Slang::is_glsl(slang)) {
        l("glsl_name: {}\n", attr.name);
    } else if (Slang::is_hlsl(slang)) {
        l("hlsl_sem_name: {}\n", attr.sem_name);
        l("hlsl_sem_index: {}\n", attr.sem_index);
    }
    l_close();
}

void YamlGenerator::gen_uniform_block(const GenInput& gen, const UniformBlock& ub, Slang::Enum slang) {
    l_open("-\n");
    l("slot: {}\n", ub.sokol_slot);
    l("stage: {}\n", shader_stage(ub.stage));
    l("size: {}\n", roundup(ub.struct_info.size, 16));
    l("struct_name: {}\n", ub.name);
    l("inst_name: {}\n", ub.inst_name);
    if (Slang::is_hlsl(slang)) {
        l("hlsl_register_b_n: {}\n", ub.hlsl_register_b_n);
    } else if (Slang::is_msl(slang)) {
        l("msl_buffer_n: {}\n", ub.msl_buffer_n);
    } else if (Slang::is_wgsl(slang)) {
        l("wgsl_group0_binding_n: {}\n", ub.wgsl_group0_binding_n);
    } else if (Slang::is_glsl(slang)) {
        l_open("glsl_uniforms:\n");
        if (ub.flattened) {
            l_open("-\n");
            l("type: {}\n", flattened_uniform_type(ub.struct_info.struct_items[0].type));
            l("array_count: {}\n", roundup(ub.struct_info.size, 16) / 16);
            l("offset: 0\n");
            l("glsl_name: {}\n", ub.name);
            l_close();
        } else {
            for (const Type& u: ub.struct_info.struct_items) {
                l_open("-\n");
                l("type: {}\n", uniform_type(u.type));
                l("array_count: {}\n", u.array_count);
                l("offset: {}\n", u.offset);
                l("glsl_name: {}\n", u.name);
                l_close();
            }
        }
        l_close();
    }
    if (gen.args.reflection) {
        gen_uniform_block_refl(ub);
    }
    l_close();
}

void YamlGenerator::gen_uniform_block_refl(const UniformBlock& ub) {
    l_open("members:\n");
    for (const Type& u: ub.struct_info.struct_items) {
        l_open("-\n");
        l("name: {}\n", u.name);
        l("type: {}\n", uniform_type(u.type));
        l("array_count: {}\n", u.array_count);
        l("offset: {}\n", u.offset);
        l_close();
    }
    l_close();
}

void YamlGenerator::gen_storage_buffer(const StorageBuffer& sbuf, Slang::Enum slang) {
    const auto& item = sbuf.struct_info.struct_items[0];
    l_open("-\n");
    l("slot: {}\n", sbuf.sokol_slot);
    l("stage: {}\n", shader_stage(sbuf.stage));
    l("size: {}\n", sbuf.struct_info.size);
    l("align: {}\n", sbuf.struct_info.align);
    l("struct_name: {}\n", sbuf.name);
    l("inst_name: {}\n", sbuf.inst_name);
    l("inner_struct_name: {}\n", item.struct_typename);
    l("readonly: {}\n", sbuf.readonly);
    if (Slang::is_hlsl(slang)) {
        if (sbuf.hlsl_register_t_n >= 0) {
            l("hlsl_register_t_n: {}\n", sbuf.hlsl_register_t_n);
        }
        if (sbuf.hlsl_register_u_n >= 0) {
            l("hlsl_register_u_n: {}\n", sbuf.hlsl_register_u_n);
        }
    } else if (Slang::is_msl(slang)) {
        l("msl_buffer_n: {}\n", sbuf.msl_buffer_n);
    } else if (Slang::is_wgsl(slang)) {
        l("wgsl_group1_binding_n: {}\n", sbuf.wgsl_group1_binding_n);
    } else if (Slang::is_glsl(slang)) {
        l("glsl_binding_n: {}\n", sbuf.glsl_binding_n);
    }
    l_close();
}

void YamlGenerator::gen_image(const Image& img, Slang::Enum slang) {
    l_open("-\n");
    l("slot: {}\n", img.sokol_slot);
    l("stage: {}\n", shader_stage(img.stage));
    l("name: {}\n", img.name);
    l("multisampled: {}\n", img.multisampled);
    l("type: {}\n", image_type(img.type));
    l("sample_type: {}\n", image_sample_type(img.sample_type));
    if (Slang::is_hlsl(slang)) {
        l("hlsl_register_t_n: {}\n", img.hlsl_register_t_n);
    } else if (Slang::is_msl(slang)) {
        l("msl_texture_n: {}\n", img.msl_texture_n);
    } else if (Slang::is_wgsl(slang)) {
        l("wgsl_group1_binding_n: {}\n", img.wgsl_group1_binding_n);
    }
    l_close();
}

void YamlGenerator::gen_sampler(const Sampler& smp, Slang::Enum slang) {
    l_open("-\n");
    l("slot: {}\n", smp.sokol_slot);
    l("stage: {}\n", shader_stage(smp.stage));
    l("name: {}\n", smp.name);
    l("sampler_type: {}\n", sampler_type(smp.type));
    if (Slang::is_hlsl(slang)) {
        l("hlsl_register_s_n: {}\n", smp.hlsl_register_s_n);
    } else if (Slang::is_msl(slang)) {
        l("msl_sampler_n: {}\n", smp.msl_sampler_n);
    } else if (Slang::is_wgsl(slang)) {
        l("wgsl_group1_binding_n: {}\n", smp.wgsl_group1_binding_n);
    }
    l_close();
}

void YamlGenerator::gen_image_sampler(const ImageSampler& img_smp, Slang::Enum slang) {
    l_open("-\n");
    l("slot: {}\n", img_smp.sokol_slot);
    l("stage: {}\n", shader_stage(img_smp.stage));
    l("name: {}\n", img_smp.name);
    l("image_name: {}\n", img_smp.image_name);
    l("sampler_name: {}\n", img_smp.sampler_name);
    if (Slang::is_glsl(slang)) {
        l("glsl_name: {}\n", img_smp.name);
    }
    l_close();
}

std::string YamlGenerator::uniform_type(Type::Enum e) {
    return Type::type_to_glsl(e);
}

std::string YamlGenerator::flattened_uniform_type(Type::Enum e) {
    switch (e) {
        case Type::Float:
        case Type::Float2:
        case Type::Float3:
        case Type::Float4:
        case Type::Mat4x4:
             return Type::type_to_glsl(Type::Float4);
        case Type::Int:
        case Type::Int2:
        case Type::Int3:
        case Type::Int4:
            return Type::type_to_glsl(Type::Int4);
        default:
            return Type::type_to_glsl(Type::Invalid);
    }
}

std::string YamlGenerator::shader_stage(ShaderStage::Enum e) {
    return ShaderStage::to_str(e);
}

std::string YamlGenerator::image_type(ImageType::Enum e) {
    return ImageType::to_str(e);
}

std::string YamlGenerator::image_sample_type(ImageSampleType::Enum e) {
    return ImageSampleType::to_str(e);
}

std::string YamlGenerator::sampler_type(SamplerType::Enum e) {
    return SamplerType::to_str(e);
}

} // namespace
