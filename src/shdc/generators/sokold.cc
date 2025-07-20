/*
    D language generator.
*/
#include "sokold.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc::gen {

using namespace refl;

void SokolDGenerator::gen_prolog(const GenInput& gen) {
    std::string root, ext;
    pystring::os::path::splitext(root, ext, gen.args.input);
    l("module shaders.{};\n", pystring::os::path::basename(root));
    l("import sg = sokol.gfx;\n");
    for (const auto& header: gen.inp.headers) {
        l("{};\n", header);
    }
    l("extern(C):\n\n");
}

void SokolDGenerator::gen_epilog(const GenInput& gen) {
    // empty
}

void SokolDGenerator::gen_prerequisites(const GenInput& gen) {
    // empty
}

void SokolDGenerator::gen_uniform_block_decl(const GenInput& gen, const UniformBlock& ub) {
    l_open("struct {} {{\n", struct_name(ub.name));
    int cur_offset = 0;
    for (const Type& uniform: ub.struct_info.struct_items) {
        const int align = (cur_offset == 0) ? ub.struct_info.align : 1;
        int next_offset = uniform.offset;
        if (next_offset > cur_offset) {
            l("align({}) ubyte[{}] _pad_{} = 0;\n", align, next_offset - cur_offset, cur_offset);
            cur_offset = next_offset;
        }
        if (gen.inp.ctype_map.count(uniform.type_as_glsl()) > 0) {
            // user-provided type names
            if (uniform.array_count == 0) {
                l("align({}) {} {};\n", align, gen.inp.ctype_map.at(uniform.type_as_glsl()), uniform.name);
            } else {
                l("align({}) {}[{}] {};\n", align, gen.inp.ctype_map.at(uniform.type_as_glsl()), uniform.array_count, uniform.name);
            }
        } else {
            // default type names (float)
            if (uniform.array_count == 0) {
                switch (uniform.type) {
                    case Type::Float:   l("align({}) float {} = 0;\n", align, uniform.name); break;
                    case Type::Float2:  l("align({}) float[2] {} = 0;\n", align, uniform.name); break;
                    case Type::Float3:  l("align({}) float[3] {} = 0;\n", align, uniform.name); break;
                    case Type::Float4:  l("align({}) float[4] {} = 0;\n", align, uniform.name); break;
                    case Type::Int:     l("align({}) int {} = 0;\n", align, uniform.name); break;
                    case Type::Int2:    l("align({}) int[2] {} = 0;\n", align, uniform.name); break;
                    case Type::Int3:    l("align({}) int[3] {} = 0;\n", align, uniform.name); break;
                    case Type::Int4:    l("align({}) int[4] {} = 0;\n", align, uniform.name); break;
                    case Type::Mat4x4:  l("align({}) float[16] {} = 0;\n", align, uniform.name); break;
                    default:            l("INVALID_UNIFORM_TYPE;\n"); break;
                }
            } else {
                switch (uniform.type) {
                    case Type::Float4:  l("align({}) float[4][{}] {} = 0;\n", align, uniform.array_count, uniform.name); break;
                    case Type::Int4:    l("align({}) int[4][{}] {} = 0;\n", align, uniform.array_count, uniform.name); break;
                    case Type::Mat4x4:  l("align({}) float[16][{}] {} = 0;\n", align, uniform.array_count, uniform.name); break;
                    default:            l("INVALID_UNIFORM_TYPE;\n"); break;
                }
            }
        }
        cur_offset += uniform.size;
    }
    // pad to multiple of 16-bytes struct size
    const int round16 = roundup(cur_offset, 16);
    if (cur_offset < round16) {
        l("align(1) ubyte[{}] _pad_{} = 0;\n", round16 - cur_offset, cur_offset);
    }
    l_close("}}\n");
}

void SokolDGenerator::gen_struct_interior_decl_std430(const GenInput& gen, const Type& struc, int alignment, int pad_to_size) {
    assert(struc.type == Type::Struct);
    assert(alignment > 0);
    assert(pad_to_size > 0);

    int cur_offset = 0;
    for (const Type& item: struc.struct_items) {
        const int align = (cur_offset == 0) ? alignment : 1;
        int next_offset = item.offset;
        if (next_offset > cur_offset) {
            l("align({}) ubyte[{}] _pad_{} = 0;\n", align, next_offset - cur_offset, cur_offset);
            cur_offset = next_offset;
        }
        if (item.type == Type::Struct) {
            // recurse into nested struct
            l_open("struct {}_s {{\n", struct_name(item.name));
            gen_struct_interior_decl_std430(gen, item, 1, item.size);
            l_close("}}\n");
            if (item.array_count == 0) {
                l("align({}) {}_s {};\n", align, struct_name(item.name), item.name);
            } else {
                l("align({}) {}_s[{}] {};\n", align, struct_name(item.name), item.array_count, item.name);
            }
        } else if (gen.inp.ctype_map.count(item.type_as_glsl()) > 0) {
            // user-provided type names
            if (item.array_count == 0) {
                l("align({}) {} {};\n", align, gen.inp.ctype_map.at(item.type_as_glsl()), item.name);
            } else {
                l("align({}) {}[{}] {};\n", align, gen.inp.ctype_map.at(item.type_as_glsl()), item.array_count, item.name);
            }
        } else {
            // default typenames
            if (item.array_count == 0) {
                switch (item.type) {
                    // NOTE: bool => int is not a bug!
                    case Type::Bool:    l("align({}) int {} = 0;\n",       align, item.name); break;
                    case Type::Bool2:   l("align({}) int[2] {} = 0;\n",    align, item.name); break;
                    case Type::Bool3:   l("align({}) int[3] {} = 0;\n",    align, item.name); break;
                    case Type::Bool4:   l("align({}) int[4] {} = 0;\n",    align, item.name); break;
                    case Type::Int:     l("align({}) int {} = 0;\n",       align, item.name); break;
                    case Type::Int2:    l("align({}) int[2] {} = 0;\n",    align, item.name); break;
                    case Type::Int3:    l("align({}) int[3] {} = 0;\n",    align, item.name); break;
                    case Type::Int4:    l("align({}) int[4] {} = 0;\n",    align, item.name); break;
                    case Type::UInt:    l("align({}) uint {} = 0;\n",      align, item.name); break;
                    case Type::UInt2:   l("align({}) uint[2] {} = 0;\n",   align, item.name); break;
                    case Type::UInt3:   l("align({}) uint[3] {} = 0;\n",   align, item.name); break;
                    case Type::UInt4:   l("align({}) uint[4] {} = 0;\n",   align, item.name); break;
                    case Type::Float:   l("align({}) float {} = 0;\n",     align, item.name); break;
                    case Type::Float2:  l("align({}) float[2] {} = 0;\n",  align, item.name); break;
                    case Type::Float3:  l("align({}) float[3] {} = 0;\n",  align, item.name); break;
                    case Type::Float4:  l("align({}) float[4] {} = 0;\n",  align, item.name); break;
                    case Type::Mat2x1:  l("align({}) float[2] {} = 0;\n",  align, item.name); break;
                    case Type::Mat2x2:  l("align({}) float[4] {} = 0;\n",  align, item.name); break;
                    case Type::Mat2x3:  l("align({}) float[6] {} = 0;\n",  align, item.name); break;
                    case Type::Mat2x4:  l("align({}) float[8] {} = 0;\n",  align, item.name); break;
                    case Type::Mat3x1:  l("align({}) float[3] {} = 0;\n",  align, item.name); break;
                    case Type::Mat3x2:  l("align({}) float[6] {} = 0;\n",  align, item.name); break;
                    case Type::Mat3x3:  l("align({}) float[9] {} = 0;\n",  align, item.name); break;
                    case Type::Mat3x4:  l("align({}) float[12] {} = 0;\n", align, item.name); break;
                    case Type::Mat4x1:  l("align({}) float[4] {} = 0;\n",  align, item.name); break;
                    case Type::Mat4x2:  l("align({}) float[8] {} = 0;\n",  align, item.name); break;
                    case Type::Mat4x3:  l("align({}) float[12] {} = 0;\n", align, item.name); break;
                    case Type::Mat4x4:  l("align({}) float[16] {} = 0;\n", align, item.name); break;
                    default: l("INVALID_TYPE\n"); break;
                }
            } else {
                switch (item.type) {
                    // NOTE: bool => int is not a bug!
                    case Type::Bool:    l("align({}) int[{}] {} = 0;\n",       align, item.array_count, item.name); break;
                    case Type::Bool2:   l("align({}) int[2][{}] {} = 0;\n",    align, item.array_count, item.name); break;
                    case Type::Bool3:   l("align({}) int[3][{}] {} = 0;\n",    align, item.array_count, item.name); break;
                    case Type::Bool4:   l("align({}) int[4][{}] {} = 0;\n",    align, item.array_count, item.name); break;
                    case Type::Int:     l("align({}) int[{}] {} = 0;",         align, item.array_count, item.name); break;
                    case Type::Int2:    l("align({}) int[2] {} = 0;\n",        align, item.array_count, item.name); break;
                    case Type::Int3:    l("align({}) int[3] {} = 0;\n",        align, item.array_count, item.name); break;
                    case Type::Int4:    l("align({}) int[4] {} = 0;\n",        align, item.array_count, item.name); break;
                    case Type::UInt:    l("align({}) uint[{}] {} = 0;\n",      align, item.array_count, item.name); break;
                    case Type::UInt2:   l("align({}) uint[2][{}] {} = 0;\n",   align, item.array_count, item.name); break;
                    case Type::UInt3:   l("align({}) uint[3][{}] {} = 0;\n",   align, item.array_count, item.name); break;
                    case Type::UInt4:   l("align({}) uint[4][{}] {} = 0;\n",   align, item.array_count, item.name); break;
                    case Type::Float:   l("align({}) float[{}] {} = 0;\n",     align, item.array_count, item.name); break;
                    case Type::Float2:  l("align({}) float[2][{}] {} = 0;\n",  align, item.array_count, item.name); break;
                    case Type::Float3:  l("align({}) float[3][{}] {} = 0;\n",  align, item.array_count, item.name); break;
                    case Type::Float4:  l("align({}) float[4][{}] {} = 0;\n",  align, item.array_count, item.name); break;
                    case Type::Mat2x1:  l("align({}) float[2][{}] {} = 0;\n",  align, item.array_count, item.name); break;
                    case Type::Mat2x2:  l("align({}) float[4][{}] {} = 0;\n",  align, item.array_count, item.name); break;
                    case Type::Mat2x3:  l("align({}) float[6][{}] {} = 0;\n",  align, item.array_count, item.name); break;
                    case Type::Mat2x4:  l("align({}) float[8][{}] {} = 0;\n",  align, item.array_count, item.name); break;
                    case Type::Mat3x1:  l("align({}) float[3][{}] {} = 0;\n",  align, item.array_count, item.name); break;
                    case Type::Mat3x2:  l("align({}) float[6][{}] {} = 0;\n",  align, item.array_count, item.name); break;
                    case Type::Mat3x3:  l("align({}) float[9][{}] {} = 0;\n",  align, item.array_count, item.name); break;
                    case Type::Mat3x4:  l("align({}) float[12][{}] {} = 0;\n", align, item.array_count, item.name); break;
                    case Type::Mat4x1:  l("align({}) float[4][{}] {} = 0;\n",  align, item.array_count, item.name); break;
                    case Type::Mat4x2:  l("align({}) float[8][{}] {} = 0;\n",  align, item.array_count, item.name); break;
                    case Type::Mat4x3:  l("align({}) float[12][{}] {} = 0;\n", align, item.array_count, item.name); break;
                    case Type::Mat4x4:  l("align({}) float[16][{}] {} = 0;\n", align, item.array_count, item.name); break;
                    default: l("INVALID_TYPE\n"); break;
                }
            }
        }
        cur_offset += item.size;
    }
    if (cur_offset < pad_to_size) {
        l("align(1) ubyte[{}] _pad_{} = 0;\n", pad_to_size - cur_offset, cur_offset);
    }
}

void SokolDGenerator::gen_storage_buffer_decl(const GenInput& gen, const Type& struc) {
    l_open("struct {} {{\n", struct_name(struc.struct_typename));
    gen_struct_interior_decl_std430(gen, struc, struc.align, struc.size);
    l_close("}}\n");
}

void SokolDGenerator::gen_shader_array_start(const GenInput& gen, const std::string& array_name, size_t num_bytes, Slang::Enum slang) {
    l("__gshared char[{}] {} = [\n", num_bytes, array_name);

}
void SokolDGenerator::gen_shader_array_end(const GenInput& gen) {
    l("\n];\n");
}

void SokolDGenerator::gen_shader_desc_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("sg.ShaderDesc {}ShaderDesc(sg.Backend backend) @trusted @nogc nothrow {{\n", prog.name);
    l("sg.ShaderDesc desc;\n");
    l("desc.label = \"{}_shader\";\n", prog.name);
    l_open("switch (backend) {{\n");
    for (int i = 0; i < Slang::Num; i++) {
        Slang::Enum slang = Slang::from_index(i);
        if (gen.args.slang & Slang::bit(slang)) {
            l_open("case {}:\n", backend(slang));
            for (int stage_index = 0; stage_index < ShaderStage::Num; stage_index++) {
                const ShaderStageArrayInfo& info = shader_stage_array_info(gen, prog, ShaderStage::from_index(stage_index), slang);
                if (info.stage == ShaderStage::Invalid) {
                    continue;
                }
                const StageReflection& refl = prog.stages[stage_index];
                std::string dsn;
                switch (info.stage) {
                    case ShaderStage::Vertex: dsn = "desc.vertex_func"; break;
                    case ShaderStage::Fragment: dsn = "desc.fragment_func"; break;
                    case ShaderStage::Compute: dsn = "desc.compute_func"; break;
                    default: dsn = "INVALID"; break;
                }
                if (info.has_bytecode) {
                    l("{}.bytecode.ptr = {}.ptr;\n", dsn, info.bytecode_array_name);
                    l("{}.bytecode.size = {};\n", dsn, info.bytecode_array_size);
                } else {
                    l("{}.source = &{}[0];\n", dsn, info.source_array_name);
                    const char* d3d11_tgt = hlsl_target(slang, info.stage);
                    if (d3d11_tgt) {
                        l("{}.d3d11_target = \"{}\";\n", dsn, d3d11_tgt);
                    }
                }
                l("{}.entry = \"{}\";\n", dsn, refl.entry_point_by_slang(slang));
            }
            if (Slang::is_msl(slang) && prog.has_cs()) {
                l("desc.mtl_threads_per_threadgroup.x = {};\n", prog.cs().cs_workgroup_size[0]);
                l("desc.mtl_threads_per_threadgroup.y = {};\n", prog.cs().cs_workgroup_size[1]);
                l("desc.mtl_threads_per_threadgroup.z = {};\n", prog.cs().cs_workgroup_size[2]);
            }
            if (prog.has_vs()) {
                for (int attr_index = 0; attr_index < StageAttr::Num; attr_index++) {
                    const StageAttr& attr = prog.vs().inputs[attr_index];
                    if (attr.slot >= 0) {
                        l("desc.attrs[{}].base_type = {};\n", attr_index, attr_basetype(attr.type_info.basetype()));
                        if (Slang::is_glsl(slang)) {
                            l("desc.attrs[{}].glsl_name = \"{}\";\n", attr_index, attr.name);
                        } else if (Slang::is_hlsl(slang)) {
                            l("desc.attrs[{}].hlsl_sem_name = \"{}\";\n", attr_index, attr.sem_name);
                            l("desc.attrs[{}].hlsl_sem_index = {};\n", attr_index, attr.sem_index);
                        }
                    }
                }
            }
            for (int ub_index = 0; ub_index < Bindings::MaxUniformBlocks; ub_index++) {
                const UniformBlock* ub = prog.bindings.find_uniform_block_by_sokol_slot(ub_index);
                if (ub) {
                    const std::string ubn = fmt::format("desc.uniform_blocks[{}]", ub_index);
                    l("{}.stage = {};\n", ubn, shader_stage(ub->stage));
                    l("{}.layout = sg.UniformLayout.Std140;\n", ubn);
                    l("{}.size = {};\n", ubn, roundup(ub->struct_info.size, 16));
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlsl_register_b_n = {};\n", ubn, ub->hlsl_register_b_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.msl_buffer_n = {};\n", ubn, ub->msl_buffer_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgsl_group0_binding_n = {};\n", ubn, ub->wgsl_group0_binding_n);
                    } if (Slang::is_glsl(slang) && (ub->struct_info.struct_items.size() > 0)) {
                        if (ub->flattened) {
                            // NOT A BUG (to take the type from the first struct item, but the size from the toplevel ub)
                            l("{}.glsl_uniforms[0].type = {};\n", ubn, flattened_uniform_type(ub->struct_info.struct_items[0].type));
                            l("{}.glsl_uniforms[0].array_count = {};\n", ubn, roundup(ub->struct_info.size, 16) / 16);
                            l("{}.glsl_uniforms[0].glsl_name = \"{}\";\n", ubn, ub->name);
                        } else {
                            for (int u_index = 0; u_index < (int)ub->struct_info.struct_items.size(); u_index++) {
                                const Type& u = ub->struct_info.struct_items[u_index];
                                const std::string un = fmt::format("{}.glsl_uniforms[{}]", ubn, u_index);
                                l("{}.type = {};\n", un, uniform_type(u.type));
                                l("{}.array_count = {};\n", un, u.array_count);
                                l("{}.glsl_name = \"{}.{}\";\n", un, ub->inst_name, u.name);
                            }
                        }
                    }
                }
            }
            for (int sbuf_index = 0; sbuf_index < Bindings::MaxStorageBufferBindingsPerStage; sbuf_index++) {
                const StorageBuffer* sbuf = prog.bindings.find_storage_buffer_by_sokol_slot(sbuf_index);
                if (sbuf) {
                    const std::string& sbn = fmt::format("desc.storage_buffers[{}]", sbuf_index);
                    l("{}.stage = {};\n", sbn, shader_stage(sbuf->stage));
                    l("{}.readonly = {};\n", sbn, sbuf->readonly);
                    if (Slang::is_hlsl(slang)) {
                        if (sbuf->hlsl_register_t_n >= 0) {
                            l("{}.hlsl_register_t_n = {};\n", sbn, sbuf->hlsl_register_t_n);
                        }
                        if (sbuf->hlsl_register_u_n >= 0) {
                            l("{}.hlsl_register_u_n = {};\n", sbn, sbuf->hlsl_register_u_n);
                        }
                    } else if (Slang::is_msl(slang)) {
                        l("{}.msl_buffer_n = {};\n", sbn, sbuf->msl_buffer_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgsl_group1_binding_n = {};\n", sbn, sbuf->wgsl_group1_binding_n);
                    } else if (Slang::is_glsl(slang)) {
                        l("{}.glsl_binding_n = {};\n", sbn, sbuf->glsl_binding_n);
                    }
                }
            }
            for (int simg_index = 0; simg_index < Bindings::MaxStorageImageBindingsPerStage; simg_index++) {
                const StorageImage* simg = prog.bindings.find_storage_image_by_sokol_slot(simg_index);
                if (simg) {
                    const std::string& sin = fmt::format("desc.storage_images[{}]", simg_index);
                    l("{}.stage = {};\n", sin, shader_stage(simg->stage));
                    l("{}.image_type = {};\n", sin, image_type(simg->type));
                    l("{}.access_format = {};\n", sin, storage_pixel_format(simg->access_format));
                    l("{}.writeonly = {};\n", sin, simg->writeonly);
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlsl_register_u_n = {};\n", sin, simg->hlsl_register_u_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.msl_texture_n = {};\n", sin, simg->msl_texture_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgsl_group1_binding_n = {};\n", sin, simg->wgsl_group1_binding_n);
                    } else if (Slang::is_glsl(slang)) {
                        l("{}.glsl_binding_n = {};\n", sin, simg->glsl_binding_n);
                    }
                }
            }
            for (int tex_index = 0; tex_index < Bindings::MaxTextureBindingsPerStage; tex_index++) {
                const Texture* tex = prog.bindings.find_texture_by_sokol_slot(tex_index);
                if (tex) {
                    const std::string tn = fmt::format("desc.textures[{}]", tex_index);
                    l("{}.stage = {};\n", tn, shader_stage(tex->stage));
                    l("{}.multisampled = {};\n", tn, tex->multisampled ? "true" : "false");
                    l("{}.image_type = {};\n", tn, image_type(tex->type));
                    l("{}.sample_type = {};\n", tn, image_sample_type(tex->sample_type));
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlsl_register_t_n = {};\n", tn, tex->hlsl_register_t_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.msl_texture_n = {};\n", tn, tex->msl_texture_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgsl_group1_binding_n = {};\n", tn, tex->wgsl_group1_binding_n);
                    }
                }
            }
            for (int smp_index = 0; smp_index < Bindings::MaxSamplers; smp_index++) {
                const Sampler* smp = prog.bindings.find_sampler_by_sokol_slot(smp_index);
                if (smp) {
                    const std::string sn = fmt::format("desc.samplers[{}]", smp_index);
                    l("{}.stage = {};\n", sn, shader_stage(smp->stage));
                    l("{}.sampler_type = {};\n", sn, sampler_type(smp->type));
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlsl_register_s_n = {};\n", sn, smp->hlsl_register_s_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.msl_sampler_n = {};\n", sn, smp->msl_sampler_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgsl_group1_binding_n = {};\n", sn, smp->wgsl_group1_binding_n);
                    }
                }
            }
            for (int tex_smp_index = 0; tex_smp_index < Bindings::MaxTextureSamplers; tex_smp_index++) {
                const TextureSampler* tex_smp = prog.bindings.find_texture_sampler_by_sokol_slot(tex_smp_index);
                if (tex_smp) {
                    const std::string tsn = fmt::format("desc.texture_sampler_pairs[{}]", tex_smp_index);
                    l("{}.stage = {};\n", tsn, shader_stage(tex_smp->stage));
                    l("{}.texture_slot = {};\n", tsn, prog.bindings.find_texture_by_name(tex_smp->texture_name)->sokol_slot);
                    l("{}.sampler_slot = {};\n", tsn, prog.bindings.find_sampler_by_name(tex_smp->sampler_name)->sokol_slot);
                    if (Slang::is_glsl(slang)) {
                        l("{}.glsl_name = \"{}\";\n", tsn, tex_smp->name);
                    }
                }
            }
            l("break;\n");
            l_close(); // current switch branch
        }
    }
    l("default: break;\n");
    l_close("}}\n"); // close switch statement
    l("return desc;\n");
    l_close("}}\n"); // close function
}

std::string SokolDGenerator::lang_name() {
    return "D";
}

std::string SokolDGenerator::comment_block_start() {
    return "/*";
}

std::string SokolDGenerator::comment_block_end() {
    return "*/";
}

std::string SokolDGenerator::comment_block_line_prefix() {
    return "";
}

std::string SokolDGenerator::shader_bytecode_array_name(const std::string& snippet_name, Slang::Enum slang) {
    return pystring::upper(fmt::format("{}_bytecode_{}", snippet_name, Slang::to_str(slang)));
}

std::string SokolDGenerator::shader_source_array_name(const std::string& snippet_name, Slang::Enum slang) {
    return pystring::upper(fmt::format("{}_source_{}", snippet_name, Slang::to_str(slang)));
}

std::string SokolDGenerator::get_shader_desc_help(const std::string& prog_name) {
    return fmt::format("{}ShaderDesc(sg.queryBackend());\n", prog_name);
}

std::string SokolDGenerator::shader_stage(ShaderStage::Enum e) {
    switch (e) {
        case ShaderStage::Vertex: return "sg.ShaderStage.Vertex";
        case ShaderStage::Fragment: return "sg.ShaderStage.Fragment";
        case ShaderStage::Compute: return "sg.ShaderStage.Compute";
        default: return "INVALID";
    }
}

std::string SokolDGenerator::attr_basetype(Type::Enum e) {
    switch (e) {
        case Type::Float:   return "sg.ShaderAttrBaseType.Float";
        case Type::Int:     return "sg.ShaderAttrBaseType.Sint";
        case Type::UInt:    return "sg.ShaderAttrBaseType.Uint";
        default: return "INVALID";
    }
}

std::string SokolDGenerator::uniform_type(Type::Enum e) {
    switch (e) {
        case Type::Float:  return "sg.UniformType.Float";
        case Type::Float2: return "sg.UniformType.Float2";
        case Type::Float3: return "sg.UniformType.Float3";
        case Type::Float4: return "sg.UniformType.Float4";
        case Type::Int:    return "sg.UniformType.Int";
        case Type::Int2:   return "sg.UniformType.Int2";
        case Type::Int3:   return "sg.UniformType.Int3";
        case Type::Int4:   return "sg.UniformType.Int4";
        case Type::Mat4x4: return "sg.UniformType.Mat4";
        default: return "INVALID";
    }
}

std::string SokolDGenerator::flattened_uniform_type(Type::Enum e) {
    switch (e) {
        case Type::Float:
        case Type::Float2:
        case Type::Float3:
        case Type::Float4:
        case Type::Mat4x4:
             return "sg.UniformType.Float4";
        case Type::Int:
        case Type::Int2:
        case Type::Int3:
        case Type::Int4:
            return "sg.UniformType.Int4";
        default:
            return "INVALID";
    }
}

std::string SokolDGenerator::image_type(ImageType::Enum e) {
    switch (e) {
        case ImageType::_2D:     return "sg.ImageType._2d";
        case ImageType::CUBE:    return "sg.ImageType.Cube";
        case ImageType::_3D:     return "sg.ImageType._3d";
        case ImageType::ARRAY:   return "sg.ImageType.Array";
        default: return "INVALID";
    }
}

std::string SokolDGenerator::image_sample_type(ImageSampleType::Enum e) {
    switch (e) {
        case ImageSampleType::FLOAT:               return "sg.ImageSampleType.Float";
        case ImageSampleType::DEPTH:               return "sg.ImageSampleType.Depth";
        case ImageSampleType::SINT:                return "sg.ImageSampleType.Sint";
        case ImageSampleType::UINT:                return "sg.ImageSampleType.Uint";
        case ImageSampleType::UNFILTERABLE_FLOAT:  return "sg.ImageSampleType.UnfilterableFloat";
        default: return "INVALID";
    }
}

std::string SokolDGenerator::sampler_type(SamplerType::Enum e) {
    switch (e) {
        case SamplerType::FILTERING:     return "sg.SamplerType.Filtering";
        case SamplerType::COMPARISON:    return "sg.SamplerType.Comparison";
        case SamplerType::NONFILTERING:  return "sg.SamplerType.Nonfiltering";
        default: return "INVALID";
    }
}

std::string SokolDGenerator::storage_pixel_format(refl::StoragePixelFormat::Enum e) {
    switch (e) {
        case StoragePixelFormat::RGBA8:     return "sg.PixelFormat.Rgba8";
        case StoragePixelFormat::RGBA8SN:   return "sg.PixelFormat.Rgba8sn";
        case StoragePixelFormat::RGBA8UI:   return "sg.PixelFormat.Rgba8ui";
        case StoragePixelFormat::RGBA8SI:   return "sg.PixelFormat.Rgbs8si";
        case StoragePixelFormat::RGBA16UI:  return "sg.PixelFormat.Rgba16ui";
        case StoragePixelFormat::RGBA16SI:  return "sg.PixelFormat.Rgba16si";
        case StoragePixelFormat::RGBA16F:   return "sg.PixelFormat.Rgba16f";
        case StoragePixelFormat::R32UI:     return "sg.PixelFormat.R32ui";
        case StoragePixelFormat::R32SI:     return "sg.PixelFormat.R32si";
        case StoragePixelFormat::R32F:      return "sg.PixelFormat.R32f";
        case StoragePixelFormat::RG32UI:    return "sg.PixelFormat.Rg32ui";
        case StoragePixelFormat::RG32SI:    return "sg.PixelFormat.Rg32si";
        case StoragePixelFormat::RG32F:     return "sg.PixelFormat.Rg32f";
        case StoragePixelFormat::RGBA32UI:  return "sg.PixelFormat.Rgba32ui";
        case StoragePixelFormat::RGBA32SI:  return "sg.PixelFormat.Rgba32si";
        case StoragePixelFormat::RGBA32F:   return "sg.PixelFormat.Rgba32f";
        default: return "INVALID";
    }
}

std::string SokolDGenerator::backend(Slang::Enum e) {
    switch (e) {
        case Slang::GLSL410:
        case Slang::GLSL430:
            return "sg.Backend.Glcore";
        case Slang::GLSL300ES:
        case Slang::GLSL310ES:
            return "sg.Backend.Gles3";
        case Slang::HLSL4:
        case Slang::HLSL5:
            return "sg.Backend.D3d11";
        case Slang::METAL_MACOS:
            return "sg.Backend.Metal_macos";
        case Slang::METAL_IOS:
            return "sg.Backend.Metal_ios";
        case Slang::METAL_SIM:
            return "sg.Backend.Metal_simulator";
        case Slang::WGSL:
            return "sg.Backend.Wgpu";
        default:
            return "INVALID";
    }
}

std::string SokolDGenerator::struct_name(const std::string& name) {
    return to_pascal_case(name);
}

std::string SokolDGenerator::vertex_attr_name(const std::string& prog_name, const StageAttr& attr) {
    return pystring::upper(fmt::format("ATTR_{}_{}", prog_name, attr.name));
}

std::string SokolDGenerator::texture_bind_slot_name(const Texture& tex) {
    return pystring::upper(fmt::format("TEX_{}", tex.name));
}

std::string SokolDGenerator::sampler_bind_slot_name(const Sampler& smp) {
    return pystring::upper(fmt::format("SMP_{}", smp.name));
}

std::string SokolDGenerator::uniform_block_bind_slot_name(const UniformBlock& ub) {
    return pystring::upper(fmt::format("UB_{}", ub.name));
}

std::string SokolDGenerator::storage_buffer_bind_slot_name(const StorageBuffer& sbuf) {
    return pystring::upper(fmt::format("SBUF_{}", sbuf.name));
}

std::string SokolDGenerator::storage_image_bind_slot_name(const StorageImage& simg) {
    return pystring::upper(fmt::format("SIMG_{}", simg.name));
}

static std::string const_def(const std::string& name, int slot) {
    return fmt::format("enum {} = {};", name, slot);
}

std::string SokolDGenerator::vertex_attr_definition(const std::string& prog_name, const StageAttr& attr) {
    return const_def(vertex_attr_name(prog_name, attr), attr.slot);
}

std::string SokolDGenerator::texture_bind_slot_definition(const Texture& tex) {
    return const_def(texture_bind_slot_name(tex), tex.sokol_slot);
}

std::string SokolDGenerator::sampler_bind_slot_definition(const Sampler& smp) {
    return const_def(sampler_bind_slot_name(smp), smp.sokol_slot);
}

std::string SokolDGenerator::uniform_block_bind_slot_definition(const UniformBlock& ub) {
    return const_def(uniform_block_bind_slot_name(ub), ub.sokol_slot);
}

std::string SokolDGenerator::storage_buffer_bind_slot_definition(const StorageBuffer& sbuf) {
    return const_def(storage_buffer_bind_slot_name(sbuf), sbuf.sokol_slot);
}

std::string SokolDGenerator::storage_image_bind_slot_definition(const StorageImage& simg) {
    return const_def(storage_image_bind_slot_name(simg), simg.sokol_slot);
}

} // namespace
