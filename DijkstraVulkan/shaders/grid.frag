#version 450

layout(set = 0, binding = 0, std430) readonly buffer CellWeights {
    uint values[];
} cells;

layout(set = 0, binding = 1, std430) readonly buffer VisitedNodes {
    uint values[];
} visited;

layout(set = 0, binding = 2, std430) readonly buffer DijkstraControl {
    uint minDistance;
    uint selected;
    uint finished;
    uint found;
    uint iterations;
    uint source;
    uint target;
    uint width;
    uint height;
} controlData;

uint cellWeight(uint index) {
    uint word = cells.values[index >> 2u];
    return (word >> ((index & 3u) * 8u)) & 0xffu;
}

uint visitState(uint index) {
    uint word = visited.values[index >> 4u];
    return (word >> ((index & 15u) * 2u)) & 3u;
}

layout(location = 0) in vec2 fragUv;
layout(location = 0) out vec4 outColor;

void main() {
    vec2 scaled = clamp(fragUv, vec2(0.0), vec2(0.999999)) * vec2(controlData.width, controlData.height);
    uvec2 cell = uvec2(scaled);
    uint index = cell.y * controlData.width + cell.x;
    uint weight = cellWeight(index);
    uint nodeVisitState = visitState(index);

    vec3 color;
    if (weight == 0u) {
        color = vec3(0.035, 0.045, 0.060);
    } else if (weight == 2u) {
        color = vec3(0.12, 0.23, 0.34);
    } else if (weight == 5u) {
        color = vec3(0.25, 0.13, 0.34);
    } else {
        color = vec3(0.15, 0.17, 0.20);
    }

    if (nodeVisitState == 1u && weight != 0u) {
        color = mix(color, vec3(0.04, 0.72, 0.86), 0.62);
    }
    if (index == controlData.selected && controlData.finished == 0u) {
        color = vec3(0.88, 0.96, 1.0);
    }
    if (nodeVisitState == 2u) {
        color = vec3(1.0, 0.72, 0.16);
    }
    if (index == controlData.source) {
        color = vec3(0.18, 0.92, 0.48);
    } else if (index == controlData.target) {
        color = vec3(0.98, 0.25, 0.65);
    }

    if (max(controlData.width, controlData.height) <= 128u) {
        vec2 local = fract(scaled);
        float line = step(local.x, 0.035) + step(local.y, 0.035);
        color = mix(color, vec3(0.025, 0.030, 0.040), clamp(line, 0.0, 1.0));
    }
    outColor = vec4(color, 1.0);
}
