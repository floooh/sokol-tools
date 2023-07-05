#pragma sokol @module bla
/*
    this is a comment
*/
#pragma sokol @ctype float float
#pragma sokol @ctype vec2 hmm_vec2
#pragma sokol @ctype vec3 hmm_vec3
#pragma sokol @ctype vec4 hmm_vec4
#pragma sokol @ctype mat4 hmm_mat4

#pragma sokol @block uniforms
layout(binding=0) uniform params {
    mat4 mvp;
};
layout(binding=1) uniform params2 {
    vec2 bla;
};
#pragma sokol @end

#pragma sokol @block textures
layout(binding=0) uniform texture2D tex0;
layout(binding=1) uniform texture2D tex1;
layout(binding=2) uniform sampler smp;
#pragma sokol @end

#pragma sokol @vs vs1
#pragma sokol @msl_options flip_vert_y
#pragma sokol @include_block uniforms

layout(location=0) in vec4 position;
layout(location=1) in vec2 texcoord0;
layout(location=2) in vec4 color0;

out vec2 uv;
out vec4 color;

void main() {
    gl_Position = mvp * position;
    color = color0;
    color.xy += bla;
}
#pragma sokol @end

// the fragment shader
#pragma sokol @fs fs1
#pragma sokol @include_block textures

in vec2 uv;
in vec4 color;

out vec4 fragColor;

void main() {
    fragColor = texture(sampler2D(tex0, smp), uv) * texture(sampler2D(tex1, smp), uv*2.0) * color;
}
#pragma sokol @end

/* and the program */
#pragma sokol @program prog1 vs1 fs1

#pragma sokol @vs vs2
layout(binding=0) uniform params {
    mat4 mvp;
};
layout(binding=1) uniform params2 {
    vec2 bla;
};

layout(location=0) in vec4 pos;
layout(location=0) out vec2 uv;

void main() {
    gl_Position = mvp * pos;
    uv = pos.xy;
}
#pragma sokol @end

#pragma sokol @fs fs2
#pragma sokol @include_block textures
layout(location=0) in vec2 uv;
layout(location=0) out vec4 fragColor;

void main() {
    fragColor = texture(sampler2D(tex0, smp), uv) + texture(sampler2D(tex1, smp), uv);
}
#pragma sokol @end

#pragma sokol @program prog2 vs2 fs2
