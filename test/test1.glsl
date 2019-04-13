/*
    this is a comment
*/
@block vs_uniforms
uniform params {
    mat4 mvp;
};
@end

@block vs_inputs
layout(location=0) in vec4 position;
layout(location=1) in vec4 color0;
layout(location=0) out vec4 color;
@end

@block fs_inputs
layout(location=0) in vec4 color;
layout(location=0) out vec4 fragColor;
@end

@vs my_vs
@include_block vs_uniforms
@include_block vs_inputs
void main() {
    gl_Position = mvp * position;
    color = color0;
}
@end

// the fragment shader
@fs my_fs
@include_block fs_inputs
void main() {
    fragColor = color;
}
@end

/* and the program */
@program my_prog my_vs my_fs

