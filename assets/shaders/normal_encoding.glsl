/*
 * =================================================================================================
 *                          NORMAL ENCODING / DECODING UTILITIES
 * =================================================================================================
 *
 * Purpose: Shared GLSL functions for efficient normal encoding and decoding.
 * Supports octahedral and spheremap encoding methods.
 */

// =================================================================================================
// OCTAHEDRAL ENCODING (Recommended for G-Buffer)
// =================================================================================================
// Maps normalized 3D normals to 2D using octahedral projection
// Advantages: Better uniformity, fewer artifacts
// Storage: RG16F (2 channels)

/**
 * Encodes a normalized 3D normal to 2D octahedral coordinates.
 * 
 * @param n Normalized normal vector (length = 1)
 * @return 2D encoded normal in range [0, 1]
 */
vec2 octEncode(vec3 n) {
    // Project the sphere onto the octahedron, then onto the XY plane
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    
    // Fold the octahedron's bottom pyramid with the top
    if (n.z < 0.0) {
        vec2 wrapped = (1.0 - abs(n.yx)) * sign(n.xy);
        n.xy = wrapped;
    }
    
    // Map from [-1, 1] to [0, 1]
    return n.xy * 0.5 + 0.5;
}

/**
 * Decodes a 2D octahedral coordinate back to a normalized 3D normal.
 * 
 * @param enc 2D encoded normal in range [0, 1]
 * @return Normalized 3D normal vector
 */
vec3 octDecode(vec2 enc) {
    // Map from [0, 1] to [-1, 1]
    enc = enc * 2.0 - 1.0;
    
    // Reconstruct Z
    vec3 n = vec3(enc.x, enc.y, 1.0 - abs(enc.x) - abs(enc.y));
    
    // Unfold the octahedron
    if (n.z < 0.0) {
        vec2 wrapped = (1.0 - abs(n.yx)) * sign(n.xy);
        n.xy = wrapped;
    }
    
    return normalize(n);
}

// =================================================================================================
// SPHEREMAP ENCODING (Alternative)
// =================================================================================================
// Maps normals using spherical coordinates
// Storage: RG16F (2 channels)

/**
 * Encodes a normalized 3D normal using spheremap transform.
 * 
 * @param n Normalized normal vector (length = 1)
 * @return 2D encoded normal in range [0, 1]
 */
vec2 sphereEncode(vec3 n) {
    vec2 enc = normalize(n.xy) * sqrt(-n.z * 0.5 + 0.5);
    return enc * 0.5 + 0.5;
}

/**
 * Decodes a spheremap coordinate back to a normalized 3D normal.
 * 
 * @param enc 2D encoded normal in range [0, 1]
 * @return Normalized 3D normal vector
 */
vec3 sphereDecode(vec2 enc) {
    vec4 nn = vec4(enc, 0.0, 0.0) * vec4(2.0, 2.0, 0.0, 0.0) + vec4(-1.0, -1.0, 1.0, -1.0);
    float l = dot(nn.xyz, -nn.xyw);
    nn.z = l;
    nn.xy *= sqrt(l);
    return nn.xyz * 2.0 + vec3(0.0, 0.0, -1.0);
}

// =================================================================================================
// QUANTIZED RGBA8 ENCODING (For extremely memory-constrained scenarios)
// =================================================================================================
// Stores normals in 8-bit per channel format
// Storage: RGBA8 (only RGB used)
// Note: Lower quality than float-based methods

/**
 * Encodes a normalized 3D normal to RGBA8 with octahedral projection.
 * 
 * @param n Normalized normal vector
 * @return vec4 suitable for RGBA8 storage (RGB contains normal, A unused)
 */
vec4 octEncodeRGBA8(vec3 n) {
    vec2 oct = octEncode(n);
    
    // Add dither to reduce banding artifacts in 8-bit quantization
    vec2 dither = vec2(
        fract(sin(dot(oct, vec2(12.9898, 78.233))) * 43758.5453),
        fract(sin(dot(oct.yx, vec2(12.9898, 78.233))) * 43758.5453)
    ) / 255.0;
    
    return vec4(oct + dither, 0.0, 1.0);
}

/**
 * Decodes RGBA8 octahedral normal back to 3D.
 * 
 * @param rgba Encoded normal from RGBA8 texture
 * @return Normalized 3D normal vector
 */
vec3 octDecodeRGBA8(vec4 rgba) {
    return octDecode(rgba.rg);
}
