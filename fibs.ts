import { Configurer, Builder, ConfigDesc, Project, log, util } from 'jsr:@floooh/fibs@^1';

export function configure(c: Configurer): void {
    // add explicit macos x86_64 vs arm64 build configs
    const macos: ConfigDesc = {
        name: 'macos-base',
        platform: 'macos',
        generator: 'ninja',
        buildMode: 'debug',
    };
    const osx_x64 = { CMAKE_OSX_ARCHITECTURES: 'x86_64' };
    const osx_arm = { CMAKE_OSX_ARCHITECTURES: 'arm64' };
    c.addConfig({ ...macos, name: 'macos-x64-ninja-debug', buildMode: 'debug', cmakeVariables: osx_x64 });
    c.addConfig({ ...macos, name: 'macos-x64-ninja-release', buildMode: 'release', cmakeVariables: osx_x64 });
    c.addConfig({ ...macos, name: 'macos-arm-ninja-debug', buildMode: 'debug', cmakeVariables: osx_arm });
    c.addConfig({ ...macos, name: 'macos-arm-ninja-release', buildMode: 'release', cmakeVariables: osx_arm });

    // add a runtests command
    c.addCommand({ name: 'runtests', help: runTestsHelp, run: runTestsRun });
}

export function build(b: Builder): void {
    // global options
    b.addCmakeVariable('CMAKE_CXX_STANDARD', '20');
    if (b.isMsvc()) {
        // /W4: warning level 4
        // /MP: multithreaded compilation
        // /MT: statically linked stdlib
        b.addCompileOptions(['/W4', '/MP', '/MT']);
        b.addCompileOptions([ '/wd4715']);
        b.addCompileOptions({ opts: ['/Os'], buildMode: 'release' });
        b.addCompileDefinitions({
            _CRT_SECURE_NO_WARNINGS: '1',
            _SCL_SECURE_NO_WARNINGS: '1',
        });
    } else {
        b.addCompileOptions(['-Wall', '-Wextra', '-Wno-missing-field-initializers' ]);
        b.addCompileOptions({ opts: ['-Os'], buildMode: 'release' });
        if (b.isGcc()) {
            b.addCompileOptions(['-Wno-missing-braces']);
        }
        if (b.isClang()) {
            b.addLinkOptions(['-dead_strip'])
        }
    }

    // sokol-shdc executable
    b.addTarget('sokol-shdc', 'plain-exe', (t) => {
        t.setDir('src/shdc');
        t.addSources(sokol_shdc_sources);
        t.addIncludeDirectories(['.']);
        t.addDependencies(['fmt', 'getopt', 'pystring', 'glslang', 'SPIRV-Cross', 'tint']);
        if (b.isLinux()) {
            t.addLinkOptions(['-static']);
        }
        if (b.isGcc() || b.isClang()) {
            t.addCompileOptions(['-Wno-unused-result', '-Wno-unused-parameter']);
        }
        if (b.isMsvc()) {
            // unreferenced parameter
            // declaration of 'x' hides previous local declaration
            // conversion from 'x' to 'y' possible loss of data
            t.addCompileOptions(['/wd4100', '/wd4456', '/wd4244' ])
        }
    });

    // external libs
    b.addTarget('getopt', 'lib', (t) => {
        t.setDir('ext/getopt');
        t.setIdeFolder('ext');
        t.addSources([
            'src/getopt.c',
            'include/getopt/getopt.h'
        ]);
        if (b.isMsvc()) {
            // unreachable code
            b.addCompileOptions({ opts: ['/wd4702'], scope: 'private' });
        }
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
        if (b.isClang()) {
            t.addCompileOptions({
                opts: [ '-Wno-range-loop-analysis', '-Wno-deprecated-declarations' ],
                scope: 'private',
            });
        }
        if (b.isMsvc()) {
            // conversion from 'int' to 'uint32_t', signed/unsigned mismatch
            t.addCompileOptions({ opts: [ '/wd4127', '/wd4245' ], scope: 'private' });
            // conditional expression is constant
            t.addCompileOptions({ opts: [ '/wd4127'] , scope: 'public' });
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
        if (b.isMsvc()) {
            // declaration of 'x' hides class member
            // conversion from 'x' to 'y'...
            // signed/unsigned mismatch
            // declaration of 'x' hides function parameter
            // declaration of 'x' hides previous local declaration
            // operator '+' deprecated between enumerations of different types
            // conversion from 'x' to 'y' possible loss of data
            // conditional expression is constant
            t.addCompileOptions({
                opts: ['/wd4458', '/wd4244', '/wd4389', '/wd4457', '/wd4456', '/wd5054', '/wd4267', '/wd4127' ],
                scope: 'private'
            });
        }
        t.addDependencies(['SPIRV-Tools']);
    });
    b.addTarget('SPIRV-Cross', 'lib', (t) => {
        t.setDir('ext/SPIRV-Cross');
        t.setIdeFolder('ext');
        t.addSources(spirv_cross_sources);
        t.addIncludeDirectories(['.']);
        if (b.isClang()) {
            t.addCompileOptions({
                opts: [
                    '-Wno-range-loop-analysis',
                    '-Wno-deprecated-declarations',
                    '-Wno-deprecated-this-capture'
                ],
                scope: 'private',
            });
        }
        if (b.isMsvc()) {
            // 'return': conversion from 'x' to 'y'
            t.addCompileOptions({ opts: [ '/wd4244' ], scope: 'private' });
            // switch statement contains default but no case labels
            t.addCompileOptions({ opts: ['/wd4065'], scope: 'public' });
        }
    });
    b.addTarget('tint', 'lib', (t) => {
        t.setDir('ext/tint-extract');
        t.setIdeFolder('ext');
        t.addSources(tint_sources.map((f) => `src/tint/${f}`));
        t.addIncludeDirectories(['.', 'include']);
        t.addCompileDefinitions({
            TINT_BUILD_SPV_READER: '1',
            TINT_BUILD_WGSL_WRITER: '1',
        });
        t.addDependencies(['SPIRV-Tools']);
        if (b.isMsvc()) {
            // structure was padded...
            // 'return': conversion from 'x' to 'y'
            t.addCompileOptions({
                opts: ['/wd4457', '/wd4459', '/wd4244' ],
                scope: 'private'
            });
            // declaration of 'x' hides previous local declaration
            t.addCompileOptions({ opts: ['/wd4324'], scope: 'public' });
        }
    });
}

function runTestsHelp() {
    log.helpCmd([ 'runtests [config' ], 'run shader compilation tests');
}

async function runTestsRun(p: Project, args: string[]) {
    const configName = args[1];
    let config = p.activeConfig();
    if (configName !== undefined) {
        config = p.config(configName);
    }
    const cwd = `${p.dir()}/test`;
    const outDir = `${cwd}/out`;
    util.ensureDir(outDir);
    util.ensureDir(`${outDir}/sapp`);

    const cmd = `${p.targetDistDir('sokol-shdc', config.name)}/sokol-shdc`;
    for (const shd of test_shaders) {
        const res = await util.runCmd(cmd, {
            cwd,
            args: [
                '-i', shd,
                '-o', `${outDir}/${shd}.h`,
                '-l', 'glsl310es:glsl430:hlsl5:metal_macos:metal_ios:metal_sim',
                '-b',
            ]
        });
        if (res.exitCode !== 0) {
            Deno.exit(res.exitCode);
        }
    }
}

const test_shaders = [
    'chipvis.glsl',
    'fontstash.glsl',
    'imgui.glsl',
    'infinity.glsl',
    'inout_mismatch.glsl',
    'issue197_simple.glsl',
    // NOTE: this one crashes D3DCompiler_47.dll
    // 'issue197_complex.glsl',
    'sgl.glsl',
    'shared_ub.glsl',
    'test1.glsl',
    'test1_pragma.glsl',
    'test_nim.glsl',
    'ub_equality_1.glsl',
    'ub_equality_2.glsl',
    'uniform_types.glsl',
    'unused_vertex_attr.glsl',
    // sokol-samples shaders
    'sapp/arraytex-sapp.glsl',
    'sapp/blend-op-sapp.glsl',
    'sapp/blend-sapp.glsl',
    'sapp/bufferoffsets-sapp.glsl',
    'sapp/cgltf-sapp.glsl',
    'sapp/computeboids-sapp.glsl',
    'sapp/cube-sapp.glsl',
    'sapp/cubemap-jpeg-sapp.glsl',
    'sapp/cubemaprt-sapp.glsl',
    'sapp/customresolve-sapp.glsl',
    'sapp/debugtext-context-sapp.glsl',
    'sapp/drawcallperf-sapp.glsl',
    'sapp/dyntex-sapp.glsl',
    'sapp/dyntex3d-sapp.glsl',
    'sapp/fontstash-layers-sapp.glsl',
    'sapp/imgui-usercallback-sapp.glsl',
    'sapp/instancing-compute-sapp.glsl',
    'sapp/instancing-pull-sapp.glsl',
    'sapp/instancing-sapp.glsl',
    'sapp/layerrender-sapp.glsl',
    'sapp/loadpng-sapp.glsl',
    'sapp/mipmap-sapp.glsl',
    'sapp/miprender-sapp.glsl',
    'sapp/mrt-pixelformats-sapp.glsl',
    'sapp/mrt-sapp.glsl',
    'sapp/noentry-dll-sapp.glsl',
    'sapp/noentry-sapp.glsl',
    'sapp/noninterleaved-sapp.glsl',
    'sapp/offscreen-msaa-sapp.glsl',
    'sapp/offscreen-sapp.glsl',
    'sapp/ozz-skin-sapp.glsl',
    'sapp/ozz-storagebuffer-sapp.glsl',
    'sapp/pixelformats-sapp.glsl',
    'sapp/plmpeg-sapp.glsl',
    'sapp/primtypes-sapp.glsl',
    'sapp/quad-sapp.glsl',
    'sapp/restart-sapp.glsl',
    'sapp/sbuftex-sapp.glsl',
    'sapp/sdf-sapp.glsl',
    'sapp/shadows-depthtex-sapp.glsl',
    'sapp/shadows-sapp.glsl',
    'sapp/shapes-sapp.glsl',
    'sapp/shapes-transform-sapp.glsl',
    'sapp/shared-bindings-sapp.glsl',
    'sapp/shdfeatures-sapp.glsl',
    'sapp/tex3d-sapp.glsl',
    'sapp/texcube-sapp.glsl',
    'sapp/triangle-bufferless-sapp.glsl',
    'sapp/triangle-sapp.glsl',
    'sapp/uniformtypes-sapp.glsl',
    'sapp/uvwrap-sapp.glsl',
    'sapp/vertexindexbuffer-sapp.glsl',
    'sapp/vertexpull-sapp.glsl',
    'sapp/vertextexture-sapp.glsl',
    'sapp/write-storageimage-sapp.glsl',
];

const sokol_shdc_sources = [
    'args.cc',
    'args.h',
    'bytecode.cc',
    'bytecode.h',
    'input.cc',
    'input.h',
    'main.cc',
    'reflection.cc',
    'reflection.h',
    'spirv.cc',
    'spirv.h',
    'spirvcross.cc',
    'spirvcross.h',
    'util.cc',
    'util.h',
    'generators/bare.cc',
    'generators/bare.h',
    'generators/generate.cc',
    'generators/generate.h',
    'generators/generator.cc',
    'generators/generator.h',
    'generators/sokolc.cc',
    'generators/sokolc.h',
    'generators/sokolc2.cc',
    'generators/sokolc2.h',
    'generators/sokolc3.cc',
    'generators/sokolc3.h',
    'generators/sokold.cc',
    'generators/sokold.h',
    'generators/sokoljai.cc',
    'generators/sokoljai.h',
    'generators/sokolnim.cc',
    'generators/sokolnim.h',
    'generators/sokolodin.cc',
    'generators/sokolodin.h',
    'generators/sokolrust.cc',
    'generators/sokolrust.h',
    'generators/sokolzig.cc',
    'generators/sokolzig.h',
    'generators/yaml.cc',
    'generators/yaml.h',
    'types/reflection/bindings.h',
    'types/reflection/image_sample_type.h',
    'types/reflection/image_type.h',
    'types/reflection/program_reflection.h',
    'types/reflection/sampler_type.h',
    'types/reflection/sampler.h',
    'types/reflection/stage_attr.h',
    'types/reflection/stage_reflection.h',
    'types/reflection/storage_buffer.h',
    'types/reflection/storage_image.h',
    'types/reflection/storage_pixel_format.h',
    'types/reflection/texture_sampler.h',
    'types/reflection/texture.h',
    'types/reflection/type.h',
    'types/reflection/uniform_block.h',
    'types/bind_slot_map.h',
    'types/bindslot.h',
    'types/bytecode_blob.h',
    'types/consts.h',
    'types/errmsg.h',
    'types/format.h',
    'types/gen_input.h',
    'types/image_sample_type_tag.h',
    'types/line.h',
    'types/option.h',
    'types/program.h',
    'types/sampler_type_tag.h',
    'types/shader_stage.h',
    'types/slang.h',
    'types/snippet.h',
    'types/spirv_blob.h',
    'types/spirvcross_source.h',
];

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

const spirv_cross_sources = [
    'GLSL.std.450.h',
    'spirv_common.hpp',
    'spirv_cross_containers.hpp',
    'spirv_cross_error_handling.hpp',
    'spirv.hpp',
    'spirv_cross.hpp',
    'spirv_cross.cpp',
    'spirv_parser.hpp',
    'spirv_parser.cpp',
    'spirv_cross_parsed_ir.hpp',
    'spirv_cross_parsed_ir.cpp',
    'spirv_cfg.hpp',
    'spirv_cfg.cpp',
    'spirv_glsl.hpp',
    'spirv_glsl.cpp',
    'spirv_msl.hpp',
    'spirv_msl.cpp',
    'spirv_hlsl.hpp',
    'spirv_hlsl.cpp',
    'spirv_reflect.hpp',
    'spirv_reflect.cpp',
    'spirv_cross_util.hpp',
    'spirv_cross_util.cpp',
]

const tint_sources = [
    'api/common/vertex_pulling_config.cc',
    'api/tint.cc',
    'lang/core/access.cc',
    'lang/core/address_space.cc',
    'lang/core/attribute.cc',
    'lang/core/binary_op.cc',
    'lang/core/builtin_fn.cc',
    'lang/core/builtin_type.cc',
    'lang/core/builtin_value.cc',
    'lang/core/constant/composite.cc',
    'lang/core/constant/eval.cc',
    'lang/core/constant/invalid.cc',
    'lang/core/constant/manager.cc',
    'lang/core/constant/node.cc',
    'lang/core/constant/scalar.cc',
    'lang/core/constant/splat.cc',
    'lang/core/constant/value.cc',
    'lang/core/interpolation_sampling.cc',
    'lang/core/interpolation_type.cc',
    'lang/core/intrinsic/ctor_conv.cc',
    'lang/core/intrinsic/data.cc',
    'lang/core/intrinsic/table.cc',
    'lang/core/ir/access.cc',
    'lang/core/ir/analysis/integer_range_analysis.cc',
    'lang/core/ir/analysis/loop_analysis.cc',
    'lang/core/ir/binary.cc',
    'lang/core/ir/bitcast.cc',
    'lang/core/ir/block_param.cc',
    'lang/core/ir/block.cc',
    'lang/core/ir/break_if.cc',
    'lang/core/ir/builder.cc',
    'lang/core/ir/builtin_call.cc',
    'lang/core/ir/call.cc',
    'lang/core/ir/clone_context.cc',
    'lang/core/ir/const_param_validator.cc',
    'lang/core/ir/constant.cc',
    'lang/core/ir/constexpr_if.cc',
    'lang/core/ir/construct.cc',
    'lang/core/ir/continue.cc',
    'lang/core/ir/control_instruction.cc',
    'lang/core/ir/convert.cc',
    'lang/core/ir/core_binary.cc',
    'lang/core/ir/core_builtin_call.cc',
    'lang/core/ir/core_unary.cc',
    'lang/core/ir/disassembler.cc',
    'lang/core/ir/discard.cc',
    'lang/core/ir/evaluator.cc',
    'lang/core/ir/exit_if.cc',
    'lang/core/ir/exit_loop.cc',
    'lang/core/ir/exit_switch.cc',
    'lang/core/ir/exit.cc',
    'lang/core/ir/function_param.cc',
    'lang/core/ir/function.cc',
    'lang/core/ir/if.cc',
    'lang/core/ir/instruction_result.cc',
    'lang/core/ir/instruction.cc',
    'lang/core/ir/let.cc',
    'lang/core/ir/load_vector_element.cc',
    'lang/core/ir/load.cc',
    'lang/core/ir/loop.cc',
    'lang/core/ir/member_builtin_call.cc',
    'lang/core/ir/module.cc',
    'lang/core/ir/multi_in_block.cc',
    'lang/core/ir/next_iteration.cc',
    'lang/core/ir/operand_instruction.cc',
    'lang/core/ir/override.cc',
    'lang/core/ir/phony.cc',
    'lang/core/ir/reflection.cc',
    'lang/core/ir/return.cc',
    'lang/core/ir/store_vector_element.cc',
    'lang/core/ir/store.cc',
    'lang/core/ir/switch.cc',
    'lang/core/ir/swizzle.cc',
    'lang/core/ir/terminate_invocation.cc',
    'lang/core/ir/terminator.cc',
    'lang/core/ir/transform/add_empty_entry_point.cc',
    'lang/core/ir/transform/array_length_from_uniform.cc',
    'lang/core/ir/transform/bgra8unorm_polyfill.cc',
    'lang/core/ir/transform/binary_polyfill.cc',
    'lang/core/ir/transform/binding_remapper.cc',
    'lang/core/ir/transform/block_decorated_structs.cc',
    'lang/core/ir/transform/builtin_polyfill.cc',
    'lang/core/ir/transform/combine_access_instructions.cc',
    'lang/core/ir/transform/conversion_polyfill.cc',
    'lang/core/ir/transform/demote_to_helper.cc',
    'lang/core/ir/transform/direct_variable_access.cc',
    'lang/core/ir/transform/multiplanar_external_texture.cc',
    'lang/core/ir/transform/prepare_push_constants.cc',
    'lang/core/ir/transform/preserve_padding.cc',
    'lang/core/ir/transform/prevent_infinite_loops.cc',
    'lang/core/ir/transform/remove_continue_in_switch.cc',
    'lang/core/ir/transform/remove_terminator_args.cc',
    'lang/core/ir/transform/rename_conflicts.cc',
    'lang/core/ir/transform/robustness.cc',
    'lang/core/ir/transform/shader_io.cc',
    'lang/core/ir/transform/single_entry_point.cc',
    'lang/core/ir/transform/std140.cc',
    'lang/core/ir/transform/substitute_overrides.cc',
    'lang/core/ir/transform/value_to_let.cc',
    'lang/core/ir/transform/vectorize_scalar_matrix_constructors.cc',
    'lang/core/ir/transform/vertex_pulling.cc',
    'lang/core/ir/transform/zero_init_workgroup_memory.cc',
    'lang/core/ir/type/array_count.cc',
    'lang/core/ir/unary.cc',
    'lang/core/ir/unreachable.cc',
    'lang/core/ir/unused.cc',
    'lang/core/ir/user_call.cc',
    'lang/core/ir/validator.cc',
    'lang/core/ir/value.cc',
    'lang/core/ir/var.cc',
    'lang/core/number.cc',
    'lang/core/parameter_usage.cc',
    'lang/core/subgroup_matrix_kind.cc',
    'lang/core/texel_format.cc',
    'lang/core/type/abstract_float.cc',
    'lang/core/type/abstract_int.cc',
    'lang/core/type/abstract_numeric.cc',
    'lang/core/type/array_count.cc',
    'lang/core/type/array.cc',
    'lang/core/type/atomic.cc',
    'lang/core/type/binding_array.cc',
    'lang/core/type/bool.cc',
    'lang/core/type/builtin_structs.cc',
    'lang/core/type/depth_multisampled_texture.cc',
    'lang/core/type/depth_texture.cc',
    'lang/core/type/external_texture.cc',
    'lang/core/type/f16.cc',
    'lang/core/type/f32.cc',
    'lang/core/type/function.cc',
    'lang/core/type/i32.cc',
    'lang/core/type/i8.cc',
    'lang/core/type/input_attachment.cc',
    'lang/core/type/invalid.cc',
    'lang/core/type/manager.cc',
    'lang/core/type/matrix.cc',
    'lang/core/type/memory_view.cc',
    'lang/core/type/multisampled_texture.cc',
    'lang/core/type/node.cc',
    'lang/core/type/numeric_scalar.cc',
    'lang/core/type/pointer.cc',
    'lang/core/type/reference.cc',
    'lang/core/type/sampled_texture.cc',
    'lang/core/type/sampler_kind.cc',
    'lang/core/type/sampler.cc',
    'lang/core/type/scalar.cc',
    'lang/core/type/storage_texture.cc',
    'lang/core/type/struct.cc',
    'lang/core/type/subgroup_matrix.cc',
    'lang/core/type/texture_dimension.cc',
    'lang/core/type/texture.cc',
    'lang/core/type/type.cc',
    'lang/core/type/u32.cc',
    'lang/core/type/u64.cc',
    'lang/core/type/u8.cc',
    'lang/core/type/unique_node.cc',
    'lang/core/type/vector.cc',
    'lang/core/type/void.cc',
    'lang/core/unary_op.cc',
    'lang/spirv/builtin_fn.cc',
    'lang/spirv/intrinsic/data.cc',
    'lang/spirv/ir/builtin_call.cc',
    'lang/spirv/ir/image_from_texture.cc',
    'lang/spirv/ir/literal_operand.cc',
    'lang/spirv/reader/ast_lower/atomics.cc',
    'lang/spirv/reader/ast_lower/decompose_strided_array.cc',
    'lang/spirv/reader/ast_lower/decompose_strided_matrix.cc',
    'lang/spirv/reader/ast_lower/fold_trivial_lets.cc',
    'lang/spirv/reader/ast_lower/pass_workgroup_id_as_argument.cc',
    'lang/spirv/reader/ast_lower/transpose_row_major.cc',
    'lang/spirv/reader/ast_parser/ast_parser.cc',
    'lang/spirv/reader/ast_parser/construct.cc',
    'lang/spirv/reader/ast_parser/entry_point_info.cc',
    'lang/spirv/reader/ast_parser/enum_converter.cc',
    'lang/spirv/reader/ast_parser/function.cc',
    'lang/spirv/reader/ast_parser/namer.cc',
    'lang/spirv/reader/ast_parser/parse.cc',
    'lang/spirv/reader/ast_parser/type.cc',
    'lang/spirv/reader/ast_parser/usage.cc',
    'lang/spirv/reader/common/common.cc',
    'lang/spirv/reader/lower/atomics.cc',
    'lang/spirv/reader/lower/builtins.cc',
    'lang/spirv/reader/lower/lower.cc',
    'lang/spirv/reader/lower/shader_io.cc',
    'lang/spirv/reader/lower/vector_element_pointer.cc',
    'lang/spirv/reader/parser/parser.cc',
    'lang/spirv/reader/reader.cc',
    'lang/spirv/type/explicit_layout_array.cc',
    'lang/spirv/type/image.cc',
    'lang/spirv/type/sampled_image.cc',
    'lang/spirv/validate/validate.cc',
    'lang/spirv/writer/common/binary_writer.cc',
    'lang/spirv/writer/common/function.cc',
    'lang/spirv/writer/common/instruction.cc',
    'lang/spirv/writer/common/module.cc',
    'lang/spirv/writer/common/operand.cc',
    'lang/spirv/writer/common/option_helper.cc',
    'lang/spirv/writer/common/output.cc',
    'lang/spirv/writer/helpers/generate_bindings.cc',
    'lang/spirv/writer/printer/printer.cc',
    'lang/spirv/writer/raise/builtin_polyfill.cc',
    'lang/spirv/writer/raise/expand_implicit_splats.cc',
    'lang/spirv/writer/raise/fork_explicit_layout_types.cc',
    'lang/spirv/writer/raise/handle_matrix_arithmetic.cc',
    'lang/spirv/writer/raise/merge_return.cc',
    'lang/spirv/writer/raise/pass_matrix_by_pointer.cc',
    'lang/spirv/writer/raise/raise.cc',
    'lang/spirv/writer/raise/remove_unreachable_in_loop_continuing.cc',
    'lang/spirv/writer/raise/shader_io.cc',
    'lang/spirv/writer/raise/var_for_dynamic_index.cc',
    'lang/spirv/writer/writer.cc',
    'lang/wgsl/ast/accessor_expression.cc',
    'lang/wgsl/ast/alias.cc',
    'lang/wgsl/ast/assignment_statement.cc',
    'lang/wgsl/ast/attribute.cc',
    'lang/wgsl/ast/binary_expression.cc',
    'lang/wgsl/ast/binding_attribute.cc',
    'lang/wgsl/ast/blend_src_attribute.cc',
    'lang/wgsl/ast/block_statement.cc',
    'lang/wgsl/ast/bool_literal_expression.cc',
    'lang/wgsl/ast/break_if_statement.cc',
    'lang/wgsl/ast/break_statement.cc',
    'lang/wgsl/ast/builder.cc',
    'lang/wgsl/ast/builtin_attribute.cc',
    'lang/wgsl/ast/call_expression.cc',
    'lang/wgsl/ast/call_statement.cc',
    'lang/wgsl/ast/case_selector.cc',
    'lang/wgsl/ast/case_statement.cc',
    'lang/wgsl/ast/clone_context.cc',
    'lang/wgsl/ast/color_attribute.cc',
    'lang/wgsl/ast/compound_assignment_statement.cc',
    'lang/wgsl/ast/const_assert.cc',
    'lang/wgsl/ast/const.cc',
    'lang/wgsl/ast/continue_statement.cc',
    'lang/wgsl/ast/diagnostic_attribute.cc',
    'lang/wgsl/ast/diagnostic_control.cc',
    'lang/wgsl/ast/diagnostic_directive.cc',
    'lang/wgsl/ast/diagnostic_rule_name.cc',
    'lang/wgsl/ast/disable_validation_attribute.cc',
    'lang/wgsl/ast/discard_statement.cc',
    'lang/wgsl/ast/enable.cc',
    'lang/wgsl/ast/expression.cc',
    'lang/wgsl/ast/extension.cc',
    'lang/wgsl/ast/float_literal_expression.cc',
    'lang/wgsl/ast/for_loop_statement.cc',
    'lang/wgsl/ast/function.cc',
    'lang/wgsl/ast/group_attribute.cc',
    'lang/wgsl/ast/id_attribute.cc',
    'lang/wgsl/ast/identifier_expression.cc',
    'lang/wgsl/ast/identifier.cc',
    'lang/wgsl/ast/if_statement.cc',
    'lang/wgsl/ast/increment_decrement_statement.cc',
    'lang/wgsl/ast/index_accessor_expression.cc',
    'lang/wgsl/ast/input_attachment_index_attribute.cc',
    'lang/wgsl/ast/int_literal_expression.cc',
    'lang/wgsl/ast/internal_attribute.cc',
    'lang/wgsl/ast/interpolate_attribute.cc',
    'lang/wgsl/ast/invariant_attribute.cc',
    'lang/wgsl/ast/let.cc',
    'lang/wgsl/ast/literal_expression.cc',
    'lang/wgsl/ast/location_attribute.cc',
    'lang/wgsl/ast/loop_statement.cc',
    'lang/wgsl/ast/member_accessor_expression.cc',
    'lang/wgsl/ast/module.cc',
    'lang/wgsl/ast/must_use_attribute.cc',
    'lang/wgsl/ast/node.cc',
    'lang/wgsl/ast/override.cc',
    'lang/wgsl/ast/parameter.cc',
    'lang/wgsl/ast/phony_expression.cc',
    'lang/wgsl/ast/pipeline_stage.cc',
    'lang/wgsl/ast/requires.cc',
    'lang/wgsl/ast/return_statement.cc',
    'lang/wgsl/ast/row_major_attribute.cc',
    'lang/wgsl/ast/stage_attribute.cc',
    'lang/wgsl/ast/statement.cc',
    'lang/wgsl/ast/stride_attribute.cc',
    'lang/wgsl/ast/struct_member_align_attribute.cc',
    'lang/wgsl/ast/struct_member_offset_attribute.cc',
    'lang/wgsl/ast/struct_member_size_attribute.cc',
    'lang/wgsl/ast/struct_member.cc',
    'lang/wgsl/ast/struct.cc',
    'lang/wgsl/ast/switch_statement.cc',
    'lang/wgsl/ast/templated_identifier.cc',
    'lang/wgsl/ast/transform/add_empty_entry_point.cc',
    'lang/wgsl/ast/transform/array_length_from_uniform.cc',
    'lang/wgsl/ast/transform/binding_remapper.cc',
    'lang/wgsl/ast/transform/builtin_polyfill.cc',
    'lang/wgsl/ast/transform/canonicalize_entry_point_io.cc',
    'lang/wgsl/ast/transform/data.cc',
    'lang/wgsl/ast/transform/demote_to_helper.cc',
    'lang/wgsl/ast/transform/direct_variable_access.cc',
    'lang/wgsl/ast/transform/disable_uniformity_analysis.cc',
    'lang/wgsl/ast/transform/expand_compound_assignment.cc',
    'lang/wgsl/ast/transform/first_index_offset.cc',
    'lang/wgsl/ast/transform/fold_constants.cc',
    'lang/wgsl/ast/transform/get_insertion_point.cc',
    'lang/wgsl/ast/transform/hoist_to_decl_before.cc',
    'lang/wgsl/ast/transform/manager.cc',
    'lang/wgsl/ast/transform/multiplanar_external_texture.cc',
    'lang/wgsl/ast/transform/preserve_padding.cc',
    'lang/wgsl/ast/transform/promote_initializers_to_let.cc',
    'lang/wgsl/ast/transform/promote_side_effects_to_decl.cc',
    'lang/wgsl/ast/transform/remove_continue_in_switch.cc',
    'lang/wgsl/ast/transform/remove_phonies.cc',
    'lang/wgsl/ast/transform/remove_unreachable_statements.cc',
    'lang/wgsl/ast/transform/renamer.cc',
    'lang/wgsl/ast/transform/robustness.cc',
    'lang/wgsl/ast/transform/simplify_pointers.cc',
    'lang/wgsl/ast/transform/single_entry_point.cc',
    'lang/wgsl/ast/transform/substitute_override.cc',
    'lang/wgsl/ast/transform/transform.cc',
    'lang/wgsl/ast/transform/unshadow.cc',
    'lang/wgsl/ast/transform/vectorize_scalar_matrix_initializers.cc',
    'lang/wgsl/ast/transform/vertex_pulling.cc',
    'lang/wgsl/ast/transform/zero_init_workgroup_memory.cc',
    'lang/wgsl/ast/type_decl.cc',
    'lang/wgsl/ast/type.cc',
    'lang/wgsl/ast/unary_op_expression.cc',
    'lang/wgsl/ast/var.cc',
    'lang/wgsl/ast/variable_decl_statement.cc',
    'lang/wgsl/ast/variable.cc',
    'lang/wgsl/ast/while_statement.cc',
    'lang/wgsl/ast/workgroup_attribute.cc',
    'lang/wgsl/builtin_fn.cc',
    'lang/wgsl/diagnostic_rule.cc',
    'lang/wgsl/diagnostic_severity.cc',
    'lang/wgsl/extension.cc',
    'lang/wgsl/feature_status.cc',
    'lang/wgsl/inspector/entry_point.cc',
    'lang/wgsl/inspector/inspector.cc',
    'lang/wgsl/inspector/resource_binding.cc',
    'lang/wgsl/inspector/scalar.cc',
    'lang/wgsl/intrinsic/ctor_conv.cc',
    'lang/wgsl/intrinsic/data.cc',
    'lang/wgsl/ir/builtin_call.cc',
    'lang/wgsl/ir/unary.cc',
    'lang/wgsl/language_feature.cc',
    'lang/wgsl/program/clone_context.cc',
    'lang/wgsl/program/program_builder.cc',
    'lang/wgsl/program/program.cc',
    'lang/wgsl/reserved_words.cc',
    'lang/wgsl/resolver/dependency_graph.cc',
    'lang/wgsl/resolver/incomplete_type.cc',
    'lang/wgsl/resolver/resolve.cc',
    'lang/wgsl/resolver/resolver.cc',
    'lang/wgsl/resolver/sem_helper.cc',
    'lang/wgsl/resolver/uniformity.cc',
    'lang/wgsl/resolver/unresolved_identifier.cc',
    'lang/wgsl/resolver/validator.cc',
    'lang/wgsl/sem/accessor_expression.cc',
    'lang/wgsl/sem/array_count.cc',
    'lang/wgsl/sem/array.cc',
    'lang/wgsl/sem/behavior.cc',
    'lang/wgsl/sem/block_statement.cc',
    'lang/wgsl/sem/break_if_statement.cc',
    'lang/wgsl/sem/builtin_enum_expression.cc',
    'lang/wgsl/sem/builtin_fn.cc',
    'lang/wgsl/sem/call_target.cc',
    'lang/wgsl/sem/call.cc',
    'lang/wgsl/sem/expression.cc',
    'lang/wgsl/sem/for_loop_statement.cc',
    'lang/wgsl/sem/function_expression.cc',
    'lang/wgsl/sem/function.cc',
    'lang/wgsl/sem/if_statement.cc',
    'lang/wgsl/sem/index_accessor_expression.cc',
    'lang/wgsl/sem/info.cc',
    'lang/wgsl/sem/load.cc',
    'lang/wgsl/sem/loop_statement.cc',
    'lang/wgsl/sem/materialize.cc',
    'lang/wgsl/sem/member_accessor_expression.cc',
    'lang/wgsl/sem/module.cc',
    'lang/wgsl/sem/node.cc',
    'lang/wgsl/sem/statement.cc',
    'lang/wgsl/sem/struct.cc',
    'lang/wgsl/sem/switch_statement.cc',
    'lang/wgsl/sem/type_expression.cc',
    'lang/wgsl/sem/value_constructor.cc',
    'lang/wgsl/sem/value_conversion.cc',
    'lang/wgsl/sem/value_expression.cc',
    'lang/wgsl/sem/variable.cc',
    'lang/wgsl/sem/while_statement.cc',
    'lang/wgsl/writer/ast_printer/ast_printer.cc',
    'lang/wgsl/writer/ir_to_program/ir_to_program.cc',
    'lang/wgsl/writer/options.cc',
    'lang/wgsl/writer/output.cc',
    'lang/wgsl/writer/raise/ptr_to_ref.cc',
    'lang/wgsl/writer/raise/raise.cc',
    'lang/wgsl/writer/raise/value_to_let.cc',
    'lang/wgsl/writer/syntax_tree_printer/syntax_tree_printer.cc',
    'lang/wgsl/writer/writer.cc',
    'utils/bytes/buffer_reader.cc',
    'utils/bytes/reader.cc',
    'utils/containers/containers.cc',
    'utils/diagnostic/diagnostic.cc',
    'utils/diagnostic/formatter.cc',
    'utils/diagnostic/source.cc',
    'utils/generation_id.cc',
    'utils/ice/debugger.cc',
    'utils/ice/ice.cc',
    'utils/macros/macros.cc',
    'utils/math/math.cc',
    'utils/memory/memory.cc',
    'utils/reflection.cc',
    'utils/result.cc',
    'utils/rtti/castable.cc',
    'utils/rtti/switch.cc',
    'utils/strconv/float_to_string.cc',
    'utils/symbol/symbol_table.cc',
    'utils/symbol/symbol.cc',
    'utils/system/env_other.cc',
    'utils/system/terminal_other.cc',
    'utils/text_generator/text_generator.cc',
    'utils/text/base64.cc',
    'utils/text/color_mode.cc',
    'utils/text/string_stream.cc',
    'utils/text/string.cc',
    'utils/text/styled_text_printer_other.cc',
    'utils/text/styled_text_printer.cc',
    'utils/text/styled_text_theme.cc',
    'utils/text/styled_text.cc',
    'utils/text/unicode.cc',
];
