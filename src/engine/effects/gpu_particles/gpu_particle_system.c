/*
 * gpu_particle_system.c
 * GPU-driven particle system using Metal
 */

#import "gpu_particle_system.h"

void gpu_particles_emit(gpu_particle_system_t* sys, particle_emitter_t* emitter,
                         id<MTLComputeCommandEncoder> encoder, float dt) {
    if (!sys || !emitter || !encoder) return;

    emitter->emit_accumulator += emitter->emit_rate * dt;
    uint32_t emit_count = (uint32_t)emitter->emit_accumulator;
    emitter->emit_accumulator -= (float)emit_count;

    if (emit_count == 0) return;

    emitter->emit_count = emit_count;
    emitter->frame++; // Increment frame for randomness

    [encoder setComputePipelineState:sys->spawn_pipeline];
    [encoder setBuffer:sys->particles.positions offset:0 atIndex:0];
    [encoder setBuffer:sys->particles.velocities offset:0 atIndex:1];
    [encoder setBuffer:sys->particles.colors offset:0 atIndex:2];
    [encoder setBuffer:sys->particles.sizes offset:0 atIndex:3];
    [encoder setBuffer:sys->particles.lifetimes offset:0 atIndex:4];
    [encoder setBuffer:sys->particles.dead_list offset:0 atIndex:5];
    [encoder setBuffer:sys->particles.counters offset:0 atIndex:6];
    [encoder setBytes:emitter length:sizeof(particle_emitter_t) atIndex:7];

    MTLSize threadgroupSize = MTLSizeMake(64, 1, 1);
    MTLSize threadgroups = MTLSizeMake((emit_count + 63) / 64, 1, 1);
    
    [encoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadgroupSize];
}

void gpu_particles_update(gpu_particle_system_t* sys, update_uniforms_t* uniforms,
                           id<MTLComputeCommandEncoder> encoder) {
    if (!sys || !uniforms || !encoder) return;

    [encoder setComputePipelineState:sys->update_pipeline];
    [encoder setBuffer:sys->particles.positions offset:0 atIndex:0];
    [encoder setBuffer:sys->particles.velocities offset:0 atIndex:1];
    [encoder setBuffer:sys->particles.colors offset:0 atIndex:2];
    [encoder setBuffer:sys->particles.sizes offset:0 atIndex:3];
    [encoder setBuffer:sys->particles.lifetimes offset:0 atIndex:4];
    [encoder setBuffer:sys->particles.alive_list offset:0 atIndex:5];
    [encoder setBuffer:sys->particles.dead_list offset:0 atIndex:6];
    [encoder setBuffer:sys->particles.counters offset:0 atIndex:7];
    [encoder setBytes:uniforms length:sizeof(update_uniforms_t) atIndex:8];

    // Dispatch based on alive particles
    // Note: This requires the indirect dispatch or reading the counter back. 
    // For now, using indirect dispatch if available or a safe max dispatch.
    // The prompt's shader uses `if (tid >= counters.alive_count) return;` 
    // so we can dispatch for max_particles or use indirect dispatch.
    // The struct has `indirect_args`, let's assume we use that for update too if it was populated,
    // but typically update runs on all potential active particles or similar.
    // Given the prompt's `particle_update` kernel checks `tid >= counters.alive_count`,
    // and `alive_list` is packed, we should ideally dispatch `alive_count` threads.
    // Since we don't have `alive_count` on CPU easily without readback, 
    // and `indirect_args` is usually for drawing, we might need an indirect dispatch buffer for update too 
    // or just dispatch max_particles (wasteful) or manage it.
    // However, the `particle_update` kernel reads from `alive_list[tid]`, so we definitely need to dispatch 
    // at least `alive_count` threads. 
    // Let's assume for this phase we dispatch max_particles or a fixed block, 
    // but optimal is indirect dispatch. The prompt showed `indirect_args` for *draw*.
    // I will dispatch `(max_particles + 63) / 64` groups to be safe and simple for now, as the shader bounds check handles safety.
    
    MTLSize threadgroupSize = MTLSizeMake(64, 1, 1);
    MTLSize threadgroups = MTLSizeMake((sys->particles.max_particles + 63) / 64, 1, 1);
    [encoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadgroupSize];
}

void gpu_particles_render(gpu_particle_system_t* sys, id<MTLRenderCommandEncoder> encoder) {
    if (!sys || !encoder) return;

    [encoder setRenderPipelineState:sys->render_pipeline];
    [encoder setVertexBuffer:sys->particles.positions offset:0 atIndex:0];
    [encoder setVertexBuffer:sys->particles.colors offset:0 atIndex:1];
    [encoder setVertexBuffer:sys->particles.sizes offset:0 atIndex:2];

    // Indirect draw based on alive count stored in indirect_args
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle
             indirectBuffer:sys->particles.indirect_args
       indirectBufferOffset:0];
}
