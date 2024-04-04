// NOTE: this shader should crash with a SPIRVCross error 'Access chains that result in an array can not be flattened'
#pragma sokol @vs vs
uniform vs_params {
    vec4 mvp[4];
};

in vec4 position;
in vec2 texcoord0;

out vec2 uv;

vec4 xform(vec4 mat[4], vec4 v) {
    return vec4(dot(mat[0], position), dot(mat[1], position), dot(mat[2], position), dot(mat[3], position));
}

void main() {
    gl_Position = xform(mvp, position);
    uv = texcoord0;
}
#pragma sokol @end

#pragma sokol @fs fs
uniform sampler2D tex;

in vec2 uv;
out vec4 frag_color;

void main() {
    frag_color = texture(tex, uv);
}
#pragma sokol @end

#pragma sokol @program texgrid vs fs