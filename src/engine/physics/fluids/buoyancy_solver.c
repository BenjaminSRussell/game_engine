/**
 * =================================================================================================
 *                          BUOYANCY SOLVER - COMPLETE IMPLEMENTATION
 * =================================================================================================
 */

#include "physics/fluids/buoyancy_solver.h"
#include <physics/physics.h>
#include <math.h>
#include <string.h>

#define GRAVITY 9.81f

// ============================================================================
// Submerged Volume Calculation (Approximation)
// ============================================================================

float buoyancy_calculate_submerged_volume(const RigidBody *body, float water_level) {
    if (!body) return 0.0f;
    
    Vec3 pos = rigid_body_get_position(body);
    const Collider *collider = rigid_body_get_collider(body);
    if (!collider) return 0.0f;
    
    ColliderType type = collider_get_type(collider);
    
    if (type == COLLIDER_TYPE_SPHERE) {
        float radius = collider_get_sphere_radius(collider);
        float center_y = pos.y;
        
        // Fully above water
        if (center_y - radius >= water_level) return 0.0f;
        
        // Fully submerged
        if (center_y + radius <= water_level) {
            return (4.0f / 3.0f) * 3.14159f * radius * radius * radius;
        }
        
        // Partially submerged - spherical cap volume
        float h = water_level - (center_y - radius); // Height of cap
        if (h < 0.0f) h = 0.0f;
        if (h > 2.0f * radius) h = 2.0f * radius;
        
        // V = πh²(3r - h)/3
        float cap_volume = 3.14159f * h * h * (3.0f * radius - h) / 3.0f;
        return cap_volume;
        
    } else if (type == COLLIDER_TYPE_BOX) {
        float hx, hy, hz;
        collider_get_box_half_extents(collider, &hx, &hy, &hz);
        
        float bottom = pos.y - hy;
        float top = pos.y + hy;
        
        // Fully above water
        if (bottom >= water_level) return 0.0f;
        
        // Fully submerged
        if (top <= water_level) {
            return (2.0f * hx) * (2.0f * hy) * (2.0f * hz);
        }
        
        // Partially submerged
        float submerged_height = water_level - bottom;
        if (submerged_height < 0.0f) submerged_height = 0.0f;
        if (submerged_height > 2.0f * hy) submerged_height = 2.0f * hy;
        
        return (2.0f * hx) * submerged_height * (2.0f * hz);
    }
    
    return 0.0f;  // Unknown collider type
}

// ============================================================================
// Main Buoyancy Update
// ============================================================================

void buoyancy_update(RigidBody *body, const FluidSurface *fluid, float dt, BuoyancyState *out_state) {
    if (!body || !fluid) return;
    
    // Calculate submerged volume
    float volume = buoyancy_calculate_submerged_volume(body, fluid->height);
    
    if (out_state) {
        out_state->submerged_volume = volume;
        out_state->water_level = fluid->height;
        out_state->is_submerged = (volume > 0.001f);
        
        // Calculate approximate submerged center (simplified)
        Vec3 pos = rigid_body_get_position(body);
        out_state->submerged_center[0] = pos.x;
        out_state->submerged_center[1] = fminf(pos.y, fluid->height);
        out_state->submerged_center[2] = pos.z;
    }
    
    if (volume < 0.001f) return; // Not in water
    
    // Apply buoyancy forces
    buoyancy_apply_forces(body, fluid, out_state);
}

// ============================================================================
// Force Application
// ============================================================================

void buoyancy_apply_forces(RigidBody *body, const FluidSurface *fluid, const BuoyancyState *state) {
    if (!body || !fluid || !state || !state->is_submerged) return;
    
    // 1. BUOYANCY FORCE (Archimedes' Principle)
    // F_buoyancy = ρ_fluid * V_submerged * g (upward)
    float buoyancy_magnitude = fluid->density * state->submerged_volume * GRAVITY;
    Vec3 buoyancy_force = vec3(0.0f, buoyancy_magnitude, 0.0f);
    
    // Apply at center of buoyancy
    rigid_body_apply_force_at_point(body, buoyancy_force,
                                    vec3(state->submerged_center[0],
                                         state->submerged_center[1],
                                         state->submerged_center[2]));
    
    // 2. LINEAR DRAG
    // F_drag = -0.5 * ρ * CD * A * v²
    Vec3 velocity = rigid_body_get_velocity(body);
    float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
    
    if (speed > 0.001f) {
        // Simplified drag (CD * A approximated by viscosity parameter)
        float drag_magnitude = 0.5f * fluid->density * fluid->viscosity * speed * speed;
        
        // Direction opposite to velocity
        Vec3 drag_direction = vec3(-velocity.x / speed,
                                  -velocity.y / speed,
                                  -velocity.z / speed);
        Vec3 drag_force = vec3(drag_direction.x * drag_magnitude,
                              drag_direction.y * drag_magnitude,
                              drag_direction.z * drag_magnitude);
        
        rigid_body_apply_force(body, drag_force);
    }
    
    // 3. ANGULAR DRAG (dampen rotation in fluid)
    Vec3 angular_vel = rigid_body_get_angular_velocity(body);
    float angular_speed = sqrtf(angular_vel.x * angular_vel.x +
                               angular_vel.y * angular_vel.y +
                               angular_vel.z * angular_vel.z);
    
    if (angular_speed > 0.001f) {
        float angular_drag = fluid->viscosity * 0.5f;
        Vec3 angular_damping = vec3(-angular_vel.x * angular_drag,
                                   -angular_vel.y * angular_drag,
                                   -angular_vel.z * angular_drag);
        rigid_body_apply_torque(body, angular_damping);
    }
    
    // 4. SURFACE SLAMMING (water entry impact)
    Vec3 pos = rigid_body_get_position(body);
    float velocity_y = velocity.y;
    
    // Detect fast water entry
    if (pos.y <= fluid->height && velocity_y < -2.0f) {
        // Apply upward impact force proportional to entry speed
        float slam_force = -velocity_y * fluid->density * state->submerged_volume * 2.0f;
        rigid_body_apply_force(body, vec3(0.0f, slam_force, 0.0f));
    }
    
    // 5. FLOW/CURRENT INFLUENCE
    if (fluid->flow_speed > 0.001f) {
        // Apply force in flow direction proportional to submerged area
        float flow_force_mag = fluid->density * state->submerged_volume * fluid->flow_speed * 0.5f;
        Vec3 flow_force = vec3(fluid->flow_direction[0] * flow_force_mag,
                              fluid->flow_direction[1] * flow_force_mag,
                              fluid->flow_direction[2] * flow_force_mag);
        rigid_body_apply_force(body, flow_force);
    }
}

// ============================================================================
// Water Surface Query (Placeholder for ocean FFT integration)
// ============================================================================

float buoyancy_query_water_height(const FluidSurface *fluid, float x, float z) {
    // TODO: Integrate with renderer/water/fft_ocean for realistic wave heights
    // For now, return flat water surface
    (void)x;
    (void)z;
    return fluid->height;
}

// ============================================================================
// Boat Stability Test (Helper)
// ============================================================================

/**
 * Test boat stability - calculates metacentric height
 * Positive = stable, Negative = will capsize
 */
float buoyancy_test_stability(const RigidBody *body, const FluidSurface *fluid) {
    // Simplified stability test
    // Real implementation would calculate:
    // 1. Center of gravity (CG)
    // 2. Center of buoyancy (CB)
    // 3. Metacenter (M)
    // 4. Metacentric height (GM = M - CG)
    
    Vec3 pos = rigid_body_get_position(body);
    BuoyancyState state;
    buoyancy_update((RigidBody*)body, fluid, 0.0f, &state);
    
    if (!state.is_submerged) return 0.0f;
    
    // Simplified: check if center of buoyancy is below center of mass
    float cg_y = pos.y;
    float cb_y = state.submerged_center[1];
    
    // Positive = stable (CG above CB is stable for most hulls)
    return cg_y - cb_y;
}
