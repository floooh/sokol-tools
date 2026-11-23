// NOTE: the storage image formats in this file are not supported
// on WGSL and GLSL ES
@cs cs_float_a
layout(local_size_x=8, local_size_y=8, local_size_z=1) in;
layout(binding=0, r8) uniform writeonly image2D img_r8;
layout(binding=1, r8_snorm) uniform writeonly image2D img_r8_snorm;
layout(binding=2, r16) uniform writeonly image2D img_r16;
layout(binding=3, r16_snorm) uniform writeonly image2D img_r16_snorm;
void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    imageStore(img_r8, pos, vec4(1.0));
    imageStore(img_r8_snorm, pos, vec4(1.0));
    imageStore(img_r16, pos, vec4(1.0));
    imageStore(img_r16_snorm, pos, vec4(1.0));
}
@end

@cs cs_float_b
layout(local_size_x=8, local_size_y=8, local_size_z=1) in;
layout(binding=0, r16f) uniform writeonly image2D img_r16f;
layout(binding=1, rg8) uniform writeonly image2D img_rg8;
layout(binding=2, rg8_snorm) uniform writeonly image2D img_rg8_snorm;
layout(binding=3, rg16) uniform writeonly image2D img_rg16;
void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    imageStore(img_r16f, pos, vec4(1.0));
    imageStore(img_rg8, pos, vec4(1.0));
    imageStore(img_rg8_snorm, pos, vec4(1.0));
    imageStore(img_rg16, pos, vec4(1.0));
}
@end

@cs cs_float_c
layout(local_size_x=8, local_size_y=8, local_size_z=1) in;
layout(binding=0, rg16_snorm) uniform writeonly image2D img_rg16_snorm;
layout(binding=1, rg16f) uniform writeonly image2D img_rg16f;
layout(binding=2, rgb10_a2) uniform writeonly image2D img_rgb10_a2;
layout(binding=3, r11f_g11f_b10f) uniform writeonly image2D img_r11f_g11f_b10f;
void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    imageStore(img_rg16_snorm, pos, vec4(1.0));
    imageStore(img_rg16f, pos, vec4(1.0));
    imageStore(img_rgb10_a2, pos, vec4(1.0));
    imageStore(img_r11f_g11f_b10f, pos, vec4(1.0));
}
@end

@cs cs_float_d
layout(local_size_x=8, local_size_y=8, local_size_z=1) in;
layout(binding=0, rgba16) uniform writeonly image2D img_rgba16;
layout(binding=1, rgba16_snorm) uniform writeonly image2D img_rgba16_snorm;
void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    imageStore(img_rgba16, pos, vec4(1.0));
    imageStore(img_rgba16_snorm, pos, vec4(1.0));
}
@end

@cs cs_uint
layout(local_size_x=8, local_size_y=8, local_size_z=1) in;
layout(binding=0, r8ui) uniform writeonly uimage2D img_r8ui;
layout(binding=1, rg8ui) uniform writeonly uimage2D img_rg8ui;
layout(binding=2, r16ui) uniform writeonly uimage2D img_r16ui;
layout(binding=3, rg16ui) uniform writeonly uimage2D img_rg16ui;
void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    imageStore(img_r8ui, pos, uvec4(1));
    imageStore(img_rg8ui, pos, uvec4(1));
    imageStore(img_r16ui, pos, uvec4(1));
    imageStore(img_rg16ui, pos, uvec4(1));
}
@end

@cs cs_int
layout(local_size_x=8, local_size_y=8, local_size_z=1) in;
layout(binding=0, r8i) uniform writeonly iimage2D img_r8i;
layout(binding=1, rg8i) uniform writeonly iimage2D img_rg8i;
layout(binding=2, r16i) uniform writeonly iimage2D img_r16i;
layout(binding=3, rg16i) uniform writeonly iimage2D img_rg16i;
void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    imageStore(img_r8i, pos, ivec4(1));
    imageStore(img_rg8i, pos, ivec4(1));
    imageStore(img_r16i, pos, ivec4(1));
    imageStore(img_rg16i, pos, ivec4(1));
}
@end

@program compute_storage_formats_float_a cs_float_a
@program compute_storage_formats_float_b cs_float_b
@program compute_storage_formats_float_c cs_float_c
@program compute_storage_formats_float_d cs_float_d
@program compute_storage_formats_uint cs_uint
@program compute_storage_formats_int cs_int
