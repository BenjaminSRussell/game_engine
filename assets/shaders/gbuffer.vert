#version 450
/*
 * G-Buffer Vertex Shader
 * Outputs: World position, Normal, Tangent, UVs, Clip Position (for velocity)
 */

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent; // Assuming TBN is needed for normal mapping
layout(location = 3) in vec2 inUV;

// Instancing or Standard Uniforms
layout(set = 0, binding = 0) uniform GlobalUniforms {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec3 cameraPosition;
    mat4 prevViewProjection; // For velocity calculation
} global;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 prevModel; // For velocity
} push;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec3 outBitangent;
layout(location = 4) out vec2 outUV;
layout(location = 5) out vec4 outClipPos;
layout(location = 6) out vec4 outPrevClipPos;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    outWorldPos = worldPos.xyz;

    // Normal Matrix (approx if uniform scale)
    mat3 normalMatrix = mat3(push.model);
    outNormal = normalize(normalMatrix * inNormal);
    outTangent = normalize(normalMatrix * inTangent);
    // Bitangent is often computed via cross product if storing TBN
    outBitangent = normalize(cross(outNormal, outTangent)); // Check handedness if needed

    outUV = inUV;

    // Current Clip Position
    outClipPos = global.viewProjection * worldPos;
    gl_Position = outClipPos;

    // Previous Clip Position (for Velocity)
    outPrevClipPos = global.prevViewProjection * push.prevModel * vec4(inPosition, 1.0);
}
