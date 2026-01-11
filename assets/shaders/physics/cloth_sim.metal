// cloth_sim.metal
// Cloth Simulation Compute Shaders
#include <metal_stdlib>
using namespace metal;

struct ClothParticle {
    float3 position;
    float3 prev_position;
    float3 velocity;
    float3 normal;
    float mass;
    float inv_mass;
    bool pinned;
};

struct ClothConstraint {
    uint index_a;
    uint index_b;
    float rest_length;
    float stiffness;
};

struct ClothUniforms {
    float dt;
    float3 gravity;
    float damping;
};

kernel void cloth_integrate(
    device ClothParticle* particles [[buffer(0)]],
    constant ClothUniforms& uniforms [[buffer(1)]],
    uint gid [[thread_position_in_grid]]
) {
    if (particles[gid].pinned) return;
    
    // Verlet integration
    float3 pos = particles[gid].position;
    float3 prev = particles[gid].prev_position;
    
    float3 vel = (pos - prev) * (1.0 - uniforms.damping);
    float3 next = pos + vel + uniforms.gravity * uniforms.dt * uniforms.dt;
    
    particles[gid].prev_position = pos;
    particles[gid].position = next;
    particles[gid].velocity = (next - pos) / uniforms.dt;
}

kernel void cloth_solve_constraints(
    device ClothParticle* particles [[buffer(0)]],
    device ClothConstraint* constraints [[buffer(1)]],
    uint gid [[thread_position_in_grid]]
) {
    ClothConstraint c = constraints[gid];
    
    float3 p1 = particles[c.index_a].position;
    float3 p2 = particles[c.index_b].position;
    
    float3 delta = p2 - p1;
    float dist = length(delta);
    
    if (dist > 0.0001) {
        float correction = (dist - c.rest_length) / dist * 0.5 * c.stiffness;
        float3 offset = delta * correction;
        
        if (!particles[c.index_a].pinned) particles[c.index_a].position += offset;
        if (!particles[c.index_b].pinned) particles[c.index_b].position -= offset;
    }
}
