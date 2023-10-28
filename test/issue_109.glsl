@ctype mat4 hmm_mat4

@vs vs
uniform vs_params {
    mat4 mvp;
};

in vec4 pos;
in vec2 texcoord0;

out vec2 uv;

void main() {
    gl_Position = mvp * pos;
    uv = texcoord0 * 5.0;
}
@end

@fs fs
uniform texture2D u_atlas_texture;
uniform texture2D u_atlas_outline;
uniform sampler u_atlas_sampler;

in vec2 uv;
out vec4 frag_color;

void main() {
  vec2 atlas_size = vec2(textureSize(sampler2D(u_atlas_texture, u_atlas_sampler), 0));
  vec2 v_texture_coord = uv / atlas_size;
  vec4 fill = texture(sampler2D(u_atlas_texture, u_atlas_sampler), v_texture_coord);
  vec4 outline = texture(sampler2D(u_atlas_outline, u_atlas_sampler), v_texture_coord);
  frag_color = fill + outline * (1.0 - fill.a);
}
@end

@program texcube vs fs
