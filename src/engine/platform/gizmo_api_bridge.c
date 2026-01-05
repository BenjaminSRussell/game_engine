// Gizmo API Bridge Implementation
#include "../include/core/logger.h"
#include "../include/platform/gizmo_api_bridge.h"
#include <stdbool.h>

// Local definitions removed as they are in gizmo_api_bridge.h

// Global state tracking for bridge
static GizmoMode g_mode = GIZMO_MODE_TRANSLATE;
static GizmoSpace g_space = GIZMO_SPACE_WORLD;
static bool g_enabled = true;
static bool g_snap_enabled = false;
static float g_translation_snap = 1.0f;
static float g_rotation_snap = 15.0f;
static float g_scale_snap = 0.1f;
static float g_size = 1.0f;
static float g_opacity = 1.0f;

void gizmo_set_mode(GizmoMode mode) {
  g_mode = mode;
  LOG_WARN("gizmo_set_mode stubbed due to API conflict");
}

GizmoMode gizmo_get_mode(void) { return g_mode; }

void gizmo_set_space(GizmoSpace space) {
  g_space = space;
  LOG_WARN("gizmo_set_space stubbed due to API conflict");
}

GizmoSpace gizmo_get_space(void) { return g_space; }

void gizmo_set_enabled(bool enabled) {
  g_enabled = enabled;
  LOG_WARN("gizmo_set_enabled stubbed due to API conflict");
}

bool gizmo_is_enabled(void) { return g_enabled; }

void gizmo_set_snap_enabled(bool enabled) {
  g_snap_enabled = enabled;
  LOG_WARN("gizmo_set_snap_enabled stubbed due to API conflict");
}

bool gizmo_is_snap_enabled(void) { return g_snap_enabled; }

void gizmo_set_translation_snap(float value) {
  g_translation_snap = value;
}

float gizmo_get_translation_snap(void) { return g_translation_snap; }

void gizmo_set_rotation_snap(float degrees) {
  g_rotation_snap = degrees;
}

float gizmo_get_rotation_snap(void) { return g_rotation_snap; }

void gizmo_set_scale_snap(float value) {
  g_scale_snap = value;
}

float gizmo_get_scale_snap(void) { return g_scale_snap; }

void gizmo_set_size(float size) {
  g_size = size;
}

float gizmo_get_size(void) { return g_size; }

void gizmo_set_opacity(float opacity) {
  g_opacity = opacity;
}

float gizmo_get_opacity(void) { return g_opacity; }

void gizmo_set_axis_enabled(uint32_t axis_index, bool enabled) {
}

void gizmo_set_axis_color(uint32_t axis_index, float r, float g, float b) {
}

