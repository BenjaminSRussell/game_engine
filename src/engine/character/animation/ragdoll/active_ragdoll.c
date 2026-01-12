#include "character/ragdoll/ragdoll_physics.h"
#include "core/common/memory/allocator.h"
#include "core/logger.h"
#include "physics/physics.h"
#include <string.h>
#include <stdlib.h>

#define MAX_RAGDOLLS 128
#define MAX_RAGDOLL_BONES 64
#define MAX_RAGDOLL_CONSTRAINTS 128
#define RAGDOLL_BLEND_TIME 2.0f
#define RAGDOLL_WAKE_UP_DISTANCE 0.5f

typedef enum ragdoll_state {
    RAGDOLL_STATE_ANIMATED = 0,
    RAGDOLL_STATE_BLENDING,
    RAGDOLL_STATE_PHYSICS,
    RAGDOLL_STATE_BLEND_BACK
} ragdoll_state_t;

typedef struct ragdoll_bone {
    uint32_t bone_id;
    uint32_t physics_body_id;
    
    vec3_t local_position;
    quat_t local_rotation;
    vec3_t world_position;
    quat_t world_rotation;
    
    float mass;
    vec3_t inertia;
    float damping;
    float angular_damping;
    
    vec3_t velocity;
    vec3_t angular_velocity;
    
    vec3_t accumulated_force;
    vec3_t accumulated_torque;
    
    bool is_dynamic;
    bool is_kinematic;
    bool collision_enabled;
} ragdoll_bone_t;

typedef struct ragdoll_constraint {
    uint32_t constraint_id;
    uint32_t bone_a_id;
    uint32_t bone_b_id;
    
    vec3_t local_anchor_a;
    vec3_t local_anchor_b;
    
    vec3_t local_axis_a;
    vec3_t local_axis_b;
    
    float min_distance;
    float max_distance;
    float min_angle;
    float max_angle;
    
    float stiffness;
    float damping;
    
    bool enabled;
    bool breakable;
    float break_force;
    float break_torque;
} ragdoll_constraint_t;

typedef struct ragdoll_instance {
    uint32_t ragdoll_id;
    uint32_t entity_id;
    uint32_t skeleton_id;
    
    ragdoll_bone_t bones[MAX_RAGDOLL_BONES];
    uint32_t bone_count;
    
    ragdoll_constraint_t constraints[MAX_RAGDOLL_CONSTRAINTS];
    uint32_t constraint_count;
    
    ragdoll_state_t state;
    float blend_factor;
    float blend_time;
    
    vec3_t center_of_mass;
    vec3_t linear_velocity;
    vec3_t angular_velocity;
    
    float total_mass;
    bool auto_sleep;
    bool is_sleeping;
    float sleep_timer;
    
    uint32_t creation_time;
    uint32_t last_update_time;
} ragdoll_instance_t;

typedef struct ragdoll_system {
    ragdoll_instance_t ragdolls[MAX_RAGDOLLS];
    uint32_t ragdoll_count;
    
    uint32_t next_ragdoll_id;
    uint32_t next_constraint_id;
    
    float gravity;
    float time_scale;
    float max_velocity;
    
    bool initialized;
} ragdoll_system_t;

static ragdoll_system_t g_ragdoll_system = {0};

// Forward declarations
static ragdoll_instance_t* get_ragdoll(uint32_t ragdoll_id);
static ragdoll_bone_t* get_ragdoll_bone(ragdoll_instance_t* ragdoll, uint32_t bone_id);
static void update_ragdoll_physics(ragdoll_instance_t* ragdoll, float delta_time);
static void solve_ragdoll_constraints(ragdoll_instance_t* ragdoll);
static void blend_to_animation(ragdoll_instance_t* ragdoll, float delta_time);
static void blend_to_physics(ragdoll_instance_t* ragdoll, float delta_time);
static bool should_sleep(ragdoll_instance_t* ragdoll);
static void wake_up_ragdoll(ragdoll_instance_t* ragdoll);

bool ragdoll_init(void) {
    if (g_ragdoll_system.initialized) {
        return true;
    }
    
    memset(&g_ragdoll_system, 0, sizeof(g_ragdoll_system));
    g_ragdoll_system.next_ragdoll_id = 1;
    g_ragdoll_system.next_constraint_id = 1;
    g_ragdoll_system.gravity = -9.81f;
    g_ragdoll_system.time_scale = 1.0f;
    g_ragdoll_system.max_velocity = 50.0f;
    
    g_ragdoll_system.initialized = true;
    log_info("Ragdoll physics system initialized");
    return true;
}

void ragdoll_shutdown(void) {
    if (!g_ragdoll_system.initialized) {
        return;
    }
    
    // Clean up physics bodies
    for (uint32_t i = 0; i < g_ragdoll_system.ragdoll_count; i++) {
        ragdoll_instance_t* ragdoll = &g_ragdoll_system.ragdolls[i];
        for (uint32_t j = 0; j < ragdoll->bone_count; j++) {
            if (ragdoll->bones[j].physics_body_id != 0) {
                physics_destroy_body(ragdoll->bones[j].physics_body_id);
            }
        }
    }
    
    memset(&g_ragdoll_system, 0, sizeof(g_ragdoll_system));
    log_info("Ragdoll physics system shutdown");
}

uint32_t ragdoll_create(uint32_t entity_id, uint32_t skeleton_id) {
    if (!g_ragdoll_system.initialized || g_ragdoll_system.ragdoll_count >= MAX_RAGDOLLS) {
        return 0;
    }
    
    ragdoll_instance_t* ragdoll = &g_ragdoll_system.ragdolls[g_ragdoll_system.ragdoll_count];
    memset(ragdoll, 0, sizeof(ragdoll_instance_t));
    
    ragdoll->ragdoll_id = g_ragdoll_system.next_ragdoll_id++;
    ragdoll->entity_id = entity_id;
    ragdoll->skeleton_id = skeleton_id;
    ragdoll->state = RAGDOLL_STATE_ANIMATED;
    ragdoll->auto_sleep = true;
    ragdoll->blend_factor = 0.0f;
    
    // Initialize bones from skeleton
    // This would typically query the animation system for bone data
    // For now, create placeholder bones
    ragdoll->bone_count = 16;  // Placeholder
    for (uint32_t i = 0; i < ragdoll->bone_count; i++) {
        ragdoll_bone_t* bone = &ragdoll->bones[i];
        bone->bone_id = i;
        bone->mass = 1.0f;
        bone->damping = 0.1f;
        bone->angular_damping = 0.1f;
        bone->is_dynamic = true;
        bone->collision_enabled = true;
        
        // Create physics body for this bone
        bone->physics_body_id = physics_create_body(PHYSICS_BODY_DYNAMIC);
        if (bone->physics_body_id != 0) {
            physics_body_set_mass(bone->physics_body_id, bone->mass);
            physics_body_set_damping(bone->physics_body_id, bone->damping, bone->angular_damping);
        }
    }
    
    // Create constraints between bones
    // This would create realistic joint constraints based on skeleton structure
    ragdoll->constraint_count = ragdoll->bone_count - 1;
    for (uint32_t i = 0; i < ragdoll->constraint_count; i++) {
        ragdoll_constraint_t* constraint = &ragdoll->constraints[i];
        constraint->constraint_id = g_ragdoll_system.next_constraint_id++;
        constraint->bone_a_id = i;
        constraint->bone_b_id = i + 1;
        constraint->stiffness = 1000.0f;
        constraint->damping = 50.0f;
        constraint->enabled = true;
        constraint->breakable = false;
        
        // Create physics constraint
        physics_create_constraint(PHYSICS_CONSTRAINT_BALL_SOCKET,
                                ragdoll->bones[i].physics_body_id,
                                ragdoll->bones[i + 1].physics_body_id);
    }
    
    // Calculate total mass and center of mass
    ragdoll->total_mass = 0.0f;
    ragdoll->center_of_mass = (vec3_t){0.0f, 0.0f, 0.0f};
    
    for (uint32_t i = 0; i < ragdoll->bone_count; i++) {
        ragdoll->total_mass += ragdoll->bones[i].mass;
        ragdoll->center_of_mass = vec3_add(ragdoll->center_of_mass, 
                                         vec3_scale(ragdoll->bones[i].local_position, ragdoll->bones[i].mass));
    }
    
    if (ragdoll->total_mass > 0.0f) {
        ragdoll->center_of_mass = vec3_scale(ragdoll->center_of_mass, 1.0f / ragdoll->total_mass);
    }
    
    g_ragdoll_system.ragdoll_count++;
    log_debug("Created ragdoll %u for entity %u with %u bones", 
             ragdoll->ragdoll_id, entity_id, ragdoll->bone_count);
    
    return ragdoll->ragdoll_id;
}

bool ragdoll_activate(uint32_t ragdoll_id, float blend_time) {
    ragdoll_instance_t* ragdoll = get_ragdoll(ragdoll_id);
    if (!ragdoll || ragdoll->state != RAGDOLL_STATE_ANIMATED) {
        return false;
    }
    
    ragdoll->state = RAGDOLL_STATE_BLENDING;
    ragdoll->blend_time = blend_time;
    ragdoll->blend_factor = 0.0f;
    ragdoll->last_update_time = get_current_time_ms();
    
    // Enable physics for all bones
    for (uint32_t i = 0; i < ragdoll->bone_count; i++) {
        ragdoll_bone_t* bone = &ragdoll->bones[i];
        if (bone->physics_body_id != 0) {
            physics_body_set_type(bone->physics_body_id, PHYSICS_BODY_DYNAMIC);
            physics_body_set_collision(bone->physics_body_id, true);
        }
    }
    
    log_info("Activated ragdoll %u with blend time %.2f", ragdoll_id, blend_time);
    return true;
}

bool ragdoll_deactivate(uint32_t ragdoll_id, float blend_time) {
    ragdoll_instance_t* ragdoll = get_ragdoll(ragdoll_id);
    if (!ragdoll || ragdoll->state != RAGDOLL_STATE_PHYSICS) {
        return false;
    }
    
    ragdoll->state = RAGDOLL_STATE_BLEND_BACK;
    ragdoll->blend_time = blend_time;
    ragdoll->blend_factor = 0.0f;
    ragdoll->last_update_time = get_current_time_ms();
    
    log_info("Deactivating ragdoll %u with blend time %.2f", ragdoll_id, blend_time);
    return true;
}

void ragdoll_update(float delta_time) {
    if (!g_ragdoll_system.initialized) {
        return;
    }
    
    float scaled_delta_time = delta_time * g_ragdoll_system.time_scale;
    
    for (uint32_t i = 0; i < g_ragdoll_system.ragdoll_count; i++) {
        ragdoll_instance_t* ragdoll = &g_ragdoll_system.ragdolls[i];
        
        switch (ragdoll->state) {
            case RAGDOLL_STATE_ANIMATED:
                // Ragdoll is driven by animation
                break;
                
            case RAGDOLL_STATE_BLENDING:
                blend_to_physics(ragdoll, scaled_delta_time);
                if (ragdoll->blend_factor >= 1.0f) {
                    ragdoll->state = RAGDOLL_STATE_PHYSICS;
                    ragdoll->blend_factor = 1.0f;
                }
                break;
                
            case RAGDOLL_STATE_PHYSICS:
                update_ragdoll_physics(ragdoll, scaled_delta_time);
                
                // Check if ragdoll should sleep
                if (ragdoll->auto_sleep && should_sleep(ragdoll)) {
                    ragdoll->is_sleeping = true;
                    for (uint32_t j = 0; j < ragdoll->bone_count; j++) {
                        if (ragdoll->bones[j].physics_body_id != 0) {
                            physics_body_set_type(ragdoll->bones[j].physics_body_id, PHYSICS_BODY_STATIC);
                        }
                    }
                }
                break;
                
            case RAGDOLL_STATE_BLEND_BACK:
                blend_to_animation(ragdoll, scaled_delta_time);
                if (ragdoll->blend_factor >= 1.0f) {
                    ragdoll->state = RAGDOLL_STATE_ANIMATED;
                    ragdoll->blend_factor = 0.0f;
                    
                    // Disable physics
                    for (uint32_t j = 0; j < ragdoll->bone_count; j++) {
                        if (ragdoll->bones[j].physics_body_id != 0) {
                            physics_body_set_type(ragdoll->bones[j].physics_body_id, PHYSICS_BODY_KINEMATIC);
                        }
                    }
                }
                break;
        }
        
        ragdoll->last_update_time = get_current_time_ms();
    }
}

void ragdoll_apply_force(uint32_t ragdoll_id, uint32_t bone_index, const vec3_t force) {
    ragdoll_instance_t* ragdoll = get_ragdoll(ragdoll_id);
    if (!ragdoll || bone_index >= ragdoll->bone_count) {
        return;
    }
    
    ragdoll_bone_t* bone = &ragdoll->bones[bone_index];
    bone->accumulated_force = vec3_add(bone->accumulated_force, force);
    
    // Wake up ragdoll if sleeping
    if (ragdoll->is_sleeping) {
        wake_up_ragdoll(ragdoll);
    }
}

void ragdoll_apply_impulse(uint32_t ragdoll_id, uint32_t bone_index, const vec3_t impulse) {
    ragdoll_instance_t* ragdoll = get_ragdoll(ragdoll_id);
    if (!ragdoll || bone_index >= ragdoll->bone_count) {
        return;
    }
    
    ragdoll_bone_t* bone = &ragdoll->bones[bone_index];
    
    // Apply impulse to physics body
    if (bone->physics_body_id != 0) {
        physics_body_apply_impulse(bone->physics_body_id, impulse);
    }
    
    // Wake up ragdoll if sleeping
    if (ragdoll->is_sleeping) {
        wake_up_ragdoll(ragdoll);
    }
}

void ragdoll_apply_torque(uint32_t ragdoll_id, uint32_t bone_index, const vec3_t torque) {
    ragdoll_instance_t* ragdoll = get_ragdoll(ragdoll_id);
    if (!ragdoll || bone_index >= ragdoll->bone_count) {
        return;
    }
    
    ragdoll_bone_t* bone = &ragdoll->bones[bone_index];
    bone->accumulated_torque = vec3_add(bone->accumulated_torque, torque);
    
    // Wake up ragdoll if sleeping
    if (ragdoll->is_sleeping) {
        wake_up_ragdoll(ragdoll);
    }
}

bool ragdoll_set_bone_mass(uint32_t ragdoll_id, uint32_t bone_index, float mass) {
    ragdoll_instance_t* ragdoll = get_ragdoll(ragdoll_id);
    if (!ragdoll || bone_index >= ragdoll->bone_count) {
        return false;
    }
    
    ragdoll_bone_t* bone = &ragdoll->bones[bone_index];
    bone->mass = mass;
    
    if (bone->physics_body_id != 0) {
        physics_body_set_mass(bone->physics_body_id, mass);
    }
    
    // Recalculate total mass and center of mass
    ragdoll->total_mass = 0.0f;
    ragdoll->center_of_mass = (vec3_t){0.0f, 0.0f, 0.0f};
    
    for (uint32_t i = 0; i < ragdoll->bone_count; i++) {
        ragdoll->total_mass += ragdoll->bones[i].mass;
        ragdoll->center_of_mass = vec3_add(ragdoll->center_of_mass, 
                                         vec3_scale(ragdoll->bones[i].local_position, ragdoll->bones[i].mass));
    }
    
    if (ragdoll->total_mass > 0.0f) {
        ragdoll->center_of_mass = vec3_scale(ragdoll->center_of_mass, 1.0f / ragdoll->total_mass);
    }
    
    return true;
}

void ragdoll_get_bone_transform(uint32_t ragdoll_id, uint32_t bone_index, vec3_t* position, quat_t* rotation) {
    ragdoll_instance_t* ragdoll = get_ragdoll(ragdoll_id);
    if (!ragdoll || bone_index >= ragdoll->bone_count) {
        return;
    }
    
    ragdoll_bone_t* bone = &ragdoll->bones[bone_index];
    
    if (position) *position = bone->world_position;
    if (rotation) *rotation = bone->world_rotation;
}

void ragdoll_set_auto_sleep(uint32_t ragdoll_id, bool auto_sleep) {
    ragdoll_instance_t* ragdoll = get_ragdoll(ragdoll_id);
    if (!ragdoll) {
        return;
    }
    
    ragdoll->auto_sleep = auto_sleep;
}

void ragdoll_set_gravity(float gravity) {
    if (!g_ragdoll_system.initialized) {
        return;
    }
    
    g_ragdoll_system.gravity = gravity;
    
    // Update physics world gravity
    physics_set_gravity((vec3_t){0.0f, gravity, 0.0f});
}

void ragdoll_get_statistics(uint32_t* active_ragdolls, uint32_t* sleeping_ragdolls, float* average_bones) {
    if (!g_ragdoll_system.initialized) {
        return;
    }
    
    uint32_t active = 0, sleeping = 0;
    float total_bones = 0.0f;
    
    for (uint32_t i = 0; i < g_ragdoll_system.ragdoll_count; i++) {
        const ragdoll_instance_t* ragdoll = &g_ragdoll_system.ragdolls[i];
        
        if (ragdoll->state == RAGDOLL_STATE_PHYSICS) {
            active++;
            if (ragdoll->is_sleeping) {
                sleeping++;
            }
        }
        
        total_bones += ragdoll->bone_count;
    }
    
    if (active_ragdolls) *active_ragdolls = active;
    if (sleeping_ragdolls) *sleeping_ragdolls = sleeping;
    if (average_bones) *average_bones = g_ragdoll_system.ragdoll_count > 0 ? 
                                       total_bones / g_ragdoll_system.ragdoll_count : 0.0f;
}

// Static helper functions
static ragdoll_instance_t* get_ragdoll(uint32_t ragdoll_id) {
    for (uint32_t i = 0; i < g_ragdoll_system.ragdoll_count; i++) {
        if (g_ragdoll_system.ragdolls[i].ragdoll_id == ragdoll_id) {
            return &g_ragdoll_system.ragdolls[i];
        }
    }
    return NULL;
}

static ragdoll_bone_t* get_ragdoll_bone(ragdoll_instance_t* ragdoll, uint32_t bone_id) {
    for (uint32_t i = 0; i < ragdoll->bone_count; i++) {
        if (ragdoll->bones[i].bone_id == bone_id) {
            return &ragdoll->bones[i];
        }
    }
    return NULL;
}

static void update_ragdoll_physics(ragdoll_instance_t* ragdoll, float delta_time) {
    // Apply gravity to all bones
    for (uint32_t i = 0; i < ragdoll->bone_count; i++) {
        ragdoll_bone_t* bone = &ragdoll->bones[i];
        
        if (bone->is_dynamic && bone->physics_body_id != 0) {
            vec3_t gravity_force = (vec3_t){0.0f, g_ragdoll_system.gravity * bone->mass, 0.0f};
            physics_body_apply_force(bone->physics_body_id, gravity_force);
            
            // Apply accumulated forces
            if (vec3_length_sq(bone->accumulated_force) > 0.001f) {
                physics_body_apply_force(bone->physics_body_id, bone->accumulated_force);
                bone->accumulated_force = (vec3_t){0.0f, 0.0f, 0.0f};
            }
            
            // Apply accumulated torques
            if (vec3_length_sq(bone->accumulated_torque) > 0.001f) {
                physics_body_apply_torque(bone->physics_body_id, bone->accumulated_torque);
                bone->accumulated_torque = (vec3_t){0.0f, 0.0f, 0.0f};
            }
        }
    }
    
    // Solve constraints
    solve_ragdoll_constraints(ragdoll);
    
    // Update bone transforms from physics bodies
    for (uint32_t i = 0; i < ragdoll->bone_count; i++) {
        ragdoll_bone_t* bone = &ragdoll->bones[i];
        
        if (bone->physics_body_id != 0) {
            physics_body_get_transform(bone->physics_body_id, &bone->world_position, &bone->world_rotation);
            
            // Clamp velocities
            vec3_t velocity;
            physics_body_get_velocity(bone->physics_body_id, &velocity);
            float speed = vec3_length(velocity);
            if (speed > g_ragdoll_system.max_velocity) {
                velocity = vec3_scale(velocity, g_ragdoll_system.max_velocity / speed);
                physics_body_set_velocity(bone->physics_body_id, velocity);
            }
        }
    }
}

static void solve_ragdoll_constraints(ragdoll_instance_t* ragdoll) {
    // This would solve all constraints to maintain realistic joint behavior
    // For now, this is a placeholder that would call the physics system's constraint solver
}

static void blend_to_animation(ragdoll_instance_t* ragdoll, float delta_time) {
    if (ragdoll->blend_time <= 0.0f) {
        ragdoll->blend_factor = 1.0f;
        return;
    }
    
    ragdoll->blend_factor += delta_time / ragdoll->blend_time;
    if (ragdoll->blend_factor > 1.0f) {
        ragdoll->blend_factor = 1.0f;
    }
    
    // Blend physics transforms back to animation
    for (uint32_t i = 0; i < ragdoll->bone_count; i++) {
        ragdoll_bone_t* bone = &ragdoll->bones[i];
        
        // Get current animation transform (placeholder)
        vec3_t anim_position = bone->local_position;
        quat_t anim_rotation = bone->local_rotation;
        
        // Blend between physics and animation
        bone->world_position = vec3_lerp(bone->world_position, anim_position, ragdoll->blend_factor);
        bone->world_rotation = quat_slerp(bone->world_rotation, anim_rotation, ragdoll->blend_factor);
        
        if (bone->physics_body_id != 0) {
            physics_body_set_transform(bone->physics_body_id, bone->world_position, bone->world_rotation);
        }
    }
}

static void blend_to_physics(ragdoll_instance_t* ragdoll, float delta_time) {
    if (ragdoll->blend_time <= 0.0f) {
        ragdoll->blend_factor = 1.0f;
        return;
    }
    
    ragdoll->blend_factor += delta_time / ragdoll->blend_time;
    if (ragdoll->blend_factor > 1.0f) {
        ragdoll->blend_factor = 1.0f;
    }
    
    // Blend from animation to physics
    for (uint32_t i = 0; i < ragdoll->bone_count; i++) {
        ragdoll_bone_t* bone = &ragdoll->bones[i];
        
        // Get current animation transform (placeholder)
        vec3_t anim_position = bone->local_position;
        quat_t anim_rotation = bone->local_rotation;
        
        // Blend between animation and physics
        vec3_t target_position = bone->world_position;  // Current physics position
        quat_t target_rotation = bone->world_rotation;   // Current physics rotation
        
        bone->world_position = vec3_lerp(anim_position, target_position, ragdoll->blend_factor);
        bone->world_rotation = quat_slerp(anim_rotation, target_rotation, ragdoll->blend_factor);
        
        if (bone->physics_body_id != 0) {
            physics_body_set_transform(bone->physics_body_id, bone->world_position, bone->world_rotation);
        }
    }
}

static bool should_sleep(ragdoll_instance_t* ragdoll) {
    if (!ragdoll->auto_sleep || ragdoll->sleep_timer < 1.0f) {
        return false;
    }
    
    // Check if all bones are moving slowly
    for (uint32_t i = 0; i < ragdoll->bone_count; i++) {
        ragdoll_bone_t* bone = &ragdoll->bones[i];
        
        if (bone->physics_body_id != 0) {
            vec3_t velocity, angular_velocity;
            physics_body_get_velocity(bone->physics_body_id, &velocity);
            physics_body_get_angular_velocity(bone->physics_body_id, &angular_velocity);
            
            if (vec3_length(velocity) > 0.1f || vec3_length(angular_velocity) > 0.1f) {
                return false;
            }
        }
    }
    
    return true;
}

static void wake_up_ragdoll(ragdoll_instance_t* ragdoll) {
    if (!ragdoll->is_sleeping) {
        return;
    }
    
    ragdoll->is_sleeping = false;
    ragdoll->sleep_timer = 0.0f;
    
    // Enable physics for all bones
    for (uint32_t i = 0; i < ragdoll->bone_count; i++) {
        ragdoll_bone_t* bone = &ragdoll->bones[i];
        if (bone->physics_body_id != 0) {
            physics_body_set_type(bone->physics_body_id, PHYSICS_BODY_DYNAMIC);
        }
    }
}

uint32_t get_current_time_ms(void) {
    // Placeholder - would typically use platform-specific time function
    static uint32_t counter = 0;
    return counter += 16;  // Simulate 60 FPS
}
