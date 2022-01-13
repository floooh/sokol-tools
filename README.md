# sokol-tools

[![Build](https://github.com/floooh/sokol-tools/actions/workflows/build.yml/badge.svg)](https://github.com/floooh/sokol-tools/actions/workflows/build.yml)

Command line tools for the [sokol headers](https://github.com/floooh/sokol).

## Documentation

- [sokol_shdc](docs/sokol-shdc.md): shader-cross-compiler and code-generator for sokol_gfx.h

## Precompiled binaries and fips-integration files:

https://github.com/floooh/sokol-tools-bin

## Howto Clone, Build, Run and Debug:

#### Tool prerequisites:
```
> python --version
2.x or 3.x
> cmake --version
3.x
# optional:
> ninja --version
1.9.0
```
...plus your platform's C/C++ compiler toolchain (Xcode, Visual Studio, clang
or gcc).

#### Clone

Create a separate workspace directory, this will be populated with
dependency- and build-system-directories (the name doesn't matter),
and clone the repository **with submodules**:

```
> mkdir workspace
> cd workspace
> git clone --recursive git@github.com:floooh/sokol-tools.git
> cd sokol-tools
```

#### Build
...on macOS:
```
> ./fips set config osx-xcode-release
# or with ninja:
> ./fips set config osx-ninja-release
> ./fips build
```

...on Windows:
```
> ./fips set config win64-vstudio-release
> ./fips build
```

...on Linux:

The executables for Linux **must** be compiled via Docker since they use
static linking with musl, and Alpine Linux in Docker is the easiest option for this.

```
> cd docker
> ./build.sh
```
The compiled Linux executable will be in the same directory (docker/)

Creating a statically linked Linux executable with glibc instead of
musl will create an executable which will most likely crash before
main() is entered (this also means that development under Linux
with the current build settings isn't possible, at least the
static linking flags must be removed from the cmake files for this).

#### Run
List the compilation targets with:
```
> ./fips list targets
```

Run a compiled executable with:
```
> ./fips run [exe-target] -- [args]
```

The executables can be found in:

```
[workspace]/fips-deploy/sokol-tools/[config]/
```

#### Debug
To build for IDE debugging use any of the following build configs:
```
> ./fips set config osx-xcode-debug
> ./fips set config osx-vscode-debug
> ./fips set config linux-vscode-debug
> ./fips set config win64-vstudio-debug
> ./fips set config win64-vscode-debug
```

Then generate IDE project files and open the project in Visual Studio,
VSCode or Xcode:
```
> ./fips gen
> ./fips open
```

## Dependencies

Many thanks to:

- https://github.com/wc-duck/getopt
- https://github.com/imageworks/pystring
- https://github.com/fmtlib/fmt
- https://github.com/KhronosGroup/glslang
- https://github.com/KhronosGroup/SPIRV-Cross
- https://github.com/KhronosGroup/SPIRV-Tools.git
- https://github.com/KhronosGroup/SPIRV-Headers

