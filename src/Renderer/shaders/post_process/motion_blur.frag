#version 450 core

// Motion Blur Fragment Shader
// Implements per-pixel motion blur using velocity vectors

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uColorTexture;
uniform sampler2D uVelocityTexture;
uniform float uMotionScale;
uniform int uMotionSamples;
uniform bool uMotionDebug;

// Sample along motion vector
vec3 sampleMotion(sampler2D tex, vec2 uv, vec2 velocity, int samples) {
    vec3 color = vec3(0.0);
    float totalWeight = 0.0;
    
    for (int i = 0; i <= samples; i++) {
        float t = float(i) / float(samples);
        vec2 sampleUV = uv + velocity * t;
        
        // Check bounds
        if (sampleUV.x >= 0.0 && sampleUV.x <= 1.0 && 
            sampleUV.y >= 0.0 && sampleUV.y <= 1.0) {
            color += texture(tex, sampleUV).rgb;
            totalWeight += 1.0;
        }
    }
    
    return totalWeight > 0.0 ? color / totalWeight : texture(tex, uv).rgb;
}

// Catmull-Rom interpolation for better quality
vec3 catmullRom(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec2 samplePos = uv * texelSize - 0.5;
    vec2 texel = floor(samplePos);
    vec2 frac = samplePos - texel;
    
    vec3 a = texture(tex, (texel + vec2(-1.0, -1.0)) * texelSize).rgb;
    vec3 b = texture(tex, (texel + vec2( 0.0, -1.0)) * texelSize).rgb;
    vec3 c = texture(tex, (texel + vec2( 1.0, -1.0)) * texelSize).rgb;
    vec3 d = texture(tex, (texel + vec2( 2.0, -1.0)) * texelSize).rgb;
    
    vec3 e = texture(tex, (texel + vec2(-1.0,  0.0)) * texelSize).rgb;
    vec3 f = texture(tex, (texel + vec2( 0.0,  0.0)) * texelSize).rgb;
    vec3 g = texture(tex, (texel + vec2( 1.0,  0.0)) * texelSize).rgb;
    vec3 h = texture(tex, (texel + vec2( 2.0,  0.0)) * texelSize).rgb;
    
    vec3 i = texture(tex, (texel + vec2(-1.0,  1.0)) * texelSize).rgb;
    vec3 j = texture(tex, (texel + vec2( 0.0,  1.0)) * texelSize).rgb;
    vec3 k = texture(tex, (texel + vec2( 1.0,  1.0)) * texelSize).rgb;
    vec3 l = texture(tex, (texel + vec2( 2.0,  1.0)) * texelSize).rgb;
    
    vec3 m = texture(tex, (texel + vec2(-1.0,  2.0)) * texelSize).rgb;
    vec3 n = texture(tex, (texel + vec2( 0.0,  2.0)) * texelSize).rgb;
    vec3 o = texture(tex, (texel + vec2( 1.0,  2.0)) * texelSize).rgb;
    vec3 p = texture(tex, (texel + vec2( 2.0,  2.0)) * texelSize).rgb;
    
    vec3 col0 = mix(mix(mix(a, b, frac.x), mix(c, d, frac.x), frac.y);
    vec3 col1 = mix(mix(e, f, frac.x), mix(g, h, frac.x), frac.y);
    vec3 col2 = mix(mix(i, j, frac.x), mix(k, l, frac.x), frac.y);
    vec3 col3 = mix(mix(m, n, frac.x), mix(o, p, frac.x), frac.y);
    
    return mix(mix(col0, col1, frac.y), mix(col2, col3, frac.y), frac.y);
}

void main() {
    vec2 velocity = texture(uVelocityTexture, vTexCoord).rg * uMotionScale;
    float velocityLength = length(velocity);
    
    vec3 color;
    if (velocityLength > 0.001) {
        // Apply motion blur
        color = sampleMotion(uColorTexture, vTexCoord, velocity, uMotionSamples);
    } else {
        // No motion, use original color with high-quality filtering
        vec2 texelSize = 1.0 / textureSize(uColorTexture, 0);
        color = catmullRom(uColorTexture, vTexCoord, texelSize);
    }
    
    // Debug mode: visualize velocity
    if (uMotionDebug) {
        color = mix(color, vec3(velocityLength, 0.0, 1.0 - velocityLength), 0.5);
    }
    
    fragColor = vec4(color, 1.0);
}
