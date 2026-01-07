#version 450

// Input from vertex shader
layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in float fragAO;
layout(location = 3) in float fragLight;
layout(location = 4) in flat uint fragTextureID;
layout(location = 5) in float fragWavePhase;
layout(location = 6) in vec3 fragWorldPos;

// Output color
layout(location = 0) out vec4 outColor;

// Texture sampler
layout(binding = 0) uniform sampler2D texSampler;

// Push constants
layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
    uint tilesPerRow;
} push;

void main() {
    // Calculate texture coordinates in atlas
    // Assuming texture atlas with tiles arranged in a grid
    uint tilesPerRow = push.tilesPerRow;
    uint tileX = fragTextureID % tilesPerRow;
    uint tileY = fragTextureID / tilesPerRow;
    
    float tileSize = 1.0 / float(tilesPerRow);
    vec2 atlasUV = vec2(
        (float(tileX) + fragTexCoord.x) * tileSize,
        (float(tileY) + fragTexCoord.y) * tileSize
    );
    
    // Sample texture
    vec4 texColor = texture(texSampler, atlasUV);
    
    // Apply ambient occlusion
    vec3 aoColor = texColor.rgb * fragAO;
    
    // Apply block lighting
    vec3 litColor = aoColor * max(fragLight, 0.1); // Minimum ambient light
    
    // Simple directional lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diffuse = max(dot(normalize(fragNormal), lightDir), 0.0);
    litColor += aoColor * diffuse * 0.3; // Add some directional contribution
    
    // Output final color
    outColor = vec4(litColor, texColor.a);
    
    // Discard fully transparent fragments
    if (outColor.a < 0.01) {
        discard;
    }
}
