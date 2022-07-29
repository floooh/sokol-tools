/*
    this is a comment
*/
@header import glm as glm
@ctype float float32
@ctype vec2 glm.Vec2f
@ctype vec3 glm.Vec3f
@ctype vec4 glm.Vec4f
@ctype mat4 glm.Mat4f

@block uniforms
layout(binding=0) uniform params {
    mat4 mvp;
};
layout(binding=1) uniform params2 {
    vec2 bla;
};
@end

@block textures
layout(binding=0) uniform sampler2D tex0;
layout(binding=1) uniform sampler2D tex1;
@end

@vs vs1
@msl_options flip_vert_y
@include_block uniforms

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
@end

// the fragment shader
@fs fs1
@include_block textures

in vec2 uv;
in vec4 color;

out vec4 fragColor;

void main() {
    fragColor = texture(tex0, uv) * texture(tex1, uv*2.0) * color;
}
@end

/* and the program */
@program prog1 vs1 fs1

@vs vs2
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
@end

@fs fs2
@include_block textures
layout(location=0) in vec2 uv;
layout(location=0) out vec4 fragColor;

void main() {
    fragColor = texture(tex0, uv) + texture(tex1, uv);
}
@end

@program prog2 vs2 fs2
