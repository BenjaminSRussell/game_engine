// Physics Debug API Bridge Implementation
#include "../include/core/logger.h"
#include "../include/platform/physics_debug_api_bridge.h"

// Global state tracking for bridge (since we lack global debug instance access)
static bool g_debug_enabled = false;
// static PhysicsDebugConfig g_config = {0}; // Removed due to type mismatch

void physics_debug_set_enabled(bool enabled) {
  g_debug_enabled = enabled;
  // TODO: Forward to actual system
  LOG_WARN("physics_debug_set_enabled stubbed");
}

bool physics_debug_is_enabled(void) { return g_debug_enabled; }

void physics_debug_show_contacts(bool enabled) {
  LOG_WARN("physics_debug_show_contacts stubbed");
}

void physics_debug_show_constraints(bool enabled) {
  LOG_WARN("physics_debug_show_constraints stubbed");
}

void physics_debug_show_aabbs(bool enabled) {
  LOG_WARN("physics_debug_show_aabbs stubbed");
}

void physics_debug_show_obbs(bool enabled) {
  LOG_WARN("physics_debug_show_obbs stubbed");
}

void physics_debug_show_shapes(bool enabled) {
  LOG_WARN("physics_debug_show_shapes stubbed");
}

void physics_debug_show_velocities(bool enabled) {
  LOG_WARN("physics_debug_show_velocities stubbed");
}

void physics_debug_show_forces(bool enabled) {
  LOG_WARN("physics_debug_show_forces stubbed");
}

void physics_debug_show_center_of_mass(bool enabled) {
  LOG_WARN("physics_debug_show_center_of_mass stubbed");
}

void physics_debug_set_contact_color(float r, float g, float b) {
}

void physics_debug_set_constraint_color(float r, float g, float b) {
}

void physics_debug_set_aabb_color(float r, float g, float b) {
}

void physics_debug_set_velocity_color(float r, float g, float b) {
}

void physics_debug_set_contact_size(float size) {
}

void physics_debug_set_velocity_scale(float scale) {
}

void physics_debug_set_force_scale(float scale) {
}

void physics_debug_get_stats(PhysicsDebugStats *stats) {
  if (stats) {
      // Return zero stats
      stats->active_bodies = 0;
  }
}

