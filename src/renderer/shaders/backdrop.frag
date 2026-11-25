#version 440

// Settings backdrop, as on the landing page: a near-black grid with scattered squares.
// Squares breathe slowly and now and then one flashes lime. Corners are cut in pixel steps.

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float time;
    float px;
    vec2 size;
};

float hash(vec2 p) {
    vec3 q = fract(vec3(p.xyx) * 0.1031);
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x), mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
}
void main()
{
    vec2 p = qt_TexCoord0 * size;
    vec3 color = vec3(0.082, 0.082, 0.086);

    vec2 cell = floor(p / 16.0);
    float cloud = smoothstep(0.5, 0.82, noise(cell * 0.11 + 7.0));
    float pick = hash(cell + 3.0);
    if (pick < cloud * 0.62) {
        float breath = 0.5 + 0.5 * sin(time * (0.35 + hash(cell + 11.0) * 0.8) + hash(cell + 5.0) * 6.283);
        vec3 tone = hash(cell + 17.0) < 0.72 ? vec3(0.059, 0.059, 0.063) : vec3(0.118, 0.122, 0.129);
        color = mix(color, tone, 0.55 + 0.45 * breath);
    }
    // A rare square flashes lime and fades in steps.
    vec2 big = floor(p / 48.0);
    float beat = time * 0.21 + hash(big + 29.0) * 40.0;
    if (hash(big + floor(beat) * 1.7) > 0.986) {
        float glow = floor((1.0 - fract(beat)) * 6.0) / 6.0;
        color += vec3(0.72, 1.0, 0.35) * 0.09 * glow;
    }
    vec2 line = mod(p, 48.0);
    if (line.x < 1.0 || line.y < 1.0) color = max(color, vec3(0.126, 0.126, 0.130));

    // Window edge and stepped corners.
    vec2 step4 = floor(min(p, size - p) / px);
    float corner = step4.x + step4.y;
    if (step4.x < 1.0 || step4.y < 1.0 || corner < 4.0) color = vec3(0.255, 0.255, 0.271);
    float alpha = corner < 3.0 ? 0.0 : 1.0;
    fragColor = vec4(color, 1.0) * alpha * qt_Opacity;
}
