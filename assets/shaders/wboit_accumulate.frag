// wboit_accumulate.frag  
// WBOIT accumulation pass for transparent objects
#version 450 core

//===============================================================================
// INPUTS
//===============================================================================

in vec3 v_position;      // World space position
in vec3 v_normal;        // World space normal
in vec2 v_texcoord;      // Texture coordinates
in vec4 v_gl_frag_coord; // For depth value

//===============================================================================  
// UNIFORMS
//===============================================================================

// Camera
uniform vec3 u_camera_position;

// Material textures
uniform sampler2D u_albedo_map;
uniform sampler2D u_opacity_map;  // Alpha channel for transparency

// Material properties
uniform vec4 u_color;        // Base color with alpha
uniform float u_opacity;     // 0.0 = fully transparent, 1.0 = opaque

// WBOIT weight parameters
uniform float u_weight_bias;   // Default: 10.0
uniform float u_weight_scale;  // Default: 3000.0
uniform float u_weight_power;  // Default: 2.0

//===============================================================================
// OUTPUTS - Multiple Render Targets (MRT)
//===============================================================================

layout(location = 0) out vec4 accum_output;   // Weighted color accumulation
layout(location = 1) out float reveal_output; // Revealage (transparency)

//===============================================================================
// WBOIT WEIGHT FUNCTION
//===============================================================================

float calculate_wboit_weight(float z, float alpha) {
    // Weight function biases closer fragmentsmore strongly
    // w = alpha * clamp(10.0 / (1e-5 + depth^2), 0.01, 3000.0)
    
    float depth_weight = u_weight_bias / (1e-5 + pow(z, u_weight_power));
    depth_weight = clamp(depth_weight, 0.01, u_weight_scale);
    
    return alpha * depth_weight;
}

//===============================================================================
// MAIN
//===============================================================================

void main() {
    // Sample material
    vec4 albedo = texture(u_albedo_map, v_texcoord);
    float alpha = albedo.a * u_color.a * u_opacity;
    
    // Early discard for fully transparent pixels
    if (alpha < 0.001) {
        discard;
    }
    
    vec3 color = albedo.rgb * u_color.rgb;
    
    // Simple lighting (just a directional for WBOIT pass)
    vec3 N = normalize(v_normal);
    vec3 L = normalize(vec3(0.5, 1.0, 0.3));
    float NdotL = max(dot(N, L), 0.0);
    color *= (0.3 + 0.7 * NdotL);  // Ambient + diffuse
    
    // Calculate WBOIT weight
    float depth = gl_FragCoord.z;
    float weight = calculate_wboit_weight(depth, alpha);
    
    // MRT Output 0: Weighted color accumulation
    // RGB: premultiplied color weighted by alpha and depth
    // A: alpha weight sum
    accum_output = vec4(color * alpha * weight, alpha * weight);
    
    // MRT Output 1: Revealage (will be multiplied as 1 - alpha)
    // This uses multiplicative blending: dstColor * (1 - srcColor)
    reveal_output = alpha;
}
