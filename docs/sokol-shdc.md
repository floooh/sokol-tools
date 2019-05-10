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
necessary fips-files to hook the shader compiler into the build project.

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

```sokol-shdc``` can be invoked from the command line to convert exactly
ony annotated-GLSL source file into exactly one C header file.

Precompiled 64-bit executables for Windows, Linux and macOS are here:

https://github.com/floooh/sokol-tools-bin

Run ```sokol-shdc --help``` to see a short help text and the list of available options.

For a quick test, copy the following into a file named ```shd.glsl```:

```glsl
@vs vs
in vec4 pos;
void main() {
    gl_Position = pos;
}
@end

@fs fs
out vec4 frag_color;
void main() {
    frag_color = vec4(1.0, 0.0, 0.0, 1.0);
}
@end

@program shd vs fs
```

...and then run:

```bash
> ./sokol-shdc --input shd.glsl --output shd.h --slang glsl330:hlsl5:metal_macos
```

...this should generate a C header named ```shd.h``` in the current directory.

## Shader Tags Reference

The following ```@-tags``` can be used in *annotated GLSL* source files:

### @vs [name]

Starts a named vertex shader code block. The code between
the ```@vs``` and the next ```@end``` will be compiled as a vertex shader.

Example:

```glsl
@vs my_vertex_shader
uniform vs_params {
    mat4 mvp;
};

in vec4 position;
in vec4 color0;

out vec4 color;

void main() {
    gl_Position = mvp * position;
    color = color0;
}
@end
```

### @fs [name]

Starts a named fragment shader code block. The code between the ```@fs``` and
the next ```@end``` will be compiled as a fragment shader:

Example:

```glsl
@fs my_fragment_shader
in vec4 color;
out vec4 frag_color;

void main() {
    frag_color = color;
}
@end
```

### @program [name] [vs] [fs]

The ```@program``` tag links a vertex- and fragment-shader into a named
shader program. The program name will be used for naming the generated
```sg_shader_desc``` C struct and a C function to get a pointer to the
generated shader desc.

At least one ```@program``` tag must exist in an annotated GLSL source file.

Example for the above vertex- and fragment-shader snippets:

```glsl
@program my_program my_vertex_shader my_fragment_shader
```

This will generate a C function:

```C
static const sg_shader_desc* my_program_shader_desc(void);
```

### @block [name]

The ```@block``` tag starts a named code block which can be included in
other ```@vs```, ```@fs``` or ```@block``` code blocks. This is useful
for sharing code between functions.

Example for having a common lighting function shared between two fragment
shaders:

```glsl
@block lighting
vec3 light(vec3 base_color, vec3 eye_vec, vec3 normal, vec3 light_vec) {
    ...
}
@end

@fs fs_1
@include_block lighting

out vec4 frag_color;
void main() {
    frag_color = vec4(light(...), 1.0);
}
@end

@fs fs_2
@include_block lighting

out vec4 frag_color;
void main() {
    frag_color = vec4(0.5 * light(...), 1.0);
}
@end
```

### @end

The ```@end``` tag closes a ```@vs```, ```@fs``` or ```@block``` code block.

### @include_block [name]

```@include_block``` includes a ```@block``` into another code block. 
This is useful for sharing code snippets between different shaders.

### @ctype [glsl_type] [c_type]

The ```@ctype``` defines a type-mapping from GLSL to C in uniform blocks for
the GLSL types ```float```, ```vec2```, ```vec3```, ```vec4``` and ```mat4```
(these are the currently valid types for use in GLSL uniform blocks).

Consider the following GLSL uniform block without ```@ctype``` tags:

```glsl
@vs my_vs
uniform shape_uniforms {
    mat4 mvp;
    mat4 model;
    vec4 shape_color;
    vec4 light_dir;
    vec4 eye_pos;
};
@end
```

On the C side, this will be turned into a C struct like this:

```c
typedef struct shape_uniforms_t {
    float mvp[16];
    float model[16];
    float shape_color[4];
    float light_dir[4];
    float eye_pos[4];
} shape_uniforms_t;
```

But what if your C/C++ code uses a math library like HandmadeMath.h or
GLM?

That's where the ```@ctype``` tag comes in. For instance, with HandmadeMath.h
you would add the following two @ctypes at the top of the GLSL file to 
map the GLSL types to their matching hmm_* types:

```glsl
@ctype mat4 hmm_mat4
@ctype vec4 hmm_vec4

@vs my_vs
uniform shape_uniforms {
    mat4 mvp;
    mat4 model;
    vec4 shape_color;
    vec4 light_dir;
    vec4 eye_pos;
};
@end
```

With this change, the uniform block struct on the C is generated like this now:

```c
typedef struct shape_uniforms_t {
    hmm_mat4 mvp;
    hmm_mat4 model;
    hmm_vec4 shape_color;
    hmm_vec4 light_dir;
    hmm_vec4 eye_pos;
} shape_uniforms_t;
```

### @module [name]

The optional ```@module``` tag defines a 'namespace prefix' for all generated
C types, values, defines and functions.

For instance, when adding a ```@module``` tag to the above @ctype-example:

```glsl
@module bla

@ctype mat4 hmm_mat4
@ctype vec4 hmm_vec4

@vs my_vs
uniform shape_uniforms {
    mat4 mvp;
    mat4 model;
    vec4 shape_color;
    vec4 light_dir;
    vec4 eye_pos;
};
@end
```

...the generated C uniform block struct (allong with all other identifiers)
would get a prefix ```bla_```:

```c
typedef struct bla_shape_uniforms_t {
    hmm_mat4 mvp;
    hmm_mat4 model;
    hmm_vec4 shape_color;
    hmm_vec4 light_dir;
    hmm_vec4 eye_pos;
} bla_shape_uniforms_t;
```

### @glsl_option, @hlsl_option, @msl_option

[TODO]


## Programming Considerations

### Creating shaders and pipeline objects

### Binding images and uniform blocks

### Named or explicit binding

### Uniform blocks and C structs

(type restrictions, type mapping to C/C++, flattened uniform
blocks in GL)