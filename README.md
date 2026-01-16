# sokol-tools

[![Build](https://github.com/floooh/sokol-tools/actions/workflows/build.yml/badge.svg)](https://github.com/floooh/sokol-tools/actions/workflows/build.yml)

Command line tools for the [sokol headers](https://github.com/floooh/sokol).

## Documentation

- [sokol_shdc](docs/sokol-shdc.md): shader-cross-compiler and code-generator for sokol_gfx.h

## Precompiled binaries and fips-integration files:

https://github.com/floooh/sokol-tools-bin

## How to build, run and debug

Prerequisites:
```
> deno --version
2.6.x
> cmake --version
3.x
# optional:
> ninja --version
1.9.x
```
...plus your platform's C/C++ compiler toolchain (Xcode, Visual Studio, clang
or gcc).

...for installing Deno, see: https://docs.deno.com/runtime/getting_started/installation/

To clone, build and run:
```
git clone --recursive https://github.com/floooh/sokol-tools.git
cd sokol-tools
./fibs build
./fibs run sokol-shdc --help
```

On Linux this builds a statically linked executable. If this doesn't work
for some reason, build via Docker instead (which will compile inside an
Alpine image):

```
./build_docker.sh
```

To build for IDE debugging use any of the following build configs:
```
./fibs config macos-xcode-debug
./fibs config macos-vscode-debug
./fibs config win-vstudio-debug
./fibs config win-vscode-debug
./fibs config linux-vscode-debug
```

...then launch the project in VStudio, Xcode or VSCode via:

```
./fibs open
```

For more information about `fibs` see https://github.com/floooh/fibs

## Dependencies

Many thanks to:

- https://github.com/wc-duck/getopt
- https://github.com/imageworks/pystring
- https://github.com/fmtlib/fmt
- https://github.com/KhronosGroup/glslang
- https://github.com/KhronosGroup/SPIRV-Cross
- https://github.com/KhronosGroup/SPIRV-Tools.git
- https://github.com/KhronosGroup/SPIRV-Headers
- https://dawn.googlesource.com/tint
