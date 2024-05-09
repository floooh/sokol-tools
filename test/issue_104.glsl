// this should cause an internal SPIRV-Cross compiler error
@vs layer_vs
void main(void) {
   gl_Position =  vec4(1);
}
@end

@fs layer_fs
uniform texture2D textures[12];
uniform sampler smp;

out vec4 FragColor;

void main(void) {
   FragColor = texture(sampler2D(textures[0 ], smp), vec2(1));
}
@end

@program layer layer_vs layer_fs