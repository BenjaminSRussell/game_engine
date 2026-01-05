// include/physics/quality.h
//
// Purpose: Defines the public API for physics quality management in Minecraft v2.
// This header provides functions for managing physics performance targets,
// quality levels, and acceptance criteria validation.
//
// Public APIs:
// - `physics_quality_init`: Initializes the physics quality system.
// - `physics_quality_update_metrics`: Updates performance metrics for monitoring.
// - `physics_quality_set_level`: Sets a specific physics quality level.
// - `physics_quality_validate`: Validates physics against acceptance criteria.
// - `physics_quality_get_metrics`: Returns current quality metrics.
// - `physics_quality_set_auto_adjust`: Enables/disables automatic quality adjustment.
// - `physics_quality_get_targets`: Returns current quality targets.
// - `physics_quality_get_criteria`: Returns acceptance criteria.
// - `physics_quality_cleanup`: Destroys quality system resources.
//
// Ownership: The physics quality system manages its own state and should be
// initialized once at physics system startup and cleaned up at shutdown.
//
// Invariants:
// - Physics quality system must be initialized before calling other functions.
// - Quality levels range from LOW to ULTRA with corresponding performance targets.
// - All time values are in milliseconds, frequencies in Hz.
//
#ifndef PHYSICS_QUALITY_H
#define PHYSICS_QUALITY_H

#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

// Physics quality levels
typedef enum {
    PHYSICS_QUALITY_LOW,      // 30 FPS target, reduced accuracy
    PHYSICS_QUALITY_MEDIUM,   // 60 FPS target, balanced quality
    PHYSICS_QUALITY_HIGH,     // 60 FPS target, high accuracy
    PHYSICS_QUALITY_ULTRA     // 60 FPS target, maximum accuracy
} PhysicsQuality;

// Physics quality targets
typedef struct {
    f32 target_frame_time_ms;     // Maximum allowed frame time
    f32 max_physics_time_ms;       // Maximum time for physics update
    u32 max_active_bodies;         // Maximum active rigid bodies
    u32 max_collision_tests;        // Maximum collision tests per frame
    f32 min_simulation_frequency;  // Minimum physics simulation frequency (Hz)
    bool enable_sleeping;          // Enable body sleeping for performance
    bool enable_speculative_contacts; // Enable speculative contact generation
    f32 max_penetration_depth;     // Maximum allowed penetration before correction
    u32 max_solver_iterations;     // Maximum constraint solver iterations
} PhysicsQualityTargets;

// Physics acceptance criteria
typedef struct {
    f32 max_position_error;         // Maximum position deviation (meters)
    f32 max_velocity_error;         // Maximum velocity deviation (m/s)
    f32 max_energy_drift;           // Maximum energy system drift per second
    f32 max_angular_error;          // Maximum angular deviation (radians)
    u32 min_stability_frames;       // Minimum frames before body considered stable
    f32 max_restitution_error;       // Maximum restitution coefficient error
    f32 max_friction_error;          // Maximum friction coefficient error
    bool require_deterministic_replay; // Must support deterministic replay
    u32 max_replay_hash_mismatches; // Maximum allowed hash mismatches in replay
} PhysicsAcceptanceCriteria;

// Physics quality metrics
typedef struct {
    PhysicsQuality current_quality;
    f32 average_frame_time_ms;
    f32 worst_frame_time_ms;
    f32 target_frame_time_ms;
    f32 violation_rate;              // Percentage of frames with violations
    bool validation_passed;
} PhysicsQualityMetrics;

// Initialize physics quality system
bool physics_quality_init(PhysicsQuality initial_quality);

// Update performance metrics
void physics_quality_update_metrics(f32 frame_time_ms, f32 physics_time_ms, 
                                   u32 active_bodies, u32 collision_tests);

// Set physics quality level
void physics_quality_set_level(PhysicsQuality quality);

// Validate physics system against acceptance criteria
bool physics_quality_validate(void);

// Get current quality metrics
PhysicsQualityMetrics physics_quality_get_metrics(void);

// Enable/disable auto-adjustment
void physics_quality_set_auto_adjust(bool enabled);

// Set adjustment threshold (0.01 to 1.0)
void physics_quality_set_adjustment_threshold(f32 threshold);

// Get current quality targets
const PhysicsQualityTargets* physics_quality_get_targets(void);

// Get acceptance criteria
const PhysicsAcceptanceCriteria* physics_quality_get_criteria(void);

// Cleanup physics quality system
void physics_quality_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // PHYSICS_QUALITY_H
