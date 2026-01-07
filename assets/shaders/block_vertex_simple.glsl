#version 450

// Vertex attributes matching the Vertex struct
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in uint inAO;
layout(location = 4) in uint inLight;
layout(location = 5) in uint inTextureID;
layout(location = 6) in float inWavePhase;

// Output to fragment shader
layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out float fragAO;
layout(location = 3) out float fragLight;
layout(location = 4) out flat uint fragTextureID;
layout(location = 5) out float fragWavePhase;
layout(location = 6) out vec3 fragWorldPos;

// Push constants for transformation matrices
layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
    uint tilesPerRow;
} push;

void main() {
    // Transform vertex position
    vec4 worldPos = vec4(inPosition, 1.0);
    gl_Position = push.proj * push.view * worldPos;
    
    // Pass through attributes
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
    fragWorldPos = inPosition;
    
    // Map AO from 0-3 to 0.5-1.0 (0 = darkest, 3 = brightest)
    fragAO = 0.5 + (float(inAO) / 3.0) * 0.5;
    
    // Map light from 0-15 to 0.0-1.0
    fragLight = float(inLight) / 15.0;
    
    fragTextureID = inTextureID;
    fragWavePhase = inWavePhase;
}
