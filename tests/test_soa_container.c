/**
 * Unit Tests for SoA Rigid Body Container
 * 
 * Validates memory layout, SIMD alignment, and functional correctness.
 */

#include "../src/engine/core/containers/soa_rigidbody.h"
#include "../src/engine/core/simd/simd_math.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#define TEST_CAPACITY 1000

// Helper: Check alignment
static int is_aligned(void *ptr, size_t alignment) {
    uintptr_t addr = (uintptr_t)ptr;
    return (addr % alignment) == 0;
}

// ========================================
// Test: Container Creation and Destruction
// ========================================

void test_create_destroy() {
    printf("Testing soa_rigidbody_create/destroy... ");
    
    RigidBodySoA *soa = soa_rigidbody_create(TEST_CAPACITY);
    assert(soa != NULL);
    assert(soa->capacity == TEST_CAPACITY);
    assert(soa->count == 0);
    
    // Verify all arrays are allocated
    assert(soa->positions != NULL);
    assert(soa->velocities != NULL);
    assert(soa->forces != NULL);
    assert(soa->inv_masses != NULL);
    
    soa_rigidbody_destroy(soa);
    
    printf("PASSED\n");
}

// ========================================
// Test: SIMD Alignment
// ========================================

void test_alignment() {
    printf("Testing SIMD alignment (32-byte)... ");
    
    RigidBodySoA *soa = soa_rigidbody_create(TEST_CAPACITY);
    
    // Check alignment of critical arrays
    assert(is_aligned(soa->positions, 32));
    assert(is_aligned(soa->velocities, 32));
    assert(is_aligned(soa->forces, 32));
    assert(is_aligned(soa->inv_masses, 32));
    assert(is_aligned(soa->angular_velocities, 32));
    assert(is_aligned(soa->inv_inertia_0, 32));
    
    soa_rigidbody_destroy(soa);
    
    printf("PASSED\n");
}

// ========================================
// Test: Add and Remove Bodies
// ========================================

void test_add_remove() {
    printf("Testing soa_rigidbody_add/remove... ");
    
    RigidBodySoA *soa = soa_rigidbody_create(10);
    
    // Add bodies
    uint32_t idx0 = soa_rigidbody_add(soa);
    assert(idx0 == 0);
    assert(soa->count == 1);
    
    uint32_t idx1 = soa_rigidbody_add(soa);
    assert(idx1 == 1);
    assert(soa->count == 2);
    
    uint32_t idx2 = soa_rigidbody_add(soa);
    assert(idx2 == 2);
    assert(soa->count == 3);
    
    // Set distinct values
    soa_set_position(soa, 0, &((v4f){1.0f, 0.0f, 0.0f, 0.0f}));
    soa_set_position(soa, 1, &((v4f){2.0f, 0.0f, 0.0f, 0.0f}));
    soa_set_position(soa, 2, &((v4f){3.0f, 0.0f, 0.0f, 0.0f}));
    
    // Remove middle element (should swap with last)
    soa_rigidbody_remove(soa, 1);
    assert(soa->count == 2);
    
    // Check that idx 1 now has idx 2's data (swap-and-pop)
    assert(soa->positions[1].x == 3.0f);
    
    soa_rigidbody_destroy(soa);
    
    printf("PASSED\n");
}

// ========================================
// Test: Capacity Limit
// ========================================

void test_capacity_limit() {
    printf("Testing capacity limit... ");
    
    RigidBodySoA *soa = soa_rigidbody_create(5);
    
    // Fill to capacity
    for (uint32_t i = 0; i < 5; i++) {
        uint32_t idx = soa_rigidbody_add(soa);
        assert(idx == i);
    }
    
    // Try to exceed capacity
    uint32_t overflow = soa_rigidbody_add(soa);
    assert(overflow == UINT32_MAX);  // Should fail
    assert(soa->count == 5);  // Count unchanged
    
    soa_rigidbody_destroy(soa);
    
    printf("PASSED\n");
}

// ========================================
// Test: Individual Accessors
// ========================================

void test_accessors() {
    printf("Testing individual accessors... ");
    
    RigidBodySoA *soa = soa_rigidbody_create(10);
    uint32_t idx = soa_rigidbody_add(soa);
    
    // Set position
    v4f pos = V4F_VECTOR(10.0f, 20.0f, 30.0f);
    soa_set_position(soa, idx, &pos);
    
    // Get position
    v4f retrieved_pos;
    soa_get_position(soa, idx, &retrieved_pos);
    assert(retrieved_pos.x == 10.0f);
    assert(retrieved_pos.y == 20.0f);
    assert(retrieved_pos.z == 30.0f);
    
    // Set velocity
    v4f vel = V4F_VECTOR(1.0f, 2.0f, 3.0f);
    soa_set_velocity(soa, idx, &vel);
    assert(soa->velocities[idx].x == 1.0f);
    
    // Set mass
    soa_set_mass(soa, idx, 5.0f);
    assert(soa->inv_masses[idx] == 0.2f);  // 1/5 = 0.2
    
    // Set static (mass = 0 => inv_mass = 0)
    soa_set_mass(soa, idx, 0.0f);
    assert(soa->inv_masses[idx] == 0.0f);
    
    // Active flag
    soa_set_active(soa, idx, true);
    assert(soa_is_active(soa, idx));
    soa_set_active(soa, idx, false);
    assert(!soa_is_active(soa, idx));
    
    soa_rigidbody_destroy(soa);
    
    printf("PASSED\n");
}

// ========================================
// Test: Batch Array Access
// ========================================

void test_batch_access() {
    printf("Testing batch array access... ");
    
    RigidBodySoA *soa = soa_rigidbody_create(TEST_CAPACITY);
    
    // Add multiple bodies
    for (uint32_t i = 0; i < 10; i++) {
        soa_rigidbody_add(soa);
    }
    
    // Get direct pointer to arrays
    v4f *positions = soa_get_positions(soa);
    v4f *velocities = soa_get_velocities(soa);
    float *inv_masses = soa_get_inv_masses(soa);
    
    // Modify via batch access
    for (uint32_t i = 0; i < soa->count; i++) {
        positions[i] = V4F_VECTOR((float)i, (float)i * 2, (float)i * 3);
        velocities[i] = V4F_VECTOR(1.0f, 0.0f, 0.0f);
        inv_masses[i] = 1.0f;
    }
    
    // Verify
    assert(positions[5].x == 5.0f);
    assert(positions[5].y == 10.0f);
    assert(velocities[3].x == 1.0f);
    
    soa_rigidbody_destroy(soa);
    
    printf("PASSED\n");
}

// ========================================
// Test: SIMD Integration (Performance Demonstration)
// ========================================

void test_simd_integration() {
    printf("Testing SIMD vectorized integration... ");
    
    RigidBodySoA *soa = soa_rigidbody_create(1000);
    
    // Add bodies with initial state
    for (uint32_t i = 0; i < 1000; i++) {
        uint32_t idx = soa_rigidbody_add(soa);
        soa->velocities[idx] = V4F_VECTOR(1.0f, 0.0f, 0.0f);
        soa->forces[idx] = V4F_VECTOR(10.0f, 0.0f, 0.0f);
        soa->inv_masses[idx] = 1.0f;  // mass = 1 kg
    }
    
    float dt = 1.0f / 60.0f;  // 60 FPS
    
    // Vectorized integration: v += (F / m) * dt
    // Since inv_mass is separate, we compute: v += F * inv_m * dt
    // For SIMD: we can process forces directly since inv_mass is broadcast
    
    v4f *velocities = soa_get_velocities(soa);
    v4f *forces = soa_get_forces(soa);
    float *inv_masses = soa_get_inv_masses(soa);
    uint32_t count = soa->count;
    
    // SIMD batch integration
    for (uint32_t i = 0; i < count; i++) {
        // v[i] += F[i] * inv_m[i] * dt
        v4f acceleration;
        v4f_scale(&acceleration, &forces[i], inv_masses[i]);
        v4f_scale(&acceleration, &acceleration, dt);
        v4f_add(&velocities[i], &velocities[i], &acceleration);
    }
    
    // Verify: v_new = 1.0 + 10.0 * 1.0 * (1/60) ≈ 1.1667
    float expected_vel = 1.0f + 10.0f * (1.0f / 60.0f);
    assert(fabsf(velocities[0].x - expected_vel) < 1e-4f);
    assert(fabsf(velocities[999].x - expected_vel) < 1e-4f);
    
    soa_rigidbody_destroy(soa);
    
    printf("PASSED\n");
}

// ========================================
// Main Test Runner
// ========================================

int main() {
    printf("=== SoA Rigid Body Container Unit Tests ===\n\n");
    
    test_create_destroy();
    test_alignment();
    test_add_remove();
    test_capacity_limit();
    test_accessors();
    test_batch_access();
    test_simd_integration();
    
    printf("\n=== ALL TESTS PASSED ===\n");
    return 0;
}
