/*
    Generate bare output in text or binary format
*/
#include "shdc.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc {

using namespace util;

static std::string file_content;

#if defined(_MSC_VER)
#define L(str, ...) file_content.append(fmt::format(str, __VA_ARGS__))
#else
#define L(str, ...) file_content.append(fmt::format(str, ##__VA_ARGS__))
#endif

static uniform_t as_flattened_uniform(const uniform_block_t& block) {
    assert(block.flattened);
    assert(!block.uniforms.empty());

    const uniform_t::type_t type = [&block](){
        switch (block.uniforms[0].type) {
            case uniform_t::FLOAT:
            case uniform_t::FLOAT2:
            case uniform_t::FLOAT3:
            case uniform_t::FLOAT4:
            case uniform_t::MAT4:
                return uniform_t::FLOAT4;
            case uniform_t::INT:
            case uniform_t::INT2:
            case uniform_t::INT3:
            case uniform_t::INT4:
                return uniform_t::INT4;
            default:
                assert(false);
                return uniform_t::INVALID;
        }
    }();
    uniform_t uniform;
    uniform.name = block.struct_name;
    uniform.type = type;
    uniform.array_count = roundup(block.size, 16) / 16;
    return uniform;
}

static void write_attribute(const attr_t& att) {
    L("            -\n");
    L("              slot: {}\n", att.slot);
    L("              name: {}\n", att.name);
    L("              sem_name: {}\n", att.sem_name);
    L("              sem_index: {}\n", att.sem_index);
}

static void write_uniform(const uniform_t& uniform);
static void write_uniform_block(const uniform_block_t& uniform_block) {
    L("            -\n");
    L("              slot: {}\n", uniform_block.slot);
    L("              size: {}\n", uniform_block.size);
    L("              struct_name: {}\n", uniform_block.struct_name);
    L("              inst_name: {}\n", uniform_block.inst_name);
    L("              uniforms:\n");


    if (uniform_block.flattened && !uniform_block.uniforms.empty()) {
        write_uniform(as_flattened_uniform(uniform_block));
        return;
    }

    for (const auto& uniform: uniform_block.uniforms) {
        write_uniform(uniform);
    }
}

static void write_uniform(const uniform_t& uniform) {
    L("                -\n");
    L("                  name: {}\n", uniform.name);
    L("                  type: {}\n", uniform_t::type_to_str(uniform.type));
    L("                  array_count: {}\n", uniform.array_count);
    L("                  offset: {}\n", uniform.offset);
}

static void write_image(const image_t& image) {
    L("            -\n");
    L("              slot: {}\n", image.slot);
    L("              name: {}\n", image.name);
    L("              multisampled: {}\n", image.multisampled);
    L("              type: {}\n", image_type_t::to_str(image.type));
    L("              sample_type: {}\n", image_sample_type_t::to_str(image.sample_type));
}

static void write_sampler(const sampler_t& sampler) {
    L("            -\n");
    L("              slot: {}\n", sampler.slot);
    L("              name: {}\n", sampler.name);
    L("              sampler_type: {}\n", sampler_type_t::to_str(sampler.type));
}

static void write_image_sampler(const image_sampler_t& image_sampler) {
    L("            -\n");
    L("              slot: {}\n", image_sampler.slot);
    L("              name: {}\n", image_sampler.name);
    L("              image_name: {}\n", image_sampler.image_name);
    L("              sampler_name: {}\n", image_sampler.sampler_name);
}

static void write_source_reflection(const spirvcross_source_t* src) {
    L("          entry_point: {}\n", src->refl.entry_point);
    L("          inputs:\n");
    for (const auto& input: src->refl.inputs) {
        if (input.slot == -1) {
            break;
        }
        write_attribute(input);
    }
    L("          outputs:\n");
    for (const auto& output: src->refl.outputs) {
        if (output.slot == -1) {
            break;
        }
        write_attribute(output);
    }
    if (src->refl.uniform_blocks.size() > 0) {
        L("          uniform_blocks:\n");
        for (const auto& uniform_block: src->refl.uniform_blocks) {
            if (uniform_block.slot == -1) {
                break;
            }
            write_uniform_block(uniform_block);
        }
    }
    if (src->refl.images.size() > 0) {
        L("          images:\n");
        for (const auto& image: src->refl.images) {
            if (image.slot == -1) {
                break;
            }
            write_image(image);
        }
    }
    if (src->refl.samplers.size() > 0) {
        L("          samplers:\n");
        for (const auto& sampler: src->refl.samplers) {
            if (sampler.slot == -1) {
                break;
            }
            write_sampler(sampler);
        }
    }
    if (src->refl.image_samplers.size() > 0) {
        L("          image_sampler_pairs:\n");
        for (const auto& image_sampler: src->refl.image_samplers) {
            if (image_sampler.slot == -1) {
                break;
            }
            write_image_sampler(image_sampler);
        }
    }
}

static errmsg_t write_shader_sources_and_blobs(const args_t& args,
                                               const input_t& inp,
                                               const spirvcross_t& spirvcross,
                                               const bytecode_t& bytecode,
                                               slang_t::type_t slang)
{
    L("    programs:\n");
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;
        const spirvcross_source_t* vs_src = find_spirvcross_source_by_shader_name(prog.vs_name, inp, spirvcross);
        const spirvcross_source_t* fs_src = find_spirvcross_source_by_shader_name(prog.fs_name, inp, spirvcross);
        const bytecode_blob_t* vs_blob = find_bytecode_blob_by_shader_name(prog.vs_name, inp, bytecode);
        const bytecode_blob_t* fs_blob = find_bytecode_blob_by_shader_name(prog.fs_name, inp, bytecode);

        const std::string file_path_base = fmt::format("{}_{}{}_{}", args.output, mod_prefix(inp), prog.name, slang_t::to_str(slang));
        const std::string file_path_vs = fmt::format("{}_vs{}", file_path_base, bare_t::slang_file_extension(slang, vs_blob));
        const std::string file_path_fs = fmt::format("{}_fs{}", file_path_base, bare_t::slang_file_extension(slang, fs_blob));

        L("      -\n");
        L("        name: {}\n", prog.name);
        L("        vs:\n");
        L("          path: {}\n", file_path_vs);
        L("          is_binary: {}\n", vs_blob != nullptr);
        write_source_reflection(vs_src);

        L("        fs:\n");
        L("          path: {}\n", file_path_fs);
        L("          is_binary: {}\n", fs_blob != nullptr);
        write_source_reflection(fs_src);
    }

    return errmsg_t();
}

errmsg_t yaml_t::gen(const args_t& args, const input_t& inp, const std::array<spirvcross_t,slang_t::NUM>& spirvcross, const std::array<bytecode_t,slang_t::NUM>& bytecode)
{
    // first write everything into a string, and only when no errors occur,
    // dump this into a file (so we don't have half-written files lying around)
    file_content.clear();

    L("shaders:\n");

    for (int i = 0; i < slang_t::NUM; i++) {
        slang_t::type_t slang = (slang_t::type_t) i;
        if (args.slang & slang_t::bit(slang)) {
            errmsg_t err = check_errors(inp, spirvcross[i], slang);
            if (err.valid) {
                return err;
            }

            L("  -\n");
            L("    slang: {}\n", slang_t::to_str(slang));
            err = write_shader_sources_and_blobs(args, inp, spirvcross[i], bytecode[i], slang);
            if (err.valid) {
                return err;
            }
        }
    }

    // write result into output file
    const std::string file_path = fmt::format("{}_{}reflection.yaml", args.output, mod_prefix(inp));
    FILE* f = fopen(file_path.c_str(), "w");
    if (!f) {
        return errmsg_t::error(inp.base_path, 0, fmt::format("failed to open output file '{}'", args.output));
    }
    fwrite(file_content.c_str(), file_content.length(), 1, f);
    fclose(f);

    return errmsg_t();
}

}
