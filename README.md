# sokol-tools

Command line tools for the [sokol headers](https://github.com/floooh/sokol).

## Howto Clone, Build, Run and Debug:

#### Tool prerequisites:
```
> python --version
2.x or 3.x
> cmake --version
3.x
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
```

#### Build
...on macOS:
```
./fips set config osx-xcode-release
./fips build
```

...on Linux:
```
./fips set config linux-make-release
./fips build
```

...on Windows:
```
./fips set config win64-vstudio-release
./fips build
```

#### Run
List the compilation targets with:
```
./fips list targets
```

Run a compiled executable with:
```
./fips run [exe-target] -- [args]
```

The executables can be found in:

```
[workspace]/fips-deploy/sokol-tools/[config]/
```

#### Debug
To build for IDE debugging use any of the following build configs:
```
./fips set config osx-xcode-debug
./fips set config osx-vscode-debug
./fips set config linux-vscode-debug
./fips set config win64-vstudio-debug
./fips set config win64-vscode-debug
```

Then generate IDE project files and open the project in Visual Studio,
VSCode or Xcode:
```
./fips gen
./fips open
```

## Dependencies

Many thanks to:

- https://github.com/wc-duck/getopt
- https://github.com/imageworks/pystring
- https://github.com/KhronosGroup/glslang
- https://github.com/KhronosGroup/SPIRV-Cross
- https://github.com/KhronosGroup/SPIRV-Tools.git
- https://github.com/KhronosGroup/SPIRV-Headers
