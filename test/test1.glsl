/*
    this is a comment
*/
@vs my_vs
uniform params {
    mat4 mvp;
};

in vec4 position;
in vec4 color0;
out vec4 color;

void main() {
    gl_Position = mvp * position;
    color = color0;
}
@end

// the fragment shader
@fs my_fs
in vec4 color;
out vec4 fragColor;
void main() {
    fragColor = color;
}
@end

/* and the program */
@program my_prog my_vs my_fs

