#version 450 core

// Lens Flare and Optical Effects Fragment Shader
// Implements realistic lens flare, god rays, and optical artifacts

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D uColorTexture;
uniform sampler2D uLensFlareTexture;
uniform sampler2D uDirtTexture;
uniform vec3 uLightPosition; // Screen-space position of bright light
uniform float uLensFlareIntensity;
uniform float uGhostCount;
uniform float uGhostDispersal;
uniform float uHaloWidth;
uniform float uHaloIntensity;
uniform float uChromaticDistortion;
uniform bool uAnamorphicLens;
uniform float uAnamorphicSqueeze;

// Calculate screen-space distance from light
float distanceFromLight(vec2 uv, vec2 lightPos) {
    return distance(uv, lightPos);
}

// Generate ghost images
vec3 generateGhosts(vec2 uv, vec2 lightPos, sampler2D flareTex, sampler2D dirtTex) {
    vec3 ghosts = vec3(0.0);
    vec2 lightToUV = uv - lightPos;
    
    for (int i = 0; i < 8; i++) {
        if (float(i) >= uGhostCount) break;
        
        // Calculate ghost position
        float offset = float(i) * uGhostDispersal;
        vec2 ghostPos = lightPos + lightToUV * offset;
        
        // Check if ghost is on screen
        if (ghostPos.x >= 0.0 && ghostPos.x <= 1.0 &&
            ghostPos.y >= 0.0 && ghostPos.y <= 1.0) {
            
            // Sample flare texture
            vec3 flareColor = texture(flareTex, ghostPos).rgb;
            
            // Apply dirt texture
            vec3 dirtColor = texture(dirtTex, ghostPos).rgb;
            
            // Calculate ghost intensity with falloff
            float ghostIntensity = 1.0 / (offset * offset + 1.0);
            
            // Apply chromatic aberration to ghosts
            if (uChromaticDistortion > 0.0) {
                float chromaticOffset = uChromaticDistortion * offset * 0.01;
                flareColor.r = texture(flareTex, ghostPos + vec2(chromaticOffset, 0.0)).r;
                flareColor.b = texture(flareTex, ghostPos - vec2(chromaticOffset, 0.0)).b;
            }
            
            ghosts += flareColor * dirtColor * ghostIntensity;
        }
    }
    
    return ghosts;
}

// Generate halo effect
vec3 generateHalo(vec2 uv, vec2 lightPos, sampler2D flareTex) {
    float dist = distanceFromLight(uv, lightPos);
    float halo = 1.0 - smoothstep(0.0, uHaloWidth, dist);
    
    vec2 haloUV = lightPos + (uv - lightPos) * 0.8;
    vec3 haloColor = texture(flareTex, haloUV).rgb;
    
    return haloColor * halo * uHaloIntensity;
}

// Generate star burst effect
vec3 generateStarBurst(vec2 uv, vec2 lightPos) {
    vec2 lightToUV = uv - lightPos;
    float dist = length(lightToUV);
    
    if (dist < 0.001) return vec3(0.0);
    
    vec2 direction = normalize(lightToUV);
    
    // Create star pattern
    float star = 0.0;
    int numSpikes = 8;
    
    for (int i = 0; i < numSpikes; i++) {
        float angle = float(i) * 2.0 * 3.14159 / float(numSpikes);
        vec2 spikeDir = vec2(cos(angle), sin(angle));
        
        float spike = max(0.0, dot(direction, spikeDir));
        spike = pow(spike, 32.0);
        
        // Fade with distance
        spike *= 1.0 / (dist * dist + 1.0);
        
        star += spike;
    }
    
    return vec3(star) * 2.0;
}

// Apply anamorphic lens effect
vec3 applyAnamorphicLens(vec3 color, vec2 uv, vec2 lightPos) {
    if (!uAnamorphicLens) return color;
    
    vec2 lightToUV = uv - lightPos;
    float dist = abs(lightToUV.y);
    
    // Create horizontal streak
    float streak = exp(-dist * dist * 10.0);
    streak *= exp(-abs(lightToUV.x) * 2.0);
    
    // Apply squeeze effect
    vec2 streakUV = uv;
    streakUV.x = lightPos.x + (uv.x - lightPos.x) * uAnamorphicSqueeze;
    
    return color + vec3(streak * 0.5);
}

// Generate optical vignetting
float calculateOpticalVignetting(vec2 uv, vec2 lightPos) {
    vec2 center = vec2(0.5);
    vec2 fromCenter = uv - center;
    float dist = length(fromCenter);
    
    // Optical vignetting has a different falloff than regular vignetting
    float vignetting = 1.0 - smoothstep(0.3, 0.8, dist);
    vignetting = pow(vignetting, 2.0);
    
    return vignetting;
}

void main() {
    vec3 color = texture(uColorTexture, vTexCoord).rgb;
    
    // Convert light position to screen space if needed
    vec2 lightPos = uLightPosition.xy;
    
    // Calculate distance from light
    float dist = distanceFromLight(vTexCoord, lightPos);
    
    // Only apply lens effects near bright lights
    if (dist < 0.5) {
        // Generate ghosts
        vec3 ghosts = generateGhosts(vTexCoord, lightPos, uLensFlareTexture, uDirtTexture);
        
        // Generate halo
        vec3 halo = generateHalo(vTexCoord, lightPos, uLensFlareTexture);
        
        // Generate star burst
        vec3 starBurst = generateStarBurst(vTexCoord, lightPos);
        
        // Apply anamorphic lens effect
        vec3 anamorphic = applyAnamorphicLens(color, vTexCoord, lightPos);
        
        // Combine all effects
        vec3 lensEffects = ghosts + halo + starBurst;
        lensEffects *= uLensFlareIntensity;
        
        // Apply optical vignetting
        float opticalVignetting = calculateOpticalVignetting(vTexCoord, lightPos);
        lensEffects *= opticalVignetting;
        
        // Add to original color
        color += lensEffects;
    }
    
    fragColor = vec4(color, 1.0);
}
