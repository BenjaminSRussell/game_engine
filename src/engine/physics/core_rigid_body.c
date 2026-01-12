// core_rigid_body.c - Rigid Body Physics Implementation
#include <include/physics/physics.h>
#include <include/core/logger.h>
#include <include/math/vec3.h>
#include <include/math/quat.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_RIGID_BODIES 1024
#define VALIDATION_EPSILON 1e-6f

typedef struct {
  EntityID entity_id;
  BodyType type;
  Vec3 position;
  Quat rotation;
  Vec3 linear_velocity;
  Vec3 angular_velocity;
  Vec3 force_accumulator;
  Vec3 torque_accumulator;
  f32 mass;
  f32 inverse_mass;
  Mat3 inertia_tensor;
  Mat3 inverse_inertia;
  f32 restitution;
  f32 friction;
  bool is_awake;
  bool is_static;
  u32 collision_layer;
  u32 collision_mask;
} RigidBodyData;

static RigidBodyData g_rigid_bodies[MAX_RIGID_BODIES];
static u32 g_rigid_body_count = 0;

RigidBody* rigid_body_create(EntityID entity, BodyType type) {
  if (g_rigid_body_count >= MAX_RIGID_BODIES) return NULL;
  
  RigidBodyData *data = &g_rigid_bodies[g_rigid_body_count++];
  memset(data, 0, sizeof(RigidBodyData));
  
  data->entity_id = entity;
  data->type = type;
  data->mass = (type == BODY_TYPE_DYNAMIC) ? 1.0f : 0.0f;
  data->inverse_mass = (data->mass > 0.0f) ? 1.0f / data->mass : 0.0f;
  data->restitution = 0.5f;
  data->friction = 0.5f;
  data->is_awake = (type == BODY_TYPE_DYNAMIC);
  data->is_static = (type == BODY_TYPE_STATIC);
  data->collision_layer = 1;
  data->collision_mask = 0xFFFFFFFF;
  
  // Set identity rotation and inertia
  data->rotation = quat_identity();
  data->inertia_tensor = mat3_identity();
  data->inverse_inertia = mat3_identity();
  
  LOG_DEBUG("Created rigid body for entity %d, type %d", entity, type);
  return (RigidBody*)data;
}

void rigid_body_destroy(RigidBody *body) {
  if (!body) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  
  // Find and remove from array
  for (u32 i = 0; i < g_rigid_body_count; i++) {
    if (&g_rigid_bodies[i] == data) {
      // Move last element to this position
      if (i < g_rigid_body_count - 1) {
        g_rigid_bodies[i] = g_rigid_bodies[g_rigid_body_count - 1];
      }
      g_rigid_body_count--;
      break;
    }
  }
  
  LOG_DEBUG("Destroyed rigid body for entity %d", data->entity_id);
}

void rigid_body_set_mass(RigidBody *body, f32 mass) {
  if (!body) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->mass = mass;
  data->inverse_mass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
}

void rigid_body_set_position(RigidBody *body, const Vec3 *position) {
  if (!body || !position) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->position = *position;
}

void rigid_body_set_rotation(RigidBody *body, const Quat *rotation) {
  if (!body || !rotation) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->rotation = *rotation;
}

void rigid_body_set_linear_velocity(RigidBody *body, const Vec3 *velocity) {
  if (!body || !velocity) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->linear_velocity = *velocity;
}

void rigid_body_set_angular_velocity(RigidBody *body, const Vec3 *velocity) {
  if (!body || !velocity) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->angular_velocity = *velocity;
}

void rigid_body_apply_force(RigidBody *body, const Vec3 *force) {
  if (!body || !force || body->type == BODY_TYPE_STATIC) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->force_accumulator = vec3_add(data->force_accumulator, *force);
}

void rigid_body_apply_force_at_point(RigidBody *body, const Vec3 *force, const Vec3 *point) {
  if (!body || !force || !point || body->type == BODY_TYPE_STATIC) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->force_accumulator = vec3_add(data->force_accumulator, *force);
  
  // Calculate torque: r  F
  Vec3 r = vec3_sub(*point, data->position);
  Vec3 torque = vec3_cross(r, *force);
  data->torque_accumulator = vec3_add(data->torque_accumulator, torque);
}

void rigid_body_apply_impulse(RigidBody *body, const Vec3 *impulse) {
  if (!body || !impulse || body->type == BODY_TYPE_STATIC) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->linear_velocity = vec3_add(data->linear_velocity, vec3_scale(*impulse, data->inverse_mass));
}

void rigid_body_apply_impulse_at_point(RigidBody *body, const Vec3 *impulse, const Vec3 *point) {
  if (!body || !impulse || !point || body->type == BODY_TYPE_STATIC) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  
  // Linear impulse
  data->linear_velocity = vec3_add(data->linear_velocity, vec3_scale(*impulse, data->inverse_mass));
  
  // Angular impulse
  Vec3 r = vec3_sub(*point, data->position);
  Vec3 angular_impulse = vec3_cross(r, *impulse);
  Vec3 angular_velocity_change = mat3_mul_vec3(data->inverse_inertia, angular_impulse);
  data->angular_velocity = vec3_add(data->angular_velocity, angular_velocity_change);
}

void rigid_body_integrate_forces(RigidBody *body, f32 delta_time) {
  if (!body || body->type == BODY_TYPE_STATIC) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  
  // Apply gravity
  Vec3 gravity = {0.0f, -9.81f, 0.0f};
  Vec3 gravity_force = vec3_scale(gravity, data->mass);
  data->force_accumulator = vec3_add(data->force_accumulator, gravity_force);
  
  // Integrate forces to get acceleration
  Vec3 acceleration = vec3_scale(data->force_accumulator, data->inverse_mass);
  data->linear_velocity = vec3_add(data->linear_velocity, vec3_scale(acceleration, delta_time));
  
  // Integrate torques to get angular acceleration
  Vec3 angular_acceleration = mat3_mul_vec3(data->inverse_inertia, data->torque_accumulator);
  data->angular_velocity = vec3_add(data->angular_velocity, vec3_scale(angular_acceleration, delta_time));
  
  // Clear accumulators
  data->force_accumulator = (Vec3){0,0,0};
  data->torque_accumulator = (Vec3){0,0,0};
}

void rigid_body_integrate_velocity(RigidBody *body, f32 delta_time) {
  if (!body || body->type == BODY_TYPE_STATIC) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  
  // Integrate velocity to get position
  data->position = vec3_add(data->position, vec3_scale(data->linear_velocity, delta_time));
  
  // Integrate angular velocity to get rotation
  Vec3 angular_velocity_quat = vec3_scale(data->angular_velocity, 0.5f * delta_time);
  Quat rotation_change = quat_from_angular_velocity(&angular_velocity_quat);
  data->rotation = quat_normalize(quat_mul(data->rotation, rotation_change));
}

bool rigid_body_can_collide(const RigidBody *a, const RigidBody *b) {
  if (!a || !b) return false;
  
  const RigidBodyData *data_a = (const RigidBodyData*)a;
  const RigidBodyData *data_b = (const RigidBodyData*)b;
  
  // Check collision layers and masks
  bool layers_match = (data_a->collision_layer & data_b->collision_mask) != 0 &&
                     (data_b->collision_layer & data_a->collision_mask) != 0;
  
  // Static bodies can't collide with other static bodies
  bool not_both_static = !(data_a->is_static && data_b->is_static);
  
  return layers_match && not_both_static;
}

void rigid_body_set_collision_layer(RigidBody *body, u32 layer) {
  if (!body) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->collision_layer = layer;
}

void rigid_body_set_collision_mask(RigidBody *body, u32 mask) {
  if (!body) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->collision_mask = mask;
}

void rigid_body_set_material(RigidBody *body, f32 restitution, f32 friction) {
  if (!body) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->restitution = restitution;
  data->friction = friction;
}

void rigid_body_set_inertia_tensor(RigidBody *body, const Mat3 *inertia) {
  if (!body || !inertia) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->inertia_tensor = *inertia;
  
  // Calculate inverse inertia
  f32 det = mat3_determinant(inertia);
  if (fabsf(det) > 1e-6f) {
    data->inverse_inertia = mat3_scale(mat3_transpose(mat3_adjoint(inertia)), 1.0f / det);
  } else {
    data->inverse_inertia = mat3_identity();
  }
}

void rigid_body_wake_up(RigidBody *body) {
  if (!body || body->type != BODY_TYPE_DYNAMIC) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->is_awake = true;
}

void rigid_body_put_to_sleep(RigidBody *body) {
  if (!body || body->type != BODY_TYPE_DYNAMIC) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  data->is_awake = false;
  data->linear_velocity = (Vec3){0,0,0};
  data->angular_velocity = (Vec3){0,0,0};
}

bool rigid_body_is_awake(const RigidBody *body) {
  if (!body) return false;
  
  const RigidBodyData *data = (const RigidBodyData*)body;
  return data->is_awake;
}

// Getters
EntityID rigid_body_get_entity_id(const RigidBody *body) {
  if (!body) return 0;
  
  const RigidBodyData *data = (const RigidBodyData*)body;
  return data->entity_id;
}

BodyType rigid_body_get_type(const RigidBody *body) {
  if (!body) return BODY_TYPE_STATIC;
  
  const RigidBodyData *data = (const RigidBodyData*)body;
  return data->type;
}

f32 rigid_body_get_mass(const RigidBody *body) {
  if (!body) return 0.0f;
  
  const RigidBodyData *data = (const RigidBodyData*)body;
  return data->mass;
}

Vec3 rigid_body_get_position(const RigidBody *body) {
  if (!body) return (Vec3){0,0,0};
  
  const RigidBodyData *data = (const RigidBodyData*)body;
  return data->position;
}

Quat rigid_body_get_rotation(const RigidBody *body) {
  if (!body) return quat_identity();
  
  const RigidBodyData *data = (const RigidBodyData*)body;
  return data->rotation;
}

Vec3 rigid_body_get_linear_velocity(const RigidBody *body) {
  if (!body) return (Vec3){0,0,0};
  
  const RigidBodyData *data = (const RigidBodyData*)body;
  return data->linear_velocity;
}

Vec3 rigid_body_get_angular_velocity(const RigidBody *body) {
  if (!body) return (Vec3){0,0,0};
  
  const RigidBodyData *data = (const RigidBodyData*)body;
  return data->angular_velocity;
}

f32 rigid_body_get_restitution(const RigidBody *body) {
  if (!body) return 0.0f;
  
  const RigidBodyData *data = (const RigidBodyData*)body;
  return data->restitution;
}

f32 rigid_body_get_friction(const RigidBody *body) {
  if (!body) return 0.0f;
  
  const RigidBodyData *data = (const RigidBodyData*)body;
  return data->friction;
}

// Utility functions
u32 rigid_body_get_count(void) {
  return g_rigid_body_count;
}

RigidBody* rigid_body_get_by_index(u32 index) {
  if (index >= g_rigid_body_count) return NULL;
  
  return (RigidBody*)&g_rigid_bodies[index];
}

RigidBody* rigid_body_get_by_entity(EntityID entity) {
  for (u32 i = 0; i < g_rigid_body_count; i++) {
    if (g_rigid_bodies[i].entity_id == entity) {
      return (RigidBody*)&g_rigid_bodies[i];
    }
  }
  
  return NULL;
}

void rigid_body_update_all(f32 delta_time) {
  for (u32 i = 0; i < g_rigid_body_count; i++) {
    RigidBodyData *data = &g_rigid_bodies[i];
    
    if (data->is_awake && !data->is_static) {
      rigid_body_integrate_forces((RigidBody*)data, delta_time);
      rigid_body_integrate_velocity((RigidBody*)data, delta_time);
    }
  }
}

void rigid_body_clear_all(void) {
  g_rigid_body_count = 0;
  memset(g_rigid_bodies, 0, sizeof(g_rigid_bodies));
}

// Helper functions for common shapes
void rigid_body_set_sphere_inertia(RigidBody *body, f32 radius) {
  if (!body || radius <= 0.0f) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  f32 mass = data->mass;
  f32 inertia = (2.0f / 5.0f) * mass * radius * radius;
  
  Mat3 sphere_inertia = {
    {{inertia, 0.0f, 0.0f},
     {0.0f, inertia, 0.0f},
     {0.0f, 0.0f, inertia}}
  };
  
  rigid_body_set_inertia_tensor(body, &sphere_inertia);
}

void rigid_body_set_box_inertia(RigidBody *body, const Vec3 *dimensions) {
  if (!body || !dimensions) return;
  
  RigidBodyData *data = (RigidBodyData*)body;
  f32 mass = data->mass;
  f32 x = dimensions->x;
  f32 y = dimensions->y;
  f32 z = dimensions->z;
  
  Mat3 box_inertia = {
    {{(1.0f / 12.0f) * mass * (y*y + z*z), 0.0f, 0.0f},
     {0.0f, (1.0f / 12.0f) * mass * (x*x + z*z), 0.0f},
     {0.0f, 0.0f, (1.0f / 12.0f) * mass * (x*x + y*y)}}
  };
  
  rigid_body_set_inertia_tensor(body, &box_inertia);
}

// ============================================================================
// Validation Functions
// ============================================================================

static bool is_finite_vec3(const Vec3* v) {
    return isfinite(v->x) && isfinite(v->y) && isfinite(v->z);
}

static bool is_valid_quat(const Quat* q) {
    return isfinite(q->w) && isfinite(q->x) && isfinite(q->y) && isfinite(q->z) &&
           fabsf(quat_length(q) - 1.0f) < VALIDATION_EPSILON;
}

static bool is_valid_mass(f32 mass) {
    return isfinite(mass) && mass >= 0.0f;
}

bool rigid_body_validate(const RigidBody* body) {
    if (!body) {
        LOG_ERROR("Rigid body validation failed: NULL pointer");
        return false;
    }
    
    const RigidBodyData* data = (const RigidBodyData*)body;
    bool valid = true;
    
    // Validate entity ID
    if (data->entity_id == 0) {
        LOG_ERROR("Rigid body validation failed: Invalid entity ID (0)");
        valid = false;
    }
    
    // Validate position
    if (!is_finite_vec3(&data->position)) {
        LOG_ERROR("Rigid body validation failed: Invalid position (%.3f, %.3f, %.3f)",
                 data->position.x, data->position.y, data->position.z);
        valid = false;
    }
    
    // Validate rotation
    if (!is_valid_quat(&data->rotation)) {
        LOG_ERROR("Rigid body validation failed: Invalid rotation (%.3f, %.3f, %.3f, %.3f)",
                 data->rotation.w, data->rotation.x, data->rotation.y, data->rotation.z);
        valid = false;
    }
    
    // Validate velocities
    if (!is_finite_vec3(&data->linear_velocity)) {
        LOG_ERROR("Rigid body validation failed: Invalid linear velocity");
        valid = false;
    }
    
    if (!is_finite_vec3(&data->angular_velocity)) {
        LOG_ERROR("Rigid body validation failed: Invalid angular velocity");
        valid = false;
    }
    
    // Validate forces and torques
    if (!is_finite_vec3(&data->force_accumulator)) {
        LOG_ERROR("Rigid body validation failed: Invalid force accumulator");
        valid = false;
    }
    
    if (!is_finite_vec3(&data->torque_accumulator)) {
        LOG_ERROR("Rigid body validation failed: Invalid torque accumulator");
        valid = false;
    }
    
    // Validate mass
    if (!is_valid_mass(data->mass)) {
        LOG_ERROR("Rigid body validation failed: Invalid mass (%.3f)", data->mass);
        valid = false;
    }
    
    // Validate inverse mass consistency
    f32 expected_inverse_mass = (data->mass > 0.0f) ? 1.0f / data->mass : 0.0f;
    if (fabsf(data->inverse_mass - expected_inverse_mass) > VALIDATION_EPSILON) {
        LOG_ERROR("Rigid body validation failed: Inverse mass inconsistency");
        valid = false;
    }
    
    // Validate material properties
    if (!isfinite(data->restitution) || data->restitution < 0.0f || data->restitution > 1.0f) {
        LOG_ERROR("Rigid body validation failed: Invalid restitution (%.3f)", data->restitution);
        valid = false;
    }
    
    if (!isfinite(data->friction) || data->friction < 0.0f || data->friction > 1.0f) {
        LOG_ERROR("Rigid body validation failed: Invalid friction (%.3f)", data->friction);
        valid = false;
    }
    
    // Validate body type consistency
    if (data->is_static && data->type != BODY_TYPE_STATIC) {
        LOG_ERROR("Rigid body validation failed: Static flag inconsistency");
        valid = false;
    }
    
    if (!data->is_static && data->type == BODY_TYPE_STATIC) {
        LOG_ERROR("Rigid body validation failed: Type inconsistency for dynamic body");
        valid = false;
    }
    
    // Validate static body properties
    if (data->is_static) {
        if (vec3_length_squared(&data->linear_velocity) > VALIDATION_EPSILON) {
            LOG_ERROR("Rigid body validation failed: Static body has non-zero linear velocity");
            valid = false;
        }
        
        if (vec3_length_squared(&data->angular_velocity) > VALIDATION_EPSILON) {
            LOG_ERROR("Rigid body validation failed: Static body has non-zero angular velocity");
            valid = false;
        }
    }
    
    return valid;
}

bool rigid_body_validate_all(void) {
    bool all_valid = true;
    u32 invalid_count = 0;
    
    for (u32 i = 0; i < g_rigid_body_count; i++) {
        if (!rigid_body_validate((RigidBody*)&g_rigid_bodies[i])) {
            invalid_count++;
            all_valid = false;
        }
    }
    
    if (!all_valid) {
        LOG_ERROR("Rigid body validation failed: %u/%u bodies are invalid", 
                 invalid_count, g_rigid_body_count);
    } else {
        LOG_DEBUG("All %u rigid bodies passed validation", g_rigid_body_count);
    }
    
    return all_valid;
}

void rigid_body_debug_info(const RigidBody* body) {
    if (!body) {
        LOG_ERROR("Cannot debug NULL rigid body");
        return;
    }
    
    const RigidBodyData* data = (const RigidBodyData*)body;
    
    LOG_INFO("=== Rigid Body Debug Info ===");
    LOG_INFO("Entity ID: %u", data->entity_id);
    LOG_INFO("Type: %s", data->is_static ? "Static" : "Dynamic");
    LOG_INFO("Mass: %.3f", data->mass);
    LOG_INFO("Position: (%.3f, %.3f, %.3f)", data->position.x, data->position.y, data->position.z);
    LOG_INFO("Rotation: (%.3f, %.3f, %.3f, %.3f)", 
             data->rotation.w, data->rotation.x, data->rotation.y, data->rotation.z);
    LOG_INFO("Linear Velocity: (%.3f, %.3f, %.3f)", 
             data->linear_velocity.x, data->linear_velocity.y, data->linear_velocity.z);
    LOG_INFO("Angular Velocity: (%.3f, %.3f, %.3f)", 
             data->angular_velocity.x, data->angular_velocity.y, data->angular_velocity.z);
    LOG_INFO("Restitution: %.3f", data->restitution);
    LOG_INFO("Friction: %.3f", data->friction);
    LOG_INFO("Is Awake: %s", data->is_awake ? "Yes" : "No");
    LOG_INFO("Collision Layer: %u", data->collision_layer);
    LOG_INFO("Collision Mask: %u", data->collision_mask);
    LOG_INFO("Validation: %s", rigid_body_validate(body) ? "PASSED" : "FAILED");
    LOG_INFO("==============================");
}

void rigid_body_system_debug_stats(void) {
    LOG_INFO("=== Rigid Body System Statistics ===");
    LOG_INFO("Total Bodies: %u/%u", g_rigid_body_count, MAX_RIGID_BODIES);
    
    u32 dynamic_count = 0;
    u32 static_count = 0;
    u32 awake_count = 0;
    u32 sleeping_count = 0;
    
    for (u32 i = 0; i < g_rigid_body_count; i++) {
        const RigidBodyData* data = &g_rigid_bodies[i];
        
        if (data->is_static) {
            static_count++;
        } else {
            dynamic_count++;
        }
        
        if (data->is_awake) {
            awake_count++;
        } else {
            sleeping_count++;
        }
    }
    
    LOG_INFO("Dynamic Bodies: %u", dynamic_count);
    LOG_INFO("Static Bodies: %u", static_count);
    LOG_INFO("Awake Bodies: %u", awake_count);
    LOG_INFO("Sleeping Bodies: %u", sleeping_count);
    
    // Calculate system-wide energy
    f32 total_kinetic_energy = 0.0f;
    for (u32 i = 0; i < g_rigid_body_count; i++) {
        const RigidBodyData* data = &g_rigid_bodies[i];
        if (!data->is_static && data->is_awake) {
            f32 linear_ke = 0.5f * data->mass * vec3_length_squared(&data->linear_velocity);
            // Angular kinetic energy would require inertia tensor, simplified here
            total_kinetic_energy += linear_ke;
        }
    }
    
    LOG_INFO("Total Kinetic Energy: %.3f J", total_kinetic_energy);
    LOG_INFO("System Validation: %s", rigid_body_validate_all() ? "PASSED" : "FAILED");
    LOG_INFO("===================================");
}
