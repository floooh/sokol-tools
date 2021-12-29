fips_begin_lib(getopt)
    fips_dir(getopt/src GROUP .)
    fips_files(getopt.c)
    fips_dir(getopt/include/getopt GROUP .)
    fips_files(getopt.h)
fips_end_lib()
target_include_directories(getopt PUBLIC getopt/include)

fips_begin_lib(pystring)
    fips_dir(pystring GROUP .)
    fips_files(pystring.cpp pystring.h)
fips_end_lib()
target_include_directories(pystring PUBLIC pystring)

fips_begin_lib(SPIRV-Tools)
    fips_src(SPIRV-Tools/source NO_RECURSE GROUP .)
    fips_src(SPIRV-Tools/source/util GROUP util)
    fips_src(SPIRV-Tools/source/val GROUP val)
    fips_src(SPIRV-Tools/source/opt GROUP opt)
fips_end_lib()
target_include_directories(SPIRV-Tools PRIVATE generated SPIRV-Tools SPIRV-Headers/include)
target_include_directories(SPIRV-Tools PUBLIC SPIRV-Tools/include)
if (FIPS_MSVC)
    target_compile_options(SPIRV-Tools PUBLIC -D_SCL_SECURE_NO_WARNINGS)
endif()
if (FIPS_CLANG)
    target_compile_options(SPIRV-Tools PRIVATE -Wno-range-loop-analysis)
endif()

fips_begin_lib(glslang)
    if (FIPS_WINDOWS)
        fips_src(glslang/glslang/OSDependent/Windows EXCEPT main.cpp GROUP "glsang/win")
    elseif (FIPS_WASISDK)
        fips_files(glslang_osdependent_wasi.cc)
    else()
        fips_src(glslang/glslang/OSDependent/Unix GROUP "glslang/unix")
    endif()
    fips_src(glslang/OGLCompilersDLL EXCEPT hlsl*)
    fips_src(glslang/glslang EXCEPT OSDependent/* hlsl* GROUP "glslang")
    fips_src(glslang/SPIRV GROUP "SPIRV")
    fips_deps(SPIRV-Tools)
fips_end_lib()
target_include_directories(glslang PUBLIC glslang)
target_include_directories(glslang PUBLIC glslang/glslang/Public)
target_include_directories(glslang PUBLIC glslang/glslang/Include)
target_include_directories(glslang PUBLIC glslang/SPIRV)
target_include_directories(glslang PRIVATE glslang/glslang)
target_include_directories(glslang PRIVATE generated)
target_compile_definitions(glslang PRIVATE ENABLE_OPT=1)
if (FIPS_WINDOWS)
    target_compile_definitions(glslang PRIVATE GLSLANG_OSINCLUDE_WIN32)
else()
    target_compile_definitions(glslang PRIVATE GLSLANG_OSINCLUDE_UNIX)
endif()
if (FIPS_GCC OR FIPS_CLANG)
    target_compile_options(glslang PRIVATE -Wno-sign-compare -Wno-deprecated-copy -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-unused-const-variable)
endif()

fips_begin_lib(SPIRV-Cross)
    fips_dir(SPIRV-Cross GROUP .)
    fips_files(
        GLSL.std.450.h
        spirv_common.hpp
        spirv_cross_containers.hpp
        spirv_cross_error_handling.hpp
        spirv.hpp
        spirv_cross.hpp spirv_cross.cpp
        spirv_parser.hpp spirv_parser.cpp
        spirv_cross_parsed_ir.hpp spirv_cross_parsed_ir.cpp
        spirv_cfg.hpp spirv_cfg.cpp
        spirv_glsl.hpp spirv_glsl.cpp
        spirv_msl.hpp spirv_msl.cpp
        spirv_hlsl.hpp spirv_hlsl.cpp
        spirv_reflect.hpp spirv_reflect.cpp
        spirv_cross_util.hpp spirv_cross_util.cpp)
fips_end_lib()
target_compile_definitions(SPIRV-Cross PUBLIC SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS)
if (FIPS_MSVC)
    target_compile_options(SPIRV-Cross PUBLIC -D_SCL_SECURE_NO_WARNINGS)
    target_compile_options(SPIRV-Cross PUBLIC /wd4715)
endif()
if (FIPS_CLANG)
    target_compile_options(SPIRV-Cross PRIVATE -Wno-range-loop-analysis)
endif()
target_include_directories(SPIRV-Cross PUBLIC SPIRV-Cross)

fips_begin_lib(fmt)
    fips_dir(fmt/src GPOUP .)
    fips_files(format.cc)
fips_end_lib()
target_include_directories(fmt PUBLIC fmt/include)

