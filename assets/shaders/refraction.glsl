// refraction.glsl
// Refraction helper functions for shaders

// Calculates screen-space refraction UV offset
// N: View-space normal
// V: View vector (normalized, pointing to camera)
// ior: Index of Refraction
// thickness: object thickness in view space
vec2 calculate_refraction_offset(vec3 N, vec3 V, float ior, float thickness) {
    // Simple approximation: Offset based on normal x,y view space components
    // A more accurate ray-marched approach could be used for advanced effects
    
    // Scale factor based on IOR difference from air (1.0)
    float scale = (1.0 - 1.0/ior);
    
    // Offset UVs based on normal xy
    return N.xy * scale * thickness;
}

// Samples scene color with chromatic aberration
// tex: Scene color texture
// uv: Base texture coordinate
// offset: Refraction offset (calculated above)
// aberration: Strength of chromatic aberration
vec3 sample_refraction_chromatic(sampler2D tex, vec2 uv, vec2 offset, float aberration) {
    // Dispersion: Red bends less, Blue bends more
    vec2 uv_r = uv + offset * (1.0 - aberration);
    vec2 uv_g = uv + offset;
    vec2 uv_b = uv + offset * (1.0 + aberration);
    
    float r = texture(tex, uv_r).r;
    float g = texture(tex, uv_g).g;
    float b = texture(tex, uv_b).b;
    
    return vec3(r, g, b);
}
