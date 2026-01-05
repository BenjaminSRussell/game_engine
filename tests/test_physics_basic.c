#include "../src/engine/physics/physics_engine_core.h"
#include <stdio.h>
#include <assert.h>
#include <math.h>

// Mock shape functions since we haven't implemented collision shapes yet
void shape_destroy(CollisionShape *shape) {}

// Test 1: Rigid Body Creation and Defaults
void test_creation() {
    printf("Test 1: Creation... ");
    RigidBody *rb = rigid_body_create(1, RIGID_BODY_DYNAMIC);
    assert(rb != NULL);
    assert(rb->id == 1);
    assert(rb->mass == 1.0f);
    assert(rb->is_active == true);
    rigid_body_destroy(rb);
    printf("PASSED\n");
}

// Test 2: Integration (Gravity)
void test_integration() {
    printf("Test 2: Integration... ");
    RigidBody *rb = rigid_body_create(1, RIGID_BODY_DYNAMIC);
    rb->linear_damping = 0.0f; // Disable damping for gravity test
    
    // Simulate 1 second of gravity drop
    float dt = 0.016f; // 60 FPS
    float gravity = -9.8f;
    float total_time = 1.0f;
    int steps = (int)(total_time / dt);
    
    for (int i = 0; i < steps; i++) {
        // Apply gravity force: F = mg
        float force[3] = {0, rb->mass * gravity, 0};
        rigid_body_apply_force(rb, force);
        rigid_body_integrate(rb, dt);
    }
    
    // Expected velocity: v = a*t = -9.8 * 1.0 = -9.8
    // Expected position: p = 0.5*a*t^2 = 0.5 * -9.8 * 1.0 = -4.9
    
    printf("Pos Y: %.2f, Vel Y: %.2f\n", rb->position[1], rb->velocity[1]);
    
    assert(fabs(rb->velocity[1] - (-9.8f)) < 0.2f); // Allow some Euler error
    assert(fabs(rb->position[1] - (-4.9f)) < 0.2f);
    
    rigid_body_destroy(rb);
    printf("PASSED\n");
}

// Test 3: Force Application (Torque)
void test_torque() {
    printf("Test 3: Torque... ");
    RigidBody *rb = rigid_body_create(1, RIGID_BODY_DYNAMIC);
    
    // Apply force at offset to create torque
    // Force (0, 10, 0) at Point (1, 0, 0) -> CoM is (0,0,0)
    // Torque = r x F = (1,0,0) x (0,10,0) = (0, 0, 10)
    float force[3] = {0, 10.0f, 0};
    float point[3] = {1.0f, 0, 0};
    
    rigid_body_apply_force_at_point(rb, force, point);
    
    assert(fabs(rb->accumulated_torque[2] - 10.0f) < 0.001f);
    
    rigid_body_destroy(rb);
    printf("PASSED\n");
}

int main() {
    printf("=== Running Physics Basic Tests ===\n");
    test_creation();
    test_integration();
    test_torque();
    printf("All tests passed!\n");
    return 0;
}
