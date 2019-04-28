/*
    compile GLSL to SPIRV, wrapper around https://github.com/KhronosGroup/glslang
*/
#include <stdlib.h>
#include "shdc.h"
#include "fmt/format.h"
#include "pystring.h"
#include "ShaderLang.h"
#include "ResourceLimits.h"
#include "GlslangToSpv.h"
#include "spirv-tools/libspirv.hpp"

namespace shdc {

extern const TBuiltInResource DefaultTBuiltInResource;  /* actual data at end of file */

void spirv_t::initialize_spirv_tools() {
    glslang::InitializeProcess();
}

void spirv_t::finalize_spirv_tools() {
    glslang::FinalizeProcess();
}

/* merge shader snippet source into a single string */
static std::string merge_source(const input_t& inp, const snippet_t& snippet) {
    std::string src = "#version 450\n";
    for (int line_index : snippet.lines) {
        src += fmt::format("{}\n", inp.lines[line_index]);
    }
    return src;
}

/* convert a glslang info-log string to error_t's and append to out_errors */
static void infolog_to_errors(const std::string& log, const input_t& inp, int snippet_index, std::vector<error_t>& out_errors) {
    /*
        format for errors is "[ERROR|WARNING]: [pos=0?]:[line]: message"
        And a last line we need to ignore: "ERROR: N compilation errors. ..."
    */
    const snippet_t& snippet = inp.snippets[snippet_index];

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
                if (snippet_line_index > 1) {
                    snippet_line_index -= 2;
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
                out_errors.push_back(error_t(inp.path, line_index, msg));
            }
            else {
                // some error during parsing, still create an error object so the error isn't lost in the void
                out_errors.push_back(error_t(inp.path, 0, line));
            }
        }
    }
}

/* compile a vertex or fragment shader to SPIRV */
static bool compile(EShLanguage stage, spirv_t& spirv, const std::string& src, const input_t& inp, int snippet_index) {
    const char* sources[1] = { src.c_str() };

    // compile GLSL vertex- or fragment-shader
    glslang::TShader shader(stage);
    // FIXME: add custom defines here: compiler.addProcess(...)
    shader.setStrings(sources, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientOpenGL, 100/*???*/);
    shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
    shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_0);
    shader.setAutoMapBindings(true);
    if (!shader.parse(&DefaultTBuiltInResource, 100, false, EShMsgDefault)) {
        infolog_to_errors(shader.getInfoLog(), inp, snippet_index, spirv.errors);
        infolog_to_errors(shader.getInfoDebugLog(), inp, snippet_index, spirv.errors);
        return false;
    }

    // "link" into a program
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMsgDefault)) {
        infolog_to_errors(program.getInfoLog(), inp, snippet_index, spirv.errors);
        infolog_to_errors(program.getInfoDebugLog(), inp, snippet_index, spirv.errors);
        return false;
    }

    // translate intermediate representation to SPIRV
    const glslang::TIntermediate* im = program.getIntermediate(stage);
    assert(im);
    spv::SpvBuildLogger spv_logger;
    glslang::SpvOptions spv_options;
    // FIXME: generate debug info options
    // FIXME: optimize for size option
    // FIXME??? dissassemble and validate options
    spirv.blobs.push_back(spirv_blob_t(snippet_index));
    glslang::GlslangToSpv(*im, spirv.blobs.back().bytecode, &spv_logger, &spv_options);
    std::string spirv_log = spv_logger.getAllMessages();
    if (!spirv_log.empty()) {
        // FIXME: need to parse string for errors and translate to error_t objects
        fmt::print(spirv_log);
    }
    return true;
}

/* compile all shader-snippets into SPIRV bytecode */
spirv_t spirv_t::compile_glsl(const input_t& inp) {
    spirv_t spirv;

    // compile shader-snippets
    // FIXME: we may need to compile each source for each target shader dialect,
    // in case we want to add a define #define SOKOL_HLSL/MSL/GLSL etc... to the
    // shader source
    int snippet_index = 0;
    for (const snippet_t& snippet: inp.snippets) {
        if (snippet.type == snippet_t::VS) {
            // vertex shader
            std::string src = merge_source(inp, snippet);
            if (!compile(EShLangVertex, spirv, src, inp, snippet_index)) {
                // spirv.errors contains error list
                return spirv;
            }
        }
        else if (snippet.type == snippet_t::FS) {
            // fragment shader
            std::string src = merge_source(inp, snippet);
            if (!compile(EShLangFragment, spirv, src, inp, snippet_index)) {
                // spirv.errors contains error list
                return spirv;
            }
        }
        snippet_index++;
    }
    // when arriving here, no compile errors occured
    // spirv.bytecodes array contains the SPIRV-bytecode
    // for each shader snippet
    return spirv;
}

void spirv_t::dump_debug(const input_t& inp, error_t::msg_format_t err_fmt) const {
    fmt::print(stderr, "spirv_t:\n");
    if (errors.size() > 0) {
        fmt::print(stderr, "  error:\n");
        for (const error_t& err: errors) {
            fmt::print(stderr, "    {}\n", err.as_string(err_fmt));
        }
        fmt::print(stderr, "\n");
    }
    else {
        fmt::print(stderr, "  errors: none\n\n");
    }
    for (const spirv_blob_t& blob : blobs) {
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

const TBuiltInResource DefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,

    /* .limits = */ {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }};

} // namespace shdc
