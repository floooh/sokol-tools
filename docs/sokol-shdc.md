# sokol-shdc

Shader-code-generator for sokol_gfx.h

## Table of Content

- [Feature Overview](#feature-overview)
- [Build Process Integration with fips](#build-process-integration-with-fips)
- [Standalone Usage](#standalone-usage)
- [Shader Tags Reference](#shader-tags-reference)
- [Shader Authoring Considerations](#shader-authoring-considerations)
- [Runtime Inspection](#runtime-inspection)

## Feature Overview

sokol-shdc is a shader-cross-compiler and -code-generator command line tool
which translates an 'annotated GLSL' source file into a C header (or other
output formats) for use with sokol-gfx.

Shaders are written in 'Vulkan-style GLSL' (version 450 with separate texture and sampler
uniforms) and translated into the following shader dialects:

- GLSL v300es (for GLES3 and WebGL2)
- GLSL v310es (for GLES3.1, note that this currently isn't supported by sokol_gfx.h)
- GLSL v410 (for desktop GL without compute shader support)
- GLSL v430 (for desktop GL with compute shader support)
- HLSL4 or HLSL5 (for D3D11), optionally as bytecode
- Metal (for macOS and iOS), optionally as bytecode
- WGSL (for WebGPU)

This cross-compilation happens via existing Khronos and Google open source projects:

- [glslang](https://github.com/KhronosGroup/glslang): for compiling GLSL to SPIR-V
- [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools): the SPIR-V optimizer is used
to run optimization passes on the intermediate SPIRV (mainly for dead-code elimination)
- [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross): for translating the SPIRV
bytecode to GLSL dialects, HLSL and Metal
- [Tint](https://dawn.googlesource.com/tint): for translating SPIR-V to WGSL

sokol-shdc supports the following output formats:

- C for integration with [sokol_gfx.h](https://github.com/floooh/sokol)
- Zig for integration with [sokol-zig](https://github.com/floooh/sokol-zig)
- Rust for integration with [sokol-rust](https://github.com/floooh/sokol-rust)
- Odin for integration with [sokol-odin](https://github.com/floooh/sokol-odin)
- Nim for integration with [sokol-nim](https://github.com/floooh/sokol-nim)
- D for integration with [sokol-d](https://github.com/kassane/sokol-d)
- Jai for integration with [sokol-jai](https://github.com/colinbellino/sokol-jai)
- 'raw' output files in GLSL, MSL and HLSL along with reflection data in YAML files

Error messages from `glslang` are mapped back to the original annotated
source file and converted into GCC or MSVC error formats for integration with
IDEs like Visual Studio, Xcode or VSCode:

![errors in IDE](images/sokol-shdc-errors.png)

Input shader files are 'annotated' with custom **@-tags** which add meta-information to
the GLSL source files. This is used for packing vertex- and fragment-shaders
into the same source file, wrap and include reusable code blocks, and provide
additional information for the C code-generation (note the `@vs`,
`@fs`, `@end` and `@program` tags):

```glsl
@vs vs
layout(binding=0) uniform vs_params {
    mat4 mvp;
};

in vec4 position;
in vec2 texcoord0;

out vec2 uv;

void main() {
    gl_Position = mvp * position;
    uv = texcoord0;
}
@end

@fs fs
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;

in vec2 uv;
out vec4 frag_color;

void main() {
    frag_color = texture(sampler2D(tex,smp), uv);
}
@end

@program texcube vs fs
```

Note: For compatibility with other tools which parse GLSL, `#pragma sokol` may be used to prefix the tags. For example, the final line above could have also been written as:

```glsl
#pragma sokol @program texcube vs fs
```

A generated C header contains (similar information is generated for the other output languages):

- human-readable reflection info in a comment block
- a C struct for each shader uniform block and storage buffer binding
- constants for vertex attribute locations, uniform block, image, sampler
  and storage buffer binding indices
- for each shader program, a C function which returns a pointer to a
```sg_shader_desc``` for a specific sokol-gfx backend which can be passed
directly into `sg_make_shader()`
- optionally a set of C functions for runtime inspection of the generated vertex
attributes, image bind slots and uniform-block structs

For instance, creating a shader and pipeline object for the above simple *texcube*
shader program looks like this:

```c
// create a shader object from generated sg_shader_desc:
sg_shader shd = sg_make_shader(texcube_shader_desc(sg_query_backend()));

// create a pipeline object with this shader, and
// code-generated vertex attribute location constants
// (ATTR_vs_position and ATTR_vs_color0)
pip = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = shd,
    .layout = {
        .attrs = {
            [ATTR_texcube_position].format = SG_VERTEXFORMAT_FLOAT3,
            [ATTR_texcube_texcoord0].format = SG_VERTEXFORMAT_FLOAT2,
        }
    }
});
```

...and then applying resource bindings and uniform updates looks like this:

```c
sg_apply_bindings(&(sg_bindings){
    .vertex_buffers[0] = vbuf,
    .images[IMG_tex] = img,
    .samplers[SMP_smp] = smp,
});
const vs_params_t vs_params = {
    .mvp = ...,
};
sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
```

## Build Process Integration with fips

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

There are also other versions of the sokol_shader() macro:

- **sokol_shader_debuggable([filename] [slang])**: same as ```sokol_shader()``` but
  doesn't create binary blobs for HLSL and MSL. This allows source-level debugging
  in graphics debuggers.
- **sokol_shader_with_reflection([filename] [slang])**: same as ```sokol_shader()``` but
  calls sokol-shdc with the ```--reflection``` command line option, which generates
  additional runtime inspection functions
- **sokol_shader_variant([filename] [slang] [module] [defines])**: this macro additionally passed
  the ```--module``` and ```-defines``` command line arguments and allows to create a
  conditionally compiled variant of the GLSL input file, the output file will be called
  [filename].[module].h
- **sokol_shader_variant_with_reflection([filename] [slang] [module] [defines])**: same
  as ```sokol_shader_variant()```, but additionally adds the ```--reflection``` command
  line argument for generating runtime inspection functions

I recommend to initialize the `${slang}` variable together with the
target-platform defines for sokol_gfx.h For instance, the [sokol-app
samples](https://github.com/floooh/sokol-samples/tree/master/sapp) have the following
block in their CMakeLists.txt file:

```cmake
if (FIPS_EMSCRIPTEN)
    add_definitions(-DSOKOL_GLES3)
    set(slang "glsl300es")
elseif (FIPS_ANDROID)
    add_definitions(-DSOKOL_GLES3)
    set(slang "glsl300es")
elseif (SOKOL_USE_D3D11)
    add_definitions(-DSOKOL_D3D11)
    set(slang "hlsl5")
elseif (SOKOL_USE_METAL)
    add_definitions(-DSOKOL_METAL)
    if (FIPS_IOS)
        set(slang "metal_ios:metal_sim")
    else()
        set(slang "metal_macos")
    endif()
else()
    if (FIPS_IOS)
        add_definitions(-DSOKOL_GLES3)
        set(slang "glsl300es")
    else()
        add_definitions(-DSOKOL_GLCORE)
        if (FIPS_MACOS)
            set(slang "glsl410")
        else()
            set(slang "glsl410")
        endif()
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
> ./sokol-shdc --input shd.glsl --output shd.h --slang glsl430:hlsl5:metal_macos
```

...this should generate a C header named ```shd.h``` in the current directory.

### Command Line Reference

- **-h --help**: Print usage information and exit
- **-i --input=[GLSL file]**: The path to the input shader file in 'annotated
  GLSL' format, this must be either relative to the current working directory,
  or an absolute path.
- **-o --output=[path]**: The path to the generated output source file, either
  relative to the current working directory, or as absolute path. The target
  directory must exist, note that some output generators may generate
  more than one output file, in that case the -o argument is used
  as the base path
- **-t --tmpdir=[path]**: Optional path to a directory used for storing
  intermediate files when generating Metal bytecode. If no separate temporary
  directory is provided, intermediate files will be written to the same
  directory as the generated C header defined via *--output*. In both cases,
  the target directory must exist.
- **-l --slang=[shader languages]**: One or multiple output shader languages.
  If multiple languages are provided, they must be separated by a **colon**.
  Valid shader language names are:
    - **glsl410**: desktop GL 4.1 (e.g. macOS: no SSBOs and compute shaders)
    - **glsl430**: desktop GL 4.3
    - **glsl300es**: GLES3.0 / WebGL2
    - **glsl310es**: GLES3.1 (currently not supported by sokol_gfx.h)
    - **hlsl4**: D3D11
    - **hlsl5**: D3D11
    - **metal_macos**: Metal on macOS
    - **metal_ios**: Metal on iOS device
    - **metal_sim**: Metal on iOS simulator
    - **wgsl**: WebGPU

  For instance, to generate header with support for Metal on macOS and desktop GL:

  ```
  --slang glsl430:metal_macos
  ```

- **-b --bytecode**: If possible, compile shaders to bytecode instead of
embedding source code. The restrictions to generate shader bytecode are as
follows:
    - target language must be **hlsl4**, **hlsl5**, **metal_macos** or **metal_ios**
    - sokol-shdc must run on the respective platforms:
        - **hlsl4, hlsl5**: only possible when sokol-shdc is running on Windows
        - **metal_macos, metal_ios**: only possible when sokol-shdc is running on macOS

  ...if these restrictions are not met, sokol-shdc will fall back to generating
  shader source code without returning an error. Note that the **metal_sim**
  target for the iOS simulator doesn't support generating bytecode, this
  will always emit Metal source code.
- **-f --format=[sokol,sokol_impl,...]**: set output backend (default: **sokol**)
    - **sokol**: Generate a C header where data is declared as ```static``` and
      functions are declared as ```static inline```. If this header is included
      multiple times, you should be aware that the executable may contain duplicate data.
    - **sokol_impl**: This generates an STB-style header. In exactly one place where
      the header is included, the define ```SOKOL_SHDC_IMPL``` must be defined
      to compile the implementation, all other places, only the declarations
      will be included.
    - **sokol_decl**: This is a special backward-compatible mode and shouldn't
      be used. In this mode, data is declared ```static``` and functions are
      declared ```static inline```, and the implementation is included when
      the ```SOKOL_SHDC_DECL``` is *not* defined
    - **bare**: compiled shader code is written as plain text or
      binary files. For each combination of shader program and target language,
      a file name based on *--output* is constructed as follows:
        - **glsl**: *.frag.glsl and *.vert.glsl
        - **hlsl**: *.frag.hlsl and *.vert.hlsl, or *.fxc for bytecode
        - **metal**: *.frag.metal and *.vert.metal, or *.metallib for bytecode
    - **bare_yaml**: like bare, but also creates a YAML file with shader reflection information.
    - **sokol_zig**: generates output for the [sokol-zig bindings](https://github.com/floooh/sokol-zig/)
    - **sokol_odin**: generates output for the [sokol-odin bindings](https://github.com/floooh/sokol-odin)
    - **sokol_nim**: generates output for the [sokol-nim bindings](https://github.com/floooh/sokol-nim)
    - **sokol_rust**: generates output for the [sokol-rust bindings](https://github.com/floooh/sokol-rust)
    - **sokol_d**: generates output for the [sokol-d bindings](https://github.com/kassane/sokol-d)
    - **sokol_c3**: generates output for the [sokol-c3 bindings](https://github.com/floooh/sokol-c3)
    - **sokol_jai**: generates output for the [sokol-jai bindings](https://github.com/colinbellino/sokol-jai)

  Note that some options and features of sokol-shdc can be contradictory to
  (and thus, ignored by) backends. For example, the **bare** backend only
  writes shader code, and disregards all other information.
- **-e --errfmt=[gcc,msvc]**: set the error message format to be either GCC-compatible
or Visual-Studio-compatible, the default is **gcc**
- **-g --genver=[integer]**: set a version number to embed in the generated header,
this is useful to detect whether all shader files need to be recompiled because
the tooling has been updated (sokol-shdc will not check this though, this must be
done in the build-system-integration)
- **--ifdef**: this tells the code generator to wrap 3D-backend-specific
code into **#ifdef/#endif** pairs using the sokol-gfx backend-selection defines:
    - SOKOL_GLCORE
    - SOKOL_GLES3
    - SOKOL_D3D11
    - SOKOL_METAL
    - SOKOL_WGPU
- **-d --dump**: Enable verbose debug output, this basically dumps all internal
information to stdout. Useful for debugging and understanding how sokol-shdc
works, but not much else :)
- **--defines=[define1:define2:define3]**: a colon-separated list of
preprocessor defines for the initial GLSL-to-SPIRV compilation pass
- **--module=[name]**: a command-line override for the ```@module``` keyword
- **--reflection**: if present, code-generate additional runtime-inspection functions (not that this is not supported by all
  code generation backends)
- **--save-intermediate-spirv**: debug feature to save out the intermediate SPIRV blob, useful for debug inspection
- **--no-log-cmdline**: don't log the command line to the output file (useful when the output is committed to
  version control and sokol-shdc is called with absolute input/output paths)

## Shader Tags Reference

The following ```@-tags``` can be used in *annotated GLSL* source files:

### @vs [name]

Starts a named vertex shader code block. The code between
the ```@vs``` and the next ```@end``` will be compiled as a vertex shader.

Example:

```glsl
@vs my_vertex_shader
layout(binding=0) uniform vs_params {
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
the next ```@end``` will be compiled as a fragment shader.

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

### @cs [name]

Starts a named compute shader code block. The code between `@cs` and
the next `@end` will be compiled as a compute shader.

Example:

```glsl
@cs my_compute_shader

layout(binding=0) uniform cs_params {
    float dt;
    int num_particles;
};

struct particle {
    vec4 pos;
    vec4 vel;
};

layout(binding=0) buffer cs_ssbo {
    particle prt[];
};

layout(local_size_x=64, local_size_y=1, local_size_z=1) in;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= num_particles) {
        return;
    }
    vec4 pos = prt[idx].pos;
    vec4 vel = prt[idx].vel;
    vel.y -= dt;
    pos += vel * dt;
    prt[idx].pos = pos;
    prt[idx].vel = vel;
}
@end
```

### @program [name] [vs] [fs]
### @program [name] [cs]

The ```@program``` tag links stage shader functions into a shader program
consisting of either a vertex- and fragment-shader, or just a compute-shader.

The program name will be used for naming the generated ```sg_shader_desc``` C
struct and a C function to get a pointer to the generated shader desc.

At least one ```@program``` tag must exist in an annotated GLSL source file.

Example for the above shader snippets:

```glsl
// vertex- and fragment-shader
@program my_program my_vertex_shader my_fragment_shader
// ...or just a compute shader
@program my_program my_compute_shader
```

This will generate a C function:

```C
static const sg_shader_desc* my_program_shader_desc(sg_backend backend);
```

### @block [name]

The `@block` tag starts a named code block which can be included in
other `@vs`, `@fs`, `@cs` or `@block` code blocks. This is useful
for sharing code between shaders.

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

The `@end` tag closes a `@vs`, `@fs`, `@cs` or `@block` code block.

### @include_block [name]

```@include_block``` includes a ```@block``` into another code block.
This is useful for sharing code snippets between different shaders.

### @include [path]

Include a file from the files system. `@include` takes one argument:
the path of the file to be included. The path must be relative to the directory
where the top-level source file is located and must not contain whitespace or
quotes. The ```@include``` tag can appear inside or outside a code block:

```glsl
@ctype mat4 hmm_mat4

@vs vs
@include bla/vs.glsl
@end

@include fs.glsl

@program cube vs fs
```

### @ctype [glsl_type] [c_type]

The `@ctype` tag defines a type-mapping from GLSL to C or C++ in uniform blocks
for the GLSL types `float`, `vec2`, `vec3`, `vec4`, `int`, `int2`, `int3`,
`int4`, and `mat4` (these are the currently valid types for use in GLSL uniform
blocks).

Consider the following GLSL uniform block without `@ctype` tags:

```glsl
@vs my_vs
layout(binding=0) uniform shape_uniforms {
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

That's where the `@ctype` tag comes in. For instance, with HandmadeMath.h
you would add the following two @ctypes at the top of the GLSL file to
map the GLSL types to their matching hmm_* types:

```glsl
@ctype mat4 hmm_mat4
@ctype vec4 hmm_vec4

@vs my_vs
layout(binding=0) uniform shape_uniforms {
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

Note that the mapped C/C++ types must have the following byte sizes:

- `mat4`: 64 bytes
- `vec4`, `int4`: 16 bytes
- `vec3`, `int3`: 12 bytes
- `vec2`, `int2`: 8 bytes
- `float`, `int`: 4 bytes

Explicit padding bytes will be included as needed by the code generator.

### @header ...

The `@header` tag allows to inject target-language specific statements
before the generated code. This is for instance useful to include any
required dependencies.

For instance to add a **C** header include:

```glsl
@header #include "path/to/header.h"
@header #include "path/to/other_header.h"
```

This will result in the following generated C code:

```c
#include "path/to/header.h"
#include "path/to/other_header.h"
```

### @module [name]

The optional `@module` tag defines a 'namespace prefix' for all generated
C types, values, defines and functions.

For instance, when adding a `@module` tag to the above @ctype-example:

```glsl
@module bla

@ctype mat4 hmm_mat4
@ctype vec4 hmm_vec4

@vs my_vs
layout(binding=0) uniform shape_uniforms {
    mat4 mvp;
    mat4 model;
    vec4 shape_color;
    vec4 light_dir;
    vec4 eye_pos;
};
@end
```

...the generated C uniform block struct (allong with all other identifiers)
would get a prefix `bla_`:

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

Currently, `@glsl_options`, `@hlsl_options` and `@msl_options` are only
allowed inside `@vs, @end` blocks.

Example from the [mrt-sapp sample](https://floooh.github.io/sokol-html5/mrt-sapp.html),
this renders a fullscreen-quad to blit an offscreen-render-target image to screen,
which should be flipped vertically for GLSL targets:

```glsl
@vs vs_fsq
@glsl_options flip_vert_y
...
@end
```

### @image_sample_type [texture] [type]

Allows to provide a hint to the reflection code generator about the 'image sample type' of a texture
This corresponds to the sokol-gfx enum `sg_image_sample_type`, and the WebGPU type `GPUTextureSampleType`.

Valid values for `type` are:

- `float`
- `unfilterable-float`
- `sint`
- `uint`
- `depth`

Note that the annotation is usually only required for the `unfilterable-float` type, since all other
types can be inferred from the shader code.

The `unfilterable_float` annotation is required for floating point texture formats which can not be filtered
(like RGBA32F).

Example from https://github.com/floooh/sokol-samples/blob/master/sapp/ozz-skin-sapp.glsl:

```glsl
@image_sample_type joint_tex unfilterable_float
layout(binding=0) uniform texture2D joint_tex;
```

### @sampler_type [sampler] [type]

Similar to `@image_sample_type`, the tag `@sampler_type` allows to provide a hint to the code generator
about the `sampler type` of a GLSL sampler object. This corresponds to the sokol-gfx enum
`sg_sampler_type`, and the WebGPU type `GPUSamplerBindingType`.

Valid values for `type` are:

- `filtering`
- `nonfiltering`
- `comparison`

The annotation is usually only required for the `non-filtering` type, since all other types can
be inferred from the shader code.

The `non-filtering` annoation is required for samplers that used together with `unfilterable-float`
textures.

Example from https://github.com/floooh/sokol-samples/blob/master/sapp/ozz-skin-sapp.glsl:

```glsl
@sampler_type smp nonfiltering
layout(binding=0) uniform sampler smp;
```

## Shader Authoring Considerations

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

#if SOKOL_WGSL
    // target shader language is WGSL
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
sg_shader shd = sg_make_shader(shape_shader_desc(sg_query_backend()));
```

When creating a pipeline object, the shader code generator will
provide integer constants for the vertex attribute locations, unique
for each `@program`.

Consider the following vertex shader inputs in the GLSL source code
and `@program` definition:

```glsl
@vs vs
in vec4 position;
in vec3 normal;
in vec2 texcoords;
...
@end
...
@program shd vs fs
```

The vertex attribute description in the `sg_pipeline_desc` struct
could look like this (note the attribute indices names `ATTR_shd_position`, etc...):

```c
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .layout = {
        .attrs = {
            [ATTR_shd_position].format = SG_VERTEXFORMAT_FLOAT3,
            [ATTR_shd_normal].format = SG_VERTEXFORMAT_BYTE4N,
            [ATTR_shd_texcoords].format = SG_VERTEXFORMAT_SHORT2
        }
    },
    ...
});
```

It's also possible to provide explicit vertex attribute location in the
shader code, but be aware that the explicitly defined locations must
not have gaps:

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

### Defining resource bind slots

All GLSL resource types (uniform blocks, textures, samplers and storage buffers)
must be annotated with explicit bind slots via `layout(binding=N)`. Each resource
type has its own binding space which directly maps to sokol_gfx.h bindslots:

- uniform blocks:
    - `layout(binding=N) uniform { ... }` where `(N >= 0) && (N < 8)`
    - N maps to to the ub_slot params in `sg_apply_uniforms(N, ...)`
- textures:
    - `layout(binding=N) texture2D tex;` where `(N >= 0) && (N < 16)`
    - N maps to the image index in `sg_bindings.images[N]`
- samplers:
    - `layout(binding=N) sampler smp;` where `(N >= 0) && (N < 16)`
    - N maps to the sampler index in `sg_bindings.samplers[N]`
- storage buffers:
    - `layout(binding=N) readonly buffer ssbo { ... }` where `(N >= 0) && (N < 8)`
    - N maps to the storage buffer index in `sg_bindings.storage_buffers[N]`

Bindings must be unique per resource type within a shader `@program` across
shader stages, and all resources of the same type and name across all programs
in a shader source file must have matching bindings.

Violations against these rules are checked by sokol-shdc and will result in
compilation errors.

Note that bind slots are allowed to have gaps. This allows to map specific types of resources
(like diffuse vs normal-map textures) to specific bindslots even if a shader only
uses a subset of those resources. This allows to use the same `sg_bindings` struct for
different related shaders (like the different shader variants of the same material).

### Binding uniforms blocks

Uniform blocks must be annotated with `layout(binding=N)` where `(N >=0) && (N < 8)`.
Those bindings directly map to the `ub_slot` parameter in `sg_apply_uniforms(int ub_slot, ...)`.
Uniform block bindings must be unique across shader stages of a `@program`.

The sokol-shdc code generation will create a bind slot constant for each
uniquely named uniform block in a shader source file.

Consider the following uniform blocks in the vertex and fragment shader:

```glsl
@ctype mat4 hmm_mat4
@ctype vec4 hmm_vec4

@vs
layout(binding=0) uniform vs_params {
    mat4 mvp;
};
...
@end

@fs
layout(binding=1) uniform fs_params {
    vec4 color;
};
...
@end
```

The C header code generator will create two C struct and bindslot constants
looking like this:

```c
#define UB_vs_params (0)
#define UB_fs_params (1)
typedef struct vs_params_t {
    hmm_mat4 mvp;
} vs_params_t;
typedef struct fs_params_t {
    hmm_vec4 mvp;
} fs_params_t;
```

...which can be used in `sg_apply_uniforms()` calls like this:

```c
vs_params_t vs_params = {
    .mvp = ...
};
fs_params_t fs_params = {
    .color = ...
};
sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
sg_apply_uniforms(UB_fs_params, &SG_RANGE(fs_params));
```

Since the bind slots have been explicitly defined in the shader code, you
may also ignore the generated bind slot constants, but in that case you need
to make to fix up the C code when changing the bind slots in the shader code.

```c
sg_apply_uniforms(0, &SG_RANGE(vs_params));
sg_apply_uniforms(1, &SG_RANGE(fs_params));
```

### Binding images and samplers

Textures and samplers must be defined separately in the input GLSL (this is
also known as 'Vulkan-style GLSL'). Just as with uniform blocks, you must
define explicit bind slots via `layout(binding=N)`. Textures and samplers
have separate bindslot spaces, with a range of `(N >=0) && (N < 16)`.

```glsl
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;
```

The resource binding slot for texture and sampler uniforms is available
as code-generated constant:

```C
#define IMG_tex (0)
#define SMP_smp (0)
```

This is used in the `sg_bindings` struct as index into the `.images[]`,
and `.samplers[]` arrays:

```c
sg_apply_bindings(&(sg_bindings){
    .vertex_buffers[0] = vbuf,
    .images[IMG_tex] = img,
    .samplers[SMP_smp] = smp,
});
```

Instead of the generated constants you can also use the explicit binding
numbers defined in the shader source file (but when those are changed you
must not forget also fixing the C side):

```c
sg_apply_bindings(&(sg_bindings){
    .vertex_buffers[0] = vbuf,
    .images[0] = img,
    .samplers[0] = smp,
});
```

### Binding storage buffers

Note the following restrictions:

- storage buffers are not supported for the output formats `glsl300es` and `glsl410`
- in vertex- and fragment-shaders, only `readonly` bindings are supported

In the input GLSL, define a storage buffer like this:

```glsl
struct sb_vertex {
    vec4 pos;
    vec4 color;
};

// a readonly 'input buffer'
layout(binding=0) readonly buffer in_ssbo {
    sb_vertex vtx[];
};

// a read/write 'output buffer'
layout(binding=1) buffer out_ssbo {
    sb_vertex vtx[];
};
```

The binding range for storage buffers is `(N >= 0) && (N < 8)` within one
shader program and across all shader stages. `N` directly corresponds to
the index into the `sg_bindings.storage_buffers[]` array.

A binding constant will be generated for each uniquely named storage buffer
in a shader source file:

```c
#define SBUF_ssbo (0)
```

To apply a storage buffer binding, you can either use this generated constant,
or directly the binding number from the shader source file:

```c
// using the code-generated constant:
sg_apply_bindings(&(sg_bindings){
    .vertex_buffers[0] = vbuf,
    .storage_buffers = {
        [SBUF_in_ssbo] = sbuf_in,
        [SBUF_out_ssbo] = sbuf_out,
    },
});

// or directly using the explicit binding:
sg_apply_bindings(&(sg_bindings){
    .vertex_buffers[0] = vbuf,
    .storage_buffers = {
        [0] = sbuf_in,
        [1] = sbuf_out,
    },
});
```

In the shader code, the buffer content can be accessed like this (for
instance using gl_VertexIndex):

```glsl
    vec4 pos = vtx[gl_VertexIndex].pos;
    vec4 color = vtx[gl_VertexIndex].color;
```

Only one flexible array member is allowed inside a `buffer`. Note
that the name `vertex` cannot be used for a struct because it is
a reserved keyword in MSL.

On the C side, `sokol-shdc` will create a C struct `sb_vertex_t` with the required
alignment and padding (according to `std430` layout) and a `SLOT_ssbo` define
which can be used in the `sg_bindings` struct to index into the `vs.storage_buffers` or
`fs.storage_buffers` arrays.

### GLSL uniform blocks and C structs

There are a few caveats to be aware of with uniform blocks:

- The memory layout of uniform blocks on the CPU side must be compatible
  across all sokol-gfx backends. To achieve this, uniform block content
  is restricted to a subset of the std140 packing rule that's compatible
  with the glUniform() functions.

- Uniform block member types are restricted to:
    - float
    - vec2
    - vec3
    - vec4
    - int
    - ivec2
    - ivec3
    - ivec4
    - mat4

    This restriction exists so that the uniform data is compatible
    with all sokol_gfx.h backends down to GLES2 and WebGL.

- Arrays are only allowed for the following types:
    - vec4
    - int4
    - mat4

    This restriction exists because the element stride must
    be the same as the element width so that the uniform data
    is compatible both with the std140 layout and glUniformNfv() calls.

- Uniform block member alignment is as follows (compatible with std140):
    - float, int:   4 bytes
    - vec2, ivec2:  8 bytes
    - vec3, ivec3:  16 bytes
    - vec4, ivec4:  16 bytes
    - mat4:         16 bytes
    - vec4[]        16 bytes
    - ivec4[]       16 bytes
    - mat4[]        16 bytes

- For the GLSL outputs, uniform blocks will be flattened into a single
  vec4 arrays if all elements in the uniform block have the same
  'base type' (float or int):
    - 'float' base type: float, vec2, vec3, vec4, mat4
    - 'int' base type: int, ivec2, ivec3, ivec4

  The advantage of flattened uniform blocks is that they can be updated
  with a single glUniform4fv() call.

  Mixed-base-type uniform blocks are allowed, but will not be flattened, this
  means that such mixed uniform blocks require multiple glUniform() calls.

  If the performance of uniform block updates matters in the GL backends, it
  may make sense to split complex uniform blocks into two separate blocks with
  the same base type (e.g. all 'float-y' members into one uniform block, and
  all 'int-y' members into another).

  In the non-GL sokol-gfx backends (D3D11, Metal, WebGPU), uniform block
  updates are always a a single operation.

### Storage buffer content restrictions

- in vertex- and fragment-shaders, storage buffer bindings must be
  declared as `readonly`: `layout(binding=N) readonly buffer [name] { ... }`
- in compute-shaders, storage buffers bindings which are not modified by the
  shader code must be defined as `readonly` (otherwise the hazard tracking
  code in sokol_gfx.h will assume that the storage buffer content is modified
  by the shader)
- the storage buffer content must be a single flexible struct array member
- structs used in storage buffers have fewer type restrictions than
  uniform blocks, but please note that a lot of type combinations are
  little tested, when in doubt stick to the same restrictions as in
  uniform blocks

## Runtime Inspection

The hardwired uniform-block C structs and bind slot constants which are
code-generated by default may be too inflexible for some use cases like
high-level material system where different shaders must be fed from the
same superset of vertex components, buffers, textures and uniformblocks, but
each shader accepts a slightly different set of these inputs.

For such usage scenarios, sokol-shdc offers the ```--reflection``` command line
option which code-generates additional functions for runtime inspection of
vertex attributes, image, samplers, and uniform-blocks and their layout.

The functions are prefixed by the module name (defined with the ```@module``` tag
or ```--module``` command line arg) and the shader program name:

### Vertex Attribute Inspection

The function

**int [mod]_[prog]_attr_slot(const char\* attr_name)**

returns the vertex-layout slot-index of a vertex attribute by name, or **-1** if the shader program doesn't have a vertex attribute by this name.

Example code:

```c
// setup a runtime-dynamic vertex layout
sg_layout_desc layout = {
    .buffers[0].stride = sizeof(my_vertex_t)
}

// inspect the shader what vertex attribute it actually needs
// this assumes that the shader's @module name is 'mod' and
// the @program name is 'prog':
const int pos_slot = mod_prog_attr_slot("position");
if (pos_slot >= 0) {
    layout.attrs[pos_slot] = { ... };
}
const int nrm_slot = mod_prog_attr_slot("normal");
if (nrm_slot >= 0) {
    layout.attrs[nrm_slot] = { ... };
}
const int uv_slot = mod_prog_attr_slot("texcoord");
if (uv_slot >= 0) {
    layout.attrs[uv_slot] = { ... };
}

// ...use the vertex layout in sg_pipeline_desc:
sg_pipeline_desc pip_desc = {
    .layout = layout,
    ...
};
```

### Image and Sampler Bind Slot Inspection

The function

**int [mod]_[prog]_image_slot(const char\* image_name)**
**int [mod]_[prog]_sampler_slot(const char\* sampler_name)**

returns the bind-slot of an image or sampler on the given shader stage, or -1 if the
shader doesn't expect an image or sampler of that name on that shader stage.

Code example:

```c
sg_image specular_texture = ...;
sg_bindings binds = { ... };

// check if the shader expects a specular texture,
// if yes set it in the bindings struct at the expected slot index:
const int spec_tex_slot = mod_prog_image_slot("spec_tex");
const int spec_smp_slot = mod_prog_sampler_slot("spec_smp");
if ((spec_tex_slot >= 0) && (spec_smp_slot >= 0)) {
    bindings.images[spec_tex_slot] = specular_texture;
    bindings.samplers[spex_smp_slot] = specular_sampler;
}

// apply bindings
sg_apply_bindings(&binds);
```

### Uniform Block Inspection

> NOTE: some of those dynamic reflection functions are not actually needed anymore, because it's now possible to define explicit bind slots in the shader source

The function

`int [mod]_[prog]_uniformblock_slot(const char* ub_name)`

returns the bind slot of a shader's uniform block on the given shader stage, or -1
if the shader doesn't expect a uniform block of that name on that shader stage.

The function

`size_t [mod]_[prog]_uniformblock_size(const char* ub_name)`

return the size in bytes of a shader's uniform block on the given shader stage, or 0
if the shader doesn't expect a uniform block of that name on that shader stage.

Use the return values in the ```sg_apply_uniform()``` call.

Code example:

```c
// NOTE: in real-world code you'd probably want to lookup the uniform block
// slot and size upfront in initialization code, not in the hot path
const int ub_slot = prog_mod_uniformblock_slot("vs_params");
if (ub_slot >= 0) {
    const size_t ub_size = prog_mod_uniformblock_size("vs_params");
    sg_apply_uniforms(ub_slot, &(sg_range){
        .ptr = pointer_to_uniform_data,
        .size = ub_size
    });
}
```

The function

`int [mod]_[prog]_uniform_offset(const char* ub_name, const char* u_name)`

...allows to lookup the byte offset of a specific uniform within its uniform block.
This makes it possible to compose uniform data expected by a shader without the
hardwired uniform block C struct. The function returns -1 if the shader doesn't expect
a uniform block of that name, or if the uniform block is expected
but doesn't contain a uniform of that name.

It's also possible to query the ```sg_shader_uniform``` of a given uniform, which
provides additional type information with the following function:

`sg_glsl_shader_uniform [mod]_[prog]_uniform_desc(const char* ub_name, const char* u_name)`

### Storage buffer inspection

Currently, only the bind slot can be inspected for storage buffers:

`int [mod]_[prog]_storagebuffer_slot(const char* sbuf_name)`
