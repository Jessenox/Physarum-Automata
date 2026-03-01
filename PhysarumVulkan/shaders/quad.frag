#version 450

layout(set = 0, binding = 0) uniform sampler2D simTexture;
layout(push_constant) uniform QuadPushConstants {
    vec2 uvMin;
    vec2 uvMax;
} pushConstants;

layout(location = 0) in vec2 fragUv;
layout(location = 0) out vec4 outColor;

void main() {
    vec2 uv = mix(pushConstants.uvMin, pushConstants.uvMax, fragUv);
    ivec2 textureSizePx = textureSize(simTexture, 0);
    ivec2 texelCoord = clamp(
        ivec2(uv * vec2(textureSizePx)),
        ivec2(0, 0),
        textureSizePx - ivec2(1, 1));
    outColor = texelFetch(simTexture, texelCoord, 0);
}
