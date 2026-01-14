// Advanced Physics Implementation
// Implements advanced physics simulation capabilities

#include "advanced_physics.h"
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Utility functions implementation
Vec3 vec3_make(float x, float y, float z) {
    Vec3 v = {x, y, z};
    return v;
}

Vec3 vec3_add(const Vec3* a, const Vec3* b) {
    return vec3_make(a->x + b->x, a->y + b->y, a->z + b->z);
}

Vec3 vec3_subtract(const Vec3* a, const Vec3* b) {
    return vec3_make(a->x - b->x, a->y - b->y, a->z - b->z);
}

Vec3 vec3_multiply_value(const Vec3* v, float scalar) {
    return vec3_make(v->x * scalar, v->y * scalar, v->z * scalar);
}

Vec3 vec3_multiply(const Vec3* v, float scalar) {
    return vec3_make(v->x * scalar, v->y * scalar, v->z * scalar);
}

Vec3 vec3_cross(const Vec3* a, const Vec3* b) {
    return vec3_make(
        a->y * b->z - a->z * b->y,
        a->z * b->x - a->x * b->z,
        a->x * b->y - a->y * b->x
    );
}

float vec3_dot(const Vec3* a, const Vec3* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

float vec3_length(const Vec3* v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

Vec3 vec3_normalize(const Vec3* v) {
    float length = vec3_length(v);
    if (length > 0.0f) {
        return vec3_multiply(v, 1.0f / length);
    }
    return vec3_make(0.0f, 0.0f, 0.0f);
}

Mat4 mat4_identity(void) {
    Mat4 m = {0};
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
    return m;
}

Mat4 mat4_translation(const Vec3* translation) {
    Mat4 m = mat4_identity();
    m.m[12] = translation->x;
    m.m[13] = translation->y;
    m.m[14] = translation->z;
    return m;
}

Mat4 mat4_rotation(const Vec3* axis, float angle) {
    Mat4 m = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);
    float t = 1.0f - c;
    Vec3 a = vec3_normalize(axis);
    
    m.m[0] = t * a.x * a.x + c;
    m.m[1] = t * a.x * a.y - s * a.z;
    m.m[2] = t * a.x * a.z + s * a.y;
    m.m[4] = t * a.x * a.y + s * a.z;
    m.m[5] = t * a.y * a.y + c;
    m.m[6] = t * a.y * a.z - s * a.x;
    m.m[8] = t * a.x * a.z - s * a.y;
    m.m[9] = t * a.y * a.z + s * a.x;
    m.m[10] = t * a.z * a.z + c;
    
    return m;
}

Mat4 mat4_scale(const Vec3* scale) {
    Mat4 m = mat4_identity();
    m.m[0] = scale->x;
    m.m[5] = scale->y;
    m.m[10] = scale->z;
    return m;
}

Mat4 mat4_multiply(const Mat4* a, const Mat4* b) {
    Mat4 result = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i * 4 + j] += a->m[i * 4 + k] * b->m[k * 4 + j];
            }
        }
    }
    return result;
}

// Physics world implementation
PhysicsWorld* physics_world_create(const Vec3* gravity, float timeStep) {
    if (!gravity || timeStep <= 0.0f) {
        LOG_ERROR("Invalid parameters for physics world creation");
        return NULL;
    }
    
    PhysicsWorld* world = malloc(sizeof(PhysicsWorld));
    if (!world) {
        LOG_ERROR("Failed to allocate memory for physics world");
        return NULL;
    }
    
    memset(world, 0, sizeof(PhysicsWorld));
    world->gravity = *gravity;
    world->timeStep = timeStep;
    world->maxSubSteps = 4;
    world->autoSleep = true;
    world->sleepThreshold = 0.1f;
    
    // Allocate initial body array
    world->maxBodies = 1024;
    world->bodies = malloc(sizeof(PhysicsBody) * world->maxBodies);
    if (!world->bodies) {
        LOG_ERROR("Failed to allocate memory for physics bodies");
        free(world);
        return NULL;
    }
    
    LOG_INFO("Physics world created successfully");
    return world;
}

void physics_world_destroy(PhysicsWorld* world) {
    if (!world) return;
    
    // Destroy all bodies
    for (uint32_t i = 0; i < world->bodyCount; i++) {
        if (world->bodies[i].shape) {
            physics_shape_destroy(world->bodies[i].shape);
        }
    }
    
    if (world->bodies) {
        free(world->bodies);
    }
    
    free(world);
    LOG_INFO("Physics world destroyed");
}

void physics_world_step(PhysicsWorld* world, float deltaTime) {
    if (!world || deltaTime <= 0.0f) return;
    
    // Simple integration for now
    for (uint32_t i = 0; i < world->bodyCount; i++) {
        PhysicsBody* body = &world->bodies[i];
        if (body->type == PHYSICS_BODY_STATIC || !body->isAwake) continue;
        
        // Apply gravity
        Vec3 gravityForce = vec3_multiply(&world->gravity, body->mass);
        Vec3 acceleration = vec3_add(&body->acceleration, &gravityForce);
        
        // Integrate velocity
        Vec3 deltaVelocity = vec3_multiply(&acceleration, deltaTime);
        body->velocity = vec3_add(&body->velocity, &deltaVelocity);
        
        // Integrate position
        Vec3 deltaPosition = vec3_multiply(&body->velocity, deltaTime);
        body->position = vec3_add(&body->position, &deltaPosition);
        
        // Update transform
        Mat4 translation = mat4_translation(&body->position);
        body->transform = translation;
        
        // Apply damping
        body->velocity = vec3_multiply(&body->velocity, 0.99f);
        
        // Check if body should sleep
        float speed = vec3_length(&body->velocity);
        if (speed < world->sleepThreshold && world->autoSleep) {
            body->isSleeping = true;
            body->isAwake = false;
        }
    }
    
    LOG_DEBUG("Physics world stepped with dt: %.3f", deltaTime);
}

void physics_world_set_gravity(PhysicsWorld* world, const Vec3* gravity) {
    if (!world || !gravity) return;
    world->gravity = *gravity;
    LOG_DEBUG("Physics world gravity set to (%.2f, %.2f, %.2f)", gravity->x, gravity->y, gravity->z);
}

// Body management
uint64_t physics_body_create(PhysicsWorld* world, PhysicsBodyType type, const PhysicsShape* shape, const Vec3* position) {
    if (!world || !shape || !position || world->bodyCount >= world->maxBodies) {
        return 0;
    }
    
    uint64_t bodyId = world->bodyCount + 1; // 1-based indexing
    PhysicsBody* body = &world->bodies[world->bodyCount];
    
    memset(body, 0, sizeof(PhysicsBody));
    body->id = bodyId;
    body->type = type;
    body->position = *position;
    body->transform = mat4_translation(position);
    body->mass = 1.0f;
    body->inverseMass = 1.0f / body->mass;
    body->isAwake = (type != PHYSICS_BODY_STATIC);
    
    // Copy shape
    body->shape = malloc(sizeof(PhysicsShape));
    if (body->shape) {
        memcpy(body->shape, shape, sizeof(PhysicsShape));
    }
    
    world->bodyCount++;
    LOG_DEBUG("Created physics body %lu with type %d", bodyId, type);
    return bodyId;
}

void physics_body_destroy(PhysicsWorld* world, uint64_t bodyId) {
    if (!world || bodyId == 0 || bodyId > world->bodyCount) return;
    
    PhysicsBody* body = &world->bodies[bodyId - 1];
    if (body->shape) {
        physics_shape_destroy(body->shape);
        body->shape = NULL;
    }
    
    LOG_DEBUG("Destroyed physics body %lu", bodyId);
}

PhysicsBody* physics_body_get(PhysicsWorld* world, uint64_t bodyId) {
    if (!world || bodyId == 0 || bodyId > world->bodyCount) return NULL;
    return &world->bodies[bodyId - 1];
}

void physics_body_set_transform(PhysicsWorld* world, uint64_t bodyId, const Mat4* transform) {
    PhysicsBody* body = physics_body_get(world, bodyId);
    if (!body || !transform) return;
    
    body->transform = *transform;
    body->position.x = transform->m[12];
    body->position.y = transform->m[13];
    body->position.z = transform->m[14];
    
    LOG_DEBUG("Set transform for body %lu", bodyId);
}

void physics_body_set_velocity(PhysicsWorld* world, uint64_t bodyId, const Vec3* velocity) {
    PhysicsBody* body = physics_body_get(world, bodyId);
    if (!body || !velocity) return;
    
    body->velocity = *velocity;
    body->isAwake = true;
    body->isSleeping = false;
    
    LOG_DEBUG("Set velocity for body %lu", bodyId);
}

void physics_body_set_angular_velocity(PhysicsWorld* world, uint64_t bodyId, const Vec3* angularVelocity) {
    PhysicsBody* body = physics_body_get(world, bodyId);
    if (!body || !angularVelocity) return;
    
    body->angularVelocity = *angularVelocity;
    body->isAwake = true;
    body->isSleeping = false;
    
    LOG_DEBUG("Set angular velocity for body %lu", bodyId);
}

void physics_body_apply_force(PhysicsWorld* world, uint64_t bodyId, const Vec3* force, const Vec3* point) {
    PhysicsBody* body = physics_body_get(world, bodyId);
    if (!body || !force || body->type == PHYSICS_BODY_STATIC) return;
    
    Vec3 acceleration = vec3_multiply(force, body->inverseMass);
    body->acceleration = vec3_add(&body->acceleration, &acceleration);
    body->isAwake = true;
    body->isSleeping = false;
    
    LOG_DEBUG("Applied force to body %lu", bodyId);
}

void physics_body_apply_impulse(PhysicsWorld* world, uint64_t bodyId, const Vec3* impulse, const Vec3* point) {
    PhysicsBody* body = physics_body_get(world, bodyId);
    if (!body || !impulse || body->type == PHYSICS_BODY_STATIC) return;
    
    Vec3 deltaVelocity = vec3_multiply(impulse, body->inverseMass);
    body->velocity = vec3_add(&body->velocity, &deltaVelocity);
    body->isAwake = true;
    body->isSleeping = false;
    
    LOG_DEBUG("Applied impulse to body %lu", bodyId);
}

// Shape creation
PhysicsShape* physics_shape_create_sphere(float radius, const PhysicsMaterial* material) {
    if (radius <= 0.0f) return NULL;
    
    PhysicsShape* shape = malloc(sizeof(PhysicsShape));
    if (!shape) return NULL;
    
    memset(shape, 0, sizeof(PhysicsShape));
    shape->type = PHYSICS_SHAPE_SPHERE;
    shape->geometry.sphere.radius = radius;
    
    if (material) {
        shape->material = *material;
    } else {
        shape->material.friction = 0.5f;
        shape->material.restitution = 0.3f;
        shape->material.density = 1000.0f;
    }
    
    LOG_DEBUG("Created sphere shape with radius %.2f", radius);
    return shape;
}

PhysicsShape* physics_shape_create_box(const Vec3* halfExtents, const PhysicsMaterial* material) {
    if (!halfExtents) return NULL;
    
    PhysicsShape* shape = malloc(sizeof(PhysicsShape));
    if (!shape) return NULL;
    
    memset(shape, 0, sizeof(PhysicsShape));
    shape->type = PHYSICS_SHAPE_BOX;
    shape->geometry.box.halfExtents = *halfExtents;
    
    if (material) {
        shape->material = *material;
    } else {
        shape->material.friction = 0.5f;
        shape->material.restitution = 0.3f;
        shape->material.density = 1000.0f;
    }
    
    LOG_DEBUG("Created box shape with half extents (%.2f, %.2f, %.2f)", halfExtents->x, halfExtents->y, halfExtents->z);
    return shape;
}

PhysicsShape* physics_shape_create_capsule(float radius, float height, const PhysicsMaterial* material) {
    if (radius <= 0.0f || height <= 0.0f) return NULL;
    
    PhysicsShape* shape = malloc(sizeof(PhysicsShape));
    if (!shape) return NULL;
    
    memset(shape, 0, sizeof(PhysicsShape));
    shape->type = PHYSICS_SHAPE_CAPSULE;
    shape->geometry.capsule.radius = radius;
    shape->geometry.capsule.height = height;
    
    if (material) {
        shape->material = *material;
    } else {
        shape->material.friction = 0.5f;
        shape->material.restitution = 0.3f;
        shape->material.density = 1000.0f;
    }
    
    LOG_DEBUG("Created capsule shape with radius %.2f, height %.2f", radius, height);
    return shape;
}

PhysicsShape* physics_shape_create_mesh(const void* vertices, uint32_t vertexCount, const void* indices, uint32_t indexCount, const PhysicsMaterial* material) {
    if (!vertices || vertexCount == 0) return NULL;
    
    PhysicsShape* shape = malloc(sizeof(PhysicsShape));
    if (!shape) return NULL;
    
    memset(shape, 0, sizeof(PhysicsShape));
    shape->type = PHYSICS_SHAPE_MESH;
    shape->geometry.mesh.vertices = (void*)vertices;
    shape->geometry.mesh.vertexCount = vertexCount;
    shape->geometry.mesh.indices = (void*)indices;
    shape->geometry.mesh.indexCount = indexCount;
    
    if (material) {
        shape->material = *material;
    } else {
        shape->material.friction = 0.5f;
        shape->material.restitution = 0.3f;
        shape->material.density = 1000.0f;
    }
    
    LOG_DEBUG("Created mesh shape with %u vertices, %u indices", vertexCount, indexCount);
    return shape;
}

void physics_shape_destroy(PhysicsShape* shape) {
    if (!shape) return;
    
    // Free mesh data if needed
    if (shape->type == PHYSICS_SHAPE_MESH) {
        // Note: In a real implementation, we'd need to handle memory management carefully
        // For now, we assume the vertex/index data is managed elsewhere
    }
    
    free(shape);
    LOG_DEBUG("Destroyed physics shape");
}

// Collision detection
bool physics_ray_cast(PhysicsWorld* world, const Vec3* start, const Vec3* direction, float maxDistance, RaycastResult* result) {
    if (!world || !start || !direction || !result) return false;
    
    // Simple ray casting implementation
    Vec3 normalizedDir = vec3_normalize(direction);
    float closestDistance = maxDistance;
    bool hit = false;
    
    for (uint32_t i = 0; i < world->bodyCount; i++) {
        PhysicsBody* body = &world->bodies[i];
        if (!body->shape) continue;
        
        // Simple sphere intersection for now
        if (body->shape->type == PHYSICS_SHAPE_SPHERE) {
            float radius = body->shape->geometry.sphere.radius;
            Vec3 toCenter = vec3_subtract(&body->position, start);
            float projection = vec3_dot(&toCenter, &normalizedDir);
            
            if (projection > 0.0f && projection < maxDistance) {
                Vec3 scaledDir = vec3_multiply_value(&normalizedDir, projection);
                Vec3 closestPoint = vec3_add(start, &scaledDir);
                Vec3 toClosest = vec3_subtract(&body->position, &closestPoint);
                float distance = vec3_length(&toClosest);
                
                if (distance <= radius && projection < closestDistance) {
                    closestDistance = projection;
                    result->hit = true;
                    result->point = closestPoint;
                    result->normal = vec3_normalize(&toClosest);
                    result->distance = projection;
                    result->bodyId = body->id;
                    result->userData = body->userData;
                    hit = true;
                }
            }
        }
    }
    
    return hit;
}

uint32_t physics_get_contacts(PhysicsWorld* world, ContactManifold* contacts, uint32_t maxContacts) {
    if (!world || !contacts || maxContacts == 0) return 0;
    
    // Simple contact detection
    uint32_t contactCount = 0;
    
    for (uint32_t i = 0; i < world->bodyCount && contactCount < maxContacts; i++) {
        for (uint32_t j = i + 1; j < world->bodyCount && contactCount < maxContacts; j++) {
            PhysicsBody* bodyA = &world->bodies[i];
            PhysicsBody* bodyB = &world->bodies[j];
            
            if (!bodyA->shape || !bodyB->shape) continue;
            
            // Simple sphere-sphere collision
            if (bodyA->shape->type == PHYSICS_SHAPE_SPHERE && bodyB->shape->type == PHYSICS_SHAPE_SPHERE) {
                Vec3 diff = vec3_subtract(&bodyB->position, &bodyA->position);
                float distance = vec3_length(&diff);
                float radiusSum = bodyA->shape->geometry.sphere.radius + bodyB->shape->geometry.sphere.radius;
                
                if (distance < radiusSum) {
                    ContactManifold* contact = &contacts[contactCount];
                    contact->normal = vec3_normalize(&diff);
                    contact->penetration = radiusSum - distance;
                    contact->bodyA = bodyA->id;
                    contact->bodyB = bodyB->id;
                    contact->pointCount = 1;
                    Vec3 scaledNormal = vec3_multiply_value(&contact->normal, bodyA->shape->geometry.sphere.radius);
                    contact->points[0] = vec3_add(&bodyA->position, &scaledNormal);
                    
                    contactCount++;
                }
            }
        }
    }
    
    return contactCount;
}
