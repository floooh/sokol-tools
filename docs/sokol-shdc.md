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

> NOTE: even though bytecode-generation for Metal and D3D11 is described in
this document as if it would already work, this is not yet implemented

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

The generated C header contains:

- human-readable reflection info in a comment block, as well as
copy-pastable example code
- complete ```sg_shader_desc``` structs for each shader dialect
- for each shader program, a C function which returns a pointer to the right
```sg_shader_desc``` for the current sokol-gfx backend (including a runtime
fallback from GLES3/WebGL2 to GLES2/WebGL)
- constants for vertex attribute locations, uniform-block- and image-bind-slots

For instance, creating a shader and pipeline object for the above simple *triangle*
shader program looks like this:

```c
// create a shader object from generated sg_shader_desc:
sg_shader shd = sg_make_shader(triangle_shader_desc());

// create a pipeline object with this shader, and 
// code-generated vertex attribute location constants
// (ATTR_vs_position and ATTR_vs_color0)
pip = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = shd,
    .layout = {
        .attrs = {
            [ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3,
            [ATTR_vs_color0].format = SG_VERTEXFORMAT_FLOAT4
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
out-of-the box. For other build systems, sokol-shdc must be called
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
fips_end_app()
```

The ```${slang}``` variable (short for shader-language, but you can call it
any way you want) must resolve to a string with the shader dialects you want
to generate.

I recommend to initialize this ${slang} variable together with the 
target-platform defines for sokol_gfx.h For instance, the [sokol-app
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

```sokol-shdc``` can be invoked from the command line to convert 
one annotated-GLSL source file into one C header file.

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

### Command Line Reference

- **-h --help**: Print usage information and exit
- **-i --input=[GLSL file]**: The path to the input GLSL file, this must be either
relative to the current working directory, or an absolute path. 
- **-o --output=[C header]**: The path to the generated C header, either relative
to the current working directory, or as absolute path. The target directory must
exist.
- **-l --slang=[shader languages]**: One or multiple output shader languages. If
multiple languages are provided, they must be separated by a **colon**. Valid 
shader language names are:
    - **glsl330**: desktop GL
    - **glsl100**: GLES2 / WebGL
    - **glsl300es**: GLES3 / WebGL2
    - **hlsl5**: D3D11
    - **metal_macos**: Metal on macOS
    - **metal_ios**: Metal on iOS

  For instance, to generate header with support for all supported GLSL dialects:

  ```
  --slang glsl330:glsl100:glsl300es
  ```

- **-b --bytecode**: If possible, compile shaders to bytecode instead of
embedding source code. The restrictions to generate shader bytecode are as
follows:
    - target language must be **hlsl5**, **metal_macos** or **metal_ios**
    - sokol-shdc must run on the respective platforms:
        - **hlsl5**: only possible when sokol-shdc is running on Windows
        - **metal_macos, metal_ios**: only possible when sokol-shdc is running on macOS

  ...if these restrictions are not met, sokol-shdc will fall back to generating
  shader source code without returning an error.
- **-e --errfmt=[gcc,msvc]**: set the error message format to be either GCC-compatible
or Visual-Studio-compatible, the default is **gcc**
- **-g --genver=[integer]**: set a version number to embed in the generated header,
this is useful to detect whether all shader files need to be recompiled because 
the tooling has been updated (sokol-shdc will not check this though, this must be
done in the build-system-integration)
- **-n --noifdef**: by default, the C header generator will surround all 3D-backend-
specific code with an **#ifdef/#endif** pair which checks for the sokol-gfx
backend defines:
    - SOKOL_GLCORE33
    - SOKOL_GLES2
    - SOKOL_GLES3
    - SOKOL_D3D11
    - SOKOL_METAL

  Sometimes this is useful, sometimes it isn't. You can disable those
  backend-checks with the **--noifdef** option. One situation where it makes
  sense to disable the ifdefs is for application that use GLES3/WebGL2, but
  must be able to fall back to GLES2/WebGL.
- **-d --dump**: Enable verbose debug output, this basically dumps all internal
information to stdout. Useful for debugging and understanding how sokol-shdc
works, but not much else :)

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

With this change, the uniform block struct on the C side is generated like this now:

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

### @glsl_options, @hlsl_options, @msl_options

These tags can be used to define per-shader/per-language options for SPIRV-Cross
when compiling SPIR-V to GLSL, HLSL or MSL.

GL, D3D and Metal have different opinions where the origin of an image
is, or whether clipspace-z goes from 0..+1 or from -1..+1, and the
option-tags allow fine-control over those aspects with the following
arguments:

- **fixup_clipspace**:
    - GLSL: In vertex shaders, rewrite [0, w] depth (Vulkan/D3D style) to [-w, w] depth (GL style).
    - HLSL: In vertex shaders, rewrite [-w, w] depth (GL style) to [0, w] depth.
    - MSL: In vertex shaders, rewrite [-w, w] depth (GL style) to [0, w] depth.
- **flip_vert_y**: Inverts gl_Position.y or equivalent. (all shader languages)

Currently, ```@glsl_option```, ```@hlsl_option``` and ```@msl_option``` are only
allowed inside ```@vs, @end``` blocks.

Example from the [mrt-sapp sample](https://floooh.github.io/sokol-html5/wasm/mrt-sapp.html),
this renders a fullscreen-quad to blit an offscreen-render-target image to screen,
which should be flipped vertically for GLSL targets:

```glsl
@vs vs_fsq
@glsl_options flip_vert_y
...
@end
```

## Programming Considerations

### Target Shader Language Defines

In the input GLSL source, use the following checks to conditionally
compile code for the different target shader languages:

```GLSL
#if SOKOL_GLSL
    // target shader language is a GLSL dialect
#endif

#if SOKOL_HLSL
    // target shader language is HLSL
#endif

#if SOKOL_MSL
    // target shader language is MetalSL
#endif
```

Normally, SPIRV-Cross does its best to 'normalize' the differences between
GLSL, HLSL and MSL, but sometimes it's still necessary to write different
code for different target languages.

These checks are evaluated by the initial compiler pass which compiles
GLSL v450 to SPIR-V, and only make sense inside ```@vs```, ```@fs``` 
and ```@block``` code-blocks.

### Creating shaders and pipeline objects

The generated C header will contain one function for each shader program
which returns a pointer to a completely initialized static sg_shader_desc
structure, so creating a shader object becomes a one-liner.

For instance, with the following ```@program``` in the 
GLSL file:

```glsl
@program shape vs fs
```

The following code would be used to create the shader object:
```c
sg_shader shd = sg_make_shader(shape_shader_desc());
```

When creating a pipeline object, the shader code generator will
provide integer constants for the vertex attribute locations.

Consider the following vertex shader inputs in the GLSL source code:

```glsl
@vs vs
in vec4 position;
in vec3 normal;
in vec2 texcoords;
...
@end
```

The vertex attribute description in the ```sg_pipeline_desc``` struct
could look like this (note the attribute indices names ```ATTR_vs_position```, etc...):

```c
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        .attrs = {
            [ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3,
            [ATTR_vs_normal].format = SG_VERTEXFORMAT_BYTE4N,
            [ATTR_vs_texcoords].format = SG_VERTEXFORMAT_SHORT2
        }
    },
    ...
});
```

It's also possible to provide explicit vertex attribute location in the
shader code:

```glsl
@vs vs
layout(location=0) in vec4 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texcoords;
...
@end
```

When the shader code uses explicit location, the generated location
constants can be ignored on the C side:

```c
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        .attrs = {
            [0].format = SG_VERTEXFORMAT_FLOAT3,
            [1].format = SG_VERTEXFORMAT_BYTE4N,
            [2].format = SG_VERTEXFORMAT_SHORT2
        }
    },
    ...
});
```

### Binding uniforms blocks

Similar to the vertex attribute location constants, the C code generator
also provides bind slot constants for images and uniform blocks.

Consider the following uniform block in GLSL:

```glsl
uniform vs_params {
    mat4 mvp;
};
```

The C header code generator will create a C struct and a 'bind slot' constant
for the uniform block:

```c
#define SLOT_vs_params (0)
typedef struct vs_params_t {
    hmm_mat4 mvp;
} vs_params_t;
```

...which both are used in the ```sg_apply_uniforms()``` call like this:

```c
vs_params_t vs_params = {
    .mvp = ...
};
sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
```

The GLSL uniform block can have an explicit bind slot:

```glsl
layout(binding=0) uniform vs_params {
    mat4 mvp;
};
```

In this case the generated bind slot constant can be ignored since it has
been explicitely defined as 0:

```c
vs_params_t vs_params = {
    .mvp = ...
};
sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_params, sizeof(vs_params));
```

### Binding images

When using a texture sampler in a GLSL vertex- or fragment-shader like this:

```glsl
uniform sampler2D tex;
...
```

The C header code generator will create bind-slot constants with the same
naming convention as uniform blocks:

```C
#define SLOT_tex (0)
```

This is used in the ```sg_bindings``` struct as index into the ```vs_images```
or ```fs_images``` bind slot array:

```c
sg_apply_bindings(&(sg_bindings){
    .vertex_buffers[0] = vbuf,
    .fs_images[SLOT_tex] = img
});
```

Just like with uniform blocks, texture sampler bind slots can
be defined explicitely in the GLSL shader:

```glsl
layout(binding=0) uniform sampler2D tex;
...
```

...in this case the code-generated bind-slot constant can be ignored:

```c
sg_apply_bindings(&(sg_bindings){
    .vertex_buffers[0] = vbuf,
    .fs_images[0] = img
});
```

### Uniform blocks and C structs

There are a few caveats with uniform blocks:

- Member types are currently restricted to:
    - float
    - vec2
    - vec3
    - vec4
    - mat4

    This limitation is currently also present in sokol_gfx.h
    itself (SG_UNIFORMTYPE_*). More float-based types like
    mat2 and mat3 will most likely be added in the future, 
    but there's currently no portable way to support integer
    types across all backends.

- In GL, uniform blocks will be 'flattened' to arrays of vec4,
this allows to update uniform data with a single call to glUniform4v
per uniform block, no matter how many members a uniform block
actually has
