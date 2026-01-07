#version 460
#extension GL_EXT_ray_tracing : require

// Shadow miss shader - ray didn't hit anything, so not in shadow
layout(location = 1) rayPayloadInEXT bool shadowed;

void main() {
    shadowed = false;
}
