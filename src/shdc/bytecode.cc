/*
    Compile HLSL / Metal source code to bytecode, HLSL only works
    when running on Windows, Metal only works when running on macOS.

    Uses d3dcompiler.dll for HLSL, and for Metal, invokes the Metal
    compiler toolchain command line tools.

    On Metal, bytecode compilation only happens for the macOS and iOS
    targets, but not for running in the simulator, in this case,
    shaders are compiled at runtime from source code.
*/
#include "bytecode.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h> // popen etc...
#if defined(_WIN32)
#include <d3dcompiler.h>
#include <d3dcommon.h>
#endif
#include "glslang/Public/ShaderLang.h"
#include "glslang/Public/ResourceLimits.h"
#include "glslang/Include/Types.h"
#include "SPIRV/GlslangToSpv.h"
#include "util.h"

namespace shdc {

const BytecodeBlob* Bytecode::find_blob_by_snippet_index(int snippet_index) const {
    for (int i = 0; i < (int)blobs.size(); i++) {
        if (blobs[i].snippet_index == snippet_index) {
            return &blobs[i];
        }
    }
    return nullptr;
}

// MacOS/Metal specific stuff...
#if defined(__APPLE__)

// write source code to file
static bool write_source(const std::string& source_code, const std::string path) {
    FILE* f = fopen(path.c_str(), "wb");
    if (f) {
        fwrite(source_code.c_str(), source_code.length(), 1, f);
        fclose(f);
        return true;
    } else {
        return false;
    }
}

// load binary file into blob
static bool read_binary(const std::string& path, std::vector<uint8_t>& out_blob) {
    bool res = false;
    FILE* f = fopen(path.c_str(), "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        const size_t file_size = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::vector<uint8_t> blob(file_size);
        res = (file_size == fread(blob.data(), 1, file_size, f));
        fclose(f);
        if (res) {
            out_blob = std::move(blob);
        }
        return true;
    } else {
        out_blob.clear();
        return false;
    }
}

// convert errors from metal compiler format to ErrMsg objects
static void mtl_parse_errors(const std::string& output, const Input& inp, int snippet_index, std::vector<ErrMsg>& out_errors) {
    /*
        format for errors/warnings is:

        FILE:LINE:COLUMN: [error|warning]: msg
    */
    const Snippet& snippet = inp.snippets[snippet_index];
    std::vector<std::string> lines;
    pystring::splitlines(output, lines);
    std::vector<std::string> tokens;
    static const std::string colon = ":";
    for (const std::string& line: lines) {
        // split by colons
        pystring::split(line, tokens, colon);
        if ((tokens.size() > 3) && ((tokens[3] == " error") || (tokens[3] == " warning"))) {
            bool ok = false;
            int line_index = 0;
            std::string msg;
            if (tokens.size() > 4) {
                // extract line index and message
                int snippet_line_index = atoi(tokens[1].c_str());
                // correct for one-based and prolog #defines
                if (snippet_line_index >= 1) {
                    snippet_line_index -= 5;
                }
                // everything after the 4th colon is message
                for (int i = 4; i < (int)tokens.size(); i++) {
                    if (msg.empty()) {
                        msg = tokens[i];
                    } else {
                        msg = fmt::format("{}:{}", msg, tokens[i]);
                    }
                }
                // snippet-line-index to input source line index
                if ((snippet_line_index >= 0) && (snippet_line_index < (int)snippet.lines.size())) {
                    line_index = snippet.lines[snippet_line_index];
                }
                ok = true;
            }
            if (ok) {
                if (tokens[3] == " error") {
                    out_errors.push_back(inp.error(line_index, msg));
                } else {
                    out_errors.push_back(inp.warning(line_index, msg));
                }
            } else {
                // some error during parsing, output the original line so it isn't lost
                out_errors.push_back(inp.error(0, msg));
            }
        }
    }
}

// run a command line program via xcrun, capture its output and exit code
static int xcrun(const std::string& cmdline, std::string& output, Slang::Enum slang) {
    std::string cmd = "xcrun ";
    if (slang == Slang::METAL_MACOS) {
        cmd += "--sdk macosx ";
    } else {
        cmd += "--sdk iphoneos ";
    }
    cmd += cmdline;
    cmd += " 2>&1";

    int exit_code = 10;
    FILE* p = popen(cmd.c_str(), "r");
    if (p) {
        char buf[1024];
        buf[0] = 0;
        while (fgets(buf, sizeof(buf), p)) {
            output += buf;
        }
        exit_code = pclose(p);
    }
    return exit_code;
}

// run the metal compiler pass
static bool mtl_cc(const std::string& src_path, const std::string& out_dia, const std::string& out_air, Slang::Enum slang, std::string& output) {
    std::string cmdline;
    cmdline =  "metal -arch air64 -emit-llvm -ffast-math -c -serialize-diagnostics ";
    cmdline += out_dia;
    cmdline += " -o ";
    cmdline += out_air;
    if (slang == Slang::METAL_MACOS) {
        cmdline += " -mmacosx-version-min=10.11 -std=osx-metal1.1 ";
    } else {
        cmdline += " -miphoneos-version-min=9.0 -std=ios-metal1.1 ";
    }
    cmdline += src_path;
    return 0 == xcrun(cmdline, output, slang);
}

// run the metal linker pass
static bool mtl_link(const std::string& lib_path, const std::string& bin_path, Slang::Enum slang) {
    std::string dummy_output;
    std::string cmdline = fmt::format("metallib -o {} {}", bin_path, lib_path);
    return 0 == xcrun(cmdline, dummy_output, slang);
}

static Bytecode mtl_compile(const Args& args, const Input& inp, const Spirvcross& spirvcross, Slang::Enum slang) {
    Bytecode bytecode;
    std::string base_dir;
    std::string base_filename;
    pystring::os::path::split(base_dir, base_filename, inp.base_path);
    std::string base_path = fmt::format("{}{}_{}_", args.tmpdir, base_filename, Slang::to_str(slang));
    std::string src_path, dia_path, air_path, lib_path, bin_path;

    // for each vertex/fragment shader source generated by SPIRV-Cross:
    for (const SpirvcrossSource& src: spirvcross.sources) {
        std::string output;
        const Snippet& snippet = inp.snippets[src.snippet_index];
        src_path = fmt::format("{}{}.metal", base_path, snippet.name);
        dia_path = fmt::format("{}{}.dia", base_path, snippet.name);
        air_path = fmt::format("{}{}.air", base_path, snippet.name);
        bin_path = fmt::format("{}{}.metallib", base_path, snippet.name);
        // write metal source code to temp file
        if (!write_source(src.source_code, src_path)) {
            bytecode.errors.push_back(ErrMsg::error(inp.base_path, 0, fmt::format("failed to write intermediate file '{}'!", src_path)));
            break;
        }
        // compiler, link, load generated bytecode
        if (!mtl_cc(src_path, dia_path, air_path, slang, output)) {
            mtl_parse_errors(output, inp, src.snippet_index, bytecode.errors);
            break;
        }
        if (!mtl_link(air_path, bin_path, slang)) {
            mtl_parse_errors(output, inp, src.snippet_index, bytecode.errors);
            break;
        }
        std::vector<uint8_t> data;
        if (!read_binary(bin_path, data)) {
            mtl_parse_errors(output, inp, src.snippet_index, bytecode.errors);
            break;
        }
        // if hard error happened there may still have been warnings
        if (!output.empty()) {
            mtl_parse_errors(output, inp, src.snippet_index, bytecode.errors);
        }

        BytecodeBlob blob;
        blob.valid = true;
        blob.snippet_index = src.snippet_index;
        blob.data = std::move(data);
        bytecode.blobs.push_back(std::move(blob));
    }
    return bytecode;
}
#endif

/* Windows specific stuff, everything happens in memory */
#if defined(_WIN32)
static HINSTANCE d3dcompiler_dll = 0;
static pD3DCompile d3dcompile_func = 0;

static bool load_d3dcompiler_dll(void) {
    if (0 == d3dcompiler_dll) {
        d3dcompiler_dll = LoadLibraryA("d3dcompiler_47.dll");
        if (0 != d3dcompiler_dll) {
            d3dcompile_func = (pD3DCompile) GetProcAddress(d3dcompiler_dll, "D3DCompile");
        }
    }
    return 0 != d3dcompile_func;
}

static void d3d_parse_errors(const std::string& output, const Input& inp, int snippet_index, std::vector<ErrMsg>& out_errors) {
    /*
        format for errors/warnings is:

        PATH(LINE,COL-RANGE): [warning|error] ID: MESSAGE

        NOTE that path may contains a device name (e.g. C:), so ignore the
        first two characters.
    */
    std::vector<std::string> lines;
    pystring::splitlines(output, lines);
    std::vector<std::string> tokens;
    static const std::string colon = ":";
    for (std::string line: lines) {
        // neutralize any drive-name colon
        if (line[1] == ':') {
            line[1] = '_';
        }
        // split by colons
        bool ok = false;
        std::string msg;
        pystring::split(line, tokens, colon);
        if ((tokens.size() > 1) && (pystring::startswith(tokens[1], " error") || pystring::startswith(tokens[1], " warning"))) {
            if (tokens.size() > 2) {
                for (int i = 2; i < (int)tokens.size(); i++) {
                    if (msg.empty()) {
                        msg = tokens[i];
                    } else {
                        msg = fmt::format("{}:{}", msg, tokens[i]);
                    }
                }
                ok = true;
            }
        }
        if (ok) {
            if (pystring::startswith(tokens[1], " error")) {
                out_errors.push_back(ErrMsg::error(inp.base_path, 0, msg));
            } else {
                out_errors.push_back(ErrMsg::warning(inp.base_path, 0, msg));
            }
        } else {
            // some error during parsing, output the original line so it isn't lost
            out_errors.push_back(ErrMsg::error(inp.base_path, 0, line));
        }
    }
}

static Bytecode d3d_compile(const Input& inp, const Spirvcross& spirvcross, Slang::Enum slang) {
    Bytecode bytecode;
    if (!load_d3dcompiler_dll()) {
        bytecode.errors.push_back(ErrMsg::warning(inp.base_path, 0, fmt::format("failed to load d3dcompiler_47.dll!")));
        return bytecode;
    }
    for (const SpirvcrossSource& src: spirvcross.sources) {
        const Snippet& snippet = inp.snippets[src.snippet_index];
        ID3DBlob* output = NULL;
        ID3DBlob* errors = NULL;
        const char* compile_target = nullptr;
        if (slang == Slang::HLSL4) {
            switch (snippet.type) {
                case Snippet::VS: compile_target = "vs_4_0"; break;
                case Snippet::FS: compile_target = "ps_4_0"; break;
                case Snippet::CS: compile_target = "cs_4_0"; break;
                default: compile_target = "UNKNOWN"; break;
            }
        } else {
            switch (snippet.type) {
                case Snippet::VS: compile_target = "vs_5_0"; break;
                case Snippet::FS: compile_target = "ps_5_0"; break;
                case Snippet::CS: compile_target = "cs_5_0"; break;
                default: compile_target = "UNKNOWN"; break;
            }
        }
        d3dcompile_func(
            src.source_code.c_str(),        // pSrcData
            src.source_code.length(),       // SrcDataSize
            NULL,                           // pSourceName
            NULL,                           // pDefines
            NULL,                           // pInclude
            src.stage_refl.entry_point.c_str(), // entryPoint
            compile_target,                 // pTarget
            D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3, // Flags1
            0,                              // Flags2
            &output,                        // ppCode
            &errors);                       // ppErrorMsgs
        if (errors) {
            std::string err_str((const char*)errors->GetBufferPointer());
            d3d_parse_errors(err_str, inp, src.snippet_index, bytecode.errors);
        }
        if (output && (output->GetBufferSize() > 0)) {
            std::vector<uint8_t> data(output->GetBufferSize());
            memcpy(data.data(), output->GetBufferPointer(), output->GetBufferSize());
            BytecodeBlob blob;
            blob.valid = true;
            blob.snippet_index = src.snippet_index;
            blob.data = std::move(data);
            bytecode.blobs.push_back(std::move(blob));
        }
        if (errors) {
            errors->Release();
        }
        if (output) {
            output->Release();
        }
    }
    return bytecode;
}
#endif

static Bytecode spirv_compile(const Input& inp, const Spirvcross& spirvcross, Slang::Enum slang) {
    Bytecode bytecode;
    for (const SpirvcrossSource& src: spirvcross.sources) {
        const Snippet& snippet = inp.snippets[src.snippet_index];

        const char* sources[1] = { src.source_code.c_str() };
        const int sourcesLen[1] = { (int) src.source_code.length() };
        const char* sourcesNames[1] = { inp.base_path.c_str() };
        const int linenr_offset = 0;

        EShLanguage stage;
        if (Snippet::is_vs(snippet.type)) {
            stage = EShLangVertex;
        } else if (Snippet::is_fs(snippet.type)) {
            stage = EShLangFragment;
        } else {
            stage = EShLangCompute;
        }

        glslang::TShader shader(stage);
        shader.setStringsWithLengthsAndNames(sources, sourcesLen, sourcesNames, 1);
        shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
        shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_4);
        bool parse_success = shader.parse(GetDefaultResources(), 460, true, EShMsgDefault);
        util::infolog_to_errors(shader.getInfoLog(), inp, src.snippet_index, linenr_offset, bytecode.errors);
        util::infolog_to_errors(shader.getInfoDebugLog(), inp, src.snippet_index, linenr_offset, bytecode.errors);
        if (!parse_success) {
            bytecode.errors.push_back(ErrMsg::warning(inp.base_path, 0, fmt::format("failed to compile GLSL to SPIRV")));
            return bytecode;
        }

        // "link" into a program
        glslang::TProgram program;
        program.addShader(&shader);
        bool link_success = program.link(EShMsgDefault);
        util::infolog_to_errors(program.getInfoLog(), inp, src.snippet_index, linenr_offset, bytecode.errors);
        util::infolog_to_errors(program.getInfoDebugLog(), inp, src.snippet_index, linenr_offset, bytecode.errors);
        if (!link_success) {
            return bytecode;
        }
        bool map_success = program.mapIO();
        util::infolog_to_errors(program.getInfoLog(), inp, src.snippet_index, linenr_offset, bytecode.errors);
        util::infolog_to_errors(program.getInfoDebugLog(), inp, src.snippet_index, linenr_offset, bytecode.errors);
        if (!map_success) {
            return bytecode;
        }

        // translate intermediate representation to SPIRV
        std::vector<uint32_t> out_spirv;
        const glslang::TIntermediate* im = program.getIntermediate(stage);
        assert(im);
        spv::SpvBuildLogger spv_logger;
        glslang::SpvOptions spv_options;
        // disable the optimizer passes, we'll run our own after the translation
        spv_options.generateDebugInfo = false;
        spv_options.stripDebugInfo = false; // NOTE: don't set this to true as the info is needed for reflection!
        spv_options.disableOptimizer = true;
        spv_options.optimizeSize = false;
        spv_options.disassemble = false;
        spv_options.validate = false;
        spv_options.emitNonSemanticShaderDebugInfo = false;
        spv_options.emitNonSemanticShaderDebugSource = false;
        glslang::GlslangToSpv(*im, out_spirv, &spv_logger, &spv_options);
        std::string spirv_log = spv_logger.getAllMessages();
        if (!spirv_log.empty()) {
            // FIXME: need to parse string for errors and translate to ErrMsg objects?
            // haven't seen a case yet where this generates log messages
            fmt::print(stderr, "{}", spirv_log);
        }

        const uint8_t* data_ptr = (const uint8_t*)out_spirv.data();
        const size_t data_len = out_spirv.size() * sizeof(uint32_t);

        BytecodeBlob blob;
        blob.valid = true;
        blob.snippet_index = src.snippet_index;
        blob.data = std::vector<uint8_t>(data_ptr, data_ptr + data_len);
        bytecode.blobs.push_back(std::move(blob));
    }
    return bytecode;
}

Bytecode Bytecode::compile(const Args& args, const Input& inp, const Spirvcross& spirvcross, Slang::Enum slang) {
    Bytecode bytecode;
    #if defined(__APPLE__)
    // NOTE: for the iOS simulator case, don't compile bytecode but use source code
    if ((slang == Slang::METAL_MACOS) || (slang == Slang::METAL_IOS)) {
        bytecode = mtl_compile(args, inp, spirvcross, slang);
    }
    #endif
    #if defined(_WIN32)
    if (Slang::is_hlsl(slang)) {
        bytecode = d3d_compile(inp, spirvcross, slang);
    }
    #endif
    if (Slang::is_spirv(slang)) {
        bytecode = spirv_compile(inp, spirvcross, slang);
    }
    return bytecode;
}

void Bytecode::dump_debug() const {
    fmt::print(stderr, "Bytecode::dump_debug(): FIXME!\n");
}

} // namespace shdc
