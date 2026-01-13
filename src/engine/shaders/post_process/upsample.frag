#version 450 core

// Upsample Fragment Shader
// High-quality upsampling for bloom pyramid reconstruction

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uTexture;
uniform sampler2D uBaseTexture; // Lower resolution texture to blend with
uniform vec2 uTextureSize;
uniform float uBlendFactor;

// Bilinear upsampling with bilinear filtering
vec3 upsampleBilinear(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec2 halfTexel = texelSize * 0.5;
    vec2 uvScaled = uv * (1.0 + 2.0 * halfTexel) - halfTexel;
    
    return texture(tex, uvScaled).rgb;
}

// High-quality 4-tap upsampling
vec3 upsample4Tap(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec2 halfTexel = texelSize * 0.5;
    
    vec3 color = texture(tex, uv - halfTexel).rgb;
    color += texture(tex, uv + vec2(halfTexel.x, -halfTexel.y)).rgb;
    color += texture(tex, uv + vec2(-halfTexel.x, halfTexel.y)).rgb;
    color += texture(tex, uv + halfTexel).rgb;
    
    return color * 0.25;
}

// Catmull-Rom upsampling for higher quality
vec3 upsampleCatmullRom(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec2 samplePos = uv * textureSize(tex, 0) - 0.5;
    vec2 texel = floor(samplePos);
    vec2 frac = samplePos - texel;
    
    vec3 a = texture(tex, (texel + vec2(-1.0, -1.0)) / textureSize(tex, 0)).rgb;
    vec3 b = texture(tex, (texel + vec2( 0.0, -1.0)) / textureSize(tex, 0)).rgb;
    vec3 c = texture(tex, (texel + vec2( 1.0, -1.0)) / textureSize(tex, 0)).rgb;
    vec3 d = texture(tex, (texel + vec2( 2.0, -1.0)) / textureSize(tex, 0)).rgb;
    
    vec3 e = texture(tex, (texel + vec2(-1.0,  0.0)) / textureSize(tex, 0)).rgb;
    vec3 f = texture(tex, (texel + vec2( 0.0,  0.0)) / textureSize(tex, 0)).rgb;
    vec3 g = texture(tex, (texel + vec2( 1.0,  0.0)) / textureSize(tex, 0)).rgb;
    vec3 h = texture(tex, (texel + vec2( 2.0,  0.0)) / textureSize(tex, 0)).rgb;
    
    vec3 i = texture(tex, (texel + vec2(-1.0,  1.0)) / textureSize(tex, 0)).rgb;
    vec3 j = texture(tex, (texel + vec2( 0.0,  1.0)) / textureSize(tex, 0)).rgb;
    vec3 k = texture(tex, (texel + vec2( 1.0,  1.0)) / textureSize(tex, 0)).rgb;
    vec3 l = texture(tex, (texel + vec2( 2.0,  1.0)) / textureSize(tex, 0)).rgb;
    
    vec3 m = texture(tex, (texel + vec2(-1.0,  2.0)) / textureSize(tex, 0)).rgb;
    vec3 n = texture(tex, (texel + vec2( 0.0,  2.0)) / textureSize(tex, 0)).rgb;
    vec3 o = texture(tex, (texel + vec2( 1.0,  2.0)) / textureSize(tex, 0)).rgb;
    vec3 p = texture(tex, (texel + vec2( 2.0,  2.0)) / textureSize(tex, 0)).rgb;
    
    vec3 col0 = mix(mix(mix(a, b, frac.x), mix(c, d, frac.x), frac.y);
    vec3 col1 = mix(mix(e, f, frac.x), mix(g, h, frac.x), frac.y);
    vec3 col2 = mix(mix(i, j, frac.x), mix(k, l, frac.x), frac.y);
    vec3 col3 = mix(mix(m, n, frac.x), mix(o, p, frac.x), frac.y);
    
    return mix(mix(col0, col1, frac.y), mix(col2, col3, frac.y), frac.y);
}

void main() {
    vec2 texelSize = 1.0 / uTextureSize;
    
    // Upsample the current texture
    vec3 upsampledColor = upsample4Tap(uTexture, vTexCoord, texelSize);
    
    // Sample the base texture (lower resolution)
    vec3 baseColor = texture(uBaseTexture, vTexCoord).rgb;
    
    // Blend between upsampled and base texture
    vec3 finalColor = mix(baseColor, upsampledColor, uBlendFactor);
    
    fragColor = vec4(finalColor, 1.0);
}
