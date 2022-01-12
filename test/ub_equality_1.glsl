@vs vs_test
layout(location=0) in vec2 a_position;
void main() {
    gl_Position = vec4(a_position, 0., 1.);
}
@end

@fs fs_test1
out vec4 out_color;
layout(binding = 0) uniform fs_block_test {
    vec4 u_color1;
    vec4 u_color2;
};
void main() {
    out_color = u_color1 * 0.5 + u_color2 * 0.5;
}
@end

@fs fs_test2
out vec4 out_color;
layout(binding = 0) uniform fs_block_test {
    vec4 u_color1;
    vec4 u_color2;
};
void main() {
    // out_color = u_color1;                            // works
    // out_color = u_color1 + u_color2;                 // works
    // out_color = u_color1 * 0.9 + u_color2 * 0.5;     // works

    out_color = 0.9 * u_color1 + u_color2;              // doesn't work
    // out_color = vec4(1) * u_color1;                  // doesn't work
    // out_color = vec4(1) * u_color1 + u_color2;       // doesn't work
}
@end

@program test1 vs_test fs_test1
@program test2 vs_test fs_test2