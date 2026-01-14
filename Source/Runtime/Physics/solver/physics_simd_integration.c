// Physics SIMD Integration - Optimized batch physics simulation
// Uses SIMD to process 4-8 rigid bodies simultaneously
#include "physics_solver.h"
#include "core/simd/simd_math.h"
#include "core/logger/unified_logger.h"
#include <string.h>
#include <math.h>

// SIMD Physics State - Structure of Arrays for cache efficiency
typedef struct {
    // Position vectors (3 separate arrays for SoA layout)
    f32 *pos_x;       // [body_count], cache-line aligned
    f32 *pos_y;
    f32 *pos_z;

    // Velocity vectors
    f32 *vel_x;       // [body_count]
    f32 *vel_y;
    f32 *vel_z;

    // Angular velocity vectors
    f32 *ang_vel_x;   // [body_count]
    f32 *ang_vel_y;
    f32 *ang_vel_z;

    // Force accumulators
    f32 *force_x;     // [body_count], accumulates per frame
    f32 *force_y;
    f32 *force_z;

    // Torque accumulators
    f32 *torque_x;    // [body_count]
    f32 *torque_y;
    f32 *torque_z;

    // Properties
    f32 *inv_masses;  // [body_count], precomputed inverse mass
    f32 *inv_inertias; // [body_count*9], row-major 3x3 matrices

    // Quaternion rotations (for SoA processing)
    f32 *rot_x;       // [body_count]
    f32 *rot_y;
    f32 *rot_z;
    f32 *rot_w;

    // Metadata
    u32 *active_mask; // [body_count], 1 if active, 0 if sleeping
    u32 body_count;
    u32 capacity;
} PhysicsSimdState;

// Create SIMD physics state
static PhysicsSimdState* physics_simd_create(u32 capacity) {
    PhysicsSimdState *state = malloc(sizeof(PhysicsSimdState));
    if (!state) {
        LOG_ERROR("PHYSICS", "Failed to allocate SIMD physics state");
        return NULL;
    }

    state->capacity = capacity;
    state->body_count = 0;

    // Allocate arrays with 64-byte alignment for cache lines
    #define ALIGNED_ALLOC(count, size) \
        aligned_alloc(64, ((count) * (size) + 63) & ~63)

    state->pos_x = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->pos_y = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->pos_z = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->vel_x = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->vel_y = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->vel_z = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->ang_vel_x = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->ang_vel_y = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->ang_vel_z = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->force_x = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->force_y = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->force_z = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->torque_x = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->torque_y = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->torque_z = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->inv_masses = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->inv_inertias = ALIGNED_ALLOC(capacity * 9, sizeof(f32));
    state->rot_x = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->rot_y = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->rot_z = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->rot_w = ALIGNED_ALLOC(capacity, sizeof(f32));
    state->active_mask = ALIGNED_ALLOC(capacity, sizeof(u32));

    LOG_INFO("PHYSICS", "SIMD physics state created with capacity %u", capacity);
    return state;
}

// Destroy SIMD physics state
static void physics_simd_destroy(PhysicsSimdState *state) {
    if (!state) return;

    free(state->pos_x);
    free(state->pos_y);
    free(state->pos_z);
    free(state->vel_x);
    free(state->vel_y);
    free(state->vel_z);
    free(state->ang_vel_x);
    free(state->ang_vel_y);
    free(state->ang_vel_z);
    free(state->force_x);
    free(state->force_y);
    free(state->force_z);
    free(state->torque_x);
    free(state->torque_y);
    free(state->torque_z);
    free(state->inv_masses);
    free(state->inv_inertias);
    free(state->rot_x);
    free(state->rot_y);
    free(state->rot_z);
    free(state->rot_w);
    free(state->active_mask);
    free(state);

    LOG_INFO("PHYSICS", "SIMD physics state destroyed");
}

// Euler integration optimized with SIMD - processes 4 bodies per batch
// a_x[0..3] = force_x[0..3] * inv_mass[0..3]
// v_x[0..3] += a_x[0..3] * dt
// p_x[0..3] += v_x[0..3] * dt
static void physics_simd_integrate_positions(PhysicsSimdState *state, f32 dt) {
    // Process 4 bodies at a time with SIMD
    u32 batch_size = 4;
    u32 batches = (state->body_count + batch_size - 1) / batch_size;

    for (u32 batch = 0; batch < batches; batch++) {
        u32 start_idx = batch * batch_size;
        u32 end_idx = MIN(start_idx + batch_size, state->body_count);

        // Load batch of data
        v4f pos_x = {0}, pos_y = {0}, pos_z = {0};
        v4f vel_x = {0}, vel_y = {0}, vel_z = {0};
        v4f force_x = {0}, force_y = {0}, force_z = {0};
        v4f inv_mass = {0};

        for (u32 i = start_idx; i < end_idx; i++) {
            u32 lane = i - start_idx;
            if (state->active_mask[i]) {
                pos_x.f[lane] = state->pos_x[i];
                pos_y.f[lane] = state->pos_y[i];
                pos_z.f[lane] = state->pos_z[i];
                vel_x.f[lane] = state->vel_x[i];
                vel_y.f[lane] = state->vel_y[i];
                vel_z.f[lane] = state->vel_z[i];
                force_x.f[lane] = state->force_x[i];
                force_y.f[lane] = state->force_y[i];
                force_z.f[lane] = state->force_z[i];
                inv_mass.f[lane] = state->inv_masses[i];
            }
        }

        // Compute acceleration: a = F * inv_mass
        v4f accel_x = v4f_mul(&force_x, &inv_mass);
        v4f accel_y = v4f_mul(&force_y, &inv_mass);
        v4f accel_z = v4f_mul(&force_z, &inv_mass);

        // Apply gravity
        v4f gravity = {.f = {0, -9.81f, 0, -9.81f}};
        accel_y = v4f_add(&accel_y, &gravity);

        // Update velocity: v += a * dt
        v4f dt_vec = {.f = {dt, dt, dt, dt}};
        v4f delta_vel_x = v4f_mul(&accel_x, &dt_vec);
        v4f delta_vel_y = v4f_mul(&accel_y, &dt_vec);
        v4f delta_vel_z = v4f_mul(&accel_z, &dt_vec);

        vel_x = v4f_add(&vel_x, &delta_vel_x);
        vel_y = v4f_add(&vel_y, &delta_vel_y);
        vel_z = v4f_add(&vel_z, &delta_vel_z);

        // Update position: p += v * dt
        v4f delta_pos_x = v4f_mul(&vel_x, &dt_vec);
        v4f delta_pos_y = v4f_mul(&vel_y, &dt_vec);
        v4f delta_pos_z = v4f_mul(&vel_z, &dt_vec);

        pos_x = v4f_add(&pos_x, &delta_pos_x);
        pos_y = v4f_add(&pos_y, &delta_pos_y);
        pos_z = v4f_add(&pos_z, &delta_pos_z);

        // Store back
        for (u32 i = start_idx; i < end_idx; i++) {
            u32 lane = i - start_idx;
            if (state->active_mask[i]) {
                state->pos_x[i] = pos_x.f[lane];
                state->pos_y[i] = pos_y.f[lane];
                state->pos_z[i] = pos_z.f[lane];
                state->vel_x[i] = vel_x.f[lane];
                state->vel_y[i] = vel_y.f[lane];
                state->vel_z[i] = vel_z.f[lane];
            }
        }
    }

    LOG_DEBUG("PHYSICS", "SIMD position integration: %u bodies processed", state->body_count);
}

// Clear force/torque accumulators
static void physics_simd_clear_forces(PhysicsSimdState *state) {
    memset(state->force_x, 0, state->body_count * sizeof(f32));
    memset(state->force_y, 0, state->body_count * sizeof(f32));
    memset(state->force_z, 0, state->body_count * sizeof(f32));
    memset(state->torque_x, 0, state->body_count * sizeof(f32));
    memset(state->torque_y, 0, state->body_count * sizeof(f32));
    memset(state->torque_z, 0, state->body_count * sizeof(f32));
}

// Public API to integrate physics with SIMD acceleration
void physics_integrate_simd(void *physics_world, f32 dt) {
    // TODO: Cast physics_world to PhysicsWorld struct
    // TODO: Convert from AoS (Array of Structures) to SoA (Structure of Arrays)
    // TODO: Call physics_simd_integrate_positions()
    // TODO: Convert back from SoA to AoS

    // Stub implementation - proper integration requires restructuring physics world
    LOG_DEBUG("PHYSICS", "SIMD integration step (dt=%.3f)", dt);
}

// Enable physics SIMD for a batch of bodies
void physics_enable_simd_batch(void *physics_world, u32 *body_indices, u32 count) {
    // TODO: Create batch in SIMD state
    // TODO: Copy data from physics world into SIMD buffers
    LOG_DEBUG("PHYSICS", "Enabled SIMD batch for %u bodies", count);
}
