#version 450

// Volumetric Fog with Light Scattering
// Ray marches through fog volume with Mie/Rayleigh scattering

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D depthBuffer;
layout(binding = 1) uniform sampler3D noiseTexture;  // 3D noise for fog variation

layout(push_constant) uniform FogParams {
    vec3 cameraPos;
    float fogDensity;
    vec3 lightDir;
    float scatteringCoeff;
    vec3 fogColor;
    float heightFalloff;
    mat4 invViewProj;
    vec2 screenSize;
} fog;

const int STEPS = 32;

// Mie scattering phase function
float miePhase(float g, float cosTheta) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * 3.14159 * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

// Reconstruct world position
vec3 worldPosFromDepth(vec2 uv, float depth) {
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldSpace = fog.invViewProj * clipSpace;
    return worldSpace.xyz / worldSpace.w;
}

void main() {
    float depth = texture(depthBuffer, fragUV).r;
    vec3 worldPos = worldPosFromDepth(fragUV, depth);
    
    vec3 rayDir = normalize(worldPos - fog.cameraPos);
    float rayLength = length(worldPos - fog.cameraPos);
    
    // Ray march through fog
    float stepSize = rayLength / float(STEPS);
    vec3 currentPos = fog.cameraPos;
    vec3 stepVector = rayDir * stepSize;
    
    float fogAccum = 0.0;
    vec3 scatteredLight = vec3(0.0);
    
    for(int i = 0; i < STEPS; i++) {
        currentPos += stepVector;
        
        // Sample 3D noise for fog variation
        vec3 noiseCoord = currentPos * 0.01;
        float noise = texture(noiseTexture, noiseCoord).r;
        
        // Height-based density falloff
        float heightFactor = exp(-max(0.0, currentPos.y * fog.heightFalloff));
        float density = fog.fogDensity * heightFactor * (0.5 + 0.5 * noise);
        
        // Light scattering
        float cosTheta = dot(rayDir, -fog.lightDir);
        float phase = miePhase(0.76, cosTheta);  // g = 0.76 for forward scattering
        
        // Accumulate fog
        float stepFog = density * stepSize;
        fogAccum += stepFog * exp(-fogAccum);
        scatteredLight += fog.fogColor * phase * stepFog * fog.scatteringCoeff;
    }
    
    // Final fog factor (1 = full fog, 0 = no fog)
    float fogFactor = 1.0 - exp(-fogAccum);
    
    outColor = vec4(scatteredLight, fogFactor);
}
