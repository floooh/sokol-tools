//------------------------------------------------------------------------------
//  Shader code for texcube-sapp sample.
//
//  NOTE: This source file also uses the '#pragma sokol' form of the
//  custom tags.
//------------------------------------------------------------------------------
#pragma sokol @ctype mat4 hmm_mat4

#pragma sokol @vs vs
uniform vs_params {
    mat4 mvp;
};

in vec4 pos;
in vec2 texcoord0;
in vec4 color0;

out vec4 color;
out vec2 uv;

void main() {
    gl_Position = mvp * pos;
    color = color0;
    uv = texcoord0 * 5.0;
}
#pragma sokol @end

#pragma sokol @fs fs
uniform texture2D tex0;
uniform texture2D tex1;
uniform sampler smp0;

in vec4 color;
in vec2 uv;
out vec4 frag_color;

void main() {
    vec4 c0 = texture(sampler2D(tex0, smp0), uv);
    vec4 c1 = texture(sampler2D(tex1, smp0), uv);
    frag_color = c0 + c1 + color;
}
#pragma sokol @end

#pragma sokol @program texcube vs fs
