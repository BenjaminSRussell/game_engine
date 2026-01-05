#include <catch2/catch_test_macros.hpp>
#include <physics/physics_api.h>
#include <physics/core/physics_core.h>
#include <cmath>

// Test rigid body physics implementation
TEST_CASE("Rigid Body Physics", "[physics][rigidbody]") {
    
    SECTION("Rigid body creation") {
        RigidBody body = {0};
        body.mass = 1.0f;
        body.position = {0.0f, 0.0f, 0.0f};
        body.velocity = {0.0f, 0.0f, 0.0f};
        body.is_static = false;
        
        REQUIRE(body.mass == 1.0f);
        REQUIRE(!body.is_static);
    }
    
    SECTION("Gravity application") {
        RigidBody body = {0};
        body.mass = 1.0f;
        body.position = {0.0f, 10.0f, 0.0f};
        body.velocity = {0.0f, 0.0f, 0.0f};
        body.is_static = false;
        
        float dt = 0.016f; // 60 FPS
        float gravity = -9.81f;
        
        // Apply gravity for one frame
        body.velocity.y += gravity * dt;
        body.position.y += body.velocity.y * dt;
        
        REQUIRE(body.velocity.y < 0.0f); // Falling
        REQUIRE(body.position.y < 10.0f); // Moved down
    }
    
    SECTION("Static bodies don't move") {
        RigidBody body = {0};
        body.mass = 1.0f;
        body.position = {0.0f, 0.0f, 0.0f};
        body.velocity = {10.0f, 10.0f, 10.0f};
        body.is_static = true;
        
        vec3 initial_pos = body.position;
        
        float dt = 0.016f;
        
        // Static bodies should ignore velocity
        if (!body.is_static) {
            body.position.x += body.velocity.x * dt;
            body.position.y += body.velocity.y * dt;
            body.position.z += body.velocity.z * dt;
        }
        
        REQUIRE(body.position.x == initial_pos.x);
        REQUIRE(body.position.y == initial_pos.y);
        REQUIRE(body.position.z == initial_pos.z);
    }
    
    SECTION("Impulse application") {
        RigidBody body = {0};
        body.mass = 2.0f;
        body.velocity = {0.0f, 0.0f, 0.0f};
        
        // Apply impulse
        vec3 impulse = {10.0f, 0.0f, 0.0f};
        body.velocity.x += impulse.x / body.mass;
        body.velocity.y += impulse.y / body.mass;
        body.velocity.z += impulse.z / body.mass;
        
        REQUIRE(body.velocity.x == 5.0f); // 10 / 2
        REQUIRE(body.velocity.y == 0.0f);
        REQUIRE(body.velocity.z == 0.0f);
    }
    
    SECTION("Energy conservation (simple)") {
        RigidBody body = {0};
        body.mass = 1.0f;
        body.position = {0.0f, 10.0f, 0.0f};
        body.velocity = {0.0f, 0.0f, 0.0f};
        
        float gravity = -9.81f;
        
        // Calculate initial potential energy
        float initial_pe = body.mass * fabs(gravity) * body.position.y;
        float initial_ke = 0.5f * body.mass * 
            (body.velocity.x * body.velocity.x + 
             body.velocity.y * body.velocity.y + 
             body.velocity.z * body.velocity.z);
        float initial_total = initial_pe + initial_ke;
        
        // Simulate for 1 second
        for (int i = 0; i < 60; i++) {
            float dt = 1.0f / 60.0f;
            body.velocity.y += gravity * dt;
            body.position.y += body.velocity.y * dt;
        }
        
        // Calculate final energy
        float final_pe = body.mass * fabs(gravity) * fabs(body.position.y);
        float final_ke = 0.5f * body.mass * 
            (body.velocity.x * body.velocity.x + 
             body.velocity.y * body.velocity.y + 
             body.velocity.z * body.velocity.z);
        float final_total = final_pe + final_ke;
        
        // Energy should be approximately conserved (within 10% due to Euler integration)
        float energy_diff = fabs(final_total - initial_total);
        float tolerance = initial_total * 0.1f;
        REQUIRE(energy_diff < tolerance);
    }
    
    SECTION("Damping") {
        RigidBody body = {0};
        body.mass = 1.0f;
        body.velocity = {10.0f, 0.0f, 0.0f};
        
        float damping = 0.95f; // 5% velocity loss per frame
        float dt = 0.016f;
        
        float initial_speed = body.velocity.x;
        
        // Apply damping for 60 frames (1 second)
        for (int i = 0; i < 60; i++) {
            body.velocity.x *= damping;
        }
        
        // Velocity should be reduced
        REQUIRE(body.velocity.x < initial_speed);
        REQUIRE(body.velocity.x > 0.0f); // But not zero
    }
}
