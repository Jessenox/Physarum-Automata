#version 450

layout(set = 0, binding = 0, std430) readonly buffer SimulationStates {
    uint values[];
} simulationStates;

layout(set = 0, binding = 1, std430) readonly buffer SimulationPalette {
    uint colors[9];
} simulationPalette;

layout(push_constant) uniform QuadPushConstants {
    vec2 uvMin;
    vec2 uvMax;
    uint gridWidth;
    uint gridHeight;
} pushConstants;

layout(location = 0) in vec2 fragUv;
layout(location = 0) out vec4 outColor;

void main() {
    vec2 uv = mix(pushConstants.uvMin, pushConstants.uvMax, fragUv);
    uint cellX = min(uint(clamp(uv.x, 0.0, 0.999999) * float(pushConstants.gridWidth)), pushConstants.gridWidth - 1u);
    uint cellY = min(uint(clamp(uv.y, 0.0, 0.999999) * float(pushConstants.gridHeight)), pushConstants.gridHeight - 1u);
    uint packedState = simulationStates.values[cellY * pushConstants.gridWidth + cellX];
    uint state = packedState % 9u;
    outColor = unpackUnorm4x8(simulationPalette.colors[state]);
}
