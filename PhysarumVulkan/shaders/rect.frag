#version 450

layout(push_constant) uniform RectPushConstants {
    vec4 color;
} pushConstants;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = pushConstants.color;
}
