
@block shared
layout(binding=0) uniform params {
    mat4 mvp;
    float bla;
};
@end

@vs vs1
@include_block shared
in vec4 position;
out vec4 color;
void main() {
    gl_Position = position * mvp;
    color = vec4(bla, bla, bla, 1.0);
}
@end

@vs vs2
layout(binding=1) uniform xxx {
    float blub;
};
@include_block shared
in vec4 position;
out vec4 color;
void main() {
    gl_Position = position * mvp;
    color = vec4(1.0, 1.0, blub, 1.0);
}
@end

@fs fs
in vec4 color;
out vec4 fragColor;
void main() {
    fragColor = color;
}
@end

@program p1 vs1 fs
@program p2 vs2 fs