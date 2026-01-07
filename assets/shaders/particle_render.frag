#version 450

// Particle fragment shader - soft circle with color fade

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in float fragLife;

layout(location = 0) out vec4 outColor;

void main() {
    // Soft circle falloff
    vec2 center = fragUV - vec2(0.5);
    float dist = length(center) * 2.0;
    
    // Smooth circle edge
    float alpha = 1.0 - smoothstep(0.8, 1.0, dist);
    
    // Fade out as particle dies
    float lifeFade = clamp(fragLife, 0.0, 1.0);
    
    // Discard fully transparent pixels
    if (alpha * lifeFade < 0.01) {
        discard;
    }
    
    outColor = vec4(fragColor.rgb, fragColor.a * alpha * lifeFade);
}
