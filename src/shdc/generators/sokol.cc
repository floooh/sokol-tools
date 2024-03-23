/*
    Generate output header in C for sokol_gfx.h
*/
#include "sokol.h"
#include "util.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc::gen {

using namespace util;
using namespace refl;

static const char* sokol_define(Slang::Enum slang) {
    switch (slang) {
        case Slang::GLSL410:      return "SOKOL_GLCORE";
        case Slang::GLSL430:      return "SOKOL_GLCORE";
        case Slang::GLSL300ES:    return "SOKOL_GLES3";
        case Slang::HLSL4:        return "SOKOL_D3D11";
        case Slang::HLSL5:        return "SOKOL_D3D11";
        case Slang::METAL_MACOS:  return "SOKOL_METAL";
        case Slang::METAL_IOS:    return "SOKOL_METAL";
        case Slang::METAL_SIM:    return "SOKOL_METAL";
        case Slang::WGSL:         return "SOKOL_WGPU";
        default: return "<INVALID>";
    }
}

static std::string func_prefix(const Args& args) {
    if (args.output_format != Format::SOKOL_IMPL) {
        return std::string("static inline ");
    } else {
        return std::string();
    }
}

void SokolGenerator::gen_prolog(const GenInput& gen) {
    l("#pragma once\n");
}

void SokolGenerator::gen_epilog(const GenInput& gen) {
    // empty
}

void SokolGenerator::gen_prerequisites(const GenInput& gen) {
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
                l("int {}{}_uniform_offset(sg_shader_stage stage, const char* ub_name, const char* u_name);\n", mod_prefix, prog.name);
                l("sg_shader_uniform_desc {}{}_uniform_desc(sg_shader_stage stage, const char* ub_name, const char* u_name);\n", mod_prefix, prog.name);
            }
        }
    }
}

void SokolGenerator::gen_uniformblock_decl(const GenInput &gen, const UniformBlock& ub) {
    reset_indent();
    l("#pragma pack(push,1)\n");
    int cur_offset = 0;
    l("SOKOL_SHDC_ALIGN(16) typedef struct {} {{\n", struct_name(ub.struct_name));
    indent();
    for (const Uniform& uniform: ub.uniforms) {
        int next_offset = uniform.offset;
        if (next_offset > cur_offset) {
            l("uint8_t _pad_{}[{}];\n", cur_offset, next_offset - cur_offset);
            cur_offset = next_offset;
        }
        if (gen.inp.ctype_map.count(uniform.type_as_glsl()) > 0) {
            // user-provided type names
            if (uniform.array_count == 1) {
                l("{} {};\n", gen.inp.ctype_map.at(uniform.type_as_glsl()), uniform.name);
            } else {
                l("{} {}[{}];\n", gen.inp.ctype_map.at(uniform.type_as_glsl()), uniform.name, uniform.array_count);
            }
        } else {
            // default type names (float)
            if (uniform.array_count == 1) {
                switch (uniform.type) {
                    case Uniform::FLOAT:      l("float {};\n", uniform.name); break;
                    case Uniform::FLOAT2:     l("float {}[2];\n", uniform.name); break;
                    case Uniform::FLOAT3:     l("float {}[3];\n", uniform.name); break;
                    case Uniform::FLOAT4:     l("float {}[4];\n", uniform.name); break;
                    case Uniform::INT:        l("int {};\n", uniform.name); break;
                    case Uniform::INT2:       l("int {}[2];\n", uniform.name); break;
                    case Uniform::INT3:       l("int {}[3];\n", uniform.name); break;
                    case Uniform::INT4:       l("int {}[4];\n", uniform.name); break;
                    case Uniform::MAT4:       l("float {}[16];\n", uniform.name); break;
                    default:                  l("INVALID_UNIFORM_TYPE;\n"); break;
                }
            } else {
                switch (uniform.type) {
                    case Uniform::FLOAT4:     l("float {}[{}][4];\n", uniform.name, uniform.array_count); break;
                    case Uniform::INT4:       l("int {}[{}][4];\n",   uniform.name, uniform.array_count); break;
                    case Uniform::MAT4:       l("float {}[{}][16];\n", uniform.name, uniform.array_count); break;
                    default:                  l("INVALID_UNIFORM_TYPE;\n"); break;
                }
            }
        }
        cur_offset += uniform.size_bytes();
    }
    // pad to multiple of 16-bytes struct size
    const int round16 = roundup(cur_offset, 16);
    if (cur_offset != round16) {
        l("uint8_t _pad_{}[{}];\n", cur_offset, round16 - cur_offset);
    }
    dedent();
    l("}} {};\n", struct_name(ub.struct_name));
    l("#pragma pack(pop)\n");
}

void SokolGenerator::gen_shader_desc_func(const GenInput& gen, const Program& prog) {
    reset_indent();
    l("{}const sg_shader_desc* {}{}_shader_desc(sg_backend backend) {{\n", func_prefix(gen.args), mod_prefix, prog.name);
    indent();
    for (int i = 0; i < Slang::NUM; i++) {
        Slang::Enum slang = Slang::from_index(i);
        if (gen.args.slang & Slang::bit(slang)) {
            if (gen.args.ifdef) {
                l("#if defined({})\n", sokol_define(slang));
            }
            l("if (backend == {}) {{\n", backend(slang));
            indent();
            l("static sg_shader_desc desc;\n");
            l("static bool valid;\n");
            l("if (!valid) {{\n");
            indent();
            l("valid = true;\n");
            const ShaderStageInfo attr_info = get_shader_stage_info(gen, prog, ShaderStage::VS, slang);
            for (int attr_index = 0; attr_index < VertexAttr::NUM; attr_index++) {
                const VertexAttr& attr = attr_info.source->refl.inputs[attr_index];
                if (attr.slot >= 0) {
                    if (Slang::is_glsl(slang)) {
                        l("desc.attrs[{}].name = \"{}\";\n", attr_index, attr.name);
                    } else if (Slang::is_hlsl(slang)) {
                        l("desc.attrs[{}].sem_name = \"{}\";\n", attr_index, attr.sem_name);
                        l("desc.attrs[{}].sem_index = {};\n", attr_index, attr.sem_index);
                    }
                }
            }
            for (int stage_index = 0; stage_index < 2; stage_index++) {
                const ShaderStageInfo info = get_shader_stage_info(gen, prog, ShaderStage::from_index(stage_index), slang);
                const std::string dsn = fmt::format("desc.{}", info.stage_name);
                if (info.bytecode) {
                    l("{}.bytecode.ptr = {};\n", dsn, info.bytecode_array_name);
                    l("{}.bytecode.size = {};\n", dsn, info.bytecode->data.size());
                } else {
                    l("{}.source = {};\n", dsn, info.source_array_name);
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
                l("{}.entry = \"{}\";\n", dsn, info.source->refl.entry_point);
                for (int ub_index = 0; ub_index < UniformBlock::NUM; ub_index++) {
                    const UniformBlock* ub = info.source->refl.bindings.find_uniform_block_by_slot(ub_index);
                    if (ub) {
                        const std::string ubn = fmt::format("{}.uniform_blocks[{}]", dsn, ub_index);
                        l("{}.size = {};\n", ubn, roundup(ub->size, 16));
                        l("{}.layout = SG_UNIFORMLAYOUT_STD140;\n", ubn);
                        if (Slang::is_glsl(slang) && (ub->uniforms.size() > 0)) {
                            if (ub->flattened) {
                                l("{}.uniforms[0].name = \"{}\";\n", ubn, ub->struct_name);
                                l("{}.uniforms[0].type = {};\n", ubn, flattened_uniform_type(ub->uniforms[0].type));
                                l("{}.uniforms[0].array_count = {};\n", ubn, roundup(ub->size, 16) / 16);
                            } else {
                                for (int u_index = 0; u_index < (int)ub->uniforms.size(); u_index++) {
                                    const Uniform& u = ub->uniforms[u_index];
                                    const std::string un = fmt::format("{}.uniforms[{}]", ubn, u_index);
                                    l("{}.name = \"{}.{}\";\n", un, ub->inst_name, u.name);
                                    l("{}.type = {};\n", un, uniform_type(u.type));
                                    l(".array_count = {};\n", un, u.array_count);
                                }
                            }
                        }
                    }
                }
                for (int img_index = 0; img_index < Image::NUM; img_index++) {
                    const Image* img = info.source->refl.bindings.find_image_by_slot(img_index);
                    if (img) {
                        const std::string in = fmt::format("{}.images[{}]", dsn, img_index);
                        l("{}.used = true;\n", in);
                        l("{}.multisampled = {};\n", in, img->multisampled ? "true" : "false");
                        l("{}.image_type = {};\n", in, image_type(img->type));
                        l("{}.sample_type = {};\n", in, image_sample_type(img->sample_type));
                    }
                }
                for (int smp_index = 0; smp_index < Sampler::NUM; smp_index++) {
                    const Sampler* smp = info.source->refl.bindings.find_sampler_by_slot(smp_index);
                    if (smp) {
                        const std::string sn = fmt::format("{}.samplers[{}]", dsn, smp_index);
                        l("{}.used = true;\n", sn);
                        l("{}.sampler_type = {};\n", sn, sampler_type(smp->type));
                    }
                }
                for (int img_smp_index = 0; img_smp_index < ImageSampler::NUM; img_smp_index++) {
                    const ImageSampler* img_smp = info.source->refl.bindings.find_image_sampler_by_slot(img_smp_index);
                    if (img_smp) {
                        const std::string isn = fmt::format("{}.image_sampler_pairs[{}]", dsn, img_smp_index);
                        l("{}.used = true;\n", isn);
                        l("{}.image_slot = {};\n", isn, info.source->refl.bindings.find_image_by_name(img_smp->image_name)->slot);
                        l("{}.sampler_slot = {};\n", isn, info.source->refl.bindings.find_sampler_by_name(img_smp->sampler_name)->slot);
                        if (Slang::is_glsl(slang)) {
                            l("{}.glsl_name = \"{}\";\n", isn, img_smp->name);
                        }
                    }
                }
            }
            l("desc.label = \"{}{}_shader\";\n", mod_prefix, prog.name);
            dedent();
            l("}}\n");
            l("return &desc;\n");
            dedent();
            l("}}\n");
            if (gen.args.ifdef) {
                l("#endif /* {} */\n", sokol_define(slang));
            }
        }
    }
    l("return 0;\n");
    dedent();
    l("}}\n");
}

void SokolGenerator::gen_attr_slot_refl_func(const GenInput& gen, const Program& prog) {
    const ShaderStageInfo info = get_shader_stage_info(gen, prog, ShaderStage::VS, Slang::first_valid(gen.args.slang));
    const Reflection& vs_refl = info.source->refl;
    l("{}int {}{}_attr_slot(const char* attr_name) {{\n", func_prefix(gen.args), mod_prefix, prog.name);
    indent();
    l("(void)attr_name;\n");
    for (const VertexAttr& attr: vs_refl.inputs) {
        if (attr.slot >= 0) {
            l("if (0 == strcmp(attr_name, \"{}\")) {{\n", attr.name);
            indent();
            l("return {};\n", attr.slot);
            dedent();
            l("}}\n");
        }
    }
    l("return -1;\n");
    dedent();
    l("}}\n");
}

void SokolGenerator::gen_image_slot_refl_func(const GenInput& gen, const Program& prog) {
    const Slang::Enum slang = Slang::first_valid(gen.args.slang);
    l("{}int {}{}_image_slot(sg_shader_stage stage, const char* img_name) {{\n", func_prefix(gen.args), mod_prefix, prog.name);
    indent();
    l("(void)stage; (void)img_name;\n");
    for (int stage_index = 0; stage_index < 2; stage_index++) {
        const ShaderStageInfo info = get_shader_stage_info(gen, prog, ShaderStage::from_index(stage_index), slang);
        const Reflection& refl = info.source->refl;
        if (!refl.bindings.images.empty()) {
            l("if (SG_SHADERSTAGE_{} == stage) {{\n", pystring::upper(info.stage_name));
            indent();
            for (const Image& img: refl.bindings.images) {
                if (img.slot >= 0) {
                    l("if (0 == strcmp(img_name, \"{}\")) {{\n", img.name);
                    indent();
                    l("return {};\n", img.slot);
                    dedent();
                    l("}}\n");
                }
            }
            dedent();
            l("}}\n");
        }
    }
    l("return -1;\n");
    dedent();
    l("}}\n");
}

void SokolGenerator::gen_sampler_slot_refl_func(const GenInput& gen, const Program& prog) {
    const Slang::Enum slang = Slang::first_valid(gen.args.slang);
    l("{}int {}{}_sampler_slot(sg_shader_stage stage, const char* smp_name) {{\n", func_prefix(gen.args), mod_prefix, prog.name);
    indent();
    l("(void)stage; (void)smp_name;\n");
    for (int stage_index = 0; stage_index < 2; stage_index++) {
        const ShaderStageInfo info = get_shader_stage_info(gen, prog, ShaderStage::from_index(stage_index), slang);
        const Reflection& refl = info.source->refl;
        if (!refl.bindings.samplers.empty()) {
            l("if (SG_SHADERSTAGE_{} == stage) {{\n", pystring::upper(info.stage_name));
            indent();
            for (const Sampler& smp: refl.bindings.samplers) {
                if (smp.slot >= 0) {
                    l("if (0 == strcmp(smp_name, \"{}\")) {{\n", smp.name);
                    indent();
                    l("return {};\n", smp.slot);
                    dedent();
                    l("}}\n");
                }
            }
            dedent();
            l("}}\n");
        }
    }
    l("return -1;\n");
    dedent();
    l("}}\n");
}

void SokolGenerator::gen_uniformblock_slot_refl_func(const GenInput& gen, const Program& prog) {
    const Slang::Enum slang = Slang::first_valid(gen.args.slang);
    l("{}int {}{}_uniformblock_slot(sg_shader_stage stage, const char* ub_name) {{\n", func_prefix(gen.args), mod_prefix, prog.name);
    indent();
    l("(void)stage; (void)ub_name;\n");
    for (int stage_index = 0; stage_index < 2; stage_index++) {
        const ShaderStageInfo info = get_shader_stage_info(gen, prog, ShaderStage::from_index(stage_index), slang);
        const Reflection& refl = info.source->refl;
        if (!refl.bindings.uniform_blocks.empty()) {
            l("if (SG_SHADERSTAGE_{} == stage) {{\n", pystring::upper(info.stage_name));
            indent();
            for (const UniformBlock& ub: refl.bindings.uniform_blocks) {
                if (ub.slot >= 0) {
                    l("if (0 == strcmp(ub_name, \"{}\")) {{\n", ub.struct_name);
                    indent();
                    l("return {};\n", ub.slot);
                    dedent();
                    l("}}\n");
                }
            }
            dedent();
            l("}}\n");
        }
    }
    l("return -1;\n");
    dedent();
    l("}}\n");
}

void SokolGenerator::gen_uniformblock_size_refl_func(const GenInput& gen, const Program& prog) {
    const Slang::Enum slang = Slang::first_valid(gen.args.slang);
    l("{}size_t {}{}_uniformblock_size(sg_shader_stage stage, const char* ub_name) {{\n", func_prefix(gen.args), mod_prefix, prog.name);
    indent();
    l("(void)stage; (void)ub_name;\n");
    for (int stage_index = 0; stage_index < 2; stage_index++) {
        const ShaderStageInfo info = get_shader_stage_info(gen, prog, ShaderStage::from_index(stage_index), slang);
        const Reflection& refl = info.source->refl;
        if (!refl.bindings.uniform_blocks.empty()) {
            l("if (SG_SHADERSTAGE_{} == stage) {{\n", pystring::upper(info.stage_name));
            indent();
            for (const UniformBlock& ub: refl.bindings.uniform_blocks) {
                if (ub.slot >= 0) {
                    l("if (0 == strcmp(ub_name, \"{}\")) {{\n", ub.struct_name);
                    indent();
                    l("return sizeof({});\n", struct_name(ub.struct_name));
                    dedent();
                    l("}}\n");
                }
            }
            dedent();
            l("}}\n");
        }
    }
    l("return 0;\n");
    dedent();
    l("}}\n");
}

void SokolGenerator::gen_uniform_offset_refl_func(const GenInput& gen, const Program& prog) {
    const Slang::Enum slang = Slang::first_valid(gen.args.slang);
    l("{}int {}{}_uniform_offset(sg_shader_stage stage, const char* ub_name, const char* u_name) {{\n", func_prefix(gen.args), mod_prefix, prog.name);
    indent();
    l("(void)stage; (void)ub_name; (void)u_name;\n");
    for (int stage_index = 0; stage_index < 2; stage_index++) {
        const ShaderStageInfo info = get_shader_stage_info(gen, prog, ShaderStage::from_index(stage_index), slang);
        const Reflection& refl = info.source->refl;
        if (!refl.bindings.uniform_blocks.empty()) {
            l("if (SG_SHADERSTAGE_{} == stage) {{\n", pystring::upper(info.stage_name));
            indent();
            for (const UniformBlock& ub: refl.bindings.uniform_blocks) {
                if (ub.slot >= 0) {
                    l("if (0 == strcmp(ub_name, \"{}\")) {{\n", ub.struct_name);
                    indent();
                    for (const Uniform& u: ub.uniforms) {
                        l("if (0 == strcmp(u_name, \"{}\")) {{\n", u.name);
                        indent();
                        l("return {};\n", u.offset);
                        dedent();
                        l("}}\n");
                    }
                    dedent();
                    l("}}\n");
                }
            }
            dedent();
            l("}}\n");
        }
    }
    l("return -1;\n");
    dedent();
    l("}}\n");
}

void SokolGenerator::gen_uniform_desc_refl_func(const GenInput& gen, const Program& prog) {
    const Slang::Enum slang = Slang::first_valid(gen.args.slang);
    l("{}sg_shader_uniform_desc {}{}_uniform_desc(sg_shader_stage stage, const char* ub_name, const char* u_name) {{\n", func_prefix(gen.args), mod_prefix, prog.name);
    indent();
    l("(void)stage; (void)ub_name; (void)u_name;\n");
    l("#if defined(__cplusplus)\n");
    l("sg_shader_uniform_desc desc = {{}};\n");
    l("#else\n");
    l("sg_shader_uniform_desc desc = {{0}};\n");
    l("#endif\n");
    for (int stage_index = 0; stage_index < 2; stage_index++) {
        const ShaderStageInfo info = get_shader_stage_info(gen, prog, ShaderStage::from_index(stage_index), slang);
        const Reflection& refl = info.source->refl;
        if (!refl.bindings.uniform_blocks.empty()) {
            l("if (SG_SHADERSTAGE_{} == stage) {{\n", pystring::upper(info.stage_name));
            indent();
            for (const UniformBlock& ub: refl.bindings.uniform_blocks) {
                if (ub.slot >= 0) {
                    l("if (0 == strcmp(ub_name, \"{}\")) {{\n", ub.struct_name);
                    indent();
                    for (const Uniform& u: ub.uniforms) {
                        l("if (0 == strcmp(u_name, \"{}\")) {{\n", u.name);
                        indent();
                        l("desc.name = \"{}\";\n", u.name);
                        l("desc.type = {};\n", uniform_type(u.type));
                        l("desc.array_count = {};\n", u.array_count);
                        l("return desc;\n");
                        dedent();
                        l("}}\n");
                    }
                    dedent();
                    l("}}\n");
                }
            }
            dedent();
            l("}}\n");
        }
    }
    l("return desc;\n");
    dedent();
    l("}}\n");
}

void SokolGenerator::gen_shader_array_start(const GenInput& gen, const std::string& array_name, size_t num_bytes, Slang::Enum slang) {
    if (gen.args.ifdef) {
        l("#if defined({})\n", sokol_define(slang));
    }
    l("static const uint8_t {}[{}] = {{\n", array_name, num_bytes);
}

void SokolGenerator::gen_shader_array_end(const GenInput& gen) {
    l("\n}};\n");
    if (gen.args.ifdef) {
        l("#endif\n");
    }
}

void SokolGenerator::gen_stb_impl_start(const GenInput &gen) {
    if (gen.args.output_format == Format::SOKOL_IMPL) {
        l("#if defined(SOKOL_SHDC_IMPL)\n");
    }
}

void SokolGenerator::gen_stb_impl_end(const GenInput& gen) {
    if (gen.args.output_format == Format::SOKOL_IMPL) {
        l("#endif // SOKOL_SHDC_IMPL");
    }
}

std::string SokolGenerator::shader_bytecode_array_name(const std::string& name, Slang::Enum slang) {
    return fmt::format("{}{}_bytecode_{}", mod_prefix, name, Slang::to_str(slang));
}

std::string SokolGenerator::shader_source_array_name(const std::string& name, Slang::Enum slang) {
    return fmt::format("{}{}_source_{}", mod_prefix, name, Slang::to_str(slang));
}

std::string SokolGenerator::comment_block_start() {
    return "/*";
}

std::string SokolGenerator::comment_block_end() {
    return "*/";
}

std::string SokolGenerator::comment_block_line(const std::string& str) {
    return fmt::format("    {}", str);
}

std::string SokolGenerator::lang_name() {
    return "C";
}

std::string SokolGenerator::get_shader_desc_help(const std::string& prog_name) {
    return fmt::format("{}{}_shader_desc(sg_query_backend());\n", mod_prefix, prog_name);
}

std::string SokolGenerator::uniform_type(Uniform::Type t) {
    switch (t) {
        case Uniform::FLOAT:  return "SG_UNIFORMTYPE_FLOAT";
        case Uniform::FLOAT2: return "SG_UNIFORMTYPE_FLOAT2";
        case Uniform::FLOAT3: return "SG_UNIFORMTYPE_FLOAT3";
        case Uniform::FLOAT4: return "SG_UNIFORMTYPE_FLOAT4";
        case Uniform::INT:    return "SG_UNIFORMTYPE_INT";
        case Uniform::INT2:   return "SG_UNIFORMTYPE_INT2";
        case Uniform::INT3:   return "SG_UNIFORMTYPE_INT3";
        case Uniform::INT4:   return "SG_UNIFORMTYPE_INT4";
        case Uniform::MAT4:   return "SG_UNIFORMTYPE_MAT4";
        default: return "INVALID";
    }
}

std::string SokolGenerator::flattened_uniform_type(Uniform::Type t) {
    switch (t) {
        case Uniform::FLOAT:
        case Uniform::FLOAT2:
        case Uniform::FLOAT3:
        case Uniform::FLOAT4:
        case Uniform::MAT4:
             return "SG_UNIFORMTYPE_FLOAT4";
        case Uniform::INT:
        case Uniform::INT2:
        case Uniform::INT3:
        case Uniform::INT4:
            return "SG_UNIFORMTYPE_INT4";
        default:
            return "INVALID";
    }
}

std::string SokolGenerator::image_type(ImageType::Enum e) {
    switch (e) {
        case ImageType::_2D:     return "SG_IMAGETYPE_2D";
        case ImageType::CUBE:    return "SG_IMAGETYPE_CUBE";
        case ImageType::_3D:     return "SG_IMAGETYPE_3D";
        case ImageType::ARRAY:   return "SG_IMAGETYPE_ARRAY";
        default: return "INVALID";
    }
}

std::string SokolGenerator::image_sample_type(refl::ImageSampleType::Enum e) {
    switch (e) {
        case ImageSampleType::FLOAT: return "SG_IMAGESAMPLETYPE_FLOAT";
        case ImageSampleType::DEPTH: return "SG_IMAGESAMPLETYPE_DEPTH";
        case ImageSampleType::SINT:  return "SG_IMAGESAMPLETYPE_SINT";
        case ImageSampleType::UINT:  return "SG_IMAGESAMPLETYPE_UINT";
        case ImageSampleType::UNFILTERABLE_FLOAT:  return "SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT";
        default: return "INVALID";
    }
}

std::string SokolGenerator::sampler_type(refl::SamplerType::Enum e) {
    switch (e) {
        case SamplerType::FILTERING:     return "SG_SAMPLERTYPE_FILTERING";
        case SamplerType::COMPARISON:    return "SG_SAMPLERTYPE_COMPARISON";
        case SamplerType::NONFILTERING:  return "SG_SAMPLERTYPE_NONFILTERING";
        default: return "INVALID";
    }
}

std::string SokolGenerator::backend(Slang::Enum e) {
    switch (e) {
        case Slang::GLSL410:      return "SG_BACKEND_GLCORE";
        case Slang::GLSL430:      return "SG_BACKEND_GLCORE";
        case Slang::GLSL300ES:    return "SG_BACKEND_GLES3";
        case Slang::HLSL4:        return "SG_BACKEND_D3D11";
        case Slang::HLSL5:        return "SG_BACKEND_D3D11";
        case Slang::METAL_MACOS:  return "SG_BACKEND_METAL_MACOS";
        case Slang::METAL_IOS:    return "SG_BACKEND_METAL_IOS";
        case Slang::METAL_SIM:    return "SG_BACKEND_METAL_SIMULATOR";
        case Slang::WGSL:         return "SG_BACKEND_WGPU";
        default: return "<INVALID>";
    }
}

std::string SokolGenerator::struct_name(const std::string& name) {
    return fmt::format("{}{}_t", mod_prefix, name);
}

std::string SokolGenerator::vertex_attr_name(const std::string& snippet_name, const refl::VertexAttr& attr) {
    return fmt::format("ATTR_{}{}_{}", mod_prefix, snippet_name, attr.name);
}

std::string SokolGenerator::image_bind_slot_name(const refl::Image& img) {
    return fmt::format("SLOT_{}{}", mod_prefix, img.name);
}

std::string SokolGenerator::sampler_bind_slot_name(const refl::Sampler& smp) {
    return fmt::format("SLOT_{}{}", mod_prefix, smp.name);
}

std::string SokolGenerator::uniform_block_bind_slot_name(const refl::UniformBlock& ub) {
    return fmt::format("SLOT_{}{}", mod_prefix, ub.struct_name);
}

std::string SokolGenerator::vertex_attr_definition(const std::string& snippet_name, const refl::VertexAttr& attr) {
    return fmt::format("#define {} ({})", vertex_attr_name(snippet_name, attr), attr.slot);
}

std::string SokolGenerator::image_bind_slot_definition(const refl::Image& img) {
    return fmt::format("#define {} ({})", image_bind_slot_name(img), img.slot);
}

std::string SokolGenerator::sampler_bind_slot_definition(const refl::Sampler& smp) {
    return fmt::format("#define {} ({})", sampler_bind_slot_name(smp), smp.slot);
}

std::string SokolGenerator::uniform_block_bind_slot_definition(const refl::UniformBlock& ub) {
    return fmt::format("#define {} ({})", uniform_block_bind_slot_name(ub), ub.slot);
}

} // namespace
