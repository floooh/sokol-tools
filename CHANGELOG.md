CHANGELOG
=========

#### **16-Jul-2023**

> NOTE: this update is required for the [image/sampler object split in sokol-gfx](https://github.com/floooh/sokol/blob/master/CHANGELOG.md#16-jul-2023), and in turn it doesn't work with older sokol-gfx versions (use the git tag `pre-separate-samplers` if you need to stick to the older version)

- input shader source code must now be written in Vulkan-GLSL-style with separate
  texture and sampler uniforms, this means that old code like this:

    ```glsl
    uniform sampler2D tex;

    void main() {
        frag_color = texture(tex, uv);
    }
    ```
  ...needs to be rewritten to look like this:

    ```glsl
    uniform texture2D tex;
    uniform sampler smp;

    void main() {
        frag_color = texture(sampler2D(tex, smp), uv);
    }
    ```

- the code-generation has been updated for the new 'separate image and sampler objects'
  sokol-gfx update
- sokol-shdc will now emit an error if it encounters 'old' OpenGL-GLSL-style code
  with combined image samplers
- the Google Tint library has been integrated to convert the SPIRV output from
  `glslang` to WGSL source code
- the output language option `wgpu` has been renamed to `wgsl`
- preliminary support for WGSL output has been added
- output for GLSL100 (GLES2) is still supported but has been deprecated

#### **09-Jul-2023**
- set CMAKE_OSX_DEPLOYMENT_TARGET to allow running sokol-shdc on older macOS versions

#### **01-Jul-2023**
- Nim backend: write addr() instead of deprecated unsafeAddr()

#### **08-Jun-2023**
- add a --save-intermediate-spirv cmdline option

#### **25-May-2023**
- update all Khronos dependencies to 1.3.246.1
- disable line directives for good

#### **24-May-2023**
- add a --nolinedirectives cmdline option which omits the #line debug mapping statements

#### **22-May-2023**
- update external dependencies to latest versions
- bump ios-metal version to 1.1 (1.0 is deprecated and doesn't fma() which
  is now emitted by SPIRVCross)

#### **18-Mar-2023**
- added a Rust backend (many thank to @ErikWDev!)

...

- **30-Jul-2022**:
    - BREAKING CHANGE: renamed the ```@cimport``` tag to ```@header```, and change behaviour so that
      it inserts the literal text after ```@header``` before the generated code (e.g. no more
      target-language specific behaviour)
    - Added an Odin code generator for the official Odin bindings (https://github.com/floooh/sokol-odin)

- **14-Jun-2022**:
    - initial Nim support for use with the sokol-nim bindings
    - new tag: ```@cimport``` to pull in C headers, or Zig and Nim modules

- **07-Jan-2022**:
    - support for additional uniform types:
        - int => SG_UNIFORMTYPE_INT
        - ivec2 => SG_UNIFORMTYPE_INT2
        - ivec3 => SG_UNIFORMTYPE_INT3
        - ivec4 => SG_UNIFORMTYPE_INT4
    - add support for mixed-type uniform blocks, those will not be flattened
      for the output GLSL dialects
    - stricter checks for allowed types in uniform blocks (e.g. arrays
      are only allowed for vec4, ivec4 and mat4)

- **01-Jul-2021**:
    - A new command line option ```--reflection``` which adds a small set of
    runtime introspection functions to the generated code (currently only in the C
    code generator)
    - A new command line option ```--defines``` to pass a combination
      of preprocessor defines to the GLSL compilation pass, this can be
      used to stamp out different 'variations' from the same GLSL
      source file
    - A new command line option ```--module``` which allows to define or
      override the ```@module``` name from the command line. This is useful
      in combination with the new ```--defines``` option to make type-
      and function-names generated from the same source file unique.

- **10-Feb-2021**:
    - the C code generator has been updated for the latest API-breaking-changes
    in sokol_gfx.h
    - the generated function which returns an initialized sg_shader_desc struct
    now takes an input parameter to select the backend-specific code instead
    of calling the ```sg_query_backend()``` function
    - a code generator for the [sokol-zig bindings](https://github.com/floooh/sokol-zig/) has been
    added, used with ```--format sokol_zig```

- **03-Oct-2020**: The command line option ```--noifdef``` is now obsolete (but
  still accepted) and has been replaced with ```--ifdef```. The default
  behaviour is now to *not* wrap the generated platform specific code with
  ifdefs. If you rely on that behaviour, call sokol-shdc with the new
  ```--ifdef``` flag.
