import { Builder } from 'jsr:@floooh/fibs@^1';

export function build(b: Builder): void {
    b.addCmakeVariable('CMAKE_CXX_STANDARD', '20');
    if (b.isMsvc()) {
        b.addCompileOptions(['/W4'])
        b.addCompileOptions({ opts: ['/Os'], buildMode: 'release' });
        b.addCompileDefinitions({ _CRT_SECURE_NO_WARNINGS: '1' });
    } else {
        b.addCompileOptions(['-Wall', '-Wextra', '-Wno-missing-field-initializers' ]);
        b.addCompileOptions({ opts: ['-Os'], buildMode: 'release' });
        if (b.isGcc()) {
            b.addCompileOptions(['-Wno-missing-braces']);
        }
    }

    // external libs
    b.addTarget('getopt', 'lib', (t) => {
        t.setDir('ext/getopt');
        t.setIdeFolder('ext');
        t.addSources([
            'src/getopt.c',
            'include/getopt/getopt.h'
        ]);
        t.addIncludeDirectories(['include']);
    });
    b.addTarget('pystring', 'lib', (t) => {
        t.setDir('ext/pystring');
        t.setIdeFolder('ext');
        t.addSources(['pystring.cpp', 'pystring.h']);
        t.addIncludeDirectories(['.']);
    });
    b.addTarget('fmt', 'lib', (t) => {
        t.setDir('ext/fmt');
        t.setIdeFolder('ext');
        t.addSources(['src/format.cc', 'src/os.cc']);
        t.addIncludeDirectories(['include']);
        t.addCompileDefinitions({ FMT_UNICODE: '0' });
    });
    b.addTarget('SPIRV-Tools', 'lib', (t) => {
        t.setDir('ext/SPIRV-Tools');
        t.setIdeFolder('ext');
        t.addSources(spirv_tools_sources.map((s) => `source/${s}`));
        t.addIncludeDirectories([
            '.',
            'include',
            `../generated`,
            `../SPIRV-Headers/include`
        ]);
        if (b.isMsvc()) {
            t.addCompileDefinitions({ _SCL_SECURE_NO_WARNINGS: '1' });
        }
        if (b.isClang()) {
            t.addCompileOptions({
                opts: [ '-Wno-range-loop-analysis', '-Wno-deprecated-declarations' ],
                scope: 'private',
            });
        }
    });
    b.addTarget('glslang', 'lib', (t) => {
        t.setDir('ext/glslang');
        t.setIdeFolder('ext');
        t.addSources(glslslang_common_sources);
        if (b.isWindows()) {
            t.addSources(['glslang/OSDependent/Windows/ossource.cpp']);
        } else if (b.isWasi()) {
            t.addSources(['../glslang_osdependent_wasi.cc']);
        } else {
            t.addSources(['glslang/OSDependent/Unix/ossource.cpp']);
        }
        t.addIncludeDirectories(['.']);
        t.addIncludeDirectories({ dirs: ['glslang', '../generated' ], scope: 'private' });
        t.addCompileDefinitions({ defs: { ENABLE_OPT: '1' }, scope: 'private' });
        if (b.isWindows()) {
            t.addCompileDefinitions({ defs: { GLSLANG_OSINCLUDE_WIN32: '1' }, scope: 'private' });
        } else {
            t.addCompileDefinitions({ defs: { GLSLANG_OSINCLUDE_UNIX: '1'}, scope: 'private' });
        }
        if (b.isClang() || b.isGcc()) {
            t.addCompileOptions({
                opts: [
                    '-Wno-sign-compare',
                    '-Wno-deprecated-copy',
                    '-Wno-implicit-fallthrough',
                    '-Wno-unused-parameter',
                    '-Wno-unused-const-variable',
                    '-Wno-unused-but-set-variable'
                ],
                scope: 'private',
            });
        }
        t.addDependencies(['SPIRV-Tools']);
    });
}

const spirv_tools_sources = [
    'assembly_grammar.cpp',
    'binary.cpp',
    'diagnostic.cpp',
    'disassemble.cpp',
    'enum_string_mapping.cpp',
    'ext_inst.cpp',
    'extensions.cpp',
    'libspirv.cpp',
    'name_mapper.cpp',
    'opcode.cpp',
    'operand.cpp',
    'parsed_operand.cpp',
    'pch_source.cpp',
    'print.cpp',
    'software_version.cpp',
    'spirv_endian.cpp',
    'spirv_fuzzer_options.cpp',
    'spirv_optimizer_options.cpp',
    'spirv_reducer_options.cpp',
    'spirv_target_env.cpp',
    'spirv_validator_options.cpp',
    'table.cpp',
    'text_handler.cpp',
    'text.cpp',
    'to_string.cpp',

    'util/bit_vector.cpp',
    'util/parse_number.cpp',
    'util/string_utils.cpp',
    'util/timer.cpp',

    'val/basic_block.cpp',
    'val/construct.cpp',
    'val/function.cpp',
    'val/instruction.cpp',
    'val/validate_adjacency.cpp',
    'val/validate_annotation.cpp',
    'val/validate_arithmetics.cpp',
    'val/validate_atomics.cpp',
    'val/validate_barriers.cpp',
    'val/validate_bitwise.cpp',
    'val/validate_builtins.cpp',
    'val/validate_capability.cpp',
    'val/validate_cfg.cpp',
    'val/validate_composites.cpp',
    'val/validate_constants.cpp',
    'val/validate_conversion.cpp',
    'val/validate_debug.cpp',
    'val/validate_decorations.cpp',
    'val/validate_derivatives.cpp',
    'val/validate_execution_limitations.cpp',
    'val/validate_extensions.cpp',
    'val/validate_function.cpp',
    'val/validate_id.cpp',
    'val/validate_image.cpp',
    'val/validate_instruction.cpp',
    'val/validate_interfaces.cpp',
    'val/validate_invalid_type.cpp',
    'val/validate_layout.cpp',
    'val/validate_literals.cpp',
    'val/validate_logicals.cpp',
    'val/validate_memory_semantics.cpp',
    'val/validate_memory.cpp',
    'val/validate_mesh_shading.cpp',
    'val/validate_misc.cpp',
    'val/validate_mode_setting.cpp',
    'val/validate_non_uniform.cpp',
    'val/validate_primitives.cpp',
    'val/validate_ray_query.cpp',
    'val/validate_ray_tracing_reorder.cpp',
    'val/validate_ray_tracing.cpp',
    'val/validate_scopes.cpp',
    'val/validate_small_type_uses.cpp',
    'val/validate_tensor_layout.cpp',
    'val/validate_type.cpp',
    'val/validate.cpp',
    'val/validation_state.cpp',

    'opt/aggressive_dead_code_elim_pass.cpp',
    'opt/amd_ext_to_khr.cpp',
    'opt/analyze_live_input_pass.cpp',
    'opt/basic_block.cpp',
    'opt/block_merge_pass.cpp',
    'opt/block_merge_util.cpp',
    'opt/build_module.cpp',
    'opt/ccp_pass.cpp',
    'opt/cfg_cleanup_pass.cpp',
    'opt/cfg.cpp',
    'opt/code_sink.cpp',
    'opt/combine_access_chains.cpp',
    'opt/compact_ids_pass.cpp',
    'opt/composite.cpp',
    'opt/const_folding_rules.cpp',
    'opt/constants.cpp',
    'opt/control_dependence.cpp',
    'opt/convert_to_half_pass.cpp',
    'opt/convert_to_sampled_image_pass.cpp',
    'opt/copy_prop_arrays.cpp',
    'opt/dataflow.cpp',
    'opt/dead_branch_elim_pass.cpp',
    'opt/dead_insert_elim_pass.cpp',
    'opt/dead_variable_elimination.cpp',
    'opt/debug_info_manager.cpp',
    'opt/decoration_manager.cpp',
    'opt/def_use_manager.cpp',
    'opt/desc_sroa_util.cpp',
    'opt/desc_sroa.cpp',
    'opt/dominator_analysis.cpp',
    'opt/dominator_tree.cpp',
    'opt/eliminate_dead_constant_pass.cpp',
    'opt/eliminate_dead_functions_pass.cpp',
    'opt/eliminate_dead_functions_util.cpp',
    'opt/eliminate_dead_io_components_pass.cpp',
    'opt/eliminate_dead_members_pass.cpp',
    'opt/eliminate_dead_output_stores_pass.cpp',
    'opt/feature_manager.cpp',
    'opt/fix_func_call_arguments.cpp',
    'opt/fix_storage_class.cpp',
    'opt/flatten_decoration_pass.cpp',
    'opt/fold_spec_constant_op_and_composite_pass.cpp',
    'opt/fold.cpp',
    'opt/folding_rules.cpp',
    'opt/freeze_spec_constant_value_pass.cpp',
    'opt/function.cpp',
    'opt/graphics_robust_access_pass.cpp',
    'opt/if_conversion.cpp',
    'opt/inline_exhaustive_pass.cpp',
    'opt/inline_opaque_pass.cpp',
    'opt/inline_pass.cpp',
    'opt/instruction_list.cpp',
    'opt/instruction.cpp',
    'opt/interface_var_sroa.cpp',
    'opt/interp_fixup_pass.cpp',
    'opt/invocation_interlock_placement_pass.cpp',
    'opt/ir_context.cpp',
    'opt/ir_loader.cpp',
    'opt/licm_pass.cpp',
    'opt/liveness.cpp',
    'opt/local_access_chain_convert_pass.cpp',
    'opt/local_redundancy_elimination.cpp',
    'opt/local_single_block_elim_pass.cpp',
    'opt/local_single_store_elim_pass.cpp',
    'opt/loop_dependence_helpers.cpp',
    'opt/loop_dependence.cpp',
    'opt/loop_descriptor.cpp',
    'opt/loop_fission.cpp',
    'opt/loop_fusion_pass.cpp',
    'opt/loop_fusion.cpp',
    'opt/loop_peeling.cpp',
    'opt/loop_unroller.cpp',
    'opt/loop_unswitch_pass.cpp',
    'opt/loop_utils.cpp',
    'opt/mem_pass.cpp',
    'opt/merge_return_pass.cpp',
    'opt/modify_maximal_reconvergence.cpp',
    'opt/module.cpp',
    'opt/opextinst_forward_ref_fixup_pass.cpp',
    'opt/optimizer.cpp',
    'opt/pass_manager.cpp',
    'opt/pass.cpp',
    'opt/pch_source_opt.cpp',
    'opt/private_to_local_pass.cpp',
    'opt/propagator.cpp',
    'opt/reduce_load_size.cpp',
    'opt/redundancy_elimination.cpp',
    'opt/register_pressure.cpp',
    'opt/relax_float_ops_pass.cpp',
    'opt/remove_dontinline_pass.cpp',
    'opt/remove_duplicates_pass.cpp',
    'opt/remove_unused_interface_variables_pass.cpp',
    'opt/replace_desc_array_access_using_var_index.cpp',
    'opt/replace_invalid_opc.cpp',
    'opt/resolve_binding_conflicts_pass.cpp',
    'opt/scalar_analysis_simplification.cpp',
    'opt/scalar_analysis.cpp',
    'opt/scalar_replacement_pass.cpp',
    'opt/set_spec_constant_default_value_pass.cpp',
    'opt/simplification_pass.cpp',
    'opt/split_combined_image_sampler_pass.cpp',
    'opt/spread_volatile_semantics.cpp',
    'opt/ssa_rewrite_pass.cpp',
    'opt/strength_reduction_pass.cpp',
    'opt/strip_debug_info_pass.cpp',
    'opt/strip_nonsemantic_info_pass.cpp',
    'opt/struct_cfg_analysis.cpp',
    'opt/struct_packing_pass.cpp',
    'opt/switch_descriptorset_pass.cpp',
    'opt/trim_capabilities_pass.cpp',
    'opt/type_manager.cpp',
    'opt/types.cpp',
    'opt/unify_const_pass.cpp',
    'opt/upgrade_memory_model.cpp',
    'opt/value_number_table.cpp',
    'opt/vector_dce.cpp',
    'opt/workaround1209.cpp',
    'opt/wrap_opkill.cpp',
];

const glslslang_common_sources = [
    'glslang/CInterface/glslang_c_interface.cpp',
    'glslang/GenericCodeGen/CodeGen.cpp',
    'glslang/GenericCodeGen/Link.cpp',
    'glslang/MachineIndependent/preprocessor/PpAtom.cpp',
    'glslang/MachineIndependent/preprocessor/PpContext.cpp',
    'glslang/MachineIndependent/preprocessor/Pp.cpp',
    'glslang/MachineIndependent/preprocessor/PpScanner.cpp',
    'glslang/MachineIndependent/preprocessor/PpTokens.cpp',
    'glslang/MachineIndependent/attribute.cpp',
    'glslang/MachineIndependent/Constant.cpp',
    'glslang/MachineIndependent/glslang_tab.cpp',
    'glslang/MachineIndependent/InfoSink.cpp',
    'glslang/MachineIndependent/Initialize.cpp',
    'glslang/MachineIndependent/Intermediate.cpp',
    'glslang/MachineIndependent/intermOut.cpp',
    'glslang/MachineIndependent/IntermTraverse.cpp',
    'glslang/MachineIndependent/iomapper.cpp',
    'glslang/MachineIndependent/limits.cpp',
    'glslang/MachineIndependent/linkValidate.cpp',
    'glslang/MachineIndependent/parseConst.cpp',
    'glslang/MachineIndependent/ParseContextBase.cpp',
    'glslang/MachineIndependent/ParseHelper.cpp',
    'glslang/MachineIndependent/PoolAlloc.cpp',
    'glslang/MachineIndependent/propagateNoContraction.cpp',
    'glslang/MachineIndependent/reflection.cpp',
    'glslang/MachineIndependent/RemoveTree.cpp',
    'glslang/MachineIndependent/Scan.cpp',
    'glslang/MachineIndependent/ShaderLang.cpp',
    'glslang/MachineIndependent/SymbolTable.cpp',
    'glslang/MachineIndependent/Versions.cpp',
    'glslang/MachineIndependent/SpirvIntrinsics.cpp',
    'glslang/ResourceLimits/ResourceLimits.cpp',
    'SPIRV/disassemble.cpp',
    'SPIRV/doc.cpp',
    'SPIRV/GlslangToSpv.cpp',
    'SPIRV/InReadableOrder.cpp',
    'SPIRV/Logger.cpp',
    'SPIRV/SpvBuilder.cpp',
    'SPIRV/SpvPostProcess.cpp',
    'SPIRV/SPVRemapper.cpp',
    'SPIRV/SpvTools.cpp',
];
