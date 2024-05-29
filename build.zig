const std = @import("std");
const Build = std.Build;

const common_flags = [_][]const u8{
    "-fstrict-aliasing",
};
const common_c_flags = common_flags;
const common_cpp_flags = common_flags;

const spvcross_public_cpp_flags = [_][]const u8{};

const tint_public_cpp_flags = [_][]const u8{
    "-DTINT_BUILD_SPV_READER",
    "-DTINT_BUILD_WGSL_WRITER",
};

pub fn build(b: *Build) void {
    _ = build_exe(b, b.standardTargetOptions(.{}), b.standardOptimizeOption(.{}), "");
}

pub fn build_exe(
    b: *Build,
    target: Build.ResolvedTarget,
    mode: std.builtin.OptimizeMode,
    comptime prefix_path: []const u8,
) *Build.Step.Compile {
    const dir = prefix_path ++ "src/shdc/";
    const sources = [_][]const u8{
        "args.cc",
        "bytecode.cc",
        "input.cc",
        "main.cc",
        "reflection.cc",
        "spirv.cc",
        "spirvcross.cc",
        "generators/bare.cc",
        "generators/generate.cc",
        "generators/generator.cc",
        "generators/sokolc.cc",
        "generators/sokold.cc",
        "generators/sokolnim.cc",
        "generators/sokolodin.cc",
        "generators/sokolrust.cc",
        "generators/sokoljai.cc",
        "generators/sokolzig.cc",
        "generators/sokoljai.cc",
        "generators/yaml.cc",
    };
    const incl_dirs = [_][]const u8{
        "src/shdc",
        "ext/fmt/include",
        "ext/SPIRV-Cross",
        "ext/pystring",
        "ext/getopt/include",
        "ext/glslang",
        "ext/glslang/glslang/Public",
        "ext/glslang/glslang/Include",
        "ext/glslang/SPIRV",
        "ext/SPIRV-Tools/include",
        "ext/tint/include",
        "ext/tint",
    };
    const flags = common_cpp_flags ++ spvcross_public_cpp_flags ++ tint_public_cpp_flags;

    const exe = b.addExecutable(.{
        .name = "sokol-shdc",
        .target = target,
        .optimize = mode,
    });
    if (exe.rootModuleTarget().abi != .msvc)
        exe.linkLibCpp()
    else
        exe.linkLibC();
    exe.linkLibrary(lib_fmt(b, target, mode, prefix_path));
    exe.linkLibrary(lib_getopt(b, target, mode, prefix_path));
    exe.linkLibrary(lib_pystring(b, target, mode, prefix_path));
    exe.linkLibrary(lib_spirvcross(b, target, mode, prefix_path));
    exe.linkLibrary(lib_spirvtools(b, target, mode, prefix_path));
    exe.linkLibrary(lib_glslang(b, target, mode, prefix_path));
    exe.linkLibrary(lib_tint(b, target, mode, prefix_path));
    inline for (incl_dirs) |incl_dir| {
        exe.addIncludePath(b.path(prefix_path ++ incl_dir));
    }
    inline for (sources) |src| {
        exe.addCSourceFile(.{ .file = b.path(dir ++ src), .flags = &flags });
    }
    b.installArtifact(exe);
    return exe;
}

fn lib_getopt(
    b: *Build,
    target: Build.ResolvedTarget,
    mode: std.builtin.OptimizeMode,
    comptime prefix_path: []const u8,
) *Build.Step.Compile {
    const lib = b.addStaticLibrary(.{
        .name = "getopt",
        .target = target,
        .optimize = mode,
    });
    lib.linkLibC();
    lib.addIncludePath(b.path(prefix_path ++ "ext/getopt/include"));
    const flags = common_c_flags;
    lib.addCSourceFile(.{ .file = b.path(prefix_path ++ "ext/getopt/src/getopt.c"), .flags = &flags });
    return lib;
}

fn lib_pystring(
    b: *Build,
    target: Build.ResolvedTarget,
    mode: std.builtin.Mode,
    comptime prefix_path: []const u8,
) *Build.Step.Compile {
    const lib = b.addStaticLibrary(.{
        .name = "pystring",
        .target = target,
        .optimize = mode,
    });
    if (lib.rootModuleTarget().abi != .msvc)
        lib.linkLibCpp()
    else
        lib.linkLibC();
    const flags = common_cpp_flags;
    lib.addCSourceFile(.{ .file = b.path(prefix_path ++ "ext/pystring/pystring.cpp"), .flags = &flags });
    return lib;
}

fn lib_fmt(
    b: *Build,
    target: Build.ResolvedTarget,
    mode: std.builtin.OptimizeMode,
    comptime prefix_path: []const u8,
) *Build.Step.Compile {
    const dir = prefix_path ++ "ext/fmt/src/";
    const sources = [_][]const u8{ "format.cc", "os.cc" };
    const lib = b.addStaticLibrary(.{
        .name = "fmt",
        .target = target,
        .optimize = mode,
    });
    if (lib.rootModuleTarget().abi != .msvc)
        lib.linkLibCpp()
    else
        lib.linkLibC();
    lib.addIncludePath(b.path(prefix_path ++ "ext/fmt/include"));
    const flags = common_cpp_flags;
    inline for (sources) |src| {
        lib.addCSourceFile(.{ .file = b.path(dir ++ src), .flags = &flags });
    }
    return lib;
}

fn lib_spirvcross(
    b: *Build,
    target: Build.ResolvedTarget,
    mode: std.builtin.OptimizeMode,
    comptime prefix_path: []const u8,
) *Build.Step.Compile {
    const dir = prefix_path ++ "ext/SPIRV-Cross/";
    const sources = [_][]const u8{
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
    const flags = common_cpp_flags ++ spvcross_public_cpp_flags;

    const lib = b.addStaticLibrary(.{
        .name = "spirvcross",
        .target = target,
        .optimize = mode,
    });
    if (lib.rootModuleTarget().abi != .msvc)
        lib.linkLibCpp()
    else
        lib.linkLibC();
    lib.addIncludePath(b.path("ext/SPIRV-Cross"));
    inline for (sources) |src| {
        lib.addCSourceFile(.{ .file = b.path(dir ++ src), .flags = &flags });
    }
    return lib;
}

fn lib_glslang(
    b: *Build,
    target: Build.ResolvedTarget,
    mode: std.builtin.OptimizeMode,
    comptime prefix_path: []const u8,
) *Build.Step.Compile {
    const lib = b.addStaticLibrary(.{
        .name = "glslang",
        .target = target,
        .optimize = mode,
    });

    const dir = prefix_path ++ "ext/glslang/";
    const sources = [_][]const u8{
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
        "glslang/MachineIndependent/PoolAlloc.cpp",
        "glslang/MachineIndependent/propagateNoContraction.cpp",
        "glslang/MachineIndependent/reflection.cpp",
        "glslang/MachineIndependent/RemoveTree.cpp",
        "glslang/MachineIndependent/Scan.cpp",
        "glslang/MachineIndependent/ShaderLang.cpp",
        "glslang/MachineIndependent/SymbolTable.cpp",
        "glslang/MachineIndependent/Versions.cpp",
        "glslang/MachineIndependent/SpirvIntrinsics.cpp",
        "glslang/ResourceLimits/ResourceLimits.cpp",
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
    const incl_dirs = [_][]const u8{
        "ext/generated",
        "ext/glslang",
        "ext/glslang/glslang",
        "ext/glslang/glslang/Public",
        "ext/glslang/glslang/Include",
        "ext/glslang/SPIRV",
        "ext/SPIRV-Tools/include",
    };
    const win_sources = [_][]const u8{"glslang/OSDependent/Windows/ossource.cpp"};
    const unix_sources = [_][]const u8{"glslang/OSDependent/Unix/ossource.cpp"};
    const cmn_flags = common_cpp_flags ++ [_][]const u8{"-DENABLE_OPT=1"};
    const win_flags = cmn_flags ++ [_][]const u8{"-DGLSLANG_OSINCLUDE_WIN32"};
    const unx_flags = cmn_flags ++ [_][]const u8{"-DGLSLANG_OSINCLUDE_UNIX"};
    const flags = if (lib.rootModuleTarget().os.tag == .windows) win_flags else unx_flags;

    if (lib.rootModuleTarget().abi != .msvc)
        lib.linkLibCpp()
    else
        lib.linkLibC();
    inline for (incl_dirs) |incl_dir| {
        lib.addIncludePath(b.path(prefix_path ++ incl_dir));
    }
    inline for (sources) |src| {
        lib.addCSourceFile(.{ .file = b.path(dir ++ src), .flags = &flags });
    }
    if (lib.rootModuleTarget().os.tag == .windows) {
        inline for (win_sources) |src| {
            lib.addCSourceFile(.{ .file = b.path(dir ++ src), .flags = &flags });
        }
    } else {
        inline for (unix_sources) |src| {
            lib.addCSourceFile(.{ .file = b.path(dir ++ src), .flags = &flags });
        }
    }
    return lib;
}

fn lib_spirvtools(
    b: *Build,
    target: Build.ResolvedTarget,
    mode: std.builtin.Mode,
    comptime prefix_path: []const u8,
) *Build.Step.Compile {
    const dir = prefix_path ++ "ext/SPIRV-Tools/source/";
    const sources = [_][]const u8{
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
        "val/validate_mesh_shading.cpp",
        "val/validate_misc.cpp",
        "val/validate_mode_setting.cpp",
        "val/validate_non_uniform.cpp",
        "val/validate_primitives.cpp",
        "val/validate_ray_query.cpp",
        "val/validate_ray_tracing.cpp",
        "val/validate_ray_tracing_reorder.cpp",
        "val/validate_scopes.cpp",
        "val/validate_small_type_uses.cpp",
        "val/validate_type.cpp",
        "val/validation_state.cpp",
        "opt/aggressive_dead_code_elim_pass.cpp",
        "opt/amd_ext_to_khr.cpp",
        "opt/analyze_live_input_pass.cpp",
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
        "opt/control_dependence.cpp",
        "opt/convert_to_half_pass.cpp",
        "opt/copy_prop_arrays.cpp",
        "opt/dataflow.cpp",
        "opt/dead_branch_elim_pass.cpp",
        "opt/dead_insert_elim_pass.cpp",
        "opt/dead_variable_elimination.cpp",
        "opt/decoration_manager.cpp",
        "opt/def_use_manager.cpp",
        "opt/desc_sroa.cpp",
        "opt/dominator_analysis.cpp",
        "opt/dominator_tree.cpp",
        "opt/eliminate_dead_constant_pass.cpp",
        "opt/eliminate_dead_functions_pass.cpp",
        "opt/eliminate_dead_functions_util.cpp",
        "opt/eliminate_dead_io_components_pass.cpp",
        "opt/eliminate_dead_members_pass.cpp",
        "opt/eliminate_dead_output_stores_pass.cpp",
        "opt/feature_manager.cpp",
        "opt/fix_func_call_arguments.cpp",
        "opt/fix_storage_class.cpp",
        "opt/flatten_decoration_pass.cpp",
        "opt/fold.cpp",
        "opt/folding_rules.cpp",
        "opt/fold_spec_constant_op_and_composite_pass.cpp",
        "opt/freeze_spec_constant_value_pass.cpp",
        "opt/function.cpp",
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
        "opt/interface_var_sroa.cpp",
        "opt/ir_context.cpp",
        "opt/ir_loader.cpp",
        "opt/licm_pass.cpp",
        "opt/liveness.cpp",
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
        "opt/propagator.cpp",
        "opt/reduce_load_size.cpp",
        "opt/redundancy_elimination.cpp",
        "opt/register_pressure.cpp",
        "opt/relax_float_ops_pass.cpp",
        "opt/remove_dontinline_pass.cpp",
        "opt/remove_duplicates_pass.cpp",
        "opt/replace_invalid_opc.cpp",
        "opt/scalar_analysis.cpp",
        "opt/scalar_analysis_simplification.cpp",
        "opt/scalar_replacement_pass.cpp",
        "opt/set_spec_constant_default_value_pass.cpp",
        "opt/simplification_pass.cpp",
        "opt/spread_volatile_semantics.cpp",
        "opt/ssa_rewrite_pass.cpp",
        "opt/strength_reduction_pass.cpp",
        "opt/strip_debug_info_pass.cpp",
        "opt/struct_cfg_analysis.cpp",
        "opt/type_manager.cpp",
        "opt/types.cpp",
        "opt/unify_const_pass.cpp",
        "opt/upgrade_memory_model.cpp",
        "opt/value_number_table.cpp",
        "opt/vector_dce.cpp",
        "opt/workaround1209.cpp",
        "opt/wrap_opkill.cpp",
        "opt/strip_nonsemantic_info_pass.cpp",
        "opt/convert_to_sampled_image_pass.cpp",
        "opt/interp_fixup_pass.cpp",
        "opt/inst_debug_printf_pass.cpp",
        "opt/remove_unused_interface_variables_pass.cpp",
        "opt/debug_info_manager.cpp",
        "opt/replace_desc_array_access_using_var_index.cpp",
        "opt/desc_sroa_util.cpp",
    };
    const incl_dirs = [_][]const u8{
        "ext/generated",
        "ext/SPIRV-Tools",
        "ext/SPIRV-Tools/include",
        "ext/SPIRV-Headers/include",
    };
    const flags = common_cpp_flags;

    const lib = b.addStaticLibrary(.{
        .name = "spirvtools",
        .target = target,
        .optimize = mode,
    });
    if (lib.rootModuleTarget().abi != .msvc)
        lib.linkLibCpp()
    else
        lib.linkLibC();
    inline for (incl_dirs) |incl_dir| {
        lib.addIncludePath(b.path(prefix_path ++ incl_dir));
    }
    inline for (sources) |src| {
        lib.addCSourceFile(.{ .file = b.path(dir ++ src), .flags = &flags });
    }
    return lib;
}

fn lib_tint(
    b: *Build,
    target: Build.ResolvedTarget,
    mode: std.builtin.Mode,
    comptime prefix_path: []const u8,
) *Build.Step.Compile {
    const dir = prefix_path ++ "ext/tint/src/tint/";
    const sources = [_][]const u8{
        "clone_context.cc",
        "debug.cc",
        "number.cc",
        "program.cc",
        "program_builder.cc",
        "program_id.cc",
        "source.cc",
        "symbol.cc",
        "symbol_table.cc",
        "tint.cc",
        "ast/accessor_expression.cc",
        "ast/alias.cc",
        "ast/assignment_statement.cc",
        "ast/attribute.cc",
        "ast/binary_expression.cc",
        "ast/binding_attribute.cc",
        "ast/bitcast_expression.cc",
        "ast/block_statement.cc",
        "ast/bool_literal_expression.cc",
        "ast/break_if_statement.cc",
        "ast/break_statement.cc",
        "ast/builtin_attribute.cc",
        "ast/call_expression.cc",
        "ast/call_statement.cc",
        "ast/case_selector.cc",
        "ast/case_statement.cc",
        "ast/compound_assignment_statement.cc",
        "ast/const.cc",
        "ast/const_assert.cc",
        "ast/continue_statement.cc",
        "ast/diagnostic_attribute.cc",
        "ast/diagnostic_control.cc",
        "ast/diagnostic_directive.cc",
        "ast/diagnostic_rule_name.cc",
        "ast/disable_validation_attribute.cc",
        "ast/discard_statement.cc",
        "ast/enable.cc",
        "ast/expression.cc",
        "ast/extension.cc",
        "ast/float_literal_expression.cc",
        "ast/for_loop_statement.cc",
        "ast/function.cc",
        "ast/group_attribute.cc",
        "ast/id_attribute.cc",
        "ast/identifier.cc",
        "ast/identifier_expression.cc",
        "ast/if_statement.cc",
        "ast/increment_decrement_statement.cc",
        "ast/index_accessor_expression.cc",
        "ast/int_literal_expression.cc",
        "ast/internal_attribute.cc",
        "ast/interpolate_attribute.cc",
        "ast/invariant_attribute.cc",
        "ast/let.cc",
        "ast/literal_expression.cc",
        "ast/location_attribute.cc",
        "ast/loop_statement.cc",
        "ast/member_accessor_expression.cc",
        "ast/module.cc",
        "ast/must_use_attribute.cc",
        "ast/node.cc",
        "ast/override.cc",
        "ast/parameter.cc",
        "ast/phony_expression.cc",
        "ast/pipeline_stage.cc",
        "ast/return_statement.cc",
        "ast/stage_attribute.cc",
        "ast/statement.cc",
        "ast/stride_attribute.cc",
        "ast/struct_member_align_attribute.cc",
        "ast/struct_member_offset_attribute.cc",
        "ast/struct_member_size_attribute.cc",
        "ast/struct_member.cc",
        "ast/struct.cc",
        "ast/switch_statement.cc",
        "ast/templated_identifier.cc",
        "ast/type.cc",
        "ast/type_decl.cc",
        "ast/unary_op_expression.cc",
        "ast/unary_op.cc",
        "ast/var.cc",
        "ast/variable_decl_statement.cc",
        "ast/variable.cc",
        "ast/while_statement.cc",
        "ast/workgroup_attribute.cc",
        "ast/transform/add_empty_entry_point.cc",
        "ast/transform/add_block_attribute.cc",
        "ast/transform/array_length_from_uniform.cc",
        "ast/transform/binding_remapper.cc",
        "ast/transform/builtin_polyfill.cc",
        "ast/transform/calculate_array_length.cc",
        "ast/transform/clamp_frag_depth.cc",
        "ast/transform/canonicalize_entry_point_io.cc",
        "ast/transform/combine_samplers.cc",
        "ast/transform/decompose_memory_access.cc",
        "ast/transform/decompose_strided_array.cc",
        "ast/transform/decompose_strided_matrix.cc",
        "ast/transform/demote_to_helper.cc",
        "ast/transform/direct_variable_access.cc",
        "ast/transform/disable_uniformity_analysis.cc",
        "ast/transform/expand_compound_assignment.cc",
        "ast/transform/first_index_offset.cc",
        "ast/transform/for_loop_to_loop.cc",
        "ast/transform/localize_struct_array_assignment.cc",
        "ast/transform/merge_return.cc",
        "ast/transform/module_scope_var_to_entry_point_param.cc",
        "ast/transform/multiplanar_external_texture.cc",
        "ast/transform/num_workgroups_from_uniform.cc",
        "ast/transform/packed_vec3.cc",
        "ast/transform/pad_structs.cc",
        "ast/transform/preserve_padding.cc",
        "ast/transform/promote_initializers_to_let.cc",
        "ast/transform/promote_side_effects_to_decl.cc",
        "ast/transform/remove_continue_in_switch.cc",
        "ast/transform/remove_phonies.cc",
        "ast/transform/remove_unreachable_statements.cc",
        "ast/transform/renamer.cc",
        "ast/transform/robustness.cc",
        "ast/transform/simplify_pointers.cc",
        "ast/transform/single_entry_point.cc",
        "ast/transform/spirv_atomic.cc",
        "ast/transform/std140.cc",
        "ast/transform/substitute_override.cc",
        "ast/transform/texture_1d_to_2d.cc",
        "ast/transform/transform.cc",
        "ast/transform/truncate_interstage_variables.cc",
        "ast/transform/unshadow.cc",
        "ast/transform/utils/get_insertion_point.cc",
        "ast/transform/utils/hoist_to_decl_before.cc",
        "ast/transform/var_for_dynamic_index.cc",
        "ast/transform/vectorize_matrix_conversions.cc",
        "ast/transform/vectorize_scalar_matrix_initializers.cc",
        "ast/transform/vertex_pulling.cc",
        "ast/transform/while_to_loop.cc",
        "ast/transform/zero_init_workgroup_memory.cc",
        "builtin/access.cc",
        "builtin/address_space.cc",
        "builtin/attribute.cc",
        "builtin/builtin.cc",
        "builtin/builtin_value.cc",
        "builtin/diagnostic_rule.cc",
        "builtin/diagnostic_severity.cc",
        "builtin/extension.cc",
        "builtin/function.cc",
        "builtin/interpolation_sampling.cc",
        "builtin/interpolation_type.cc",
        "builtin/texel_format.cc",
        "constant/composite.cc",
        "constant/scalar.cc",
        "constant/splat.cc",
        "constant/node.cc",
        "constant/value.cc",
        "diagnostic/diagnostic.cc",
        "diagnostic/formatter.cc",
        "diagnostic/printer.cc",
        "inspector/entry_point.cc",
        "inspector/inspector.cc",
        "inspector/resource_binding.cc",
        "inspector/scalar.cc",
        "reader/reader.cc",
        "reader/spirv/construct.cc",
        "reader/spirv/entry_point_info.cc",
        "reader/spirv/enum_converter.cc",
        "reader/spirv/function.cc",
        "reader/spirv/namer.cc",
        "reader/spirv/parser_type.cc",
        "reader/spirv/parser.cc",
        "reader/spirv/parser_impl.cc",
        "reader/spirv/usage.cc",
        "resolver/builtin_structs.cc",
        "resolver/const_eval.cc",
        "resolver/ctor_conv_intrinsic.cc",
        "resolver/dependency_graph.cc",
        "resolver/intrinsic_table.cc",
        "resolver/resolver.cc",
        "resolver/sem_helper.cc",
        "resolver/uniformity.cc",
        "resolver/validator.cc",
        "sem/array_count.cc",
        "sem/behavior.cc",
        "sem/block_statement.cc",
        "sem/break_if_statement.cc",
        "sem/builtin.cc",
        "sem/builtin_enum_expression.cc",
        "sem/call_target.cc",
        "sem/call.cc",
        "sem/expression.cc",
        "sem/for_loop_statement.cc",
        "sem/function_expression.cc",
        "sem/function.cc",
        "sem/if_statement.cc",
        "sem/index_accessor_expression.cc",
        "sem/info.cc",
        "sem/load.cc",
        "sem/loop_statement.cc",
        "sem/materialize.cc",
        "sem/member_accessor_expression.cc",
        "sem/module.cc",
        "sem/node.cc",
        "sem/parameter_usage.cc",
        "sem/statement.cc",
        "sem/struct.cc",
        "sem/switch_statement.cc",
        "sem/type_expression.cc",
        "sem/variable.cc",
        "sem/value_constructor.cc",
        "sem/value_conversion.cc",
        "sem/value_expression.cc",
        "sem/while_statement.cc",
        "transform/manager.cc",
        "transform/transform.cc",
        "type/abstract_float.cc",
        "type/abstract_int.cc",
        "type/abstract_numeric.cc",
        "type/array.cc",
        "type/array_count.cc",
        "type/atomic.cc",
        "type/bool.cc",
        "type/depth_multisampled_texture.cc",
        "type/depth_texture.cc",
        "type/external_texture.cc",
        "type/f16.cc",
        "type/f32.cc",
        "type/i32.cc",
        "type/manager.cc",
        "type/matrix.cc",
        "type/multisampled_texture.cc",
        "type/node.cc",
        "type/pointer.cc",
        "type/reference.cc",
        "type/sampled_texture.cc",
        "type/sampler.cc",
        "type/sampler_kind.cc",
        "type/storage_texture.cc",
        "type/struct.cc",
        "type/texture.cc",
        "type/texture_dimension.cc",
        "type/type.cc",
        "type/u32.cc",
        "type/unique_node.cc",
        "type/vector.cc",
        "type/void.cc",
        "utils/castable.cc",
        "utils/debugger.cc",
        "utils/string.cc",
        "utils/string_stream.cc",
        "utils/unicode.cc",
        "writer/append_vector.cc",
        "writer/array_length_from_uniform_options.cc",
        "writer/binding_remapper_options.cc",
        "writer/check_supported_extensions.cc",
        "writer/external_texture_options.cc",
        "writer/flatten_bindings.cc",
        "writer/float_to_string.cc",
        "writer/text_generator.cc",
        "writer/text.cc",
        "writer/writer.cc",
        "writer/wgsl/generator.cc",
        "writer/wgsl/generator_impl.cc",
    };
    const incl_dirs = [_][]const u8{
        "ext/generated",
        "ext/SPIRV-Headers/include",
        "ext/SPIRV-Tools",
        "ext/SPIRV-Tools/include",
        "ext/tint",
        "ext/tint/include",
    };
    const flags = common_cpp_flags ++ tint_public_cpp_flags;

    const lib = b.addStaticLibrary(.{
        .name = "tint",
        .target = target,
        .optimize = mode,
    });
    if (lib.rootModuleTarget().abi != .msvc)
        lib.linkLibCpp()
    else
        lib.linkLibC();
    inline for (incl_dirs) |incl_dir| {
        lib.addIncludePath(b.path(prefix_path ++ incl_dir));
    }
    inline for (sources) |src| {
        lib.addCSourceFile(.{ .file = b.path(dir ++ src), .flags = &flags });
    }
    return lib;
}
