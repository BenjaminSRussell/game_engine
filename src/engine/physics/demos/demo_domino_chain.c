// Physics Demo: Domino Chain - Chain reaction simulation
#include "physics_demo_framework.c"
#include <math/math.h>
#include <stdlib.h>

static RigidBody* dominos[50];
static int domino_count = 0;
static bool demo_reset = false;
static float domino_height = 1.0f;
static float domino_width = 0.1f;
static float domino_depth = 0.3f;
static float spacing = 0.4f;

void domino_chain_init(PhysicsWorld* world) {
    LOG_INFO("Initializing Domino Chain Demo");
    
    // Clear existing dominos
    for (int i = 0; i < domino_count; i++) {
        physics_world_remove_body(world, dominos[i]);
        rigid_body_destroy(dominos[i]);
    }
    domino_count = 0;
    
    // Create ground plane
    physics_demo_create_plane((Vec3){0, 1, 0}, 0.0f);
    
    // Create a line of dominos in a curve pattern
    for (int i = 0; i < 30; i++) {
        float angle = i * 0.15f; // Create a gentle curve
        float radius = 5.0f;
        
        Vec3 position = {
            radius * sinf(angle),
            domino_height / 2.0f + 0.1f,
            radius * cosf(angle) - 5.0f
        };
        
        Vec3 size = {domino_width, domino_height, domino_depth};
        dominos[domino_count] = physics_demo_create_box(position, size, 0.5f);
        
        // Rotate domino to face the curve
        Quat rotation = quat_from_axis_angle((Vec3){0, 1, 0}, angle + M_PI/2);
        rigid_body_set_rotation(dominos[domino_count], rotation);
        
        // Make dominos slightly less bouncy
        rigid_body_set_restitution(dominos[domino_count], 0.3f);
        rigid_body_set_friction(dominos[domino_count], 0.8f);
        
        domino_count++;
    }
    
    // Create a staircase of dominos
    for (int step = 0; step < 10; step++) {
        for (int col = 0; col < 2; col++) {
            Vec3 position = {
                8.0f + col * spacing,
                step * 0.3f + domino_height / 2.0f + 0.1f,
                -2.0f + step * spacing
            };
            
            Vec3 size = {domino_width, domino_height, domino_depth};
            dominos[domino_count] = physics_demo_create_box(position, size, 0.3f);
            
            domino_count++;
        }
    }
    
    // Create a "pusher" ball to start the chain reaction
    Vec3 pusher_pos = {-8.0f, 0.5f, -5.0f};
    dominos[domino_count] = physics_demo_create_sphere(pusher_pos, 0.2f, 2.0f);
    rigid_body_set_velocity(dominos[domino_count], (Vec3){3.0f, 0.0f, 0.0f});
    domino_count++;
    
    demo_reset = false;
}

void domino_chain_update(PhysicsWorld* world, float dt) {
    // Reset demo on spacebar
    if (key_pressed(KEY_SPACE)) {
        demo_reset = true;
    }
    
    // Add another pusher ball on 'P' key
    if (key_pressed(KEY_P)) {
        Vec3 pusher_pos = {8.0f, 2.0f, 2.0f};
        if (domino_count < 49) {
            dominos[domino_count] = physics_demo_create_sphere(pusher_pos, 0.2f, 1.0f);
            rigid_body_set_velocity(dominos[domino_count], (Vec3){-2.0f, 0.0f, -2.0f});
            domino_count++;
        }
    }
    
    if (demo_reset) {
        domino_chain_init(world);
    }
}

void domino_chain_render(Renderer* renderer, PhysicsWorld* world) {
    // Render all dominos
    for (int i = 0; i < domino_count; i++) {
        physics_demo_render_body(renderer, dominos[i]);
    }
    
    // Render debug info
    physics_demo_render_debug_info(renderer, world);
    
    // Render demo-specific info
    renderer_set_text_color(renderer, (Vec3){0, 1, 0});
    renderer_draw_text(renderer, "Domino Chain Demo", 10, 320, 14);
    renderer_set_text_color(renderer, (Vec3){0.8, 0.8, 0.8});
    renderer_draw_text(renderer, "Space: Reset | P: Add pusher ball", 10, 340, 12);
}

void domino_chain_cleanup(PhysicsWorld* world) {
    for (int i = 0; i < domino_count; i++) {
        physics_world_remove_body(world, dominos[i]);
        rigid_body_destroy(dominos[i]);
    }
    domino_count = 0;
}

// Register the demo
REGISTER_DEMO(Domino Chain, domino_chain_init, domino_chain_update, domino_chain_render, domino_chain_cleanup);
