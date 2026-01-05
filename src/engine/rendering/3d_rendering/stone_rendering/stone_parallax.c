#include "../3d_rendering.h"
#include <math.h>

/* ==================== Parallax Occlusion Mapping ==================== */

// Placeholder texture sampling
static float sample_height(ResourceHandle height_map, Vec2 uv) {
    // In real implementation: access texture data
    return 1.0f; // Dummy return
}

typedef struct {
    float height_scale;
    int min_steps;
    int max_steps;
    float reference_plane; // 0.0 to 1.0, typically 0.0 (surface) or 0.5
} pom_params_t;

static float mix(float a, float b, float t) {
    return a * (1.0f - t) + b * t;
}

static float absf(float x) {
    return x < 0.0f ? -x : x;
}

// Returns the UV offset to be added to original UVs
Vec2 parallax_occlusion_mapping(ResourceHandle height_map, Vec2 uv, Vec3 view_dir_ts, pom_params_t* params) {
    // view_dir_ts: View direction in Tangent Space (Must be normalized)
    
    // 1. Calculate number of layers
    // More layers at shallow angles. view_dir_ts.z corresponds to dot(N, V) since N=(0,0,1) in TS
    float min_layers = (float)params->min_steps;
    float max_layers = (float)params->max_steps;
    
    // simple linear interpolation based on angle
    // If looking straight down (z=1), use min samples. If shallow (z=0), use max.
    float num_layers = mix(max_layers, min_layers, absf(view_dir_ts.z));
    
    float layer_depth = 1.0f / num_layers;
    float current_layer_depth = 0.0f;
    
    // 2. Calculate UV delta
    // P = V * height_scale
    // But we strictly march in UV space which is 2D. 
    // Shift magnitude is related to view vector projection onto surface.
    // delta_coord = (View.xy / View.z) * height_scale
    
    // Protective check for z ~ 0
    float view_z = view_dir_ts.z;
    if (view_z < 0.001f) view_z = 0.001f;
    
    Vec2 P;
    P.x = view_dir_ts.x * params->height_scale;
    P.y = view_dir_ts.y * params->height_scale;
    
    P.x /= view_z;
    P.y /= view_z;
    
    Vec2 delta_uv;
    delta_uv.x = P.x / num_layers;
    delta_uv.y = P.y / num_layers;
    
    // 3. Ray Marching Loop
    Vec2 current_uv = uv;
    float current_depth_map_value = sample_height(height_map, current_uv);
    
    // While our ray is ABOVE the surface height...
    // Ray depth starts at 0.0 and increases by layer_depth
    // Surface height acts as a blocker. 0 being bottom, 1 being top is common, 
    // BUT usually in POM: 0.0 is surface, -1.0 is deep.
    // Let's stick to: we march "down" into the surface.
    // Ray depth goes 0.0 -> 1.0. 
    // Height map: 1.0 (white) = surface ("high"), 0.0 (black) = deep ("low").
    // Wait, POM usually defines ray depth 0 as top surface.
    // We check: is RayDepth > DerivedHeight?
    // DerivedHeight = (1.0 - TextureSample) probably creates a "hole".
    // Let's assume texture: 1.0 = top, 0.0 = bottom.
    // Ray depth starts at 0.0 (top).
    // We intersect when RayDepth > (1.0 - Sample) ? Or if we view depth as 0..1 going down?
    
    // Standard approach:
    // Texture: 1=high, 0=low.
    // Ray depth: 0 at start, increases.
    // Condition to continue: RayDepth < HeightAtCurrentUV ? no...
    // Condition to continue: RayDepth < (Value from Map converted to depth form)
    // Usually depth form = (1.0 - height_sample).
    // So while current_layer_depth < (1.0 - sample_height(...))
    
    // Let's use the USER PROMPT logic if provided, but user prompt code was incomplete.
    // "while (current_depth < current_height)" suggests current_height is defined as depth?
    // Let's assume sample_height returns DEPTH directly (1.0=deep, 0.0=surface) or we treat it as Height (1.0=surface).
    // Common convention: White=High.
    // So DepthMap = 1.0 - HeightMap.
    
    float current_map_depth = 1.0f - current_depth_map_value; // Assuming white is high
    
    while (current_layer_depth < current_map_depth) {
        // Shift data
        current_uv.x -= delta_uv.x;
        current_uv.y -= delta_uv.y;
        
        current_depth_map_value = sample_height(height_map, current_uv);
        current_map_depth = 1.0f - current_depth_map_value;
        
        current_layer_depth += layer_depth;
    }
    
    // 4. Parallax Occlusion (Refinement)
    // Determine the split between the step before collision and after collision
    Vec2 prev_uv;
    prev_uv.x = current_uv.x + delta_uv.x;
    prev_uv.y = current_uv.y + delta_uv.y;
    
    // Depths at these steps
    float after_depth  = current_map_depth - current_layer_depth;
    float before_depth = (1.0f - sample_height(height_map, prev_uv)) - (current_layer_depth - layer_depth);
    
    // Interpolate
    float weight = after_depth / (after_depth - before_depth);
    Vec2 final_uv;
    final_uv.x = prev_uv.x * weight + current_uv.x * (1.0f - weight);
    final_uv.y = prev_uv.y * weight + current_uv.y * (1.0f - weight);
    
    return final_uv;
}
