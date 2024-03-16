/*
    Generate bare output in text or binary format
*/
#include "bare.h"
#include "util.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc {

using namespace util;

static ErrMsg write_stage(const std::string& file_path,
                            const SpirvcrossSource* src,
                            const BytecodeBlob* blob)
{
    // write text or binary to output file
    FILE* f = fopen(file_path.c_str(), "wb");
    if (!f) {
        return ErrMsg::error(file_path, 0, fmt::format("failed to open output file '{}'", file_path));
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
        return ErrMsg::error(file_path, 0, fmt::format("failed to write output file '{}'", file_path));
    }
    fclose(f);
    return ErrMsg();
}

static ErrMsg write_shader_sources_and_blobs(const Args& args,
                                               const Input& inp,
                                               const spirvcross_t& spirvcross,
                                               const Bytecode& bytecode,
                                               Slang::type_t slang)
{
    for (const auto& item: inp.programs) {
        const Program& prog = item.second;
        const SpirvcrossSource* vs_src = find_spirvcross_source_by_shader_name(prog.vs_name, inp, spirvcross);
        const SpirvcrossSource* fs_src = find_spirvcross_source_by_shader_name(prog.fs_name, inp, spirvcross);
        const BytecodeBlob* vs_blob = find_bytecode_blob_by_shader_name(prog.vs_name, inp, bytecode);
        const BytecodeBlob* fs_blob = find_bytecode_blob_by_shader_name(prog.fs_name, inp, bytecode);

        const std::string file_path_base = fmt::format("{}_{}{}_{}", args.output, mod_prefix(inp), prog.name, Slang::to_str(slang));
        const std::string file_path_vs = fmt::format("{}_vs{}", file_path_base, util::slang_file_extension(slang, vs_blob));
        const std::string file_path_fs = fmt::format("{}_fs{}", file_path_base, util::slang_file_extension(slang, fs_blob));

        ErrMsg err;
        err = write_stage(file_path_vs, vs_src, vs_blob);
        if (err.has_error) {
            return err;
        }
        err = write_stage(file_path_fs, fs_src, fs_blob);
        if (err.has_error) {
            return err;
        }
    }

    return ErrMsg();
}

ErrMsg bare_t::gen(const Args& args, const Input& inp,
                     const std::array<spirvcross_t,Slang::NUM>& spirvcross,
                     const std::array<Bytecode,Slang::NUM>& bytecode)
{
    for (int i = 0; i < Slang::NUM; i++) {
        Slang::type_t slang = (Slang::type_t) i;
        if (args.slang & Slang::bit(slang)) {
            ErrMsg err = check_errors(inp, spirvcross[i], slang);
            if (err.has_error) {
                return err;
            }
            err = write_shader_sources_and_blobs(args, inp, spirvcross[i], bytecode[i], slang);
            if (err.has_error) {
                return err;
            }
        }
    }

    return ErrMsg();
}

} // namespace shdc
