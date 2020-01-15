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
in vec4 color0;
in vec2 texcoord0;

out vec4 color;
out vec2 uv;

void main() {
    gl_Position = mvp * pos;
    color = color0;
    uv = texcoord0 * 5.0;
}
#pragma sokol @end

#pragma sokol @fs fs
layout(binding=0) uniform sampler2D tex1;
layout(binding=1) uniform sampler2D tex0;

in vec4 color;
in vec2 uv;
out vec4 frag_color;

void main() {
    vec4 color0 = texture(tex0, uv);
    vec4 color1 = texture(tex1, uv);
    frag_color = (color0 + color1) * color;
}
#pragma sokol @end

#pragma sokol @program texcube vs fs
