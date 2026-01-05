#version 460
#extension GL_EXT_ray_tracing : require

// Shadow miss shader - ray didn't hit anything, so not in shadow
layout(location = 1) rayPayloadInEXT ShadowPayload shadow_payload;

// Data structures
struct ShadowPayload {
    float visibility;
};

// Main shadow miss shader
void main() {
    // Ray reached the sky/light source without hitting any geometry
    // This means the point is fully illuminated (not in shadow)
    shadow_payload.visibility = 1.0;
}
