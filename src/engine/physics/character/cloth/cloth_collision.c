#include "cloth_common.h"
#include <stdlib.h>

void cloth_setup_collision(ClothMesh* cloth, uint32_t max_colliders) {
    if (!cloth) return;
    
    // Create broad collider buffer
    cloth->colliders_buffer = [cloth->particles_buffer.device newBufferWithLength:sizeof(ClothCollider) * max_colliders 
                                                                         options:MTLResourceStorageModeShared];
    cloth->collider_count = 0; // Starts empty
}

void cloth_add_sphere_collider(ClothMesh* cloth, simd_float3 position, float radius) {
    if (!cloth || !cloth->colliders_buffer) return;
    
    // Simplistic check for capacity, real engine would track capacity vs count
    // Assuming we have space for now
    
    ClothCollider* colliders = (ClothCollider*)cloth->colliders_buffer.contents;
    ClothCollider* c = &colliders[cloth->collider_count++];
    
    c->position = position;
    c->radius = radius;
    c->type = COLLIDER_SPHERE;
    c->axis = simd_make_float3(0,0,0);
    c->height = 0;
}

void cloth_add_capsule_collider(ClothMesh* cloth, simd_float3 position, simd_float3 axis, float radius, float height) {
    if (!cloth || !cloth->colliders_buffer) return;
    
    ClothCollider* colliders = (ClothCollider*)cloth->colliders_buffer.contents;
    ClothCollider* c = &colliders[cloth->collider_count++];
    
    c->position = position;
    c->radius = radius;
    c->type = COLLIDER_CAPSULE;
    c->axis = axis; // Direction or offset
    c->height = height;
}

// Host-side collision solving is rarely used with GPU cloth, 
// usually we dispatch a compute shader for this.
// So this file mainly manages the data structures passed to GPU.
