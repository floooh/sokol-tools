/*
    Generate bare output in text or binary format
*/
#include "bare.h"
#include "util.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc::gen {

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
    } else {
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
                                               const Spirvcross& spirvcross,
                                               const Bytecode& bytecode,
                                               Slang::Enum slang)
{
    for (const auto& item: inp.programs) {
        const Program& prog = item.second;
        const SpirvcrossSource* vs_src = find_spirvcross_source_by_shader_name(prog.vs_name, inp, spirvcross);
        const SpirvcrossSource* fs_src = find_spirvcross_source_by_shader_name(prog.fs_name, inp, spirvcross);
        const BytecodeBlob* vs_blob = find_bytecode_blob_by_shader_name(prog.vs_name, inp, bytecode);
        const BytecodeBlob* fs_blob = find_bytecode_blob_by_shader_name(prog.fs_name, inp, bytecode);

        const std::string file_path_base = fmt::format("{}_{}{}_{}", args.output, mod_prefix(inp), prog.name, Slang::to_str(slang));
        const std::string file_path_vs = fmt::format("{}_vs{}", file_path_base, util::slang_file_extension(slang, vs_blob != nullptr));
        const std::string file_path_fs = fmt::format("{}_fs{}", file_path_base, util::slang_file_extension(slang, fs_blob != nullptr));

        ErrMsg err;
        err = write_stage(file_path_vs, vs_src, vs_blob);
        if (err.valid()) {
            return err;
        }
        err = write_stage(file_path_fs, fs_src, fs_blob);
        if (err.valid()) {
            return err;
        }
    }

    return ErrMsg();
}

static ErrMsg _generate(const GenInput& gen) {
    for (int i = 0; i < Slang::Num; i++) {
        Slang::Enum slang = (Slang::Enum) i;
        if (gen.args.slang & Slang::bit(slang)) {
            ErrMsg err = check_errors(gen.inp, gen.spirvcross[i], slang);
            if (err.valid()) {
                return err;
            }
            err = write_shader_sources_and_blobs(gen.args, gen.inp, gen.spirvcross[i], gen.bytecode[i], slang);
            if (err.valid()) {
                return err;
            }
        }
    }

    return ErrMsg();
}

//------------------------------------------------------------------------------
ErrMsg BareGenerator::generate(const GenInput& gen) {
    return _generate(gen);
}

} // namespace
