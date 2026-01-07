// Bounds Visualization API Bridge Implementation

#include "../include/core/logger.h"
#include "../include/platform/bounds_visualization_api_bridge.h"
#include <stdbool.h>
#include <stdint.h>

// Mock state
static float g_color[4] = {0.0f, 1.0f, 0.0f, 1.0f};
static float g_width = 1.0f;

void bounds_vis_set_aabb_enabled(uint64_t entity_id, bool enabled) {
  LOG_INFO("Bounds Vis: Entity %llu AABB %s", entity_id,
           enabled ? "Enabled" : "Disabled");
}

void bounds_vis_set_obb_enabled(uint64_t entity_id, bool enabled) {
  LOG_INFO("Bounds Vis: Entity %llu OBB %s", entity_id,
           enabled ? "Enabled" : "Disabled");
}

void bounds_vis_set_sphere_enabled(uint64_t entity_id, bool enabled) {
  LOG_INFO("Bounds Vis: Entity %llu Sphere %s", entity_id,
           enabled ? "Enabled" : "Disabled");
}

bool bounds_vis_get_aabb_enabled(uint64_t entity_id) { return false; }
bool bounds_vis_get_obb_enabled(uint64_t entity_id) { return false; }
bool bounds_vis_get_sphere_enabled(uint64_t entity_id) { return false; }

void bounds_vis_set_color(float r, float g, float b, float a) {
  g_color[0] = r;
  g_color[1] = g;
  g_color[2] = b;
  g_color[3] = a;
}

void bounds_vis_set_width(float width) { g_width = width; }
