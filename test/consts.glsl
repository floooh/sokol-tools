@const int NUM_LIGHTS 4
@const uint MAX_ITEMS 0xFF
@const int MIN_INT -2147483648
@const uint MAX_UINT 4294967295
@const bool USE_FOG true
@const bool use_shadows false

@vs vs
layout(binding=0) uniform vs_params {
    vec4 light_pos[NUM_LIGHTS];
};
in vec4 pos;
void main() {
    vec4 p = pos;
    for (int i = 0; i < NUM_LIGHTS; i++) {
        p += light_pos[i];
    }
    if (USE_FOG && !use_shadows) {
        p *= float(MAX_ITEMS);
    }
    gl_Position = p;
}
@end

@fs fs
out vec4 frag_color;
void main() {
    frag_color = vec4(float(MIN_INT), float(MAX_UINT), 0.0, 1.0);
}
@end

@program consts vs fs
