// LLM maintained.
//------------------------------------------------------------------------------
//  All bindings are within limits, but the number of texture-sampler pairs
//  exceeds the limit by one (expected to fail compilation):
//  - 8 uniform blocks (bindings 0..7)
//  - 32 texture views (bindings 0..31)
//  - 12 samplers (bindings 0..11)
//  - 33 texture-sampler pairs (tex16 is additionally paired with smp7)
//
//  Resources are split across the vertex- and fragment-stage because
//  metal_ios only allows 31 textures per shader stage, and a resource
//  binding can only be used in one stage.
//------------------------------------------------------------------------------
@vs vs
layout(binding=0) uniform ub0 { vec4 ub0_val; };
layout(binding=1) uniform ub1 { vec4 ub1_val; };
layout(binding=2) uniform ub2 { vec4 ub2_val; };
layout(binding=3) uniform ub3 { vec4 ub3_val; };

layout(binding=0) uniform texture2D tex0;
layout(binding=1) uniform texture2D tex1;
layout(binding=2) uniform texture2D tex2;
layout(binding=3) uniform texture2D tex3;
layout(binding=4) uniform texture2D tex4;
layout(binding=5) uniform texture2D tex5;
layout(binding=6) uniform texture2D tex6;
layout(binding=7) uniform texture2D tex7;
layout(binding=8) uniform texture2D tex8;
layout(binding=9) uniform texture2D tex9;
layout(binding=10) uniform texture2D tex10;
layout(binding=11) uniform texture2D tex11;
layout(binding=12) uniform texture2D tex12;
layout(binding=13) uniform texture2D tex13;
layout(binding=14) uniform texture2D tex14;
layout(binding=15) uniform texture2D tex15;

layout(binding=0) uniform sampler smp0;
layout(binding=1) uniform sampler smp1;
layout(binding=2) uniform sampler smp2;
layout(binding=3) uniform sampler smp3;
layout(binding=4) uniform sampler smp4;
layout(binding=5) uniform sampler smp5;

in vec4 position;
in vec2 texcoord0;
out vec4 color;
out vec2 uv;

void main() {
    gl_Position = position + ub0_val + ub1_val + ub2_val + ub3_val;
    vec4 c = vec4(0.0);
    c += textureLod(sampler2D(tex0, smp0), texcoord0, 0.0);
    c += textureLod(sampler2D(tex1, smp1), texcoord0, 0.0);
    c += textureLod(sampler2D(tex2, smp2), texcoord0, 0.0);
    c += textureLod(sampler2D(tex3, smp3), texcoord0, 0.0);
    c += textureLod(sampler2D(tex4, smp4), texcoord0, 0.0);
    c += textureLod(sampler2D(tex5, smp5), texcoord0, 0.0);
    c += textureLod(sampler2D(tex6, smp0), texcoord0, 0.0);
    c += textureLod(sampler2D(tex7, smp1), texcoord0, 0.0);
    c += textureLod(sampler2D(tex8, smp2), texcoord0, 0.0);
    c += textureLod(sampler2D(tex9, smp3), texcoord0, 0.0);
    c += textureLod(sampler2D(tex10, smp4), texcoord0, 0.0);
    c += textureLod(sampler2D(tex11, smp5), texcoord0, 0.0);
    c += textureLod(sampler2D(tex12, smp0), texcoord0, 0.0);
    c += textureLod(sampler2D(tex13, smp1), texcoord0, 0.0);
    c += textureLod(sampler2D(tex14, smp2), texcoord0, 0.0);
    c += textureLod(sampler2D(tex15, smp3), texcoord0, 0.0);
    color = c;
    uv = texcoord0;
}
@end

@fs fs
layout(binding=4) uniform ub4 { vec4 ub4_val; };
layout(binding=5) uniform ub5 { vec4 ub5_val; };
layout(binding=6) uniform ub6 { vec4 ub6_val; };
layout(binding=7) uniform ub7 { vec4 ub7_val; };

layout(binding=16) uniform texture2D tex16;
layout(binding=17) uniform texture2D tex17;
layout(binding=18) uniform texture2D tex18;
layout(binding=19) uniform texture2D tex19;
layout(binding=20) uniform texture2D tex20;
layout(binding=21) uniform texture2D tex21;
layout(binding=22) uniform texture2D tex22;
layout(binding=23) uniform texture2D tex23;
layout(binding=24) uniform texture2D tex24;
layout(binding=25) uniform texture2D tex25;
layout(binding=26) uniform texture2D tex26;
layout(binding=27) uniform texture2D tex27;
layout(binding=28) uniform texture2D tex28;
layout(binding=29) uniform texture2D tex29;
layout(binding=30) uniform texture2D tex30;
layout(binding=31) uniform texture2D tex31;

layout(binding=6) uniform sampler smp6;
layout(binding=7) uniform sampler smp7;
layout(binding=8) uniform sampler smp8;
layout(binding=9) uniform sampler smp9;
layout(binding=10) uniform sampler smp10;
layout(binding=11) uniform sampler smp11;

in vec4 color;
in vec2 uv;
out vec4 frag_color;

void main() {
    vec4 c = color + ub4_val + ub5_val + ub6_val + ub7_val;
    c += texture(sampler2D(tex16, smp6), uv);
    c += texture(sampler2D(tex16, smp7), uv);
    c += texture(sampler2D(tex17, smp7), uv);
    c += texture(sampler2D(tex18, smp8), uv);
    c += texture(sampler2D(tex19, smp9), uv);
    c += texture(sampler2D(tex20, smp10), uv);
    c += texture(sampler2D(tex21, smp11), uv);
    c += texture(sampler2D(tex22, smp6), uv);
    c += texture(sampler2D(tex23, smp7), uv);
    c += texture(sampler2D(tex24, smp8), uv);
    c += texture(sampler2D(tex25, smp9), uv);
    c += texture(sampler2D(tex26, smp10), uv);
    c += texture(sampler2D(tex27, smp11), uv);
    c += texture(sampler2D(tex28, smp6), uv);
    c += texture(sampler2D(tex29, smp7), uv);
    c += texture(sampler2D(tex30, smp8), uv);
    c += texture(sampler2D(tex31, smp9), uv);
    frag_color = c;
}
@end

@program bind_limits vs fs
