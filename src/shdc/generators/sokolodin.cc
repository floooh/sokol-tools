/*
    Generate sokol-odin module.
*/
#include "sokolodin.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc::gen {

using namespace refl;

ErrMsg SokolOdinGenerator::begin(const GenInput& gen) {
    if (!gen.inp.module.empty()) {
        mod_prefix = fmt::format("{}_", gen.inp.module);
    }
    return Generator::begin(gen);
}

void SokolOdinGenerator::gen_prolog(const GenInput& gen) {
    for (const auto& header: gen.inp.headers) {
        l("{}\n", header);
    }
}

void SokolOdinGenerator::gen_epilog(const GenInput& gen) {
    // empty
}

void SokolOdinGenerator::gen_prerequisites(const GenInput& gen) {
    // empty
}

void SokolOdinGenerator::gen_uniform_block_decl(const GenInput& gen, const UniformBlock& ub) {
    l_open("{} :: struct #align({}) {{\n", struct_name(ub.name), ub.struct_info.align);
    l_open("using _: struct #packed {{\n");
    int cur_offset = 0;
    for (const Type& uniform: ub.struct_info.struct_items) {
        int next_offset = uniform.offset;
        if (next_offset > cur_offset) {
            l("_: [{}]u8,\n", next_offset - cur_offset);
            cur_offset = next_offset;
        }
        if (gen.inp.ctype_map.count(uniform.type_as_glsl()) > 0) {
            // user-provided type names
            if (uniform.array_count == 0) {
                l("{}: {},\n", uniform.name, gen.inp.ctype_map.at(uniform.type_as_glsl()));
            } else {
                l("{}: [{}]{},\n", uniform.name, uniform.array_count, gen.inp.ctype_map.at(uniform.type_as_glsl()));
            }
        } else {
            // default type names (float)
            if (uniform.array_count == 0) {
                switch (uniform.type) {
                    case Type::Float:   l("{}: f32,\n", uniform.name); break;
                    case Type::Float2:  l("{}: [2]f32,\n", uniform.name); break;
                    case Type::Float3:  l("{}: [3]f32,\n", uniform.name); break;
                    case Type::Float4:  l("{}: [4]f32,\n", uniform.name); break;
                    case Type::Int:     l("{}: i32,\n", uniform.name); break;
                    case Type::Int2:    l("{}: [2]i32,\n", uniform.name); break;
                    case Type::Int3:    l("{}: [3]i32,\n", uniform.name); break;
                    case Type::Int4:    l("{}: [4]i32,\n", uniform.name); break;
                    case Type::Mat4x4:  l("{}: [16]f32,\n", uniform.name); break;
                    default:                  l("INVALID_UNIFORM_TYPE\n"); break;
                }
            } else {
                switch (uniform.type) {
                    case Type::Float4:  l("{}: [{}][4]f32,\n", uniform.name, uniform.array_count); break;
                    case Type::Int4:    l("{}: [{}][4]i32,\n", uniform.name, uniform.array_count); break;
                    case Type::Mat4x4:  l("{}: [{}][16]f32,\n", uniform.name, uniform.array_count); break;
                    default:            l("INVALID_UNIFORM_TYPE\n"); break;
                }
            }
        }
        cur_offset += uniform.size;
    }
    // pad to multiple of 16-bytes struct size
    const int round16 = roundup(cur_offset, 16);
    if (cur_offset < round16) {
        l("_: [{}]u8,\n", round16 - cur_offset);
    }
    l_close("}},\n");
    l_close("}}\n");
}

void SokolOdinGenerator::gen_struct_interior_decl_std430(const GenInput& gen, const Type& struc, int pad_to_size) {
    assert(struc.type == Type::Struct);
    assert(pad_to_size > 0);

    int cur_offset = 0;
    for (const Type& item: struc.struct_items) {
        int next_offset = item.offset;
        if (next_offset > cur_offset) {
            l("_: [{}]u8,\n", next_offset - cur_offset);
            cur_offset = next_offset;
        }
        if (item.type == Type::Struct) {
            // recurse into nested struct
            if (item.array_count == 0) {
                l_open("{}: struct {{\n", item.name);
            } else {
                l_open("{}: [{}]struct {{\n", item.name, item.array_count);
            }
            gen_struct_interior_decl_std430(gen, item, item.size);
            l_close("}},\n");
        } else if (gen.inp.ctype_map.count(item.type_as_glsl()) > 0) {
            // user-provided type names
            if (item.array_count == 0) {
                l("{}: {},\n", item.name, gen.inp.ctype_map.at(item.type_as_glsl()));
            } else {
                l("{}: [{}]{},\n", item.name, item.array_count, gen.inp.ctype_map.at(item.type_as_glsl()));
            }
        } else {
            // default typenames
            if (item.array_count == 0) {
                switch (item.type) {
                    // NOTE: bool => int is not a bug!
                    case Type::Bool:    l("{}: i32,\n", item.name); break;
                    case Type::Bool2:   l("{}: [2]i32,\n", item.name); break;
                    case Type::Bool3:   l("{}: [3]i32,\n", item.name); break;
                    case Type::Bool4:   l("{}: [4]i32,\n", item.name); break;
                    case Type::Int:     l("{}: i32,\n", item.name); break;
                    case Type::Int2:    l("{}: [2]i32,\n", item.name); break;
                    case Type::Int3:    l("{}: [3]i32,\n", item.name); break;
                    case Type::Int4:    l("{}: [4]i32,\n", item.name); break;
                    case Type::UInt:    l("{}: u32,\n", item.name); break;
                    case Type::UInt2:   l("{}: [2]u32,\n", item.name); break;
                    case Type::UInt3:   l("{}: [3]u32,\n", item.name); break;
                    case Type::UInt4:   l("{}: [4]u32,\n", item.name); break;
                    case Type::Float:   l("{}: f32,\n", item.name); break;
                    case Type::Float2:  l("{}: [2]f32,\n", item.name); break;
                    case Type::Float3:  l("{}: [3]f32,\n", item.name); break;
                    case Type::Float4:  l("{}: [4]f32,\n", item.name); break;
                    case Type::Mat2x1:  l("{}: [2]f32,\n", item.name); break;
                    case Type::Mat2x2:  l("{}: [4]f32,\n", item.name); break;
                    case Type::Mat2x3:  l("{}: [6]f32,\n", item.name); break;
                    case Type::Mat2x4:  l("{}: [8]f32,\n", item.name); break;
                    case Type::Mat3x1:  l("{}: [3]f32,\n", item.name); break;
                    case Type::Mat3x2:  l("{}: [6]f32,\n", item.name); break;
                    case Type::Mat3x3:  l("{}: [9]f32,\n", item.name); break;
                    case Type::Mat3x4:  l("{}: [12]f32,\n", item.name); break;
                    case Type::Mat4x1:  l("{}: [4]f32,\n", item.name); break;
                    case Type::Mat4x2:  l("{}: [8]f32,\n", item.name); break;
                    case Type::Mat4x3:  l("{}: [12]f32,\n", item.name); break;
                    case Type::Mat4x4:  l("{}: [16]f32,\n", item.name); break;
                    default: l("INVALID_TYPE\n"); break;
                }
            } else {
                switch (item.type) {
                    // NOTE: bool => int is not a bug!
                    case Type::Bool:    l("{}: [{}]i32,\n", item.name, item.array_count); break;
                    case Type::Bool2:   l("{}: [{}][2]i32,\n", item.name, item.array_count); break;
                    case Type::Bool3:   l("{}: [{}][3]i32,\n", item.name, item.array_count); break;
                    case Type::Bool4:   l("{}: [{}][4]i32,\n", item.name, item.array_count); break;
                    case Type::Int:     l("{}: [{}]i32,\n", item.name, item.array_count); break;
                    case Type::Int2:    l("{}: [{}][2]i32,\n", item.name, item.array_count); break;
                    case Type::Int3:    l("{}: [{}][3]i32,\n", item.name, item.array_count); break;
                    case Type::Int4:    l("{}: [{}][4]i32,\n", item.name, item.array_count); break;
                    case Type::UInt:    l("{}: [{}]u32,\n", item.name, item.array_count); break;
                    case Type::UInt2:   l("{}: [{}][2]u32,\n", item.name, item.array_count); break;
                    case Type::UInt3:   l("{}: [{}][3]u32,\n", item.name, item.array_count); break;
                    case Type::UInt4:   l("{}: [{}][4]u32,\n", item.name, item.array_count); break;
                    case Type::Float:   l("{}: [{}]f32,\n", item.name, item.array_count); break;
                    case Type::Float2:  l("{}: [{}][2]f32,\n", item.name, item.array_count); break;
                    case Type::Float3:  l("{}: [{}][3]f32,\n", item.name, item.array_count); break;
                    case Type::Float4:  l("{}: [{}][4]f32,\n", item.name, item.array_count); break;
                    case Type::Mat2x1:  l("{}: [{}][2]f32,\n", item.name, item.array_count); break;
                    case Type::Mat2x2:  l("{}: [{}][4]f32,\n", item.name, item.array_count); break;
                    case Type::Mat2x3:  l("{}: [{}][6]f32,\n", item.name, item.array_count); break;
                    case Type::Mat2x4:  l("{}: [{}][8]f32,\n", item.name, item.array_count); break;
                    case Type::Mat3x1:  l("{}: [{}][3]f32,\n", item.name, item.array_count); break;
                    case Type::Mat3x2:  l("{}: [{}][6]f32,\n", item.name, item.array_count); break;
                    case Type::Mat3x3:  l("{}: [{}][9]f32,\n", item.name, item.array_count); break;
                    case Type::Mat3x4:  l("{}: [{}][12]f32,\n", item.name, item.array_count); break;
                    case Type::Mat4x1:  l("{}: [{}][4]f32,\n", item.name, item.array_count); break;
                    case Type::Mat4x2:  l("{}: [{}][8]f32,\n", item.name, item.array_count); break;
                    case Type::Mat4x3:  l("{}: [{}][12]f32,\n", item.name, item.array_count); break;
                    case Type::Mat4x4:  l("{}: [{}][16]f32,\n", item.name, item.array_count); break;
                    default: l("INVALID_TYPE\n"); break;
                }
            }
        }
        cur_offset += item.size;
    }
    if (cur_offset < pad_to_size) {
        l("_: [{}]u8,\n", pad_to_size - cur_offset);
    }
}

void SokolOdinGenerator::gen_storage_buffer_decl(const GenInput& gen, const StorageBuffer& sbuf) {
    const auto& item = sbuf.struct_info.struct_items[0];
    l_open("{} :: struct #align({}) {{\n", struct_name(item.struct_typename), sbuf.struct_info.align);
    l_open("using _: struct #packed {{\n");
    gen_struct_interior_decl_std430(gen, item, sbuf.struct_info.size);
    l_close("}},\n");
    l_close("}}\n");
}

void SokolOdinGenerator::gen_shader_desc_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}_shader_desc :: proc (backend: sg.Backend) -> sg.Shader_Desc {{\n", prog.name);
    l("desc: sg.Shader_Desc\n");
    l("desc.label = \"{}_shader\"\n", prog.name);
    l("#partial switch backend {{\n");
    for (int i = 0; i < Slang::Num; i++) {
        Slang::Enum slang = Slang::from_index(i);
        if (gen.args.slang & Slang::bit(slang)) {
            l_open("case {}:\n", backend(slang));
            for (int stage_index = 0; stage_index < ShaderStage::Num; stage_index++) {
                const ShaderStageArrayInfo& info = shader_stage_array_info(gen, prog, ShaderStage::from_index(stage_index), slang);
                const StageReflection& refl = prog.stages[stage_index];
                const std::string dsn = fmt::format("desc.{}", info.stage == ShaderStage::Vertex ? "vertex_func" : "fragment_func");
                if (info.has_bytecode) {
                    l("{}.bytecode.ptr = &{}\n", dsn, info.bytecode_array_name);
                    l("{}.bytecode.size = {}\n", dsn, info.bytecode_array_size);
                } else {
                    l("{}.source = transmute(cstring)&{}\n", dsn, info.source_array_name);
                    const char* d3d11_tgt = nullptr;
                    if (slang == Slang::HLSL4) {
                        d3d11_tgt = (0 == stage_index) ? "vs_4_0" : "ps_4_0";
                    } else if (slang == Slang::HLSL5) {
                        d3d11_tgt = (0 == stage_index) ? "vs_5_0" : "ps_5_0";
                    }
                    if (d3d11_tgt) {
                        l("{}.d3d11_target = \"{}\"\n", dsn, d3d11_tgt);
                    }
                }
                l("{}.entry = \"{}\"\n", dsn, refl.entry_point_by_slang(slang));
            }
            for (int attr_index = 0; attr_index < StageAttr::Num; attr_index++) {
                const StageAttr& attr = prog.vs().inputs[attr_index];
                if (attr.slot >= 0) {
                    if (Slang::is_glsl(slang)) {
                        l("desc.attrs[{}].glsl_name = \"{}\"\n", attr_index, attr.name);
                    } else if (Slang::is_hlsl(slang)) {
                        l("desc.attrs[{}].hlsl_sem_name = \"{}\"\n", attr_index, attr.sem_name);
                        l("desc.attrs[{}].hlsl_sem_index = {}\n", attr_index, attr.sem_index);
                    }
                }
            }
            for (int ub_index = 0; ub_index < Bindings::MaxUniformBlocks; ub_index++) {
                const UniformBlock* ub = prog.bindings.find_uniform_block_by_sokol_slot(ub_index);
                if (ub) {
                    const std::string ubn = fmt::format("desc.uniform_blocks[{}]", ub_index);
                    l("{}.stage = {}\n", ubn, shader_stage(ub->stage));
                    l("{}.layout = .STD140\n", ubn);
                    l("{}.size = {}\n", ubn, roundup(ub->struct_info.size, 16));
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlsl_register_b_n = {}\n", ubn, ub->hlsl_register_b_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.msl_buffer_n = {}\n", ubn, ub->msl_buffer_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgsl_group0_binding_n = {}\n", ubn, ub->wgsl_group0_binding_n);
                    } else if (Slang::is_glsl(slang) && (ub->struct_info.struct_items.size() > 0)) {
                        if (ub->flattened) {
                            // NOT A BUG (to take the type from the first struct item, but the size from the toplevel ub)
                            l("{}.glsl_uniforms[0].type = {}\n", ubn, flattened_uniform_type(ub->struct_info.struct_items[0].type));
                            l("{}.glsl_uniforms[0].array_count = {}\n", ubn, roundup(ub->struct_info.size, 16) / 16);
                            l("{}.glsl_uniforms[0].glsl_name = \"{}\"\n", ubn, ub->name);
                        } else {
                            for (int u_index = 0; u_index < (int)ub->struct_info.struct_items.size(); u_index++) {
                                const Type& u = ub->struct_info.struct_items[u_index];
                                const std::string un = fmt::format("{}.glsl_uniforms[{}]", ubn, u_index);
                                l("{}.type = {}\n", un, uniform_type(u.type));
                                l("{}.array_count = {}\n", un, u.array_count);
                                l("{}.glsl_name = \"{}.{}\"\n", un, ub->inst_name, u.name);
                            }
                        }
                    }
                }
            }
            for (int sbuf_index = 0; sbuf_index < Bindings::MaxStorageBuffers; sbuf_index++) {
                const StorageBuffer* sbuf = prog.bindings.find_storage_buffer_by_sokol_slot(sbuf_index);
                if (sbuf) {
                    const std::string& sbn = fmt::format("desc.storage_buffers[{}]", sbuf_index);
                    l("{}.stage = {}\n", sbn, shader_stage(sbuf->stage));
                    l("{}.readonly = {}\n", sbn, sbuf->readonly);
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlsl_register_t_n = {}\n", sbn, sbuf->hlsl_register_t_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.msl_buffer_n = {}\n", sbn, sbuf->msl_buffer_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgsl_group1_binding_n = {}\n", sbn, sbuf->wgsl_group1_binding_n);
                    } else if (Slang::is_glsl(slang)) {
                        l("{}.glsl_binding_n = {}\n", sbn, sbuf->glsl_binding_n);
                    }
                }
            }
            for (int img_index = 0; img_index < Bindings::MaxImages; img_index++) {
                const Image* img = prog.bindings.find_image_by_sokol_slot(img_index);
                if (img) {
                    const std::string in = fmt::format("desc.images[{}]", img_index);
                    l("{}.stage = {}\n", in, shader_stage(img->stage));
                    l("{}.multisampled = {}\n", in, img->multisampled ? "true" : "false");
                    l("{}.image_type = {}\n", in, image_type(img->type));
                    l("{}.sample_type = {}\n", in, image_sample_type(img->sample_type));
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlsl_register_t_n = {}\n", in, img->hlsl_register_t_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.msl_texture_n = {}\n", in, img->msl_texture_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgsl_group1_binding_n = {}\n", in, img->wgsl_group1_binding_n);
                    }
                }
            }
            for (int smp_index = 0; smp_index < Bindings::MaxSamplers; smp_index++) {
                const Sampler* smp = prog.bindings.find_sampler_by_sokol_slot(smp_index);
                if (smp) {
                    const std::string sn = fmt::format("desc.samplers[{}]", smp_index);
                    l("{}.stage = {}\n", sn, shader_stage(smp->stage));
                    l("{}.sampler_type = {}\n", sn, sampler_type(smp->type));
                    if (Slang::is_hlsl(slang)) {
                        l("{}.hlsl_register_s_n = {}\n", sn, smp->hlsl_register_s_n);
                    } else if (Slang::is_msl(slang)) {
                        l("{}.msl_sampler_n = {}\n", sn, smp->msl_sampler_n);
                    } else if (Slang::is_wgsl(slang)) {
                        l("{}.wgsl_group1_binding_n = {}\n", sn, smp->wgsl_group1_binding_n);
                    }
                }
            }
            for (int img_smp_index = 0; img_smp_index < Bindings::MaxImageSamplers; img_smp_index++) {
                const ImageSampler* img_smp = prog.bindings.find_image_sampler_by_sokol_slot(img_smp_index);
                if (img_smp) {
                    const std::string isn = fmt::format("desc.image_sampler_pairs[{}]", img_smp_index);
                    l("{}.stage = {}\n", isn, shader_stage(img_smp->stage));
                    l("{}.image_slot = {}\n", isn, prog.bindings.find_image_by_name(img_smp->image_name)->sokol_slot);
                    l("{}.sampler_slot = {}\n", isn, prog.bindings.find_sampler_by_name(img_smp->sampler_name)->sokol_slot);
                    if (Slang::is_glsl(slang)) {
                        l("{}.glsl_name = \"{}\"\n", isn, img_smp->name);
                    }
                }
            }
            l_close(); // current switch branch
        }
    }
    l("}}\n"); // close switch statement
    l("return desc\n");
    l_close("}}\n"); // close function
}

void SokolOdinGenerator::gen_attr_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}{}_attr_slot :: proc (attr_name: string) -> int {{\n", mod_prefix, prog.name);
    for (const StageAttr& attr: prog.vs().inputs) {
        if (attr.slot >= 0) {
            l_open("if attr_name == \"{}\" {{\n", attr.name);
            l("return {}\n", attr.slot);
            l_close("}}\n");
        }
    }
    l("return -1\n");
    l_close("}}\n");
}

void SokolOdinGenerator::gen_image_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}{}_image_slot :: proc (img_name: string) -> int {{\n", mod_prefix, prog.name);
    for (const Image& img: prog.bindings.images) {
        if (img.sokol_slot >= 0) {
            l_open("if img_name == \"{}\" {{\n", img.name);
            l("return {}\n", img.sokol_slot);
            l_close("}}\n");
        }
    }
    l("return -1\n");
    l_close("}}\n");
}

void SokolOdinGenerator::gen_sampler_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}{}_sampler_slot :: proc (smp_name: string) -> int {{\n", mod_prefix, prog.name);
    for (const Sampler& smp: prog.bindings.samplers) {
        if (smp.sokol_slot >= 0) {
            l_open("if smp_name == \"{}\" {{\n", smp.name);
            l("return {}\n", smp.sokol_slot);
            l_close("}}\n");
        }
    }
    l("return -1\n");
    l_close("}}\n");
}

void SokolOdinGenerator::gen_uniform_block_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}{}_uniformblock_slot :: proc (ub_name: string) -> int {{\n", mod_prefix, prog.name);
    for (const UniformBlock& ub: prog.bindings.uniform_blocks) {
        if (ub.sokol_slot >= 0) {
            l_open("if ub_name == \"{}\" {{\n", ub.name);
            l("return {}\n", ub.sokol_slot);
            l_close("}}\n");
        }
    }
    l("return -1\n");
    l_close("}}\n");
}

void SokolOdinGenerator::gen_uniform_block_size_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}{}_uniformblock_size :: proc (ub_name: string) -> int {{\n", mod_prefix, prog.name);
    for (const UniformBlock& ub: prog.bindings.uniform_blocks) {
        if (ub.sokol_slot >= 0) {
            l_open("if ub_name == \"{}\" {{\n", ub.name);
            l("return size_of({})\n", struct_name(ub.name));
            l_close("}}\n");
        }
    }
    l("return 0\n");
    l_close("}}\n");
}

void SokolOdinGenerator::gen_storage_buffer_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}{}_storagebuffer_slot :: proc (sbuf_name: string) -> int {{\n", mod_prefix, prog.name);
    for (const StorageBuffer& sbuf: prog.bindings.storage_buffers) {
        if (sbuf.sokol_slot >= 0) {
            l_open("if sbuf_name == \"{}\" {{\n", sbuf.name);
            l("return {}\n", sbuf.sokol_slot);
            l_close("}}\n");
        }
    }
    l("return -1\n");
    l_close("}}\n");
}

void SokolOdinGenerator::gen_uniform_offset_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}{}_uniform_offset :: proc (ub_name, u_name: string) -> int {{\n", mod_prefix, prog.name);
    for (const UniformBlock& ub: prog.bindings.uniform_blocks) {
        if (ub.sokol_slot >= 0) {
            l_open("if ub_name == \"{}\" {{\n", ub.name);
            for (const Type& u: ub.struct_info.struct_items) {
                l_open("if u_name == \"{}\" {{\n", u.name);
                l("return {}\n", u.offset);
                l_close("}}\n");
            }
            l_close("}}\n");
        }
    }
    l("return -1\n");
    l_close("}}\n");
}

void SokolOdinGenerator::gen_uniform_desc_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}{}_uniform_desc :: proc (ub_name, u_name: string) -> sg.Glsl_Shader_Uniform {{\n", mod_prefix, prog.name);
    l("res := sg.Glsl_Shader_Uniform {{}};\n");
    for (const UniformBlock& ub: prog.bindings.uniform_blocks) {
        if (ub.sokol_slot >= 0) {
            l_open("if ub_name == \"{}\" {{\n", ub.name);
            for (const Type& u: ub.struct_info.struct_items) {
                l_open("if u_name == \"{}\" {{\n", u.name);
                l("res.type = {};\n", uniform_type(u.type));
                l("res.array_count = {};\n", u.array_count);
                l("res.glsl_name = \"{}\";\n", u.name);
                l("return res\n");
                l_close("}}\n");
            }
            l_close("}}\n");
        }
    }
    l("return res\n");
    l_close("}}\n");
}

void SokolOdinGenerator::gen_shader_array_start(const GenInput& gen, const std::string& array_name, size_t num_bytes, Slang::Enum slang) {
    l("@(private=\"file\")\n{} := [{}]u8 {{\n", array_name, num_bytes);
}

void SokolOdinGenerator::gen_shader_array_end(const GenInput& gen) {
    l("\n}}\n");
}

std::string SokolOdinGenerator::lang_name() {
    return "Odin";
}

std::string SokolOdinGenerator::comment_block_start() {
    return "/*";
}

std::string SokolOdinGenerator::comment_block_end() {
    return "*/";
}

std::string SokolOdinGenerator::comment_block_line_prefix() {
    return "";
}

std::string SokolOdinGenerator::shader_bytecode_array_name(const std::string& snippet_name, Slang::Enum slang) {
    return fmt::format("{}_bytecode_{}", snippet_name, Slang::to_str(slang));
}

std::string SokolOdinGenerator::shader_source_array_name(const std::string& snippet_name, Slang::Enum slang) {
    return fmt::format("{}_source_{}", snippet_name, Slang::to_str(slang));
}

std::string SokolOdinGenerator::get_shader_desc_help(const std::string& prog_name) {
    return fmt::format("{}_shader_desc(sg.query_backend())\n", prog_name);
}

std::string SokolOdinGenerator::shader_stage(const ShaderStage::Enum e) {
    switch (e) {
        case ShaderStage::Vertex: return ".VERTEX";
        case ShaderStage::Fragment: return ".FRAGMENT";
        default: return "INVALID";
    }
}

std::string SokolOdinGenerator::uniform_type(Type::Enum e) {
    switch (e) {
        case Type::Float:  return ".FLOAT";
        case Type::Float2: return ".FLOAT2";
        case Type::Float3: return ".FLOAT3";
        case Type::Float4: return ".FLOAT4";
        case Type::Int:    return ".INT";
        case Type::Int2:   return ".INT2";
        case Type::Int3:   return ".INT3";
        case Type::Int4:   return ".INT4";
        case Type::Mat4x4: return ".MAT4";
        default: return "INVALID";
    }
}

std::string SokolOdinGenerator::flattened_uniform_type(Type::Enum e) {
    switch (e) {
        case Type::Float:
        case Type::Float2:
        case Type::Float3:
        case Type::Float4:
        case Type::Mat4x4:
             return ".FLOAT4";
        case Type::Int:
        case Type::Int2:
        case Type::Int3:
        case Type::Int4:
            return ".INT4";
        default:
            return "INVALID";
    }
}

std::string SokolOdinGenerator::image_type(ImageType::Enum e) {
    switch (e) {
        case ImageType::_2D:     return "._2D";
        case ImageType::CUBE:    return ".CUBE";
        case ImageType::_3D:     return "._3D";
        case ImageType::ARRAY:   return ".ARRAY";
        default: return "INVALID";
    }
}

std::string SokolOdinGenerator::image_sample_type(ImageSampleType::Enum e) {
    switch (e) {
        case ImageSampleType::FLOAT: return ".FLOAT";
        case ImageSampleType::DEPTH: return ".DEPTH";
        case ImageSampleType::SINT:  return ".SINT";
        case ImageSampleType::UINT:  return ".UINT";
        case ImageSampleType::UNFILTERABLE_FLOAT:  return ".UNFILTERABLE_FLOAT";
        default: return "INVALID";
    }
}

std::string SokolOdinGenerator::sampler_type(SamplerType::Enum e) {
    switch (e) {
        case SamplerType::FILTERING:     return ".FILTERING";
        case SamplerType::COMPARISON:    return ".COMPARISON";
        case SamplerType::NONFILTERING:  return ".NONFILTERING";
        default: return "INVALID";
    }
}

std::string SokolOdinGenerator::backend(Slang::Enum e) {
    switch (e) {
        case Slang::GLSL410:
        case Slang::GLSL430:
            return ".GLCORE";
        case Slang::GLSL300ES:
            return ".GLES3";
        case Slang::HLSL4:
        case Slang::HLSL5:
            return ".D3D11";
        case Slang::METAL_MACOS:
            return ".METAL_MACOS";
        case Slang::METAL_IOS:
            return ".METAL_IOS";
        case Slang::METAL_SIM:
            return ".METAL_SIMULATOR";
        case Slang::WGSL:
            return ".WGPU";
        default:
            return "INVALID";
    }
}

std::string SokolOdinGenerator::struct_name(const std::string& name) {
    return to_ada_case(fmt::format("{}{}", mod_prefix, name));
}

std::string SokolOdinGenerator::vertex_attr_name(const std::string& prog_name, const StageAttr& attr) {
    return fmt::format("ATTR_{}{}_{}", mod_prefix, prog_name, attr.name);
}

std::string SokolOdinGenerator::image_bind_slot_name(const Image& img) {
    return fmt::format("IMG_{}{}", mod_prefix, img.name);
}

std::string SokolOdinGenerator::sampler_bind_slot_name(const Sampler& smp) {
    return fmt::format("SMP_{}{}", mod_prefix, smp.name);
}

std::string SokolOdinGenerator::uniform_block_bind_slot_name(const UniformBlock& ub) {
    return fmt::format("UB_{}{}", mod_prefix, ub.name);
}

std::string SokolOdinGenerator::storage_buffer_bind_slot_name(const StorageBuffer& sbuf) {
    return fmt::format("SBUF_{}{}", mod_prefix, sbuf.name);
}

std::string SokolOdinGenerator::vertex_attr_definition(const std::string& prog_name, const StageAttr& attr) {
    return fmt::format("{} :: {}", vertex_attr_name(prog_name, attr), attr.slot);
}

std::string SokolOdinGenerator::image_bind_slot_definition(const Image& img) {
    return fmt::format("{} :: {}", image_bind_slot_name(img), img.sokol_slot);
}

std::string SokolOdinGenerator::sampler_bind_slot_definition(const Sampler& smp) {
    return fmt::format("{} :: {}", sampler_bind_slot_name(smp), smp.sokol_slot);
}

std::string SokolOdinGenerator::uniform_block_bind_slot_definition(const UniformBlock& ub) {
    return fmt::format("{} :: {}", uniform_block_bind_slot_name(ub), ub.sokol_slot);
}

std::string SokolOdinGenerator::storage_buffer_bind_slot_definition(const StorageBuffer& sbuf) {
    return fmt::format("{} :: {}", storage_buffer_bind_slot_name(sbuf), sbuf.sokol_slot);
}

} // namespace
