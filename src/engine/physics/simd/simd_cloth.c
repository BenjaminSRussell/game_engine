#include "simd_cloth.h"
#include <stdlib.h>
#include <string.h>

void simd_cloth_create(simd_cloth_system* cloth, u32 width, u32 height) {
    cloth->width = width;
    cloth->height = height;
    cloth->particle_count = width * height;
    cloth->constraint_count = (width - 1) * height + width * (height - 1); // Horizontal + vertical
    
    // Allocate particles
    cloth->particles = malloc(cloth->particle_count * sizeof(simd_cloth_particle));
    memset(cloth->particles, 0, cloth->particle_count * sizeof(simd_cloth_particle));
    
    // Allocate constraints
    cloth->constraints = malloc(cloth->constraint_count * sizeof(simd_cloth_constraint));
    memset(cloth->constraints, 0, cloth->constraint_count * sizeof(simd_cloth_constraint));
    
    // Initialize particles
    __m256 one_mass = _mm256_set1_ps(1.0f);
    __m256 inv_mass = _mm256_set1_ps(1.0f);
    
    for (u32 i = 0; i < cloth->particle_count; i += 8) {
        simd_cloth_particle* particle = &cloth->particles[i];
        
        // Set initial positions in a grid
        u32 x = i % width;
        u32 y = i / width;
        
        particle->position.x = _mm256_set1_ps((f32)x);
        particle->position.y = _mm256_set1_ps((f32)y);
        particle->position.z = _mm256_set1_ps(0.0f);
        
        particle->old_position = particle->position;
        particle->velocity.x = _mm256_setzero_ps();
        particle->velocity.y = _mm256_setzero_ps();
        particle->velocity.z = _mm256_setzero_ps();
        particle->force.x = _mm256_setzero_ps();
        particle->force.y = _mm256_setzero_ps();
        particle->force.z = _mm256_setzero_ps();
        
        particle->mass = one_mass;
        particle->inv_mass = inv_mass;
        particle->pinned = 0;
    }
    
    // Initialize constraints
    u32 constraint_idx = 0;
    __m256 rest_length = _mm256_set1_ps(1.0f);
    __m256 stiffness = _mm256_set1_ps(0.8f);
    
    // Horizontal constraints
    for (u32 y = 0; y < height; y++) {
        for (u32 x = 0; x < width - 1; x++) {
            u32 idx = y * width + x;
            cloth->constraints[constraint_idx].particle_a = idx;
            cloth->constraints[constraint_idx].particle_b = idx + 1;
            cloth->constraints[constraint_idx].rest_length = rest_length;
            cloth->constraints[constraint_idx].stiffness = stiffness;
            constraint_idx++;
        }
    }
    
    // Vertical constraints
    for (u32 y = 0; y < height - 1; y++) {
        for (u32 x = 0; x < width; x++) {
            u32 idx = y * width + x;
            cloth->constraints[constraint_idx].particle_a = idx;
            cloth->constraints[constraint_idx].particle_b = idx + width;
            cloth->constraints[constraint_idx].rest_length = rest_length;
            cloth->constraints[constraint_idx].stiffness = stiffness;
            constraint_idx++;
        }
    }
    
    // Set default parameters
    cloth->damping = _mm256_set1_ps(0.99f);
    cloth->gravity = _mm256_set1_ps(-9.81f);
}

void simd_cloth_destroy(simd_cloth_system* cloth) {
    if (cloth->particles) {
        free(cloth->particles);
        cloth->particles = NULL;
    }
    
    if (cloth->constraints) {
        free(cloth->constraints);
        cloth->constraints = NULL;
    }
    
    cloth->particle_count = 0;
    cloth->constraint_count = 0;
}

void simd_cloth_apply_forces(simd_cloth_system* cloth) {
    __m256 gravity_vec = _mm256_set1_ps(0.0f);
    __m256 gravity_y = cloth->gravity;
    
    for (u32 i = 0; i < cloth->particle_count; i += 8) {
        simd_cloth_particle* particle = &cloth->particles[i];
        
        // Apply gravity
        particle->force.y = _mm256_fmadd_ps(particle->mass, gravity_y, particle->force.y);
        
        // Apply damping
        particle->force.x = _mm256_mul_ps(particle->force.x, cloth->damping);
        particle->force.y = _mm256_mul_ps(particle->force.y, cloth->damping);
        particle->force.z = _mm256_mul_ps(particle->force.z, cloth->damping);
    }
}

void simd_cloth_satisfy_constraints(simd_cloth_system* cloth, u32 iterations) {
    for (u32 iter = 0; iter < iterations; iter++) {
        for (u32 i = 0; i < cloth->constraint_count; i += 8) {
            simd_cloth_constraint* constraint = &cloth->constraints[i];
            
            // Load particle positions
            simd_cloth_particle* particle_a = &cloth->particles[constraint->particle_a];
            simd_cloth_particle* particle_b = &cloth->particles[constraint->particle_b];
            
            // Calculate distance between particles
            __m256 dx = _mm256_sub_ps(particle_b->position.x, particle_a->position.x);
            __m256 dy = _mm256_sub_ps(particle_b->position.y, particle_a->position.y);
            __m256 dz = _mm256_sub_ps(particle_b->position.z, particle_a->position.z);
            
            __m256 distance_sq = _mm256_add_ps(_mm256_add_ps(
                _mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy)), _mm256_mul_ps(dz, dz));
            __m256 distance = _mm256_sqrt_ps(distance_sq);
            
            // Avoid division by zero
            __m256 min_dist = _mm256_set1_ps(0.001f);
            distance = _mm256_max_ps(distance, min_dist);
            
            // Calculate correction
            __m256 rest_length = constraint->rest_length;
            __m256 difference = _mm256_sub_ps(distance, rest_length);
            __m256 correction_factor = _mm256_div_ps(difference, distance);
            __m256 half_correction = _mm256_mul_ps(correction_factor, _mm256_set1_ps(0.5f));
            __m256 stiffness = constraint->stiffness;
            __m256 final_correction = _mm256_mul_ps(half_correction, stiffness);
            
            // Apply correction
            __m256 correction_x = _mm256_mul_ps(dx, final_correction);
            __m256 correction_y = _mm256_mul_ps(dy, final_correction);
            __m256 correction_z = _mm256_mul_ps(dz, final_correction);
            
            particle_a->position.x = _mm256_add_ps(particle_a->position.x, correction_x);
            particle_a->position.y = _mm256_add_ps(particle_a->position.y, correction_y);
            particle_a->position.z = _mm256_add_ps(particle_a->position.z, correction_z);
            
            particle_b->position.x = _mm256_sub_ps(particle_b->position.x, correction_x);
            particle_b->position.y = _mm256_sub_ps(particle_b->position.y, correction_y);
            particle_b->position.z = _mm256_sub_ps(particle_b->position.z, correction_z);
        }
    }
}

void simd_cloth_integrate(simd_cloth_system* cloth, f32 dt) {
    __m256 dt_vec = _mm256_set1_ps(dt);
    __m256 half_dt_sq = _mm256_set1_ps(dt * dt * 0.5f);
    
    for (u32 i = 0; i < cloth->particle_count; i += 8) {
        simd_cloth_particle* particle = &cloth->particles[i];
        
        // Skip pinned particles
        if (particle->pinned) continue;
        
        // Verlet integration
        __m256 acceleration_x = _mm256_mul_ps(particle->force.x, particle->inv_mass);
        __m256 acceleration_y = _mm256_mul_ps(particle->force.y, particle->inv_mass);
        __m256 acceleration_z = _mm256_mul_ps(particle->force.z, particle->inv_mass);
        
        // Calculate new position
        __m256 new_x = _mm256_add_ps(_mm256_add_ps(particle->position.x, 
            _mm256_mul_ps(particle->position.x, _mm256_set1_ps(2.0f))), 
            _mm256_mul_ps(particle->old_position.x, _mm256_set1_ps(-1.0f)));
        new_x = _mm256_fmadd_ps(acceleration_x, half_dt_sq, new_x);
        
        __m256 new_y = _mm256_add_ps(_mm256_add_ps(particle->position.y, 
            _mm256_mul_ps(particle->position.y, _mm256_set1_ps(2.0f))), 
            _mm256_mul_ps(particle->old_position.y, _mm256_set1_ps(-1.0f)));
        new_y = _mm256_fmadd_ps(acceleration_y, half_dt_sq, new_y);
        
        __m256 new_z = _mm256_add_ps(_mm256_add_ps(particle->position.z, 
            _mm256_mul_ps(particle->position.z, _mm256_set1_ps(2.0f))), 
            _mm256_mul_ps(particle->old_position.z, _mm256_set1_ps(-1.0f)));
        new_z = _mm256_fmadd_ps(acceleration_z, half_dt_sq, new_z);
        
        // Update velocity (for damping calculations)
        particle->velocity.x = _mm256_div_ps(_mm256_sub_ps(new_x, particle->old_position.x), dt_vec);
        particle->velocity.y = _mm256_div_ps(_mm256_sub_ps(new_y, particle->old_position.y), dt_vec);
        particle->velocity.z = _mm256_div_ps(_mm256_sub_ps(new_z, particle->old_position.z), dt_vec);
        
        // Update positions
        particle->old_position = particle->position;
        particle->position.x = new_x;
        particle->position.y = new_y;
        particle->position.z = new_z;
        
        // Reset forces
        particle->force.x = _mm256_setzero_ps();
        particle->force.y = _mm256_setzero_ps();
        particle->force.z = _mm256_setzero_ps();
    }
}

void simd_cloth_update(simd_cloth_system* cloth, f32 dt) {
    simd_cloth_apply_forces(cloth);
    simd_cloth_satisfy_constraints(cloth, 2); // 2 constraint iterations
    simd_cloth_integrate(cloth, dt);
}

void simd_cloth_apply_wind(simd_cloth_system* cloth, const simd_vec3* wind_direction, __m256 wind_strength) {
    for (u32 i = 0; i < cloth->particle_count; i += 8) {
        simd_cloth_particle* particle = &cloth->particles[i];
        
        // Calculate wind force based on velocity difference
        __m256 wind_force_x = _mm256_mul_ps(wind_direction->x, wind_strength);
        __m256 wind_force_y = _mm256_mul_ps(wind_direction->y, wind_strength);
        __m256 wind_force_z = _mm256_mul_ps(wind_direction->z, wind_strength);
        
        // Apply some randomness for turbulence
        __m256 turbulence = _mm256_set1_ps(0.1f);
        wind_force_x = _mm256_fmadd_ps(_mm256_set1_ps(sin(i * 0.1f)), turbulence, wind_force_x);
        wind_force_z = _mm256_fmadd_ps(_mm256_set1_ps(cos(i * 0.1f)), turbulence, wind_force_z);
        
        // Add to particle forces
        particle->force.x = _mm256_add_ps(particle->force.x, wind_force_x);
        particle->force.y = _mm256_add_ps(particle->force.y, wind_force_y);
        particle->force.z = _mm256_add_ps(particle->force.z, wind_force_z);
    }
}

void simd_cloth_sphere_collision(simd_cloth_system* cloth, const simd_vec3* sphere_center, __m256 sphere_radius) {
    for (u32 i = 0; i < cloth->particle_count; i += 8) {
        simd_cloth_particle* particle = &cloth->particles[i];
        
        // Calculate distance from particle to sphere center
        __m256 dx = _mm256_sub_ps(particle->position.x, sphere_center->x);
        __m256 dy = _mm256_sub_ps(particle->position.y, sphere_center->y);
        __m256 dz = _mm256_sub_ps(particle->position.z, sphere_center->z);
        
        __m256 distance_sq = _mm256_add_ps(_mm256_add_ps(
            _mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy)), _mm256_mul_ps(dz, dz));
        __m256 distance = _mm256_sqrt_ps(distance_sq);
        
        // Check if particle is inside sphere
        __m256 radius_sq = _mm256_mul_ps(sphere_radius, sphere_radius);
        __m256 inside_sphere = _mm256_cmp_ps(distance_sq, radius_sq, _CMP_LT_OQ);
        
        // Push particle out of sphere
        __m256 min_dist = _mm256_set1_ps(0.001f);
        distance = _mm256_max_ps(distance, min_dist);
        
        __m256 push_distance = _mm256_sub_ps(sphere_radius, distance);
        __m256 push_x = _mm256_mul_ps(_mm256_div_ps(dx, distance), push_distance);
        __m256 push_y = _mm256_mul_ps(_mm256_div_ps(dy, distance), push_distance);
        __m256 push_z = _mm256_mul_ps(_mm256_div_ps(dz, distance), push_distance);
        
        // Apply push only if inside sphere
        particle->position.x = _mm256_add_ps(particle->position.x, _mm256_and_ps(inside_sphere, push_x));
        particle->position.y = _mm256_add_ps(particle->position.y, _mm256_and_ps(inside_sphere, push_y));
        particle->position.z = _mm256_add_ps(particle->position.z, _mm256_and_ps(inside_sphere, push_z));
    }
}
