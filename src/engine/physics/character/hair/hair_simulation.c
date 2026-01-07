#include "hair_common.h"

void hair_simulate_step(id<MTLComputeCommandEncoder> encoder, HairSystem* system, float dt, simd_float3 gravity, simd_float3 wind) {
    if (!system || !system->simulation_pipeline) return;
    
    [encoder setComputePipelineState:system->simulation_pipeline];
    [encoder setBuffer:system->strands_buffer offset:0 atIndex:0];
    
    HairSimulationParams params;
    params.dt = dt;
    params.gravity = gravity;
    params.damping = 0.95f;
    params.wind_strength = simd_length(wind);
    params.wind_dir = (params.wind_strength > 0) ? simd_normalize(wind) : simd_make_float3(0,0,0);
    
    [encoder setBytes:&params length:sizeof(params) atIndex:1];
    
    // One thread per control point or per strand?
    // Per strand processing is easier for length constraints (serial solve per strand).
    // Let's launch one thread per strand segment or control point if doing global loose constraints,
    // but typically hair simulation is done per-strand efficiently.
    // Let's dispatch threads per control point for simple PBD force app, 
    // and rely on a specific 'solve' kernel for length constraints if needed, 
    // or do it all in one thread-per-strand kernel.
    // C prompt implied `hair_simulate_step` calls compute.
    
    // We'll assume a Thread-Per-Strand kernel for efficient length constraint solving in local memory.
    uint32_t threadGroupSize = 64;
    MTLSize threadGroup = MTLSizeMake(threadGroupSize, 1, 1);
    MTLSize gridSize = MTLSizeMake(system->strand_count, 1, 1);
    
    // If strand count > grid size, we might need to adjust, but dispatchThreads handles this in Metal 2+
    [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadGroup];
}
