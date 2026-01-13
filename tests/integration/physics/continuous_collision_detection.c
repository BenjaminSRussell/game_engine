// continuous_collision_detection.c
//
// Purpose: Continuous collision detection system for high-speed objects
// Implements TODO-0043: Continuous collision detection on high-speed objects

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

// Vector3 structure
typedef struct {
    float x, y, z;
} Vec3;

// Vector operations
static inline Vec3 vec3_add(Vec3 a, Vec3 b) { return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline Vec3 vec3_sub(Vec3 a, Vec3 b) { return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline Vec3 vec3_mul(Vec3 v, float s) { return (Vec3){v.x * s, v.y * s, v.z * s}; }
static inline Vec3 vec3_div(Vec3 v, float s) { return (Vec3){v.x / s, v.y / s, v.z / s}; }
static inline float vec3_dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline float vec3_length_sq(Vec3 v) { return vec3_dot(v, v); }
static inline float vec3_length(Vec3 v) { return sqrtf(vec3_length_sq(v)); }
static inline Vec3 vec3_normalize(Vec3 v) { 
    float len = vec3_length(v);
    return len > 0.0f ? vec3_div(v, len) : (Vec3){0, 0, 0};
}
static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// Bounding volume types
typedef enum {
    BV_TYPE_SPHERE,
    BV_TYPE_AABB,
    BV_TYPE_OBB,
    BV_TYPE_CAPSULE,
    BV_TYPE_COUNT
} BoundingVolumeType;

// Bounding volume structures
typedef struct {
    Vec3 center;
    float radius;
} BoundingSphere;

typedef struct {
    Vec3 min;
    Vec3 max;
} AABB;

typedef struct {
    Vec3 center;
    Vec3 extents; // Half-dimensions
    Vec3 axes[3]; // Local coordinate axes
} OBB;

typedef struct {
    Vec3 start;
    Vec3 end;
    float radius;
} Capsule;

// Unified bounding volume
typedef struct {
    BoundingVolumeType type;
    union {
        BoundingSphere sphere;
        AABB aabb;
        OBB obb;
        Capsule capsule;
    };
} BoundingVolume;

// Rigid body with continuous collision detection
typedef struct {
    uint32_t id;
    BoundingVolume bv;
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    float mass;
    float restitution;
    float friction;
    bool is_high_speed;
    bool is_static;
    uint32_t collision_mask;
    uint32_t collision_layer;
} RigidBody;

// Continuous collision detection result
typedef struct {
    bool hit;
    float time_of_impact;
    Vec3 contact_point;
    Vec3 contact_normal;
    Vec3 penetration_depth;
    uint32_t body_a_id;
    uint32_t body_b_id;
} CCDResult;

// CCD System
typedef struct {
    RigidBody* bodies;
    uint32_t capacity;
    uint32_t count;
    CCDResult* results;
    uint32_t result_capacity;
    uint32_t result_count;
    float max_timestep;
    float ccd_threshold;
    bool is_initialized;
} CCDSystem;

// Global CCD system
static CCDSystem g_ccd_system = {0};

// Initialize CCD system
bool ccd_system_init(uint32_t max_bodies, float max_timestep, float ccd_threshold) {
    printf("Initializing CCD system with %u bodies, max timestep: %.4f, threshold: %.4f\n",
           max_bodies, max_timestep, ccd_threshold);
    
    g_ccd_system.bodies = calloc(max_bodies, sizeof(RigidBody));
    if (!g_ccd_system.bodies) {
        printf("Error: Failed to allocate bodies array\n");
        return false;
    }
    
    g_ccd_system.results = calloc(max_bodies * 4, sizeof(CCDResult)); // Assume max 4 collisions per body
    if (!g_ccd_system.results) {
        free(g_ccd_system.bodies);
        printf("Error: Failed to allocate results array\n");
        return false;
    }
    
    g_ccd_system.capacity = max_bodies;
    g_ccd_system.result_capacity = max_bodies * 4;
    g_ccd_system.max_timestep = max_timestep;
    g_ccd_system.ccd_threshold = ccd_threshold;
    g_ccd_system.is_initialized = true;
    
    printf("CCD system initialized successfully\n");
    return true;
}

// Add rigid body to CCD system
uint32_t ccd_add_body(const RigidBody* body) {
    if (!g_ccd_system.is_initialized || g_ccd_system.count >= g_ccd_system.capacity) {
        printf("Error: CCD system not initialized or at capacity\n");
        return 0;
    }
    
    g_ccd_system.bodies[g_ccd_system.count] = *body;
    g_ccd_system.bodies[g_ccd_system.count].id = g_ccd_system.count + 1;
    
    // Check if this is a high-speed object
    float speed = vec3_length(body->velocity);
    g_ccd_system.bodies[g_ccd_system.count].is_high_speed = (speed > g_ccd_system.ccd_threshold);
    
    printf("Added body %u: speed=%.2f, high_speed=%s\n",
           g_ccd_system.bodies[g_ccd_system.count].id, speed,
           g_ccd_system.bodies[g_ccd_system.count].is_high_speed ? "YES" : "NO");
    
    return g_ccd_system.bodies[g_ccd_system.count++].id;
}

// Ray-sphere intersection
static bool ray_sphere_intersect(Vec3 ray_start, Vec3 ray_dir, Vec3 sphere_center, 
                               float sphere_radius, float* t_min, float* t_max) {
    Vec3 oc = vec3_sub(ray_start, sphere_center);
    float a = vec3_dot(ray_dir, ray_dir);
    float b = 2.0f * vec3_dot(oc, ray_dir);
    float c = vec3_dot(oc, oc) - sphere_radius * sphere_radius;
    
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0.0f) return false;
    
    float sqrt_disc = sqrtf(discriminant);
    float t1 = (-b - sqrt_disc) / (2.0f * a);
    float t2 = (-b + sqrt_disc) / (2.0f * a);
    
    if (t1 > t2) {
        float temp = t1; t1 = t2; t2 = temp;
    }
    
    *t_min = t1;
    *t_max = t2;
    return true;
}

// Sphere-sweep sphere intersection (continuous collision detection)
static bool sphere_sweep_sphere(const BoundingSphere* sphere_a, const Vec3* velocity_a,
                               const BoundingSphere* sphere_b, const Vec3* velocity_b,
                               float* time_of_impact, Vec3* contact_point) {
    Vec3 relative_velocity = vec3_sub(*velocity_a, *velocity_b);
    Vec3 start_center = vec3_sub(sphere_a->center, sphere_b->center);
    float radius_sum = sphere_a->radius + sphere_b->radius;
    
    // Quadratic equation coefficients
    float a = vec3_dot(relative_velocity, relative_velocity);
    float b = 2.0f * vec3_dot(start_center, relative_velocity);
    float c = vec3_dot(start_center, start_center) - radius_sum * radius_sum;
    
    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f || a == 0.0f) return false;
    
    float sqrt_disc = sqrtf(discriminant);
    float t1 = (-b - sqrt_disc) / (2.0f * a);
    float t2 = (-b + sqrt_disc) / (2.0f * a);
    
    // We want the first positive time of impact
    float t = (t1 > 0.0f) ? t1 : t2;
    if (t < 0.0f || t > 1.0f) return false;
    
    *time_of_impact = t;
    
    // Calculate contact point
    Vec3 sphere_a_at_impact = vec3_add(sphere_a->center, vec3_mul(*velocity_a, t));
    Vec3 sphere_b_at_impact = vec3_add(sphere_b->center, vec3_mul(*velocity_b, t));
    Vec3 normal = vec3_normalize(vec3_sub(sphere_a_at_impact, sphere_b_at_impact));
    *contact_point = vec3_add(sphere_b_at_impact, vec3_mul(normal, sphere_b->radius));
    
    return true;
}

// AABB-sweep AABB intersection
static bool aabb_sweep_aabb(const AABB* aabb_a, const Vec3* velocity_a,
                            const AABB* aabb_b, const Vec3* velocity_b,
                            float* time_of_impact, Vec3* contact_normal) {
    Vec3 relative_velocity = vec3_sub(*velocity_a, *velocity_b);
    
    // Check for separation in each axis
    float t_enter = 0.0f;
    float t_exit = 1.0f;
    Vec3 normal = {0};
    
    for (int i = 0; i < 3; i++) {
        float a_min = (i == 0) ? aabb_a->min.x : (i == 1) ? aabb_a->min.y : aabb_a->min.z;
        float a_max = (i == 0) ? aabb_a->max.x : (i == 1) ? aabb_a->max.y : aabb_a->max.z;
        float b_min = (i == 0) ? aabb_b->min.x : (i == 1) ? aabb_b->min.y : aabb_b->min.z;
        float b_max = (i == 0) ? aabb_b->max.x : (i == 1) ? aabb_b->max.y : aabb_b->max.z;
        float v = (i == 0) ? relative_velocity.x : (i == 1) ? relative_velocity.y : relative_velocity.z;
        
        if (v == 0.0f) {
            // No relative motion in this axis
            if (a_max < b_min || a_min > b_max) return false;
        } else {
            float t1 = (b_min - a_max) / v;
            float t2 = (b_max - a_min) / v;
            
            if (t1 > t2) {
                float temp = t1; t1 = t2; t2 = temp;
            }
            
            if (t1 > t_enter) {
                t_enter = t1;
                normal = (i == 0) ? (Vec3){v < 0 ? 1 : -1, 0, 0} :
                        (i == 1) ? (Vec3){0, v < 0 ? 1 : -1, 0} :
                        (Vec3){0, 0, v < 0 ? 1 : -1};
            }
            
            if (t2 < t_exit) t_exit = t2;
            
            if (t_enter > t_exit) return false;
        }
    }
    
    if (t_enter < 0.0f || t_enter > 1.0f) return false;
    
    *time_of_impact = t_enter;
    *contact_normal = vec3_normalize(normal);
    return true;
}

// Perform continuous collision detection between two bodies
static bool ccd_body_pair(const RigidBody* body_a, const RigidBody* body_b, 
                          float timestep, CCDResult* result) {
    // Skip static-static pairs
    if (body_a->is_static && body_b->is_static) return false;
    
    // Check collision mask
    if ((body_a->collision_mask & body_b->collision_layer) == 0 &&
        (body_b->collision_mask & body_a->collision_layer) == 0) {
        return false;
    }
    
    // Only perform CCD on high-speed objects or when colliding with high-speed objects
    if (!body_a->is_high_speed && !body_b->is_high_speed) {
        return false; // Use discrete collision detection for normal-speed objects
    }
    
    // Calculate swept positions
    Vec3 velocity_a = vec3_mul(body_a->velocity, timestep);
    Vec3 velocity_b = vec3_mul(body_b->velocity, timestep);
    
    bool hit = false;
    float toi = 1.0f;
    Vec3 contact_point = {0};
    Vec3 contact_normal = {0};
    
    // Perform CCD based on bounding volume types
    if (body_a->bv.type == BV_TYPE_SPHERE && body_b->bv.type == BV_TYPE_SPHERE) {
        hit = sphere_sweep_sphere(&body_a->bv.sphere, &velocity_a,
                                 &body_b->bv.sphere, &velocity_b,
                                 &toi, &contact_point);
        contact_normal = vec3_normalize(vec3_sub(
            vec3_add(body_a->bv.sphere.center, vec3_mul(velocity_a, toi)),
            vec3_add(body_b->bv.sphere.center, vec3_mul(velocity_b, toi))));
    } else if (body_a->bv.type == BV_TYPE_AABB && body_b->bv.type == BV_TYPE_AABB) {
        hit = aabb_sweep_aabb(&body_a->bv.aabb, &velocity_a,
                              &body_b->bv.aabb, &velocity_b,
                              &toi, &contact_normal);
        // Calculate contact point for AABB
        Vec3 a_center = vec3_mul(vec3_add(body_a->bv.aabb.min, body_a->bv.aabb.max), 0.5f);
        Vec3 b_center = vec3_mul(vec3_add(body_b->bv.aabb.min, body_b->bv.aabb.max), 0.5f);
        contact_point = vec3_add(b_center, vec3_mul(contact_normal, 
            vec3_length(vec3_sub(a_center, b_center)) * 0.5f));
    }
    // Add more BV type combinations as needed...
    
    if (hit && toi >= 0.0f && toi <= 1.0f) {
        result->hit = true;
        result->time_of_impact = toi * timestep;
        result->contact_point = contact_point;
        result->contact_normal = contact_normal;
        result->body_a_id = body_a->id;
        result->body_b_id = body_b->id;
        return true;
    }
    
    return false;
}

// Perform continuous collision detection for all bodies
uint32_t ccd_update(float timestep) {
    if (!g_ccd_system.is_initialized) return 0;
    
    g_ccd_system.result_count = 0;
    
    // Clamp timestep to maximum allowed
    float clamped_timestep = fminf(timestep, g_ccd_system.max_timestep);
    
    // Check all body pairs
    for (uint32_t i = 0; i < g_ccd_system.count; i++) {
        for (uint32_t j = i + 1; j < g_ccd_system.count; j++) {
            const RigidBody* body_a = &g_ccd_system.bodies[i];
            const RigidBody* body_b = &g_ccd_system.bodies[j];
            
            CCDResult result = {0};
            if (ccd_body_pair(body_a, body_b, clamped_timestep, &result)) {
                if (g_ccd_system.result_count < g_ccd_system.result_capacity) {
                    g_ccd_system.results[g_ccd_system.result_count++] = result;
                }
            }
        }
    }
    
    printf("CCD update: timestep=%.4f, found %u collisions\n", clamped_timestep, g_ccd_system.result_count);
    return g_ccd_system.result_count;
}

// Get CCD results
const CCDResult* ccd_get_results(uint32_t* count) {
    if (!g_ccd_system.is_initialized) return NULL;
    *count = g_ccd_system.result_count;
    return g_ccd_system.results;
}

// Test high-speed bullet scenario
bool test_high_speed_bullet(void) {
    printf("\n=== High-Speed Bullet Test ===\n");
    
    bool test_passed = true;
    
    // Create a slow-moving target
    RigidBody target = {
        .bv = {.type = BV_TYPE_SPHERE, .sphere = {.center = {10, 0, 0}, .radius = 1.0f}},
        .position = {10, 0, 0},
        .velocity = {0, 0, 0},
        .mass = 100.0f,
        .is_static = false,
        .is_high_speed = false,
        .collision_mask = 0xFFFF,
        .collision_layer = 1
    };
    
    // Create a high-speed bullet
    RigidBody bullet = {
        .bv = {.type = BV_TYPE_SPHERE, .sphere = {.center = {0, 0, 0}, .radius = 0.1f}},
        .position = {0, 0, 0},
        .velocity = {100, 0, 0}, // 100 units/second
        .mass = 0.01f,
        .is_static = false,
        .is_high_speed = true,
        .collision_mask = 0xFFFF,
        .collision_layer = 2
    };
    
    uint32_t target_id = ccd_add_body(&target);
    uint32_t bullet_id = ccd_add_body(&bullet);
    
    // Update CCD system
    uint32_t collision_count = ccd_update(0.1f); // 100ms timestep
    
    if (collision_count == 0) {
        printf("Error: Bullet should have hit target\n");
        test_passed = false;
    } else {
        const CCDResult* results = ccd_get_results(&collision_count);
        printf("Bullet collision detected at t=%.4f\n", results[0].time_of_impact);
        printf("Contact point: (%.2f, %.2f, %.2f)\n",
               results[0].contact_point.x, results[0].contact_point.y, results[0].contact_point.z);
        
        // Verify collision is reasonable
        if (results[0].time_of_impact < 0.08f || results[0].time_of_impact > 0.12f) {
            printf("Error: Unexpected time of impact\n");
            test_passed = false;
        }
    }
    
    return test_passed;
}

// Test tunneling prevention
bool test_tunneling_prevention(void) {
    printf("\n=== Tunneling Prevention Test ===\n");
    
    bool test_passed = true;
    
    // Create a thin wall
    RigidBody wall = {
        .bv = {.type = BV_TYPE_AABB, .aabb = {.min = {5, -2, -2}, .max = {5.1f, 2, 2}}},
        .position = {5.05f, 0, 0},
        .velocity = {0, 0, 0},
        .mass = 1000.0f,
        .is_static = true,
        .is_high_speed = false,
        .collision_mask = 0xFFFF,
        .collision_layer = 1
    };
    
    // Create a very fast object that would normally tunnel
    RigidBody fast_object = {
        .bv = {.type = BV_TYPE_SPHERE, .sphere = {.center = {0, 0, 0}, .radius = 0.5f}},
        .position = {0, 0, 0},
        .velocity = {1000, 0, 0}, // Very fast!
        .mass = 1.0f,
        .is_static = false,
        .is_high_speed = true,
        .collision_mask = 0xFFFF,
        .collision_layer = 2
    };
    
    ccd_add_body(&wall);
    ccd_add_body(&fast_object);
    
    // Update with large timestep
    uint32_t collision_count = ccd_update(0.1f);
    
    if (collision_count == 0) {
        printf("Error: Fast object should have collided with wall\n");
        test_passed = false;
    } else {
        printf("Tunneling prevented: collision detected at t=%.4f\n",
               g_ccd_system.results[0].time_of_impact);
    }
    
    return test_passed;
}

// Test multiple high-speed objects
bool test_multiple_high_speed(void) {
    printf("\n=== Multiple High-Speed Objects Test ===\n");
    
    bool test_passed = true;
    uint32_t collision_count = 0;
    
    // Create multiple high-speed objects moving in different directions
    for (int i = 0; i < 5; i++) {
        float angle = (float)i * 2.0f * M_PI / 5.0f;
        RigidBody object = {
            .bv = {.type = BV_TYPE_SPHERE, .sphere = {.center = {0, 0, 0}, .radius = 0.5f}},
            .position = {cosf(angle) * 5.0f, sinf(angle) * 5.0f, 0},
            .velocity = {-cosf(angle) * 50.0f, -sinf(angle) * 50.0f, 0},
            .mass = 1.0f,
            .is_static = false,
            .is_high_speed = true,
            .collision_mask = 0xFFFF,
            .collision_layer = 1
        };
        ccd_add_body(&object);
    }
    
    // Update and check for collisions
    collision_count = ccd_update(0.2f);
    
    if (collision_count == 0) {
        printf("Error: Multiple high-speed objects should have collided\n");
        test_passed = false;
    } else {
        printf("Multiple collisions detected: %u\n", collision_count);
        
        // Verify all collisions are reasonable
        for (uint32_t i = 0; i < collision_count; i++) {
            if (g_ccd_system.results[i].time_of_impact < 0.0f || 
                g_ccd_system.results[i].time_of_impact > 0.2f) {
                printf("Error: Invalid time of impact in collision %u\n", i);
                test_passed = false;
            }
        }
    }
    
    return test_passed;
}

// Generate CCD performance report
void generate_ccd_report(void) {
    printf("\n=== CCD Performance Report ===\n");
    
    uint32_t high_speed_count = 0;
    uint32_t static_count = 0;
    float total_speed = 0.0f;
    float max_speed = 0.0f;
    
    for (uint32_t i = 0; i < g_ccd_system.count; i++) {
        const RigidBody* body = &g_ccd_system.bodies[i];
        float speed = vec3_length(body->velocity);
        total_speed += speed;
        max_speed = fmaxf(max_speed, speed);
        
        if (body->is_high_speed) high_speed_count++;
        if (body->is_static) static_count++;
    }
    
    printf("Total Bodies: %u\n", g_ccd_system.count);
    printf("High-Speed Bodies: %u (%.1f%%)\n", high_speed_count, 
           (float)high_speed_count / g_ccd_system.count * 100.0f);
    printf("Static Bodies: %u (%.1f%%)\n", static_count,
           (float)static_count / g_ccd_system.count * 100.0f);
    printf("Average Speed: %.2f\n", total_speed / g_ccd_system.count);
    printf("Maximum Speed: %.2f\n", max_speed);
    printf("CCD Threshold: %.2f\n", g_ccd_system.ccd_threshold);
    printf("Max Timestep: %.4f\n", g_ccd_system.max_timestep);
    printf("Recent Collisions: %u\n", g_ccd_system.result_count);
}

// Cleanup CCD system
void ccd_system_cleanup(void) {
    if (!g_ccd_system.is_initialized) return;
    
    printf("Cleaning up CCD system...\n");
    
    free(g_ccd_system.bodies);
    free(g_ccd_system.results);
    memset(&g_ccd_system, 0, sizeof(g_ccd_system));
    
    printf("CCD system cleaned up\n");
}

// Main verification function
int main(void) {
    printf("=== Continuous Collision Detection Verification ===\n");
    printf("Implementing TODO-0043: Continuous collision detection on high-speed objects\n\n");
    
    // Initialize CCD system
    if (!ccd_system_init(100, 0.1f, 10.0f)) {
        printf("Failed to initialize CCD system\n");
        return 1;
    }
    
    // Run tests
    bool bullet_test_passed = test_high_speed_bullet();
    bool tunneling_test_passed = test_tunneling_prevention();
    bool multiple_test_passed = test_multiple_high_speed();
    
    // Generate report
    generate_ccd_report();
    
    // Cleanup
    ccd_system_cleanup();
    
    printf("\n=== Verification Complete ===\n");
    bool all_passed = bullet_test_passed && tunneling_test_passed && multiple_test_passed;
    printf("Result: %s\n", all_passed ? "PASSED" : "FAILED");
    
    return all_passed ? 0 : 1;
}
