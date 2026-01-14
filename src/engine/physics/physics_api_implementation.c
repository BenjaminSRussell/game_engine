// physics_api_implementation.c
// Comprehensive Physics API Implementation

#include "physics.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Internal Physics World Structure
struct PhysicsWorld {
    Vec3 gravity;
    f32 fixed_timestep;
    u32 velocity_iterations;
    u32 position_iterations;
    
    // Body management
    RigidBody* bodies;
    u32 body_count;
    u32 body_capacity;
    
    // Collision detection
    CollisionManifold* manifolds;
    u32 manifold_count;
    u32 manifold_capacity;
    
    // Performance tracking
    PhysicsPerformance performance;
    
    // Threading
    void* solver_mutex;
    bool deterministic_mode;
};

// Internal Rigid Body Structure
struct RigidBody {
    BodyType type;
    Vec3 position;
    Vec3 velocity;
    Vec3 force;
    Vec3 acceleration;
    Quat rotation;
    Vec3 angular_velocity;
    Vec3 torque;
    Vec3 angular_acceleration;
    
    f32 mass;
    f32 inv_mass;
    f32 inertia;
    f32 inv_inertia;
    f32 restitution;
    f32 friction;
    f32 linear_damping;
    f32 angular_damping;
    
    Collider* collider;
    bool is_sleeping;
    f32 sleep_timer;
    
    // User data
    void* user_data;
};

// Internal Collider Structure
struct Collider {
    ColliderType type;
    union {
        struct { f32 radius; } sphere;
        struct { Vec3 half_extents; } box;
        struct { f32 radius; f32 height; } capsule;
        struct { void* mesh_data; } mesh;
    } shape;
    
    PhysicsMaterial material;
    
    // Transform relative to body
    Vec3 local_position;
    Quat local_rotation;
};

// Internal Character Controller Structure
struct CharacterController {
    RigidBody* body;
    Vec3 ground_normal;
    bool is_grounded;
    f32 step_height;
    f32 slope_limit;
    f32 move_speed;
    f32 jump_height;
    f32 air_control;
    
    // Movement state
    Vec3 move_direction;
    bool jump_requested;
};

// -----------------------------------------------------------------------------
// Physics World Implementation
// -----------------------------------------------------------------------------

PhysicsWorld* physics_world_create(PhysicsConfig config) {
    PhysicsWorld* world = malloc(sizeof(PhysicsWorld));
    if (!world) {
        LOG_ERROR("Failed to allocate physics world");
        return NULL;
    }
    
    memset(world, 0, sizeof(PhysicsWorld));
    
    world->gravity = config.gravity;
    world->fixed_timestep = config.fixed_timestep;
    world->velocity_iterations = config.velocity_iterations;
    world->position_iterations = config.position_iterations;
    
    // Initialize body storage
    world->body_capacity = 1024;
    world->bodies = malloc(sizeof(RigidBody) * world->body_capacity);
    if (!world->bodies) {
        LOG_ERROR("Failed to allocate body storage");
        free(world);
        return NULL;
    }
    
    // Initialize manifold storage
    world->manifold_capacity = 2048;
    world->manifolds = malloc(sizeof(CollisionManifold) * world->manifold_capacity);
    if (!world->manifolds) {
        LOG_ERROR("Failed to allocate manifold storage");
        free(world->bodies);
        free(world);
        return NULL;
    }
    
    // Initialize threading
    world->solver_mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init((pthread_mutex_t*)world->solver_mutex, NULL);
    
    LOG_INFO("Physics world created successfully");
    return world;
}

void physics_world_destroy(PhysicsWorld* world) {
    if (!world) return;
    
    // Destroy all bodies
    for (u32 i = 0; i < world->body_count; i++) {
        if (world->bodies[i].collider) {
            collider_destroy(world->bodies[i].collider);
        }
    }
    
    // Cleanup resources
    free(world->bodies);
    free(world->manifolds);
    
    if (world->solver_mutex) {
        pthread_mutex_destroy((pthread_mutex_t*)world->solver_mutex);
        free(world->solver_mutex);
    }
    
    free(world);
    LOG_INFO("Physics world destroyed");
}

void physics_world_init(PhysicsWorld* world, const Vec3* gravity) {
    if (!world || !gravity) return;
    
    world->gravity = *gravity;
    LOG_INFO("Physics world initialized with gravity: (%.2f, %.2f, %.2f)", 
             gravity->x, gravity->y, gravity->z);
}

void physics_world_free(PhysicsWorld* world) {
    physics_world_destroy(world);
}

void physics_world_step(PhysicsWorld* world, f32 delta_time) {
    if (!world) return;
    
    f64 start_time = get_current_time_ms();
    
    // Fixed timestep with accumulator
    static f32 accumulator = 0.0f;
    accumulator += delta_time;
    
    while (accumulator >= world->fixed_timestep) {
        // Apply forces
        for (u32 i = 0; i < world->body_count; i++) {
            RigidBody* body = &world->bodies[i];
            if (body->type == BODY_TYPE_DYNAMIC) {
                // Apply gravity
                body->force = vec3_add(&body->force, &world->gravity);
                
                // Update acceleration
                body->acceleration = vec3_multiply(&body->force, body->inv_mass);
                body->angular_acceleration = vec3_multiply(&body->torque, body->inv_inertia);
                
                // Integrate velocity
                body->velocity = vec3_add(&body->velocity, 
                    vec3_multiply(&body->acceleration, world->fixed_timestep));
                body->angular_velocity = vec3_add(&body->angular_velocity,
                    vec3_multiply(&body->angular_acceleration, world->fixed_timestep));
                
                // Apply damping
                body->velocity = vec3_multiply(&body->velocity, 
                    1.0f - body->linear_damping * world->fixed_timestep);
                body->angular_velocity = vec3_multiply(&body->angular_velocity,
                    1.0f - body->angular_damping * world->fixed_timestep);
                
                // Clear forces
                body->force = (Vec3){0, 0, 0};
                body->torque = (Vec3){0, 0, 0};
            }
        }
        
        // Broadphase collision detection
        world->manifold_count = 0;
        for (u32 i = 0; i < world->body_count; i++) {
            for (u32 j = i + 1; j < world->body_count; j++) {
                CollisionManifold manifold;
                if (physics_check_collision(&world->bodies[i], &world->bodies[j], &manifold)) {
                    if (world->manifold_count < world->manifold_capacity) {
                        world->manifolds[world->manifold_count++] = manifold;
                    }
                }
            }
        }
        
        // Solve collisions
        pthread_mutex_lock((pthread_mutex_t*)world->solver_mutex);
        for (u32 iteration = 0; iteration < world->velocity_iterations; iteration++) {
            for (u32 i = 0; i < world->manifold_count; i++) {
                physics_resolve_collision(world->manifolds[i].body_a, 
                                      world->manifolds[i].body_b,
                                      &world->manifolds[i]);
            }
        }
        pthread_mutex_unlock((pthread_mutex_t*)world->solver_mutex);
        
        // Position correction
        for (u32 iteration = 0; iteration < world->position_iterations; iteration++) {
            for (u32 i = 0; i < world->manifold_count; i++) {
                // Position correction using Baumgarte stabilization
                CollisionManifold* manifold = &world->manifolds[i];
                const f32 percent = 0.2f;
                const f32 slop = 0.01f;
                Vec3 correction = vec3_multiply(&manifold->normal, 
                    (manifold->depth - slop) * percent / 
                    (manifold->body_a->inv_mass + manifold->body_b->inv_mass));
                
                manifold->body_a->position = vec3_add(&manifold->body_a->position,
                    vec3_multiply(&correction, manifold->body_a->inv_mass));
                manifold->body_b->position = vec3_subtract(&manifold->body_b->position,
                    vec3_multiply(&correction, manifold->body_b->inv_mass));
            }
        }
        
        // Integrate positions
        for (u32 i = 0; i < world->body_count; i++) {
            RigidBody* body = &world->bodies[i];
            if (body->type == BODY_TYPE_DYNAMIC) {
                body->position = vec3_add(&body->position,
                    vec3_multiply(&body->velocity, world->fixed_timestep));
                
                // Update rotation (simplified)
                f32 angle = vec3_length(&body->angular_velocity) * world->fixed_timestep;
                if (angle > 0.0f) {
                    Vec3 axis = vec3_normalize(&body->angular_velocity);
                    Quat rotation = quat_from_axis_angle(&axis, angle);
                    body->rotation = quat_multiply(&body->rotation, &rotation);
                }
            }
        }
        
        accumulator -= world->fixed_timestep;
    }
    
    // Update performance stats
    f64 end_time = get_current_time_ms();
    world->performance.total_time_ms = end_time - start_time;
    world->performance.bodies_processed = world->body_count;
    world->performance.collision_pairs = world->manifold_count;
}

RigidBody* physics_world_add_body(PhysicsWorld* world, RigidBody* body) {
    if (!world || !body) return NULL;
    
    if (world->body_count >= world->body_capacity) {
        // Expand capacity
        world->body_capacity *= 2;
        RigidBody* new_bodies = realloc(world->bodies, 
            sizeof(RigidBody) * world->body_capacity);
        if (!new_bodies) {
            LOG_ERROR("Failed to expand body storage");
            return NULL;
        }
        world->bodies = new_bodies;
    }
    
    world->bodies[world->body_count] = *body;
    return &world->bodies[world->body_count++];
}

void physics_world_remove_body(PhysicsWorld* world, RigidBody* body) {
    if (!world || !body) return;
    
    // Find and remove body
    for (u32 i = 0; i < world->body_count; i++) {
        if (&world->bodies[i] == body) {
            // Shift remaining bodies
            memmove(&world->bodies[i], &world->bodies[i + 1],
                sizeof(RigidBody) * (world->body_count - i - 1));
            world->body_count--;
            return;
        }
    }
}

RaycastResult physics_raycast(PhysicsWorld* world, Vec3 origin, Vec3 direction, f32 max_distance) {
    RaycastResult result = {0};
    result.origin = origin;
    result.direction = direction;
    result.max_distance = max_distance;
    result.hit = false;
    result.hit_distance = max_distance;
    
    if (!world) return result;
    
    Vec3 normalized_dir = vec3_normalize(&direction);
    f32 closest_distance = max_distance;
    
    for (u32 i = 0; i < world->body_count; i++) {
        RigidBody* body = &world->bodies[i];
        if (!body->collider) continue;
        
        // Simple sphere intersection for now
        if (body->collider->type == COLLIDER_TYPE_SPHERE) {
            f32 radius = body->collider->shape.sphere.radius;
            Vec3 to_center = vec3_subtract(&body->position, &origin);
            f32 projection = vec3_dot(&to_center, &normalized_dir);
            
            if (projection < 0.0f || projection > closest_distance) continue;
            
            Vec3 closest_point = vec3_add(&origin, 
                vec3_multiply(&normalized_dir, projection));
            f32 distance_to_center = vec3_length(&vec3_subtract(&closest_point, &body->position));
            
            if (distance_to_center <= radius) {
                f32 hit_distance = projection - sqrtf(radius * radius - distance_to_center * distance_to_center);
                if (hit_distance < closest_distance && hit_distance >= 0.0f) {
                    closest_distance = hit_distance;
                    result.hit = true;
                    result.hit_distance = hit_distance;
                    result.hit_point = closest_point;
                    result.hit_normal = vec3_normalize(&vec3_subtract(&closest_point, &body->position));
                    result.hit_body = body;
                }
            }
        }
    }
    
    return result;
}

// -----------------------------------------------------------------------------
// Rigid Body Implementation
// -----------------------------------------------------------------------------

RigidBody* rigid_body_create(BodyType type, Vec3 position) {
    RigidBody* body = malloc(sizeof(RigidBody));
    if (!body) {
        LOG_ERROR("Failed to allocate rigid body");
        return NULL;
    }
    
    memset(body, 0, sizeof(RigidBody));
    
    body->type = type;
    body->position = position;
    body->rotation = quat_identity();
    body->mass = 1.0f;
    body->inv_mass = (type == BODY_TYPE_DYNAMIC) ? 1.0f : 0.0f;
    body->inertia = 1.0f;
    body->inv_inertia = (type == BODY_TYPE_DYNAMIC) ? 1.0f : 0.0f;
    body->restitution = 0.5f;
    body->friction = 0.5f;
    body->linear_damping = 0.01f;
    body->angular_damping = 0.01f;
    
    LOG_INFO("Rigid body created at position: (%.2f, %.2f, %.2f)", 
             position.x, position.y, position.z);
    return body;
}

void rigid_body_destroy(RigidBody* body) {
    if (!body) return;
    
    if (body->collider) {
        collider_destroy(body->collider);
    }
    
    free(body);
    LOG_INFO("Rigid body destroyed");
}

void rigid_body_set_mass(RigidBody* body, f32 mass) {
    if (!body || body->type != BODY_TYPE_DYNAMIC) return;
    
    body->mass = mass;
    body->inv_mass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
    
    // Update inertia (simplified as sphere)
    f32 radius = 1.0f; // Default radius
    body->inertia = 0.4f * mass * radius * radius;
    body->inv_inertia = (body->inertia > 0.0f) ? 1.0f / body->inertia : 0.0f;
}

void rigid_body_set_friction(RigidBody* body, f32 friction) {
    if (!body) return;
    body->friction = clamp_f32(friction, 0.0f, 1.0f);
}

void rigid_body_set_restitution(RigidBody* body, f32 restitution) {
    if (!body) return;
    body->restitution = clamp_f32(restitution, 0.0f, 1.0f);
}

void rigid_body_set_linear_damping(RigidBody* body, f32 damping) {
    if (!body) return;
    body->linear_damping = clamp_f32(damping, 0.0f, 1.0f);
}

void rigid_body_set_angular_damping(RigidBody* body, f32 damping) {
    if (!body) return;
    body->angular_damping = clamp_f32(damping, 0.0f, 1.0f);
}

void rigid_body_set_position(RigidBody* body, Vec3 position) {
    if (!body) return;
    body->position = position;
}

Vec3 rigid_body_get_position(const RigidBody* body) {
    if (!body) return (Vec3){0, 0, 0};
    return body->position;
}

void rigid_body_set_rotation(RigidBody* body, Quat rotation) {
    if (!body) return;
    body->rotation = quat_normalize(&rotation);
}

Quat rigid_body_get_rotation(const RigidBody* body) {
    if (!body) return quat_identity();
    return body->rotation;
}

Vec3 rigid_body_get_velocity(const RigidBody* body) {
    if (!body) return (Vec3){0, 0, 0};
    return body->velocity;
}

void rigid_body_set_velocity(RigidBody* body, Vec3 velocity) {
    if (!body) return;
    body->velocity = velocity;
}

void rigid_body_add_force(RigidBody* body, Vec3 force) {
    if (!body || body->type != BODY_TYPE_DYNAMIC) return;
    body->force = vec3_add(&body->force, &force);
}

void rigid_body_add_impulse(RigidBody* body, Vec3 impulse) {
    if (!body || body->type != BODY_TYPE_DYNAMIC) return;
    body->velocity = vec3_add(&body->velocity, 
        vec3_multiply(&impulse, body->inv_mass));
}

void rigid_body_clear_forces(RigidBody* body) {
    if (!body) return;
    body->force = (Vec3){0, 0, 0};
    body->torque = (Vec3){0, 0, 0};
}

f32 rigid_body_get_inv_mass(const RigidBody* body) {
    if (!body) return 0.0f;
    return body->inv_mass;
}

f32 rigid_body_get_restitution(const RigidBody* body) {
    if (!body) return 0.0f;
    return body->restitution;
}

Collider* rigid_body_get_collider(const RigidBody* body) {
    if (!body) return NULL;
    return body->collider;
}

bool rigid_body_is_sleeping(const RigidBody* body) {
    if (!body) return false;
    return body->is_sleeping;
}

// -----------------------------------------------------------------------------
// Collider Implementation
// -----------------------------------------------------------------------------

Collider* collider_create_sphere(f32 radius) {
    Collider* collider = malloc(sizeof(Collider));
    if (!collider) {
        LOG_ERROR("Failed to allocate sphere collider");
        return NULL;
    }
    
    memset(collider, 0, sizeof(Collider));
    collider->type = COLLIDER_TYPE_SPHERE;
    collider->shape.sphere.radius = radius;
    
    // Default material
    collider->material.friction = 0.5f;
    collider->material.restitution = 0.5f;
    collider->material.density = 1.0f;
    
    LOG_INFO("Sphere collider created with radius: %.2f", radius);
    return collider;
}

Collider* collider_create_box(Vec3 half_extents) {
    Collider* collider = malloc(sizeof(Collider));
    if (!collider) {
        LOG_ERROR("Failed to allocate box collider");
        return NULL;
    }
    
    memset(collider, 0, sizeof(Collider));
    collider->type = COLLIDER_TYPE_BOX;
    collider->shape.box.half_extents = half_extents;
    
    // Default material
    collider->material.friction = 0.5f;
    collider->material.restitution = 0.5f;
    collider->material.density = 1.0f;
    
    LOG_INFO("Box collider created with half extents: (%.2f, %.2f, %.2f)", 
             half_extents.x, half_extents.y, half_extents.z);
    return collider;
}

void collider_destroy(Collider* collider) {
    if (!collider) return;
    
    if (collider->type == COLLIDER_TYPE_MESH && collider->shape.mesh.mesh_data) {
        // Cleanup mesh data if needed
        free(collider->shape.mesh.mesh_data);
    }
    
    free(collider);
    LOG_INFO("Collider destroyed");
}

void rigid_body_attach_collider(RigidBody* body, Collider* collider) {
    if (!body || !collider) return;
    
    if (body->collider) {
        collider_destroy(body->collider);
    }
    
    body->collider = collider;
    LOG_INFO("Collider attached to rigid body");
}

ColliderType collider_get_type(const Collider* collider) {
    if (!collider) return COLLIDER_TYPE_SPHERE;
    return collider->type;
}

void collider_get_box_half_extents(const Collider* collider, f32* out_x, f32* out_y, f32* out_z) {
    if (!collider || collider->type != COLLIDER_TYPE_BOX) {
        if (out_x) *out_x = 0.0f;
        if (out_y) *out_y = 0.0f;
        if (out_z) *out_z = 0.0f;
        return;
    }
    
    if (out_x) *out_x = collider->shape.box.half_extents.x;
    if (out_y) *out_y = collider->shape.box.half_extents.y;
    if (out_z) *out_z = collider->shape.box.half_extents.z;
}

f32 collider_get_sphere_radius(const Collider* collider) {
    if (!collider || collider->type != COLLIDER_TYPE_SPHERE) return 0.0f;
    return collider->shape.sphere.radius;
}

// -----------------------------------------------------------------------------
// Collision Detection Implementation
// -----------------------------------------------------------------------------

bool physics_check_collision(RigidBody* a, RigidBody* b, CollisionManifold* out) {
    if (!a || !b || !out) return false;
    
    if (!a->collider || !b->collider) return false;
    
    // Simple sphere-sphere collision for now
    if (a->collider->type == COLLIDER_TYPE_SPHERE && 
        b->collider->type == COLLIDER_TYPE_SPHERE) {
        
        Vec3 diff = vec3_subtract(&b->position, &a->position);
        f32 distance = vec3_length(&diff);
        f32 radius_sum = a->collider->shape.sphere.radius + b->collider->shape.sphere.radius;
        
        if (distance < radius_sum) {
            out->body_a = a;
            out->body_b = b;
            out->depth = radius_sum - distance;
            out->normal = (distance > 0.0f) ? vec3_multiply(&diff, 1.0f / distance) : (Vec3){1, 0, 0};
            out->point = vec3_add(&a->position, vec3_multiply(&out->normal, 
                a->collider->shape.sphere.radius - out->depth * 0.5f));
            return true;
        }
    }
    
    return false;
}

void physics_resolve_collision(RigidBody* a, RigidBody* b, CollisionManifold* m) {
    if (!a || !b || !m) return;
    
    // Calculate relative velocity
    Vec3 relative_velocity = vec3_subtract(&b->velocity, &a->velocity);
    f32 velocity_along_normal = vec3_dot(&relative_velocity, &m->normal);
    
    // Don't resolve if velocities are separating
    if (velocity_along_normal > 0.0f) return;
    
    // Calculate restitution
    f32 restitution = fmaxf(a->restitution, b->restitution);
    
    // Calculate impulse scalar
    f32 impulse_scalar = -(1.0f + restitution) * velocity_along_normal;
    impulse_scalar /= a->inv_mass + b->inv_mass;
    
    // Apply impulse
    Vec3 impulse = vec3_multiply(&m->normal, impulse_scalar);
    
    if (a->type == BODY_TYPE_DYNAMIC) {
        a->velocity = vec3_subtract(&a->velocity, vec3_multiply(&impulse, a->inv_mass));
    }
    if (b->type == BODY_TYPE_DYNAMIC) {
        b->velocity = vec3_add(&b->velocity, vec3_multiply(&impulse, b->inv_mass));
    }
    
    // Apply friction
    Vec3 tangent = vec3_subtract(&relative_velocity, 
        vec3_multiply(&m->normal, velocity_along_normal));
    f32 tangent_length = vec3_length(&tangent);
    
    if (tangent_length > 0.001f) {
        tangent = vec3_multiply(&tangent, 1.0f / tangent_length);
        f32 friction_impulse = -velocity_along_normal * fmaxf(a->friction, b->friction);
        
        Vec3 friction = vec3_multiply(&tangent, friction_impulse);
        
        if (a->type == BODY_TYPE_DYNAMIC) {
            a->velocity = vec3_subtract(&a->velocity, vec3_multiply(&friction, a->inv_mass));
        }
        if (b->type == BODY_TYPE_DYNAMIC) {
            b->velocity = vec3_add(&b->velocity, vec3_multiply(&friction, b->inv_mass));
        }
    }
}

// -----------------------------------------------------------------------------
// Helper Functions Implementation
// -----------------------------------------------------------------------------

RigidBody* physics_create_box(PhysicsWorld* world, Vec3 pos, Vec3 size, f32 mass, PhysicsMaterial* mat) {
    if (!world) return NULL;
    
    RigidBody* body = rigid_body_create(BODY_TYPE_DYNAMIC, pos);
    if (!body) return NULL;
    
    rigid_body_set_mass(body, mass);
    
    Collider* collider = collider_create_box((Vec3){size.x * 0.5f, size.y * 0.5f, size.z * 0.5f});
    if (!collider) {
        rigid_body_destroy(body);
        return NULL;
    }
    
    if (mat) {
        collider->material = *mat;
    }
    
    rigid_body_attach_collider(body, collider);
    physics_world_add_body(world, body);
    
    return body;
}

// -----------------------------------------------------------------------------
// Performance and Debug Implementation
// -----------------------------------------------------------------------------

bool Physics_Initialize(int max_bodies) {
    LOG_INFO("Physics system initialized with max bodies: %d", max_bodies);
    return true;
}

void Physics_Shutdown(void) {
    LOG_INFO("Physics system shutdown");
}

void Physics_Simulation_Step(float delta_time) {
    // This would be called from the main physics update
    LOG_DEBUG("Physics simulation step: %.3f", delta_time);
}

const PhysicsPerformance* Physics_GetPerformanceStats(void) {
    static PhysicsPerformance stats = {0};
    return &stats;
}

void Physics_SetDeterministicMode(bool enabled) {
    LOG_INFO("Physics deterministic mode: %s", enabled ? "enabled" : "disabled");
}

// -----------------------------------------------------------------------------
// Utility Functions
// -----------------------------------------------------------------------------

static f64 get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (f64)ts.tv_sec * 1000.0 + (f64)ts.tv_nsec / 1000000.0;
}

static f32 clamp_f32(f32 value, f32 min, f32 max) {
    return fmaxf(min, fminf(max, value));
}

static Quat quat_identity(void) {
    return (Quat){0, 0, 0, 1};
}

static Quat quat_normalize(const Quat* q) {
    f32 length = sqrtf(q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w);
    if (length > 0.0f) {
        return (Quat){q->x / length, q->y / length, q->z / length, q->w / length};
    }
    return quat_identity();
}

static Quat quat_multiply(const Quat* a, const Quat* b) {
    return (Quat){
        a->w * b->x + a->x * b->w + a->y * b->z - a->z * b->y,
        a->w * b->y - a->x * b->z + a->y * b->w + a->z * b->x,
        a->w * b->z + a->x * b->y - a->y * b->x + a->z * b->w,
        a->w * b->w - a->x * b->x - a->y * b->y - a->z * b->z
    };
}

static Quat quat_from_axis_angle(const Vec3* axis, f32 angle) {
    f32 half_angle = angle * 0.5f;
    f32 sin_half = sinf(half_angle);
    Vec3 normalized_axis = vec3_normalize(axis);
    
    return (Quat){
        normalized_axis.x * sin_half,
        normalized_axis.y * sin_half,
        normalized_axis.z * sin_half,
        cosf(half_angle)
    };
}
