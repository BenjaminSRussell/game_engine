#version 450 core
/*
 * wboit_composite.frag
 * WBOIT Composite pass fragment shader.
 * Blends the accumulation and reveal buffers onto the backbuffer.
 */

// =============================================================================
// INPUTS
// =============================================================================
layout(location = 0) in vec2 f_uv;

// =============================================================================
// UNIFORMS
// =============================================================================
layout(set = 0, binding = 0) uniform sampler2D t_accum;
layout(set = 0, binding = 1) uniform sampler2D t_reveal;

// =============================================================================
// OUTPUTS
// =============================================================================
layout(location = 0) out vec4 out_color;

// =============================================================================
// MAIN
// =============================================================================
void main() {
    // 1. Fetch from Reveal Buffer
    // Reveal stores product(1 - a).
    // So 'revealage' is the fraction of background visible.
    // 1.0 = fully visible (0 opacity in front)
    // 0.0 = fully occluded (1 opacity in front)
    float reveal = texture(t_reveal, f_uv).r;
    
    // Optimization: Skip fully transparent pixels
    if (abs(reveal - 1.0) < 1e-5) {
        discard;
    }
    
    // 2. Fetch from Accumulation Buffer
    // Stores sum(color * w) in rgb, sum(alpha * w) in a
    vec4 accum = texture(t_accum, f_uv);
    
    // Avoid division by zero
    float range = max(accum.a, 1e-5);
    
    // 3. Calculate Average Color
    vec3 average_color = accum.rgb / range;
    
    // 4. Blend
    // dst = average_color * (1 - reveal) + bg * reveal
    // The blending equation usually handles the 'bg * reveal' part (SrcAlpha, OneMinusSrcAlpha) 
    // or (One, OneMinusSrcAlpha) depending on setup.
    // Standard WBOIT Composite Blend:
    // SrcFactor = ONE_MINUS_SRC_ALPHA (using output alpha as '1-reveal')
    // DstFactor = SRC_ALPHA (this doesn't sound right for (1-reveal))
    
    // McGuire 2013 Composite Step:
    // GlBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    // Output Color = average_color
    // Output Alpha = 1 - reveal
    
    // Result = average_color * (1 - reveal) + Background * reveal
    // This matches standard alpha blending if OutputAlpha is (1-reveal).
    
    out_color = vec4(average_color, 1.0 - reveal);
}
