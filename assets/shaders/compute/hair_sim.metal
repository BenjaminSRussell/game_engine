#include <metal_stdlib>
using namespace metal;

struct HairControlPoint {
    float3 position;
    float inv_mass;
    float3 velocity;
    float thickness;
    float4 tangent; 
};

struct HairParams {
    float3 gravity;
    float dt;
    float damping;
    float wind_strength;
    float3 wind_dir;
    float _pad;
};

// One thread per strand
kernel void hair_simulate(device HairControlPoint* all_points [[buffer(0)]],
                          constant HairParams& params [[buffer(1)]],
                          uint id [[thread_position_in_grid]]) {
    
    // Constants matching C side
    const uint POINTS_PER_STRAND = 16;
    
    uint base_idx = id * POINTS_PER_STRAND;
    
    // 1. Integration (Apply Forces)
    // Skip root (point 0)
    for (uint i = 1; i < POINTS_PER_STRAND; i++) {
        uint idx = base_idx + i;
        device HairControlPoint& p = all_points[idx];
        
        if (p.inv_mass == 0.0) continue;
        
        // Forces
        float3 force = params.gravity;
        
        // Simple Wind
        if (params.wind_strength > 0) {
            float noise = fract(sin(dot(p.position, float3(12.9898, 78.233, 45.163))) * 43758.5453);
            force += params.wind_dir * params.wind_strength * (0.5 + 0.5 * noise);
        }
        
        float3 accel = force * p.inv_mass;
        p.velocity += accel * params.dt;
        p.velocity *= params.damping;
        
        p.position += p.velocity * params.dt;
    }
    
    // 2. Length Constraints (Follow The Leader / FTl)
    // Enforce distance between i and i-1
    // We iterate multiple times or just once for speed
    // Ideally we know the rest length. Assuming uniform rest length for now or derived from initial.
    // For this demo, let's assume a fixed segment length based on initialization logic
    // But better to store it. We'll simplify and enforce the distance to previous particle based on current distance? 
    // No, that's wrong. We need the rest length.
    // Piling hacks: Let's assume rest_length is stored or passed. 
    // Or we just calculate it from the discrepancy?
    // Let's assume a constant rest length for all segments for now (e.g. 0.02) or calculate it.
    // Since we don't have it in the struct, we'll assume the initial setup was rigid and just keep them 'close' 
    // or rely on a hardcoded value that matches `hair_strand.c` (0.3 length / 15 segments = 0.02).
    float rest_length = 0.02; 
    
    // Iterative solve
    const int ITERATIONS = 3;
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (uint i = 1; i < POINTS_PER_STRAND; i++) {
            uint idx_prev = base_idx + i - 1;
            uint idx_curr = base_idx + i;
            
            device HairControlPoint& p_prev = all_points[idx_prev];
            device HairControlPoint& p_curr = all_points[idx_curr];
            
            float3 delta = p_curr.position - p_prev.position;
            float dist = length(delta);
            
            if (dist > 1e-5) {
                float diff = (dist - rest_length) / dist;
                float3 correction = delta * diff * 0.9; // stiffness
                
                // If prev is pinned (root), it doesn't move. 
                // If i=1, prev is root (inv_mass 0).
                // But generally, usually only move child towards parent in detailed hair (FTL)
                // But PBD moves both.
                
                float w1 = p_prev.inv_mass;
                float w2 = p_curr.inv_mass;
                float w_sum = w1 + w2;
                
                if (w_sum > 0.0) {
                    p_prev.position += correction * (w1 / w_sum);
                    p_curr.position -= correction * (w2 / w_sum);
                }
            }
        }
    }
    
    // Update Tangents for rendering (Kajiya-Kay needs tangent)
    for (uint i = 0; i < POINTS_PER_STRAND; i++) {
        uint idx = base_idx + i;
        float3 t;
        if (i == 0) {
             t = normalize(all_points[base_idx + 1].position - all_points[base_idx].position);
        } else if (i == POINTS_PER_STRAND - 1) {
             t = normalize(all_points[base_idx + i].position - all_points[base_idx + i - 1].position);
        } else {
             t = normalize(all_points[base_idx + i + 1].position - all_points[base_idx + i - 1].position);
        }
        all_points[idx].tangent = float4(t, 0.0);
    }
}
