
varying vec2 uv;

uniform sampler2D texUnit;
uniform float offset;
uniform vec2 halfpixel;
uniform bool finalRound;
uniform sampler2D alphaMask;
uniform sampler2D original;

vec4 sum()
{
    vec4 sum = texture2D(texUnit, uv + vec2(-halfpixel.x * 2.0, 0.0) * offset);
    sum += texture2D(texUnit, uv + vec2(-halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture2D(texUnit, uv + vec2(0.0, halfpixel.y * 2.0) * offset);
    sum += texture2D(texUnit, uv + vec2(halfpixel.x, halfpixel.y) * offset) * 2.0;
    sum += texture2D(texUnit, uv + vec2(halfpixel.x * 2.0, 0.0) * offset);
    sum += texture2D(texUnit, uv + vec2(halfpixel.x, -halfpixel.y) * offset) * 2.0;
    sum += texture2D(texUnit, uv + vec2(0.0, -halfpixel.y * 2.0) * offset);
    sum += texture2D(texUnit, uv + vec2(-halfpixel.x, -halfpixel.y) * offset) * 2.0;
    return sum / 12.0;
}

void main(void)
{
    if (finalRound) {
        vec2 uv2 = vec2(uv.x, 1.0 - uv.y);
        float alpha = texture2D(alphaMask, uv2).a;
        if (alpha == 0.) {
            discard;
        }
        gl_FragColor = mix(texture2D(original, uv), sum(), alpha);
    } else {
        gl_FragColor = sum();
    }
}
