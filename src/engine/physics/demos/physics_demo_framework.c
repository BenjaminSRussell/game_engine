// Physics Demo Framework - Common utilities for physics demonstrations
#include "physics_demo_framework.h"
#include "core/logger.h"
#include "rendering/renderer.h"
#include "engine/include/math/math.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Demo registry
static PhysicsDemo* active_demo = NULL;
static PhysicsDemo demos[16];
static int demo_count = 0;

void physics_demo_render_body(Renderer* renderer, RigidBody* body) {
    if (!renderer || !body) return;
    
    Vec3 pos = rigid_body_get_position(body);
    Vec3 scale = {1.0f, 1.0f, 1.0f};
    Quat rotation = {0, 0, 0, 1}; // Identity quaternion
    
    // Render as a simple box for now
    renderer_draw_box(renderer, pos, scale, rotation, (Color){0.2f, 0.6f, 1.0f, 1.0f});
}

void physics_demo_render_debug_info(Renderer* renderer, PhysicsWorld* world) {
    if (!renderer || !world) return;
    
    // Render debug info like collision bounds, velocities, etc.
    int body_count = physics_world_get_body_count(world);
    char debug_text[256];
    snprintf(debug_text, sizeof(debug_text), "Bodies: %d", body_count);
    
    renderer_set_text_color(renderer, (Vec3){1, 1, 1});
    renderer_draw_text(renderer, debug_text, 10, 300, 12);
}

void renderer_set_text_color(Renderer* renderer, Vec3 color) {
    if (!renderer) return;
    // Implementation would set text color for subsequent text rendering
    (void)color;
}

void renderer_draw_text(Renderer* renderer, const char* text, int x, int y, int size) {
    if (!renderer || !text) return;
    // Implementation would render text at specified position
    (void)x; (void)y; (void)size;
}

void physics_demo_register(const char* name, 
                          PhysicsDemoInit init_fn,
                          PhysicsDemoUpdate update_fn,
                          PhysicsDemoRender render_fn,
                          PhysicsDemoCleanup cleanup_fn) {
    if (demo_count >= 16) {
        LOG_ERROR("Maximum demo count reached");
        return;
    }
    
    PhysicsDemo* demo = &demos[demo_count++];
    strncpy(demo->name, name, sizeof(demo->name) - 1);
    demo->name[sizeof(demo->name) - 1] = '\0';
    demo->init = init_fn;
    demo->update = update_fn;
    demo->render = render_fn;
    demo->cleanup = cleanup_fn;
}

bool physics_demo_load(const char* name, PhysicsWorld* world) {
    for (int i = 0; i < demo_count; i++) {
        if (strcmp(demos[i].name, name) == 0) {
            active_demo = &demos[i];
            if (active_demo->init) {
                active_demo->init(world);
            }
            return true;
        }
    }
    return false;
}

void physics_demo_update(PhysicsWorld* world, float dt) {
    if (active_demo && active_demo->update) {
        active_demo->update(world, dt);
    }
}

void physics_demo_render(Renderer* renderer, PhysicsWorld* world) {
    if (active_demo && active_demo->render) {
        active_demo->render(renderer, world);
    }
}

void physics_demo_cleanup(PhysicsWorld* world) {
    if (active_demo && active_demo->cleanup) {
        active_demo->cleanup(world);
    }
    active_demo = NULL;
}

PhysicsDemo* physics_demo_get_active() {
    return active_demo;
}

int physics_demo_get_count() {
    return demo_count;
}

PhysicsDemo* physics_demo_get_all() {
    return demos;
}