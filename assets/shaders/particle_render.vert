#version 450

// Particle vertex shader - renders instanced point sprites from SSBO data

// Per-instance particle data from SSBO (bound as vertex buffer)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in float inLife;
layout(location = 2) in vec3 inVelocity;
layout(location = 3) in float inSize;
layout(location = 4) in vec3 inAcceleration;
layout(location = 5) in float inRotation;
layout(location = 6) in vec4 inColor;
layout(location = 7) in float inAge;

// Push constants for view/projection
layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec3 cameraRight;
    float _pad0;
    vec3 cameraUp;
    float _pad1;
} pc;

// Output to fragment shader
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out float fragLife;

// Quad vertices for billboard (6 vertices per particle)
const vec2 QUAD_VERTS[6] = vec2[6](
    vec2(-0.5, -0.5),
    vec2( 0.5, -0.5),
    vec2( 0.5,  0.5),
    vec2(-0.5, -0.5),
    vec2( 0.5,  0.5),
    vec2(-0.5,  0.5)
);

const vec2 QUAD_UVS[6] = vec2[6](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

void main() {
    // Skip dead particles
    if (inLife <= 0.0) {
        gl_Position = vec4(0.0);
        return;
    }

    // Get quad vertex for this invocation
    int vertexIndex = gl_VertexIndex % 6;
    vec2 quadPos = QUAD_VERTS[vertexIndex];

    // Apply rotation
    float c = cos(inRotation);
    float s = sin(inRotation);
    vec2 rotatedPos = vec2(
        quadPos.x * c - quadPos.y * s,
        quadPos.x * s + quadPos.y * c
    );

    // Billboard in world space
    vec3 worldPos = inPosition 
        + pc.cameraRight * rotatedPos.x * inSize
        + pc.cameraUp * rotatedPos.y * inSize;

    gl_Position = pc.viewProj * vec4(worldPos, 1.0);

    // Pass data to fragment shader
    fragColor = inColor;
    fragUV = QUAD_UVS[vertexIndex];
    fragLife = inLife;
}
