// Physics Demo: Ragdoll Physics - Character simulation with constraints
#include "physics_demo_framework.c"
#include "engine/include/math/math_all.h"
#include <stdlib.h>

static RigidBody* ragdoll_parts[15];
static Constraint* ragdoll_constraints[14];
static int part_count = 0;
static int constraint_count = 0;
static bool demo_reset = false;

// Ragdoll body parts indices
enum {
    RAGDOLL_HEAD = 0,
    RAGDOLL_CHEST,
    RAGDOLL_PELVIS,
    RAGDOLL_LEFT_UPPER_ARM,
    RAGDOLL_LEFT_LOWER_ARM,
    RAGDOLL_RIGHT_UPPER_ARM,
    RAGDOLL_RIGHT_LOWER_ARM,
    RAGDOLL_LEFT_UPPER_LEG,
    RAGDOLL_LEFT_LOWER_LEG,
    RAGDOLL_RIGHT_UPPER_LEG,
    RAGDOLL_RIGHT_LOWER_LEG,
    RAGDOLL_LEFT_HAND,
    RAGDOLL_RIGHT_HAND,
    RAGDOLL_LEFT_FOOT,
    RAGDOLL_RIGHT_FOOT
};

void ragdoll_init(PhysicsWorld* world) {
    LOG_INFO("Initializing Ragdoll Demo");
    
    // Clear existing ragdoll
    for (int i = 0; i < part_count; i++) {
        physics_world_remove_body(world, ragdoll_parts[i]);
        rigid_body_destroy(ragdoll_parts[i]);
    }
    for (int i = 0; i < constraint_count; i++) {
        physics_world_remove_constraint(world, ragdoll_constraints[i]);
        constraint_destroy(ragdoll_constraints[i]);
    }
    part_count = 0;
    constraint_count = 0;
    
    // Create ground plane
    physics_demo_create_plane((Vec3){0, 1, 0}, 0.0f);
    
    // Create ragdoll parts (simplified humanoid)
    Vec3 base_pos = {0.0f, 5.0f, 0.0f};
    
    // Head
    ragdoll_parts[RAGDOLL_HEAD] = physics_demo_create_sphere(
        (Vec3){base_pos.x, base_pos.y + 1.8f, base_pos.z}, 0.15f, 3.0f);
    part_count++;
    
    // Chest
    ragdoll_parts[RAGDOLL_CHEST] = physics_demo_create_box(
        (Vec3){base_pos.x, base_pos.y + 1.3f, base_pos.z}, 
        (Vec3){0.4f, 0.6f, 0.2f}, 8.0f);
    part_count++;
    
    // Pelvis
    ragdoll_parts[RAGDOLL_PELVIS] = physics_demo_create_box(
        (Vec3){base_pos.x, base_pos.y + 0.7f, base_pos.z}, 
        (Vec3){0.3f, 0.3f, 0.2f}, 6.0f);
    part_count++;
    
    // Arms
    ragdoll_parts[RAGDOLL_LEFT_UPPER_ARM] = physics_demo_create_box(
        (Vec3){base_pos.x - 0.4f, base_pos.y + 1.3f, base_pos.z}, 
        (Vec3){0.08f, 0.35f, 0.08f}, 2.0f);
    part_count++;
    
    ragdoll_parts[RAGDOLL_LEFT_LOWER_ARM] = physics_demo_create_box(
        (Vec3){base_pos.x - 0.4f, base_pos.y + 0.8f, base_pos.z}, 
        (Vec3){0.06f, 0.35f, 0.06f}, 1.5f);
    part_count++;
    
    ragdoll_parts[RAGDOLL_RIGHT_UPPER_ARM] = physics_demo_create_box(
        (Vec3){base_pos.x + 0.4f, base_pos.y + 1.3f, base_pos.z}, 
        (Vec3){0.08f, 0.35f, 0.08f}, 2.0f);
    part_count++;
    
    ragdoll_parts[RAGDOLL_RIGHT_LOWER_ARM] = physics_demo_create_box(
        (Vec3){base_pos.x + 0.4f, base_pos.y + 0.8f, base_pos.z}, 
        (Vec3){0.06f, 0.35f, 0.06f}, 1.5f);
    part_count++;
    
    // Legs
    ragdoll_parts[RAGDOLL_LEFT_UPPER_LEG] = physics_demo_create_box(
        (Vec3){base_pos.x - 0.15f, base_pos.y + 0.3f, base_pos.z}, 
        (Vec3){0.1f, 0.4f, 0.1f}, 3.0f);
    part_count++;
    
    ragdoll_parts[RAGDOLL_LEFT_LOWER_LEG] = physics_demo_create_box(
        (Vec3){base_pos.x - 0.15f, base_pos.y - 0.2f, base_pos.z}, 
        (Vec3){0.08f, 0.4f, 0.08f}, 2.5f);
    part_count++;
    
    ragdoll_parts[RAGDOLL_RIGHT_UPPER_LEG] = physics_demo_create_box(
        (Vec3){base_pos.x + 0.15f, base_pos.y + 0.3f, base_pos.z}, 
        (Vec3){0.1f, 0.4f, 0.1f}, 3.0f);
    part_count++;
    
    ragdoll_parts[RAGDOLL_RIGHT_LOWER_LEG] = physics_demo_create_box(
        (Vec3){base_pos.x + 0.15f, base_pos.y - 0.2f, base_pos.z}, 
        (Vec3){0.08f, 0.4f, 0.08f}, 2.5f);
    part_count++;
    
    // Create ball joints (constraints)
    // Head to chest
    ragdoll_constraints[constraint_count] = constraint_create_ball_joint(
        ragdoll_parts[RAGDOLL_HEAD], ragdoll_parts[RAGDOLL_CHEST],
        (Vec3){0, -0.15f, 0}, (Vec3){0, 0.3f, 0});
    physics_world_add_constraint(world, ragdoll_constraints[constraint_count++]);
    
    // Chest to pelvis
    ragdoll_constraints[constraint_count] = constraint_create_ball_joint(
        ragdoll_parts[RAGDOLL_CHEST], ragdoll_parts[RAGDOLL_PELVIS],
        (Vec3){0, -0.3f, 0}, (Vec3){0, 0.15f, 0});
    physics_world_add_constraint(world, ragdoll_constraints[constraint_count++]);
    
    // Arms to chest
    ragdoll_constraints[constraint_count] = constraint_create_ball_joint(
        ragdoll_parts[RAGDOLL_LEFT_UPPER_ARM], ragdoll_parts[RAGDOLL_CHEST],
        (Vec3){0, 0.175f, 0}, (Vec3){-0.2f, 0.3f, 0});
    physics_world_add_constraint(world, ragdoll_constraints[constraint_count++]);
    
    ragdoll_constraints[constraint_count] = constraint_create_ball_joint(
        ragdoll_parts[RAGDOLL_RIGHT_UPPER_ARM], ragdoll_parts[RAGDOLL_CHEST],
        (Vec3){0, 0.175f, 0}, (Vec3){0.2f, 0.3f, 0});
    physics_world_add_constraint(world, ragdoll_constraints[constraint_count++]);
    
    // Lower arms to upper arms
    ragdoll_constraints[constraint_count] = constraint_create_hinge_joint(
        ragdoll_parts[RAGDOLL_LEFT_LOWER_ARM], ragdoll_parts[RAGDOLL_LEFT_UPPER_ARM],
        (Vec3){0, 0.175f, 0}, (Vec3){0, -0.175f, 0}, (Vec3){1, 0, 0});
    physics_world_add_constraint(world, ragdoll_constraints[constraint_count++]);
    
    ragdoll_constraints[constraint_count] = constraint_create_hinge_joint(
        ragdoll_parts[RAGDOLL_RIGHT_LOWER_ARM], ragdoll_parts[RAGDOLL_RIGHT_UPPER_ARM],
        (Vec3){0, 0.175f, 0}, (Vec3){0, -0.175f, 0}, (Vec3){1, 0, 0});
    physics_world_add_constraint(world, ragdoll_constraints[constraint_count++]);
    
    // Legs to pelvis
    ragdoll_constraints[constraint_count] = constraint_create_ball_joint(
        ragdoll_parts[RAGDOLL_LEFT_UPPER_LEG], ragdoll_parts[RAGDOLL_PELVIS],
        (Vec3){0, 0.2f, 0}, (Vec3){-0.15f, -0.15f, 0});
    physics_world_add_constraint(world, ragdoll_constraints[constraint_count++]);
    
    ragdoll_constraints[constraint_count] = constraint_create_ball_joint(
        ragdoll_parts[RAGDOLL_RIGHT_UPPER_LEG], ragdoll_parts[RAGDOLL_PELVIS],
        (Vec3){0, 0.2f, 0}, (Vec3){0.15f, -0.15f, 0});
    physics_world_add_constraint(world, ragdoll_constraints[constraint_count++]);
    
    // Lower legs to upper legs
    ragdoll_constraints[constraint_count] = constraint_create_hinge_joint(
        ragdoll_parts[RAGDOLL_LEFT_LOWER_LEG], ragdoll_parts[RAGDOLL_LEFT_UPPER_LEG],
        (Vec3){0, 0.2f, 0}, (Vec3){0, -0.2f, 0}, (Vec3){1, 0, 0});
    physics_world_add_constraint(world, ragdoll_constraints[constraint_count++]);
    
    ragdoll_constraints[constraint_count] = constraint_create_hinge_joint(
        ragdoll_parts[RAGDOLL_RIGHT_LOWER_LEG], ragdoll_parts[RAGDOLL_RIGHT_UPPER_LEG],
        (Vec3){0, 0.2f, 0}, (Vec3){0, -0.2f, 0}, (Vec3){1, 0, 0});
    physics_world_add_constraint(world, ragdoll_constraints[constraint_count++]);
    
    demo_reset = false;
}

void ragdoll_update(PhysicsWorld* world, float dt) {
    // Reset demo on spacebar
    if (key_pressed(KEY_SPACE)) {
        demo_reset = true;
    }
    
    // Apply random impulse to chest on 'F' key (like being hit)
    if (key_pressed(KEY_F)) {
        Vec3 impulse = {
            (rand() % 200 - 100) * 0.02f,
            (rand() % 100) * 0.02f,
            (rand() % 200 - 100) * 0.02f
        };
        rigid_body_apply_impulse(ragdoll_parts[RAGDOLL_CHEST], impulse);
    }
    
    // Drop ragdoll from higher on 'R' key
    if (key_pressed(KEY_R)) {
        for (int i = 0; i < part_count; i++) {
            Vec3 pos = rigid_body_get_position(ragdoll_parts[i]);
            pos.y += 3.0f;
            rigid_body_set_position(ragdoll_parts[i], pos);
            rigid_body_set_velocity(ragdoll_parts[i], (Vec3){0, 0, 0});
        }
    }
    
    if (demo_reset) {
        ragdoll_init(world);
    }
}

void ragdoll_render(Renderer* renderer, PhysicsWorld* world) {
    // Render ragdoll parts
    for (int i = 0; i < part_count; i++) {
        // Color code different body parts
        Vec3 color;
        switch (i) {
            case RAGDOLL_HEAD:
                color = (Vec3){0.8, 0.6, 0.4}; // Skin color
                break;
            case RAGDOLL_CHEST:
            case RAGDOLL_PELVIS:
                color = (Vec3){0.2, 0.3, 0.6}; // Blue shirt
                break;
            default:
                color = (Vec3){0.3, 0.5, 0.2}; // Green pants
                break;
        }
        
        Vec3 pos = rigid_body_get_position(ragdoll_parts[i]);
        Quat rot = rigid_body_get_rotation(ragdoll_parts[i]);
        Shape* shape = rigid_body_get_shape(ragdoll_parts[i]);
        
        renderer_push_matrix(renderer);
        renderer_translate(renderer, pos);
        renderer_rotate_quat(renderer, rot);
        
        switch (shape->type) {
            case SHAPE_BOX:
                renderer_draw_box(renderer, shape->box.size, color);
                break;
            case SHAPE_SPHERE:
                renderer_draw_sphere(renderer, shape->sphere.radius, color);
                break;
            default:
                renderer_draw_box(renderer, (Vec3){0.1, 0.1, 0.1}, color);
                break;
        }
        
        renderer_pop_matrix(renderer);
    }
    
    // Render constraints as lines
    renderer_set_color(renderer, (Vec3){1, 0, 0});
    for (int i = 0; i < constraint_count; i++) {
        Vec3 anchor_a, anchor_b;
        constraint_get_anchors(ragdoll_constraints[i], &anchor_a, &anchor_b);
        renderer_draw_line(renderer, anchor_a, anchor_b);
    }
    
    // Render debug info
    physics_demo_render_debug_info(renderer, world);
    
    // Render demo-specific info
    renderer_set_text_color(renderer, (Vec3){0, 1, 0});
    renderer_draw_text(renderer, "Ragdoll Physics Demo", 10, 320, 14);
    renderer_set_text_color(renderer, (Vec3){0.8, 0.8, 0.8});
    renderer_draw_text(renderer, "Space: Reset | F: Hit ragdoll | R: Drop from height", 10, 340, 12);
}

void ragdoll_cleanup(PhysicsWorld* world) {
    for (int i = 0; i < constraint_count; i++) {
        physics_world_remove_constraint(world, ragdoll_constraints[i]);
        constraint_destroy(ragdoll_constraints[i]);
    }
    
    for (int i = 0; i < part_count; i++) {
        physics_world_remove_body(world, ragdoll_parts[i]);
        rigid_body_destroy(ragdoll_parts[i]);
    }
    
    part_count = 0;
    constraint_count = 0;
}

// Register the demo
REGISTER_DEMO(Ragdoll Physics, ragdoll_init, ragdoll_update, ragdoll_render, ragdoll_cleanup);
