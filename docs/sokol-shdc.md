# sokol-shdc

Shader-code-generator for sokol_gfx.h

## Feature Overview

sokol-shdc is a shader-cross-compiler and -code-generator which translates
an 'annotated GLSL' file into a C header with for use with sokol_gfx.

Shaders are written in 'modern GLSL' (v450) and translated into the following
shader dialects:

- GLSL v100 (for GLES2 and WebGL)
- GLSL v300es (for GLES3 and WebGL2)
- GLSL v330 (for desktop GL)
- HLSL5 (for D3D11), optionally as bytecode
- Metal (for macOS and iOS), optionally as bytecode

This cross-compilation happens via existing Khronos open source projects:

- https://github.com/KhronosGroup/glslang: for compiling GLSL to SPIR-V
- https://github.com/KhronosGroup/SPIRV-Tools: the SPIR-V optimizer is used
to run optimization passes on the intermediate SPIRV
- https://github.com/KhronosGroup/SPIRV-Cross: for translating the SPIRV
bytecode to GLSL dialects, HLSL and Metal

Error messages from ```glslang``` are mapped back to the original annotated
source file and converted into GCC or MSVC error formats for integration with
IDEs like Visual Studio, Xcode or VSCode:

![errors in IDE](images/sokol-shdc-errors.png)

Shader files are 'annotated' with custom @-tags which add meta-information to
the GLSL source files. This is used for packing vertex- and fragment-shaders
into the same code, mark and include reusable code blocks, and provide
additional information for the C code-generation (note the ```@vs```,
```@fs```, ```@end``` and ```@program``` tags):

```glsl
@vs vs
in vec4 position;
in vec4 color0;
out vec4 color;
void main() {
    gl_Position = position;
    color = color0;
}
@end

@fs fs
in vec4 color;
out vec4 frag_color;
void main() {
    frag_color = color;
}
@end

@program triangle vs fs
```

The generated C header contains the following data:

- human-readable reflection info in a comment block, as well as
copy-pastable example code
- complete ```sg_shader_desc``` structs for each shader dialect
- for each shader program a C function which returns a pointer to the right ```sg_shader_desc``` for the current sokol-gfx backend (including a runtime fallback from GLES3/WebGL2 to GLES2/WebGL)
- constants for vertex attribute locations, uniform-block- and image-bind-slots

For instance, creating a shader and pipeline object for the above simple *triangle*
shader program looks like this:

```c
// create a shader object from generated sg_shader_desc:
sg_shader shd = sg_make_shader(triangle_shader_desc());

// create a pipeline object with this shader, and 
// code-generated vertex attribute location constants
// (vs_position and vs_color0)
pip = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = shd,
    .layout = {
        .attrs = {
            [vs_position].format = SG_VERTEXFORMAT_FLOAT3,
            [vs_color0].format = SG_VERTEXFORMAT_FLOAT4
        }
    }
});
```


## Build Process Integration

### ...with fips and cmake

### ...with other build systems

## Standalone Usage

## Shader Tags Reference

## Programming Considerations

### Creating shaders and pipeline objects

### Binding images and uniform blocks

### Named or explicit binding

### Uniform blocks and C structs

(type restrictions, type mapping to C/C++, flattened uniform
blocks in GL)