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
    l_open("struct {} {{\n", struct_name(ub.struct_info.name));
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

void SokolDGenerator::gen_storage_buffer_decl(const GenInput& gen, const StorageBuffer& sbuf) {
    const auto& item = sbuf.struct_info.struct_items[0];
    l_open("struct {} {{\n", struct_name(item.struct_typename));
    gen_struct_interior_decl_std430(gen, item, sbuf.struct_info.align, sbuf.struct_info.size);
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
            for (int attr_index = 0; attr_index < StageAttr::Num; attr_index++) {
                const StageAttr& attr = prog.vs().inputs[attr_index];
                if (attr.slot >= 0) {
                    if (Slang::is_glsl(slang)) {
                        l("desc.attrs[{}].name = \"{}\";\n", attr_index, attr.name);
                    } else if (Slang::is_hlsl(slang)) {
                        l("desc.attrs[{}].sem_name = \"{}\";\n", attr_index, attr.sem_name);
                        l("desc.attrs[{}].sem_index = {};\n", attr_index, attr.sem_index);
                    }
                }
            }
            for (int stage_index = 0; stage_index < ShaderStage::Num; stage_index++) {
                const ShaderStageArrayInfo& info = shader_stage_array_info(gen, prog, ShaderStage::from_index(stage_index), slang);
                const StageReflection& refl = prog.stages[stage_index];
                const std::string dsn = fmt::format("desc.{}", pystring::lower(refl.stage_name));
                if (info.has_bytecode) {
                    l("{}.bytecode.ptr = {}.ptr;\n", dsn, info.bytecode_array_name);
                    l("{}.bytecode.size = {};\n", dsn, info.bytecode_array_size);
                } else {
                    l("{}.source = &{}[0];\n", dsn, info.source_array_name);
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
                for (int ub_index = 0; ub_index < Bindings::MaxUniformBlocks; ub_index++) {
                    const UniformBlock* ub = refl.bindings.find_uniform_block_by_slot(ub_index);
                    if (ub) {
                        const std::string ubn = fmt::format("{}.uniform_blocks[{}]", dsn, ub_index);
                        l("{}.size = {};\n", ubn, roundup(ub->struct_info.size, 16));
                        l("{}.layout = sg.UniformLayout.Std140;\n", ubn);
                        if (Slang::is_glsl(slang) && (ub->struct_info.struct_items.size() > 0)) {
                            if (ub->flattened) {
                                l("{}.uniforms[0].name = \"{}\";\n", ubn, ub->struct_info.name);
                                // NOT A BUG (to take the type from the first struct item, but the size from the toplevel ub)
                                l("{}.uniforms[0].type = {};\n", ubn, flattened_uniform_type(ub->struct_info.struct_items[0].type));
                                l("{}.uniforms[0].array_count = {};\n", ubn, roundup(ub->struct_info.size, 16) / 16);
                            } else {
                                for (int u_index = 0; u_index < (int)ub->struct_info.struct_items.size(); u_index++) {
                                    const Type& u = ub->struct_info.struct_items[u_index];
                                    const std::string un = fmt::format("{}.uniforms[{}]", ubn, u_index);
                                    l("{}.name = \"{}.{}\";\n", un, ub->inst_name, u.name);
                                    l("{}.type = {};\n", un, uniform_type(u.type));
                                    l("{}.array_count = {};\n", un, u.array_count);
                                }
                            }
                        }
                    }
                }
                for (int sbuf_index = 0; sbuf_index < Bindings::MaxStorageBuffers; sbuf_index++) {
                    const StorageBuffer* sbuf = refl.bindings.find_storage_buffer_by_slot(sbuf_index);
                    if (sbuf) {
                        const std::string& sbn = fmt::format("{}.storage_buffers[{}]", dsn, sbuf_index);
                        l("{}.used = true;\n", sbn);
                        l("{}.readonly = {};\n", sbn, sbuf->readonly);
                    }
                }
                for (int img_index = 0; img_index < Bindings::MaxImages; img_index++) {
                    const Image* img = refl.bindings.find_image_by_slot(img_index);
                    if (img) {
                        const std::string in = fmt::format("{}.images[{}]", dsn, img_index);
                        l("{}.used = true;\n", in);
                        l("{}.multisampled = {};\n", in, img->multisampled ? "true" : "false");
                        l("{}.image_type = {};\n", in, image_type(img->type));
                        l("{}.sample_type = {};\n", in, image_sample_type(img->sample_type));
                    }
                }
                for (int smp_index = 0; smp_index < Bindings::MaxSamplers; smp_index++) {
                    const Sampler* smp = refl.bindings.find_sampler_by_slot(smp_index);
                    if (smp) {
                        const std::string sn = fmt::format("{}.samplers[{}]", dsn, smp_index);
                        l("{}.used = true;\n", sn);
                        l("{}.sampler_type = {};\n", sn, sampler_type(smp->type));
                    }
                }
                for (int img_smp_index = 0; img_smp_index < Bindings::MaxImageSamplers; img_smp_index++) {
                    const ImageSampler* img_smp = refl.bindings.find_image_sampler_by_slot(img_smp_index);
                    if (img_smp) {
                        const std::string isn = fmt::format("{}.image_sampler_pairs[{}]", dsn, img_smp_index);
                        l("{}.used = true;\n", isn);
                        l("{}.image_slot = {};\n", isn, refl.bindings.find_image_by_name(img_smp->image_name)->slot);
                        l("{}.sampler_slot = {};\n", isn, refl.bindings.find_sampler_by_name(img_smp->sampler_name)->slot);
                        if (Slang::is_glsl(slang)) {
                            l("{}.glsl_name = \"{}\";\n", isn, img_smp->name);
                        }
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

std::string SokolDGenerator::backend(Slang::Enum e) {
    switch (e) {
        case Slang::GLSL410:
        case Slang::GLSL430:
            return "sg.Backend.Glcore";
        case Slang::GLSL300ES:
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

std::string SokolDGenerator::vertex_attr_name(const StageAttr& attr) {
    return pystring::upper(fmt::format("ATTR_{}_{}", attr.snippet_name, attr.name));
}

std::string SokolDGenerator::image_bind_slot_name(const std::string& prog_name, const Image& img) {
    return pystring::upper(fmt::format("IMG_{}_{}", prog_name, img.name));
}

std::string SokolDGenerator::sampler_bind_slot_name(const std::string& prog_name, const Sampler& smp) {
    return pystring::upper(fmt::format("SMP_{}_{}", prog_name, smp.name));
}

std::string SokolDGenerator::uniform_block_bind_slot_name(const std::string& prog_name, const UniformBlock& ub) {
    return pystring::upper(fmt::format("UB_{}_{}", prog_name, ub.struct_info.name));
}

std::string SokolDGenerator::storage_buffer_bind_slot_name(const std::string& prog_name, const StorageBuffer& sbuf) {
    return pystring::upper(fmt::format("SBUF_{}_{}", prog_name, sbuf.struct_info.name));
}

static std::string const_def(const std::string& name, int slot) {
    return fmt::format("enum {} = {};", name, slot);
}

std::string SokolDGenerator::vertex_attr_definition(const StageAttr& attr) {
    return const_def(vertex_attr_name(attr), attr.slot);
}

std::string SokolDGenerator::image_bind_slot_definition(const std::string& prog_name, const Image& img) {
    return const_def(image_bind_slot_name(prog_name, img), img.slot);
}

std::string SokolDGenerator::sampler_bind_slot_definition(const std::string& prog_name, const Sampler& smp) {
    return const_def(sampler_bind_slot_name(prog_name, smp), smp.slot);
}

std::string SokolDGenerator::uniform_block_bind_slot_definition(const std::string& prog_name, const UniformBlock& ub) {
    return const_def(uniform_block_bind_slot_name(prog_name, ub), ub.slot);
}

std::string SokolDGenerator::storage_buffer_bind_slot_definition(const std::string& prog_name, const StorageBuffer& sbuf) {
    return const_def(storage_buffer_bind_slot_name(prog_name, sbuf), sbuf.slot);
}

} // namespace
