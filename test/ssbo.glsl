@vs vs
uniform vs_params {
    mat4 mvp;
};

struct vertex {
    vec4 pos;
    vec4 color;
};

readonly buffer ssbo {
    vertex vtx[];
};

out vec4 color;

void main() {
    gl_Position = mvp * vtx[gl_VertexIndex].pos;
    color = vtx[gl_VertexIndex].color;
}
@end

@fs fs
in vec4 color;
out vec4 frag_color;
void main() {
    frag_color = color;
}
@end

@program ssbo vs fs
