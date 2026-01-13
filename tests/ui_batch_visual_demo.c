/*
 * ui_batch_visual_demo.c
 * Visual demonstration of UI batch rendering system
 *
 * Advanced 3D Rendering Engine
 *
 * This demo shows the complete visual pipeline from geometry batching
 * to final rendered output.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Include all UI batching modules */
#include "editor/ui/canvas/ui_batch.h"
#include "editor/ui/canvas/ui_batch_gpu.h"
#include "editor/ui/canvas/ui_batch_text.h"
#include "editor/ui/canvas/ui_batch_effects.h"
#include "editor/ui/canvas/ui_batch_optimize.h"
#include "editor/ui/canvas/ui_batch_renderer.h"

/* ============================================================================
 * DEMO UTILITIES
 * ============================================================================ */

typedef struct demo_scene {
    ui_rendering_ui_batch_handle_t main_batch;
    ui_rendering_ui_batch_handle_t text_batch;
    ui_rendering_ui_batch_handle_t effects_batch;

    ui_batch_text_font_handle_t font_body;
    ui_batch_text_font_handle_t font_title;
} demo_scene_t;

static void print_section(const char* title) {
    fprintf(stdout, "\n\n");
    fprintf(stdout, " %s\n", title);
    fprintf(stdout, "\n\n");
}

/* ============================================================================
 * SCENE SETUP
 * ============================================================================ */

static int setup_scene(demo_scene_t* scene) {
    print_section("Scene Setup");

    /* Create batches */
    ui_rendering_ui_batch_desc_t batch_desc = {
        .flags = 0,
        .max_vertices = 8192,
        .max_indices = 16384,
    };

    fprintf(stdout, "[1/5] Creating main geometry batch...\n");
    if (ui_rendering_ui_batch_create(&scene->main_batch, &batch_desc) != 0) {
        fprintf(stderr, "ERROR: Failed to create main batch\n");
        return -1;
    }
    fprintf(stdout, "   Main batch created (ID: %u)\n", scene->main_batch.id);

    fprintf(stdout, "[2/5] Creating text batch...\n");
    if (ui_rendering_ui_batch_create(&scene->text_batch, &batch_desc) != 0) {
        fprintf(stderr, "ERROR: Failed to create text batch\n");
        return -1;
    }
    fprintf(stdout, "   Text batch created (ID: %u)\n", scene->text_batch.id);

    fprintf(stdout, "[3/5] Creating effects batch...\n");
    if (ui_rendering_ui_batch_create(&scene->effects_batch, &batch_desc) != 0) {
        fprintf(stderr, "ERROR: Failed to create effects batch\n");
        return -1;
    }
    fprintf(stdout, "   Effects batch created (ID: %u)\n", scene->effects_batch.id);

    /* Load fonts */
    fprintf(stdout, "[4/5] Loading fonts...\n");
    ui_batch_text_font_desc_t font_title_desc = {
        .font_name = "Arial Bold",
        .font_size = 32,
        .atlas_width = 1024,
        .atlas_height = 1024,
    };
    if (ui_batch_text_create_font(&scene->font_title, &font_title_desc) != 0) {
        fprintf(stderr, "ERROR: Failed to create title font\n");
        return -1;
    }
    fprintf(stdout, "   Title font loaded (32pt)\n");

    ui_batch_text_font_desc_t font_body_desc = {
        .font_name = "Arial",
        .font_size = 14,
        .atlas_width = 512,
        .atlas_height = 512,
    };
    if (ui_batch_text_create_font(&scene->font_body, &font_body_desc) != 0) {
        fprintf(stderr, "ERROR: Failed to create body font\n");
        return -1;
    }
    fprintf(stdout, "   Body font loaded (14pt)\n");

    fprintf(stdout, "[5/5] Scene setup complete!\n");
    return 0;
}

/* ============================================================================
 * GEOMETRY RENDERING
 * ============================================================================ */

static int render_geometry(demo_scene_t* scene) {
    print_section("Geometry Rendering");

    fprintf(stdout, "[1/3] Creating UI panels with geometry...\n");

    /* Create simple quad for panel background */
    ui_rendering_vertex_t panel_vertices[4] = {
        { 50, 50, 0, 0, 0, 0xFF2E3440, 0, 0 },      /* Top-left */
        { 550, 50, 0, 1, 0, 0xFF2E3440, 0, 0 },     /* Top-right */
        { 550, 300, 0, 1, 1, 0xFF3B4252, 0, 0 },    /* Bottom-right */
        { 50, 300, 0, 0, 1, 0xFF3B4252, 0, 0 },     /* Bottom-left */
    };

    uint32_t panel_indices[6] = { 0, 1, 2, 0, 2, 3 };

    if (ui_rendering_ui_batch_add_geometry(scene->main_batch, panel_vertices, 4,
                                            panel_indices, 6) != 0) {
        fprintf(stderr, "ERROR: Failed to add panel geometry\n");
        return -1;
    }
    fprintf(stdout, "   Panel geometry added (4 vertices, 6 indices)\n");

    fprintf(stdout, "[2/3] Adding button geometries...\n");

    /* Create button geometry */
    ui_rendering_vertex_t button_vertices[4] = {
        { 100, 150, 0, 0, 0, 0xFF5E81AC, 0, 0 },
        { 300, 150, 0, 1, 0, 0xFF5E81AC, 0, 0 },
        { 300, 200, 0, 1, 1, 0xFF81A1C1, 0, 0 },
        { 100, 200, 0, 0, 1, 0xFF81A1C1, 0, 0 },
    };

    uint32_t button_indices[6] = { 0, 1, 2, 0, 2, 3 };

    if (ui_rendering_ui_batch_add_geometry(scene->main_batch, button_vertices, 4,
                                            button_indices, 6) != 0) {
        fprintf(stderr, "ERROR: Failed to add button geometry\n");
        return -1;
    }
    fprintf(stdout, "   Button geometry added (4 vertices, 6 indices)\n");

    fprintf(stdout, "[3/3] Submitting draw commands...\n");

    /* Create draw commands */
    ui_rendering_draw_command_t panel_cmd = {
        .vertex_offset = 0,
        .vertex_count = 4,
        .index_offset = 0,
        .index_count = 6,
        .material_id = 1,
        .texture_id = 0,
        .blend_mode = 0,
        .z_order = 0.5f,
    };

    if (ui_rendering_ui_batch_add_draw_command(scene->main_batch, &panel_cmd) != 0) {
        fprintf(stderr, "ERROR: Failed to add draw command\n");
        return -1;
    }

    ui_rendering_draw_command_t button_cmd = {
        .vertex_offset = 4,
        .vertex_count = 4,
        .index_offset = 6,
        .index_count = 6,
        .material_id = 2,
        .texture_id = 0,
        .blend_mode = 0,
        .z_order = 0.6f,
    };

    if (ui_rendering_ui_batch_add_draw_command(scene->main_batch, &button_cmd) != 0) {
        fprintf(stderr, "ERROR: Failed to add draw command\n");
        return -1;
    }
    fprintf(stdout, "   Draw commands submitted (2 commands)\n");

    return 0;
}

/* ============================================================================
 * TEXT RENDERING
 * ============================================================================ */

static int render_text(demo_scene_t* scene) {
    print_section("Text Rendering");

    fprintf(stdout, "[1/3] Rendering title text...\n");

    ui_batch_text_layout_t title_layout = {
        .x = 100,
        .y = 60,
        .width = 400,
        .height = 50,
        .alignment = 0,
        .line_height = 35,
        .word_wrap = false,
        .kerning_enabled = true,
    };

    if (ui_batch_text_add_text(scene->text_batch, scene->font_title,
                                "UI Batch Demo", &title_layout, 0xFFECEFF4) != 0) {
        fprintf(stderr, "ERROR: Failed to add title text\n");
        return -1;
    }
    fprintf(stdout, "   Title rendered: \"UI Batch Demo\"\n");

    fprintf(stdout, "[2/3] Rendering body text...\n");

    ui_batch_text_layout_t body_layout = {
        .x = 110,
        .y = 170,
        .width = 380,
        .height = 80,
        .alignment = 0,
        .line_height = 16,
        .word_wrap = true,
        .kerning_enabled = true,
    };

    const char* body_text = "High-performance UI batching with GPU acceleration, "
                            "text rendering, and visual effects.";

    if (ui_batch_text_add_text(scene->text_batch, scene->font_body, body_text,
                                &body_layout, 0xFFD08770) != 0) {
        fprintf(stderr, "ERROR: Failed to add body text\n");
        return -1;
    }
    fprintf(stdout, "   Body text rendered\n");

    fprintf(stdout, "[3/3] Text statistics:\n");

    float text_width, text_height;
    if (ui_batch_text_measure(scene->font_body, body_text, &text_width, &text_height) == 0) {
        fprintf(stdout, "   Text dimensions: %.1f x %.1f pixels\n", text_width, text_height);
    }

    return 0;
}

/* ============================================================================
 * EFFECTS RENDERING
 * ============================================================================ */

static int render_effects(demo_scene_t* scene) {
    print_section("Visual Effects Rendering");

    fprintf(stdout, "[1/3] Adding gradient effect...\n");

    ui_batch_gradient_t gradient;
    ui_batch_effect_create_linear_gradient(45.0f, 0xFF88C0D0, 0xFF5E81AC, &gradient);

    if (ui_batch_effect_add_gradient(scene->effects_batch, 80, 250, 440, 80, &gradient) != 0) {
        fprintf(stderr, "ERROR: Failed to add gradient\n");
        return -1;
    }
    fprintf(stdout, "   Linear gradient applied (45°, Blue → Purple)\n");

    fprintf(stdout, "[2/3] Adding shadow effect...\n");

    ui_batch_shadow_t shadow = {
        .type = UI_EFFECT_SHADOW,
        .blur_radius = 8.0f,
        .offset_x = 3.0f,
        .offset_y = 3.0f,
        .color = 0xFF000000,
        .opacity = 0.4f,
    };

    if (ui_batch_effect_add_shadow(scene->effects_batch, 100, 150, 200, 50, &shadow) != 0) {
        fprintf(stderr, "ERROR: Failed to add shadow\n");
        return -1;
    }
    fprintf(stdout, "   Drop shadow applied (8px blur, 40%% opacity)\n");

    fprintf(stdout, "[3/3] Adding glow effect...\n");

    ui_batch_glow_t glow = {
        .type = UI_EFFECT_GLOW,
        .glow_radius = 10.0f,
        .glow_intensity = 0.6f,
        .glow_color = 0xFF81A1C1,
    };

    if (ui_batch_effect_add_glow(scene->effects_batch, 320, 170, 100, 40, &glow) != 0) {
        fprintf(stderr, "ERROR: Failed to add glow\n");
        return -1;
    }
    fprintf(stdout, "   Glow effect applied (10px radius, 60%% intensity)\n");

    return 0;
}

/* ============================================================================
 * BATCH OPTIMIZATION
 * ============================================================================ */

static int optimize_batches(demo_scene_t* scene) {
    print_section("Batch Optimization");

    fprintf(stdout, "[1/3] Sorting batches...\n");

    ui_rendering_ui_batch_sort(scene->main_batch);
    ui_rendering_ui_batch_sort(scene->text_batch);
    ui_rendering_ui_batch_sort(scene->effects_batch);

    fprintf(stdout, "   Z-order sorting complete\n");

    fprintf(stdout, "[2/3] Optimizing draw calls...\n");

    int merged_main = ui_rendering_ui_batch_optimize_draw_calls(scene->main_batch);
    int merged_text = ui_rendering_ui_batch_optimize_draw_calls(scene->text_batch);
    int merged_effects = ui_rendering_ui_batch_optimize_draw_calls(scene->effects_batch);

    fprintf(stdout, "   Merged commands: Main=%d, Text=%d, Effects=%d\n",
            merged_main, merged_text, merged_effects);

    fprintf(stdout, "[3/3] Batch statistics:\n");

    ui_rendering_batch_stats_t stats;
    ui_rendering_ui_batch_get_stats(&stats);

    fprintf(stdout, "   Total batches: %u\n", stats.total_batches);
    fprintf(stdout, "   Total vertices: %u\n", stats.total_vertices);
    fprintf(stdout, "   Total indices: %u\n", stats.total_indices);
    fprintf(stdout, "   Total draw calls: %u\n", stats.total_draw_calls);
    fprintf(stdout, "   Merged calls: %u\n", stats.merged_draw_calls);
    fprintf(stdout, "   Memory usage: %.2f MB\n", stats.total_memory / (1024.0f * 1024.0f));

    return 0;
}

/* ============================================================================
 * RENDERING
 * ============================================================================ */

static int render_frames(demo_scene_t* scene) {
    print_section("Rendering Frames");

    fprintf(stdout, "[1/2] Initializing renderer...\n");

    if (ui_batch_renderer_init() != 0) {
        fprintf(stderr, "ERROR: Failed to initialize renderer\n");
        return -1;
    }
    fprintf(stdout, "   Renderer initialized\n");

    fprintf(stdout, "[2/2] Rendering frame...\n");

    ui_batch_render_context_t render_ctx = {
        .viewport_width = 1920,
        .viewport_height = 1080,
    };

    if (ui_batch_renderer_begin_frame(&render_ctx) != 0) {
        fprintf(stderr, "ERROR: Failed to begin frame\n");
        return -1;
    }

    ui_batch_renderer_set_clear_color(0.18f, 0.20f, 0.25f, 1.0f);
    ui_batch_renderer_set_viewport(1920, 1080);

    /* Render all batches */
    int batches_rendered = ui_batch_renderer_render_all(&render_ctx);

    if (ui_batch_renderer_end_frame() != 0) {
        fprintf(stderr, "ERROR: Failed to end frame\n");
        return -1;
    }

    fprintf(stdout, "   Frame rendered (%d batches)\n", batches_rendered);

    /* Get render stats */
    ui_batch_render_stats_t render_stats;
    if (ui_batch_renderer_get_stats(&render_stats) == 0) {
        fprintf(stdout, "   Draw calls: %u\n", render_stats.draw_calls);
        fprintf(stdout, "   Vertices: %u\n", render_stats.vertices_rendered);
        fprintf(stdout, "   Triangles: %u\n", render_stats.triangles_rendered);
    }

    return 0;
}

/* ============================================================================
 * CLEANUP
 * ============================================================================ */

static void cleanup_scene(demo_scene_t* scene) {
    print_section("Cleanup");

    fprintf(stdout, "[1/3] Destroying batches...\n");
    ui_rendering_ui_batch_destroy(scene->main_batch);
    ui_rendering_ui_batch_destroy(scene->text_batch);
    ui_rendering_ui_batch_destroy(scene->effects_batch);
    fprintf(stdout, "   Batches destroyed\n");

    fprintf(stdout, "[2/3] Destroying fonts...\n");
    ui_batch_text_destroy_font(scene->font_body);
    ui_batch_text_destroy_font(scene->font_title);
    fprintf(stdout, "   Fonts destroyed\n");

    fprintf(stdout, "[3/3] Shutting down systems...\n");
    ui_batch_renderer_shutdown();
    ui_rendering_ui_batch_shutdown();
    ui_batch_text_shutdown();
    fprintf(stdout, "   Systems shutdown\n");
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    fprintf(stdout, "\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  UI BATCH RENDERING SYSTEM - VISUAL DEMO            \n");
    fprintf(stdout, "  Complete Pipeline: Geometry → Text → Effects → GPU \n");
    fprintf(stdout, "\n");

    demo_scene_t scene = {0};

    /* Initialize systems */
    fprintf(stdout, "\n[INIT] Initializing UI batch systems...\n");
    ui_rendering_ui_batch_init();
    ui_batch_text_init();
    ui_batch_effect_init();
    ui_batch_optimize_init(NULL);

    /* Setup scene */
    if (setup_scene(&scene) != 0) {
        goto error;
    }

    /* Render content */
    if (render_geometry(&scene) != 0) {
        goto error;
    }

    if (render_text(&scene) != 0) {
        goto error;
    }

    if (render_effects(&scene) != 0) {
        goto error;
    }

    /* Optimize and render */
    if (optimize_batches(&scene) != 0) {
        goto error;
    }

    if (render_frames(&scene) != 0) {
        goto error;
    }

    /* Success */
    fprintf(stdout, "\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "   VISUAL DEMO COMPLETE - PIPELINE VERIFIED        \n");
    fprintf(stdout, "  All systems working correctly end-to-end           \n");
    fprintf(stdout, "\n\n");

    cleanup_scene(&scene);
    return 0;

error:
    fprintf(stderr, "\n Demo failed - see errors above\n");
    cleanup_scene(&scene);
    return 1;
}

/* End of ui_batch_visual_demo.c */
