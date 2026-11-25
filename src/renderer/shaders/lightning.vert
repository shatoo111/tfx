#version 440

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texCoord;
layout(location = 0) out vec2 offsetFromCenter;
layout(location = 1) out vec2 localPosition;

layout(std140, binding = 0) uniform FlashUniforms {
    mat4 matrix;
    float opacity;
    float radius;
} flash;

void main()
{
    offsetFromCenter = texCoord * 2.0 - 1.0;
    localPosition = position.xy;
    gl_Position = flash.matrix * position;
}
