// Physics Demo: Vehicle Physics - Car simulation with wheels and suspension
#include "physics_demo_framework.c"
#include "engine/include/math/math.h"
#include <stdlib.h>

static RigidBody* car_body;
static RigidBody* wheels[4];
static Constraint* suspension[4];
static bool demo_reset = false;
static float engine_force = 0.0f;
static float steering_angle = 0.0f;

// Wheel indices
enum {
    WHEEL_FRONT_LEFT = 0,
    WHEEL_FRONT_RIGHT,
    WHEEL_REAR_LEFT,
    WHEEL_REAR_RIGHT
};

void vehicle_physics_init(PhysicsWorld* world) {
    LOG_INFO("Initializing Vehicle Physics Demo");
    
    // Clear existing vehicle
    if (car_body) {
        physics_world_remove_body(world, car_body);
        rigid_body_destroy(car_body);
        car_body = NULL;
    }
    
    for (int i = 0; i < 4; i++) {
        if (wheels[i]) {
            physics_world_remove_body(world, wheels[i]);
            rigid_body_destroy(wheels[i]);
            wheels[i] = NULL;
        }
        if (suspension[i]) {
            physics_world_remove_constraint(world, suspension[i]);
            constraint_destroy(suspension[i]);
            suspension[i] = NULL;
        }
    }
    
    // Create ground plane
    physics_demo_create_plane((Vec3){0, 1, 0}, 0.0f);
    
    // Create some obstacles
    physics_demo_create_box((Vec3){5.0f, 0.5f, 0.0f}, (Vec3){1.0f, 1.0f, 1.0f}, 10.0f);
    physics_demo_create_box((Vec3){-5.0f, 0.5f, 3.0f}, (Vec3){1.0f, 1.0f, 1.0f}, 10.0f);
    physics_demo_create_sphere((Vec3){0.0f, 1.0f, 6.0f}, 0.5f, 5.0f);
    
    // Create car body
    Vec3 car_pos = {0.0f, 1.0f, 0.0f};
    car_body = physics_demo_create_box(car_pos, (Vec3){2.0f, 0.8f, 4.0f}, 150.0f);
    rigid_body_set_friction(car_body, 0.3f);
    rigid_body_set_restitution(car_body, 0.1f);
    
    // Create wheels
    float wheel_radius = 0.4f;
    float wheel_mass = 5.0f;
    
    // Front wheels
    wheels[WHEEL_FRONT_LEFT] = physics_demo_create_sphere(
        (Vec3){-1.2f, 0.4f, 1.5f}, wheel_radius, wheel_mass);
    wheels[WHEEL_FRONT_RIGHT] = physics_demo_create_sphere(
        (Vec3){1.2f, 0.4f, 1.5f}, wheel_radius, wheel_mass);
    
    // Rear wheels
    wheels[WHEEL_REAR_LEFT] = physics_demo_create_sphere(
        (Vec3){-1.2f, 0.4f, -1.5f}, wheel_radius, wheel_mass);
    wheels[WHEEL_REAR_RIGHT] = physics_demo_create_sphere(
        (Vec3){1.2f, 0.4f, -1.5f}, wheel_radius, wheel_mass);
    
    // Set wheel properties
    for (int i = 0; i < 4; i++) {
        rigid_body_set_friction(wheels[i], 1.5f); // High friction for grip
        rigid_body_set_restitution(wheels[i], 0.0f); // No bounce
    }
    
    // Create suspension (spring constraints)
    float spring_strength = 500.0f;
    float damping = 50.0f;
    
    for (int i = 0; i < 4; i++) {
        Vec3 wheel_pos = rigid_body_get_position(wheels[i]);
        Vec3 attachment_point = vec3_sub(wheel_pos, (Vec3){0, 0.4f, 0});
        
        suspension[i] = constraint_create_spring(
            car_body, wheels[i],
            attachment_point, (Vec3){0, 0, 0},
            spring_strength, damping, 0.3f
        );
        physics_world_add_constraint(world, suspension[i]);
    }
    
    engine_force = 0.0f;
    steering_angle = 0.0f;
    demo_reset = false;
}

void vehicle_physics_update(PhysicsWorld* world, float dt) {
    // Handle input
    if (key_pressed(KEY_SPACE)) {
        demo_reset = true;
    }
    
    // Engine control
    if (key_down(KEY_UP) || key_down(KEY_W)) {
        engine_force = 300.0f;
    } else if (key_down(KEY_DOWN) || key_down(KEY_S)) {
        engine_force = -200.0f;
    } else {
        engine_force *= 0.9f; // Gradual slowdown
    }
    
    // Steering control
    if (key_down(KEY_LEFT) || key_down(KEY_A)) {
        steering_angle = fmaxf(steering_angle - dt * 2.0f, -0.5f);
    } else if (key_down(KEY_RIGHT) || key_down(KEY_D)) {
        steering_angle = fminf(steering_angle + dt * 2.0f, 0.5f);
    } else {
        steering_angle *= 0.9f; // Auto-center
    }
    
    // Apply engine force to rear wheels
    Vec3 car_forward = rigid_body_get_forward_vector(car_body);
    Vec3 engine_force_vec = vec3_scale(car_forward, engine_force);
    
    rigid_body_apply_force(wheels[WHEEL_REAR_LEFT], engine_force_vec);
    rigid_body_apply_force(wheels[WHEEL_REAR_RIGHT], engine_force_vec);
    
    // Apply steering forces to front wheels
    if (fabsf(steering_angle) > 0.01f) {
        Vec3 steer_force = vec3_scale(rigid_body_get_right_vector(car_body), steering_angle * 100.0f);
        rigid_body_apply_force(wheels[WHEEL_FRONT_LEFT], steer_force);
        rigid_body_apply_force(wheels[WHEEL_FRONT_RIGHT], steer_force);
    }
    
    // Apply downforce for stability
    Vec3 downforce = vec3_scale((Vec3){0, -1, 0}, 50.0f);
    rigid_body_apply_force(car_body, downforce);
    
    // Reset demo
    if (demo_reset) {
        vehicle_physics_init(world);
    }
}

void vehicle_physics_render(Renderer* renderer, PhysicsWorld* world) {
    // Render car body
    physics_demo_render_body(renderer, car_body);
    
    // Render wheels
    for (int i = 0; i < 4; i++) {
        physics_demo_render_body(renderer, wheels[i]);
    }
    
    // Render suspension as lines
    renderer_set_color(renderer, (Vec3){0.5, 0.5, 0.5});
    for (int i = 0; i < 4; i++) {
        Vec3 wheel_pos = rigid_body_get_position(wheels[i]);
        Vec3 attachment_point = vec3_sub(wheel_pos, (Vec3){0, 0.4f, 0});
        renderer_draw_line(renderer, attachment_point, wheel_pos);
    }
    
    // Render velocity vector
    Vec3 car_pos = rigid_body_get_position(car_body);
    Vec3 car_vel = rigid_body_get_velocity(car_body);
    Vec3 vel_end = vec3_add(car_pos, vec3_scale(car_vel, 0.2f));
    renderer_set_color(renderer, (Vec3){0, 1, 0});
    renderer_draw_line(renderer, car_pos, vel_end);
    
    // Render debug info
    physics_demo_render_debug_info(renderer, world);
    
    // Render demo-specific info
    renderer_set_text_color(renderer, (Vec3){0, 1, 0});
    renderer_draw_text(renderer, "Vehicle Physics Demo", 10, 320, 14);
    renderer_set_text_color(renderer, (Vec3){0.8, 0.8, 0.8});
    renderer_draw_text(renderer, "W/S or Arrow Up/Down: Accelerate/Brake", 10, 340, 12);
    renderer_draw_text(renderer, "A/D or Arrow Left/Right: Steer", 10, 360, 12);
    renderer_draw_text(renderer, "Space: Reset", 10, 380, 12);
    
    // Render speed
    float speed = vec3_length(car_vel);
    char speed_text[64];
    snprintf(speed_text, sizeof(speed_text), "Speed: %.1f m/s", speed);
    renderer_draw_text(renderer, speed_text, 10, 400, 12);
}

void vehicle_physics_cleanup(PhysicsWorld* world) {
    if (car_body) {
        physics_world_remove_body(world, car_body);
        rigid_body_destroy(car_body);
        car_body = NULL;
    }
    
    for (int i = 0; i < 4; i++) {
        if (wheels[i]) {
            physics_world_remove_body(world, wheels[i]);
            rigid_body_destroy(wheels[i]);
            wheels[i] = NULL;
        }
        if (suspension[i]) {
            physics_world_remove_constraint(world, suspension[i]);
            constraint_destroy(suspension[i]);
            suspension[i] = NULL;
        }
    }
}

// Register the demo
REGISTER_DEMO(Vehicle Physics, vehicle_physics_init, vehicle_physics_update, vehicle_physics_render, vehicle_physics_cleanup);
