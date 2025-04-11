#version 140

uniform sampler2D texUnit;
uniform float offset;
uniform vec2 halfpixel;
uniform bool finalRound;
uniform sampler2D alphaMask;
uniform sampler2D original;

in vec2 uv;

out vec4 fragColor;

void main(void)
{
    vec4 sum = texture(texUnit, uv + vec2(-halfpixel.x * 2.0, 0.0) * offset);
    sum += texture(texUnit, uv + vec2(-halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, uv + vec2(0.0, halfpixel.y * 2.0) * offset);
    sum += texture(texUnit, uv + vec2(halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, uv + vec2(halfpixel.x * 2.0, 0.0) * offset);
    sum += texture(texUnit, uv + vec2(halfpixel.x, -halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, uv + vec2(0.0, -halfpixel.y * 2.0) * offset);
    sum += texture(texUnit, uv + vec2(-halfpixel.x, -halfpixel.y) * offset) * 2.0;

    if (finalRound) {
        vec2 uv2 = vec2(uv.x, 1.0 - uv.y);
        float alpha = texture(alphaMask, uv2).r;
//         if (alpha == 0) {
//             fragColor = texture(original, uv);
//             return;
//         }

// //         vec2 a = 2*uv - 1;
// //         float d = dot(a, a) - 1;
// //         float alpha = clamp(0, 1, -10 * d);
// //         fragColor = vec4(sum.rgb / 12.0, 0.1);
// //         fragColor = vec4(sum.rgb / 12.0, texture(alphaMask, uv).r);
        vec4 blurred = sum / 12.0;
//         vec4 blurred = vec4(0, 1, 0, 1);
        fragColor = mix(texture(original, uv), blurred, alpha);
//
// //         fragColor = vec4(sum.rgb, sum.a * alpha);
    } else {
        fragColor = sum / 12.0;
    }
}
