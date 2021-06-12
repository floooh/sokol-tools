const bld = @import("std").build;

pub fn build(b: *bld.Builder) void {
    const dir = "src/shdc/";
    const sources = [_][]const u8 {
        "args.cc",
        "bare.cc",
        "bytecode.cc",
        "input.cc",
        "main.cc",
        "sokol.cc",
        "sokolzig.cc",
        "spirv.cc",
        "spirvcross.cc"
    };
    const incl_dirs = [_][] const u8 {
        "ext/fmt/include",
        "ext/SPIRV-Cross",
        "ext/pystring",
        "ext/getopt/include",
        "ext/glslang",
        "ext/glslang/glslang/Public",
        "ext/glslang/glslang/Include",
        "ext/glslang/SPIRV",
        "ext/SPIRV-Tools/include"
    };
    const flags = [_][]const u8 { };

    const exe = b.addExecutable("sokol-shdc", null);
    exe.setTarget(b.standardTargetOptions(.{}));
    exe.setBuildMode(b.standardReleaseOptions());
    exe.linkSystemLibrary("c");
    exe.linkSystemLibrary("c++");
    exe.linkLibrary(lib_fmt(b));
    exe.linkLibrary(lib_getopt(b));
    exe.linkLibrary(lib_pystring(b));
    exe.linkLibrary(lib_spirvcross(b));
    exe.linkLibrary(lib_spirvtools(b));
    exe.linkLibrary(lib_glslang(b));
    inline for (incl_dirs) |incl_dir| {
        exe.addIncludeDir(incl_dir);
    }
    inline for (sources) |src| {
        exe.addCSourceFile(dir ++ src, &flags);
    }
    exe.install();
}

fn lib_getopt(b: *bld.Builder) *bld.LibExeObjStep {
    const lib = b.addStaticLibrary("getopt", null);
    lib.setBuildMode(b.standardReleaseOptions());
    lib.linkSystemLibrary("c");
    lib.addIncludeDir("ext/getopt/include");
    const flags = [_][]const u8 { };
    lib.addCSourceFile("ext/getopt/src/getopt.c", &flags);
    return lib;
}

fn lib_pystring(b: *bld.Builder) *bld.LibExeObjStep {
    const lib = b.addStaticLibrary("pystring", null);
    lib.setBuildMode(b.standardReleaseOptions());
    lib.linkSystemLibrary("c");
    lib.linkSystemLibrary("c++");
    const flags = [_][]const u8 { };
    lib.addCSourceFile("ext/pystring/pystring.cpp", &flags);
    return lib;
}

fn lib_fmt(b: *bld.Builder) *bld.LibExeObjStep {
    const dir = "ext/fmt/src/";
    const sources = [_][]const u8 {
        "format.cc",
        "posix.cc"
    };
    const lib = b.addStaticLibrary("fmt", null);
    lib.setBuildMode(b.standardReleaseOptions());
    lib.linkSystemLibrary("c");
    lib.linkSystemLibrary("c++");
    lib.addIncludeDir("ext/fmt/include");
    const flags = [_][]const u8 { };
    inline for (sources) |src| {
        lib.addCSourceFile(dir ++ src, &flags);
    }
    return lib;
}

fn lib_spirvcross(b: *bld.Builder) *bld.LibExeObjStep {
    const dir = "ext/SPIRV-Cross/";
    const sources = [_][]const u8 {
        "spirv_cross.cpp",
        "spirv_parser.cpp",
        "spirv_cross_parsed_ir.cpp",
        "spirv_cfg.cpp",
        "spirv_glsl.cpp",
        "spirv_msl.cpp",
        "spirv_hlsl.cpp",
        "spirv_reflect.cpp",
        "spirv_cross_util.cpp",
    };
    const flags = [_][]const u8 {
        "-DSPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS",
    };

    const lib = b.addStaticLibrary("spirvcross", null);
    lib.setBuildMode(b.standardReleaseOptions());
    lib.linkSystemLibrary("c");
    lib.linkSystemLibrary("c++");
    lib.addIncludeDir("ext/SPIRV-Cross");
    inline for (sources) |src| {
        lib.addCSourceFile(dir ++ src, &flags);
    }
    return lib;
}

fn lib_glslang(b: *bld.Builder) *bld.LibExeObjStep {
    const lib = b.addStaticLibrary("glslang", null);

    const dir = "ext/glslang/";
    const sources = [_][]const u8 {
        "OGLCompilersDLL/InitializeDll.cpp",
        "glslang/CInterface/glslang_c_interface.cpp",
        "glslang/GenericCodeGen/CodeGen.cpp",
        "glslang/GenericCodeGen/Link.cpp",
        "glslang/MachineIndependent/preprocessor/PpAtom.cpp",
        "glslang/MachineIndependent/preprocessor/PpContext.cpp",
        "glslang/MachineIndependent/preprocessor/Pp.cpp",
        "glslang/MachineIndependent/preprocessor/PpScanner.cpp",
        "glslang/MachineIndependent/preprocessor/PpTokens.cpp",
        "glslang/MachineIndependent/attribute.cpp",
        "glslang/MachineIndependent/Constant.cpp",
        "glslang/MachineIndependent/glslang_tab.cpp",
        "glslang/MachineIndependent/InfoSink.cpp",
        "glslang/MachineIndependent/Initialize.cpp",
        "glslang/MachineIndependent/Intermediate.cpp",
        "glslang/MachineIndependent/intermOut.cpp",
        "glslang/MachineIndependent/IntermTraverse.cpp",
        "glslang/MachineIndependent/iomapper.cpp",
        "glslang/MachineIndependent/limits.cpp",
        "glslang/MachineIndependent/linkValidate.cpp",
        "glslang/MachineIndependent/parseConst.cpp",
        "glslang/MachineIndependent/ParseContextBase.cpp",
        "glslang/MachineIndependent/ParseHelper.cpp",
        "glslang/MachineIndependent/pch.cpp",
        "glslang/MachineIndependent/PoolAlloc.cpp",
        "glslang/MachineIndependent/propagateNoContraction.cpp",
        "glslang/MachineIndependent/reflection.cpp",
        "glslang/MachineIndependent/RemoveTree.cpp",
        "glslang/MachineIndependent/Scan.cpp",
        "glslang/MachineIndependent/ShaderLang.cpp",
        "glslang/MachineIndependent/SymbolTable.cpp",
        "glslang/MachineIndependent/Versions.cpp",
        "SPIRV/disassemble.cpp",
        "SPIRV/doc.cpp",
        "SPIRV/GlslangToSpv.cpp",
        "SPIRV/InReadableOrder.cpp",
        "SPIRV/Logger.cpp",
        "SPIRV/SpvBuilder.cpp",
        "SPIRV/SpvPostProcess.cpp",
        "SPIRV/SPVRemapper.cpp",
        "SPIRV/SpvTools.cpp",
    };
    const incl_dirs = [_][]const u8 {
        "ext/glslang",
        "ext/glslang/glslang",
        "ext/glslang/glslang/Public",
        "ext/glslang/glslang/Include",
        "ext/glslang/SPIRV",
        "ext/SPIRV-Tools/include"
    };
    const win_sources = [_][]const u8 {
        "glslang/OSDependent/Windows/ossource.cpp"
    };
    const unix_sources = [_][]const u8 {
        "glslang/OSDependent/Unix/ossource.cpp"
    };
    const os_define = if (lib.target.isWindows()) "-DGLSLANG_OSINCLUDE_WIN32" else "-DGLSLANG_OSINCLUDE_UNIX";
    const flags = [_][]const u8 {
        os_define,
        "-DENABLE_OPT=1",
    };

    lib.setBuildMode(b.standardReleaseOptions());
    lib.linkSystemLibrary("c");
    lib.linkSystemLibrary("c++");
    inline for (incl_dirs) |incl_dir| {
        lib.addIncludeDir(incl_dir);
    }
    inline for (sources) |src| {
        lib.addCSourceFile(dir ++ src, &flags);
    }
    if (lib.target.isWindows()) {
        inline for (win_sources) |src| {
            lib.addCSourceFile(dir ++ src, &flags);
        }
    }
    else {
        inline for (unix_sources) |src| {
            lib.addCSourceFile(dir ++ src, &flags);
        }
    }
    return lib;
}

fn lib_spirvtools(b: *bld.Builder) *bld.LibExeObjStep {
    const dir = "ext/SPIRV-Tools/source/";
    const sources = [_][]const u8 {
        "assembly_grammar.cpp",
        "binary.cpp",
        "diagnostic.cpp",
        "disassemble.cpp",
        "enum_string_mapping.cpp",
        "extensions.cpp",
        "ext_inst.cpp",
        "libspirv.cpp",
        "name_mapper.cpp",
        "opcode.cpp",
        "operand.cpp",
        "parsed_operand.cpp",
        "pch_source.cpp",
        "print.cpp",
        "software_version.cpp",
        "spirv_endian.cpp",
        "spirv_fuzzer_options.cpp",
        "spirv_optimizer_options.cpp",
        "spirv_reducer_options.cpp",
        "spirv_target_env.cpp",
        "spirv_validator_options.cpp",
        "table.cpp",
        "text.cpp",
        "text_handler.cpp",
        "util/bit_vector.cpp",
        "util/parse_number.cpp",
        "util/string_utils.cpp",
        "util/timer.cpp",
        "val/basic_block.cpp",
        "val/construct.cpp",
        "val/function.cpp",
        "val/instruction.cpp",
        "val/validate_adjacency.cpp",
        "val/validate_annotation.cpp",
        "val/validate_arithmetics.cpp",
        "val/validate_atomics.cpp",
        "val/validate_barriers.cpp",
        "val/validate_bitwise.cpp",
        "val/validate_builtins.cpp",
        "val/validate_capability.cpp",
        "val/validate_cfg.cpp",
        "val/validate_composites.cpp",
        "val/validate_constants.cpp",
        "val/validate_conversion.cpp",
        "val/validate.cpp",
        "val/validate_debug.cpp",
        "val/validate_decorations.cpp",
        "val/validate_derivatives.cpp",
        "val/validate_execution_limitations.cpp",
        "val/validate_extensions.cpp",
        "val/validate_function.cpp",
        "val/validate_id.cpp",
        "val/validate_image.cpp",
        "val/validate_instruction.cpp",
        "val/validate_interfaces.cpp",
        "val/validate_layout.cpp",
        "val/validate_literals.cpp",
        "val/validate_logicals.cpp",
        "val/validate_memory.cpp",
        "val/validate_memory_semantics.cpp",
        "val/validate_misc.cpp",
        "val/validate_mode_setting.cpp",
        "val/validate_non_uniform.cpp",
        "val/validate_primitives.cpp",
        "val/validate_scopes.cpp",
        "val/validate_small_type_uses.cpp",
        "val/validate_type.cpp",
        "val/validation_state.cpp",
        "opt/aggressive_dead_code_elim_pass.cpp",
        "opt/amd_ext_to_khr.cpp",
        "opt/basic_block.cpp",
        "opt/block_merge_pass.cpp",
        "opt/block_merge_util.cpp",
        "opt/build_module.cpp",
        "opt/ccp_pass.cpp",
        "opt/cfg_cleanup_pass.cpp",
        "opt/cfg.cpp",
        "opt/code_sink.cpp",
        "opt/combine_access_chains.cpp",
        "opt/compact_ids_pass.cpp",
        "opt/composite.cpp",
        "opt/constants.cpp",
        "opt/const_folding_rules.cpp",
        "opt/convert_to_half_pass.cpp",
        "opt/copy_prop_arrays.cpp",
        "opt/dead_branch_elim_pass.cpp",
        "opt/dead_insert_elim_pass.cpp",
        "opt/dead_variable_elimination.cpp",
        "opt/decompose_initialized_variables_pass.cpp",
        "opt/decoration_manager.cpp",
        "opt/def_use_manager.cpp",
        "opt/desc_sroa.cpp",
        "opt/dominator_analysis.cpp",
        "opt/dominator_tree.cpp",
        "opt/eliminate_dead_constant_pass.cpp",
        "opt/eliminate_dead_functions_pass.cpp",
        "opt/eliminate_dead_functions_util.cpp",
        "opt/eliminate_dead_members_pass.cpp",
        "opt/feature_manager.cpp",
        "opt/fix_storage_class.cpp",
        "opt/flatten_decoration_pass.cpp",
        "opt/fold.cpp",
        "opt/folding_rules.cpp",
        "opt/fold_spec_constant_op_and_composite_pass.cpp",
        "opt/freeze_spec_constant_value_pass.cpp",
        "opt/function.cpp",
        "opt/generate_webgpu_initializers_pass.cpp",
        "opt/graphics_robust_access_pass.cpp",
        "opt/if_conversion.cpp",
        "opt/inline_exhaustive_pass.cpp",
        "opt/inline_opaque_pass.cpp",
        "opt/inline_pass.cpp",
        "opt/inst_bindless_check_pass.cpp",
        "opt/inst_buff_addr_check_pass.cpp",
        "opt/instruction.cpp",
        "opt/instruction_list.cpp",
        "opt/instrument_pass.cpp",
        "opt/ir_context.cpp",
        "opt/ir_loader.cpp",
        "opt/legalize_vector_shuffle_pass.cpp",
        "opt/licm_pass.cpp",
        "opt/local_access_chain_convert_pass.cpp",
        "opt/local_redundancy_elimination.cpp",
        "opt/local_single_block_elim_pass.cpp",
        "opt/local_single_store_elim_pass.cpp",
        "opt/loop_dependence.cpp",
        "opt/loop_dependence_helpers.cpp",
        "opt/loop_descriptor.cpp",
        "opt/loop_fission.cpp",
        "opt/loop_fusion.cpp",
        "opt/loop_fusion_pass.cpp",
        "opt/loop_peeling.cpp",
        "opt/loop_unroller.cpp",
        "opt/loop_unswitch_pass.cpp",
        "opt/loop_utils.cpp",
        "opt/mem_pass.cpp",
        "opt/merge_return_pass.cpp",
        "opt/module.cpp",
        "opt/optimizer.cpp",
        "opt/pass.cpp",
        "opt/pass_manager.cpp",
        "opt/pch_source_opt.cpp",
        "opt/private_to_local_pass.cpp",
        "opt/process_lines_pass.cpp",
        "opt/propagator.cpp",
        "opt/reduce_load_size.cpp",
        "opt/redundancy_elimination.cpp",
        "opt/register_pressure.cpp",
        "opt/relax_float_ops_pass.cpp",
        "opt/remove_duplicates_pass.cpp",
        "opt/replace_invalid_opc.cpp",
        "opt/scalar_analysis.cpp",
        "opt/scalar_analysis_simplification.cpp",
        "opt/scalar_replacement_pass.cpp",
        "opt/set_spec_constant_default_value_pass.cpp",
        "opt/simplification_pass.cpp",
        "opt/split_invalid_unreachable_pass.cpp",
        "opt/ssa_rewrite_pass.cpp",
        "opt/strength_reduction_pass.cpp",
        "opt/strip_atomic_counter_memory_pass.cpp",
        "opt/strip_debug_info_pass.cpp",
        "opt/strip_reflect_info_pass.cpp",
        "opt/struct_cfg_analysis.cpp",
        "opt/type_manager.cpp",
        "opt/types.cpp",
        "opt/unify_const_pass.cpp",
        "opt/upgrade_memory_model.cpp",
        "opt/value_number_table.cpp",
        "opt/vector_dce.cpp",
        "opt/workaround1209.cpp",
        "opt/wrap_opkill.cpp",
    };
    const incl_dirs = [_][]const u8 {
        "ext/generated",
        "ext/SPIRV-Tools",
        "ext/SPIRV-Tools/include",
        "ext/SPIRV-Headers/include",
    };
    const flags = [_][]const u8 { };

    const lib = b.addStaticLibrary("spirvtools", null);
    lib.setBuildMode(b.standardReleaseOptions());
    lib.linkSystemLibrary("c");
    lib.linkSystemLibrary("c++");
    inline for (incl_dirs) |incl_dir| {
        lib.addIncludeDir(incl_dir);
    }
    inline for (sources) |src| {
        lib.addCSourceFile(dir ++ src, &flags);
    }
    return lib;
}
