// Suspension Bridge - Complete implementation (7 TODOs)
#include "physics/vehicles/suspension_bridge.h"
#include <physics/physics.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    Vec3 position;
    Vec3 velocity;
    float mass;
    bool fixed;  // Anchor points
} BridgeSegment;

typedef struct {
    BridgeSegment *segments;
    uint32_t segment_count;
    float cable_stiffness;
    float cable_damping;
    float max_tension;
    
    // Wind
    Vec3 wind_velocity;
    float wind_strength;
    
    // Sound
    float stress_level; // 0-1, for creaking sounds
} SuspensionBridge;

Suspension Bridge *bridge_create(Vec3 start, Vec3 end, uint32_t segments, float cable_stiffness) {
    SuspensionBridge *bridge = malloc(sizeof(SuspensionBridge));
    bridge->segments = calloc(segments, sizeof(BridgeSegment));
    bridge->segment_count = segments;
    bridge->cable_stiffness = cable_stiffness;
    bridge->cable_damping = 50.0f;
    bridge->max_tension = 10000.0f;
    bridge->stress_level = 0.0f;
    
    // Generate chain segments
    for (uint32_t i = 0; i < segments; i++) {
        float t = (float)i / (segments - 1);
        bridge->segments[i].position = vec3_lerp(start, end, t);
        bridge->segments[i].velocity = vec3(0, 0, 0);
        bridge->segments[i].mass = 10.0f;
        bridge->segments[i].fixed = (i == 0 || i == segments - 1);
    }
    
    return bridge;
}

void bridge_update(SuspensionBridge *bridge, float dt) {
    float max_stress = 0.0f;
    
    for (uint32_t i = 0; i < bridge->segment_count; i++) {
        if (bridge->segments[i].fixed) continue;
        
        BridgeSegment *seg = &bridge->segments[i];
        Vec3 force = vec3(0, -9.81f * seg->mass, 0); // Gravity
        
        // Spring forces to neighbors
        if (i > 0) {
            Vec3 to_prev = vec3_sub(bridge->segments[i-1].position, seg->position);
            float dist = vec3_length(to_prev);
            float tension = (dist - 1.0f) * bridge->cable_stiffness;
            force = vec3_add(force, vec3_scale(vec3_normalize(to_prev), tension));
            max_stress = fmaxf(max_stress, fabsf(tension) / bridge->max_tension);
        }
        if (i < bridge->segment_count - 1) {
            Vec3 to_next = vec3_sub(bridge->segments[i+1].position, seg->position);
            float dist = vec3_length(to_next);
            float tension = (dist - 1.0f) * bridge->cable_stiffness;
            force = vec3_add(force, vec3_scale(vec3_normalize(to_next), tension));
        }
        
        // Wind influence
        Vec3 wind_force = vec3_scale(bridge->wind_velocity, bridge->wind_strength * 0.1f);
        force = vec3_add(force, wind_force);
        
        // Damping
        force = vec3_sub(force, vec3_scale(seg->velocity, bridge->cable_damping));
        
        // Integrate
        Vec3 accel = vec3_scale(force, 1.0f / seg->mass);
        seg->velocity = vec3_add(seg->velocity, vec3_scale(accel, dt));
        seg->position = vec3_add(seg->position, vec3_scale(seg->velocity, dt));
        
        // Breakage check
        if (max_stress > 1.0f) {
            // Bridge breaks - mark segments as free-falling
            seg->fixed = false;
        }
    }
    
    bridge->stress_level = max_stress;
}

float bridge_get_stress(const SuspensionBridge *bridge) {
    return bridge->stress_level; // For creaking sound emission
}
