#version 450 core

// Downsample Fragment Shader
// High-quality downsampling for bloom pyramid generation

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uTexture;
uniform vec2 uTextureSize;

// High-quality 13-tap downsampling
vec3 downsample13(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec3 color = vec3(0.0);
    
    // Sample pattern for high-quality downsampling
    vec2 offsets[13] = vec2[](
        vec2(-1.0, -1.0), vec2(0.0, -1.0), vec2(1.0, -1.0),
        vec2(-1.0,  0.0), vec2(0.0,  0.0), vec2(1.0,  0.0),
        vec2(-1.0,  1.0), vec2(0.0,  1.0), vec2(1.0,  1.0),
        vec2(-2.0, -2.0), vec2(2.0, -2.0),
        vec2(-2.0,  2.0), vec2(2.0,  2.0)
    );
    
    float weights[13] = float[](
        1.0, 2.0, 1.0,
        2.0, 4.0, 2.0,
        1.0, 2.0, 1.0,
        0.5, 0.5,
        0.5, 0.5
    );
    
    float totalWeight = 0.0;
    for (int i = 0; i < 13; i++) {
        vec2 sampleUV = uv + offsets[i] * texelSize;
        color += texture(tex, sampleUV).rgb * weights[i];
        totalWeight += weights[i];
    }
    
    return color / totalWeight;
}

// Kawase blur for bloom
vec3 kawaseBlur(sampler2D tex, vec2 uv, vec2 texelSize, float offset) {
    vec3 color = vec3(0.0);
    
    color += texture(tex, uv + vec2(-offset, -offset) * texelSize).rgb;
    color += texture(tex, uv + vec2(offset, -offset) * texelSize).rgb;
    color += texture(tex, uv + vec2(-offset, offset) * texelSize).rgb;
    color += texture(tex, uv + vec2(offset, offset) * texelSize).rgb;
    
    return color * 0.25;
}

void main() {
    vec2 texelSize = 1.0 / uTextureSize;
    vec3 color = downsample13(uTexture, vTexCoord, texelSize);
    
    // Alternative: Kawase blur for better bloom
    // vec3 color = kawaseBlur(uTexture, vTexCoord, texelSize, 1.0);
    
    fragColor = vec4(color, 1.0);
}
