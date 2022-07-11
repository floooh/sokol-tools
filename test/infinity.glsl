@ctype mat4 hmm_mat4

@vs vs
uniform vs_params {
    mat4 mvp;
};

in vec4 pos;
in vec4 color0;
in vec2 texcoord0;

out vec4 color;
out vec2 uv;

void main() {
    gl_Position = mvp * pos;
    const float infinity = 1.0 / 0.0;
    color = color0 * infinity;
    uv = texcoord0 * 5.0;
}
@end

@fs fs
uniform sampler2D tex1;
uniform sampler2D tex0;

in vec4 color;
in vec2 uv;
out vec4 frag_color;

void main() {
    vec4 color0 = texture(tex0, uv);
    vec4 color1 = texture(tex1, uv);
    frag_color = (color0 + color1) * color;
}
@end

@program texcube vs fs

