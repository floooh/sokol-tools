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
    l("#[repr(C, align(16))]\n");
    l_open("pub struct {} {{\n", struct_name(ub.struct_info.name));
    int cur_offset = 0;
    for (const Type& uniform: ub.struct_info.struct_items) {
        int next_offset = uniform.offset;
        if (next_offset > cur_offset) {
            l("_pad_{}: [u8; {}],\n", cur_offset, next_offset - cur_offset);
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
        l("_pad_{}: [u8; {}],\n", cur_offset, round16 - cur_offset);
    }
    l_close("}}\n");
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
            for (int attr_index = 0; attr_index < StageAttr::Num; attr_index++) {
                const StageAttr& attr = prog.vs().inputs[attr_index];
                if (attr.slot >= 0) {
                    if (Slang::is_glsl(slang)) {
                        l("desc.attrs[{}].name = c\"{}\".as_ptr();\n", attr_index, attr.name);
                    } else if (Slang::is_hlsl(slang)) {
                        l("desc.attrs[{}].sem_name = c\"{}\".as_ptr();\n", attr_index, attr.sem_name);
                        l("desc.attrs[{}].sem_index = {};\n", attr_index, attr.sem_index);
                    }
                }
            }
            for (int stage_index = 0; stage_index < ShaderStage::Num; stage_index++) {
                const ShaderStageArrayInfo& info = shader_stage_array_info(gen, prog, ShaderStage::from_index(stage_index), slang);
                const StageReflection& refl = prog.stages[stage_index];
                const std::string dsn = fmt::format("desc.{}", pystring::lower(refl.stage_name));
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
                l("{}.entry = c\"{}\".as_ptr();\n", dsn, refl.entry_point);
                for (int ub_index = 0; ub_index < UniformBlock::Num; ub_index++) {
                    const UniformBlock* ub = refl.bindings.find_uniform_block_by_slot(ub_index);
                    if (ub) {
                        const std::string ubn = fmt::format("{}.uniform_blocks[{}]", dsn, ub_index);
                        l("{}.size = {};\n", ubn, roundup(ub->struct_info.size, 16));
                        l("{}.layout = sg::UniformLayout::Std140;\n", ubn);
                        if (Slang::is_glsl(slang) && (ub->struct_info.struct_items.size() > 0)) {
                            if (ub->flattened) {
                                l("{}.uniforms[0].name = c\"{}\".as_ptr();\n", ubn, ub->struct_info.name);
                                l("{}.uniforms[0]._type = {};\n", ubn, flattened_uniform_type(ub->struct_info.struct_items[0].type));
                                l("{}.uniforms[0].array_count = {};\n", ubn, roundup(ub->struct_info.struct_items[0].size, 16) / 16);
                            } else {
                                for (int u_index = 0; u_index < (int)ub->struct_info.struct_items.size(); u_index++) {
                                    const Type& u = ub->struct_info.struct_items[u_index];
                                    const std::string un = fmt::format("{}.uniforms[{}]", ubn, u_index);
                                    l("{}.name = c\"{}:{}\".as_ptr();\n", un, ub->inst_name, u.name);
                                    l("{}._type = {};\n", un, uniform_type(u.type));
                                    l(".array_count = {};\n", un, u.array_count);
                                }
                            }
                        }
                    }
                }
                for (int img_index = 0; img_index < Image::Num; img_index++) {
                    const Image* img = refl.bindings.find_image_by_slot(img_index);
                    if (img) {
                        const std::string in = fmt::format("{}.images[{}]", dsn, img_index);
                        l("{}.used = true;\n", in);
                        l("{}.multisampled = {};\n", in, img->multisampled ? "true" : "false");
                        l("{}.image_type = {};\n", in, image_type(img->type));
                        l("{}.sample_type = {};\n", in, image_sample_type(img->sample_type));
                    }
                }
                for (int smp_index = 0; smp_index < Sampler::Num; smp_index++) {
                    const Sampler* smp = refl.bindings.find_sampler_by_slot(smp_index);
                    if (smp) {
                        const std::string sn = fmt::format("{}.samplers[{}]", dsn, smp_index);
                        l("{}.used = true;\n", sn);
                        l("{}.sampler_type = {};\n", sn, sampler_type(smp->type));
                    }
                }
                for (int img_smp_index = 0; img_smp_index < ImageSampler::Num; img_smp_index++) {
                    const ImageSampler* img_smp = refl.bindings.find_image_sampler_by_slot(img_smp_index);
                    if (img_smp) {
                        const std::string isn = fmt::format("{}.image_sampler_pairs[{}]", dsn, img_smp_index);
                        l("{}.used = true;\n", isn);
                        l("{}.image_slot = {};\n", isn, refl.bindings.find_image_by_name(img_smp->image_name)->slot);
                        l("{}.sampler_slot = {};\n", isn, refl.bindings.find_sampler_by_name(img_smp->sampler_name)->slot);
                        if (Slang::is_glsl(slang)) {
                            l("{}.glsl_name = c\"{}\".as_ptr();\n", isn, img_smp->name);
                        }
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

void SokolRustGenerator::gen_attr_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    // FIXME
}

void SokolRustGenerator::gen_image_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    // FIXME
}

void SokolRustGenerator::gen_sampler_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    // FIXME
}

void SokolRustGenerator::gen_uniform_block_slot_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    // FIXME
}

void SokolRustGenerator::gen_uniform_block_size_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    // FIXME
}

void SokolRustGenerator::gen_uniform_offset_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    // FIXME
}

void SokolRustGenerator::gen_uniform_desc_refl_func(const GenInput& gen, const ProgramReflection& prog) {
    // FIXME
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
        case Slang::GLSL410:      return "sg::Backend::Glcore";
        case Slang::GLSL430:      return "sg::Backend::Glcore";
        case Slang::GLSL300ES:    return "sg::Backend::Gles3";
        case Slang::HLSL4:        return "sg::Backend::D3d11";
        case Slang::HLSL5:        return "sg::Backend::D3d11";
        case Slang::METAL_MACOS:  return "sg::Backend::MetalMacos";
        case Slang::METAL_IOS:    return "sg::Backend::MetalIos";
        case Slang::METAL_SIM:    return "sg::Backend::MetalSimulator";
        case Slang::WGSL:         return "sg::Backend::Wgpu";
        default: return "INVALID";
    }
}

std::string SokolRustGenerator::struct_name(const std::string& name) {
    return to_pascal_case(name);
}

std::string SokolRustGenerator::vertex_attr_name(const std::string& snippet_name, const StageAttr& attr) {
    return pystring::upper(fmt::format("ATTR_{}_{}", snippet_name, attr.name));
}

std::string SokolRustGenerator::image_bind_slot_name(const Image& img) {
    return pystring::upper(fmt::format("SLOT_{}", img.name));
}

std::string SokolRustGenerator::sampler_bind_slot_name(const Sampler& smp) {
    return pystring::upper(fmt::format("SLOT_{}", smp.name));
}

std::string SokolRustGenerator::uniform_block_bind_slot_name(const UniformBlock& ub) {
    return pystring::upper(fmt::format("SLOT_{}", ub.struct_info.name));
}

std::string SokolRustGenerator::vertex_attr_definition(const std::string& snippet_name, const StageAttr& attr) {
    return fmt::format("pub const {}: usize = {};", vertex_attr_name(snippet_name, attr), attr.slot);
}

std::string SokolRustGenerator::image_bind_slot_definition(const Image& img) {
    return fmt::format("pub const {}: usize = {};", image_bind_slot_name(img), img.slot);
}

std::string SokolRustGenerator::sampler_bind_slot_definition(const Sampler& smp) {
    return fmt::format("pub const {}: usize = {};", sampler_bind_slot_name(smp), smp.slot);
}

std::string SokolRustGenerator::uniform_block_bind_slot_definition(const UniformBlock& ub) {
    return fmt::format("pub const {}: usize = {};", uniform_block_bind_slot_name(ub), ub.slot);
}

} // namespace
