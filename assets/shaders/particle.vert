// assets/shaders/particle.vert
// Particle vertex shader for billboard rendering
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;
layout(location = 3) in float inSize;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec3 cameraRight;
    vec3 cameraUp;
} pc;

layout(location = 0) out VS_OUT {
    vec2 uv;
    vec4 color;
    float depth;
} vs_out;

void main() {
    // Billboard quad expansion
    // Each particle is 2 triangles (6 vertices)
    // Vertex index within the quad (0-5)
    int vertexInQuad = gl_VertexIndex % 6;

    // Generate billboard corners
    vec3 offsetDir = vec3(0.0);
    vec2 quadUV = vec2(0.0);

    // Create 2D billboard offset based on vertex position
    float halfSize = inSize * 0.5;

    if (vertexInQuad == 0) {
        offsetDir = -pc.cameraRight * halfSize - pc.cameraUp * halfSize;
        quadUV = vec2(0.0, 1.0);
    } else if (vertexInQuad == 1) {
        offsetDir = pc.cameraRight * halfSize - pc.cameraUp * halfSize;
        quadUV = vec2(1.0, 1.0);
    } else if (vertexInQuad == 2) {
        offsetDir = pc.cameraRight * halfSize + pc.cameraUp * halfSize;
        quadUV = vec2(1.0, 0.0);
    } else if (vertexInQuad == 3) {
        offsetDir = -pc.cameraRight * halfSize - pc.cameraUp * halfSize;
        quadUV = vec2(0.0, 1.0);
    } else if (vertexInQuad == 4) {
        offsetDir = pc.cameraRight * halfSize + pc.cameraUp * halfSize;
        quadUV = vec2(1.0, 0.0);
    } else {
        offsetDir = -pc.cameraRight * halfSize + pc.cameraUp * halfSize;
        quadUV = vec2(0.0, 0.0);
    }

    // Billboard position in world space
    vec3 worldPos = inPosition + offsetDir;

    // Transform to clip space
    vec4 clipPos = pc.viewProj * vec4(worldPos, 1.0);
    gl_Position = clipPos;

    // Pass data to fragment shader
    vs_out.uv = inUV * quadUV;  // Combine particle UV with quad UV
    vs_out.color = inColor;
    vs_out.depth = clipPos.z / clipPos.w;
}
