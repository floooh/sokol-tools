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

static const int NUM_VERTEX_ATTRS = 16;         /* SG_MAX_VERTEX_ATTRIBUTES */
static const int NUM_UNIFORM_BLOCKS = 4;        /* SG_MAX_SHADERSTAGE_UBS */
static const int NUM_IMAGES = 12;               /* SG_MAX_SHADERSTAGE_IMAGES */
static const int NUM_UNIFORMBLOCK_MEMBERS = 16; /* SG_MAX_UB_MEMBERS */

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

static std::string lib_prefix(const input_t& inp) {
    if (inp.lib.empty()) {
        return "";
    }
    else {
        return fmt::format("{}_", inp.lib);
    }
}

static void write_bind_slots(FILE* f, const input_t& inp, const spirvcross_refl_t* refl, slang_t::type_t slang) {
    for (const uniform_block_t& ub: refl->uniform_blocks) {
        fmt::print(f, "static const int {}{}_slot = {};\n", lib_prefix(inp), ub.name, ub.slot);
    }
    for (const image_t& img: refl->images) {
        fmt::print(f, "static const int {}{}_slot = {};\n", lib_prefix(inp), img.name, img.slot);
    }
}

static void write_uniform_blocks(FILE* f, const input_t& inp, const spirvcross_refl_t* refl, slang_t::type_t slang) {
    for (const uniform_block_t& ub: refl->uniform_blocks) {
        fmt::print(f, "#pragma pack(push,1)\n");
        int cur_offset = 0;
        fmt::print(f, "typedef struct {}{}_t {{\n", lib_prefix(inp), ub.name);
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
        fmt::print(f, "}} {}_t;\n", ub.name);
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

static const attr_t* find_vertex_attr(const spirvcross_refl_t& refl, int slot) {
    for (const attr_t& attr: refl.attrs) {
        if (attr.slot == slot) {
            return &attr;
        }
    }
    return nullptr;
}

static const uniform_block_t* find_uniform_block(const spirvcross_refl_t& refl, int slot) {
    for (const uniform_block_t& ub: refl.uniform_blocks) {
        if (ub.slot == slot) {
            return &ub;
        }
    }
    return nullptr;
}

static const image_t* find_image(const spirvcross_refl_t& refl, int slot) {
    for (const image_t& img: refl.images) {
        if (img.slot == slot) {
            return &img;
        }
    }
    return nullptr;
}

static void write_stage(FILE* f, const char* stage_name, const program_t& prog, const spirvcross_source_t* src, slang_t::type_t slang) {
    fmt::print(f, "  {{ /* {} */\n", stage_name);
    std::vector<std::string> lines;
    pystring::splitlines(src->source_code, lines);
    for (const auto& line: lines) {
        fmt::print(f, "    \"{}\\n\"\n", line);
    }
    fmt::print(f, "    ,\n");
    fmt::print(f, "    0, /* bytecode */\n");
    fmt::print(f, "    0, /* bytecode_size */\n");
    fmt::print(f, "    \"{}\", /* entry */\n", src->refl.entry_point);
    fmt::print(f, "    {{ /* uniform blocks */\n");
    for (int ub_index = 0; ub_index < NUM_UNIFORM_BLOCKS; ub_index++) {
        const uniform_block_t* ub = find_uniform_block(src->refl, ub_index);
        fmt::print(f, "      {{\n");
        if (ub) {
            fmt::print(f, "        {}, /* size */\n", is_glsl(slang) ? roundup(ub->size,16):ub->size);
            fmt::print(f, "        {{ /* uniforms */");
            for (int u_index = 0; u_index < NUM_UNIFORMBLOCK_MEMBERS; u_index++) {
                if (0 == u_index) {
                    fmt::print(f, "{{\"{}\",SG_UNIFORMTYPE_FLOAT4,{}}},", ub->name, roundup(ub->size,16)/16);
                }
                else {
                    fmt::print(f, "{{0,0,0}},");
                }
            }
            fmt::print(f, " }},\n");
        }
        else {
            fmt::print(f, "        0, /* size */\n");
            fmt::print(f, "        {{ /* uniforms */");
            for (int u_index = 0; u_index < NUM_UNIFORMBLOCK_MEMBERS; u_index++) {
                fmt::print(f, "{{0,0,0}},");
            }
            fmt::print(f, " }},\n");
        }
        fmt::print(f, "      }},\n");
    }
    fmt::print(f, "    }},\n");
    fmt::print(f, "    {{ /* images */ ");
    for (int img_index = 0; img_index < NUM_IMAGES; img_index++) {
        const image_t* img = find_image(src->refl, img_index);
        if (img) {
            fmt::print(f, "{{\"{}\",{}}},", img->name, img_type_to_sokol_type_str(img->type));
        }
        else {
            fmt::print(f, "{{0,0}},");
        }
    }
    fmt::print(f, " }},\n");
    fmt::print(f, "  }},\n");
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

        /* write buffer and texture bind slot constants */
        write_bind_slots(f, inp, &vs_src->refl, slang);
        write_bind_slots(f, inp, &fs_src->refl, slang);

        /* write uniform-block structs */
        write_uniform_blocks(f, inp, &vs_src->refl, slang);
        write_uniform_blocks(f, inp, &fs_src->refl, slang);

        /* write shader desc */
        fmt::print(f, "static sg_shader_desc {}{}_shader_desc = {{\n", lib_prefix(inp), prog.name);
        fmt::print(f, "  0, /* _start_canary */\n");
        fmt::print(f, "  {{ /*attrs*/");
        for (int attr_index = 0; attr_index < NUM_VERTEX_ATTRS; attr_index++) {
            const attr_t* attr = find_vertex_attr(vs_src->refl, attr_index);
            if (attr) {
                fmt::print(f, "{{\"{}\",\"{}\",{}}},", attr->name, attr->sem_name, attr->sem_index);
            }
            else {
                fmt::print(f, "{{0,0,0}},");
            }
        }
        fmt::print(f, " }},\n");
        write_stage(f, "vs", prog, vs_src, slang);
        write_stage(f, "fs", prog, fs_src, slang);
        fmt::print(f, "  \"{}_shader\", /* label */\n", prog.name);
        fmt::print(f, "  0, /* _end_canary */\n");
        fmt::print(f, "}};\n");
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
