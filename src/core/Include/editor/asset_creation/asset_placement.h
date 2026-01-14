#pragma once

#include <core/types.h>
#include <math/vec3.h>

#include <ecs/ecs.h>

// Asset Placement System
typedef struct {
  Vec3 position; // Current cursor position?
  Vec3 rotation;
  Vec3 scale;
  u32 asset_id;
  bool is_valid;

  // Grid settings
  float grid_size; // Changed from Vec3 to float based on usage
  bool grid_snapping_enabled;
  bool show_grid_labels;

  // Preview state
  Entity preview_entity;
  bool preview_visible;
  Vec3 preview_position;
  float preview_time;

  // Interactive state
  bool rotation_active;
  float current_rotation;
  bool scale_active;

  // Snapping state
  bool surface_snapping_enabled;
  bool snap_position_valid;
  Vec3 snap_position;
  Vec3 snap_normal;
  bool snap_bounds_valid;
  Vec3 snap_bounds_min;
  Vec3 snap_bounds_max;

  // Collision and Validation
  bool collision_check_enabled;
  bool show_collision_spheres;
  bool show_ground_contact;

  // Asset info
  AssetType current_type;
} AssetPlacementContext;

bool asset_placement_init(void);
void asset_placement_shutdown(void);
void asset_placement_place_asset(u32 asset_id, Vec3 position);
void asset_placement_reset_context(void);
