#include "cloth_common.h"
#include <stdlib.h>

// Helper to add constraint safely
static void add_constraint_to_list(ClothConstraint* list, uint32_t* count, 
                                   ClothParticle* particles,
                                   uint32_t a, uint32_t b, float stiffness) {
    ClothConstraint* c = &list[(*count)++];
    c->particle_a = a;
    c->particle_b = b;
    c->stiffness = stiffness;
    
    // Calculate rest length
    c->rest_length = simd_distance(particles[a].position, particles[b].position);
}

void cloth_generate_constraints(ClothMesh* cloth, float structural_stiff, float shear_stiff, float bend_stiff) {
    if (!cloth || !cloth->particles_buffer) return;
    
    uint32_t width = cloth->grid_width;
    uint32_t height = cloth->grid_height;
    uint32_t num_particles = cloth->particle_count;
    
    // Estimate max constraints
    // Structural: ~2 per particle
    // Shear: ~2 per particle
    // Bending: ~2 per particle
    uint32_t max_constraints = num_particles * 6;
    ClothConstraint* constraints = (ClothConstraint*)malloc(sizeof(ClothConstraint) * max_constraints);
    uint32_t count = 0;
    
    ClothParticle* particles = (ClothParticle*)cloth->particles_buffer.contents;
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t i = y * width + x;
            
            // Structural
            if (x < width - 1) 
                add_constraint_to_list(constraints, &count, particles, i, i + 1, structural_stiff);
            if (y < height - 1) 
                add_constraint_to_list(constraints, &count, particles, i, i + width, structural_stiff);
                
            // Shear
            if (x < width - 1 && y < height - 1) {
                add_constraint_to_list(constraints, &count, particles, i, i + width + 1, shear_stiff);
                add_constraint_to_list(constraints, &count, particles, i + 1, i + width, shear_stiff);
            }
            
            // Bending (skip 1 node)
            if (x < width - 2) 
                add_constraint_to_list(constraints, &count, particles, i, i + 2, bend_stiff);
            if (y < height - 2) 
                add_constraint_to_list(constraints, &count, particles, i, i + width * 2, bend_stiff);
        }
    }
    
    cloth->constraint_count = count;
    cloth->constraints_buffer = [cloth->particles_buffer.device newBufferWithBytes:constraints 
                                                                           length:sizeof(ClothConstraint) * count 
                                                                          options:MTLResourceStorageModeShared];
    free(constraints);
}
