#version 440

// Pixel curtain: blocks of one colour vanish in a scattered order while `open` goes from 0 to 1.

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float open;
    float cell;
    vec2 size;
    vec4 color;
    vec4 spark;
};

float hash(vec2 p) {
    vec3 q = fract(vec3(p.xyx) * 0.1031);
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}
void main()
{
    vec2 block = floor(qt_TexCoord0 * size / cell);
    float rise = 1.0 - (block.y + 0.5) * cell / size.y;
    float order = hash(block) * 0.55 + rise * 0.45;
    float left = order - (open * 1.08 - 0.04);
    float alpha = left > 0.0 ? 1.0 : 0.0;
    vec4 tone = left < 0.035 ? spark : color;
    fragColor = vec4(tone.rgb, 1.0) * alpha * qt_Opacity;
}
