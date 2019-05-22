/*
    Generate the output header for sokol_gfx.h
*/
#include "shdc.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc {

static std::string file_content;

#if defined(_MSC_VER)
#define L(str, ...) file_content.append(fmt::format(str, __VA_ARGS__))
#else
#define L(str, ...) file_content.append(fmt::format(str, ##__VA_ARGS__))
#endif

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

static std::string mod_prefix(const input_t& inp) {
    if (inp.module.empty()) {
        return "";
    }
    else {
        return fmt::format("{}_", inp.module);
    }
}

static const char* img_type_to_sokol_type_str(image_t::type_t type) {
    switch (type) {
        case image_t::IMAGE_2D: return "SG_IMAGETYPE_2D";
        case image_t::IMAGE_CUBE: return "SG_IMAGETYPE_CUBE";
        case image_t::IMAGE_3D: return "SG_IMAGETYPE_3D";
        case image_t::IMAGE_ARRAY: return "SG_IMAGETYPE_ARRAY";
        default: return "INVALID";
    }
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

static const char* sokol_define(slang_t::type_t slang) {
    switch (slang) {
        case slang_t::GLSL330:      return "SOKOL_GLCORE33";
        case slang_t::GLSL100:      return "SOKOL_GLES2";
        case slang_t::GLSL300ES:    return "SOKOL_GLES3";
        case slang_t::HLSL5:        return "SOKOL_HLSL5";
        case slang_t::METAL_MACOS:  return "SOKOL_METAL";
        case slang_t::METAL_IOS:    return "SOKOL_METAL";
        default: return "<INVALID>";
    }
}

static const char* sokol_backend(slang_t::type_t slang) {
    switch (slang) {
        case slang_t::GLSL330:      return "SG_BACKEND_GLCORE33";
        case slang_t::GLSL100:      return "SG_BACKEND_GLES2";
        case slang_t::GLSL300ES:    return "SG_BACKEND_GLES3";
        case slang_t::HLSL5:        return "SG_BACKEND_D3D11";
        case slang_t::METAL_MACOS:  return "SG_BACKEND_METAL_MACOS";
        case slang_t::METAL_IOS:    return "SG_BACKEND_METAL_IOS";
        default: return "<INVALID>";
    }
}

static void write_header(const args_t& args, const input_t& inp, const spirvcross_t& spirvcross) {
    L("/*\n");
    L("    #version:{}# (machine generated, don't edit!)\n\n", args.gen_version);
    L("    Generated by sokol-shdc (https://github.com/floooh/sokol-tools)\n\n");
    L("    Overview:\n\n");
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;

        int vs_snippet_index = inp.snippet_map.at(prog.vs_name);
        int fs_snippet_index = inp.snippet_map.at(prog.fs_name);
        int vs_src_index = spirvcross.find_source_by_snippet_index(vs_snippet_index);
        int fs_src_index = spirvcross.find_source_by_snippet_index(fs_snippet_index);
        assert((vs_src_index >= 0) && (fs_src_index >= 0));
        const spirvcross_source_t& vs_src = spirvcross.sources[vs_src_index];
        const spirvcross_source_t& fs_src = spirvcross.sources[fs_src_index];
        L("        Shader program '{}':\n", prog.name);
        L("            Get shader desc: {}{}_shader_desc()\n", mod_prefix(inp), prog.name);
        L("            Vertex shader: {}\n", prog.vs_name);
        L("                Attribute slots:\n");
        const snippet_t& vs_snippet = inp.snippets[vs_src.snippet_index];
        for (const attr_t& attr: vs_src.refl.inputs) {
            if (attr.slot >= 0) {
                L("                    ATTR_{}{}_{} = {}\n", mod_prefix(inp), vs_snippet.name, attr.name, attr.slot);
            }
        }
        for (const uniform_block_t& ub: vs_src.refl.uniform_blocks) {
            L("                Uniform block '{}':\n", ub.name);
            L("                    C struct: {}{}_t\n", mod_prefix(inp), ub.name);
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), ub.name, ub.slot);
        }
        for (const image_t& img: vs_src.refl.images) {
            L("                Image '{}':\n", img.name);
            L("                    Type: {}\n", img_type_to_sokol_type_str(img.type));
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), img.name, img.slot);
        }
        L("            Fragment shader: {}\n", prog.fs_name);
        for (const uniform_block_t& ub: fs_src.refl.uniform_blocks) {
            L("                Uniform block '{}':\n", ub.name);
            L("                    C struct: {}{}_t\n", mod_prefix(inp), ub.name);
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), ub.name, ub.slot);
        }
        for (const image_t& img: fs_src.refl.images) {
            L("                Image '{}':\n", img.name);
            L("                    Type: {}\n", img_type_to_sokol_type_str(img.type));
            L("                    Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), img.name, img.slot);
        }
        L("\n");
    }
    L("\n");
    L("    Shader descriptor structs:\n\n");
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;
        L("        sg_shader {} = sg_make_shader({}{}_shader_desc());\n", prog.name, mod_prefix(inp), prog.name);
    }
    L("\n");
    for (const spirvcross_source_t& src: spirvcross.sources) {
        if (src.refl.stage == stage_t::VS) {
            const snippet_t& vs_snippet = inp.snippets[src.snippet_index];
            L("    Vertex attribute locations for vertex shader '{}':\n\n", vs_snippet.name);
            L("        sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){{\n");
            L("            .layout = {{\n");
            L("                .attrs = {{\n");
            for (const attr_t& attr: src.refl.inputs) {
                if (attr.slot >= 0) {
                    L("                    [ATTR_{}{}_{}] = {{ ... }},\n", mod_prefix(inp), vs_snippet.name, attr.name);
                }
            }
            L("                }},\n");
            L("            }},\n");
            L("            ...}});\n");
            L("\n");
        }
    }
    L("    Image bind slots, use as index in sg_bindings.vs_images[] or .fs_images[]\n\n");
    for (const image_t& img: spirvcross.unique_images) {
        L("        SLOT_{}{} = {};\n", mod_prefix(inp), img.name, img.slot);
    }
    L("\n");
    for (const uniform_block_t& ub: spirvcross.unique_uniform_blocks) {
        L("    Bind slot and C-struct for uniform block '{}':\n\n", ub.name);
        L("        {}{}_t {} = {{\n", mod_prefix(inp), ub.name, ub.name);
        for (const uniform_t& uniform: ub.uniforms) {
            L("            .{} = ...;\n", uniform.name);
        };
        L("        }};\n");
        L("        sg_apply_uniforms(SG_SHADERSTAGE_[VS|FS], SLOT_{}{}, &{}, sizeof({}));\n", mod_prefix(inp), ub.name, ub.name, ub.name);
        L("\n");
    }
    L("*/\n");
    L("#if !defined(SOKOL_GFX_INCLUDED)\n");
    L("#error \"Please include sokol_gfx.h before {}\"\n", pystring::os::path::basename(args.output));
    L("#endif\n");
}

static void write_vertex_attrs(const input_t& inp, const spirvcross_t& spirvcross) {
    // vertex attributes
    for (const spirvcross_source_t& src: spirvcross.sources) {
        if (src.refl.stage == stage_t::VS) {
            const snippet_t& vs_snippet = inp.snippets[src.snippet_index];
            for (const attr_t& attr: src.refl.inputs) {
                if (attr.slot >= 0) {
                    L("#define ATTR_{}{}_{} ({})\n", mod_prefix(inp), vs_snippet.name, attr.name, attr.slot);
                }
            }
        }
    }
}

static void write_images_bind_slots(const input_t& inp, const spirvcross_t& spirvcross) {
    for (const image_t& img: spirvcross.unique_images) {
        L("#define SLOT_{}{} ({})\n", mod_prefix(inp), img.name, img.slot);
    }
}

static void write_uniform_blocks(const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang) {
    for (const uniform_block_t& ub: spirvcross.unique_uniform_blocks) {
        L("#define SLOT_{}{} ({})\n", mod_prefix(inp), ub.name, ub.slot);
        L("#pragma pack(push,1)\n");
        int cur_offset = 0;
        L("typedef struct {}{}_t {{\n", mod_prefix(inp), ub.name);
        for (const uniform_t& uniform: ub.uniforms) {
            int next_offset = uniform.offset;
            if (next_offset > cur_offset) {
                L("    uint8_t _pad_{}[{}];\n", cur_offset, next_offset - cur_offset);
                cur_offset = next_offset;
            }
            if (inp.type_map.count(uniform_type_str(uniform.type)) > 0) {
                // user-provided type names
                if (uniform.array_count == 1) {
                    L("    {} {};\n", inp.type_map.at(uniform_type_str(uniform.type)), uniform.name);
                }
                else {
                    L("    {} {}[{}];\n", inp.type_map.at(uniform_type_str(uniform.type)), uniform.name, uniform.array_count);
                }
            }
            else {
                // default type names (float)
                if (uniform.array_count == 1) {
                    switch (uniform.type) {
                        case uniform_t::FLOAT:   L("    float {};\n", uniform.name); break;
                        case uniform_t::FLOAT2:  L("    float {}[2];\n", uniform.name); break;
                        case uniform_t::FLOAT3:  L("    float {}[3];\n", uniform.name); break;
                        case uniform_t::FLOAT4:  L("    float {}[4];\n", uniform.name); break;
                        case uniform_t::MAT4:    L("    float {}[16];\n", uniform.name); break;
                        default:                 L("    INVALID_UNIFORM_TYPE;\n"); break;
                    }
                }
                else {
                    switch (uniform.type) {
                        case uniform_t::FLOAT:   L("    float {}[{}];\n", uniform.name, uniform.array_count); break;
                        case uniform_t::FLOAT2:  L("    float {}[{}][2];\n", uniform.name, uniform.array_count); break;
                        case uniform_t::FLOAT3:  L("    float {}[{}][3];\n", uniform.name, uniform.array_count); break;
                        case uniform_t::FLOAT4:  L("    float {}[{}][4];\n", uniform.name, uniform.array_count); break;
                        case uniform_t::MAT4:    L("    float {}[{}][16];\n", uniform.name, uniform.array_count); break;
                        default:                 L("    INVALID_UNIFORM_TYPE;\n"); break;
                    }
                }
            }
            cur_offset += uniform_type_size(uniform.type) * uniform.array_count;
        }
        /* pad to multiple of 16-bytes struct size */
        const int round16 = roundup(cur_offset, 16);
        if (cur_offset != round16) {
            L("    uint8_t _pad_{}[{}];\n", cur_offset, round16-cur_offset);
        }
        L("}} {}{}_t;\n", mod_prefix(inp), ub.name);
        L("#pragma pack(pop)\n");
    }
}

static void write_stage(const char* stage_name, const program_t& prog, const spirvcross_source_t& src, slang_t::type_t slang) {
    L("  {{ /* {} */\n", stage_name);
    std::vector<std::string> lines;
    pystring::splitlines(src.source_code, lines);
    for (std::string line: lines) {
        // escape special characters
        line = pystring::replace(line, "\"", "\\\"");
        L("    \"{}\\n\"\n", line);
    }
    L("    ,\n");
    L("    0, /* bytecode */\n");
    L("    0, /* bytecode_size */\n");
    L("    \"{}\", /* entry */\n", src.refl.entry_point);
    L("    {{ /* uniform blocks */\n");
    for (int ub_index = 0; ub_index < uniform_block_t::NUM; ub_index++) {
        const uniform_block_t* ub = find_uniform_block(src.refl, ub_index);
        L("      {{\n");
        if (ub) {
            L("        {}, /* size */\n", roundup(ub->size,16));
            L("        {{ /* uniforms */");
            for (int u_index = 0; u_index < uniform_t::NUM; u_index++) {
                if (0 == u_index) {
                    L("{{\"{}\",SG_UNIFORMTYPE_FLOAT4,{}}},", ub->name, roundup(ub->size,16)/16);
                }
                else {
                    L("{{0,SG_UNIFORMTYPE_INVALID,0}},");
                }
            }
            L(" }},\n");
        }
        else {
            L("        0, /* size */\n");
            L("        {{ /* uniforms */");
            for (int u_index = 0; u_index < uniform_t::NUM; u_index++) {
                L("{{0,SG_UNIFORMTYPE_INVALID,0}},");
            }
            L(" }},\n");
        }
        L("      }},\n");
    }
    L("    }},\n");
    L("    {{ /* images */ ");
    for (int img_index = 0; img_index < image_t::NUM; img_index++) {
        const image_t* img = find_image(src.refl, img_index);
        if (img) {
            L("{{\"{}\",{}}},", img->name, img_type_to_sokol_type_str(img->type));
        }
        else {
            L("{{0,_SG_IMAGETYPE_DEFAULT}},");
        }
    }
    L(" }},\n");
    L("  }},\n");
}

static void write_shader_descs(const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang) {
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;
        int vs_snippet_index = inp.snippet_map.at(prog.vs_name);
        int fs_snippet_index = inp.snippet_map.at(prog.fs_name);
        int vs_src_index = spirvcross.find_source_by_snippet_index(vs_snippet_index);
        int fs_src_index = spirvcross.find_source_by_snippet_index(fs_snippet_index);
        assert((vs_src_index >= 0) && (fs_src_index >= 0));
        const spirvcross_source_t& vs_src = spirvcross.sources[vs_src_index];
        const spirvcross_source_t& fs_src = spirvcross.sources[fs_src_index];

        /* write shader desc */
        L("static sg_shader_desc {}{}_shader_desc_{} = {{\n", mod_prefix(inp), prog.name, slang_t::to_str(slang));
        L("  0, /* _start_canary */\n");
        L("  {{ /*attrs*/");
        for (int attr_index = 0; attr_index < attr_t::NUM; attr_index++) {
            const attr_t& attr = vs_src.refl.inputs[attr_index];
            if (attr.slot >= 0) {
                L("{{\"{}\",\"{}\",{}}},", attr.name, attr.sem_name, attr.sem_index);
            }
            else {
                L("{{0,0,0}},");
            }
        }
        L(" }},\n");
        write_stage("vs", prog, vs_src, slang);
        write_stage("fs", prog, fs_src, slang);
        L("  \"{}_shader\", /* label */\n", prog.name);
        L("  0, /* _end_canary */\n");
        L("}};\n");
    }
}

static errmsg_t check_errors(const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang) {
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;
        int vs_snippet_index = inp.snippet_map.at(prog.vs_name);
        int fs_snippet_index = inp.snippet_map.at(prog.fs_name);
        int vs_src_index = spirvcross.find_source_by_snippet_index(vs_snippet_index);
        int fs_src_index = spirvcross.find_source_by_snippet_index(fs_snippet_index);
        if (vs_src_index < 0) {
            return errmsg_t(inp.path, inp.snippets[vs_snippet_index].lines[0],
                fmt::format("no generated '{}' source for vertex shader '{}' in program '{}'",
                    slang_t::to_str(slang), prog.vs_name, prog.name));
        }
        if (fs_src_index < 0) {
            return errmsg_t(inp.path, inp.snippets[vs_snippet_index].lines[0],
                fmt::format("no generated '{}' source for fragment shader '{}' in program '{}'",
                    slang_t::to_str(slang), prog.fs_name, prog.name));
        }
    }
    // all ok
    return errmsg_t();
}

errmsg_t sokol_t::gen(const args_t& args, const input_t& inp,
                     const std::array<spirvcross_t,slang_t::NUM>& spirvcross,
                     const std::array<bytecode_t,slang_t::NUM>& bytecode)
{
    // first write everything into a string, and only when no errors occur,
    // dump this into a file (so we don't have half-written files lying around)
    file_content.clear();

    L("#pragma once\n");
    errmsg_t err;
    bool comment_header_written = false;
    bool common_decls_written = false;
    for (int i = 0; i < slang_t::NUM; i++) {
        slang_t::type_t slang = (slang_t::type_t) i;
        if (args.slang & slang_t::bit(slang)) {
            errmsg_t err = check_errors(inp, spirvcross[i], slang);
            if (err.valid) {
                return err;
            }
            if (!comment_header_written) {
                write_header(args, inp, spirvcross[i]);
                comment_header_written = true;
            }
            if (!common_decls_written) {
                common_decls_written = true;
                write_vertex_attrs(inp, spirvcross[i]);
                write_images_bind_slots(inp, spirvcross[i]);
                write_uniform_blocks(inp, spirvcross[i], slang);
            }
            if (!args.no_ifdef) {
                L("#if defined({})\n", sokol_define(slang));
            }
            write_shader_descs(inp, spirvcross[i], slang);
            if (!args.no_ifdef) {
                L("#endif /* {} */\n", sokol_define(slang));
            }
        }
    }

    // write access functions which return sg_shader_desc pointers
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;
        L("static inline const sg_shader_desc* {}{}_shader_desc(void) {{\n", mod_prefix(inp), prog.name);
        for (int i = 0; i < slang_t::NUM; i++) {
            slang_t::type_t slang = (slang_t::type_t) i;
            if (args.slang & slang_t::bit(slang)) {
                if (!args.no_ifdef) {
                    L("    #if defined({})\n", sokol_define(slang));
                }
                L("    if (sg_query_backend() == {}) {{\n", sokol_backend(slang));
                L("        return &{}{}_shader_desc_{};\n", mod_prefix(inp), prog.name, slang_t::to_str(slang));
                L("    }}\n");
                if (!args.no_ifdef) {
                    L("    #endif /* {} */\n", sokol_define(slang));
                }
            }
        }
        L("    return 0; /* can't happen */\n");
        L("}}\n");
    }

    // write result into output file
    FILE* f = fopen(args.output.c_str(), "w");
    if (!f) {
        return errmsg_t(inp.path, 0, fmt::format("failed to open output file '{}'", args.output));
    }
    fwrite(file_content.c_str(), file_content.length(), 1, f);
    fclose(f);
    return errmsg_t();
}

} // namespace shdc
