// Physics Demo: Newton's Cradle - Conservation of momentum
#include "physics_demo_framework.c"
#include "engine/include/math/math.h"
#include <stdlib.h>

static RigidBody* balls[5];
static bool demo_reset = false;
static float ball_radius = 0.3f;
static float string_length = 2.0f;

void newton_cradle_init(PhysicsWorld* world) {
    LOG_INFO("Initializing Newton's Cradle Demo");
    
    // Clear existing balls
    for (int i = 0; i < 5; i++) {
        if (balls[i]) {
            physics_world_remove_body(world, balls[i]);
            rigid_body_destroy(balls[i]);
            balls[i] = NULL;
        }
    }
    
    // Create ground plane
    physics_demo_create_plane((Vec3){0, 1, 0}, -3.0f);
    
    // Create 5 balls in a row
    for (int i = 0; i < 5; i++) {
        Vec3 position = {
            (i - 2.0f) * ball_radius * 2.2f,
            string_length,
            0.0f
        };
        
        balls[i] = physics_demo_create_sphere(position, ball_radius, 1.0f);
        
        // Make balls slightly less bouncy for realistic cradle effect
        rigid_body_set_restitution(balls[i], 0.8f);
        rigid_body_set_friction(balls[i], 0.1f);
    }
    
    // Lift the first ball to start
    Vec3 start_pos = rigid_body_get_position(balls[0]);
    start_pos.y += 1.5f;
    start_pos.x -= 1.5f;
    rigid_body_set_position(balls[0], start_pos);
    rigid_body_set_velocity(balls[0], (Vec3){0, 0, 0});
    
    demo_reset = false;
}

void newton_cradle_update(PhysicsWorld* world, float dt) {
    // Apply string constraints (simplified - just pull balls toward rest position)
    for (int i = 0; i < 5; i++) {
        Vec3 pos = rigid_body_get_position(balls[i]);
        Vec3 rest_pos = {
            (i - 2.0f) * ball_radius * 2.2f,
            string_length,
            0.0f
        };
        
        Vec3 to_rest = vec3_sub(rest_pos, pos);
        float dist = vec3_length(to_rest);
        
        if (dist > 0.01f) {
            // Apply spring force toward rest position
            Vec3 force = vec3_scale(to_rest, 50.0f);
            rigid_body_apply_force(balls[i], force);
        }
    }
    
    // Reset demo on spacebar
    if (key_pressed(KEY_SPACE)) {
        demo_reset = true;
    }
    
    // Apply impulse to first ball on 'A' key
    if (key_pressed(KEY_A)) {
        Vec3 impulse = {-5.0f, 0.0f, 0.0f};
        rigid_body_apply_impulse(balls[0], impulse);
    }
    
    // Apply impulse to last ball on 'D' key
    if (key_pressed(KEY_D)) {
        Vec3 impulse = {5.0f, 0.0f, 0.0f};
        rigid_body_apply_impulse(balls[4], impulse);
    }
    
    if (demo_reset) {
        newton_cradle_init(world);
    }
}

void newton_cradle_render(Renderer* renderer, PhysicsWorld* world) {
    // Render strings
    renderer_set_color(renderer, (Vec3){0.5, 0.5, 0.5});
    for (int i = 0; i < 5; i++) {
        Vec3 pos = rigid_body_get_position(balls[i]);
        Vec3 top = {
            (i - 2.0f) * ball_radius * 2.2f,
            string_length + 1.0f,
            0.0f
        };
        renderer_draw_line(renderer, top, pos);
    }
    
    // Render balls
    for (int i = 0; i < 5; i++) {
        physics_demo_render_body(renderer, balls[i]);
    }
    
    // Render debug info
    physics_demo_render_debug_info(renderer, world);
    
    // Render demo-specific info
    renderer_set_text_color(renderer, (Vec3){0, 1, 0});
    renderer_draw_text(renderer, "Newton's Cradle Demo", 10, 320, 14);
    renderer_set_text_color(renderer, (Vec3){0.8, 0.8, 0.8});
    renderer_draw_text(renderer, "Space: Reset | A: Push left ball | D: Push right ball", 10, 340, 12);
}

void newton_cradle_cleanup(PhysicsWorld* world) {
    for (int i = 0; i < 5; i++) {
        if (balls[i]) {
            physics_world_remove_body(world, balls[i]);
            rigid_body_destroy(balls[i]);
            balls[i] = NULL;
        }
    }
}

// Register the demo
REGISTER_DEMO(Newton Cradle, newton_cradle_init, newton_cradle_update, newton_cradle_render, newton_cradle_cleanup);
