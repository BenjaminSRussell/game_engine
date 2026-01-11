// World partition system for large open worlds
#ifndef WORLD_PARTITION_H
#define WORLD_PARTITION_H

#include "include/common.h"
#include "include/math/bounds.h"
#include "include/math/vec3.h"

#define MAX_WORLD_CELLS 4096
#define WORLD_CELL_SIZE 512.0f // 512 meters per cell

typedef enum {
  CELL_UNLOADED,
  CELL_LOADING,
  CELL_LOADED,
  CELL_ACTIVE,
  CELL_UNLOADING
} CellState;

typedef struct {
  i32 x, y; // Grid coordinates
  BoundingBox bounds;
  CellState state;

  void *level_data;
  size_t data_size;

  char file_path[256];
  f32 load_priority;

  u32 entity_count;
  u32 *entity_ids;

  bool persistent; // Never unload
} WorldCell;

typedef struct {
  WorldCell cells[MAX_WORLD_CELLS];
  u32 cell_count;

  Vec3 player_position;
  f32 load_radius;
  f32 unload_radius;

  u32 max_loaded_cells;
  u32 loaded_cell_count;

  char world_name[128];
  char base_path[256];

} WorldPartitionSystem;

#ifdef __cplusplus
extern "C" {
#endif

WorldPartitionSystem *world_partition_create(const char *world_name,
                                             const char *base_path);
void world_partition_destroy(WorldPartitionSystem *system);

void world_partition_update(WorldPartitionSystem *system, Vec3 player_pos,
                            f32 delta_time);
WorldCell *world_partition_get_cell(WorldPartitionSystem *system, i32 x, i32 y);

void world_partition_load_cell(WorldPartitionSystem *system, WorldCell *cell);
void world_partition_unload_cell(WorldPartitionSystem *system, WorldCell *cell);

#ifdef __cplusplus
}
#endif

#endif
