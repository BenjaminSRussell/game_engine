// Behavior test for editor/editor_context.c, the shared context EditorApp is built on.
//
// The unit is compiled directly rather than linked to Engine, which does not build on
// origin/main. The Engine-side symbols it calls are stubbed below: the logger, the mat4
// projection builders, and the destroy functions of the terrain and material systems
// (those sources are not part of this test).

#include "core/logger.h"
#include "editor_common.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

// --- Engine-side stubs -----------------------------------------------------------------

void unified_logger_log(LogLevel level, LogCategory category, const char* file, int line,
                        const char* function, const char* format, ...) {
    (void)level;
    (void)category;
    (void)file;
    (void)line;
    (void)function;
    (void)format;
}

Mat4 mat4_look_at(Vec3 eye, Vec3 target, Vec3 up) {
    (void)eye;
    (void)target;
    (void)up;
    return mat4_identity();
}

Mat4 mat4_perspective(f32 fov, f32 aspect, f32 near_z, f32 far_z) {
    (void)fov;
    (void)aspect;
    (void)near_z;
    (void)far_z;
    return mat4_identity();
}

static int g_terrain_destroy_calls = 0;
static int g_material_destroy_calls = 0;

void terrain_sculpting_destroy(TerrainSculptingSystem* system) {
    (void)system;
    g_terrain_destroy_calls++;
}

void material_graph_destroy(MaterialGraph* graph) {
    (void)graph;
    g_material_destroy_calls++;
}

// --- Check helpers ---------------------------------------------------------------------

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                                  \
    do {                                                                             \
        g_checks++;                                                                  \
        if (!(cond)) {                                                               \
            g_failures++;                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
        }                                                                            \
    } while (0)

#define CHECK_NEAR(actual, expected) CHECK(fabsf((float)(actual) - (float)(expected)) < 1e-3f)

// --- Test tool ------------------------------------------------------------------------

typedef struct {
    bool init_ok;
    int init_calls;
    int shutdown_calls;
    int update_calls;
    f32 last_dt;
} TestTool;

// The lifecycle callbacks find their state through ctx->active_tool, which
// editor_set_active_tool sets before init and clears after a failed init.
static bool test_tool_init(EditorContext* ctx) {
    TestTool* state = (TestTool*)ctx->active_tool->user_data;
    state->init_calls++;
    return state->init_ok;
}

static void test_tool_shutdown(EditorContext* ctx) {
    TestTool* state = (TestTool*)ctx->active_tool->user_data;
    state->shutdown_calls++;
}

static void test_tool_update(EditorContext* ctx, f32 delta_time) {
    TestTool* state = (TestTool*)ctx->active_tool->user_data;
    state->update_calls++;
    state->last_dt = delta_time;
}

static void make_tool(EditorTool* tool, const char* name, TestTool* state) {
    memset(tool, 0, sizeof(*tool));
    tool->name = name;
    tool->description = "test tool";
    tool->init = test_tool_init;
    tool->shutdown = test_tool_shutdown;
    tool->update = test_tool_update;
    tool->user_data = state;
}

// --- Tests -----------------------------------------------------------------------------

static void test_create_and_init(void) {
    EditorContext* ctx = editor_context_create();
    CHECK(ctx != NULL);
    if (!ctx) {
        return;
    }

    CHECK(!ctx->is_initialized);
    CHECK(ctx->tool_count == 0);
    CHECK(editor_get_active_tool(ctx) == NULL);

    CHECK(editor_context_init(ctx));
    CHECK(ctx->is_initialized);
    CHECK(ctx->show_grid);
    CHECK(ctx->show_gizmos);
    CHECK_NEAR(ctx->grid_size, 1.0f);
    CHECK(ctx->undo_capacity == 100);
    CHECK(ctx->viewport.width == 1280);
    CHECK(ctx->viewport.height == 720);

    // A second init keeps the state it already has.
    ctx->grid_size = 5.0f;
    CHECK(editor_context_init(ctx));
    CHECK_NEAR(ctx->grid_size, 5.0f);

    editor_context_destroy(ctx);
}

static void test_shutdown_releases_systems(void) {
    TerrainSculptingSystem terrain;
    MaterialGraph graph;
    EditorContext* ctx = editor_context_create();
    CHECK(ctx != NULL);
    if (!ctx) {
        return;
    }
    CHECK(editor_context_init(ctx));

    // The stubs never dereference these; only their pointer values matter here.
    ctx->terrain_system = &terrain;
    ctx->material_graph = &graph;
    g_terrain_destroy_calls = 0;
    g_material_destroy_calls = 0;

    editor_context_shutdown(ctx);
    CHECK(!ctx->is_initialized);
    CHECK(ctx->terrain_system == NULL);
    CHECK(ctx->material_graph == NULL);
    CHECK(g_terrain_destroy_calls == 1);
    CHECK(g_material_destroy_calls == 1);
    CHECK(ctx->viewport.width == 0);

    // Shutting down an uninitialized context does nothing.
    editor_context_shutdown(ctx);
    CHECK(g_terrain_destroy_calls == 1);

    editor_context_destroy(ctx);
}

static void test_destroy_shuts_down_initialized_context(void) {
    TerrainSculptingSystem terrain;
    EditorContext* ctx = editor_context_create();
    CHECK(ctx != NULL);
    if (!ctx) {
        return;
    }
    CHECK(editor_context_init(ctx));
    ctx->terrain_system = &terrain;

    g_terrain_destroy_calls = 0;
    editor_context_destroy(ctx);
    CHECK(g_terrain_destroy_calls == 1);

    editor_context_destroy(NULL);
}

#define TEST_MAX_TOOLS 16

static void test_tool_registry_limit(void) {
    static EditorTool tools[TEST_MAX_TOOLS + 1];
    static TestTool states[TEST_MAX_TOOLS + 1];
    static char names[TEST_MAX_TOOLS + 1][16];

    EditorContext* ctx = editor_context_create();
    CHECK(ctx != NULL);
    if (!ctx) {
        return;
    }
    CHECK(editor_context_init(ctx));

    for (int i = 0; i <= TEST_MAX_TOOLS; i++) {
        snprintf(names[i], sizeof(names[i]), "tool_%d", i);
        make_tool(&tools[i], names[i], &states[i]);
        states[i].init_ok = true;
    }

    for (int i = 0; i < TEST_MAX_TOOLS; i++) {
        CHECK(editor_register_tool(ctx, &tools[i]));
    }
    CHECK(ctx->tool_count == TEST_MAX_TOOLS);
    CHECK(!editor_register_tool(ctx, &tools[TEST_MAX_TOOLS]));
    CHECK(ctx->tool_count == TEST_MAX_TOOLS);
    CHECK(!editor_register_tool(ctx, NULL));
    CHECK(!editor_register_tool(NULL, &tools[0]));

    editor_context_destroy(ctx);
}

static void test_tool_activation(void) {
    TestTool state_a = {.init_ok = true};
    TestTool state_b = {.init_ok = true};
    TestTool state_c = {.init_ok = false};
    EditorTool tool_a;
    EditorTool tool_b;
    EditorTool tool_c;
    make_tool(&tool_a, "a", &state_a);
    make_tool(&tool_b, "b", &state_b);
    make_tool(&tool_c, "c", &state_c);

    EditorContext* ctx = editor_context_create();
    CHECK(ctx != NULL);
    if (!ctx) {
        return;
    }
    CHECK(editor_context_init(ctx));
    CHECK(editor_register_tool(ctx, &tool_a));
    CHECK(editor_register_tool(ctx, &tool_b));
    CHECK(editor_register_tool(ctx, &tool_c));

    CHECK(editor_set_active_tool(ctx, "a"));
    CHECK(editor_get_active_tool(ctx) == &tool_a);
    CHECK(state_a.init_calls == 1);
    CHECK(state_a.shutdown_calls == 0);

    // Switching shuts the previous tool down before the next one initialises.
    CHECK(editor_set_active_tool(ctx, "b"));
    CHECK(editor_get_active_tool(ctx) == &tool_b);
    CHECK(state_a.shutdown_calls == 1);
    CHECK(state_b.init_calls == 1);

    // An unknown name leaves the active tool alone.
    CHECK(!editor_set_active_tool(ctx, "missing"));
    CHECK(editor_get_active_tool(ctx) == &tool_b);
    CHECK(state_b.shutdown_calls == 0);

    // A failing init clears the active tool.
    CHECK(!editor_set_active_tool(ctx, "c"));
    CHECK(editor_get_active_tool(ctx) == NULL);
    CHECK(state_b.shutdown_calls == 1);
    CHECK(state_c.init_calls == 1);

    CHECK(!editor_set_active_tool(ctx, NULL));
    CHECK(!editor_set_active_tool(NULL, "a"));

    editor_context_destroy(ctx);
}

static void test_update_dispatch(void) {
    TestTool state = {.init_ok = true};
    EditorTool tool;
    make_tool(&tool, "updater", &state);

    EditorContext* ctx = editor_context_create();
    CHECK(ctx != NULL);
    if (!ctx) {
        return;
    }
    CHECK(editor_context_init(ctx));
    CHECK(editor_register_tool(ctx, &tool));
    CHECK(editor_set_active_tool(ctx, "updater"));

    editor_context_update(ctx, 0.25f);
    CHECK(state.update_calls == 1);
    CHECK_NEAR(state.last_dt, 0.25f);

    // Default camera: yaw 0, pitch -30, distance 20, target at the origin.
    CHECK_NEAR(ctx->viewport.camera_position.x, 0.0f);
    CHECK_NEAR(ctx->viewport.camera_position.y, -10.0f);
    CHECK_NEAR(ctx->viewport.camera_position.z, 17.3205f);

    // A failing init leaves no active tool, so the next update does not reach the old one.
    TestTool failing = {.init_ok = false};
    EditorTool failing_tool;
    make_tool(&failing_tool, "failing", &failing);
    CHECK(editor_register_tool(ctx, &failing_tool));
    CHECK(!editor_set_active_tool(ctx, "failing"));
    editor_context_update(ctx, 0.5f);
    CHECK(state.update_calls == 1);

    editor_context_destroy(ctx);
}

static void test_camera_orbit(void) {
    EditorViewport vp;
    CHECK(editor_viewport_init(&vp, 800, 600));
    vp.camera_target.x = 1.0f;
    vp.camera_target.y = 2.0f;
    vp.camera_target.z = 3.0f;
    vp.camera_distance = 10.0f;
    vp.camera_pitch = 0.0f;

    vp.camera_yaw = 0.0f;
    editor_viewport_update_camera(&vp, 0.016f);
    CHECK_NEAR(vp.camera_position.x, 1.0f);
    CHECK_NEAR(vp.camera_position.y, 2.0f);
    CHECK_NEAR(vp.camera_position.z, 13.0f);

    vp.camera_yaw = 90.0f;
    editor_viewport_update_camera(&vp, 0.016f);
    CHECK_NEAR(vp.camera_position.x, 11.0f);
    CHECK_NEAR(vp.camera_position.z, 3.0f);

    editor_viewport_shutdown(&vp);
}

static void test_viewport_drag_rotates_and_clamps(void) {
    EditorViewport vp;
    CHECK(editor_viewport_init(&vp, 800, 600));
    vp.camera_yaw = 0.0f;
    vp.camera_pitch = 0.0f;

    // Pressing starts a drag without rotating.
    editor_viewport_handle_input(&vp, 100, 100, true);
    CHECK(vp.is_rotating);
    CHECK_NEAR(vp.camera_yaw, 0.0f);
    CHECK_NEAR(vp.camera_pitch, 0.0f);

    // Dragging by (+10, -4) turns yaw by +5 and pitch by -2.
    editor_viewport_handle_input(&vp, 110, 96, true);
    CHECK_NEAR(vp.camera_yaw, 5.0f);
    CHECK_NEAR(vp.camera_pitch, -2.0f);

    editor_viewport_handle_input(&vp, 110, 96, false);
    CHECK(!vp.is_rotating);

    // Pitch is clamped to +/-89 degrees.
    vp.camera_pitch = 85.0f;
    vp.is_rotating = true;
    vp.last_mouse_pos.x = 0.0f;
    vp.last_mouse_pos.y = 0.0f;
    editor_viewport_handle_input(&vp, 0, 100, true);
    CHECK_NEAR(vp.camera_pitch, 89.0f);

    editor_viewport_shutdown(&vp);
}

static void test_resize(void) {
    EditorViewport vp;
    CHECK(editor_viewport_init(&vp, 800, 600));

    editor_viewport_resize(&vp, 1024, 768);
    CHECK(vp.width == 1024);
    CHECK(vp.height == 768);

    // Resizing to the current size changes nothing.
    editor_viewport_resize(&vp, 1024, 768);
    CHECK(vp.width == 1024);
    CHECK(vp.height == 768);

    editor_viewport_shutdown(&vp);
}

int main(void) {
    test_create_and_init();
    test_shutdown_releases_systems();
    test_destroy_shuts_down_initialized_context();
    test_tool_registry_limit();
    test_tool_activation();
    test_update_dispatch();
    test_camera_orbit();
    test_viewport_drag_rotates_and_clamps();
    test_resize();

    if (g_failures != 0) {
        fprintf(stderr, "test_editor_context: %d of %d checks failed\n", g_failures, g_checks);
        return 1;
    }
    printf("test_editor_context: %d checks passed\n", g_checks);
    return 0;
}
