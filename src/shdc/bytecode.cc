/*
    Compile HLSL / Metal source code to bytecode, HLSL only works
    when running on Windos, Metal only works when running on macOS.

    Uses d3dcompiler.dll for HLSL, and for Metal, invokes the Metal
    compiler toolchain commandline tools.
*/
#include "shdc.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h> // popen etc...
#if defined(_WIN32)
#include <d3dcompiler.h>
#include <d3dcommon.h>
#endif

namespace shdc {

int bytecode_t::find_blob_by_snippet_index(int snippet_index) const {
    for (int i = 0; i < (int)blobs.size(); i++) {
        if (blobs[i].snippet_index == snippet_index) {
            return i;
        }
    }
    return -1;
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
    }
    else {
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
    }
    else {
        out_blob.clear();
        return false;
    }
}

// convert errors from metal compiler format to error_t objects
static void mtl_parse_errors(const std::string& output, const input_t& inp, int snippet_index, std::vector<errmsg_t>& out_errors) {
    /*
        format for errors/warnings is:

        FILE:LINE:COLUMN: [error|warning]: msg

        NOTE: we cannot map the line numbers back to the original GLSL source
        (this would require support for #line statements in SPIRV-Cross), so
        we'll just emit the errors and warnings at the first line
    */
    std::vector<std::string> lines;
    pystring::splitlines(output, lines);
    std::vector<std::string> tokens;
    static const std::string colon = ":";
    for (const std::string& line: lines) {
        // split by colons
        pystring::split(line, tokens, colon);
        if ((tokens.size() > 3) && ((tokens[3] == " error") || (tokens[3] == " warning"))) {
            bool ok = false;
            std::string msg;
            if (tokens.size() > 4) {
                for (int i = 4; i < (int)tokens.size(); i++) {
                    if (msg.empty()) {
                        msg = tokens[i];
                    }
                    else {
                        msg = fmt::format("{}:{}", msg, tokens[i]);
                    }
                }
                ok = true;
            }
            if (ok) {
                if (tokens[3] == " error") {
                    out_errors.push_back(errmsg_t::error(inp.path, 0, msg));
                }
                else {
                    out_errors.push_back(errmsg_t::warning(inp.path, 0, msg));
                }
            }
            else {
                // some error during parsing, output the original line so it isn't lost
                out_errors.push_back(errmsg_t::error(inp.path, 0, msg));
            }
        }
    }
}

// run a command line program via xcrun, capture its output and exit code
static int xcrun(const std::string& cmdline, std::string& output, slang_t::type_t slang) {
    std::string cmd = "xcrun ";
    if (slang == slang_t::METAL_MACOS) {
        cmd += "--sdk macosx ";
    }
    else {
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
static bool mtl_cc(const std::string& src_path, const std::string& out_dia, const std::string& out_air, slang_t::type_t slang, std::string& output) {
    std::string cmdline;
    cmdline =  "metal -arch air64 -emit-llvm -ffast-math -c -serialize-diagnostics ";
    cmdline += out_dia;
    cmdline += " -o ";
    cmdline += out_air;
    if (slang == slang_t::METAL_IOS) {
        cmdline += " -miphoneos-version-min=9.0 -std=ios-metal1.0 ";
    }
    else {
        cmdline += " -mmacosx-version-min=10.11 -std=osx-metal1.1 ";
    }
    cmdline += src_path;
    return 0 == xcrun(cmdline, output, slang);
}

// run the metal linker pass
static bool mtl_link(const std::string& lib_path, const std::string& bin_path, slang_t::type_t slang) {
    std::string dummy_output;
    std::string cmdline = fmt::format("metallib -o {} {}", bin_path, lib_path);
    return 0 == xcrun(cmdline, dummy_output, slang);
}

static bytecode_t mtl_compile(const args_t& args, const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang) {
    bytecode_t bytecode;
    std::string base_path = fmt::format("{}{}_{}_", args.tmpdir, inp.filename, slang_t::to_str(slang)); 
    std::string src_path, dia_path, air_path, lib_path, bin_path;

    // for each vertex/fragment shader source generated by SPIRV-Cross:
    bool error = false;
    for (const spirvcross_source_t& src: spirvcross.sources) {
        std::string output;
        const snippet_t& snippet = inp.snippets[src.snippet_index];
        src_path = fmt::format("{}{}.metal", base_path, snippet.name);
        dia_path = fmt::format("{}{}.dia", base_path, snippet.name);
        air_path = fmt::format("{}{}.air", base_path, snippet.name);
        bin_path = fmt::format("{}{}.metallib", base_path, snippet.name);
        // write metal source code to temp file
        if (!write_source(src.source_code, src_path)) {
            bytecode.errors.push_back(errmsg_t::error(inp.path, 0, fmt::format("failed to write intermediate file '{}'!", src_path)));
            break;
        }
        // compiler, link, load generated bytecode
        if (!error) {
            if (!mtl_cc(src_path, dia_path, air_path, slang, output)) {
                error = true;
                mtl_parse_errors(output, inp, src.snippet_index, bytecode.errors);
                break;
            }
        }
        if (!error) {
            if (!mtl_link(air_path, bin_path, slang)) {
                error = true;
                mtl_parse_errors(output, inp, src.snippet_index, bytecode.errors);
                break;
            }
        }
        std::vector<uint8_t> data;
        if (!error) {
            if (!read_binary(bin_path, data)) {
                error = true;
                mtl_parse_errors(output, inp, src.snippet_index, bytecode.errors);
                break;
            }
        }
        // if hard error happened there may still have been warnings
        if (!output.empty()) {
            mtl_parse_errors(output, inp, src.snippet_index, bytecode.errors);
        }

        if (!error) {
            bytecode_blob_t blob;
            blob.valid = true;
            blob.snippet_index = src.snippet_index;
            blob.data = std::move(data);
            bytecode.blobs.push_back(std::move(blob));
        }
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

static void d3d_parse_errors(const std::string& output, const input_t& inp, int snippet_index, std::vector<errmsg_t>& out_errors) {
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
        pystring::split(line, tokens, colon);
        if ((tokens.size() > 1) && (pystring::startswith(tokens[1], " error") || pystring::startswith(tokens[1], " warning"))) {
            bool ok = false;
            std::string msg;
            if (tokens.size() > 2) {
                for (int i = 2; i < (int)tokens.size(); i++) {
                    if (msg.empty()) {
                        msg = tokens[i];
                    }
                    else {
                        msg = fmt::format("{}:{}", msg, tokens[i]);
                    }
                }
                ok = true;
            }
            if (ok) {
                if (pystring::startswith(tokens[1], " error")) {
                    out_errors.push_back(errmsg_t::error(inp.path, 0, msg));
                }
                else {
                    out_errors.push_back(errmsg_t::warning(inp.path, 0, msg));
                }
            }
            else {
                // some error during parsing, output the original line so it isn't lost
                out_errors.push_back(errmsg_t::error(inp.path, 0, msg));
            }
        }
    }
}

static bytecode_t d3d_compile(const args_t& args, const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang) {
    bytecode_t bytecode;
    if (!load_d3dcompiler_dll()) {
        bytecode.errors.push_back(errmsg_t::warning(inp.path, 0, fmt::format("failed to load d3dcompiler_47.dll!")));
        return bytecode;
    }
    for (const spirvcross_source_t& src: spirvcross.sources) {
        const snippet_t& snippet = inp.snippets[src.snippet_index];
        ID3DBlob* output = NULL;
        ID3DBlob* errors = NULL;
        const char* compile_target = (snippet.type == snippet_t::VS) ? "vs_5_0" : "ps_5_0";
        d3dcompile_func(
            src.source_code.c_str(),        // pSrcData
            src.source_code.length(),       // SrcDataSize
            NULL,                           // pSourceName
            NULL,                           // pDefines
            NULL,                           // pInclude
            src.refl.entry_point.c_str(),   // entryPoint   
            compile_target,                 // pTarget
            D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_OPTIMIZATION_LEVEL3, /* Flags1 */
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
            bytecode_blob_t blob;
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

bytecode_t bytecode_t::compile(const args_t& args, const input_t& inp, const spirvcross_t& spirvcross, slang_t::type_t slang) {
    bytecode_t bytecode;
    #if defined(__APPLE__)
    if ((slang == slang_t::METAL_MACOS) || (slang == slang_t::METAL_IOS)) {
        bytecode = mtl_compile(args, inp, spirvcross, slang);
    }
    #endif
    #if defined(_WIN32)
    if (slang == slang_t::HLSL5) {
        bytecode = d3d_compile(args, inp, spirvcross, slang);
    }
    #endif
    return bytecode;
}

void bytecode_t::dump_debug() const {
    fmt::print(stderr, "bytecode_t::dump_debug(): FIXME!\n");
}

} // namespace shdc

