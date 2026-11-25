#version 440

layout(location = 0) in vec2 offsetFromCenter;
layout(location = 1) in vec2 localPosition;
layout(location = 0) out vec4 color;

layout(std140, binding = 0) uniform FlashUniforms {
    mat4 matrix;
    float opacity;
    float radius;
} flash;

void main()
{
    // Use one shade per block and keep the broad, cool flash's original arc.
    const float cell = 8.0;
    vec2 samplePosition = (floor(localPosition / cell) + 0.5) * cell;
    vec2 offset = offsetFromCenter + (samplePosition - localPosition) / flash.radius;
    float falloff = max(0.0, 1.0 - dot(offset, offset));
    float band = floor(falloff * falloff * falloff * 8.0 + 0.5) / 8.0;
    float alpha = floor(flash.opacity * band * 200.0 + 0.5) / 200.0;
    color = vec4(202.0 / 255.0, 222.0 / 255.0, 1.0, 1.0) * alpha;
}
