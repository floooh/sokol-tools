import sys, os
from mod import log, project, settings

shaders = [
    'chipvis.glsl',
    'fontstash.glsl',
    'imgui.glsl',
    'infinity.glsl',
    'inout_mismatch.glsl',
    'sgl.glsl',
    'shared_ub.glsl',
    'test1.glsl',
    'test1_pragma.glsl',
    'test_nim.glsl',
    'ub_equality_1.glsl',
    'ub_equality_2.glsl',
    'uniform_types.glsl',
    'unused_vertex_attr.glsl',
    # sokol-samples shaders
    'sapp/arraytex-sapp.glsl',
    'sapp/blend-sapp.glsl',
    'sapp/bufferoffsets-sapp.glsl',
    'sapp/cgltf-sapp.glsl',
    'sapp/cube-sapp.glsl',
    'sapp/cubemap-jpeg-sapp.glsl',
    'sapp/cubemaprt-sapp.glsl',
    'sapp/debugtext-context-sapp.glsl',
    'sapp/drawcallperf-sapp.glsl',
    'sapp/dyntex-sapp.glsl',
    'sapp/dyntex3d-sapp.glsl',
    'sapp/fontstash-layers-sapp.glsl',
    'sapp/imgui-usercallback-sapp.glsl',
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
    'sapp/shdfeatures-sapp.glsl',
    'sapp/tex3d-sapp.glsl',
    'sapp/texcube-sapp.glsl',
    'sapp/triangle-bufferless-sapp.glsl',
    'sapp/triangle-sapp.glsl',
    'sapp/uniformtypes-sapp.glsl',
    'sapp/uvwrap-sapp.glsl',
    'sapp/vertexpull-sapp.glsl',
]

def run_sokol_shdc(fips_dir, proj_dir, cfg_name, out_path, shader_filename):
    if cfg_name is None:
        cfg_name = settings.get(proj_dir, 'config')
    cwd = proj_dir + '/test'
    args = [
        '-i', shader_filename,
        '-o', f'{out_path}/{shader_filename}.h',
        '-l', 'glsl300es:glsl430:hlsl4:metal_macos:metal_ios:metal_sim',
        '-b',
    ]
    log.info(f'==> {shader_filename} => {out_path}/{shader_filename}.h:')
    exit_code = project.run(fips_dir, proj_dir, cfg_name, 'sokol-shdc', args, cwd)
    if exit_code != 0:
        sys.exit(exit_code)

def run(fips_dir, proj_dir, args):
    cfg_name = None
    if len(args) > 0:
        cfg_name = args[0]
    out_path = f'{proj_dir}/test/out'
    if not os.path.isdir(out_path):
        os.makedirs(out_path)
    if not os.path.isdir(f'{out_path}/sapp'):
        os.makedirs(f'{out_path}/sapp')
    for shader in shaders:
        run_sokol_shdc(fips_dir, proj_dir, cfg_name, out_path, shader)

def help():
    log.info(log.YELLOW + 'fips run_tests [cfg]\n' + log.DEF + '    run shader compilation tests')
