#version 450 core

// Velocity Buffer Fragment Shader
// Generates motion vectors for temporal effects

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uCurrentDepthTexture;
uniform sampler2D uPreviousDepthTexture;
uniform mat4 uInverseViewProjection;
uniform mat4 uPreviousViewProjection;
uniform mat4 uCurrentViewProjection;
uniform vec2 uJitterOffset;

vec3 reconstructWorldPos(sampler2D depthTex, vec2 uv, mat4 invVP) {
    float depth = texture(depthTex, uv).r;
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 worldPos = invVP * clipPos;
    return worldPos.xyz / worldPos.w;
}

void main() {
    vec3 currentWorldPos = reconstructWorldPos(uCurrentDepthTexture, vTexCoord, uInverseViewProjection);
    vec3 previousWorldPos = reconstructWorldPos(uPreviousDepthTexture, vTexCoord, uInverseViewProjection);
    
    vec4 currentClip = uCurrentViewProjection * vec4(currentWorldPos, 1.0);
    vec4 previousClip = uPreviousViewProjection * vec4(previousWorldPos, 1.0);
    
    currentClip /= currentClip.w;
    previousClip /= previousClip.w;
    
    vec2 velocity = (currentClip.xy - previousClip.xy) * 0.5;
    velocity += uJitterOffset;
    
    fragColor = vec4(velocity, 0.0, 1.0);
}
