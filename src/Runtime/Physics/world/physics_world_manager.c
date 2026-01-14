/**
 * @file physics_world_manager.c
 * @brief Physics world manager implementation
 *
 * Central management system for the physics world that coordinates
 * all physics systems, provides unified API, and manages object lifecycles.
 */

#include "physics_world_manager.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>

// ========================================
// Constants
// ========================================

#define DEFAULT_MAX_OBJECTS 10000
#define DEFAULT_MAX_CONSTRAINTS 5000
#define DEFAULT_MAX_CONTACTS 20000
#define DEFAULT_MEMORY_BUDGET (64 * 1024 * 1024) // 64MB
#define MIN_TIME_STEP 0.001f
#define MAX_TIME_STEP 0.033f
#define DEFAULT_SLEEP_THRESHOLD 0.01f
#define DEFAULT_WAKE_THRESHOLD 0.1f

// ========================================
// Utility Functions
// ========================================

static uint64_t get_next_id(void) {
    static uint64_t next_id = 1;
    return next_id++;
}

static float vec3_length(const float *v) {
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static void vec3_normalize(float *result, const float *v) {
    float len = vec3_length(v);
    if (len > 0.0001f) {
        result[0] = v[0] / len;
        result[1] = v[1] / len;
        result[2] = v[2] / len;
    } else {
        result[0] = result[1] = result[2] = 0.0f;
    }
}

static void vec3_cross(float *result, const float *a, const float *b) {
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
}

static float vec3_dot(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void vec3_add(float *result, const float *a, const float *b) {
    result[0] = a[0] + b[0];
    result[1] = a[1] + b[1];
    result[2] = a[2] + b[2];
}

static void vec3_sub(float *result, const float *a, const float *b) {
    result[0] = a[0] - b[0];
    result[1] = a[1] - b[1];
    result[2] = a[2] - b[2];
}

static void vec3_mul(float *result, const float *v, float s) {
    result[0] = v[0] * s;
    result[1] = v[1] * s;
    result[2] = v[2] * s;
}

static void mat4_to_quat(const float *m, float *q) {
    float trace = m[0] + m[5] + m[10];
    if (trace > 0.0f) {
        float s = 0.5f / sqrtf(trace + 1.0f);
        q[3] = 0.25f / s;
        q[0] = (m[6] - m[9]) * s;
        q[1] = (m[8] - m[2]) * s;
        q[2] = (m[1] - m[4]) * s;
    } else {
        if (m[0] > m[5] && m[0] > m[10]) {
            float s = 2.0f * sqrtf(1.0f + m[0] - m[5] - m[10]);
            q[3] = (m[6] - m[9]) / s;
            q[0] = 0.25f * s;
            q[1] = (m[1] + m[4]) / s;
            q[2] = (m[8] + m[2]) / s;
        } else if (m[5] > m[10]) {
            float s = 2.0f * sqrtf(1.0f + m[5] - m[0] - m[10]);
            q[3] = (m[8] - m[2]) / s;
            q[0] = (m[1] + m[4]) / s;
            q[1] = 0.25f * s;
            q[2] = (m[6] + m[9]) / s;
        } else {
            float s = 2.0f * sqrtf(1.0f + m[10] - m[0] - m[5]);
            q[3] = (m[1] - m[4]) / s;
            q[0] = (m[8] + m[2]) / s;
            q[1] = (m[6] + m[9]) / s;
            q[2] = 0.25f * s;
        }
    }
}

// ========================================
// Object Management
// ========================================

static PhysicsObject* find_object_by_id(PhysicsWorld *world, uint64_t object_id) {
    if (!world || object_id == 0) return NULL;
    
    for (int i = 0; i < world->object_count; i++) {
        if (world->objects[i].id == object_id) {
            return &world->objects[i];
        }
    }
    return NULL;
}

static int find_object_index_by_id(PhysicsWorld *world, uint64_t object_id) {
    if (!world || object_id == 0) return -1;
    
    for (int i = 0; i < world->object_count; i++) {
        if (world->objects[i].id == object_id) {
            return i;
        }
    }
    return -1;
}

static PhysicsBody* find_body_by_object_id(PhysicsWorld *world, uint64_t object_id) {
    if (!world || object_id == 0) return NULL;
    
    for (int i = 0; i < world->body_count; i++) {
        if (world->bodies[i].object && world->bodies[i].object->id == object_id) {
            return &world->bodies[i];
        }
    }
    return NULL;
}

static PhysicsConstraint* find_constraint_by_id(PhysicsWorld *world, uint64_t constraint_id) {
    if (!world || constraint_id == 0) return NULL;
    
    for (int i = 0; i < world->constraint_count; i++) {
        if (world->constraints[i].id == constraint_id) {
            return &world->constraints[i];
        }
    }
    return NULL;
}

// ========================================
// Public API Implementation
// ========================================

PhysicsWorldConfig physics_world_get_default_config(void) {
    PhysicsWorldConfig config = {
        .gravity = {0.0f, -9.81f, 0.0f},
        .time_step = 0.016f, // 60 FPS
        .max_time_step = 0.033f, // 30 FPS minimum
        .velocity_iterations = 8,
        .position_iterations = 3,
        
        .enable_sleeping = true,
        .enable_warm_starting = true,
        .enable_adaptive_time_step = false,
        .sleep_threshold = DEFAULT_SLEEP_THRESHOLD,
        .wake_up_threshold = DEFAULT_WAKE_THRESHOLD,
        
        .enable_collision_detection = true,
        .enable_collision_response = true,
        .enable_continuous_collision = false,
        .collision_margin = 0.01f,
        
        .enable_debug_draw = false,
        .enable_profiling = false,
        .enable_validation = false,
        .enable_logging = false,
        
        .max_objects = DEFAULT_MAX_OBJECTS,
        .max_constraints = DEFAULT_MAX_CONSTRAINTS,
        .max_contacts = DEFAULT_MAX_CONTACTS,
        .memory_budget = DEFAULT_MEMORY_BUDGET
    };
    
    return config;
}

PhysicsWorld* physics_world_create(const PhysicsWorldConfig *config) {
    PhysicsWorld *world = (PhysicsWorld*)calloc(1, sizeof(PhysicsWorld));
    if (!world) return NULL;
    
    // Set configuration
    if (config) {
        world->config = *config;
    } else {
        world->config = physics_world_get_default_config();
    }
    
    // Allocate object arrays
    world->object_capacity = world->config.max_objects;
    world->objects = (PhysicsObject*)calloc(world->object_capacity, sizeof(PhysicsObject));
    if (!world->objects) {
        free(world);
        return NULL;
    }
    
    world->body_capacity = world->config.max_objects;
    world->bodies = (PhysicsBody*)calloc(world->body_capacity, sizeof(PhysicsBody));
    if (!world->bodies) {
        free(world->objects);
        free(world);
        return NULL;
    }
    
    world->constraint_capacity = world->config.max_constraints;
    world->constraints = (PhysicsConstraint*)calloc(world->constraint_capacity, sizeof(PhysicsConstraint));
    if (!world->constraints) {
        free(world->bodies);
        free(world->objects);
        free(world);
        return NULL;
    }
    
    world->material_capacity = 256; // Default material capacity
    world->materials = (PhysicsMaterial*)calloc(world->material_capacity, sizeof(PhysicsMaterial));
    if (!world->materials) {
        free(world->constraints);
        free(world->bodies);
        free(world->objects);
        free(world);
        return NULL;
    }
    
    // Initialize state
    world->object_count = 0;
    world->body_count = 0;
    world->constraint_count = 0;
    world->material_count = 0;
    world->next_object_id = 1;
    world->current_time = 0.0f;
    world->accumulated_time = 0.0f;
    world->substep_count = 0;
    
    world->active_objects = 0;
    world->sleeping_objects = 0;
    world->collision_pairs = 0;
    world->constraint_count_active = 0;
    
    world->debug_enabled = false;
    world->debug_renderer = NULL;
    world->profiler = NULL;
    
    world->collision_callback = NULL;
    world->break_callback = NULL;
    world->sleep_callback = NULL;
    world->callback_user_data = NULL;
    
    // Create default material
    physics_world_create_material(world, "Default", 1.0f, 0.5f, 0.3f);
    
    return world;
}

void physics_world_destroy(PhysicsWorld *world) {
    if (!world) return;
    
    // Free object arrays
    free(world->objects);
    free(world->bodies);
    free(world->constraints);
    free(world->materials);
    
    free(world);
}

uint64_t physics_world_create_object(PhysicsWorld *world, PhysicsObjectType type, const PhysicsMaterial *material) {
    if (!world || world->object_count >= world->object_capacity) {
        return 0;
    }
    
    PhysicsObject *object = &world->objects[world->object_count];
    memset(object, 0, sizeof(PhysicsObject));
    
    object->id = world->next_object_id++;
    object->type = type;
    object->body_type = PHYSICS_BODY_DYNAMIC; // Default to dynamic
    object->data = NULL;
    
    // Set material
    if (material) {
        object->material = (PhysicsMaterial*)material;
    } else {
        // Use default material
        object->material = &world->materials[0];
    }
    
    object->active = true;
    object->sleeping = false;
    object->visible = true;
    object->layer_mask = 0xFFFFFFFF; // All layers
    object->category_mask = 0x00000001; // Default category
    
    // Initialize transform to identity
    memset(object->transform, 0, sizeof(float) * 16);
    object->transform[0] = object->transform[5] = object->transform[10] = object->transform[15] = 1.0f;
    
    // Initialize physics properties
    memset(object->velocity, 0, sizeof(float) * 3);
    memset(object->angular_velocity, 0, sizeof(float) * 3);
    memset(object->force, 0, sizeof(float) * 3);
    memset(object->torque, 0, sizeof(float) * 3);
    
    object->mass = 1.0f;
    memset(object->inertia, 0, sizeof(float) * 9);
    object->inertia[0] = object->inertia[4] = object->inertia[8] = 1.0f; // Identity inertia
    
    memset(object->center_of_mass, 0, sizeof(float) * 3);
    
    // Initialize bounding box
    object->bounding_box[0] = object->bounding_box[2] = object->bounding_box[4] = -1.0f;
    object->bounding_box[1] = object->bounding_box[3] = object->bounding_box[5] = 1.0f;
    
    // Initialize bounding sphere
    object->bounding_sphere[0] = object->bounding_sphere[1] = object->bounding_sphere[2] = 0.0f;
    object->bounding_sphere[3] = 1.0f;
    
    object->user_data = NULL;
    object->ref_count = 1;
    
    // Create corresponding body
    if (world->body_count < world->body_capacity) {
        PhysicsBody *body = &world->bodies[world->body_count];
        memset(body, 0, sizeof(PhysicsBody));
        
        body->object = object;
        body->mass = object->mass;
        body->inverse_mass = (body->mass > 0.0f) ? 1.0f / body->mass : 0.0f;
        
        // Copy inertia tensor
        memcpy(body->inertia, object->inertia, sizeof(float) * 9);
        // Calculate inverse inertia (simplified - assumes diagonal tensor)
        body->inverse_inertia[0] = (body->inertia[0] > 0.0f) ? 1.0f / body->inertia[0] : 0.0f;
        body->inverse_inertia[4] = (body->inertia[4] > 0.0f) ? 1.0f / body->inertia[4] : 0.0f;
        body->inverse_inertia[8] = (body->inertia[8] > 0.0f) ? 1.0f / body->inertia[8] : 0.0f;
        
        body->sleep_threshold = world->config.sleep_threshold;
        body->is_sleeping = false;
        body->island_id = 0;
        
        world->body_count++;
    }
    
    world->object_count++;
    world->active_objects++;
    
    return object->id;
}

bool physics_world_destroy_object(PhysicsWorld *world, uint64_t object_id) {
    if (!world || object_id == 0) return false;
    
    int index = find_object_index_by_id(world, object_id);
    if (index == -1) return false;
    
    PhysicsObject *object = &world->objects[index];
    
    // Remove corresponding body
    for (int i = 0; i < world->body_count; i++) {
        if (world->bodies[i].object == object) {
            // Shift remaining bodies
            memmove(&world->bodies[i], &world->bodies[i + 1], 
                   (world->body_count - i - 1) * sizeof(PhysicsBody));
            world->body_count--;
            break;
        }
    }
    
    // Remove constraints involving this object
    for (int i = 0; i < world->constraint_count; i++) {
        PhysicsConstraint *constraint = &world->constraints[i];
        if (constraint->body_a_id == object_id || constraint->body_b_id == object_id) {
            // Shift remaining constraints
            memmove(&world->constraints[i], &world->constraints[i + 1], 
                   (world->constraint_count - i - 1) * sizeof(PhysicsConstraint));
            world->constraint_count--;
            i--; // Check the same index again
        }
    }
    
    // Shift remaining objects
    memmove(&world->objects[index], &world->objects[index + 1], 
           (world->object_count - index - 1) * sizeof(PhysicsObject));
    world->object_count--;
    
    if (object->active) {
        world->active_objects--;
    }
    if (object->sleeping) {
        world->sleeping_objects--;
    }
    
    return true;
}

PhysicsObject* physics_world_get_object(PhysicsWorld *world, uint64_t object_id) {
    return find_object_by_id(world, object_id);
}

uint32_t physics_world_create_material(PhysicsWorld *world, const char *name, 
                                        float density, float friction, float restitution) {
    if (!world || world->material_count >= world->material_capacity) {
        return 0;
    }
    
    PhysicsMaterial *material = &world->materials[world->material_count];
    memset(material, 0, sizeof(PhysicsMaterial));
    
    material->density = density;
    material->friction = friction;
    material->restitution = restitution;
    material->damping = 0.01f;
    material->angular_damping = 0.01f;
    material->thickness = 0.1f;
    material->is_trigger = false;
    material->is_sensor = false;
    material->material_id = world->material_count + 1;
    
    if (name) {
        strncpy(material->name, name, sizeof(material->name) - 1);
        material->name[sizeof(material->name) - 1] = '\0';
    } else {
        snprintf(material->name, sizeof(material->name), "Material_%u", material->material_id);
    }
    
    world->material_count++;
    
    return material->material_id;
}

void physics_world_update(PhysicsWorld *world, float dt) {
    if (!world || dt <= 0.0f) return;
    
    uint64_t start_time = clock();
    
    // Clamp time step
    if (dt > world->config.max_time_step) {
        dt = world->config.max_time_step;
    }
    
    world->accumulated_time += dt;
    
    // Fixed time step with sub-stepping
    while (world->accumulated_time >= world->config.time_step) {
        float substep_dt = world->config.time_step;
        world->substep_count++;
        
        // Apply forces to all bodies
        for (int i = 0; i < world->body_count; i++) {
            PhysicsBody *body = &world->bodies[i];
            PhysicsObject *object = body->object;
            
            if (!object || !object->active || body->is_sleeping) continue;
            
            // Apply gravity
            if (body->inverse_mass > 0.0f) {
                float gravity_force[3];
                vec3_mul(gravity_force, world->config.gravity, body->mass);
                vec3_add(body->force_accumulator, body->force_accumulator, gravity_force);
            }
            
            // Calculate accelerations
            if (body->inverse_mass > 0.0f) {
                vec3_mul(body->linear_acceleration, body->force_accumulator, body->inverse_mass);
            }
            
            // Integrate velocities (simplified)
            // v = v + a * dt
            float delta_v[3];
            vec3_mul(delta_v, body->linear_acceleration, substep_dt);
            vec3_add(body->linear_velocity, body->linear_velocity, delta_v);
            
            // Apply damping
            vec3_mul(body->linear_velocity, body->linear_velocity, 
                    (1.0f - object->material->damping));
            
            // Update position (simplified)
            // p = p + v * dt
            float delta_p[3];
            vec3_mul(delta_p, body->linear_velocity, substep_dt);
            vec3_add(body->position, body->position, delta_p);
            
            // Clear forces
            memset(body->force_accumulator, 0, sizeof(float) * 3);
            memset(body->torque_accumulator, 0, sizeof(float) * 3);
        }
        
        // Update object transforms from body positions
        for (int i = 0; i < world->body_count; i++) {
            PhysicsBody *body = &world->bodies[i];
            PhysicsObject *object = body->object;
            
            if (!object) continue;
            
            // Update transform matrix (simplified - only translation)
            memset(object->transform, 0, sizeof(float) * 16);
            object->transform[0] = object->transform[5] = object->transform[10] = object->transform[15] = 1.0f;
            object->transform[12] = body->position[0];
            object->transform[13] = body->position[1];
            object->transform[14] = body->position[2];
            
            // Update velocity
            memcpy(object->velocity, body->linear_velocity, sizeof(float) * 3);
            memcpy(object->angular_velocity, body->angular_velocity, sizeof(float) * 3);
        }
        
        world->accumulated_time -= substep_dt;
        world->current_time += substep_dt;
    }
    
    // Update statistics
    world->active_objects = 0;
    world->sleeping_objects = 0;
    for (int i = 0; i < world->object_count; i++) {
        if (world->objects[i].active) {
            world->active_objects++;
            if (world->objects[i].sleeping) {
                world->sleeping_objects++;
            }
        }
    }
    
    // Calculate update time
    uint64_t end_time = clock();
    world->update_time_ms = (float)(end_time - start_time) / CLOCKS_PER_SEC * 1000.0f;
}

void physics_world_apply_force(PhysicsWorld *world, uint64_t object_id, 
                               const float *force, const float *point) {
    if (!world || !force || !point) return;
    
    PhysicsBody *body = find_body_by_object_id(world, object_id);
    if (!body || body->is_sleeping) return;
    
    // Add force to accumulator
    vec3_add(body->force_accumulator, body->force_accumulator, force);
    
    // Calculate torque (r x F)
    float r[3];
    vec3_sub(r, point, body->position);
    
    float torque[3];
    vec3_cross(torque, r, force);
    vec3_add(body->torque_accumulator, body->torque_accumulator, torque);
    
    // Wake up if sleeping
    if (body->is_sleeping) {
        body->is_sleeping = false;
        body->object->sleeping = false;
        world->sleeping_objects--;
        world->active_objects++;
        
        if (world->sleep_callback) {
            world->sleep_callback(object_id, false);
        }
    }
}

bool physics_world_ray_cast(PhysicsWorld *world, const float *origin, const float *direction,
                           float max_distance, uint64_t *hit_object, 
                           float *hit_point, float *hit_normal, float *hit_distance) {
    if (!world || !origin || !direction || max_distance <= 0.0f) return false;
    
    float closest_distance = max_distance;
    uint64_t closest_object = 0;
    float closest_point[3] = {0};
    float closest_normal[3] = {0};
    
    // Simple ray-sphere intersection test for all objects
    for (int i = 0; i < world->object_count; i++) {
        PhysicsObject *object = &world->objects[i];
        if (!object->active) continue;
        
        // Get object center from transform
        float center[3] = {
            object->transform[12],
            object->transform[13], 
            object->transform[14]
        };
        
        float radius = object->bounding_sphere[3];
        
        // Ray-sphere intersection
        float oc[3];
        vec3_sub(oc, origin, center);
        
        float a = vec3_dot(direction, direction);
        float b = 2.0f * vec3_dot(oc, direction);
        float c = vec3_dot(oc, oc) - radius * radius;
        
        float discriminant = b * b - 4 * a * c;
        if (discriminant < 0.0f) continue;
        
        float sqrt_discriminant = sqrtf(discriminant);
        float t1 = (-b - sqrt_discriminant) / (2.0f * a);
        float t2 = (-b + sqrt_discriminant) / (2.0f * a);
        
        float t = (t1 > 0.0f) ? t1 : t2;
        if (t <= 0.0f || t > closest_distance) continue;
        
        // Found closer intersection
        closest_distance = t;
        closest_object = object->id;
        
        // Calculate hit point
        vec3_mul(closest_point, direction, t);
        vec3_add(closest_point, closest_point, origin);
        
        // Calculate normal (point from center to hit point)
        vec3_sub(closest_normal, closest_point, center);
        vec3_normalize(closest_normal, closest_normal);
    }
    
    if (closest_object != 0) {
        if (hit_object) *hit_object = closest_object;
        if (hit_point) memcpy(hit_point, closest_point, sizeof(float) * 3);
        if (hit_normal) memcpy(hit_normal, closest_normal, sizeof(float) * 3);
        if (hit_distance) *hit_distance = closest_distance;
        return true;
    }
    
    return false;
}

PhysicsMaterial* physics_world_get_material(PhysicsWorld *world, uint32_t material_id) {
    if (!world) return NULL;
    for (int i = 0; i < world->material_count; i++) {
        if (world->materials[i].material_id == material_id) {
            return &world->materials[i];
        }
    }
    return NULL;
}

void physics_world_set_object_transform(PhysicsWorld *world, uint64_t object_id, const float *transform) {
    if (!world || !transform) return;
    PhysicsObject *obj = find_object_by_id(world, object_id);
    if (obj) {
        memcpy(obj->transform, transform, sizeof(float) * 16);
        // Also update body position and orientation
        PhysicsBody *body = find_body_by_object_id(world, object_id);
        if (body) {
            body->position[0] = transform[12];
            body->position[1] = transform[13];
            body->position[2] = transform[14];

            // Extract rotation
            mat4_to_quat(transform, body->orientation);
        }
    }
}

void physics_world_get_object_transform(PhysicsWorld *world, uint64_t object_id, float *transform) {
    if (!world || !transform) return;
    PhysicsObject *obj = find_object_by_id(world, object_id);
    if (obj) {
        memcpy(transform, obj->transform, sizeof(float) * 16);
    }
}

void physics_world_apply_impulse(PhysicsWorld *world, uint64_t object_id,
                                  const float *impulse, const float *point) {
    if (!world || !impulse || !point) return;
    PhysicsBody *body = find_body_by_object_id(world, object_id);
    if (body && body->inverse_mass > 0.0f) {
        // v += J * invM
        float delta_v[3];
        vec3_mul(delta_v, impulse, body->inverse_mass);
        vec3_add(body->linear_velocity, body->linear_velocity, delta_v);

        // Wake up
        if (body->is_sleeping) {
            body->is_sleeping = false;
            body->object->sleeping = false;
            world->sleeping_objects--;
            world->active_objects++;
        }
    }
}

void physics_world_apply_torque(PhysicsWorld *world, uint64_t object_id, const float *torque) {
    if (!world || !torque) return;
    PhysicsBody *body = find_body_by_object_id(world, object_id);
    if (body && !body->is_sleeping) {
        vec3_add(body->torque_accumulator, body->torque_accumulator, torque);
         // Wake up
        if (body->is_sleeping) {
            body->is_sleeping = false;
            body->object->sleeping = false;
            world->sleeping_objects--;
            world->active_objects++;
        }
    }
}

float physics_world_get_time_step(PhysicsWorld *world) {
    return world ? world->config.time_step : 0.0f;
}

void physics_world_get_stats(PhysicsWorld *world, int *object_count, int *body_count,
                              int *constraint_count, int *collision_pairs,
                              int *active_objects, int *sleeping_objects,
                              float *update_time) {
    if (!world) return;
    
    if (object_count) *object_count = world->object_count;
    if (body_count) *body_count = world->body_count;
    if (constraint_count) *constraint_count = world->constraint_count;
    if (collision_pairs) *collision_pairs = world->collision_pairs;
    if (active_objects) *active_objects = world->active_objects;
    if (sleeping_objects) *sleeping_objects = world->sleeping_objects;
    if (update_time) *update_time = world->update_time_ms;
}

// ========================================
// Serialization
// ========================================

size_t physics_world_serialize(PhysicsWorld *world, void *buffer, size_t buffer_size) {
    if (!world) return 0;

    // 1. Calculate size
    size_t size = sizeof(PhysicsWorldConfig);
    size += sizeof(int); // object count
    size += world->object_count * sizeof(PhysicsObject);
    size += sizeof(int); // body count
    size += world->body_count * sizeof(PhysicsBody);

    if (!buffer) return size;
    if (buffer_size < size) return 0;

    uint8_t *ptr = (uint8_t*)buffer;

    // 2. Serialize Config
    memcpy(ptr, &world->config, sizeof(PhysicsWorldConfig));
    ptr += sizeof(PhysicsWorldConfig);

    // 3. Serialize Objects
    memcpy(ptr, &world->object_count, sizeof(int));
    ptr += sizeof(int);

    for (int i = 0; i < world->object_count; i++) {
        PhysicsObject temp = world->objects[i];
        // Convert material pointer to ID
        if (temp.material) {
            // We use the pointer field to store the ID for serialization
            // This is hacky but avoids defining a separate struct
            // We cast ID to pointer. 64-bit ID fits in 64-bit pointer.
            // material_id is uint32_t, so it fits.
            // But we need to be careful. temp.material is a pointer.
            // We'll store (void*)(uintptr_t)material_id
            temp.material = (PhysicsMaterial*)(uintptr_t)temp.material->material_id;
        } else {
            temp.material = NULL;
        }
        temp.data = NULL; // Clear user pointers
        temp.user_data = NULL;

        memcpy(ptr, &temp, sizeof(PhysicsObject));
        ptr += sizeof(PhysicsObject);
    }

    // 4. Serialize Bodies
    memcpy(ptr, &world->body_count, sizeof(int));
    ptr += sizeof(int);

    for (int i = 0; i < world->body_count; i++) {
        PhysicsBody temp = world->bodies[i];
        // Convert object pointer to ID
        if (temp.object) {
             // Store object ID in pointer field
             temp.object = (PhysicsObject*)(uintptr_t)temp.object->id;
        } else {
            temp.object = NULL;
        }

        memcpy(ptr, &temp, sizeof(PhysicsBody));
        ptr += sizeof(PhysicsBody);
    }

    return size;
}

size_t physics_world_deserialize(PhysicsWorld *world, const void *buffer, size_t buffer_size) {
    if (!world || !buffer) return 0;

    const uint8_t *ptr = (const uint8_t*)buffer;
    size_t read_size = 0;

    // 1. Deserialize Config
    if (buffer_size < sizeof(PhysicsWorldConfig)) return 0;
    memcpy(&world->config, ptr, sizeof(PhysicsWorldConfig));
    ptr += sizeof(PhysicsWorldConfig);
    read_size += sizeof(PhysicsWorldConfig);

    // 2. Deserialize Objects
    if (buffer_size < read_size + sizeof(int)) return 0;
    int object_count;
    memcpy(&object_count, ptr, sizeof(int));
    ptr += sizeof(int);
    read_size += sizeof(int);

    if (object_count < 0 || object_count > world->object_capacity) {
        // In a real scenario, we might resize. Here we fail or clamp.
        // Assuming capacity is sufficient or we fail.
        // For replay, if we created the world with same config, capacity matches.
        if (object_count > world->object_capacity) return 0;
    }

    world->object_count = object_count;

    if (buffer_size < read_size + object_count * sizeof(PhysicsObject)) return 0;

    for (int i = 0; i < object_count; i++) {
        PhysicsObject *obj = &world->objects[i];
        memcpy(obj, ptr, sizeof(PhysicsObject));
        ptr += sizeof(PhysicsObject);
        read_size += sizeof(PhysicsObject);

        // Restore material pointer
        uint32_t mat_id = (uint32_t)(uintptr_t)obj->material;
        obj->material = physics_world_get_material(world, mat_id);
    }

    // 3. Deserialize Bodies
    if (buffer_size < read_size + sizeof(int)) return 0;
    int body_count;
    memcpy(&body_count, ptr, sizeof(int));
    ptr += sizeof(int);
    read_size += sizeof(int);

    if (body_count > world->body_capacity) return 0;
    world->body_count = body_count;

    if (buffer_size < read_size + body_count * sizeof(PhysicsBody)) return 0;

    for (int i = 0; i < body_count; i++) {
        PhysicsBody *body = &world->bodies[i];
        memcpy(body, ptr, sizeof(PhysicsBody));
        ptr += sizeof(PhysicsBody);
        read_size += sizeof(PhysicsBody);

        // Restore object pointer
        uint64_t obj_id = (uint64_t)(uintptr_t)body->object;
        body->object = find_object_by_id(world, obj_id);
    }

    // Reset internal counters
    world->current_time = 0.0f;
    world->accumulated_time = 0.0f;
    world->substep_count = 0;

    return read_size;
}
