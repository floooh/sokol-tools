/*
    Generate sokol-rust module.
*/
#include "sokolrust.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc::gen {

using namespace refl;

void SokolRustGenerator::gen_prolog(const GenInput& gen) {
    l("#![allow(dead_code)]\n");
    l("use sokol::gfx as sg;\n");
    for (const auto& header: gen.inp.headers) {
        l("{};\n", header);
    }
}

void SokolRustGenerator::gen_epilog(const GenInput& gen) {
    // empty
}

void SokolRustGenerator::gen_prerequisites(const GenInput& gen) {
    // empty
}

void SokolRustGenerator::gen_uniform_block_decl(const GenInput& gen, const UniformBlock& ub) {
    l("#[repr(C, align({}))]\n", ub.struct_info.align);
    l_open("pub struct {} {{\n", struct_name(ub.name));
    int cur_offset = 0;
    for (const Type& uniform: ub.struct_info.struct_items) {
        int next_offset = uniform.offset;
        if (next_offset > cur_offset) {
            l("pub _pad_{}: [u8; {}],\n", cur_offset, next_offset - cur_offset);
            cur_offset = next_offset;
        }
        if (gen.inp.ctype_map.count(uniform.type_as_glsl()) > 0) {
            // user-provided type names
            if (uniform.array_count == 0) {
                l("pub {}: {},\n", uniform.name, gen.inp.ctype_map.at(uniform.type_as_glsl()));
            } else {
                l("pub {}: [{}; {}],\n", uniform.name, gen.inp.ctype_map.at(uniform.type_as_glsl()), uniform.array_count);
            }
        } else {
            // default type names (float)
            if (uniform.array_count == 0) {
                switch (uniform.type) {
                    case Type::Float:   l("pub {}: f32,\n", uniform.name); break;
                    case Type::Float2:  l("pub {}: [f32; 2],\n", uniform.name); break;
                    case Type::Float3:  l("pub {}: [f32; 3],\n", uniform.name); break;
                    case Type::Float4:  l("pub {}: [f32; 4],\n", uniform.name); break;
                    case Type::Int:     l("pub {}: i32,\n", uniform.name); break;
                    case Type::Int2:    l("pub {}: [i32; 2],\n", uniform.name); break;
                    case Type::Int3:    l("pub {}: [i32; 3],\n", uniform.name); break;
                    case Type::Int4:    l("pub {}: [i32; 4],\n", uniform.name); break;
                    case Type::Mat4x4:  l("pub {}: [f32; 16],\n", uniform.name); break;
                    default:            l("INVALID_UNIFORM_TYPE"); break;
                }
            } else {
                switch (uniform.type) {
                    case Type::Float4:  l("pub {}: [[f32; 4]; {}],\n", uniform.name, uniform.array_count); break;
                    case Type::Int4:    l("pub {}: [[i32; 4]; {}],\n", uniform.name, uniform.array_count); break;
                    case Type::Mat4x4:  l("pub {}: [[f32; 16]; {}],\n", uniform.name, uniform.array_count); break;
                    default:            l("INVALID_UNIFORM_TYPE;\n"); break;
                }
            }
        }
        cur_offset += uniform.size;
    }
    // pad to multiple of 16-bytes struct size
    const int round16 = roundup(cur_offset, 16);
    if (cur_offset != round16) {
        l("pub _pad_{}: [u8; {}],\n", cur_offset, round16 - cur_offset);
    }
    l_close("}}\n");
}

void SokolRustGenerator::gen_struct_interior_decl_std430(const GenInput& gen, const Type& struc, const std::string& name, int pad_to_size) {
    assert(struc.type == Type::Struct);
    assert(pad_to_size > 0);

    int cur_offset = 0;
    for (const Type& item: struc.struct_items) {
        int next_offset = item.offset;
        if (next_offset > cur_offset) {
            l("pub _pad_{}: [u8; {}],\n", cur_offset, next_offset - cur_offset);
            cur_offset = next_offset;
        }
        if (item.type == Type::Struct) {
            // Rust doesn't allow nested struct declarations, so we need to reference
            // an externally created struct declaration
            if (item.array_count == 0) {
                l("pub {}: {}_{},", item.name, name, item.name);
            } else {
                l("pub {}: [{}_{}, {}],\n", item.name, name, item.name, item.array_count);
            }
        } else if (gen.inp.ctype_map.count(item.type_as_glsl()) > 0) {
            // user-mapped typename
            if (item.array_count == 0) {
                l("pub {}: {},\n", item.name, gen.inp.ctype_map.at(item.type_as_glsl()));
            } else {
                l("pub {}: [{}; {}],\n", item.name, gen.inp.ctype_map.at(item.type_as_glsl()), item.array_count);
            }
        } else {
            // default typenames
            if (item.array_count == 0) {
                switch (item.type) {
                    // NOTE: bool => int is not a bug!
                    case Type::Bool:    l("pub {}: i32,\n", item.name); break;
                    case Type::Bool2:   l("pub {}: [i32; 2],\n", item.name); break;
                    case Type::Bool3:   l("pub {}: [i32; 3],\n", item.name); break;
                    case Type::Bool4:   l("pub {}: [i32; 4],\n", item.name); break;
                    case Type::Int:     l("pub {}: i32,\n", item.name); break;
                    case Type::Int2:    l("pub {}: [i32; 2],\n", item.name); break;
                    case Type::Int3:    l("pub {}: [i32; 3],\n", item.name); break;
                    case Type::Int4:    l("pub {}: [i32; 4],\n", item.name); break;
                    case Type::UInt:    l("pub {}: u32,\n", item.name); break;
                    case Type::UInt2:   l("pub {}: [u32; 2],\n", item.name); break;
                    case Type::UInt3:   l("pub {}: [u32; 3],\n", item.name); break;
                    case Type::UInt4:   l("pub {}: [u32; 4],\n", item.name); break;
                    case Type::Float:   l("pub {}: f32,\n", item.name); break;
                    case Type::Float2:  l("pub {}: [f32; 2],\n", item.name); break;
                    case Type::Float3:  l("pub {}: [f32; 3],\n", item.name); break;
                    case Type::Float4:  l("pub {}: [f32; 4],\n", item.name); break;
                    case Type::Mat2x1:  l("pub {}: [f32; 2],\n", item.name); break;
                    case Type::Mat2x2:  l("pub {}: [f32; 4],\n", item.name); break;
                    case Type::Mat2x3:  l("pub {}: [f32; 6],\n", item.name); break;
                    case Type::Mat2x4:  l("pub {}: [f32; 8],\n", item.name); break;
                    case Type::Mat3x1:  l("pub {}: [f32; 3],\n", item.name); break;
                    case Type::Mat3x2:  l("pub {}: [f32; 6],\n", item.name); break;
                    case Type::Mat3x3:  l("pub {}: [f32; 9],\n", item.name); break;
                    case Type::Mat3x4:  l("pub {}: [f32; 12],\n", item.name); break;
                    case Type::Mat4x1:  l("pub {}: [f32; 4],\n", item.name); break;
                    case Type::Mat4x2:  l("pub {}: [f32; 8],\n", item.name); break;
                    case Type::Mat4x3:  l("pub {}: [f32; 12],\n", item.name); break;
                    case Type::Mat4x4:  l("pub {}: [f32; 16],\n", item.name); break;
                    default: l("INVALID_TYPE\n"); break;
                }
            } else {
                switch (item.type) {
                    // NOTE: bool => int is not a bug!
                    case Type::Bool:    l("pub {}: [i32; {}],\n", item.name, item.array_count); break;
                    case Type::Bool2:   l("pub {}: [[i32; 2]; {}],\n", item.name, item.array_count); break;
                    case Type::Bool3:   l("pub {}: [[i32; 3]; {}],\n", item.name, item.array_count); break;
                    case Type::Bool4:   l("pub {}: [[i32; 4]; {}],\n", item.name, item.array_count); break;
                    case Type::Int:     l("pub {}: [[i32; {}],\n", item.name, item.array_count); break;
                    case Type::Int2:    l("pub {}: [[i32; 2]; {}],\n", item.name, item.array_count); break;
                    case Type::Int3:    l("pub {}: [[i32; 3]; {}],\n", item.name, item.array_count); break;
                    case Type::Int4:    l("pub {}: [[i32; 4]; {}],\n", item.name, item.array_count); break;
                    case Type::UInt:    l("pub {}: [u32; {}],\n", item.name, item.array_count); break;
                    case Type::UInt2:   l("pub {}: [[u32; 2]; {}],\n", item.name, item.array_count); break;
                    case Type::UInt3:   l("pub {}: [[u32; 3]; {}],\n", item.name, item.array_count); break;
                    case Type::UInt4:   l("pub {}: [[u32; 4]; {}],\n", item.name, item.array_count); break;
                    case Type::Float:   l("pub {}: [f32; {}],\n", item.name, item.array_count); break;
                    case Type::Float2:  l("pub {}: [[f32; 2]; {}],\n", item.name, item.array_count); break;
                    case Type::Float3:  l("pub {}: [[f32; 3]; {}],\n", item.name, item.array_count); break;
                    case Type::Float4:  l("pub {}: [[f32; 4]; {}],\n", item.name, item.array_count); break;
                    case Type::Mat2x1:  l("pub {}: [[f32; 2]; {}],\n", item.name, item.array_count); break;
                    case Type::Mat2x2:  l("pub {}: [[f32; 4]; {}],\n", item.name, item.array_count); break;
                    case Type::Mat2x3:  l("pub {}: [[f32; 6]; {}],\n", item.name, item.array_count); break;
                    case Type::Mat2x4:  l("pub {}: [[f32; 8]; {}],\n", item.name, item.array_count); break;
                    case Type::Mat3x1:  l("pub {}: [[f32; 3]; {}],\n", item.name, item.array_count); break;
                    case Type::Mat3x2:  l("pub {}: [[f32; 6]; {}],\n", item.name, item.array_count); break;
                    case Type::Mat3x3:  l("pub {}: [[f32; 9]; {}],\n", item.name, item.array_count); break;
                    case Type::Mat3x4:  l("pub {}: [[f32; 12]; {}],\n", item.name, item.array_count); break;
                    case Type::Mat4x1:  l("pub {}: [[f32; 4]; {}],\n", item.name, item.array_count); break;
                    case Type::Mat4x2:  l("pub {}: [[f32; 8]; {}],\n", item.name, item.array_count); break;
                    case Type::Mat4x3:  l("pub {}: [[f32; 12]; {}],\n", item.name, item.array_count); break;
                    case Type::Mat4x4:  l("pub {}: [[f32; 16]; {}],\n", item.name, item.array_count); break;
                    default: l("INVALID_TYPE\n"); break;
                }
            }
        }
        cur_offset += item.size;
    }
    if (cur_offset < pad_to_size) {
        l("pub _pad_{}: [u8; {}],\n", cur_offset, pad_to_size - cur_offset);
    }
}

void SokolRustGenerator::recurse_unfold_structs(const GenInput& gen, const Type& struc, const std::string& name, int alignment, int pad_to_size) {
    // first recurse over nested structs because we need to extract those into
    // top-level Rust structs, extracted structs don't get an alignment since they
    // are nested
    for (const Type& item: struc.struct_items) {
        if (item.type == Type::Struct) {
            recurse_unfold_structs(gen, item, fmt::format("{}_{}", name, item.name), 0, item.size);
        }
    }
    l("#[repr(C, align({}))]\n", alignment);
    l_open("pub struct {} {{\n", struct_name(name));
    gen_struct_interior_decl_std430(gen, struc, name, pad_to_size);
    l_close("}}\n");
}

void SokolRustGenerator::gen_storage_buffer_decl(const GenInput& gen, const Type& struc) {
    // Rust doesn't allow nested struct declarations, so we need to use the same
    // awkward workaround as in Nim :/
    recurse_unfold_structs(gen, struc, struc.struct_typename, struc.align, struc.size);
}

void SokolRustGenerator::gen_shader_desc_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("pub fn {}_shader_desc(backend: sg::Backend) -> sg::ShaderDesc {{\n", prog.name);
    l("let mut desc = sg::ShaderDesc::new();\n");
    l("desc.label = c\"{}_shader\".as_ptr();\n", prog.name);
    l_open("match backend {{\n");
    for (int i = 0; i < Slang::Num; i++) {
        Slang::Enum slang = Slang::from_index(i);
        if (gen.args.slang & Slang::bit(slang)) {
            l_open("{} => {{\n", backend(slang));
            for (int stage_index = 0; stage_index < ShaderStage::Num; stage_index++) {
                const ShaderStageArrayInfo& info = shader_stage_array_info(gen, prog, ShaderStage::from_index(stage_index), slang);
                const StageReflection& refl = prog.stages[stage_index];
                const std::string dsn = fmt::format("desc.{}", info.stage == ShaderStage::Vertex ? "vertex_func" : "fragment_func");
                if (info.has_bytecode) {
                    l("{}.bytecode.ptr = &{} as *const _ as *const _;\n", dsn, info.bytecode_array_name);
                    l("{}.bytecode.size = {};\n", dsn, info.bytecode_array_size);
                } else {
                    l("{}.source = &{} as *const _ as *const _;\n", dsn, info.source_array_name);
                    const char* d3d11_tgt = nullptr;
                    if (slang == Slang::HLSL4) {
                        d3d11_tgt = (0 == stage_index) ? "vs_4_0" : "ps_4_0";
                    } else if (slang == Slang::HLSL5) {
                        d3d11_tgt = (0 == stage_index) ? "vs_5_0" : "ps_5_0";
                    }
                    if (d3d11_tgt) {
                        l("{}.d3d11_target = c\"{}\".as_ptr();\n", dsn, d3d11_tgt);
                    }
                }
                l("{}.entry = c\"{}\".as_ptr();\n", dsn, refl.entry_point_by_slang(slang));
            }
            for (int attr_index = 0; attr_index < StageAttr::Num; attr_index++) {
                const StageAttr& attr = prog.vs().inputs[attr_index];
                if (attr.slot >= 0) {
                    if (Slang::is_glsl(slang)) {
                        l("desc.attrs[{}].glsl_name = c\"{}\".as_ptr();\n", attr_index, attr.name);
                    } else if (Slang::is_hlsl(slang)) {
                        l("desc.attrs[{}].hlsl_sem_name = c\"{}\".as_ptr();\n", attr_index, attr.sem_name);
                        l("desc.attrs[{}].hlsl_sem_index = {};\n", attr_index, attr.sem_index);
                    }
                }
            }
            for (int ub_index = 0; ub_index < Bindings::MaxUniformBlocks; ub_index++) {
                const UniformBlock* ub = prog.bindings.find_uniform_block_by_sokol_slot(ub_index);
                if (ub) {
                    const std::string ubn = fmt::format("desc.uniform_blocks[{}]", ub_index);
                    l("{}.stage = {};\n", ubn, shader_stage(ub->stage));
                    l("{}.layout = sg::UniformLayout::Std140;\n", ubn);
                    l("{}.size = {};\n", ubn, roundup(ub->struct_info.size, 16));
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlsl_register_b_n = {};\n", ubn, ub->hlsl_register_b_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.msl_buffer_n = {};\n", ubn, ub->msl_buffer_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgsl_group0_binding_n = {};\n", ubn, ub->wgsl_group0_binding_n);
                    } else if (Slang::is_glsl(slang) && (ub->struct_info.struct_items.size() > 0)) {
                        if (ub->flattened) {
                            // NOT A BUG (to take the type from the first struct item, but the size from the toplevel ub)
                            l("{}.glsl_uniforms[0]._type = {};\n", ubn, flattened_uniform_type(ub->struct_info.struct_items[0].type));
                            l("{}.glsl_uniforms[0].array_count = {};\n", ubn, roundup(ub->struct_info.size, 16) / 16);
                            l("{}.glsl_uniforms[0].glsl_name = c\"{}\".as_ptr();\n", ubn, ub->name);
                        } else {
                            for (int u_index = 0; u_index < (int)ub->struct_info.struct_items.size(); u_index++) {
                                const Type& u = ub->struct_info.struct_items[u_index];
                                const std::string un = fmt::format("{}.glsl_uniforms[{}]", ubn, u_index);
                                l("{}._type = {};\n", un, uniform_type(u.type));
                                l("{}.array_count = {};\n", un, u.array_count);
                                l("{}.glsl_name = c\"{}.{}\".as_ptr();\n", un, ub->inst_name, u.name);
                            }
                        }
                    }
                }
            }
            for (int sbuf_index = 0; sbuf_index < Bindings::MaxStorageBuffers; sbuf_index++) {
                const StorageBuffer* sbuf = prog.bindings.find_storage_buffer_by_sokol_slot(sbuf_index);
                if (sbuf) {
                    const std::string& sbn = fmt::format("desc.storage_buffers[{}]", sbuf_index);
                    l("{}.stage = {};\n", sbn, shader_stage(sbuf->stage));
                    l("{}.readonly = {};\n", sbn, sbuf->readonly);
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlsl_register_t_n = {};\n", sbn, sbuf->hlsl_register_t_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.msl_buffer_n = {};\n", sbn, sbuf->msl_buffer_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgsl_group1_binding_n = {};\n", sbn, sbuf->wgsl_group1_binding_n);
                    } else if (Slang::is_glsl(slang)) {
                        l("{}.glsl_binding_n = {};\n", sbn, sbuf->glsl_binding_n);
                    }
                }
            }
            for (int img_index = 0; img_index < Bindings::MaxImages; img_index++) {
                const Image* img = prog.bindings.find_image_by_sokol_slot(img_index);
                if (img) {
                    const std::string in = fmt::format("desc.images[{}]", img_index);
                    l("{}.stage = {};\n", in, shader_stage(img->stage));
                    l("{}.multisampled = {};\n", in, img->multisampled ? "true" : "false");
                    l("{}.image_type = {};\n", in, image_type(img->type));
                    l("{}.sample_type = {};\n", in, image_sample_type(img->sample_type));
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlsl_register_t_n = {};\n", in, img->hlsl_register_t_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.msl_texture_n = {};\n", in, img->msl_texture_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgsl_group1_binding_n = {};\n", in, img->wgsl_group1_binding_n);
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
            for (int img_smp_index = 0; img_smp_index < Bindings::MaxImageSamplers; img_smp_index++) {
                const ImageSampler* img_smp = prog.bindings.find_image_sampler_by_sokol_slot(img_smp_index);
                if (img_smp) {
                    const std::string isn = fmt::format("desc.image_sampler_pairs[{}]", img_smp_index);
                    l("{}.stage = {};\n", isn, shader_stage(img_smp->stage));
                    l("{}.image_slot = {};\n", isn, prog.bindings.find_image_by_name(img_smp->image_name)->sokol_slot);
                    l("{}.sampler_slot = {};\n", isn, prog.bindings.find_sampler_by_name(img_smp->sampler_name)->sokol_slot);
                    if (Slang::is_glsl(slang)) {
                        l("{}.glsl_name = c\"{}\".as_ptr();\n", isn, img_smp->name);
                    }
                }
            }
            l_close("}},\n"); // current switch branch
        }
    }
    l("_ => {{}},\n");
    l_close("}}\n"); // close switch statement
    l("desc\n");
    l_close("}}\n"); // close function
}

void SokolRustGenerator::gen_shader_array_start(const GenInput& gen, const std::string& array_name, size_t num_bytes, Slang::Enum slang) {
    l("pub const {}: [u8; {}] = [\n", array_name, num_bytes);
}

void SokolRustGenerator::gen_shader_array_end(const GenInput& gen) {
    l("\n];\n");
}

std::string SokolRustGenerator::lang_name() {
    return "Rust";
}

std::string SokolRustGenerator::comment_block_start() {
    return "/*";
}

std::string SokolRustGenerator::comment_block_end() {
    return "*/";
}

std::string SokolRustGenerator::comment_block_line_prefix() {
    return "";
}

std::string SokolRustGenerator::shader_bytecode_array_name(const std::string& snippet_name, Slang::Enum slang) {
    return pystring::upper(fmt::format("{}_bytecode_{}", snippet_name, Slang::to_str(slang)));
}

std::string SokolRustGenerator::shader_source_array_name(const std::string& snippet_name, Slang::Enum slang) {
    return pystring::upper(fmt::format("{}_source_{}", snippet_name, Slang::to_str(slang)));
}

std::string SokolRustGenerator::get_shader_desc_help(const std::string& prog_name) {
    return fmt::format("{}_shader_desc(sg::query_backend());\n", prog_name);
}

std::string SokolRustGenerator::shader_stage(ShaderStage::Enum e) {
    switch (e) {
        case ShaderStage::Vertex: return "sg::ShaderStage::Vertex";
        case ShaderStage::Fragment: return "sg::ShaderStage::Fragment";
        default: return "INVALID";
    }
}

std::string SokolRustGenerator::uniform_type(Type::Enum e) {
    switch (e) {
        case Type::Float:  return "sg::UniformType::Float";
        case Type::Float2: return "sg::UniformType::Float2";
        case Type::Float3: return "sg::UniformType::Float3";
        case Type::Float4: return "sg::UniformType::Float4";
        case Type::Int:    return "sg::UniformType::Int";
        case Type::Int2:   return "sg::UniformType::Int2";
        case Type::Int3:   return "sg::UniformType::Int3";
        case Type::Int4:   return "sg::UniformType::Int4";
        case Type::Mat4x4: return "sg::UniformType::Mat4";
        default: return "INVALID";
    }
}

std::string SokolRustGenerator::flattened_uniform_type(Type::Enum e) {
    switch (e) {
        case Type::Float:
        case Type::Float2:
        case Type::Float3:
        case Type::Float4:
        case Type::Mat4x4:
             return "sg::UniformType::Float4";
        case Type::Int:
        case Type::Int2:
        case Type::Int3:
        case Type::Int4:
            return "sg::UniformType::Int4";
        default:
            return "INVALID";
    }
}

std::string SokolRustGenerator::image_type(ImageType::Enum e) {
    switch (e) {
        case ImageType::_2D:     return "sg::ImageType::Dim2";
        case ImageType::CUBE:    return "sg::ImageType::Cube";
        case ImageType::_3D:     return "sg::ImageType::Dim3";
        case ImageType::ARRAY:   return "sg::ImageType::Array";
        default: return "INVALID";
    }
}

std::string SokolRustGenerator::image_sample_type(ImageSampleType::Enum e) {
    switch (e) {
        case ImageSampleType::FLOAT:               return "sg::ImageSampleType::Float";
        case ImageSampleType::DEPTH:               return "sg::ImageSampleType::Depth";
        case ImageSampleType::SINT:                return "sg::ImageSampleType::Sint";
        case ImageSampleType::UINT:                return "sg::ImageSampleType::Uint";
        case ImageSampleType::UNFILTERABLE_FLOAT:  return "sg::ImageSampleType::UnfilterableFloat";
        default: return "INVALID";
    }
}

std::string SokolRustGenerator::sampler_type(SamplerType::Enum e) {
    switch (e) {
        case SamplerType::FILTERING:     return "sg::SamplerType::Filtering";
        case SamplerType::COMPARISON:    return "sg::SamplerType::Comparison";
        case SamplerType::NONFILTERING:  return "sg::SamplerType::Nonfiltering";
        default: return "INVALID";
    }
}

std::string SokolRustGenerator::backend(Slang::Enum e) {
    switch (e) {
        case Slang::GLSL410:
        case Slang::GLSL430:
            return "sg::Backend::Glcore";
        case Slang::GLSL300ES:
        case Slang::GLSL310ES:
            return "sg::Backend::Gles3";
        case Slang::HLSL4:
        case Slang::HLSL5:
            return "sg::Backend::D3d11";
        case Slang::METAL_MACOS:
            return "sg::Backend::MetalMacos";
        case Slang::METAL_IOS:
            return "sg::Backend::MetalIos";
        case Slang::METAL_SIM:
            return "sg::Backend::MetalSimulator";
        case Slang::WGSL:
            return "sg::Backend::Wgpu";
        default:
            return "INVALID";
    }
}

std::string SokolRustGenerator::struct_name(const std::string& name) {
    return to_pascal_case(name);
}

std::string SokolRustGenerator::vertex_attr_name(const std::string& prog_name, const StageAttr& attr) {
    return pystring::upper(fmt::format("ATTR_{}_{}", prog_name, attr.name));
}

std::string SokolRustGenerator::image_bind_slot_name(const Image& img) {
    return pystring::upper(fmt::format("IMG_{}", img.name));
}

std::string SokolRustGenerator::sampler_bind_slot_name(const Sampler& smp) {
    return pystring::upper(fmt::format("SMP_{}", smp.name));
}

std::string SokolRustGenerator::uniform_block_bind_slot_name(const UniformBlock& ub) {
    return pystring::upper(fmt::format("UB_{}", ub.name));
}

std::string SokolRustGenerator::storage_buffer_bind_slot_name(const StorageBuffer& sbuf) {
    return pystring::upper(fmt::format("SBUF_{}", sbuf.name));
}

std::string SokolRustGenerator::vertex_attr_definition(const std::string& prog_name, const StageAttr& attr) {
    return fmt::format("pub const {}: usize = {};", vertex_attr_name(prog_name, attr), attr.slot);
}

std::string SokolRustGenerator::image_bind_slot_definition(const Image& img) {
    return fmt::format("pub const {}: usize = {};", image_bind_slot_name(img), img.sokol_slot);
}

std::string SokolRustGenerator::sampler_bind_slot_definition(const Sampler& smp) {
    return fmt::format("pub const {}: usize = {};", sampler_bind_slot_name(smp), smp.sokol_slot);
}

std::string SokolRustGenerator::uniform_block_bind_slot_definition(const UniformBlock& ub) {
    return fmt::format("pub const {}: usize = {};", uniform_block_bind_slot_name(ub), ub.sokol_slot);
}

std::string SokolRustGenerator::storage_buffer_bind_slot_definition(const StorageBuffer& sbuf) {
    return fmt::format("pub const {}: usize = {};", storage_buffer_bind_slot_name(sbuf), sbuf.sokol_slot);
}

} // namespace
