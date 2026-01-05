#include <stdio.h>
#include <stdlib.h>
#include "../src/engine/physics/physics_engine_core.h"
#include <assert.h>
#include <math.h>

int main() {
    printf("=== Running Physics Collision Tests ===\n");
    
    // Create Bodies
    RigidBody *b1 = rigid_body_create(1, RIGID_BODY_DYNAMIC);
    RigidBody *b2 = rigid_body_create(2, RIGID_BODY_DYNAMIC);
    
    // Create Shapes
    b1->shape = shape_sphere_create(1.0f);
    b2->shape = shape_sphere_create(1.0f);
    
    // Test 1: No Collision (separated by 3 units, radii sum = 2)
    printf("Test 1: Sphere Separation... ");
    float pos1[3] = {0, 0, 0};
    float pos2[3] = {3.0f, 0, 0};
    rigid_body_set_position(b1, pos1);
    rigid_body_set_position(b2, pos2);
    
    ContactManifold *m = manifold_create();
    bool hit = collision_sphere_sphere(b1, b2, m);
    assert(!hit);
    printf("PASSED\n");
    
    // Test 2: Collision (separated by 1.5 units, radii sum = 2)
    printf("Test 2: Sphere Overlap... ");
    float pos3[3] = {1.5f, 0, 0};
    rigid_body_set_position(b2, pos3);
    
    hit = collision_sphere_sphere(b1, b2, m);
    assert(hit);
    assert(m->point_count == 1);
    assert(fabs(m->points[0].penetration_depth - 0.5f) < 0.001f);
    // Normal should point A -> B (1, 0, 0)
    assert(m->points[0].normal[0] > 0.9f);
    printf("PASSED\n");
    
    // Test 3: Box AABB Overlap
    printf("Test 3: Box AABB Overlap... ");
    m->point_count = 0; // Reset manifold
    free(b1->shape); free(b2->shape);
    b1->shape = shape_box_create(0.5f, 0.5f, 0.5f); // 1x1x1 cube
    b2->shape = shape_box_create(0.5f, 0.5f, 0.5f);
    
    rigid_body_set_position(b1, pos1); // 0,0,0
    rigid_body_set_position(b2, pos3); // 1.5,0,0
    // Box 1 X range: [-0.5, 0.5]
    // Box 2 X range: [1.0, 2.0]
    // Gap = 0.5 => No collision
    hit = collision_box_box(b1, b2, m);
    assert(!hit);
    
    float pos4[3] = {0.8f, 0, 0};
    rigid_body_set_position(b2, pos4);
    // Box 2 X range: [0.3, 1.3]
    // Box 1 X range: [-0.5, 0.5]
    // Overlap: 0.5 - 0.3 = 0.2
    hit = collision_box_box(b1, b2, m);
    assert(hit);
    float pen = m->points[0].penetration_depth;
    printf("Box Penetration: %f (Expected: 0.2)\n", pen);
    assert(fabs(pen - 0.2f) < 0.001f);
    
    printf("PASSED\n");
    
    printf("All collision tests passed!\n");
    return 0;
}
