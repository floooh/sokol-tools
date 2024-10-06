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
                if ((snippet.type != Snippet::VS) && (snippet.type != Snippet::FS)) {
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
    l_open("type {}* {{.packed.}} = object\n", struct_name(ub.struct_info.name));
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

void SokolNimGenerator::gen_storage_buffer_decl(const GenInput& gen, const StorageBuffer& sbuf) {
    // blargh, Nim doesn't allow nested types, so we need be creative and recursively
    // generate top-level structs...
    const auto& item = sbuf.struct_info.struct_items[0];
    recurse_unfold_structs(gen, item, item.struct_typename, sbuf.struct_info.align, sbuf.struct_info.size);
}

void SokolNimGenerator::gen_shader_desc_func(const GenInput& gen, const ProgramReflection& prog) {
    l_open("proc {}ShaderDesc*(backend: sg.Backend): sg.ShaderDesc =\n", to_camel_case(prog.name));
    l("result.label = \"{}_shader\"\n", prog.name);
    l_open("case backend:\n");
    for (int i = 0; i < Slang::Num; i++) {
        Slang::Enum slang = Slang::from_index(i);
        if (gen.args.slang & Slang::bit(slang)) {
            l_open("of {}:\n", backend(slang));
            for (int attr_index = 0; attr_index < StageAttr::Num; attr_index++) {
                const StageAttr& attr = prog.vs().inputs[attr_index];
                if (attr.slot >= 0) {
                    if (Slang::is_glsl(slang)) {
                        l("result.attrs[{}].name = \"{}\"\n", attr_index, attr.name);
                    } else if (Slang::is_hlsl(slang)) {
                        l("result.attrs[{}].semName = \"{}\"\n", attr_index, attr.sem_name);
                        l("result.attrs[{}].semIndex = {}\n", attr_index, attr.sem_index);
                    }
                }
            }
            for (int stage_index = 0; stage_index < ShaderStage::Num; stage_index++) {
                const ShaderStageArrayInfo& info = shader_stage_array_info(gen, prog, ShaderStage::from_index(stage_index), slang);
                const StageReflection& refl = prog.stages[stage_index];
                const std::string dsn = fmt::format("result.{}", pystring::lower(ShaderStage::to_str(refl.stage)));
                if (info.has_bytecode) {
                    l("{}.bytecode.ptr = {}\n", dsn, info.bytecode_array_name);
                    l("{}.bytecode.size = {}\n", dsn, info.bytecode_array_size);
                } else {
                    l("{}.source = cast[cstring](addr({}))\n", dsn, info.source_array_name);
                    const char* d3d11_tgt = nullptr;
                    if (slang == Slang::HLSL4) {
                        d3d11_tgt = (0 == stage_index) ? "vs_4_0" : "ps_4_0";
                    } else if (slang == Slang::HLSL5) {
                        d3d11_tgt = (0 == stage_index) ? "vs_5_0" : "ps_5_0";
                    }
                    if (d3d11_tgt) {
                        l("{}.d3d11Target = \"{}\"\n", dsn, d3d11_tgt);
                    }
                }
                l("{}.entry = \"{}\"\n", dsn, refl.entry_point_by_slang(slang));
                for (int ub_index = 0; ub_index < Bindings::MaxUniformBlocks; ub_index++) {
                    const UniformBlock* ub = refl.bindings.find_uniform_block_by_sokol_slot(ub_index);
                    if (ub) {
                        const std::string ubn = fmt::format("{}.uniformBlocks[{}]", dsn, ub_index);
                        l("{}.size = {}\n", ubn, roundup(ub->struct_info.size, 16));
                        l("{}.layout = uniformLayoutStd140\n", ubn);
                        if (Slang::is_glsl(slang) && (ub->struct_info.struct_items.size() > 0)) {
                            if (ub->flattened) {
                                l("{}.uniforms[0].name = \"{}\"\n", ubn, ub->struct_info.name);
                                // NOT A BUG (to take the type from the first struct item, but the size from the toplevel ub)
                                l("{}.uniforms[0].type = {}\n", ubn, flattened_uniform_type(ub->struct_info.struct_items[0].type));
                                l("{}.uniforms[0].arrayCount = {}\n", ubn, roundup(ub->struct_info.size, 16) / 16);
                            } else {
                                for (int u_index = 0; u_index < (int)ub->struct_info.struct_items.size(); u_index++) {
                                    const Type& u = ub->struct_info.struct_items[u_index];
                                    const std::string un = fmt::format("{}.uniforms[{}]", ubn, u_index);
                                    l("{}.name = \"{}.{}\"\n", un, ub->inst_name, u.name);
                                    l("{}.type = {}\n", un, uniform_type(u.type));
                                    l(".arrayCount = {}\n", un, u.array_count);
                                }
                            }
                        }
                    }
                }
                for (int sbuf_index = 0; sbuf_index < Bindings::MaxStorageBuffers; sbuf_index++) {
                    const StorageBuffer* sbuf = refl.bindings.find_storage_buffer_by_sokol_slot(sbuf_index);
                    if (sbuf) {
                        const std::string& sbn = fmt::format("{}.storageBuffers[{}]", dsn, sbuf_index);
                        l("{}.used = true\n", sbn);
                        l("{}.readonly = {}\n", sbn, sbuf->readonly);
                    }
                }
                for (int img_index = 0; img_index < Bindings::MaxImages; img_index++) {
                    const Image* img = refl.bindings.find_image_by_sokol_slot(img_index);
                    if (img) {
                        const std::string in = fmt::format("{}.images[{}]", dsn, img_index);
                        l("{}.used = true\n", in);
                        l("{}.multisampled = {}\n", in, img->multisampled ? "true" : "false");
                        l("{}.imageType = {}\n", in, image_type(img->type));
                        l("{}.sampleType = {}\n", in, image_sample_type(img->sample_type));
                    }
                }
                for (int smp_index = 0; smp_index < Bindings::MaxSamplers; smp_index++) {
                    const Sampler* smp = refl.bindings.find_sampler_by_sokol_slot(smp_index);
                    if (smp) {
                        const std::string sn = fmt::format("{}.samplers[{}]", dsn, smp_index);
                        l("{}.used = true\n", sn);
                        l("{}.samplerType = {}\n", sn, sampler_type(smp->type));
                    }
                }
                for (int img_smp_index = 0; img_smp_index < Bindings::MaxImageSamplers; img_smp_index++) {
                    const ImageSampler* img_smp = refl.bindings.find_image_sampler_by_sokol_slot(img_smp_index);
                    if (img_smp) {
                        const std::string isn = fmt::format("{}.imageSamplerPairs[{}]", dsn, img_smp_index);
                        l("{}.used = true\n", isn);
                        l("{}.imageSlot = {}\n", isn, refl.bindings.find_image_by_name(img_smp->image_name)->sokol_slot);
                        l("{}.samplerSlot = {}\n", isn, refl.bindings.find_sampler_by_name(img_smp->sampler_name)->sokol_slot);
                        if (Slang::is_glsl(slang)) {
                            l("{}.glslName = \"{}\"\n", isn, img_smp->name);
                        }
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

std::string SokolNimGenerator::backend(Slang::Enum e) {
    switch (e) {
        case Slang::GLSL410:
        case Slang::GLSL430:
            return "backendGlcore";
        case Slang::GLSL300ES:
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
            return "backendWgsl";
        default:
            return "<INVALID>";
    }
}

std::string SokolNimGenerator::struct_name(const std::string& name) {
    return to_pascal_case(name);
}

std::string SokolNimGenerator::vertex_attr_name(const StageAttr& attr) {
    return to_camel_case(fmt::format("ATTR_{}_{}", attr.snippet_name, attr.name));
}

std::string SokolNimGenerator::image_bind_slot_name(const std::string& prog_name, const Image& img) {
    return to_camel_case(fmt::format("IMG_{}_{}", prog_name, img.name));
}

std::string SokolNimGenerator::sampler_bind_slot_name(const std::string& prog_name, const Sampler& smp) {
    return to_camel_case(fmt::format("SMP_{}_{}", prog_name, smp.name));
}

std::string SokolNimGenerator::uniform_block_bind_slot_name(const std::string& prog_name, const UniformBlock& ub) {
    return to_camel_case(fmt::format("UB_{}_{}", prog_name, ub.struct_info.name));
}

std::string SokolNimGenerator::storage_buffer_bind_slot_name(const std::string& prog_name, const StorageBuffer& sbuf) {
    return to_camel_case(fmt::format("SBUF_{}_{}", prog_name, sbuf.struct_info.name));
}

std::string SokolNimGenerator::vertex_attr_definition(const StageAttr& attr) {
    return fmt::format("const {}* = {}", vertex_attr_name(attr), attr.slot);
}

std::string SokolNimGenerator::image_bind_slot_definition(const std::string& prog_name, const Image& img) {
    return fmt::format("const {}* = {}", image_bind_slot_name(prog_name, img), img.sokol_slot);
}

std::string SokolNimGenerator::sampler_bind_slot_definition(const std::string& prog_name, const Sampler& smp) {
    return fmt::format("const {}* = {}", sampler_bind_slot_name(prog_name, smp), smp.sokol_slot);
}

std::string SokolNimGenerator::uniform_block_bind_slot_definition(const std::string& prog_name, const UniformBlock& ub) {
    return fmt::format("const {}* = {}", uniform_block_bind_slot_name(prog_name, ub), ub.sokol_slot);
}

std::string SokolNimGenerator::storage_buffer_bind_slot_definition(const std::string& prog_name, const StorageBuffer& sbuf) {
    return fmt::format("const {}* = {}", storage_buffer_bind_slot_name(prog_name, sbuf), sbuf.sokol_slot);
}

} // namespace
