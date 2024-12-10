/*
    Generate output header in C for sokol_gfx.h
*/
#include "sokolc.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc::gen {

using namespace refl;

static const char* sokol_define(Slang::Enum slang) {
    switch (slang) {
        case Slang::GLSL410:
        case Slang::GLSL430:
            return "SOKOL_GLCORE";
        case Slang::GLSL300ES:
            return "SOKOL_GLES3";
        case Slang::HLSL4:
        case Slang::HLSL5:
            return "SOKOL_D3D11";
        case Slang::METAL_MACOS:
        case Slang::METAL_IOS:
        case Slang::METAL_SIM:
            return "SOKOL_METAL";
        case Slang::WGSL:
            return "SOKOL_WGPU";
        default:
            return "<INVALID>";
    }
}

ErrMsg SokolCGenerator::begin(const GenInput& gen) {
    if (!gen.inp.module.empty()) {
        mod_prefix = fmt::format("{}_", gen.inp.module);
    }
    if (gen.args.output_format != Format::SOKOL_IMPL) {
        func_prefix = "static inline ";
    }
    return Generator::begin(gen);
}

void SokolCGenerator::gen_prolog(const GenInput& gen) {
    l("#pragma once\n");
    for (const auto& header: gen.inp.headers) {
        l("{}\n", header);
    }
}

void SokolCGenerator::gen_epilog(const GenInput& gen) {
    // empty
}

void SokolCGenerator::gen_prerequisites(const GenInput& gen) {
    l("#if !defined(SOKOL_GFX_INCLUDED)\n");
    l("#error \"Please include sokol_gfx.h before {}\"\n", pystring::os::path::basename(gen.args.output));
    l("#endif\n");
    l("#if !defined(SOKOL_SHDC_ALIGN)\n");
    l("#if defined(_MSC_VER)\n");
    l("#define SOKOL_SHDC_ALIGN(a) __declspec(align(a))\n");
    l("#else\n");
    l("#define SOKOL_SHDC_ALIGN(a) __attribute__((aligned(a)))\n");
    l("#endif\n");
    l("#endif\n");
    if (gen.args.output_format == Format::SOKOL_IMPL) {
        for (const auto& item: gen.inp.programs) {
            const Program& prog = item.second;
            l("const sg_shader_desc* {}{}_shader_desc(sg_backend backend);\n", mod_prefix, prog.name);
            if (gen.args.reflection) {
                l("int {}{}_attr_slot(const char* attr_name);\n", mod_prefix, prog.name);
                l("int {}{}_image_slot(sg_shader_stage stage, const char* img_name);\n", mod_prefix, prog.name);
                l("int {}{}_sampler_slot(sg_shader_stage stage, const char* smp_name);\n", mod_prefix, prog.name);
                l("int {}{}_uniformblock_slot(sg_shader_stage stage, const char* ub_name);\n", mod_prefix, prog.name);
                l("size_t {}{}_uniformblock_size(sg_shader_stage stage, const char* ub_name);\n", mod_prefix, prog.name);
                l("int {}{}_storagebuffer_slot(sg_shader_stage stage, const char* sbuf_name);\n", mod_prefix, prog.name);
                l("int {}{}_uniform_offset(sg_shader_stage stage, const char* ub_name, const char* u_name);\n", mod_prefix, prog.name);
                l("sg_shader_uniform_desc {}{}_uniform_desc(sg_shader_stage stage, const char* ub_name, const char* u_name);\n", mod_prefix, prog.name);
            }
        }
    }
}

void SokolCGenerator::gen_uniform_block_decl(const GenInput &gen, const UniformBlock& ub) {
    l("#pragma pack(push,1)\n");
    int cur_offset = 0;
    l_open("SOKOL_SHDC_ALIGN({}) typedef struct {} {{\n", ub.struct_info.align, struct_name(ub.name));
    for (const Type& uniform: ub.struct_info.struct_items) {
        int next_offset = uniform.offset;
        if (next_offset > cur_offset) {
            l("uint8_t _pad_{}[{}];\n", cur_offset, next_offset - cur_offset);
            cur_offset = next_offset;
        }
        if (gen.inp.ctype_map.count(uniform.type_as_glsl()) > 0) {
            // user-provided type names
            if (uniform.array_count == 0) {
                l("{} {};\n", gen.inp.ctype_map.at(uniform.type_as_glsl()), uniform.name);
            } else {
                l("{} {}[{}];\n", gen.inp.ctype_map.at(uniform.type_as_glsl()), uniform.name, uniform.array_count);
            }
        } else {
            // default type names (float)
            if (uniform.array_count == 0) {
                switch (uniform.type) {
                    case Type::Float:   l("float {};\n", uniform.name); break;
                    case Type::Float2:  l("float {}[2];\n", uniform.name); break;
                    case Type::Float3:  l("float {}[3];\n", uniform.name); break;
                    case Type::Float4:  l("float {}[4];\n", uniform.name); break;
                    case Type::Int:     l("int {};\n", uniform.name); break;
                    case Type::Int2:    l("int {}[2];\n", uniform.name); break;
                    case Type::Int3:    l("int {}[3];\n", uniform.name); break;
                    case Type::Int4:    l("int {}[4];\n", uniform.name); break;
                    case Type::Mat4x4:  l("float {}[16];\n", uniform.name); break;
                    default:            l("INVALID_UNIFORM_TYPE;\n"); break;
                }
            } else {
                switch (uniform.type) {
                    case Type::Float4:  l("float {}[{}][4];\n", uniform.name, uniform.array_count); break;
                    case Type::Int4:    l("int {}[{}][4];\n",   uniform.name, uniform.array_count); break;
                    case Type::Mat4x4:  l("float {}[{}][16];\n", uniform.name, uniform.array_count); break;
                    default:            l("INVALID_UNIFORM_TYPE;\n"); break;
                }
            }
        }
        cur_offset += uniform.size;
    }
    // pad to multiple of 16-bytes struct size
    const int round16 = roundup(cur_offset, 16);
    if (cur_offset < round16) {
        l("uint8_t _pad_{}[{}];\n", cur_offset, round16 - cur_offset);
    }
    l_close("}} {};\n", struct_name(ub.name));
    l("#pragma pack(pop)\n");
}

void SokolCGenerator::gen_struct_interior_decl_std430(const GenInput& gen, const Type& struc, int pad_to_size) {
    assert(struc.type == Type::Struct);
    assert(pad_to_size > 0);

    int cur_offset = 0;
    for (const Type& item: struc.struct_items) {
        int next_offset = item.offset;
        if (next_offset > cur_offset) {
            l("uint8_t _pad_{}[{}];\n", cur_offset, next_offset - cur_offset);
            cur_offset = next_offset;
        }
        if (item.type == Type::Struct) {
            // recurse into nested struct
            l_open("struct {{\n");
            gen_struct_interior_decl_std430(gen, item, item.size);
            if (item.array_count == 0) {
                // FIXME: do we need any padding here if array_stride != struct-size?
                // NOTE: unbounded arrays are written as regular items
                l_close("}} {};\n", item.name);
            } else {
                l_close("}} {}[{}];\n", item.name, item.array_count);
            }
        } else if (gen.inp.ctype_map.count(item.type_as_glsl()) > 0) {
            // user-mapped typename
            if (item.array_count == 0) {
                l("{} {};\n", gen.inp.ctype_map.at(item.type_as_glsl()), item.name);
            } else {
                l("{} {}[{}];\n", gen.inp.ctype_map.at(item.type_as_glsl()), item.name, item.array_count);
            }
        } else {
            // default typenames
            if (item.array_count == 0) {
                switch (item.type) {
                    // NOTE: bool => int is not a bug!
                    case Type::Bool:    l("int32_t {};\n", item.name); break;
                    case Type::Bool2:   l("int32_t {}[2];\n", item.name); break;
                    case Type::Bool3:   l("int32_t {}[3];\n", item.name); break;
                    case Type::Bool4:   l("int32_t {}[4];\n", item.name); break;
                    case Type::Int:     l("int32_t {};\n", item.name); break;
                    case Type::Int2:    l("int32_t {}[2];\n", item.name); break;
                    case Type::Int3:    l("int32_t {}[3];\n", item.name); break;
                    case Type::Int4:    l("int32_t {}[4];\n", item.name); break;
                    case Type::UInt:    l("uint32_t {};\n", item.name); break;
                    case Type::UInt2:   l("uint32_t {}[2];\n", item.name); break;
                    case Type::UInt3:   l("uint32_t {}[3];\n", item.name); break;
                    case Type::UInt4:   l("uint32_t {}[4];\n", item.name); break;
                    case Type::Float:   l("float {};\n", item.name); break;
                    case Type::Float2:  l("float {}[2];\n", item.name); break;
                    case Type::Float3:  l("float {}[3];\n", item.name); break;
                    case Type::Float4:  l("float {}[4];\n", item.name); break;
                    case Type::Mat2x1:  l("float {}[2];\n", item.name); break;
                    case Type::Mat2x2:  l("float {}[4];\n", item.name); break;
                    case Type::Mat2x3:  l("float {}[6];\n", item.name); break;
                    case Type::Mat2x4:  l("float {}[8];\n", item.name); break;
                    case Type::Mat3x1:  l("float {}[3];\n", item.name); break;
                    case Type::Mat3x2:  l("float {}[6];\n", item.name); break;
                    case Type::Mat3x3:  l("float {}[9];\n", item.name); break;
                    case Type::Mat3x4:  l("float {}[12];\n", item.name); break;
                    case Type::Mat4x1:  l("float {}[4];\n", item.name); break;
                    case Type::Mat4x2:  l("float {}[8];\n", item.name); break;
                    case Type::Mat4x3:  l("float {}[12];\n", item.name); break;
                    case Type::Mat4x4:  l("float {}[16];\n", item.name); break;
                    default: l("INVALID_TYPE\n"); break;
                }
            } else {
                switch (item.type) {
                    // NOTE: bool => int is not a bug!
                    case Type::Bool:    l("int32_t {}[{}];\n", item.name, item.array_count); break;
                    case Type::Bool2:   l("int32_t {}[{}][2];\n", item.name, item.array_count); break;
                    case Type::Bool3:   l("int32_t {}[{}][3];\n", item.name, item.array_count); break;
                    case Type::Bool4:   l("int32_t {}[{}][4];\n", item.name, item.array_count); break;
                    case Type::Int:     l("int32_t {}[{}];\n", item.name, item.array_count); break;
                    case Type::Int2:    l("int32_t {}[{}][2];\n", item.name, item.array_count); break;
                    case Type::Int3:    l("int32_t {}[{}][3];\n", item.name, item.array_count); break;
                    case Type::Int4:    l("int32_t {}[{}][4];\n", item.name, item.array_count); break;
                    case Type::UInt:    l("uint32_t {}[{}];\n", item.name, item.array_count); break;
                    case Type::UInt2:   l("uint32_t {}[{}][2];\n", item.name, item.array_count); break;
                    case Type::UInt3:   l("uint32_t {}[{}][3];\n", item.name, item.array_count); break;
                    case Type::UInt4:   l("uint32_t {}[{}][4];\n", item.name, item.array_count); break;
                    case Type::Float:   l("float {}[{}];\n", item.name, item.array_count); break;
                    case Type::Float2:  l("float {}[{}][2];\n", item.name, item.array_count); break;
                    case Type::Float3:  l("float {}[{}][3];\n", item.name, item.array_count); break;
                    case Type::Float4:  l("float {}[{}][4];\n", item.name, item.array_count); break;
                    case Type::Mat2x1:  l("float {}[{}][2];\n", item.name, item.array_count); break;
                    case Type::Mat2x2:  l("float {}[{}][4];\n", item.name, item.array_count); break;
                    case Type::Mat2x3:  l("float {}[{}][6];\n", item.name, item.array_count); break;
                    case Type::Mat2x4:  l("float {}[{}][8];\n", item.name, item.array_count); break;
                    case Type::Mat3x1:  l("float {}[{}][3];\n", item.name, item.array_count); break;
                    case Type::Mat3x2:  l("float {}[{}][6];\n", item.name, item.array_count); break;
                    case Type::Mat3x3:  l("float {}[{}][9];\n", item.name, item.array_count); break;
                    case Type::Mat3x4:  l("float {}[{}][12];\n", item.name, item.array_count); break;
                    case Type::Mat4x1:  l("float {}[{}][4];\n", item.name, item.array_count); break;
                    case Type::Mat4x2:  l("float {}[{}][8];\n", item.name, item.array_count); break;
                    case Type::Mat4x3:  l("float {}[{}][12];\n", item.name, item.array_count); break;
                    case Type::Mat4x4:  l("float {}[{}][16];\n", item.name, item.array_count); break;
                    default: l("INVALID_TYPE\n"); break;
                }
            }
        }
        cur_offset += item.size;
    }
    if (cur_offset < pad_to_size) {
        l("uint8_t _pad_{}[{}];\n", cur_offset, pad_to_size - cur_offset);
    }
}

void SokolCGenerator::gen_storage_buffer_decl(const GenInput& gen, const StorageBuffer& sbuf) {
    l("#pragma pack(push,1)\n");
    const auto& item = sbuf.struct_info.struct_items[0];
    l_open("SOKOL_SHDC_ALIGN({}) typedef struct {} {{\n", sbuf.struct_info.align, struct_name(item.struct_typename));
    gen_struct_interior_decl_std430(gen, item, sbuf.struct_info.size);
    l_close("}} {};\n", struct_name(item.struct_typename));
    l("#pragma pack(pop)\n");
}

void SokolCGenerator::gen_shader_desc_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}const sg_shader_desc* {}{}_shader_desc(sg_backend backend) {{\n", func_prefix, mod_prefix, prog.name);
    for (int i = 0; i < Slang::Num; i++) {
        Slang::Enum slang = Slang::from_index(i);
        if (gen.args.slang & Slang::bit(slang)) {
            if (gen.args.ifdef) {
                l("#if defined({})\n", sokol_define(slang));
            }
            l_open("if (backend == {}) {{\n", backend(slang));
            l("static sg_shader_desc desc;\n");
            l("static bool valid;\n");
            l_open("if (!valid) {{\n");
            l("valid = true;\n");
            for (int stage_index = 0; stage_index < ShaderStage::Num; stage_index++) {
                const ShaderStageArrayInfo& info = shader_stage_array_info(gen, prog, ShaderStage::from_index(stage_index), slang);
                const StageReflection& refl = prog.stages[stage_index];
                const std::string dsn = fmt::format("desc.{}", info.stage == ShaderStage::Vertex ? "vertex_func" : "fragment_func");
                if (info.has_bytecode) {
                    l("{}.bytecode.ptr = {};\n", dsn, info.bytecode_array_name);
                    l("{}.bytecode.size = {};\n", dsn, info.bytecode_array_size);
                } else {
                    l("{}.source = (const char*){};\n", dsn, info.source_array_name);
                    const char* d3d11_tgt = nullptr;
                    if (slang == Slang::HLSL4) {
                        d3d11_tgt = (0 == stage_index) ? "vs_4_0" : "ps_4_0";
                    } else if (slang == Slang::HLSL5) {
                        d3d11_tgt = (0 == stage_index) ? "vs_5_0" : "ps_5_0";
                    }
                    if (d3d11_tgt) {
                        l("{}.d3d11_target = \"{}\";\n", dsn, d3d11_tgt);
                    }
                }
                l("{}.entry = \"{}\";\n", dsn, refl.entry_point_by_slang(slang));
            }
            for (int attr_index = 0; attr_index < StageAttr::Num; attr_index++) {
                const StageAttr& attr = prog.vs().inputs[attr_index];
                if (attr.slot >= 0) {
                    if (Slang::is_glsl(slang)) {
                        l("desc.attrs[{}].glsl_name = \"{}\";\n", attr_index, attr.name);
                    } else if (Slang::is_hlsl(slang)) {
                        l("desc.attrs[{}].hlsl_sem_name = \"{}\";\n", attr_index, attr.sem_name);
                        l("desc.attrs[{}].hlsl_sem_index = {};\n", attr_index, attr.sem_index);
                    }
                }
            }
            for (int ub_index = 0; ub_index < Bindings::MaxUniformBlocks; ub_index++) {
                const UniformBlock* ub = prog.bindings.find_uniform_block_by_sokol_slot(ub_index);
                if (ub) {
                    const std::string ubn = fmt::format("desc.uniform_blocks[{}]", ub_index);
                    l("{}.stage = {};\n", ubn, shader_stage(ub->stage));
                    l("{}.layout = SG_UNIFORMLAYOUT_STD140;\n", ubn);
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
                    l("{}.image_type = {};\n", in, image_type(img->type));
                    l("{}.sample_type = {};\n", in, image_sample_type(img->sample_type));
                    l("{}.multisampled = {};\n", in, img->multisampled ? "true" : "false");
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
                        l("{}.glsl_name = \"{}\";\n", isn, img_smp->name);
                    }
                }
            }
            l("desc.label = \"{}{}_shader\";\n", mod_prefix, prog.name);
            l_close("}}\n");
            l("return &desc;\n");
            l_close("}}\n");
            if (gen.args.ifdef) {
                l("#endif /* {} */\n", sokol_define(slang));
            }
        }
    }
    l("return 0;\n");
    l_close("}}\n");
}

void SokolCGenerator::gen_attr_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}int {}{}_attr_slot(const char* attr_name) {{\n", func_prefix, mod_prefix, prog.name);
    l("(void)attr_name;\n");
    for (const StageAttr& attr: prog.vs().inputs) {
        if (attr.slot >= 0) {
            l_open("if (0 == strcmp(attr_name, \"{}\")) {{\n", attr.name);
            l("return {};\n", attr.slot);
            l_close("}}\n");
        }
    }
    l("return -1;\n");
    l_close("}}\n");
}

void SokolCGenerator::gen_image_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}int {}{}_image_slot(const char* img_name) {{\n", func_prefix, mod_prefix, prog.name);
    l("(void)img_name;\n");
    for (const Image& img: prog.bindings.images) {
        if (img.sokol_slot >= 0) {
            l_open("if (0 == strcmp(img_name, \"{}\")) {{\n", img.name);
            l("return {};\n", img.sokol_slot);
            l_close("}}\n");
        }
    }
    l("return -1;\n");
    l_close("}}\n");
}

void SokolCGenerator::gen_sampler_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}int {}{}_sampler_slot(const char* smp_name) {{\n", func_prefix, mod_prefix, prog.name);
    l("(void)smp_name;\n");
    for (const Sampler& smp: prog.bindings.samplers) {
        if (smp.sokol_slot >= 0) {
            l_open("if (0 == strcmp(smp_name, \"{}\")) {{\n", smp.name);
            l("return {};\n", smp.sokol_slot);
            l_close("}}\n");
        }
    }
    l("return -1;\n");
    l_close("}}\n");
}

void SokolCGenerator::gen_uniform_block_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}int {}{}_uniformblock_slot(const char* ub_name) {{\n", func_prefix, mod_prefix, prog.name);
    l("(void)ub_name;\n");
    for (const UniformBlock& ub: prog.bindings.uniform_blocks) {
        if (ub.sokol_slot >= 0) {
            l_open("if (0 == strcmp(ub_name, \"{}\")) {{\n", ub.name);
            l("return {};\n", ub.sokol_slot);
            l_close("}}\n");
        }
    }
    l("return -1;\n");
    l_close("}}\n");
}

void SokolCGenerator::gen_uniform_block_size_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}size_t {}{}_uniformblock_size(const char* ub_name) {{\n", func_prefix, mod_prefix, prog.name);
    l("(void)ub_name;\n");
    for (const UniformBlock& ub: prog.bindings.uniform_blocks) {
        if (ub.sokol_slot >= 0) {
            l_open("if (0 == strcmp(ub_name, \"{}\")) {{\n", ub.name);
            l("return sizeof({});\n", struct_name(ub.name));
            l_close("}}\n");
        }
    }
    l("return 0;\n");
    l_close("}}\n");
}

void SokolCGenerator::gen_storage_buffer_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}int {}{}_storagebuffer_slot(const char* sbuf_name) {{\n", func_prefix, mod_prefix, prog.name);
    l("(void)sbuf_name;\n");
    for (const StorageBuffer& sbuf: prog.bindings.storage_buffers) {
        if (sbuf.sokol_slot >= 0) {
            l_open("if (0 == strcmp(sbuf_name, \"{}\")) {{\n", sbuf.name);
            l("return {};\n", sbuf.sokol_slot);
            l_close("}}\n");
        }
    }
    l("return -1;\n");
    l_close("}}\n");
}

void SokolCGenerator::gen_uniform_offset_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}int {}{}_uniform_offset(const char* ub_name, const char* u_name) {{\n", func_prefix, mod_prefix, prog.name);
    l("(void)ub_name; (void)u_name;\n");
    for (const UniformBlock& ub: prog.bindings.uniform_blocks) {
        if (ub.sokol_slot >= 0) {
            l_open("if (0 == strcmp(ub_name, \"{}\")) {{\n", ub.name);
            for (const Type& u: ub.struct_info.struct_items) {
                l_open("if (0 == strcmp(u_name, \"{}\")) {{\n", u.name);
                l("return {};\n", u.offset);
                l_close("}}\n");
            }
            l_close("}}\n");
        }
    }
    l("return -1;\n");
    l_close("}}\n");
}

void SokolCGenerator::gen_uniform_desc_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("{}sg_glsl_shader_uniform {}{}_uniform_desc(const char* ub_name, const char* u_name) {{\n", func_prefix, mod_prefix, prog.name);
    l("(void)ub_name; (void)u_name;\n");
    l("#if defined(__cplusplus)\n");
    l("sg_glsl_shader_uniform res = {{}};\n");
    l("#else\n");
    l("sg_glsl_shader_uniform res = {{0}};\n");
    l("#endif\n");
    for (const UniformBlock& ub: prog.bindings.uniform_blocks) {
        if (ub.sokol_slot >= 0) {
            l_open("if (0 == strcmp(ub_name, \"{}\")) {{\n", ub.name);
            for (const Type& u: ub.struct_info.struct_items) {
                l_open("if (0 == strcmp(u_name, \"{}\")) {{\n", u.name);
                l("res.type = {};\n", uniform_type(u.type));
                l("res.array_count = {};\n", u.array_count);
                l("res.glsl_name = \"{}\";\n", u.name);
                l("return res;\n");
                l_close("}}\n");
            }
            l_close("}}\n");
        }
    }
    l("return res;\n");
    l_close("}}\n");
}

void SokolCGenerator::gen_shader_array_start(const GenInput& gen, const std::string& array_name, size_t num_bytes, Slang::Enum slang) {
    if (gen.args.ifdef) {
        l("#if defined({})\n", sokol_define(slang));
    }
    l("static const uint8_t {}[{}] = {{\n", array_name, num_bytes);
}

void SokolCGenerator::gen_shader_array_end(const GenInput& gen) {
    l("\n}};\n");
    if (gen.args.ifdef) {
        l("#endif\n");
    }
}

void SokolCGenerator::gen_stb_impl_start(const GenInput &gen) {
    if (gen.args.output_format == Format::SOKOL_IMPL) {
        l("#if defined(SOKOL_SHDC_IMPL)\n");
    }
}

void SokolCGenerator::gen_stb_impl_end(const GenInput& gen) {
    if (gen.args.output_format == Format::SOKOL_IMPL) {
        l("#endif // SOKOL_SHDC_IMPL");
    }
}

std::string SokolCGenerator::lang_name() {
    return "C";
}

std::string SokolCGenerator::shader_bytecode_array_name(const std::string& snippet_name, Slang::Enum slang) {
    return fmt::format("{}{}_bytecode_{}", mod_prefix, snippet_name, Slang::to_str(slang));
}

std::string SokolCGenerator::shader_source_array_name(const std::string& snippet_name, Slang::Enum slang) {
    return fmt::format("{}{}_source_{}", mod_prefix, snippet_name, Slang::to_str(slang));
}

std::string SokolCGenerator::comment_block_start() {
    return "/*";
}

std::string SokolCGenerator::comment_block_end() {
    return "*/";
}

std::string SokolCGenerator::comment_block_line_prefix() {
    return "";
}

std::string SokolCGenerator::get_shader_desc_help(const std::string& prog_name) {
    return fmt::format("{}{}_shader_desc(sg_query_backend());\n", mod_prefix, prog_name);
}

std::string SokolCGenerator::shader_stage(ShaderStage::Enum e) {
    switch (e) {
        case ShaderStage::Vertex: return "SG_SHADERSTAGE_VERTEX";
        case ShaderStage::Fragment: return "SG_SHADERSTAGE_FRAGMENT";
        default: return "INVALID";
    }
}

std::string SokolCGenerator::uniform_type(Type::Enum e) {
    switch (e) {
        case Type::Float:  return "SG_UNIFORMTYPE_FLOAT";
        case Type::Float2: return "SG_UNIFORMTYPE_FLOAT2";
        case Type::Float3: return "SG_UNIFORMTYPE_FLOAT3";
        case Type::Float4: return "SG_UNIFORMTYPE_FLOAT4";
        case Type::Int:    return "SG_UNIFORMTYPE_INT";
        case Type::Int2:   return "SG_UNIFORMTYPE_INT2";
        case Type::Int3:   return "SG_UNIFORMTYPE_INT3";
        case Type::Int4:   return "SG_UNIFORMTYPE_INT4";
        case Type::Mat4x4: return "SG_UNIFORMTYPE_MAT4";
        default: return "INVALID";
    }
}

std::string SokolCGenerator::flattened_uniform_type(Type::Enum e) {
    switch (e) {
        case Type::Float:
        case Type::Float2:
        case Type::Float3:
        case Type::Float4:
        case Type::Mat4x4:
             return "SG_UNIFORMTYPE_FLOAT4";
        case Type::Int:
        case Type::Int2:
        case Type::Int3:
        case Type::Int4:
            return "SG_UNIFORMTYPE_INT4";
        default:
            return "INVALID";
    }
}

std::string SokolCGenerator::image_type(ImageType::Enum e) {
    switch (e) {
        case ImageType::_2D:     return "SG_IMAGETYPE_2D";
        case ImageType::CUBE:    return "SG_IMAGETYPE_CUBE";
        case ImageType::_3D:     return "SG_IMAGETYPE_3D";
        case ImageType::ARRAY:   return "SG_IMAGETYPE_ARRAY";
        default: return "INVALID";
    }
}

std::string SokolCGenerator::image_sample_type(ImageSampleType::Enum e) {
    switch (e) {
        case ImageSampleType::FLOAT: return "SG_IMAGESAMPLETYPE_FLOAT";
        case ImageSampleType::DEPTH: return "SG_IMAGESAMPLETYPE_DEPTH";
        case ImageSampleType::SINT:  return "SG_IMAGESAMPLETYPE_SINT";
        case ImageSampleType::UINT:  return "SG_IMAGESAMPLETYPE_UINT";
        case ImageSampleType::UNFILTERABLE_FLOAT:  return "SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT";
        default: return "INVALID";
    }
}

std::string SokolCGenerator::sampler_type(SamplerType::Enum e) {
    switch (e) {
        case SamplerType::FILTERING:     return "SG_SAMPLERTYPE_FILTERING";
        case SamplerType::COMPARISON:    return "SG_SAMPLERTYPE_COMPARISON";
        case SamplerType::NONFILTERING:  return "SG_SAMPLERTYPE_NONFILTERING";
        default: return "INVALID";
    }
}

std::string SokolCGenerator::backend(Slang::Enum e) {
    switch (e) {
        case Slang::GLSL410:
        case Slang::GLSL430:
            return "SG_BACKEND_GLCORE";
        case Slang::GLSL300ES:
            return "SG_BACKEND_GLES3";
        case Slang::HLSL4:
        case Slang::HLSL5:
            return "SG_BACKEND_D3D11";
        case Slang::METAL_MACOS:
            return "SG_BACKEND_METAL_MACOS";
        case Slang::METAL_IOS:
            return "SG_BACKEND_METAL_IOS";
        case Slang::METAL_SIM:
            return "SG_BACKEND_METAL_SIMULATOR";
        case Slang::WGSL:
            return "SG_BACKEND_WGPU";
        default:
            return "<INVALID>";
    }
}

std::string SokolCGenerator::struct_name(const std::string& name) {
    return fmt::format("{}{}_t", mod_prefix, name);
}

std::string SokolCGenerator::vertex_attr_name(const std::string& prog_name, const StageAttr& attr) {
    return fmt::format("ATTR_{}{}_{}", mod_prefix, prog_name, attr.name);
}

std::string SokolCGenerator::image_bind_slot_name(const Image& img) {
    return fmt::format("IMG_{}{}", mod_prefix, img.name);
}

std::string SokolCGenerator::sampler_bind_slot_name(const Sampler& smp) {
    return fmt::format("SMP_{}{}", mod_prefix, smp.name);
}

std::string SokolCGenerator::uniform_block_bind_slot_name(const UniformBlock& ub) {
    return fmt::format("UB_{}{}", mod_prefix, ub.name);
}

std::string SokolCGenerator::storage_buffer_bind_slot_name(const StorageBuffer& sbuf) {
    return fmt::format("SBUF_{}{}", mod_prefix, sbuf.name);
}

std::string SokolCGenerator::vertex_attr_definition(const std::string& prog_name, const StageAttr& attr) {
    return fmt::format("#define {} ({})", vertex_attr_name(prog_name, attr), attr.slot);
}

std::string SokolCGenerator::image_bind_slot_definition(const Image& img) {
    return fmt::format("#define {} ({})", image_bind_slot_name(img), img.sokol_slot);
}

std::string SokolCGenerator::sampler_bind_slot_definition(const Sampler& smp) {
    return fmt::format("#define {} ({})", sampler_bind_slot_name(smp), smp.sokol_slot);
}

std::string SokolCGenerator::uniform_block_bind_slot_definition(const UniformBlock& ub) {
    return fmt::format("#define {} ({})", uniform_block_bind_slot_name(ub), ub.sokol_slot);
}

std::string SokolCGenerator::storage_buffer_bind_slot_definition(const StorageBuffer& sbuf) {
    return fmt::format("#define {} ({})", storage_buffer_bind_slot_name(sbuf), sbuf.sokol_slot);
}

} // namespace
