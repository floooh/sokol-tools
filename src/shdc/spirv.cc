/*
    compile GLSL to SPIRV, wrapper around https://github.com/KhronosGroup/glslang
*/
#include <stdlib.h>
#include "spirv.h"
#include "fmt/format.h"
#include "pystring.h"
#include "ShaderLang.h"
#include "ResourceLimits.h"
#include "GlslangToSpv.h"
#include "spirv-tools/libspirv.hpp"
#include "spirv-tools/optimizer.hpp"

namespace shdc {

void spirv_t::initialize_spirv_tools() {
    glslang::InitializeProcess();
}

void spirv_t::finalize_spirv_tools() {
    glslang::FinalizeProcess();
}

// defined at end of file
extern const TBuiltInResource DefaultTBuiltInResource;

/* merge shader snippet source into a single string */
static std::string merge_source(const input_t& inp, const Snippet& snippet, Slang::type_t slang, const std::vector<std::string>& defines) {
    std::string src = "#version 450\n";
    if (Slang::is_glsl(slang)) {
        src += "#define SOKOL_GLSL (1)\n";
    }
    if (Slang::is_hlsl(slang)) {
        src += "#define SOKOL_HLSL (1)\n";
    }
    if (Slang::is_msl(slang)) {
        src += "#define SOKOL_MSL (1)\n";
    }
    if (Slang::is_wgsl(slang)) {
        src += "#define SOKOL_WGSL (1)\n";
    }
    for (const std::string& define : defines) {
        src += fmt::format("#define {} (1)\n", define);
    }
    for (int line_index : snippet.lines) {
        src += fmt::format("{}\n", inp.lines[line_index].line);
    }
    return src;
}

/* convert a glslang info-log string to ErrMsg's and append to out_errors */
static void infolog_to_errors(const std::string& log, const input_t& inp, int snippet_index, std::vector<ErrMsg>& out_errors) {
    /*
        format for errors is "[ERROR|WARNING]: [pos=0?]:[line]: message"
        And a last line we need to ignore: "ERROR: N compilation errors. ..."
    */
    const Snippet& snippet = inp.snippets[snippet_index];

    std::vector<std::string> lines;
    pystring::splitlines(log, lines);
    std::vector<std::string> tokens;
    static const std::string colon = ":";
    for (const std::string& line: lines) {
        // ignore the "compilation terminated" message, nothing useful in there
        if (pystring::find(line, ": compilation terminated") >= 0) {
            continue;
        }
        // split by colons
        pystring::split(line, tokens, colon);
        if ((tokens.size() > 2) && ((tokens[0] == "ERROR") || (tokens[0] == "WARNING"))) {
            bool ok = false;
            int line_index = 0;
            std::string msg;
            if (tokens.size() >= 4) {
                // extract line index and message
                int snippet_line_index = atoi(tokens[2].c_str());
                // correct for one-based and prolog #defines
                if (snippet_line_index >= 1) {
                    snippet_line_index -= 5;
                }
                // everything after the 3rd colon is 'msg'
                for (int i = 3; i < (int)tokens.size(); i++) {
                    if (msg.empty()) {
                        msg = tokens[i];
                    }
                    else {
                        msg = fmt::format("{}:{}", msg, tokens[i]);
                    }
                }
                msg = pystring::strip(msg);
                // snippet-line-index to input source line index
                if ((snippet_line_index >= 0) && (snippet_line_index < (int)snippet.lines.size())) {
                    line_index = snippet.lines[snippet_line_index];
                    ok = true;
                }
            }
            if (ok) {
                if (tokens[0] == "ERROR") {
                    out_errors.push_back(inp.error(line_index, msg));
                }
                else {
                    out_errors.push_back(inp.warning(line_index, msg));
                }
            }
            else {
                // some error during parsing, still create an error object so the error isn't lost in the void
                out_errors.push_back(inp.error(0, line));
            }
        }
    }
}

/* this is a clone of SpvTools.cpp/SpirvToolsLegalize with better control over
    what optimization passes are run (some passes may generate shader code
    which translates to valid GLSL, but invalid WebGL GLSL - e.g. simple
    bounded for-loops are converted to what looks like an unbounded loop
    ("for (;;) { }") to WebGL
*/
static void spirv_optimize(Slang::type_t slang, std::vector<uint32_t>& spirv) {
    if (slang == Slang::WGSL) {
        return;
    }
    spv_target_env target_env;
    target_env = SPV_ENV_UNIVERSAL_1_2;
    spvtools::Optimizer optimizer(target_env);
    optimizer.SetMessageConsumer(
        [](spv_message_level_t level, const char *source, const spv_position_t &position, const char *message) {
            // FIXME
        });

    optimizer.RegisterPass(spvtools::CreateDeadBranchElimPass());
/*
    optimizer.RegisterPass(spvtools::CreateMergeReturnPass());
    optimizer.RegisterPass(spvtools::CreateInlineExhaustivePass());
*/
    optimizer.RegisterPass(spvtools::CreateEliminateDeadFunctionsPass());
    optimizer.RegisterPass(spvtools::CreateScalarReplacementPass());
    optimizer.RegisterPass(spvtools::CreateLocalAccessChainConvertPass());
    optimizer.RegisterPass(spvtools::CreateLocalSingleBlockLoadStoreElimPass());
    optimizer.RegisterPass(spvtools::CreateLocalSingleStoreElimPass());
    optimizer.RegisterPass(spvtools::CreateSimplificationPass());
    optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass(true));    // NOTE: call the "preserveInterface" version of CreateAggressiveDCEPass()
    optimizer.RegisterPass(spvtools::CreateVectorDCEPass());
    optimizer.RegisterPass(spvtools::CreateDeadInsertElimPass());
    optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass(true));
    optimizer.RegisterPass(spvtools::CreateDeadBranchElimPass());
// NOTE: it's the BlockMergePass which moves the init statement of a for-loop
//       out of the for-statement, which makes it invalid for WebGL
//    optimizer.RegisterPass(spvtools::CreateBlockMergePass());
// NOTE: this is the pass which may create invalid WebGL code
//    optimizer.RegisterPass(spvtools::CreateLocalMultiStoreElimPass());
    optimizer.RegisterPass(spvtools::CreateIfConversionPass());
    optimizer.RegisterPass(spvtools::CreateSimplificationPass());
    optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass(true));
    optimizer.RegisterPass(spvtools::CreateVectorDCEPass());
    optimizer.RegisterPass(spvtools::CreateDeadInsertElimPass());
    optimizer.RegisterPass(spvtools::CreateRedundancyEliminationPass());
    optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass(true));
    optimizer.RegisterPass(spvtools::CreateCFGCleanupPass());

    spvtools::OptimizerOptions spvOptOptions;
    spvOptOptions.set_run_validator(false); // The validator may run as a separate step later on
    optimizer.Run(spirv.data(), spirv.size(), &spirv, spvOptOptions);
}

/* compile a vertex or fragment shader to SPIRV */
static bool compile(EShLanguage stage, Slang::type_t slang, const std::string& src, const input_t& inp, int snippet_index, spirv_t& out_spirv) {
    const char* sources[1] = { src.c_str() };
    const int sourcesLen[1] = { (int) src.length() };
    const char* sourcesNames[1] = { inp.base_path.c_str() };

    // compile GLSL vertex- or fragment-shader
    glslang::TShader shader(stage);
    // FIXME: add custom defines here: compiler.addProcess(...)
    shader.setStringsWithLengthsAndNames(sources, sourcesLen, sourcesNames, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_0);
    // NOTE: where using AutoMapBinding here, but this will just throw all bindings
    // into descriptor set null, which is not what we actually want.
    // We'll fix up the bindings later before calling SPIRVCross.
    shader.setAutoMapLocations(true);
    shader.setAutoMapBindings(true);
    bool parse_success = shader.parse(GetDefaultResources(), 100, false, EShMsgDefault);
    infolog_to_errors(shader.getInfoLog(), inp, snippet_index, out_spirv.errors);
    infolog_to_errors(shader.getInfoDebugLog(), inp, snippet_index, out_spirv.errors);
    if (!parse_success) {
        return false;
    }

    // "link" into a program
    glslang::TProgram program;
    program.addShader(&shader);
    bool link_success = program.link(EShMsgDefault);
    infolog_to_errors(program.getInfoLog(), inp, snippet_index, out_spirv.errors);
    infolog_to_errors(program.getInfoDebugLog(), inp, snippet_index, out_spirv.errors);
    if (!link_success) {
        return false;
    }
    bool map_success = program.mapIO();
    infolog_to_errors(program.getInfoLog(), inp, snippet_index, out_spirv.errors);
    infolog_to_errors(program.getInfoDebugLog(), inp, snippet_index, out_spirv.errors);
    if (!map_success) {
        return false;
    }

    // translate intermediate representation to SPIRV
    const glslang::TIntermediate* im = program.getIntermediate(stage);
    assert(im);
    spv::SpvBuildLogger spv_logger;
    glslang::SpvOptions spv_options;
    // disable the optimizer passes, we'll run our own after the translation
    spv_options.generateDebugInfo = false;
    spv_options.stripDebugInfo = false; // NOTE: don't set this to true as the info is needed for reflection!
    spv_options.disableOptimizer = true;
    spv_options.optimizeSize = true;
    spv_options.disassemble = false;
    spv_options.validate = false;
    spv_options.emitNonSemanticShaderDebugInfo = false;
    spv_options.emitNonSemanticShaderDebugSource = false;
    out_spirv.blobs.push_back(SpirvBlob(snippet_index));
    out_spirv.blobs.back().source = src;
    glslang::GlslangToSpv(*im, out_spirv.blobs.back().bytecode, &spv_logger, &spv_options);
    std::string spirv_log = spv_logger.getAllMessages();
    if (!spirv_log.empty()) {
        // FIXME: need to parse string for errors and translate to ErrMsg objects?
        // haven't seen a case yet where this generates log messages
        fmt::print("{}", spirv_log);
    }
    // run optimizer passes
    spirv_optimize(slang, out_spirv.blobs.back().bytecode);
    return true;
}

// compile all shader-snippets into SPIRV bytecode
spirv_t spirv_t::compile_glsl(const input_t& inp, Slang::type_t slang, const std::vector<std::string>& defines) {
    spirv_t out_spirv;

    // compile shader-snippets
    int snippet_index = 0;
    for (const Snippet& snippet: inp.snippets) {
        if (snippet.type == Snippet::VS) {
            // vertex shader
            std::string src = merge_source(inp, snippet, slang, defines);
            if (!compile(EShLangVertex, slang, src, inp, snippet_index, out_spirv)) {
                // spirv.errors contains error list
                return out_spirv;
            }
        } else if (snippet.type == Snippet::FS) {
            // fragment shader
            std::string src = merge_source(inp, snippet, slang, defines);
            if (!compile(EShLangFragment, slang, src, inp, snippet_index, out_spirv)) {
                // spirv.errors contains error list
                return out_spirv;
            }
        }
        snippet_index++;
    }
    // when arriving here, no compile errors occurred
    // spirv.bytecodes array contains the SPIRV-bytecode
    // for each shader snippet
    return out_spirv;
}

bool spirv_t::write_to_file(const args_t& args, const input_t& inp, Slang::type_t slang) {
    std::string base_dir;
    std::string base_filename;
    pystring::os::path::split(base_dir, base_filename, inp.base_path);
    std::string base_path = fmt::format("{}{}_{}_", args.tmpdir, base_filename, Slang::to_str(slang));
    for (const SpirvBlob& blob: blobs) {
        const Snippet& snippet = inp.snippets[blob.snippet_index];
        {
            const std::string path = fmt::format("{}{}.spv", base_path, snippet.name);
            FILE* fp = fopen(path.c_str(), "wb");
            if (fp) {
                fwrite(blob.bytecode.data(), sizeof(uint32_t), blob.bytecode.size(), fp);
                fclose(fp);
            } else {
                fmt::print("Failed to open '{}' for writing!\n", path);
                return false;
            }
        }
        {
            const std::string path = fmt::format("{}{}.glsl", base_path, snippet.name);
            FILE* fp = fopen(path.c_str(), "w");
            if (fp) {
                fwrite(blob.source.c_str(), 1, blob.source.length(), fp);
                fclose(fp);
            } else {
                fmt::print("Failed to open '{}' for writing!\n", path);
                return false;
            }
        }
    }
    return true;
}

void spirv_t::dump_debug(const input_t& inp, ErrMsg::msg_format_t err_fmt) const {
    fmt::print(stderr, "spirv_t:\n");
    if (errors.size() > 0) {
        fmt::print(stderr, "  error:\n");
        for (const ErrMsg& err: errors) {
            fmt::print(stderr, "    {}\n", err.as_string(err_fmt));
        }
        fmt::print(stderr, "\n");
    }
    else {
        fmt::print(stderr, "  errors: none\n\n");
    }
    for (const SpirvBlob& blob : blobs) {
        fmt::print(stderr, "  source for snippet '{}':\n", inp.snippets[blob.snippet_index].name);
        std::vector<std::string> src_lines;
        pystring::splitlines(blob.source, src_lines);
        for (const std::string& src_line: src_lines) {
            fmt::print(stderr, "    {}\n", src_line);
        }
        fmt::print(stderr, "\n");

        fmt::print(stderr, "  SPIR-V for snippet '{}':\n", inp.snippets[blob.snippet_index].name);
        spvtools::SpirvTools spirv_tools(SPV_ENV_OPENGL_4_5);
        std::string dasm_str;
        spirv_tools.Disassemble(blob.bytecode, &dasm_str, spvtools::SpirvTools::kDefaultDisassembleOption);
        std::vector<std::string> dasm_lines;
        pystring::splitlines(dasm_str, dasm_lines);
        for (const std::string& dasm_line: dasm_lines) {
            fmt::print(stderr, "    {}\n", dasm_line);
        }
        fmt::print(stderr, "\n");
    }
    fmt::print(stderr, "\n");
}

} // namespace shdc
