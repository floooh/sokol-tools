// NOTE: int64_t and uint64_t is not supported on most output
// shader dialects (HLSL5, MSL, WGSL and most GLSL versions)
@cs cs
#extension GL_ARB_gpu_shader_int64 : require

layout(binding=0) uniform params {
    int num_items;
};

struct item_t {
    uint flags;
    uint64_t val_u64;
    int val_i32;
    int64_t val_i64;
    uint64_t u64_arr[3];
    int64_t i64_arr[2];
    vec4 pos;
};

layout(binding=0) buffer ssbo_out {
    item_t items_out[];
};

layout(binding=1) readonly buffer ssbo_in {
    item_t items_in[];
};

layout(local_size_x=64, local_size_y=1, local_size_z=1) in;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= num_items) {
        return;
    }
    item_t item = items_in[idx];
    item.flags |= 1;
    item.val_u64 += uint64_t(idx) << 32;
    item.val_i32 = -item.val_i32;
    item.val_i64 = -item.val_i64 - int64_t(item.val_i32);
    for (int i = 0; i < 3; i++) {
        item.u64_arr[i] = item.u64_arr[i] * 2ul;
    }
    for (int i = 0; i < 2; i++) {
        item.i64_arr[i] = item.i64_arr[i] - 1l;
    }
    item.pos += vec4(1.0);
    items_out[idx] = item;
}
@end

@program compute_int64 cs
