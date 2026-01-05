#version 460
#extension GL_EXT_ray_tracing : require

// Miss shader - handles rays that don't hit any geometry (sky/background)
layout(location = 0) rayPayloadInEXT vec3 hitValue;

void main() {
    // Simple sky gradient based on ray direction
    vec3 rayDir = gl_WorldRayDirectionEXT;

    // Normalize to 0-1 range
    float t = 0.5 * (rayDir.y + 1.0);

    // Sky gradient from horizon to zenith
    vec3 skyHorizon = vec3(0.7, 0.8, 1.0);  // Light blue
    vec3 skyZenith = vec3(0.3, 0.5, 0.9);   // Darker blue

    hitValue = mix(skyHorizon, skyZenith, t);
}
