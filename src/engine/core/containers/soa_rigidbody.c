/**
 * Structure of Arrays (SoA) Container Implementation
 * 
 * Manages memory allocation for cache-aligned rigid body arrays.
 */

#include "core/containers/soa_rigidbody.h"
#include "core/simd/simd_math.h"
#include <stdlib.h>
#include <string.h>

// Platform-specific aligned allocation
#if defined(_WIN32)
    #define aligned_malloc(size, alignment) _aligned_malloc(size, alignment)
    #define aligned_free(ptr) _aligned_free(ptr)
#elif defined(__APPLE__) || defined(__linux__)
    #include <stdlib.h>
    #define aligned_malloc(size, alignment) aligned_alloc(alignment, size)
    #define aligned_free(ptr) free(ptr)
#else
    #define aligned_malloc(size, alignment) malloc(size)
    #define aligned_free(ptr) free(ptr)
#endif

// Alignment for SIMD (AVX requires 32-byte, AVX-512 requires 64-byte)
#define SIMD_ALIGNMENT 32

// Helper: Round size up to multiple of alignment (required for aligned_alloc on some platforms)
static inline size_t align_size(size_t size, size_t alignment) {
    return ((size + alignment - 1) / alignment) * alignment;
}

RigidBodySoA* soa_rigidbody_create(uint32_t capacity) {
    RigidBodySoA *soa = (RigidBodySoA*)malloc(sizeof(RigidBodySoA));
    if (!soa) return NULL;
    
    soa->capacity = capacity;
    soa->count = 0;
    
    // Allocate aligned arrays for vector data
    // Note: aligned_alloc requires size to be multiple of alignment on some platforms
    size_t v4f_size = align_size(sizeof(v4f) * capacity, SIMD_ALIGNMENT);
    size_t float_size = align_size(sizeof(float) * capacity, SIMD_ALIGNMENT);
    size_t uint32_size = align_size(sizeof(uint32_t) * capacity, SIMD_ALIGNMENT);
    size_t uint8_size = align_size(sizeof(uint8_t) * capacity, SIMD_ALIGNMENT);
    size_t ptr_size = sizeof(void*) * capacity;
    
    // Transform arrays
    soa->positions = (v4f*)aligned_malloc(v4f_size, SIMD_ALIGNMENT);
    soa->rotations = (v4f*)aligned_malloc(v4f_size, SIMD_ALIGNMENT);
    
    // Linear dynamics
    soa->velocities = (v4f*)aligned_malloc(v4f_size, SIMD_ALIGNMENT);
    soa->forces = (v4f*)aligned_malloc(v4f_size, SIMD_ALIGNMENT);
    soa->inv_masses = (float*)aligned_malloc(float_size, SIMD_ALIGNMENT);
    
    // Angular dynamics
    soa->angular_velocities = (v4f*)aligned_malloc(v4f_size, SIMD_ALIGNMENT);
    soa->torques = (v4f*)aligned_malloc(v4f_size, SIMD_ALIGNMENT);
    
    // Inertia tensors
    soa->inv_inertia_0 = (v4f*)aligned_malloc(v4f_size, SIMD_ALIGNMENT);
    soa->inv_inertia_1 = (v4f*)aligned_malloc(v4f_size, SIMD_ALIGNMENT);
    soa->inv_inertia_2 = (v4f*)aligned_malloc(v4f_size, SIMD_ALIGNMENT);
    
    // Material properties
    soa->friction = (float*)aligned_malloc(float_size, SIMD_ALIGNMENT);
    soa->restitution = (float*)aligned_malloc(float_size, SIMD_ALIGNMENT);
    soa->linear_damping = (float*)aligned_malloc(float_size, SIMD_ALIGNMENT);
    soa->angular_damping = (float*)aligned_malloc(float_size, SIMD_ALIGNMENT);
    
    // State and metadata
    soa->flags = (uint8_t*)aligned_malloc(uint8_size, SIMD_ALIGNMENT);
    soa->entity_ids = (uint32_t*)aligned_malloc(uint32_size, SIMD_ALIGNMENT);
    soa->user_data = (void**)malloc(ptr_size);  // No alignment needed for pointers
    
    // Check for allocation failures
    if (!soa->positions || !soa->rotations || !soa->velocities || !soa->forces ||
        !soa->inv_masses || !soa->angular_velocities || !soa->torques ||
        !soa->inv_inertia_0 || !soa->inv_inertia_1 || !soa->inv_inertia_2 ||
        !soa->friction || !soa->restitution || !soa->linear_damping ||
        !soa->angular_damping || !soa->flags || !soa->entity_ids || !soa->user_data) {
        soa_rigidbody_destroy(soa);
        return NULL;
    }
    
    // Initialize arrays to zero
    memset(soa->positions, 0, v4f_size);
    memset(soa->rotations, 0, v4f_size);
    memset(soa->velocities, 0, v4f_size);
    memset(soa->forces, 0, v4f_size);
    memset(soa->inv_masses, 0, float_size);
    memset(soa->angular_velocities, 0, v4f_size);
    memset(soa->torques, 0, v4f_size);
    memset(soa->inv_inertia_0, 0, v4f_size);
    memset(soa->inv_inertia_1, 0, v4f_size);
    memset(soa->inv_inertia_2, 0, v4f_size);
    memset(soa->friction, 0, float_size);
    memset(soa->restitution, 0, float_size);
    memset(soa->linear_damping, 0, float_size);
    memset(soa->angular_damping, 0, float_size);
    memset(soa->flags, 0, uint8_size);
    memset(soa->entity_ids, 0, uint32_size);
    memset(soa->user_data, 0, ptr_size);
    
    return soa;
}

void soa_rigidbody_destroy(RigidBodySoA *soa) {
    if (!soa) return;
    
    // Free all aligned arrays
    aligned_free(soa->positions);
    aligned_free(soa->rotations);
    aligned_free(soa->velocities);
    aligned_free(soa->forces);
    aligned_free(soa->inv_masses);
    aligned_free(soa->angular_velocities);
    aligned_free(soa->torques);
    aligned_free(soa->inv_inertia_0);
    aligned_free(soa->inv_inertia_1);
    aligned_free(soa->inv_inertia_2);
    aligned_free(soa->friction);
    aligned_free(soa->restitution);
    aligned_free(soa->linear_damping);
    aligned_free(soa->angular_damping);
    aligned_free(soa->flags);
    aligned_free(soa->entity_ids);
    free(soa->user_data);
    
    free(soa);
}

uint32_t soa_rigidbody_add(RigidBodySoA *soa) {
    if (soa->count >= soa->capacity) {
        return UINT32_MAX;  // Container full
    }
    
    uint32_t index = soa->count;
    soa->count++;
    
    // Initialize new body to safe defaults
    v4f_zero(&soa->positions[index]);
    soa->rotations[index] = V4F_VECTOR(0.0f, 0.0f, 0.0f);
    soa->rotations[index].w = 1.0f;  // Identity quaternion
    v4f_zero(&soa->velocities[index]);
    v4f_zero(&soa->forces[index]);
    soa->inv_masses[index] = 0.0f;  // Infinite mass by default
    v4f_zero(&soa->angular_velocities[index]);
    v4f_zero(&soa->torques[index]);
    
    // Identity inertia tensor
    soa->inv_inertia_0[index] = V4F_VECTOR(1.0f, 0.0f, 0.0f);
    soa->inv_inertia_1[index] = V4F_VECTOR(0.0f, 1.0f, 0.0f);
    soa->inv_inertia_2[index] = V4F_VECTOR(0.0f, 0.0f, 1.0f);
    
    // Default material
    soa->friction[index] = 0.5f;
    soa->restitution[index] = 0.0f;
    soa->linear_damping[index] = 0.01f;
    soa->angular_damping[index] = 0.05f;
    
    // Active by default
    soa->flags[index] = RIGIDBODY_FLAG_ACTIVE;
    soa->entity_ids[index] = 0;
    soa->user_data[index] = NULL;
    
    return index;
}

void soa_rigidbody_remove(RigidBodySoA *soa, uint32_t index) {
    if (index >= soa->count) return;
    
    // Swap-and-pop: move last element to removed slot
    uint32_t last = soa->count - 1;
    
    if (index != last) {
        // Copy data from last element to removed slot
        soa->positions[index] = soa->positions[last];
        soa->rotations[index] = soa->rotations[last];
        soa->velocities[index] = soa->velocities[last];
        soa->forces[index] = soa->forces[last];
        soa->inv_masses[index] = soa->inv_masses[last];
        soa->angular_velocities[index] = soa->angular_velocities[last];
        soa->torques[index] = soa->torques[last];
        soa->inv_inertia_0[index] = soa->inv_inertia_0[last];
        soa->inv_inertia_1[index] = soa->inv_inertia_1[last];
        soa->inv_inertia_2[index] = soa->inv_inertia_2[last];
        soa->friction[index] = soa->friction[last];
        soa->restitution[index] = soa->restitution[last];
        soa->linear_damping[index] = soa->linear_damping[last];
        soa->angular_damping[index] = soa->angular_damping[last];
        soa->flags[index] = soa->flags[last];
        soa->entity_ids[index] = soa->entity_ids[last];
        soa->user_data[index] = soa->user_data[last];
    }
    
    soa->count--;
}

void soa_rigidbody_clear(RigidBodySoA *soa) {
    soa->count = 0;
}
