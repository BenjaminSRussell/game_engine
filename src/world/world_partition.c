/**
 * @file world_partition.c
 * @brief ECS Implementation
 * @description Entity Component System implementation
 * @date 2026-01-13
 */

// src/engine/world/world_partition.c
#include "include/world/world_partition.h"
#include "include/core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

WorldPartitionSystem *world_partition_create(const char *world_name,
                                             const char *base_path) {
  WorldPartitionSystem *sys =
      (WorldPartitionSystem *)calloc(1, sizeof(WorldPartitionSystem));
  if (!sys) {
    LOG_ERROR("Failed to allocate WorldPartitionSystem");
    return NULL;
  }

  strncpy(sys->world_name, world_name, sizeof(sys->world_name) - 1);
  strncpy(sys->base_path, base_path, sizeof(sys->base_path) - 1);

  sys->load_radius = 512.0f;    // 1 cell radius usually
  sys->unload_radius = 1024.0f; // Hysteresis
  sys->max_loaded_cells = 64;

  // Initialize a grid of mock cells for testing/demo
  // In a real system, this would load a manifest file
  int grid_radius = 8;
  for (int x = -grid_radius; x <= grid_radius; x++) {
    for (int y = -grid_radius; y <= grid_radius; y++) {
      if (sys->cell_count >= MAX_WORLD_CELLS)
        break;

      WorldCell *cell = &sys->cells[sys->cell_count++];
      cell->x = x;
      cell->y = y;
      cell->state = CELL_UNLOADED;

      cell->bounds.min = (Vec3){x * WORLD_CELL_SIZE, 0, y * WORLD_CELL_SIZE};
      cell->bounds.max =
          (Vec3){(x + 1) * WORLD_CELL_SIZE, 256.0f, (y + 1) * WORLD_CELL_SIZE};

      snprintf(cell->file_path, sizeof(cell->file_path), "%s/cell_%d_%d.dat",
               base_path, x, y);
    }
  }

  LOG_INFO("World Partition System initialized: %s (%d cells)", world_name,
           sys->cell_count);
  return sys;
}

void world_partition_destroy(WorldPartitionSystem *sys) {
  if (!sys)
    return;

  // Unload all
  for (u32 i = 0; i < sys->cell_count; i++) {
    if (sys->cells[i].state == CELL_LOADED ||
        sys->cells[i].state == CELL_ACTIVE) {
      world_partition_unload_cell(sys, &sys->cells[i]);
    }
  }

  free(sys);
  LOG_INFO("World Partition System destroyed");
}

static f32 vec3_distance_sq(Vec3 a, Vec3 b) {
  f32 dx = a.x - b.x;
  f32 dy = a.y - b.y;
  f32 dz = a.z - b.z;
  return dx * dx + dy * dy + dz * dz;
}

void world_partition_update(WorldPartitionSystem *sys, Vec3 player_pos,
                            f32 delta_time) {
  if (!sys)
    return;

  sys->player_position = player_pos;
  f32 load_sq = sys->load_radius * sys->load_radius;
  f32 unload_sq = sys->unload_radius * sys->unload_radius;

  // Simple update loop
  for (u32 i = 0; i < sys->cell_count; i++) {
    WorldCell *cell = &sys->cells[i];

    // Calculate center of cell
    Vec3 center = {
        (cell->bounds.min.x + cell->bounds.max.x) * 0.5f,
        0.0f, // Ignore Y for distance check usually, or use player height
        (cell->bounds.min.z + cell->bounds.max.z) * 0.5f};

    // 2D distance for terrain streaming primarily
    f32 dx = center.x - player_pos.x;
    f32 dz = center.z - player_pos.z;
    f32 dist_sq = dx * dx + dz * dz;

    if (cell->state == CELL_UNLOADED) {
      if (dist_sq < load_sq) {
        world_partition_load_cell(sys, cell);
      }
    } else if (cell->state == CELL_LOADED || cell->state == CELL_ACTIVE) {
      if (!cell->persistent && dist_sq > unload_sq) {
        world_partition_unload_cell(sys, cell);
      }
    }
  }
}

WorldCell *world_partition_get_cell(WorldPartitionSystem *sys, i32 x, i32 y) {
  if (!sys)
    return NULL;
  // Linear search (Optimization: Hash map)
  for (u32 i = 0; i < sys->cell_count; i++) {
    if (sys->cells[i].x == x && sys->cells[i].y == y) {
      return &sys->cells[i];
    }
  }
  return NULL;
}

void world_partition_load_cell(WorldPartitionSystem *sys, WorldCell *cell) {
  if (!sys || !cell)
    return;

  if (cell->state != CELL_UNLOADED)
    return;

  cell->state = CELL_LOADING;
  // TODO: Async loading
  // For now, synchronous mock load

  LOG_INFO("WP: Loading Cell [%d, %d]", cell->x, cell->y);

  // Simulate data
  cell->data_size = 1024;
  cell->level_data = malloc(cell->data_size); // Mock data
  cell->entity_count = 0;                     // Empty for now

  cell->state = CELL_LOADED;
  sys->loaded_cell_count++;
}

void world_partition_unload_cell(WorldPartitionSystem *sys, WorldCell *cell) {
  if (!sys || !cell)
    return;

  if (cell->state == CELL_UNLOADED)
    return;

  LOG_INFO("WP: Unloading Cell [%d, %d]", cell->x, cell->y);

  if (cell->level_data) {
    free(cell->level_data);
    cell->level_data = NULL;
  }
  cell->data_size = 0;
  cell->entity_count = 0;

  cell->state = CELL_UNLOADED;
  sys->loaded_cell_count--;
}
