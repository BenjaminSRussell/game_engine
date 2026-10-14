// Physics Demo Framework - Header file for physics demonstration utilities
#ifndef PHYSICS_DEMO_FRAMEWORK_H
#define PHYSICS_DEMO_FRAMEWORK_H

#include "physics/physics.h"
#include "rendering/renderer.h"
#include "core/logger.h"
#include "engine/include/math/math_all.h"
#include <stdbool.h>

// Demo function types
typedef void (*PhysicsDemoInit)(PhysicsWorld* world);
typedef void (*PhysicsDemoUpdate)(PhysicsWorld* world, float dt);
typedef void (*PhysicsDemoRender)(IRenderer* renderer, PhysicsWorld* world);
typedef void (*PhysicsDemoCleanup)(PhysicsWorld* world);

// Demo structure
typedef struct {
    char name[64];
    PhysicsDemoInit init;
    PhysicsDemoUpdate update;
    PhysicsDemoRender render;
    PhysicsDemoCleanup cleanup;
} PhysicsDemo;

// Demo registration and management
void physics_demo_register(const char* name, 
                          PhysicsDemoInit init_fn,
                          PhysicsDemoUpdate update_fn,
                          PhysicsDemoRender render_fn,
                          PhysicsDemoCleanup cleanup_fn);

bool physics_demo_load(const char* name, PhysicsWorld* world);
void physics_demo_update(PhysicsWorld* world, float dt);
void physics_demo_render(IRenderer* renderer, PhysicsWorld* world);
void physics_demo_cleanup(PhysicsWorld* world);

PhysicsDemo* physics_demo_get_active();
int physics_demo_get_count();
PhysicsDemo* physics_demo_get_all();

// Rendering utilities. Draw helpers call IRenderer vtable slots.
void physics_demo_render_body(IRenderer* renderer, RigidBody* body);
void physics_demo_render_debug_info(IRenderer* renderer, PhysicsWorld* world);
void renderer_set_text_color(IRenderer* renderer, Vec3 color);
void renderer_set_color(IRenderer* renderer, Vec3 color);
void renderer_draw_text(IRenderer* renderer, const char* text, int x, int y, int size);
void renderer_draw_line(IRenderer* renderer, Vec3 start, Vec3 end);
void renderer_draw_sphere(IRenderer* renderer, float radius, Vec3 center);

bool physics_demo_framework_init(void);
void physics_demo_framework_run(void);
void physics_demo_framework_cleanup(void);

// File-scope registration. init_fn is a unique function name; name is a string.
#define REGISTER_DEMO(name, init_fn, update_fn, render_fn, cleanup_fn) \
    static void register_demo_##init_fn(void) __attribute__((constructor)); \
    static void register_demo_##init_fn(void) { \
        physics_demo_register((name), (init_fn), (update_fn), (render_fn), (cleanup_fn)); \
    }

#endif // PHYSICS_DEMO_FRAMEWORK_H
