# sokol-shdc

Shader-code-generator for sokol_gfx.h

## Feature Overview

sokol-shdc is a shader-cross-compiler and -code-generator command line tool
which translates an 'annotated GLSL' source file into a C header for use
with sokol_gfx.

Shaders are written in 'modern GLSL' (v450) and translated into the following
shader dialects:

- GLSL v100 (for GLES2 and WebGL)
- GLSL v300es (for GLES3 and WebGL2)
- GLSL v330 (for desktop GL)
- HLSL5 (for D3D11), optionally as bytecode
- Metal (for macOS and iOS), optionally as bytecode

This cross-compilation happens via existing Khronos open source projects:

- [glslang](https://github.com/KhronosGroup/glslang): for compiling GLSL to SPIR-V
- [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools): the SPIR-V optimizer is used
to run optimization passes on the intermediate SPIRV (mainly for dead-code elimination)
- [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross): for translating the SPIRV
bytecode to GLSL dialects, HLSL and Metal

Error messages from ```glslang``` are mapped back to the original annotated
source file and converted into GCC or MSVC error formats for integration with
IDEs like Visual Studio, Xcode or VSCode:

![errors in IDE](images/sokol-shdc-errors.png)

Shader files are 'annotated' with custom **@-tags** which add meta-information to
the GLSL source files. This is used for packing vertex- and fragment-shaders
into the same source file, mark and include reusable code blocks, and provide
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


## Build Process Integration with fips.

sokol-shdc can be used standalone as offline-tool from the command
line, but it's much more convenient when integrated right into the
build process. That way the edit-compile-test loop for shader
programming is the same as for regular C or C++ code.

Support for [fips](https://github.com/floooh/fips) projects comes
out-of-the box. For other build systems, oryol-shdc must be called
from a custom build job, or a custom rule to build .h files from
.glsl files.

For fips projects, first add a new dependency to the ```fips.yml``` file of your project:

```yaml
...
imports:
    sokol-tools-bin:
        git: https://github.com/floooh/sokol-tools-bin    
    ...
```

The [sokol-tools-bin](https://github.com/floooh/sokol-tools-bin) repository
contains precompiled 64-bit executables for macOS, Linux and Windows, and the
necessary fips-files to hook the shader compiler into the build projess.

After adding the new dependency to fips.yml, fetch and update the dependencies of your project:

```bash
> ./fips fetch
> ./fips update
```

Finally, for each GLSL shader file, add the cmake macro ```sokol_shader()``` to your application- or library targets:

```cmake
fips_begin_app(triangle windowed)
    ...
    sokol_shader(triangle.glsl ${slang})
fips_end()
```

The ```${slang}``` variable (short for shader-language, but you can call it
any way you want) must resolve to a string with the shader dialects you want
to have generated.

I recommend to initialize this ${slang} variable together with the 
target-platform definitions for sokol_gfx.h For instance, the [sokol-app
samples](https://github.com/floooh/sokol-samples/tree/master/sapp) have the following
block in their CMakeLists.txt file:

```cmake
if (FIPS_EMSCRIPTEN)
    add_definitions(-DSOKOL_GLES3)
    set(slang "glsl300es:glsl100")
elseif (FIPS_RASPBERRYPI)
    add_definitions(-DSOKOL_GLES2)
    set(slang "glsl100")
elseif (FIPS_ANDROID)
    add_definitions(-DSOKOL_GLES3)
    set(slang "glsl300es")
elseif (SOKOL_USE_D3D11)
    add_definitions(-DSOKOL_D3D11)
    set(slang "hlsl5")
elseif (SOKOL_USE_METAL)
    add_definitions(-DSOKOL_METAL)
    if (FIPS_IOS)
        set(slang "metal_ios")
    else()
        set(slang "metal_macos")
    endif()
else()
    if (FIPS_IOS)
        add_definitions(-DSOKOL_GLES3)
        set(slang "glsl300es")
    else()
        add_definitions(-DSOKOL_GLCORE33)
        set(slang "glsl330")
    endif()
endif()
```

After preparing the CMakeLists.txt files like this, run ```fips gen```,
followed by ```fips build``` or ```fips open``` as usual.

The output C header files will be generated *out of source* in the
build directory (not the project directory where the source code under
version control resides). This is because sokol-shdc only generates
the shader dialects needed for the platform the code is compiled for, so the
generated files look different on each platform, or even build config.

## Standalone Usage

## Shader Tags Reference

## Programming Considerations

### Creating shaders and pipeline objects

### Binding images and uniform blocks

### Named or explicit binding

### Uniform blocks and C structs

(type restrictions, type mapping to C/C++, flattened uniform
blocks in GL)