/*
    compile GLSL to SPIRV, wrapper around https://github.com/KhronosGroup/glslang
*/
#include "types.h"
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

/* compile a vertex or fragment shader to SPIRV */
static bool compile(EShLanguage stage, spirv_t& spirv, const std::string& src, int snippet_index) {
    const char* sources[1] = { src.c_str() };

    // compile GLSL vertex- or fragment-shader
    glslang::TShader shader(stage);
    // FIXME: add custom defines here: compiler.addProcess(...)
    shader.setStrings(sources, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientOpenGL, 100/*???*/);
    shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
    shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_0);
    if (!shader.parse(&DefaultTBuiltInResource, 100, false, EShMsgDefault)) {
        // FIXME: convert error log to error_t objects
        fmt::print(shader.getInfoLog());
        fmt::print(shader.getInfoDebugLog());
        return false;
    }

    // "link" into a program
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMsgDefault)) {
        // FIXME: convert error log to error_t objects
        fmt::print(program.getInfoLog());
        fmt::print(program.getInfoDebugLog());
        return false;
    }
    if (!program.mapIO()) {
        // FIXME: convert error log to error_t objects
        fmt::print(program.getInfoLog());
        fmt::print(program.getInfoDebugLog());
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
            if (!compile(EShLangVertex, spirv, src, snippet_index)) {
                // spirv.errors contains error list
                return spirv;
            }
        }
        else if (snippet.type == snippet_t::FS) {
            // fragment shader
            std::string src = merge_source(inp, snippet);
            if (!compile(EShLangFragment, spirv, src, snippet_index)) {
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

void spirv_t::dump_debug(const input_t& inp) const {
    fmt::print(stderr, "spirv_t:\n");
    // FIXME: dump errors
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
