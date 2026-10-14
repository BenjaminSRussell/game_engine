// Issue #282: physics demo framework callbacks use IRenderer, not Renderer.
#include "physics_demo_framework.h"
#include <stdio.h>
#include <string.h>

static int g_failures = 0;
static int g_box_calls = 0;
static int g_text_calls = 0;
static int g_line_calls = 0;
static int g_sphere_calls = 0;
static int g_demo_renders = 0;
static Vec3 g_last_box_center;
static Vec3 g_last_sphere_center;
static char g_last_text[256];
static IRenderer *g_seen_renderer = NULL;

static void expect_true(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        g_failures++;
    } else {
        printf("PASS: %s\n", msg);
    }
}

Vec3 rigid_body_get_position(const RigidBody *body) {
    (void)body;
    Vec3 pos = {1.0f, 2.0f, 3.0f};
    return pos;
}

int physics_world_get_body_count(PhysicsWorld *world) {
    (void)world;
    return 7;
}

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
    (void)level;
    (void)category;
    (void)file;
    (void)line;
    (void)function;
    (void)format;
}

static void mock_box(IRenderer *self, Vec3 center, Vec3 size, Quat rotation, Vec3 color) {
    (void)size;
    (void)rotation;
    g_box_calls++;
    g_last_box_center = center;
    expect_true(self != NULL, "render_debug_box receives IRenderer");
    expect_true(color.x > 0.19f && color.x < 0.21f && color.z > 0.9f,
                "body color is the framework blue");
}

static void mock_text(IRenderer *self, const char *text, Vec2 pos, f32 scale, Vec3 color) {
    (void)self;
    (void)pos;
    (void)scale;
    (void)color;
    g_text_calls++;
    snprintf(g_last_text, sizeof(g_last_text), "%s", text ? text : "");
}

static void mock_line(IRenderer *self, Vec3 start, Vec3 end, Vec3 color) {
    (void)self;
    (void)start;
    (void)end;
    (void)color;
    g_line_calls++;
}

static void mock_sphere(IRenderer *self, Vec3 center, f32 radius, Vec3 color) {
    (void)self;
    (void)radius;
    (void)color;
    g_sphere_calls++;
    g_last_sphere_center = center;
}

static void demo_render(IRenderer *renderer, PhysicsWorld *world) {
    (void)world;
    g_demo_renders++;
    g_seen_renderer = renderer;
    physics_demo_render_body(renderer, (RigidBody *)(void *)1);
    renderer_set_color(renderer, (Vec3){0.25f, 0.5f, 0.75f});
    renderer_draw_line(renderer, (Vec3){0.0f, 0.0f, 0.0f}, (Vec3){1.0f, 0.0f, 0.0f});
    renderer_draw_sphere(renderer, 0.25f, (Vec3){4.0f, 5.0f, 6.0f});
}

static void typecheck_accepts_irenderer(PhysicsDemoRender fn) {
    (void)fn;
}

int main(void) {
    IRenderer renderer;
    IRenderer empty;
    PhysicsWorld *world = (PhysicsWorld *)(void *)1;

    typecheck_accepts_irenderer(demo_render);

    memset(&renderer, 0, sizeof(renderer));
    renderer.render_debug_box = mock_box;
    renderer.render_text = mock_text;
    renderer.render_debug_line = mock_line;
    renderer.render_debug_sphere = mock_sphere;

    physics_demo_register("irenderer", NULL, NULL, demo_render, NULL);
    expect_true(physics_demo_get_count() == 1, "demo registered");
    expect_true(physics_demo_load("irenderer", world) == true, "demo loaded");

    physics_demo_render(&renderer, world);
    expect_true(g_demo_renders == 1, "PhysicsDemoRender callback ran");
    expect_true(g_seen_renderer == &renderer, "callback received the IRenderer pointer");
    expect_true(g_box_calls == 1, "render_body calls IRenderer->render_debug_box");
    expect_true(g_last_box_center.x == 1.0f && g_last_box_center.y == 2.0f &&
                    g_last_box_center.z == 3.0f,
                "box center comes from rigid_body_get_position");
    expect_true(g_line_calls == 1, "renderer_draw_line calls IRenderer->render_debug_line");
    expect_true(g_sphere_calls == 1, "renderer_draw_sphere calls IRenderer->render_debug_sphere");
    expect_true(g_last_sphere_center.x == 4.0f && g_last_sphere_center.y == 5.0f &&
                    g_last_sphere_center.z == 6.0f,
                "sphere center is the position passed by the caller");

    physics_demo_render_debug_info(&renderer, world);
    expect_true(g_text_calls == 1, "debug info calls IRenderer->render_text");
    expect_true(strcmp(g_last_text, "Bodies: 7") == 0, "debug text uses the body count");

    memset(&empty, 0, sizeof(empty));
    physics_demo_render_body(&empty, (RigidBody *)(void *)1);
    physics_demo_render_debug_info(&empty, world);
    renderer_draw_line(&empty, (Vec3){0.0f, 0.0f, 0.0f}, (Vec3){1.0f, 0.0f, 0.0f});
    renderer_draw_sphere(&empty, 1.0f, (Vec3){0.0f, 0.0f, 0.0f});
    renderer_draw_text(&empty, "ignored", 0, 0, 12);
    expect_true(g_box_calls == 1 && g_text_calls == 1 && g_line_calls == 1 && g_sphere_calls == 1,
                "null IRenderer vtable slots are no-ops");

    if (g_failures) {
        fprintf(stderr, "%d failure(s)\n", g_failures);
        return 1;
    }
    printf("All physics demo IRenderer checks passed\n");
    return 0;
}
