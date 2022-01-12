@vs vs_test
layout(location=0) in vec2 a_position;
void main() {
    gl_Position = vec4(a_position, 0., 1.);
}
@end

@fs fs_test1
out vec4 out_color;
layout(binding = 0) uniform fs_block_test {
    vec4 u_color4;
    float u_a1;
};
void main() {
    out_color = u_color4;
}
@end

@fs fs_test2
out vec4 out_color;
layout(binding = 0) uniform fs_block_test {
    vec4 u_color4;
    float u_a1;
};
void main() {
    // out_color = vec4(u_a1);              // works
    out_color = vec4(1) * u_a1;             // doesn't work
}
@end

@program test1 vs_test fs_test1
@program test2 vs_test fs_test2