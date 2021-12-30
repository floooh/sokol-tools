
@vs vs
uniform params {
    mat4 mvp[3];
};
uniform bparams {
    ivec4 b2[2];
};

in vec4 position;
in vec2 texcoord0;
in vec4 color0;

void main() {
    gl_Position = position * mvp[b2[0].x];
}
@end

@fs fs
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
@end

@program shd vs fs 
