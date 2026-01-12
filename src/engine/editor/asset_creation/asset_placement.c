// Asset Placement System implementation - STUBBED
#include <core/logger.h>
#include <core/types.h>
#include <ecs/ecs.h>
#include <math/vec3.h>
#include <stdbool.h>
#include <string.h>
// #include "asset_placement.h" // Removed to avoid circular/incomplete type
// hell

// Define dummy context locally to satisfy incomplete type usage in stubs
typedef struct {
  Vec3 position;
  Vec3 rotation;
  Vec3 scale;
  // Add enough padding or fields to match usage if needed, but for stubs it
  // matters less as long as we don't access fields that don't exist. The
  // previous code accessed: grid_size, grid_snapping_enabled,
  // collision_check_enabled
  f32 grid_size;
  bool grid_snapping_enabled;
  bool collision_check_enabled;
  bool preview_visible;
  Entity preview_entity;
  void *current_asset;
  i32 current_type; // AssetType
} AssetPlacementContext;

typedef i32 AssetType;       // Dummy
typedef i32 PlacementResult; // Dummy
#define PLACEMENT_SUCCESS 0

// Global asset placement context
static AssetPlacementContext g_context = {0};
static bool g_initialized = false;

// Forward declarations of functions we implement (to match linker expectation)
bool asset_placement_init(void);
void asset_placement_shutdown(void);
void asset_placement_update(float delta_time);
void asset_placement_render(void);
AssetPlacementContext *asset_placement_get_context(void);
void asset_placement_reset_context(void); // Internal or external?

// Implementations

bool asset_placement_init(void) {
  if (g_initialized) {
    return true;
  }

  memset(&g_context, 0, sizeof(AssetPlacementContext));
  g_context.grid_size = 1.0f;
  g_context.grid_snapping_enabled = true;
  g_context.collision_check_enabled = true;

  g_initialized = true;
  LOG_INFO("Asset placement system initialized (STUBBED)");
  return true;
}

void asset_placement_shutdown(void) {
  if (!g_initialized) {
    return;
  }

  // asset_placement_reset_context();
  g_initialized = false;
  LOG_INFO("Asset placement system shutdown (STUBBED)");
}

void asset_placement_update(float delta_time) { (void)delta_time; }

void asset_placement_render(void) {}

AssetPlacementContext *asset_placement_get_context(void) { return &g_context; }

// Stubs for other functions called by engine/editor
// We need to guess signatures or use void* to match generic calls if any.
// But mostly these are specific.

// MARK: - Asset Placement Operations

PlacementResult asset_placement_place(void *asset, Vec3 position) {
  return PLACEMENT_SUCCESS;
}

PlacementResult asset_placement_place_at_cursor(void *asset) {
  return PLACEMENT_SUCCESS;
}

void asset_placement_remove_asset(Vec3 position) {}

void asset_placement_move_asset(Vec3 from_position, Vec3 to_position) {}

// MARK: - Grid and Snapping

void asset_placement_snap_to_grid(Vec3 *position, float grid_size) {}

Vec3 asset_placement_get_snapped_position(Vec3 position, float grid_size) {
  return position;
}

void asset_placement_set_grid_size(float size) { g_context.grid_size = size; }

void asset_placement_toggle_grid_snapping(void) {
  g_context.grid_snapping_enabled = !g_context.grid_snapping_enabled;
}

// MARK: - Collision and Validation

bool asset_placement_is_valid_position(Vec3 position, AssetType type) {
  return true;
}

bool asset_placement_check_collision(Vec3 position, AssetType type) {
  return false;
}

bool asset_placement_check_collision_at_position(Vec3 position,
                                                 AssetType type) {
  return false;
}

bool asset_placement_is_in_bounds(Vec3 position) { return true; }

void asset_placement_toggle_collision_check(void) {
  g_context.collision_check_enabled = !g_context.collision_check_enabled;
}

// MARK: - Preview System

void asset_placement_set_preview_asset(void *asset, AssetType type) {}

void asset_placement_update_preview_position(Vec3 world_position) {}

void asset_placement_show_preview(bool show) {
  g_context.preview_visible = show;
}

void asset_placement_update_preview(void) {}

// MARK: - Asset Management

void asset_placement_set_current_asset(void *asset, AssetType type) {
  g_context.current_asset = asset;
  g_context.current_type = type;
}

void *asset_placement_get_current_asset(void) {
  return g_context.current_asset;
}

AssetType asset_placement_get_current_type(void) {
  return g_context.current_type;
}

// MARK: - Utility Functions

const char *asset_placement_get_result_string(PlacementResult result) {
  return "SUCCESS";
}

bool asset_placement_is_in_placement_mode(void) {
  return g_context.current_asset != NULL;
}

void asset_placement_reset_context(void) {
  memset(&g_context, 0, sizeof(AssetPlacementContext));
  g_context.grid_size = 1.0f;
}

// Add any missing functions identified in logs
void asset_placement_render_preview(void) {}
void asset_placement_render_collision_preview(void) {}
void asset_placement_render_grid(void) {}
float asset_placement_get_ground_height_at_position(float x, float z) {
  return 0.0f;
}
