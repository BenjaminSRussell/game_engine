// Advanced Physics System
// Provides advanced physics simulation capabilities

#ifndef ADVANCED_PHYSICS_H
#define ADVANCED_PHYSICS_H

#include <stdbool.h>
#include <stdint.h>
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

// Physics vector types
typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float x, y, z, w;
} Vec4;

typedef struct {
    float m[16]; // Column-major 4x4 matrix
} Mat4;

// Physics body types
typedef enum {
    PHYSICS_BODY_STATIC,
    PHYSICS_BODY_DYNAMIC,
    PHYSICS_BODY_KINEMATIC
} PhysicsBodyType;

// Physics shape types
typedef enum {
    PHYSICS_SHAPE_SPHERE,
    PHYSICS_SHAPE_BOX,
    PHYSICS_SHAPE_CAPSULE,
    PHYSICS_SHAPE_MESH,
    PHYSICS_SHAPE_CONVEX_HULL
} PhysicsShapeType;

// Physics material properties
typedef struct {
    float friction;
    float restitution;
    float density;
    float rollingFriction;
    float spinningFriction;
} PhysicsMaterial;

// Physics collision shape
typedef struct {
    PhysicsShapeType type;
    union {
        struct { float radius; } sphere;
        struct { Vec3 halfExtents; } box;
        struct { float radius, height; } capsule;
        struct { void* vertices; uint32_t vertexCount; void* indices; uint32_t indexCount; } mesh;
    } geometry;
    PhysicsMaterial material;
} PhysicsShape;

// Physics rigid body
typedef struct {
    uint64_t id;
    PhysicsBodyType type;
    PhysicsShape* shape;
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    Vec3 angularVelocity;
    Vec3 angularAcceleration;
    Mat4 transform;
    float mass;
    float inverseMass;
    bool isAwake;
    bool isSleeping;
    void* userData;
} PhysicsBody;

// Physics world
typedef struct {
    Vec3 gravity;
    float timeStep;
    uint32_t maxSubSteps;
    bool autoSleep;
    float sleepThreshold;
    PhysicsBody* bodies;
    uint32_t bodyCount;
    uint32_t maxBodies;
    void* broadphase;
    void* narrowphase;
    void* constraintSolver;
} PhysicsWorld;

// Contact manifold for collision detection
typedef struct {
    Vec3 points[4];
    Vec3 normal;
    float penetration;
    uint32_t pointCount;
    uint64_t bodyA;
    uint64_t bodyB;
} ContactManifold;

// Ray cast result
typedef struct {
    bool hit;
    Vec3 point;
    Vec3 normal;
    float distance;
    uint64_t bodyId;
    void* userData;
} RaycastResult;

// Function declarations
PhysicsWorld* physics_world_create(const Vec3* gravity, float timeStep);
void physics_world_destroy(PhysicsWorld* world);
void physics_world_step(PhysicsWorld* world, float deltaTime);
void physics_world_set_gravity(PhysicsWorld* world, const Vec3* gravity);

// Body management
uint64_t physics_body_create(PhysicsWorld* world, PhysicsBodyType type, const PhysicsShape* shape, const Vec3* position);
void physics_body_destroy(PhysicsWorld* world, uint64_t bodyId);
PhysicsBody* physics_body_get(PhysicsWorld* world, uint64_t bodyId);
void physics_body_set_transform(PhysicsWorld* world, uint64_t bodyId, const Mat4* transform);
void physics_body_set_velocity(PhysicsWorld* world, uint64_t bodyId, const Vec3* velocity);
void physics_body_set_angular_velocity(PhysicsWorld* world, uint64_t bodyId, const Vec3* angularVelocity);
void physics_body_apply_force(PhysicsWorld* world, uint64_t bodyId, const Vec3* force, const Vec3* point);
void physics_body_apply_impulse(PhysicsWorld* world, uint64_t bodyId, const Vec3* impulse, const Vec3* point);

// Shape creation
PhysicsShape* physics_shape_create_sphere(float radius, const PhysicsMaterial* material);
PhysicsShape* physics_shape_create_box(const Vec3* halfExtents, const PhysicsMaterial* material);
PhysicsShape* physics_shape_create_capsule(float radius, float height, const PhysicsMaterial* material);
PhysicsShape* physics_shape_create_mesh(const void* vertices, uint32_t vertexCount, const void* indices, uint32_t indexCount, const PhysicsMaterial* material);
void physics_shape_destroy(PhysicsShape* shape);

// Collision detection
bool physics_ray_cast(PhysicsWorld* world, const Vec3* start, const Vec3* direction, float maxDistance, RaycastResult* result);
uint32_t physics_sphere_cast(PhysicsWorld* world, const Vec3* center, float radius, uint64_t* results, uint32_t maxResults);
uint32_t physics_box_cast(PhysicsWorld* world, const Vec3* center, const Vec3* halfExtents, uint64_t* results, uint32_t maxResults);
uint32_t physics_get_contacts(PhysicsWorld* world, ContactManifold* contacts, uint32_t maxContacts);

// Utility functions
Vec3 vec3_make(float x, float y, float z);
Vec3 vec3_add(const Vec3* a, const Vec3* b);
Vec3 vec3_subtract(const Vec3* a, const Vec3* b);
Vec3 vec3_multiply(const Vec3* v, float scalar);
Vec3 vec3_cross(const Vec3* a, const Vec3* b);
float vec3_dot(const Vec3* a, const Vec3* b);
float vec3_length(const Vec3* v);
Vec3 vec3_normalize(const Vec3* v);
Mat4 mat4_identity(void);
Mat4 mat4_translation(const Vec3* translation);
Mat4 mat4_rotation(const Vec3* axis, float angle);
Mat4 mat4_scale(const Vec3* scale);
Mat4 mat4_multiply(const Mat4* a, const Mat4* b);

#ifdef __cplusplus
}
#endif

#endif // ADVANCED_PHYSICS_H
