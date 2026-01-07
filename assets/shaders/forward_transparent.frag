#version 450 core
/*
 * forward_transparent.frag
 * Transparent forward pass fragment shader.
 * Implements Weighted Blended OIT (McGuire 2013).
 */

// =============================================================================
// INPUTS
// =============================================================================
layout(location = 0) in vec3 f_world_pos;
layout(location = 1) in vec3 f_normal;
layout(location = 2) in vec2 f_uv;
layout(location = 3) in vec4 f_tangent;
layout(location = 4) in vec4 f_color;

layout(push_constant) uniform MaterialPC {
    vec4 base_color;
    float roughness;
    float metallic;
    float emissive;
    // ...
} material;

// =============================================================================
// OUTPUTS
// =============================================================================
// WBOIT Render Targets
layout(location = 0) out vec4 out_accum;  // Accumulation buffer: sum(Ci * w(z, a))
layout(location = 1) out float out_reveal; // Revealage buffer: prod(1 - a)

// =============================================================================
// FUNCTIONS
// =============================================================================

// Weight function from "Weighted Blended Order-Independent Transparency" (McGuire & Bavoil 2013)
// Reference: http://jcgt.org/published/0002/02/09/
float calculate_weight(float z, float alpha) {
    // Equation 10 from the paper, tuned for the specific depth range
    // z is linear view-space depth or gl_FragCoord.z
    // Weights should decrease with depth
    
    // Simple approximation:
    // weight = alpha * max(0.01, 3000.0 * pow(1.0 - gl_FragCoord.z, 3.0));
    
    float a = min(1.0, alpha * 10.0) + 0.01;
    float b = 1.0 - gl_FragCoord.z * 0.99;
    float w = clamp(a * a * a * 1e8 * b * b * b, 1e-2, 3e3);
    
    return w;
}

// =============================================================================
// MAIN
// =============================================================================
void main() {
    // 1. Shading (Simplified PBR)
    vec4 color = material.base_color * f_color;
    
    // 2. Alpha handling
    float alpha = color.a;
    // clamp alpha to avoid errors
    alpha = clamp(alpha, 0.0, 1.0);
    
    // Premultiplied alpha for color accumulation
    vec3 premultiplied_color = color.rgb * alpha;
    
    // 3. Weight Calculation
    float w = calculate_weight(gl_FragCoord.z, alpha);
    
    // 4. Output to Accumulation Buffer
    // Store weighted color and weighted alpha coverage
    out_accum = vec4(premultiplied_color * w, alpha * w);
    
    // 5. Output to Reveal Buffer
    // Store (1 - a) for this fragment
    // Zero alpha means fully revealed (1.0)
    out_reveal = alpha; // Wait, usually it is GL_ZERO, GL_ONE_MINUS_SRC_ALPHA blend...
                        // If blend func is ZERO, ONE_MINUS_SRC_ALPHA:
                        // Dst = Dst * (1 - SrcAlpha)
                        // Initial Dst is 1.0.
                        // So we just output alpha here and let the blend unit handle the product?
                        // NO, shader output is usually just 'alpha' but the blend setup is:
                        // Accum: ONE, ONE
                        // Reveal: ZERO, ONE_MINUS_SRC_ALPHA
                        // So here we output 'alpha' to the reveal target, and the blend equation does:
                        // RevealDest = RevealDest * (1 - alpha)
                        // Correct.
                        
    out_reveal = alpha; 
}
