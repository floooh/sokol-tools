/*
    Generates the final sokol_gfx.h compatible C header with
    embedded shader source/byte code, uniform block structs
    and sg_shader_desc structs.
*/
#include "shdc.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc {

static const spirvcross_source_t* find_spirvcross_source(const spirvcross_t& spirvcross, slang_t::type_t slang, int snippet_index) {
    for (const auto& src: spirvcross.sources[(int)slang]) {
        if (src.snippet_index == snippet_index) {
            return &src;
        }
    }
    return nullptr;
}

static const char* uniform_type_str(uniform_t::type_t type) {
    switch (type) {
        case uniform_t::FLOAT: return "float";
        case uniform_t::FLOAT2: return "vec2";
        case uniform_t::FLOAT3: return "vec3";
        case uniform_t::FLOAT4: return "vec4";
        case uniform_t::MAT4: return "mat4";
        default: return "FIXME";
    }
}

static int uniform_type_size(uniform_t::type_t type) {
    switch (type) {
        case uniform_t::FLOAT:  return 4;
        case uniform_t::FLOAT2: return 8;
        case uniform_t::FLOAT3: return 12;
        case uniform_t::FLOAT4: return 16;
        case uniform_t::MAT4:   return 64;
        default: return 0;
    }
}

static int roundup(int val, int round_to) {
    return (val + (round_to - 1)) & ~(round_to - 1);
}

static bool is_glsl(slang_t::type_t slang) {
    return (slang == slang_t::GLSL330) || (slang == slang_t::GLSL100) || (slang == slang_t::GLSL300ES);
}

static void write_uniform_blocks(FILE* f, const input_t& inp, const spirvcross_refl_t* refl, slang_t::type_t slang) {
    for (const uniform_block_t& ub: refl->uniform_blocks) {
        fmt::print(f, "#pragma pack(push,1)\n");
        int cur_offset = 0;
        fmt::print(f, "typedef struct {} {{\n", ub.name);
        for (const uniform_t& uniform: ub.uniforms) {
            int next_offset = uniform.offset;
            if (next_offset > cur_offset) {
                fmt::print(f, "    uint8_t _pad_{}[{}];\n", cur_offset, next_offset - cur_offset);
            }
            if (inp.type_map.count(uniform_type_str(uniform.type)) > 0) {
                // user-provided type names
                if (uniform.array_count == 1) {
                    fmt::print(f, "    {} {};\n", inp.type_map.at(uniform_type_str(uniform.type)), uniform.name);
                }
                else {
                    fmt::print(f, "    {} {}[{}];\n", inp.type_map.at(uniform_type_str(uniform.type)), uniform.name, uniform.array_count);
                }
            }
            else {
                // default type names (float)
                if (uniform.array_count == 1) {
                    switch (uniform.type) {
                        case uniform_t::FLOAT:   fmt::print(f, "    float {};\n", uniform.name); break;
                        case uniform_t::FLOAT2:  fmt::print(f, "    float {}[2];\n", uniform.name); break;
                        case uniform_t::FLOAT3:  fmt::print(f, "    float {}[3];\n", uniform.name); break;
                        case uniform_t::FLOAT4:  fmt::print(f, "    float {}[4];\n", uniform.name); break;
                        case uniform_t::MAT4:    fmt::print(f, "    float {}[16];\n", uniform.name); break;
                        default:                 fmt::print(f, "    INVALID_UNIFORM_TYPE;\n"); break;
                    }
                }
                else {
                    switch (uniform.type) {
                        case uniform_t::FLOAT:   fmt::print(f, "    float {}[{}];\n", uniform.name, uniform.array_count); break;
                        case uniform_t::FLOAT2:  fmt::print(f, "    float {}[{}][2];\n", uniform.name, uniform.array_count); break;
                        case uniform_t::FLOAT3:  fmt::print(f, "    float {}[{}][3];\n", uniform.name, uniform.array_count); break;
                        case uniform_t::FLOAT4:  fmt::print(f, "    float {}[{}][4];\n", uniform.name, uniform.array_count); break;
                        case uniform_t::MAT4:    fmt::print(f, "    float {}[{}][16];\n", uniform.name, uniform.array_count); break;
                        default:                 fmt::print(f, "    INVALID_UNIFORM_TYPE;\n"); break;
                    }
                }
            }
            cur_offset += uniform_type_size(uniform.type) * uniform.array_count;
        }
        /* on GL, add padding bytes until struct size is multiple of 16 (because the
           uniform block has been flattened into a vec4 array
        */
        if (is_glsl(slang)) {
            const int round16 = roundup(cur_offset, 16);
            if (cur_offset != round16) {
                fmt::print(f, "    uint8_t _pad_{}[{}];\n", cur_offset, round16-cur_offset);
            }
        }
        fmt::print(f, "}} {};\n", ub.name);
        fmt::print(f, "#pragma pack(pop)\n");
    }
}

static const char* img_type_to_sokol_type_str(image_t::type_t type) {
    switch (type) {
        case image_t::IMAGE_2D: return "SG_IMAGETYPE_2D";
        case image_t::IMAGE_CUBE: return "SG_IMAGETYPE_CUBE;";
        case image_t::IMAGE_3D: return "SG_IMAGETYPE_3D";
        case image_t::IMAGE_ARRAY: return "SG_IMAGETYPE_ARRAY";
        default: return "INVALID";
    }
}

static error_t write_program(FILE* f, const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang) {
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;
        int vs_snippet_index = inp.snippet_map.at(prog.vs_name);
        int fs_snippet_index = inp.snippet_map.at(prog.fs_name);
        const spirvcross_source_t* vs_src = find_spirvcross_source(spirvcross, slang, vs_snippet_index);
        const spirvcross_source_t* fs_src = find_spirvcross_source(spirvcross, slang, fs_snippet_index);
        if (!vs_src) {
            return error_t(inp.path, inp.snippets[vs_snippet_index].lines[0],
                fmt::format("no generated '{}' source for vertex shader '{}' in program '{}'",
                    slang_t::to_str(slang), prog.vs_name, prog.name));
        }
        if (!fs_src) {
            return error_t(inp.path, inp.snippets[vs_snippet_index].lines[0],
                fmt::format("no generated '{}' source for fragment shader '{}' in program '{}'",
                    slang_t::to_str(slang), prog.fs_name, prog.name));
        }

        /* write uniform-block structs */
        write_uniform_blocks(f, inp, &vs_src->refl, slang);
        write_uniform_blocks(f, inp, &fs_src->refl, slang);

        /* write function which returns a sg_shader_desc */
        fmt::print(f, "static sg_shader_desc {}_shader_desc(void) {{\n", prog.name);
        fmt::print(f, "    sg_shader_desc desc;\n");
        fmt::print(f, "    memset(&desc, 0, sizeof(desc));\n");
        fmt::print(f, "    desc.label = \"{}_shader\";\n", prog.name);
        fmt::print(f, "    desc.vs.source = {}_{}_{}_src;\n", prog.name, prog.vs_name, slang_t::to_str(slang));
        fmt::print(f, "    desc.vs.entry = \"{}\";\n", vs_src->refl.entry_point);
        for (const uniform_block_t& ub: vs_src->refl.uniform_blocks) {
            fmt::print(f, "    /* uniform block: {} */\n", ub.name);
            fmt::print(f, "    desc.vs.uniform_blocks[{}].size = {};\n", ub.slot, is_glsl(slang) ? roundup(ub.size,16):ub.size);
            /* GLSL uniform blocks are flattened! */
            fmt::print(f, "    desc.vs.uniform_blocks[{}].uniforms[0].name = \"{}\";\n", ub.slot, ub.name);
            fmt::print(f, "    desc.vs.uniform_blocks[{}].uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;\n", ub.slot);
            fmt::print(f, "    desc.vs.uniform_blocks[{}].uniforms[0].array_count = {};\n", ub.slot, roundup(ub.size,16)/16);
        }
        for (const image_t& img: vs_src->refl.images) {
            fmt::print(f, "    desc.vs.images[{}].name = \"{}\";\n", img.slot, img.name);
            fmt::print(f, "    desc.vs.images[{}].type = \"{}\";\n", img.slot, img_type_to_sokol_type_str(img.type));
        }
        for (const uniform_block_t& ub: fs_src->refl.uniform_blocks) {
            fmt::print(f, "    /* uniform block: {} */\n", ub.name);
            fmt::print(f, "    desc.fs.uniform_blocks[{}].size = {};\n", ub.slot, ub.size);
            /* GLSL uniform blocks are flattened! */
            fmt::print(f, "    desc.fs.uniform_blocks[{}].uniforms[0].name = \"{}\";\n", ub.slot, ub.name);
            fmt::print(f, "    desc.fs.uniform_blocks[{}].uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;\n", ub.slot);
            fmt::print(f, "    desc.fs.uniform_blocks[{}].uniforms[0].array_count = {};\n", ub.slot, roundup(ub.size,16)/16);
        }
        for (const image_t& img: fs_src->refl.images) {
            fmt::print(f, "    desc.fs.images[{}].name = \"{}\";\n", img.slot, img.name);
            fmt::print(f, "    desc.fs.images[{}].type = \"{}\";\n", img.slot, img_type_to_sokol_type_str(img.type));
        }
        fmt::print(f, "    return desc;\n");
        fmt::print(f, "}}\n");

        /* write shader sources */
        std::vector<std::string> vs_lines, fs_lines;
        pystring::splitlines(vs_src->source_code, vs_lines);
        pystring::splitlines(fs_src->source_code, fs_lines);
        fmt::print(f, "static const char* {}_{}_{}_src =\n", prog.name, prog.vs_name, slang_t::to_str(slang));
        for (const auto& line: vs_lines) {
            fmt::print(f, "\"{}\\n\"\n", line);
        }
        fmt::print(f, ";\n");
        fmt::print(f, "static const char* {}_{}_{}_src =\n", prog.name, prog.fs_name, slang_t::to_str(slang));
        for (const auto& line: fs_lines) {
            fmt::print(f, "\"{}\\n\"\n", line);
        }
        fmt::print(f, ";\n");
    }
    return error_t();
}

error_t sokol_t::gen(const args_t& args, const input_t& inp, const spirvcross_t& spirvcross, const bytecode_t& bytecode) {
    FILE* f = fopen(args.output.c_str(), "w");
    if (!f) {
        return error_t(inp.path, 0, fmt::format("failed to open output file '{}'", args.output));
    }

    fmt::print(f, "#pragma once\n");
    fmt::print(f, "/* #version:{}# machine generated, don't edit */\n", args.gen_version);
    fmt::print(f, "#include <stdint.h>\n");
    fmt::print(f, "#include <string.h>\n");
    fmt::print(f, "#if !defined(SOKOL_GFX_INCLUDED)\n");
    fmt::print(f, "#error \"Please include sokol_gfx.h before {}\"\n", pystring::os::path::basename(args.output));
    fmt::print(f, "#endif\n");
    if (args.slang & slang_t::bit(slang_t::GLSL330)) {
        if (!args.no_ifdef) { fmt::print(f, "#if defined(SOKOL_GLCORE33)\n"); }
        write_program(f, inp, spirvcross, slang_t::GLSL330);
        if (!args.no_ifdef) { fmt::print(f, "#endif /* SOKOL_GLCORE33 */\n"); }
    }
    if (args.slang & slang_t::bit(slang_t::GLSL300ES)) {
        if (!args.no_ifdef) { fmt::print(f, "#if defined(SOKOL_GLES3)\n"); }
        write_program(f, inp, spirvcross, slang_t::GLSL300ES);
        if (!args.no_ifdef) { fmt::print(f, "#endif /* SOKOL_GLES3 */\n"); }
    }
    if (args.slang & slang_t::bit(slang_t::GLSL100)) {
        if (!args.no_ifdef) { fmt::print(f, "#if defined(SOKOL_GLES2)\n"); }
        write_program(f, inp, spirvcross, slang_t::GLSL100);
        if (!args.no_ifdef) { fmt::print(f, "#endif /* SOKOL_GLES2 */\n"); }
    }
    if (args.slang & slang_t::bit(slang_t::HLSL5)) {
        if (!args.no_ifdef) { fmt::print(f, "#if defined(SOKOL_D3D11)\n"); }
        write_program(f, inp, spirvcross, slang_t::HLSL5);
        if (!args.no_ifdef) { fmt::print(f, "#endif /* SOKOL_D3D11 */\n"); }
    }
    if (args.slang & slang_t::bit(slang_t::METAL_MACOS)) {
        if (!args.no_ifdef) { fmt::print(f, "#if defined(SOKOL_METAL)\n"); }
        write_program(f, inp, spirvcross, slang_t::METAL_MACOS);
        if (!args.no_ifdef) { fmt::print(f, "#endif /* SOKOL_METAL */\n"); }
    }
    if (args.slang & slang_t::bit(slang_t::METAL_IOS)) {
        if (!args.no_ifdef) { fmt::print(f, "#if defined(SOKOL_METAL)\n"); }
        write_program(f, inp, spirvcross, slang_t::METAL_IOS);
        if (!args.no_ifdef) { fmt::print(f, "#endif /* SOKOL_METAL */\n"); }
    }
    fclose(f);
    return error_t();
}

} // namespace shdc
