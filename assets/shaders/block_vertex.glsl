#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in int inAO;        // u8 -> int
layout(location = 4) in int inLight;     // u8 -> int
layout(location = 5) in int inTextureID;
layout(location = 6) in float inWavePhase; // Per-vertex base wave phase (water) // u8 -> int

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out float fragAO;
layout(location = 3) out float fragLight;
layout(location = 5) out flat int fragTextureID;
layout(location = 6) out float fragWavePhase;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
    
    // Map AO 0-3 to 0.5-1.0
    // 0 = 0.5 (darkest), 3 = 1.0 (brightest)
    // Formula: 0.5 + (ao / 3.0) * 0.5
    fragAO = 0.5 + (float(inAO) / 3.0) * 0.5;
    
    // Map Light 0-15 to 0.0-1.0
    fragLight = float(inLight) / 15.0;
    
    fragTextureID = inTextureID;
    fragWavePhase = inWavePhase;
}
