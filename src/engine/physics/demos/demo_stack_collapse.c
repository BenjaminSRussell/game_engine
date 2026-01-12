// Physics Demo: Stack Collapse - Demonstrates stacking and stability
#include "physics_demo_framework.c"
#include <math/math.h>
#include <stdlib.h>

static RigidBody* boxes[20];
static int box_count = 0;
static bool demo_reset = false;

void stack_collapse_init(PhysicsWorld* world) {
    LOG_INFO("Initializing Stack Collapse Demo");
    
    // Clear existing boxes
    for (int i = 0; i < box_count; i++) {
        physics_world_remove_body(world, boxes[i]);
        rigid_body_destroy(boxes[i]);
    }
    box_count = 0;
    
    // Create ground plane
    physics_demo_create_plane((Vec3){0, 1, 0}, 0.0f);
    
    // Create a tall stack of boxes
    float box_size = 0.5f;
    float spacing = 0.01f;
    
    for (int layer = 0; layer < 10; layer++) {
        for (int box = 0; box < 2; box++) {
            Vec3 position = {
                box * (box_size + spacing) - box_size/2.0f,
                layer * (box_size + spacing) + box_size/2.0f + 0.1f,
                0.0f
            };
            
            boxes[box_count] = physics_demo_create_box(position, (Vec3){box_size, box_size, box_size}, 1.0f);
            
            // Add some random rotation for instability
            Quat random_rot = quat_from_euler(
                (rand() % 10 - 5) * 0.01f,
                (rand() % 10 - 5) * 0.01f,
                0.0f
            );
            rigid_body_set_rotation(boxes[box_count], random_rot);
            
            box_count++;
        }
    }
    
    // Add a heavy ball on top to trigger collapse
    Vec3 ball_pos = {0.0f, 10.0f * box_size + 2.0f, 0.0f};
    boxes[box_count] = physics_demo_create_sphere(ball_pos, 0.3f, 5.0f);
    box_count++;
    
    demo_reset = false;
}

void stack_collapse_update(PhysicsWorld* world, float dt) {
    // Reset demo on spacebar
    if (key_pressed(KEY_SPACE)) {
        demo_reset = true;
    }
    
    if (demo_reset) {
        stack_collapse_init(world);
    }
    
    // Add some random impulse occasionally for more chaos
    static float timer = 0.0f;
    timer += dt;
    if (timer > 3.0f && box_count > 0) {
        int random_box = rand() % box_count;
        Vec3 impulse = {
            (rand() % 200 - 100) * 0.01f,
            (rand() % 100) * 0.01f,
            (rand() % 200 - 100) * 0.01f
        };
        rigid_body_apply_impulse(boxes[random_box], impulse);
        timer = 0.0f;
    }
}

void stack_collapse_render(Renderer* renderer, PhysicsWorld* world) {
    // Render all boxes
    for (int i = 0; i < box_count; i++) {
        physics_demo_render_body(renderer, boxes[i]);
    }
    
    // Render debug info
    physics_demo_render_debug_info(renderer, world);
    
    // Render demo-specific info
    renderer_set_text_color(renderer, (Vec3){0, 1, 0});
    renderer_draw_text(renderer, "Stack Collapse Demo", 10, 320, 14);
    renderer_set_text_color(renderer, (Vec3){0.8, 0.8, 0.8});
    renderer_draw_text(renderer, "Space: Reset", 10, 340, 12);
}

void stack_collapse_cleanup(PhysicsWorld* world) {
    for (int i = 0; i < box_count; i++) {
        physics_world_remove_body(world, boxes[i]);
        rigid_body_destroy(boxes[i]);
    }
    box_count = 0;
}

// Register the demo
REGISTER_DEMO(Stack Collapse, stack_collapse_init, stack_collapse_update, stack_collapse_render, stack_collapse_cleanup);
