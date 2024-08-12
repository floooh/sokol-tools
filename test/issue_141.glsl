@ctype mat4 Mat4
@ctype vec4 Float4
@ctype vec3 Float3
@ctype vec2 Float2


//
// Mark: Layer
//

@vs layer_vs

in mat2 aRotateScale;
in mat2 aUVRotateScale;
in vec4 aColor; // premultiplied alpha
in vec4 aBorderColor; // premultiplied alpha
in vec2 aUVTranslate;
in vec2 aVertexPosition;
in vec2 aTranslate;
in vec2 aSize;
in float aCornerRadius;
in float aParam;
in float aSamplerIndex;

#define aBorderWidthOrShadowRadius aParam // sokol doesn't allow attributes names longer than 16 bytes

out vec4 color;
out vec4 borderColor; // premultiplied alpha
out vec2 coord;
out vec2 uv;
out vec2 half_size;
out float cornerRadius;
out float borderWidthOrShadowRadius;
out float samplerIndex;

void main(void) {
    color = aColor;
    half_size = aSize/2.0;
    cornerRadius = aCornerRadius;
    borderWidthOrShadowRadius = aBorderWidthOrShadowRadius;
    borderColor = aBorderColor;
    samplerIndex = aSamplerIndex;

    gl_Position =  vec4(aRotateScale * aVertexPosition + aTranslate, 1, 1);
    coord = (aVertexPosition - 0.5) * aSize;
    uv = aUVRotateScale * aVertexPosition + aUVTranslate;
}
@end

@fs layer_fs
#define MAX_SUPPORTED_SAMPLERS 12

uniform texture2D texture0;
uniform texture2D texture1;
uniform texture2D texture2;
uniform texture2D texture3;
uniform texture2D texture4;
uniform texture2D texture5;
uniform texture2D texture6;
uniform texture2D texture7;
uniform texture2D texture8;
uniform texture2D texture9;
uniform texture2D texture10;
uniform texture2D texture11;
uniform sampler layer_sampler;
in vec4 color; // must be premultiplied alpha
in vec4 borderColor; // must be premultiplied alpha
in vec2 coord;
in vec2 uv;
in vec2 half_size;
in float cornerRadius;
in float borderWidthOrShadowRadius;
in float samplerIndex;

out vec4 FragColor;

float round_rect_sdf(vec2 p, vec2 rect_half_size, float radius){
    vec2 d = abs(p) - rect_half_size + radius;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - radius;
}

vec4 source_over(vec4 upper,vec4 lower) {
    return upper + lower*(1.0 - upper.a);
}

void main(void) {

        float b = round_rect_sdf( coord, half_size, cornerRadius );

        vec4 image_color = vec4(0);

        if      (samplerIndex < 0.5)  {image_color = texture(sampler2D(texture0, layer_sampler), uv);}
        else if (samplerIndex < 1.5)  {image_color = texture(sampler2D(texture1, layer_sampler), uv);}
        else if (samplerIndex < 2.5)  {image_color = texture(sampler2D(texture2, layer_sampler), uv);}
        else if (samplerIndex < 3.5)  {image_color = texture(sampler2D(texture3, layer_sampler), uv);}
        else if (samplerIndex < 4.5)  {image_color = texture(sampler2D(texture4, layer_sampler), uv);}
        else if (samplerIndex < 5.5)  {image_color = texture(sampler2D(texture5, layer_sampler), uv);}
        else if (samplerIndex < 6.5)  {image_color = texture(sampler2D(texture6, layer_sampler), uv);}
        else if (samplerIndex < 7.5)  {image_color = texture(sampler2D(texture7, layer_sampler), uv);}
        else if (samplerIndex < 8.5)  {image_color = texture(sampler2D(texture8, layer_sampler), uv);}
        else if (samplerIndex < 9.5)  {image_color = texture(sampler2D(texture9, layer_sampler), uv);}
        else if (samplerIndex < 10.5) {image_color = texture(sampler2D(texture10, layer_sampler), uv);}
        else if (samplerIndex < 11.5) {image_color = texture(sampler2D(texture11, layer_sampler), uv);}

        vec4 composite_color = source_over(image_color,color);


        vec4 color_with_border = composite_color;
        if (b+borderWidthOrShadowRadius > -1.5) {
            vec4 border = borderColor * (clamp(b+borderWidthOrShadowRadius+1.5,0.0,1.0));
            color_with_border = source_over(border, composite_color);
        }

        // multiplying by clamp instead of discarding works around an AMD graphics driver bug caused by
        // using "discard" or "return", it seems any early exit from the shader triggers the bug
        // the bug requires referencing more than one sampler in the shader and at least one of the samplers is mipmapped
        float alpha = (1.-clamp(b+1.5,0.,1.));
        FragColor = color_with_border * alpha;
}
@end

@program layer layer_vs layer_fs