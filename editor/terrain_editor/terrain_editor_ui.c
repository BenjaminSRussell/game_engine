#include "../editor_common.h"
#include "core/logger.h"
#include <stdio.h>

/**
 * =================================================================================================
 *                           TERRAIN EDITOR UI
 * =================================================================================================
 * 
 * Implements the introspection UI (Sidebar) and 3D overlay for the Terrain Editor.
 */

// Simulated UI state
static struct {
    bool show_brush_settings;
    bool show_stats;
} terrain_ui_state = {true, true};

// Mock drawing functions for sidebar UI
static void ui_begin_window(const char* name) {}
static void ui_end_window(void) {}
static void ui_slider_float(const char* label, f32* value, f32 min, f32 max) {}
static void ui_radio_button(const char* label, bool* active) {}
static void ui_text(const char* fmt, ...) {}

void terrain_editor_ui_render_sidebar(EditorContext* ctx) {
    if (!ctx || !ctx->terrain_system) return;
    TerrainSculptingSystem* ts = ctx->terrain_system;

    ui_begin_window("Terrain Tools");

    // Operation Selection
    ui_text("Operation:");
    bool op_raise = (ts->current_brush.operation == SCULPT_RAISE);
    ui_radio_button("Raise", &op_raise);
    if (op_raise) ts->current_brush.operation = SCULPT_RAISE;
    
    bool op_lower = (ts->current_brush.operation == SCULPT_LOWER);
    ui_radio_button("Lower", &op_lower);
    if (op_lower) ts->current_brush.operation = SCULPT_LOWER;
    
    // Brush Settings
    ui_text("Settings:");
    ui_slider_float("Radius", &ts->current_brush.radius, 1.0f, 100.0f);
    ui_slider_float("Strength", &ts->current_brush.strength, 0.0f, 1.0f);
    ui_slider_float("Hardness", &ts->current_brush.hardness, 0.0f, 1.0f);

    ui_end_window();
}

void terrain_editor_ui_render_viewport_overlay(EditorContext* ctx) {
    if (!ctx || !ctx->terrain_system) return;
    TerrainSculptingSystem* ts = ctx->terrain_system;
    
    // Draw Brush Cursor
    // In a real implementation this would perform a raycast against the terrain geometry
    // and draw a 3D ring or disk at the intersection point.
    
    // For now we simulate the logic:
    // 1. Raycast from camera mouse pos -> terrain
    // 2. If hit, draw circle at hit.position with radius = brush.radius
    
    // Mock call:
    // render_debug_circle3d(hit_pos, ts->current_brush.radius, COLOR_CYAN);
}
