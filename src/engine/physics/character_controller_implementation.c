// character_controller_implementation.c
// Character Controller Implementation

#include "physics.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// -----------------------------------------------------------------------------
// Character Controller Implementation
// -----------------------------------------------------------------------------

CharacterController* character_controller_create(PhysicsWorld* world, Vec3 position) {
    if (!world) {
        LOG_ERROR("Cannot create character controller without physics world");
        return NULL;
    }
    
    CharacterController* controller = malloc(sizeof(CharacterController));
    if (!controller) {
        LOG_ERROR("Failed to allocate character controller");
        return NULL;
    }
    
    memset(controller, 0, sizeof(CharacterController));
    
    // Create rigid body for character
    controller->body = rigid_body_create(BODY_TYPE_DYNAMIC, position);
    if (!controller->body) {
        LOG_ERROR("Failed to create rigid body for character controller");
        free(controller);
        return NULL;
    }
    
    // Set up character properties
    rigid_body_set_mass(controller->body, 70.0f); // Average human mass
    rigid_body_set_linear_damping(controller->body, 0.1f);
    rigid_body_set_angular_damping(controller->body, 0.5f);
    
    // Create capsule collider for character
    Collider* collider = malloc(sizeof(Collider));
    if (!collider) {
        LOG_ERROR("Failed to allocate collider for character controller");
        rigid_body_destroy(controller->body);
        free(controller);
        return NULL;
    }
    
    memset(collider, 0, sizeof(Collider));
    collider->type = COLLIDER_TYPE_CAPSULE;
    collider->shape.capsule.radius = 0.4f;
    collider->shape.capsule.height = 1.8f;
    collider->material.friction = 0.8f;
    collider->material.restitution = 0.0f;
    
    rigid_body_attach_collider(controller->body, collider);
    physics_world_add_body(world, controller->body);
    
    // Initialize controller state
    controller->step_height = 0.5f;
    controller->slope_limit = 45.0f * M_PI / 180.0f; // Convert to radians
    controller->move_speed = 5.0f;
    controller->jump_height = 2.0f;
    controller->air_control = 0.3f;
    controller->is_grounded = false;
    controller->ground_normal = (Vec3){0, 1, 0};
    controller->move_direction = (Vec3){0, 0, 0};
    controller->jump_requested = false;
    
    LOG_INFO("Character controller created at position: (%.2f, %.2f, %.2f)", 
             position.x, position.y, position.z);
    return controller;
}

void character_controller_destroy(CharacterController* controller) {
    if (!controller) return;
    
    if (controller->body) {
        // Remove from physics world and destroy
        // Note: We don't have direct access to world here, so just destroy the body
        rigid_body_destroy(controller->body);
    }
    
    free(controller);
    LOG_INFO("Character controller destroyed");
}

void character_controller_set_move_speed(CharacterController* controller, f32 speed) {
    if (!controller) return;
    controller->move_speed = fmaxf(0.0f, speed);
    LOG_DEBUG("Character controller move speed set to: %.2f", speed);
}

void character_controller_set_jump_height(CharacterController* controller, f32 height) {
    if (!controller) return;
    controller->jump_height = fmaxf(0.0f, height);
    LOG_DEBUG("Character controller jump height set to: %.2f", height);
}

void character_controller_set_step_height(CharacterController* controller, f32 height) {
    if (!controller) return;
    controller->step_height = fmaxf(0.0f, height);
    LOG_DEBUG("Character controller step height set to: %.2f", height);
}

void character_controller_set_slope_limit(CharacterController* controller, f32 angle_degrees) {
    if (!controller) return;
    controller->slope_limit = angle_degrees * M_PI / 180.0f;
    LOG_DEBUG("Character controller slope limit set to: %.2f degrees", angle_degrees);
}

void character_controller_set_air_control(CharacterController* controller, f32 air_control) {
    if (!controller) return;
    controller->air_control = clamp_f32(air_control, 0.0f, 1.0f);
    LOG_DEBUG("Character controller air control set to: %.2f", air_control);
}

bool character_controller_is_grounded(CharacterController* controller) {
    if (!controller) return false;
    return controller->is_grounded;
}

Vec3 character_controller_get_velocity(CharacterController* controller) {
    if (!controller || !controller->body) return (Vec3){0, 0, 0};
    return rigid_body_get_velocity(controller->body);
}

Vec3 character_controller_get_position(CharacterController* controller) {
    if (!controller || !controller->body) return (Vec3){0, 0, 0};
    return rigid_body_get_position(controller->body);
}

void character_controller_move(CharacterController* controller, PhysicsWorld* world, 
                           Vec3 direction, f32 delta_time) {
    if (!controller || !world || !controller->body) return;
    
    // Normalize movement direction
    f32 length = sqrtf(direction.x * direction.x + direction.z * direction.z);
    if (length > 0.001f) {
        controller->move_direction.x = direction.x / length;
        controller->move_direction.z = direction.z / length;
        controller->move_direction.y = 0.0f; // No vertical movement from input
    } else {
        controller->move_direction = (Vec3){0, 0, 0};
    }
    
    // Calculate movement velocity
    Vec3 current_velocity = rigid_body_get_velocity(controller->body);
    Vec3 target_velocity = vec3_multiply(&controller->move_direction, controller->move_speed);
    
    // Apply air control if not grounded
    if (!controller->is_grounded) {
        target_velocity.x = current_velocity.x + (target_velocity.x - current_velocity.x) * controller->air_control;
        target_velocity.z = current_velocity.z + (target_velocity.z - current_velocity.z) * controller->air_control;
    } else {
        // Ground movement - directly set horizontal velocity
        target_velocity.y = current_velocity.y; // Preserve vertical velocity
    }
    
    rigid_body_set_velocity(controller->body, target_velocity);
}

void character_controller_jump(CharacterController* controller, PhysicsWorld* world) {
    if (!controller || !world || !controller->body) return;
    
    if (controller->is_grounded) {
        // Calculate jump velocity from jump height
        // v = sqrt(2 * g * h)
        f32 gravity = 9.81f;
        f32 jump_velocity = sqrtf(2.0f * gravity * controller->jump_height);
        
        Vec3 current_velocity = rigid_body_get_velocity(controller->body);
        current_velocity.y = jump_velocity;
        rigid_body_set_velocity(controller->body, current_velocity);
        
        controller->is_grounded = false;
        controller->jump_requested = false;
        
        LOG_DEBUG("Character controller jump: velocity %.2f", jump_velocity);
    }
}

void character_controller_update(CharacterController* controller, PhysicsWorld* world, f32 delta_time) {
    if (!controller || !world || !controller->body) return;
    
    // Perform ground check
    character_controller_perform_ground_check(controller, world);
    
    // Handle jump request
    if (controller->jump_requested) {
        character_controller_jump(controller, world);
    }
    
    // Apply gravity if not grounded
    if (!controller->is_grounded) {
        Vec3 current_velocity = rigid_body_get_velocity(controller->body);
        current_velocity.y -= 9.81f * delta_time; // Apply gravity
        rigid_body_set_velocity(controller->body, current_velocity);
    }
    
    // Update position and rotation
    Vec3 position = rigid_body_get_position(controller->body);
    
    // Keep character upright (zero out rotation)
    Quat upright = quat_identity();
    rigid_body_set_rotation(controller->body, upright);
}

void character_controller_teleport(CharacterController* controller, Vec3 position) {
    if (!controller || !controller->body) return;
    
    rigid_body_set_position(controller->body, position);
    
    // Reset velocity
    rigid_body_set_velocity(controller->body, (Vec3){0, 0, 0});
    
    // Reset grounded state
    controller->is_grounded = false;
    controller->jump_requested = false;
    
    LOG_INFO("Character controller teleported to: (%.2f, %.2f, %.2f)", 
             position.x, position.y, position.z);
}

void character_controller_reset(CharacterController* controller) {
    if (!controller || !controller->body) return;
    
    // Reset velocity
    rigid_body_set_velocity(controller->body, (Vec3){0, 0, 0});
    rigid_body_clear_forces(controller->body);
    
    // Reset state
    controller->is_grounded = false;
    controller->ground_normal = (Vec3){0, 1, 0};
    controller->move_direction = (Vec3){0, 0, 0};
    controller->jump_requested = false;
    
    // Reset rotation to upright
    Quat upright = quat_identity();
    rigid_body_set_rotation(controller->body, upright);
    
    LOG_INFO("Character controller reset");
}

// -----------------------------------------------------------------------------
// System Management
// -----------------------------------------------------------------------------

static CharacterController** g_controllers = NULL;
static u32 g_controller_count = 0;
static u32 g_controller_capacity = 0;

void character_controller_init_system(void) {
    g_controller_capacity = 64;
    g_controllers = malloc(sizeof(CharacterController*) * g_controller_capacity);
    g_controller_count = 0;
    
    LOG_INFO("Character controller system initialized");
}

void character_controller_cleanup_system(void) {
    if (g_controllers) {
        for (u32 i = 0; i < g_controller_count; i++) {
            character_controller_destroy(g_controllers[i]);
        }
        free(g_controllers);
        g_controllers = NULL;
    }
    
    g_controller_count = 0;
    g_controller_capacity = 0;
    
    LOG_INFO("Character controller system cleaned up");
}

void character_controller_update_all(PhysicsWorld* world, f32 delta_time) {
    if (!world || !g_controllers) return;
    
    for (u32 i = 0; i < g_controller_count; i++) {
        character_controller_update(g_controllers[i], world, delta_time);
    }
}

// -----------------------------------------------------------------------------
// Internal Helper Functions
// -----------------------------------------------------------------------------

static void character_controller_perform_ground_check(CharacterController* controller, PhysicsWorld* world) {
    if (!controller || !world || !controller->body) return;
    
    Vec3 position = rigid_body_get_position(controller->body);
    f32 capsule_radius = 0.4f;
    f32 capsule_height = 1.8f;
    f32 ground_check_distance = capsule_radius * 1.1f;
    
    // Cast ray downward from bottom of capsule
    Vec3 ray_start = (Vec3){position.x, position.y - capsule_height * 0.5f + capsule_radius, position.z};
    Vec3 ray_direction = (Vec3){0, -1, 0};
    
    RaycastResult result = physics_raycast(world, ray_start, ray_direction, ground_check_distance);
    
    if (result.hit) {
        controller->is_grounded = true;
        controller->ground_normal = result.hit_normal;
        
        // Snap to ground if very close
        f32 snap_distance = 0.05f;
        if (result.hit_distance < snap_distance) {
            Vec3 new_position = position;
            new_position.y = result.hit_point.y + capsule_height * 0.5f - capsule_radius + 0.01f;
            rigid_body_set_position(controller->body, new_position);
            
            // Zero out downward velocity
            Vec3 current_velocity = rigid_body_get_velocity(controller->body);
            if (current_velocity.y < 0.0f) {
                current_velocity.y = 0.0f;
                rigid_body_set_velocity(controller->body, current_velocity);
            }
        }
    } else {
        controller->is_grounded = false;
        controller->ground_normal = (Vec3){0, 1, 0};
    }
}

static bool character_controller_register(CharacterController* controller) {
    if (!controller || !g_controllers) return false;
    
    if (g_controller_count >= g_controller_capacity) {
        // Expand capacity
        g_controller_capacity *= 2;
        CharacterController** new_controllers = realloc(g_controllers, 
            sizeof(CharacterController*) * g_controller_capacity);
        if (!new_controllers) {
            LOG_ERROR("Failed to expand character controller array");
            return false;
        }
        g_controllers = new_controllers;
    }
    
    g_controllers[g_controller_count++] = controller;
    return true;
}

static void character_controller_unregister(CharacterController* controller) {
    if (!controller || !g_controllers) return;
    
    for (u32 i = 0; i < g_controller_count; i++) {
        if (g_controllers[i] == controller) {
            // Shift remaining controllers
            memmove(&g_controllers[i], &g_controllers[i + 1],
                sizeof(CharacterController*) * (g_controller_count - i - 1));
            g_controller_count--;
            return;
        }
    }
}

// -----------------------------------------------------------------------------
// Utility Functions
// -----------------------------------------------------------------------------

static f32 clamp_f32(f32 value, f32 min, f32 max) {
    return fmaxf(min, fminf(max, value));
}

static Vec3 vec3_make(f32 x, f32 y, f32 z) {
    return (Vec3){x, y, z};
}

static Vec3 vec3_add(const Vec3* a, const Vec3* b) {
    return (Vec3){a->x + b->x, a->y + b->y, a->z + b->z};
}

static Vec3 vec3_subtract(const Vec3* a, const Vec3* b) {
    return (Vec3){a->x - b->x, a->y - b->y, a->z - b->z};
}

static Vec3 vec3_multiply(const Vec3* v, f32 scalar) {
    return (Vec3){v->x * scalar, v->y * scalar, v->z * scalar};
}

static f32 vec3_dot(const Vec3* a, const Vec3* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static f32 vec3_length(const Vec3* v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

static Vec3 vec3_normalize(const Vec3* v) {
    f32 length = vec3_length(v);
    if (length > 0.0f) {
        return vec3_multiply(v, 1.0f / length);
    }
    return (Vec3){0, 0, 0};
}

static Quat quat_identity(void) {
    return (Quat){0, 0, 0, 1};
}
