#version 450

layout(push_constant) uniform UiPushConstants {
    vec4 color;
} pushConstants;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = pushConstants.color;
}
