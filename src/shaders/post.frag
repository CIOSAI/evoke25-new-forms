#version 460

out vec4 C;

layout(binding = 5) uniform sampler2D tex_render;

layout(binding = 0, r32ui) uniform coherent uimage2D tex_r;
layout(binding = 1, r32ui) uniform coherent uimage2D tex_g;
layout(binding = 2, r32ui) uniform coherent uimage2D tex_b;


float luma(vec3 col) {
    return dot(col, vec3(0.2126729, 0.7151522, 0.0721750));
}

#define SPAN_MAX   (8.0)
#define REDUCE_MIN (1.0/128.0)
#define REDUCE_MUL (1.0/32.0)
vec4 FXAA(sampler2D tex, vec2 uv) {
    vec2 size = vec2(textureSize(tex, 0).xy);
    vec2 px = 1.0 / size;
    vec3 col   = texture(tex, uv).rgb;
    vec3 col00 = texture(tex, uv + vec2(-0.5) * px).rgb;
    vec3 col11 = texture(tex, uv + vec2( 0.5) * px).rgb;
    vec3 col10 = texture(tex, uv + vec2(0.5, -0.5) * px).rgb;
    vec3 col01 = texture(tex, uv + vec2(-0.5, 0.5) * px).rgb;

    float lum = luma(col);
    float lum00 = luma(col00);
    float lum11 = luma(col11);
    float lum10 = luma(col10);
    float lum01 = luma(col01);

    vec2 dir = vec2((lum01 + lum11) - (lum00 + lum10),
    (lum00 + lum01) - (lum10 + lum11));

    float dirReduce = max((lum00 + lum10 + lum01 + lum11) * REDUCE_MUL, REDUCE_MIN);

    float rcpDir = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = clamp(dir * rcpDir, -SPAN_MAX, SPAN_MAX) * px;

    vec4 A = 0.5 * (
    texture(tex, uv - dir * (1.0/6.0)) +
    texture(tex, uv + dir * (1.0/6.0)));

    vec4 B = A * 0.5 + 0.25 * (
    texture(tex, uv - dir * 0.5) +
    texture(tex, uv + dir * 0.5));

    float lumMin = min(lum, min(min(lum00, lum10), min(lum01, lum11)));
    float lumMax = max(lum, max(max(lum00, lum10), max(lum01, lum11)));

    float lumB = luma(B.rgb);

    return ((lumB < lumMin) || (lumB > lumMax)) ? A : B;
}

void main() {
    //vec2 u = uvn;
		vec2 u = gl_FragCoord.xy/vec2(textureSize(tex_render, 0).xy);
    

    C=pow(max(FXAA(tex_render, u),0.),vec4(.4545));

		imageStore(tex_r, ivec2(gl_FragCoord.xy), ivec4(0));
		imageStore(tex_g, ivec2(gl_FragCoord.xy), ivec4(1));
		imageStore(tex_b, ivec2(gl_FragCoord.xy), ivec4(2));

    C.w = 1.0;
}
