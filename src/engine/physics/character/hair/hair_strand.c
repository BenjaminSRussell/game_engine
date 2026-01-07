#include "hair_common.h"
#include <stdlib.h>

// Helper to generate a random float between 0 and 1
static float random_float() {
    return (float)rand() / (float)RAND_MAX;
}

HairSystem* hair_system_create(id<MTLDevice> device, uint32_t count, id<MTLTexture> density_map) {
    if (!device) return NULL;
    
    HairSystem* system = (HairSystem*)calloc(1, sizeof(HairSystem));
    system->strand_count = count;
    system->points_per_strand = HAIR_SEGMENTS_PER_STRAND;
    system->density_map = density_map; // Retain if needed, omitting for brevity
    
    uint32_t total_points = count * system->points_per_strand;
    size_t buffer_size = sizeof(HairControlPoint) * total_points;
    
    HairControlPoint* initial_data = (HairControlPoint*)malloc(buffer_size);
    
    // Initialize strands
    for (uint32_t i = 0; i < count; i++) {
        // Random root position on a sphere/scalp (simple placeholder: sphere)
        // In a real engine, we'd sample the density map or a mesh
        float theta = random_float() * 2.0f * M_PI;
        float phi = acosf(2.0f * random_float() - 1.0f);
        float r = 0.5f; // Head radius
        
        simd_float3 root_pos = simd_make_float3(
            r * sinf(phi) * cosf(theta),
            r * sinf(phi) * sinf(theta),
            r * cosf(phi)
        );
        
        simd_float3 normal = simd_normalize(root_pos);
        float strand_len = 0.3f + random_float() * 0.2f;
        float seg_len = strand_len / (HAIR_SEGMENTS_PER_STRAND - 1);
        
        for (uint32_t j = 0; j < HAIR_SEGMENTS_PER_STRAND; j++) {
            uint32_t idx = i * HAIR_SEGMENTS_PER_STRAND + j;
            HairControlPoint* p = &initial_data[idx];
            
            // Extrude outwards
            p->position = root_pos + normal * (float)j * seg_len;
            
            // Gravity sag (simple pre-sag)
            p->position.y -= 0.05f * (float)j * (float)j * seg_len;
            
            p->velocity = simd_make_float3(0,0,0);
            p->inv_mass = (j == 0) ? 0.0f : 1.0f; // Root is pinned
            p->thickness = 0.005f * (1.0f - (float)j / HAIR_SEGMENTS_PER_STRAND); // Taper
            p->tangent = simd_make_float4(normal.x, normal.y, normal.z, 0.0f);
        }
    }
    
    system->strands_buffer = [device newBufferWithBytes:initial_data length:buffer_size options:MTLResourceStorageModeShared];
    free(initial_data);
    
    // Create Index Buffer for Line Strip Rendering
    // Each strand is a line strip. Primitive restart or separate draw calls?
    // Using simple line list or strip index buffer with restart index is common.
    // Here we'll generate indices for LineStrip with primitive restart or just Lines (Segments)
    // To support tessellation, we might need patch control points.
    // Let's assume indices for [0,1, 1,2, 2,3...] for debug lines first.
    
    uint32_t segments = HAIR_SEGMENTS_PER_STRAND - 1;
    uint32_t index_count = count * segments * 2;
    uint32_t* indices = (uint32_t*)malloc(sizeof(uint32_t) * index_count);
    
    uint32_t idx_ptr = 0;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t base = i * HAIR_SEGMENTS_PER_STRAND;
        for (uint32_t j = 0; j < segments; j++) {
            indices[idx_ptr++] = base + j;
            indices[idx_ptr++] = base + j + 1;
        }
    }
    
    system->render_index_buffer = [device newBufferWithBytes:indices length:sizeof(uint32_t) * index_count options:MTLResourceStorageModeShared];
    free(indices);
    
    return system;
}

void hair_system_destroy(HairSystem* system) {
    if (system) {
        free(system);
    }
}
