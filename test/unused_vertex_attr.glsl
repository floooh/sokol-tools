@module debug

// Uniform type definitions on CPU side
@ctype mat4 ab::mat4_t
@ctype vec2 ab::vec2_t
@ctype vec3 ab::vec3_t
@ctype vec4 ab::vec4_t

// Vertex shader
@vs vs
layout(binding = 0) uniform vs_uniforms {
    mat4 u_projMat;
    mat4 u_modelMat;
};

//layout(location = 0) in vec2 a_position;
//layout(location = 1) in vec2 a_texCoords;
//layout(location = 2) in vec4 a_color;

in vec2 a_position;
in vec2 a_texCoords;
in vec4 a_color;

out vec4 v_color;

void main() {
  v_color      = a_color;
  gl_Position  = u_projMat * u_modelMat * vec4(a_position, 0, 1);
  gl_PointSize = 10.0;
}
@end

// Fragment shader
@fs fs
in vec4 v_color;

out vec4 out_color;

void main() {
    out_color = v_color;
}
@end

#pragma sokol @program prog vs fs