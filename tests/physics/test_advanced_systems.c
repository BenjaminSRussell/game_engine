/**
 * COMPREHENSIVE PHYSICS TESTS - PART 3: Advanced Systems
 * Tests for: Soft Bodies, Cloth, Ragdoll, Constraints, CCD, Particles
 */

#include "physics_test.h"
#include <physics/advanced/complete_physics_batch.c>
#include <physics/advanced/final_systems_batch.c>

// ============================================================================
// SOFT BODY PBD TESTS
// ============================================================================

void test_soft_body_creation() {
    SoftBody *sb = soft_body_create(100);
    ASSERT_NOT_NULL(sb);
    ASSERT_INT_EQ(sb->particle_count, 100);
    ASSERT_NOT_NULL(sb->positions);
    ASSERT_NOT_NULL(sb->velocities);
    ASSERT_NOT_NULL(sb->inv_masses);
    
    // Check default mass
    ASSERT_FLOAT_EQ(sb->inv_masses[0], 1.0f, 0.01f);
    
    free(sb->positions);
    free(sb->velocities);
    free(sb->inv_masses);
    free(sb);
}

void test_soft_body_pbd_constraint() {
    SoftBody *sb = soft_body_create(2);
    
    // Set up distance constraint
    sb->distance_constraints = malloc(2 * sizeof(uint32_t));
    sb->rest_lengths = malloc(sizeof(float));
    sb->distance_constraints[0] = 0;
    sb->distance_constraints[1] = 1;
    sb->rest_lengths[0] = 1.0f;
    sb->constraint_count = 1;
    
    // Set initial positions
    sb->positions[0] = 0.0f; sb->positions[1] = 0.0f; sb->positions[2] = 0.0f;
    sb->positions[3] = 2.0f; sb->positions[4] = 0.0f; sb->positions[5] = 0.0f;
    
    // Update (should enforce distance constraint)
    soft_body_update_pbd(sb, 0.016f, 10);
    
    // Distance should be closer to rest length
    float dx = sb->positions[3] - sb->positions[0];
    float dist = sqrtf(dx * dx);
    ASSERT_TRUE(fabsf(dist - 1.0f) < 0.5f); // Moved toward 1.0
    
    free(sb->positions);
    free(sb->velocities);
    free(sb->inv_masses);
    free(sb->distance_constraints);
    free(sb->rest_lengths);
    free(sb);
}

// ============================================================================
// CLOTH SIMULATION TESTS
// ============================================================================

void test_cloth_creation() {
    ClothGrid *cloth = cloth_create(10, 10, 1.0f);
    ASSERT_NOT_NULL(cloth);
    ASSERT_INT_EQ(cloth->width, 10);
    ASSERT_INT_EQ(cloth->height, 10);
    ASSERT_NOT_NULL(cloth->soft_body);
    
    // Check particles were created
    ASSERT_INT_EQ(cloth->soft_body->particle_count, 100);
    
    // Check constraints (structural)
    ASSERT_TRUE(cloth->soft_body->constraint_count > 0);
    
    free(cloth->soft_body->positions);
    free(cloth->soft_body->velocities);
    free(cloth->soft_body->inv_masses);
    free(cloth->soft_body->distance_constraints);
    free(cloth->soft_body->rest_lengths);
    free(cloth->soft_body);
    free(cloth);
}

void test_cloth_pinned_vertices() {
    ClothGrid *cloth = cloth_create(5, 5, 1.0f);
    
    // Top corners should be pinned (inv_mass = 0)
    ASSERT_FLOAT_EQ(cloth->soft_body->inv_masses[0], 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(cloth->soft_body->inv_masses[4], 0.0f, 0.01f);
    
    // Other vertices should be free
    ASSERT_TRUE(cloth->soft_body->inv_masses[12] > 0.0f);
    
    free(cloth->soft_body->positions);
    free(cloth->soft_body->velocities);
    free(cloth->soft_body->inv_masses);
    free(cloth->soft_body->distance_constraints);
    free(cloth->soft_body->rest_lengths);
    free(cloth->soft_body);
    free(cloth);
}

// ============================================================================
// RAGDOLL TESTS
// ============================================================================

void test_ragdoll_creation() {
    Ragdoll *ragdoll = ragdoll_create(10);
    ASSERT_NOT_NULL(ragdoll);
    ASSERT_INT_EQ(ragdoll->joint_count, 10);
    ASSERT_NOT_NULL(ragdoll->joints);
    ASSERT_NOT_NULL(ragdoll->pose_target);
    
    // PD gains should be positive
    ASSERT_TRUE(ragdoll->pd_gain_p > 0.0f);
    ASSERT_TRUE(ragdoll->pd_gain_d > 0.0f);
    
    free(ragdoll->joints);
    free(ragdoll->pose_target);
    free(ragdoll);
}

// ============================================================================
// CONSTRAINT TESTS
// ============================================================================

void test_hinge_joint_constraint() {
    float pos_a[3] = {0, 0, 0};
    float pos_b[3] = {2, 1, 0}; // Off-axis
    float axis[3] = {0, 1, 0}; // Y-axis
    
    hinge_joint_solve(pos_a, pos_b, axis, 0.0001f, 0.016f);
    
    // After solving, should be aligned with axis
    // (perpendicular component reduced)
    ASSERT_TRUE(fabsf(pos_b[0] - pos_a[0]) < 2.0f);
    ASSERT_TRUE(fabsf(pos_b[2] - pos_a[2]) < 1.0f);
}

void test_ball_socket_joint() {
    float pos_a[3] = {0, 0, 0};
    float pos_b[3] = {2, 0, 0};
    float anchor[3] = {1, 0, 0};
    
    ball_socket_joint_solve(pos_a, pos_b, anchor, 0.0001f, 0.016f);
    
    // Bodies should move toward anchor constraint
    ASSERT_TRUE(fabsf(pos_a[0]) > 0.0f); // Moved right
    ASSERT_TRUE(fabsf(pos_b[0]) < 2.0f); // Moved left
}

// ============================================================================
// CCD TESTS
// ============================================================================

void test_swept_sphere_collision() {
    float start[3] = {0, 0, 0};
    float end[3] = {10, 0, 0};
    float radius = 1.0f;
    
    float obstacle[3] = {5, 0, 0};
    float obs_radius = 1.0f;
    
    float hit_t;
    bool hit = swept_sphere_test(start, end, radius, obstacle, obs_radius, &hit_t);
    
    ASSERT_TRUE(hit);
    ASSERT_TRUE(hit_t > 0.0f && hit_t < 1.0f);
    ASSERT_TRUE(hit_t < 0.5f); // Should hit before midpoint
}

void test_swept_sphere_miss() {
    float start[3] = {0, 0, 0};
    float end[3] = {10, 0, 0};
    float radius = 1.0f;
    
    float obstacle[3] = {5, 10, 0}; // Far above
    float obs_radius = 1.0f;
    
    float hit_t;
    bool hit = swept_sphere_test(start, end, radius, obstacle, obs_radius, &hit_t);
    
    ASSERT_FALSE(hit);
}

// ============================================================================
// PARTICLE SYSTEM TESTS
// ============================================================================

void test_particle_system_creation() {
    ParticleSystem *ps = particle_system_create(1000);
    ASSERT_NOT_NULL(ps);
    ASSERT_INT_EQ(ps->capacity, 1000);
    ASSERT_INT_EQ(ps->active_count, 0);
    ASSERT_NOT_NULL(ps->particles);
    
    free(ps->particles);
    free(ps);
}

void test_particle_emission() {
    ParticleSystem *ps = particle_system_create(100);
    ps->emission_rate = 10.0f; // 10 per second
    
    // Update for 1 second
    for (int i = 0; i < 60; i++) {
        particle_system_update(ps, 1.0f / 60.0f);
    }
    
    // Should have emitted ~10 particles
    ASSERT_TRUE(ps->active_count >= 8 && ps->active_count <= 12);
    
    free(ps->particles);
    free(ps);
}

void test_particle_lifetime() {
    ParticleSystem *ps = particle_system_create(100);
    
    // Add particle with short lifetime
    ps->particles[0].lifetime = 0.1f;
    ps->active_count = 1;
    
    // Update for longer than lifetime
    particle_system_update(ps, 0.2f);
    
    // Particle should be dead
    ASSERT_INT_EQ(ps->active_count, 0);
    
    free(ps->particles);
    free(ps);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    TEST_SUITE_BEGIN("Physics Advanced Systems");
    
    printf("\n--- Soft Body PBD Tests ---\n");
    RUN_TEST(test_soft_body_creation);
    RUN_TEST(test_soft_body_pbd_constraint);
    
    printf("\n--- Cloth Tests ---\n");
    RUN_TEST(test_cloth_creation);
    RUN_TEST(test_cloth_pinned_vertices);
    
    printf("\n--- Ragdoll Tests ---\n");
    RUN_TEST(test_ragdoll_creation);
    
    printf("\n--- Constraint Tests ---\n");
    RUN_TEST(test_hinge_joint_constraint);
    RUN_TEST(test_ball_socket_joint);
    
    printf("\n--- CCD Tests ---\n");
    RUN_TEST(test_swept_sphere_collision);
    RUN_TEST(test_swept_sphere_miss);
    
    printf("\n--- Particle System Tests ---\n");
    RUN_TEST(test_particle_system_creation);
    RUN_TEST(test_particle_emission);
    RUN_TEST(test_particle_lifetime);
    
    TEST_SUITE_END();
}
