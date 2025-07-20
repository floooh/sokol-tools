/*
    Generate sokol-nim module.
*/
#include "sokolnim.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc::gen {

using namespace refl;

// need to special-case the gen-shader-arrays function because Nim
// needs the type appeneded to the first array element
void SokolNimGenerator::gen_shader_arrays(const GenInput& gen) {
    for (int slang_idx = 0; slang_idx < Slang::Num; slang_idx++) {
        Slang::Enum slang = Slang::from_index(slang_idx);
        if (gen.args.slang & Slang::bit(slang)) {
            const Spirvcross& spirvcross = gen.spirvcross[slang];
            const Bytecode& bytecode = gen.bytecode[slang];
            for (int snippet_index = 0; snippet_index < (int)gen.inp.snippets.size(); snippet_index++) {
                const Snippet& snippet = gen.inp.snippets[snippet_index];
                if ((snippet.type != Snippet::VS) && (snippet.type != Snippet::FS) && (snippet.type != Snippet::CS)) {
                    continue;
                }
                const SpirvcrossSource* src = spirvcross.find_source_by_snippet_index(snippet_index);
                assert(src);
                const BytecodeBlob* blob = bytecode.find_blob_by_snippet_index(snippet_index);
                std::vector<std::string> lines;
                pystring::splitlines(src->source_code, lines);
                // first write the source code in a comment block
                cbl_start();
                for (const std::string& line: lines) {
                    cbl("{}\n", replace_C_comment_tokens(line));
                }
                cbl_end();
                if (blob) {
                    const std::string array_name = shader_bytecode_array_name(snippet.name, slang);
                    gen_shader_array_start(gen, array_name, blob->data.size(), slang);
                    const size_t len = blob->data.size();
                    for (size_t i = 0; i < len; i++) {
                        if ((i & 15) == 0) {
                            l("    ");
                        }
                        if (0 == i) {
                            l("{:#04x}'u8,", blob->data[i]);
                        } else {
                            l("{:#04x},", blob->data[i]);
                        }
                        if ((i & 15) == 15) {
                            l("\n");
                        }
                    }
                    gen_shader_array_end(gen);
                } else {
                    // if no bytecode exists, write the source code, but also a byte array with a trailing 0
                    const std::string array_name = shader_source_array_name(snippet.name, slang);
                    const size_t len = src->source_code.length() + 1;
                    gen_shader_array_start(gen, array_name, len, slang);
                    for (size_t i = 0; i < len; i++) {
                        if ((i & 15) == 0) {
                            l("    ");
                        }
                        if (0 == i) {
                            l("{:#04x}'u8,", (int)src->source_code[i]);
                        } else {
                            l("{:#04x},", (int)src->source_code[i]);
                        }
                        if ((i & 15) == 15) {
                            l("\n");
                        }
                    }
                    gen_shader_array_end(gen);
                }
            }
        }
    }
}

void SokolNimGenerator::gen_prolog(const GenInput& gen) {
    l("import sokol/gfx as sg\n");
    for (const auto& header: gen.inp.headers) {
        l("{}\n", header);
    }
}

void SokolNimGenerator::gen_epilog(const GenInput& gen) {
    // empty
}

void SokolNimGenerator::gen_prerequisites(const GenInput& gen) {
    // empty
}

void SokolNimGenerator::gen_uniform_block_decl(const GenInput& gen, const UniformBlock& ub) {
    l_open("type {}* {{.packed.}} = object\n", struct_name(ub.name));
    int cur_offset = 0;
    for (const Type& uniform: ub.struct_info.struct_items) {
        int next_offset = uniform.offset;
        if (next_offset > cur_offset) {
            l("pad_{}: array[{}, uint8]\n", cur_offset, next_offset - cur_offset);
            cur_offset = next_offset;
        }
        const std::string align = (0 == cur_offset) ? fmt::format(" {{.align({}).}}", ub.struct_info.align) : "";
        if (gen.inp.ctype_map.count(uniform.type_as_glsl()) > 0) {
            // user-provided type names
            if (uniform.array_count == 0) {
                l("{}*{}: {}\n", uniform.name, align, gen.inp.ctype_map.at(uniform.type_as_glsl()));
            } else {
                l("{}*{}: array[{}, {}]\n", uniform.name, align, uniform.array_count, gen.inp.ctype_map.at(uniform.type_as_glsl()));
            }
        } else {
            // default type names (float)
            if (uniform.array_count == 0) {
                switch (uniform.type) {
                    case Type::Float:   l("{}*{}: float32\n", uniform.name, align); break;
                    case Type::Float2:  l("{}*{}: array[2, float32]\n", uniform.name, align); break;
                    case Type::Float3:  l("{}*{}: array[3, float32]\n", uniform.name, align); break;
                    case Type::Float4:  l("{}*{}: array[4, float32]\n", uniform.name, align); break;
                    case Type::Int:     l("{}*{}: int32\n", uniform.name, align); break;
                    case Type::Int2:    l("{}*{}: array[2, int32]\n", uniform.name, align); break;
                    case Type::Int3:    l("{}*{}: array[3, int32]\n", uniform.name, align); break;
                    case Type::Int4:    l("{}*{}: array[4, int32]\n", uniform.name, align); break;
                    case Type::Mat4x4:  l("{}*{}: array[16, float32]\n", uniform.name, align); break;
                    default:            l("INVALID_UNIFORM_TYPE"); break;
                }
            } else {
                switch (uniform.type) {
                    case Type::Float4:  l("{}*{}: array[{}, array[4, float32]]\n", uniform.name, align, uniform.array_count); break;
                    case Type::Int4:    l("{}*{}: array[{}, array[4, int32]]\n", uniform.name, align, uniform.array_count); break;
                    case Type::Mat4x4:  l("{}*{}: array[{}, array[16, float32]]\n", uniform.name, align, uniform.array_count); break;
                    default:            l("INVALID_UNIFORM_TYPE\n"); break;
                }
            }
        }
        cur_offset += uniform.size;
    }
    // pad to multiple of 16-bytes struct size
    const int round16 = roundup(cur_offset, 16);
    if (cur_offset != round16) {
        l("pad_{}: array[{}, uint8]\n", cur_offset, round16 - cur_offset);
    }
    l_close("\n");
}

void SokolNimGenerator::gen_struct_interior_decl_std430(const GenInput& gen, const Type& struc, const std::string& name, int alignment, int pad_to_size) {
    assert(struc.type == Type::Struct);
    assert(pad_to_size > 0);

    int cur_offset = 0;
    for (const Type& item: struc.struct_items) {
        int next_offset = item.offset;
        if (next_offset > cur_offset) {
            l("pad_{}: array[{}, uint8]\n", cur_offset, next_offset - cur_offset);
            cur_offset = next_offset;
        }
        std::string align = "";
        if ((cur_offset == 0) && (alignment > 0)) {
            align = fmt::format(" {{.align({}).}}", alignment);
        }
        if (item.type == Type::Struct) {
            // nested structs are regular members in Nim
            if (item.array_count == 0) {
                l("{}*{}: {}_{}\n", item.name, align, name, item.name);
            } else {
                l("{}*{}: array[{}, {}_{}]\n", item.name, align, item.array_count, name, item.name);
            }
        } else if (gen.inp.ctype_map.count(item.type_as_glsl()) > 0) {
            // user-provided type names
            if (item.array_count == 0) {
                l("{}*{}: {}\n", item.name, align, gen.inp.ctype_map.at(item.type_as_glsl()));
            } else {
                l("{}*{}: array[{}, {}]\n", item.name, align, item.array_count, gen.inp.ctype_map.at(item.type_as_glsl()));
            }
        } else {
            // default typenames
            if (item.array_count == 0) {
                switch (item.type) {
                    // NOTE: bool => int is not a bug!
                    case Type::Bool:    l("{}*{}: int32\n",              item.name, align); break;
                    case Type::Bool2:   l("{}*{}: array[2, int32]\n",    item.name, align); break;
                    case Type::Bool3:   l("{}*{}: array[3, int32]\n",    item.name, align); break;
                    case Type::Bool4:   l("{}*{}: array[4, int32]\n",    item.name, align); break;
                    case Type::Int:     l("{}*{}: int32\n",              item.name, align); break;
                    case Type::Int2:    l("{}*{}: array[2, int32]\n",    item.name, align); break;
                    case Type::Int3:    l("{}*{}: array[3, int32]\n",    item.name, align); break;
                    case Type::Int4:    l("{}*{}: array[4, int32]\n",    item.name, align); break;
                    case Type::UInt:    l("{}*{}: uint32\n",             item.name, align); break;
                    case Type::UInt2:   l("{}*{}: array[2, uint32]\n",   item.name, align); break;
                    case Type::UInt3:   l("{}*{}: array[3, uint32]\n",   item.name, align); break;
                    case Type::UInt4:   l("{}*{}: array[4, uint32]\n",   item.name, align); break;
                    case Type::Float:   l("{}*{}: float32\n",            item.name, align); break;
                    case Type::Float2:  l("{}*{}: array[2, float32]\n",  item.name, align); break;
                    case Type::Float3:  l("{}*{}: array[3, float32]\n",  item.name, align); break;
                    case Type::Float4:  l("{}*{}: array[4, float32]\n",  item.name, align); break;
                    case Type::Mat2x1:  l("{}*{}: array[2, float32]\n",  item.name, align); break;
                    case Type::Mat2x2:  l("{}*{}: array[4, float32]\n",  item.name, align); break;
                    case Type::Mat2x3:  l("{}*{}: array[6, float32]\n",  item.name, align); break;
                    case Type::Mat2x4:  l("{}*{}: array[8, float32]\n",  item.name, align); break;
                    case Type::Mat3x1:  l("{}*{}: array[3, float32]\n",  item.name, align); break;
                    case Type::Mat3x2:  l("{}*{}: array[6, float32]\n",  item.name, align); break;
                    case Type::Mat3x3:  l("{}*{}: array[9, float32]\n",  item.name, align); break;
                    case Type::Mat3x4:  l("{}*{}: array[12, float32]\n", item.name, align); break;
                    case Type::Mat4x1:  l("{}*{}: array[4, float32]\n",  item.name, align); break;
                    case Type::Mat4x2:  l("{}*{}: array[8, float32]\n",  item.name, align); break;
                    case Type::Mat4x3:  l("{}*{}: array[12, float32]\n", item.name, align); break;
                    case Type::Mat4x4:  l("{}*{}: array[16, float32]\n", item.name, align); break;
                    default: l("INVALID_TYPE\n"); break;
                }
            } else {
                switch (item.type) {
                    // NOTE: bool => int is not a bug!
                    case Type::Bool:    l("{}*{}: array[{}, int32]\n",              item.name, align, item.array_count); break;
                    case Type::Bool2:   l("{}*{}: array[{}, array[2, int32]]\n",    item.name, align, item.array_count); break;
                    case Type::Bool3:   l("{}*{}: array[{}, array[3, int32]]\n",    item.name, align, item.array_count); break;
                    case Type::Bool4:   l("{}*{}: array[{}, array[4, int32]]\n",    item.name, align, item.array_count); break;
                    case Type::Int:     l("{}*{}: array[{}, int32]\n",              item.name, align, item.array_count); break;
                    case Type::Int2:    l("{}*{}: array[{}, array[2, int32]]\n",    item.name, align, item.array_count); break;
                    case Type::Int3:    l("{}*{}: array[{}, array[3, int32]]\n",    item.name, align, item.array_count); break;
                    case Type::Int4:    l("{}*{}: array[{}, array[4, int32]]\n",    item.name, align, item.array_count); break;
                    case Type::UInt:    l("{}*{}: array[{}, uint32]\n",             item.name, align, item.array_count); break;
                    case Type::UInt2:   l("{}*{}: array[{}, array[2, uint32]]\n",   item.name, align, item.array_count); break;
                    case Type::UInt3:   l("{}*{}: array[{}, array[3, uint32]]\n",   item.name, align, item.array_count); break;
                    case Type::UInt4:   l("{}*{}: array[{}, array[4, uint32]]\n",   item.name, align, item.array_count); break;
                    case Type::Float:   l("{}*{}: array[{}, float32]\n",            item.name, align, item.array_count); break;
                    case Type::Float2:  l("{}*{}: array[{}, array[2, float32]]\n",  item.name, align, item.array_count); break;
                    case Type::Float3:  l("{}*{}: array[{}, array[3, float32]]\n",  item.name, align, item.array_count); break;
                    case Type::Float4:  l("{}*{}: array[{}, array[4, float32]]\n",  item.name, align, item.array_count); break;
                    case Type::Mat2x1:  l("{}*{}: array[{}, array[2, float32]]\n",  item.name, align, item.array_count); break;
                    case Type::Mat2x2:  l("{}*{}: array[{}, array[4, float32]]\n",  item.name, align, item.array_count); break;
                    case Type::Mat2x3:  l("{}*{}: array[{}, array[6, float32]]\n",  item.name, align, item.array_count); break;
                    case Type::Mat2x4:  l("{}*{}: array[{}, array[8, float32]]\n",  item.name, align, item.array_count); break;
                    case Type::Mat3x1:  l("{}*{}: array[{}, array[3, float32]]\n",  item.name, align, item.array_count); break;
                    case Type::Mat3x2:  l("{}*{}: array[{}, array[6, float32]]\n",  item.name, align, item.array_count); break;
                    case Type::Mat3x3:  l("{}*{}: array[{}, array[9, float32]]\n",  item.name, align, item.array_count); break;
                    case Type::Mat3x4:  l("{}*{}: array[{}, array[12, float32]]\n", item.name, align, item.array_count); break;
                    case Type::Mat4x1:  l("{}*{}: array[{}, array[4, float32]]\n",  item.name, align, item.array_count); break;
                    case Type::Mat4x2:  l("{}*{}: array[{}, array[8, float32]]\n",  item.name, align, item.array_count); break;
                    case Type::Mat4x3:  l("{}*{}: array[{}, array[12, float32]]\n", item.name, align, item.array_count); break;
                    case Type::Mat4x4:  l("{}*{}: array[{}, array[16, float32]]\n", item.name, align, item.array_count); break;
                    default: l("INVALID_TYPE\n"); break;
                }
            }
        }
        cur_offset += item.size;
    }
    if (cur_offset < pad_to_size) {
        l("pad_{}: array[{}, uint8]\n", cur_offset, pad_to_size - cur_offset);
    }
}

void SokolNimGenerator::recurse_unfold_structs(const GenInput& gen, const Type& struc, const std::string& name, int alignment, int pad_to_size) {
    // first recurse over nested structs because we need to extract those into
    // top-level Nim structs, extracted structs don't get an alignment since they
    // are nested
    for (const Type& item: struc.struct_items) {
        if (item.type == Type::Struct) {
            recurse_unfold_structs(gen, item, fmt::format("{}_{}", name, item.name), 0, item.size);
        }
    }
    l_open("type {}* {{.packed.}} = object\n", struct_name(name));
    gen_struct_interior_decl_std430(gen, struc, name, alignment, pad_to_size);
    l_close("\n");
}

void SokolNimGenerator::gen_storage_buffer_decl(const GenInput& gen, const Type& struc) {
    // blargh, Nim doesn't allow nested types, so we need be creative and recursively
    // generate top-level structs...
    recurse_unfold_structs(gen, struc, struc.struct_typename, struc.align, struc.size);
}

void SokolNimGenerator::gen_shader_desc_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("proc {}ShaderDesc*(backend: sg.Backend): sg.ShaderDesc =\n", to_camel_case(prog.name));
    l("result.label = \"{}_shader\"\n", prog.name);
    l_open("case backend:\n");
    for (int i = 0; i < Slang::Num; i++) {
        Slang::Enum slang = Slang::from_index(i);
        if (gen.args.slang & Slang::bit(slang)) {
            l_open("of {}:\n", backend(slang));
            for (int stage_index = 0; stage_index < ShaderStage::Num; stage_index++) {
                const ShaderStageArrayInfo& info = shader_stage_array_info(gen, prog, ShaderStage::from_index(stage_index), slang);
                if (info.stage == ShaderStage::Invalid) {
                    continue;
                }
                const StageReflection& refl = prog.stages[stage_index];
                std::string dsn;
                switch (info.stage) {
                    case ShaderStage::Vertex: dsn = "result.vertexFunc"; break;
                    case ShaderStage::Fragment: dsn = "result.fragmentFunc"; break;
                    case ShaderStage::Compute: dsn = "result.computeFunc"; break;
                    default: dsn = "INVALID"; break;
                }
                if (info.has_bytecode) {
                    l("{}.bytecode.ptr = {}\n", dsn, info.bytecode_array_name);
                    l("{}.bytecode.size = {}\n", dsn, info.bytecode_array_size);
                } else {
                    l("{}.source = cast[cstring](addr({}))\n", dsn, info.source_array_name);
                    const char* d3d11_tgt = hlsl_target(slang, info.stage);
                    if (d3d11_tgt) {
                        l("{}.d3d11Target = \"{}\"\n", dsn, d3d11_tgt);
                    }
                }
                l("{}.entry = \"{}\"\n", dsn, refl.entry_point_by_slang(slang));
            }
            if (Slang::is_msl(slang) && prog.has_cs()) {
                l("result.mtlThreadsPerThreadgroup.x = {}\n", prog.cs().cs_workgroup_size[0]);
                l("result.mtlThreadsPerThreadgroup.y = {}\n", prog.cs().cs_workgroup_size[1]);
                l("result.mtlThreadsPerThreadgroup.z = {}\n", prog.cs().cs_workgroup_size[2]);
            }
            if (prog.has_vs()) {
                for (int attr_index = 0; attr_index < StageAttr::Num; attr_index++) {
                    const StageAttr& attr = prog.vs().inputs[attr_index];
                    if (attr.slot >= 0) {
                        l("result.attrs[{}].base_type = {}\n", attr_index, attr_basetype(attr.type_info.basetype()));
                        if (Slang::is_glsl(slang)) {
                            l("result.attrs[{}].glslName = \"{}\"\n", attr_index, attr.name);
                        } else if (Slang::is_hlsl(slang)) {
                            l("result.attrs[{}].hlslSemName = \"{}\"\n", attr_index, attr.sem_name);
                            l("result.attrs[{}].hlslSemIndex = {}\n", attr_index, attr.sem_index);
                        }
                    }
                }
            }
            for (int ub_index = 0; ub_index < Bindings::MaxUniformBlocks; ub_index++) {
                const UniformBlock* ub = prog.bindings.find_uniform_block_by_sokol_slot(ub_index);
                if (ub) {
                    const std::string ubn = fmt::format("result.uniformBlocks[{}]", ub_index);
                    l("{}.stage = {}\n", ubn, shader_stage(ub->stage));
                    l("{}.layout = uniformLayoutStd140\n", ubn);
                    l("{}.size = {}\n", ubn, roundup(ub->struct_info.size, 16));
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlslRegisterBN = {}\n", ubn, ub->hlsl_register_b_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.mslBufferN = {}\n", ubn, ub->msl_buffer_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgslGroup0BindingN = {}\n", ubn, ub->wgsl_group0_binding_n);
                    } else if (Slang::is_glsl(slang) && (ub->struct_info.struct_items.size() > 0)) {
                        if (ub->flattened) {
                            // NOT A BUG (to take the type from the first struct item, but the size from the toplevel ub)
                            l("{}.glslUniforms[0].type = {}\n", ubn, flattened_uniform_type(ub->struct_info.struct_items[0].type));
                            l("{}.glslUniforms[0].arrayCount = {}\n", ubn, roundup(ub->struct_info.size, 16) / 16);
                            l("{}.glslUniforms[0].glslName = \"{}\"\n", ubn, ub->name);
                        } else {
                            for (int u_index = 0; u_index < (int)ub->struct_info.struct_items.size(); u_index++) {
                                const Type& u = ub->struct_info.struct_items[u_index];
                                const std::string un = fmt::format("{}.glsl_uniforms[{}]", ubn, u_index);
                                l("{}.type = {}\n", un, uniform_type(u.type));
                                l("{}.arrayCount = {}\n", un, u.array_count);
                                l("{}.glslName = \"{}.{}\"\n", un, ub->inst_name, u.name);
                            }
                        }
                    }
                }
            }
            for (int sbuf_index = 0; sbuf_index < Bindings::MaxStorageBufferBindingsPerStage; sbuf_index++) {
                const StorageBuffer* sbuf = prog.bindings.find_storage_buffer_by_sokol_slot(sbuf_index);
                if (sbuf) {
                    const std::string& sbn = fmt::format("result.storageBuffers[{}]", sbuf_index);
                    l("{}.stage = {}\n", sbn, shader_stage(sbuf->stage));
                    l("{}.readonly = {}\n", sbn, sbuf->readonly);
                    if (Slang::is_hlsl(slang)) {
                        if (sbuf->hlsl_register_t_n >= 0) {
                            l("{}.hlslRegisterTN = {}\n", sbn, sbuf->hlsl_register_t_n);
                        }
                        if (sbuf->hlsl_register_u_n >= 0) {
                            l("{}.hlslRegisterUN = {}\n", sbn, sbuf->hlsl_register_u_n);
                        }
                    } else if (Slang::is_msl(slang)) {
                        l("{}.mslBufferN = {}\n", sbn, sbuf->msl_buffer_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgslGroup1BindingN = {}\n", sbn, sbuf->wgsl_group1_binding_n);
                    } else if (Slang::is_glsl(slang)) {
                        l("{}.glslBindingN = {}\n", sbn, sbuf->glsl_binding_n);
                    }
                }
            }
            for (int simg_index = 0; simg_index < Bindings::MaxStorageImageBindingsPerStage; simg_index++) {
                const StorageImage* simg = prog.bindings.find_storage_image_by_sokol_slot(simg_index);
                if (simg) {
                    const std::string& sin = fmt::format("result.storageImages[{}]", simg_index);
                    l("{}.stage = {}\n", sin, shader_stage(simg->stage));
                    l("{}.imageType = {}\n", sin, image_type(simg->type));
                    l("{}.accessFormat = {}\n", sin, storage_pixel_format(simg->access_format));
                    l("{}.writeonly = {}\n", sin, simg->writeonly);
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlslRegisterUN = {}\n", sin, simg->hlsl_register_u_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.mslTextureN = {}\n", sin, simg->msl_texture_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgslGroup1BindingN = {}\n", sin, simg->wgsl_group1_binding_n);
                    } else if (Slang::is_glsl(slang)) {
                        l("{}.glslBindingN = {}\n", sin, simg->glsl_binding_n);
                    }
                }
            }
            for (int tex_index = 0; tex_index < Bindings::MaxTextureBindingsPerStage; tex_index++) {
                const Texture* tex = prog.bindings.find_texture_by_sokol_slot(tex_index);
                if (tex) {
                    const std::string tn = fmt::format("result.textures[{}]", tex_index);
                    l("{}.stage = {}\n", tn, shader_stage(tex->stage));
                    l("{}.multisampled = {}\n", tn, tex->multisampled ? "true" : "false");
                    l("{}.imageType = {}\n", tn, image_type(tex->type));
                    l("{}.sampleType = {}\n", tn, image_sample_type(tex->sample_type));
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlslRegisterTN = {}\n", tn, tex->hlsl_register_t_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.mslTextureN = {}\n", tn, tex->msl_texture_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgslGroup1BindingN = {}\n", tn, tex->wgsl_group1_binding_n);
                    }
                }
            }
            for (int smp_index = 0; smp_index < Bindings::MaxSamplers; smp_index++) {
                const Sampler* smp = prog.bindings.find_sampler_by_sokol_slot(smp_index);
                if (smp) {
                    const std::string sn = fmt::format("result.samplers[{}]", smp_index);
                    l("{}.stage = {}\n", sn, shader_stage(smp->stage));
                    l("{}.samplerType = {}\n", sn, sampler_type(smp->type));
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlslRegisterSN = {}\n", sn, smp->hlsl_register_s_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.mslSamplerN = {}\n", sn, smp->msl_sampler_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgslGroup1BindingN = {}\n", sn, smp->wgsl_group1_binding_n);
                    }
                }
            }
            for (int tex_smp_index = 0; tex_smp_index < Bindings::MaxTextureSamplers; tex_smp_index++) {
                const TextureSampler* tex_smp = prog.bindings.find_texture_sampler_by_sokol_slot(tex_smp_index);
                if (tex_smp) {
                    const std::string tsn = fmt::format("result.textureSamplerPairs[{}]", tex_smp_index);
                    l("{}.stage = {}\n", tsn, shader_stage(tex_smp->stage));
                    l("{}.textureSlot = {}\n", tsn, prog.bindings.find_texture_by_name(tex_smp->texture_name)->sokol_slot);
                    l("{}.samplerSlot = {}\n", tsn, prog.bindings.find_sampler_by_name(tex_smp->sampler_name)->sokol_slot);
                    if (Slang::is_glsl(slang)) {
                        l("{}.glslName = \"{}\"\n", tsn, tex_smp->name);
                    }
                }
            }
            l_close(); // current switch branch
        }
    }
    l("else: discard\n");
    l_close();
    l_close();
}

void SokolNimGenerator::gen_shader_array_start(const GenInput& gen, const std::string& array_name, size_t num_bytes, Slang::Enum slang) {
    l("const {}: array[{}, uint8] = [\n", array_name, num_bytes);
}

void SokolNimGenerator::gen_shader_array_end(const GenInput& gen) {
    l("\n]\n");
}

std::string SokolNimGenerator::lang_name() {
    return "Nim";
}

std::string SokolNimGenerator::comment_block_start() {
    return "#";
}

std::string SokolNimGenerator::comment_block_end() {
    return "#";
}

std::string SokolNimGenerator::comment_block_line_prefix() {
    return "#";
}

std::string SokolNimGenerator::shader_bytecode_array_name(const std::string& snippet_name, Slang::Enum slang) {
    return to_camel_case(fmt::format("{}_bytecode_{}", snippet_name, Slang::to_str(slang)));
}

std::string SokolNimGenerator::shader_source_array_name(const std::string& snippet_name, Slang::Enum slang) {
    return to_camel_case(fmt::format("{}_source_{}", snippet_name, Slang::to_str(slang)));
}

std::string SokolNimGenerator::get_shader_desc_help(const std::string& prog_name) {
    return fmt::format("{}ShaderDesc(sg.queryBackend())\n", to_camel_case(prog_name));
}

std::string SokolNimGenerator::shader_stage(ShaderStage::Enum e) {
    switch (e) {
        case ShaderStage::Vertex: return "shaderStageVertex";
        case ShaderStage::Fragment: return "shaderStageFragment";
        case ShaderStage::Compute: return "shaderStageCompute";
        default: return "INVALID";
    }
}

std::string SokolNimGenerator::attr_basetype(Type::Enum e) {
    switch (e) {
        case Type::Float:   return "shaderAttrBaseTypeFloat";
        case Type::Int:     return "shaderAttrBaseTypeSint";
        case Type::UInt:    return "shaderAttrBaseTypeUint";
        default: return "INVALID";
    }
}

std::string SokolNimGenerator::uniform_type(Type::Enum e) {
    switch (e) {
        case Type::Float:   return "uniformTypeFloat";
        case Type::Float2:  return "uniformTypeFloat2";
        case Type::Float3:  return "uniformTypeFloat3";
        case Type::Float4:  return "uniformTypeFloat4";
        case Type::Int:     return "uniformTypeInt";
        case Type::Int2:    return "uniformTypeInt2";
        case Type::Int3:    return "uniformTypeInt3";
        case Type::Int4:    return "uniformTypeInt4";
        case Type::Mat4x4:  return "uniformTypeMat4";
        default: return "INVALID";
    }
}

std::string SokolNimGenerator::flattened_uniform_type(Type::Enum e) {
    switch (e) {
        case Type::Float:
        case Type::Float2:
        case Type::Float3:
        case Type::Float4:
        case Type::Mat4x4:
             return "uniformTypeFloat4";
        case Type::Int:
        case Type::Int2:
        case Type::Int3:
        case Type::Int4:
            return "uniformTypeInt4";
        default:
            return "INVALID";
    }
}

std::string SokolNimGenerator::image_type(ImageType::Enum e) {
    switch (e) {
        case ImageType::_2D:     return "imageType2d";
        case ImageType::CUBE:    return "imageTypeCube";
        case ImageType::_3D:     return "imageType3d";
        case ImageType::ARRAY:   return "imageTypeArray";
        default: return "INVALID";
    }
}

std::string SokolNimGenerator::image_sample_type(ImageSampleType::Enum e) {
    switch (e) {
        case ImageSampleType::FLOAT:               return "imageSampleTypeFloat";
        case ImageSampleType::DEPTH:               return "imageSampleTypeDepth";
        case ImageSampleType::SINT:                return "imageSampleTypeSint";
        case ImageSampleType::UINT:                return "imageSamplerTypeUint";
        case ImageSampleType::UNFILTERABLE_FLOAT:  return "imageSamplerTypeUnfilterableFloat";
        default: return "INVALID";
    }
}

std::string SokolNimGenerator::sampler_type(SamplerType::Enum e) {
    switch (e) {
        case SamplerType::FILTERING:     return "samplerTypeFiltering";
        case SamplerType::COMPARISON:    return "samplerTypeComparison";
        case SamplerType::NONFILTERING:  return "samplerTypeNonfiltering";
        default: return "INVALID";
    }
}

std::string SokolNimGenerator::storage_pixel_format(refl::StoragePixelFormat::Enum e) {
    switch (e) {
        case StoragePixelFormat::RGBA8:     return "pixelFormatRgba8";
        case StoragePixelFormat::RGBA8SN:   return "pixelFormatRgba8sn";
        case StoragePixelFormat::RGBA8UI:   return "pixelFormatRgba8ui";
        case StoragePixelFormat::RGBA8SI:   return "pixelFormatRgbs8si";
        case StoragePixelFormat::RGBA16UI:  return "pixelFormatRgba16ui";
        case StoragePixelFormat::RGBA16SI:  return "pixelFormatRgba16si";
        case StoragePixelFormat::RGBA16F:   return "pixelFormatRgba16f";
        case StoragePixelFormat::R32UI:     return "pixelFormatR32ui";
        case StoragePixelFormat::R32SI:     return "pixelFormatR32si";
        case StoragePixelFormat::R32F:      return "pixelFormatR32f";
        case StoragePixelFormat::RG32UI:    return "pixelFormatRg32ui";
        case StoragePixelFormat::RG32SI:    return "pixelFormatRg32si";
        case StoragePixelFormat::RG32F:     return "pixelFormatRg32f";
        case StoragePixelFormat::RGBA32UI:  return "pixelFormatRgba32ui";
        case StoragePixelFormat::RGBA32SI:  return "pixelFormatRgba32si";
        case StoragePixelFormat::RGBA32F:   return "pixelFormatRgba32f";
        default: return "INVALID";
    }
}

std::string SokolNimGenerator::backend(Slang::Enum e) {
    switch (e) {
        case Slang::GLSL410:
        case Slang::GLSL430:
            return "backendGlcore";
        case Slang::GLSL300ES:
        case Slang::GLSL310ES:
            return "backendGles3";
        case Slang::HLSL4:
        case Slang::HLSL5:
            return "backendD3d11";
        case Slang::METAL_MACOS:
            return "backendMetalMacos";
        case Slang::METAL_IOS:
            return "backendMetalIos";
        case Slang::METAL_SIM:
            return "backendMetalSimulator";
        case Slang::WGSL:
            return "backendWgpu";
        default:
            return "<INVALID>";
    }
}

std::string SokolNimGenerator::struct_name(const std::string& name) {
    return to_pascal_case(name);
}

std::string SokolNimGenerator::vertex_attr_name(const std::string& prog_name, const StageAttr& attr) {
    return to_camel_case(fmt::format("ATTR_{}_{}", prog_name, attr.name));
}

std::string SokolNimGenerator::texture_bind_slot_name(const Texture& tex) {
    return to_camel_case(fmt::format("TEX_{}", tex.name));
}

std::string SokolNimGenerator::sampler_bind_slot_name(const Sampler& smp) {
    return to_camel_case(fmt::format("SMP_{}", smp.name));
}

std::string SokolNimGenerator::uniform_block_bind_slot_name(const UniformBlock& ub) {
    return to_camel_case(fmt::format("UB_{}", ub.name));
}

std::string SokolNimGenerator::storage_buffer_bind_slot_name(const StorageBuffer& sbuf) {
    return to_camel_case(fmt::format("SBUF_{}", sbuf.name));
}

std::string SokolNimGenerator::storage_image_bind_slot_name(const StorageImage& simg) {
    return to_camel_case(fmt::format("SIMG_{}", simg.name));
}

std::string SokolNimGenerator::vertex_attr_definition(const std::string& prog_name, const StageAttr& attr) {
    return fmt::format("const {}* = {}", vertex_attr_name(prog_name, attr), attr.slot);
}

std::string SokolNimGenerator::texture_bind_slot_definition(const Texture& tex) {
    return fmt::format("const {}* = {}", texture_bind_slot_name(tex), tex.sokol_slot);
}

std::string SokolNimGenerator::sampler_bind_slot_definition(const Sampler& smp) {
    return fmt::format("const {}* = {}", sampler_bind_slot_name(smp), smp.sokol_slot);
}

std::string SokolNimGenerator::uniform_block_bind_slot_definition(const UniformBlock& ub) {
    return fmt::format("const {}* = {}", uniform_block_bind_slot_name(ub), ub.sokol_slot);
}

std::string SokolNimGenerator::storage_buffer_bind_slot_definition(const StorageBuffer& sbuf) {
    return fmt::format("const {}* = {}", storage_buffer_bind_slot_name(sbuf), sbuf.sokol_slot);
}

std::string SokolNimGenerator::storage_image_bind_slot_definition(const StorageImage& simg) {
    return fmt::format("const {}* = {}", storage_image_bind_slot_name(simg), simg.sokol_slot);
}

} // namespace
