#pragma once

#include <core/types.h>
#include <ecs/ecs.h>
#include <math/vec3.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct World World;

// Asset types for placement
typedef enum {
  ASSET_TYPE_BLOCK,
  ASSET_TYPE_ITEM,
  ASSET_TYPE_MOB,
  ASSET_TYPE_PROP,
  ASSET_TYPE_LIGHT
} AssetType;

// Asset placement context
typedef struct {
  AssetType current_type;
  void *current_asset;
  Vec3 preview_position;
  bool preview_visible;
  bool grid_snapping_enabled;
  float grid_size;
  bool collision_check_enabled;
  Entity preview_entity;
} AssetPlacementContext;

// Placement result
typedef enum {
  PLACEMENT_SUCCESS,
  PLACEMENT_FAILED_INVALID_POSITION,
  PLACEMENT_FAILED_COLLISION,
  PLACEMENT_FAILED_OUT_OF_BOUNDS,
  PLACEMENT_FAILED_INVALID_ASSET
} PlacementResult;

// MARK: - Asset Placement System Management

bool asset_placement_init(void);
void asset_placement_shutdown(void);
void asset_placement_update(float delta_time);
void asset_placement_render(void);

AssetPlacementContext *asset_placement_get_context(void);

// MARK: - Asset Placement Operations

PlacementResult asset_placement_place(void *asset, Vec3 position);
PlacementResult asset_placement_place_at_cursor(void *asset);
void asset_placement_remove_asset(Vec3 position);
void asset_placement_move_asset(Vec3 from_position, Vec3 to_position);

// MARK: - Grid and Snapping

void asset_placement_snap_to_grid(Vec3 *position, float grid_size);
Vec3 asset_placement_get_snapped_position(Vec3 position, float grid_size);
void asset_placement_set_grid_size(float size);
void asset_placement_toggle_grid_snapping(void);

// MARK: - Collision and Validation

bool asset_placement_is_valid_position(Vec3 position, AssetType type);
bool asset_placement_check_collision(Vec3 position, AssetType type);
bool asset_placement_is_in_bounds(Vec3 position);
void asset_placement_toggle_collision_check(void);

// MARK: - Preview System

void asset_placement_set_preview_asset(void *asset, AssetType type);
void asset_placement_update_preview_position(Vec3 world_position);
void asset_placement_show_preview(bool show);
void asset_placement_update_preview(void);

// MARK: - Asset Management

void asset_placement_set_current_asset(void *asset, AssetType type);
void *asset_placement_get_current_asset(void);
AssetType asset_placement_get_current_type(void);

// MARK: - Utility Functions

const char *asset_placement_get_result_string(PlacementResult result);
bool asset_placement_is_in_placement_mode(void);
void asset_placement_reset_context(void);
