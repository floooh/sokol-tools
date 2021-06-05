/*
    Generate bare output in text or binary format
*/
#include "shdc.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc {

using namespace util;

static const char* slang_file_extension(slang_t::type_t c, bool binary) {
    switch (c) {
        case slang_t::GLSL330:
        case slang_t::GLSL100:
        case slang_t::GLSL300ES:
            return ".glsl";
        case slang_t::HLSL4:
        case slang_t::HLSL5:
            return binary ? ".fxc" : ".hlsl";
        case slang_t::METAL_MACOS:
        case slang_t::METAL_IOS:
        case slang_t::METAL_SIM:
            return binary ? ".metallib" : ".metal";
        default:
            return "";
    }
}

static errmsg_t write_stage(const std::string& file_path,
                            const spirvcross_source_t* src,
                            const bytecode_blob_t* blob)
{
    // write text or binary to output file
    FILE* f = fopen(file_path.c_str(), "wb");
    if (!f) {
        return errmsg_t::error(file_path, 0, fmt::format("failed to open output file '{}'", file_path));
    }
    const void* write_data;
    size_t write_count;
    if (blob) {
        write_data = blob->data.data();
        write_count = blob->data.size();
    }
    else {
        assert(src);
        write_data = src->source_code.data();
        write_count = src->source_code.length();
    }
    size_t written = fwrite(write_data, 1, write_count, f);
    if (written != write_count) {
        return errmsg_t::error(file_path, 0, fmt::format("failed to write output file '{}'", file_path));
    }
    fclose(f);
    return errmsg_t();
}

static errmsg_t write_shader_sources_and_blobs(const args_t& args,
                                               const input_t& inp,
                                               const spirvcross_t& spirvcross,
                                               const bytecode_t& bytecode,
                                               slang_t::type_t slang)
{
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;
        const spirvcross_source_t* vs_src = find_spirvcross_source_by_shader_name(prog.vs_name, inp, spirvcross);
        const spirvcross_source_t* fs_src = find_spirvcross_source_by_shader_name(prog.fs_name, inp, spirvcross);
        const bytecode_blob_t* vs_blob = find_bytecode_blob_by_shader_name(prog.vs_name, inp, bytecode);
        const bytecode_blob_t* fs_blob = find_bytecode_blob_by_shader_name(prog.fs_name, inp, bytecode);
        std::string file_path_vs = fmt::format("{}_{}{}_{}_vs{}", args.output, mod_prefix(inp), prog.name, slang_t::to_str(slang), slang_file_extension(slang, vs_blob));
        std::string file_path_fs = fmt::format("{}_{}{}_{}_fs{}", args.output, mod_prefix(inp), prog.name, slang_t::to_str(slang), slang_file_extension(slang, fs_blob));

        errmsg_t err;
        err = write_stage(file_path_vs, vs_src, vs_blob);
        if (err.valid) {
            return err;
        }
        err = write_stage(file_path_fs, fs_src, fs_blob);
        if (err.valid) {
            return err;
        }
    }

    return errmsg_t();
}

errmsg_t bare_t::gen(const args_t& args, const input_t& inp,
                     const std::array<spirvcross_t,slang_t::NUM>& spirvcross,
                     const std::array<bytecode_t,slang_t::NUM>& bytecode)
{
    for (int i = 0; i < slang_t::NUM; i++) {
        slang_t::type_t slang = (slang_t::type_t) i;
        if (args.slang & slang_t::bit(slang)) {
            errmsg_t err = check_errors(inp, spirvcross[i], slang);
            if (err.valid) {
                return err;
            }
            err = write_shader_sources_and_blobs(args, inp, spirvcross[i], bytecode[i], slang);
            if (err.valid) {
                return err;
            }
        }
    }

    return errmsg_t();
}

} // namespace shdc
