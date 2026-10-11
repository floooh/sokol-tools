// test shader for writing storage buffer from fragment shader
@block common
struct counter {
    uint num_pixels;
    uint max_depth;
};
@end

@vs vs_count
layout(binding=0) uniform vs_params {
    mat4 mvp;
};
in vec4 position;
flat out uint inst_id;

void main() {
    gl_Position = mvp * position;
    inst_id = uint(gl_InstanceIndex);
}
@end

@fs fs_count
@include_block common

layout(binding=0) buffer fs_counters { counter counters[]; };

flat in uint inst_id;
out vec4 frag_color;

void main() {
    atomicAdd(counters[inst_id].num_pixels, 1);
    atomicMax(counters[inst_id].max_depth, uint(gl_FragCoord.z * 16777215.0));
    frag_color = vec4(1.0);
}
@end

@program count vs_count fs_count
