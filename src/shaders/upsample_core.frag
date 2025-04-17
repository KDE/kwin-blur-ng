#version 140

in vec2 uv;
out vec4 fragColor;

uniform sampler2D texUnit;
uniform float offset;
uniform vec2 halfpixel;
uniform bool finalRound;
uniform sampler2D alphaMask;
uniform sampler2D original;

vec4 sum()
{
    vec4 sum = texture(texUnit, uv + vec2(-halfpixel.x * 2.0, 0.0) * offset);
    sum += texture(texUnit, uv + vec2(-halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, uv + vec2(0.0, halfpixel.y * 2.0) * offset);
    sum += texture(texUnit, uv + vec2(halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, uv + vec2(halfpixel.x * 2.0, 0.0) * offset);
    sum += texture(texUnit, uv + vec2(halfpixel.x, -halfpixel.y) * offset) * 2.0;
    sum += texture(texUnit, uv + vec2(0.0, -halfpixel.y * 2.0) * offset);
    sum += texture(texUnit, uv + vec2(-halfpixel.x, -halfpixel.y) * offset) * 2.0;
    return sum / 12.0;
}

void main(void)
{
    if (finalRound) {
        vec2 uv2 = vec2(uv.x, 1.0 - uv.y);
        float alpha = texture(alphaMask, uv2).r;
        if (alpha == 0.) {
            fragColor = texture(original, uv);
            return;
        }
        fragColor = mix(texture(original, uv), sum(), alpha);
    } else {
        fragColor = sum();
    }
}
