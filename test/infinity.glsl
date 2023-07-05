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
uniform texture2D tex0;
uniform texture2D tex1;
uniform sampler smp;

in vec4 color;
in vec2 uv;
out vec4 frag_color;

void main() {
    vec4 color0 = texture(sampler2D(tex0, smp), uv);
    vec4 color1 = texture(sampler2D(tex1, smp), uv);
    frag_color = (color0 + color1) * color;
}
@end

@program texcube vs fs
