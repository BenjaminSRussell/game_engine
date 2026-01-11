// world_partition.c - World Partition System Implementation
#include <include/editor/scene_tools/world_partition.h>
#include <include/core/logger.h>
#include <include/math/vec3.h>
#include <stdlib.h>
#include <string.h>

#define WORLD_PARTITION_SIZE 64
#define WORLD_PARTITION_LOAD_RADIUS 3
#define WORLD_PARTITION_MAX_LOADED_CELLS 100
#define WORLD_PARTITION_CELL_SIZE 1024.0f

typedef struct {
  int x, z;
  bool loaded;
  bool pending_load;
  bool pending_unload;
  u64 load_time;
  u64 last_access_time;
  void* cell_data;
  u32 entity_count;
} WorldCell;

typedef struct {
  WorldCell cells[WORLD_PARTITION_MAX_LOADED_CELLS];
  u32 loaded_cell_count;
  int player_cell_x, player_cell_z;
  bool is_initialized;
  u64 current_time;
} WorldPartition;

static WorldPartition g_world_partition = {0};

void world_partition_init(void) {
  if (g_world_partition.is_initialized) {
    LOG_WARN("World partition already initialized");
    return;
  }
  
  memset(&g_world_partition, 0, sizeof(WorldPartition));
  g_world_partition.player_cell_x = 0;
  g_world_partition.player_cell_z = 0;
  g_world_partition.current_time = 0;
  g_world_partition.is_initialized = true;
  
  LOG_INFO("World partition initialized (cell size: %.1f, max loaded: %d)", 
           WORLD_PARTITION_CELL_SIZE, WORLD_PARTITION_MAX_LOADED_CELLS);
}

void world_partition_shutdown(void) {
  if (!g_world_partition.is_initialized) return;
  
  // Unload all cells
  for (u32 i = 0; i < g_world_partition.loaded_cell_count; i++) {
    WorldCell *cell = &g_world_partition.cells[i];
    if (cell->loaded && cell->cell_data) {
      world_partition_unload_cell(cell->x, cell->z);
    }
  }
  
  memset(&g_world_partition, 0, sizeof(WorldPartition));
  LOG_INFO("World partition shutdown");
}

void world_partition_update(void) {
  if (!g_world_partition.is_initialized) return;
  
  g_world_partition.current_time++;
  
  // Update load/unload operations
  world_partition_process_load_queue();
  world_partition_process_unload_queue();
  
  // Check if player moved to a new cell
  world_partition_update_player_position();
}

void world_partition_scan(void) {
  if (!g_world_partition.is_initialized) return;
  
  // Scan for cells that should be loaded around player
  int player_x = g_world_partition.player_cell_x;
  int player_z = g_world_partition.player_cell_z;
  
  for (int x = player_x - WORLD_PARTITION_LOAD_RADIUS; 
       x <= player_x + WORLD_PARTITION_LOAD_RADIUS; x++) {
    for (int z = player_z - WORLD_PARTITION_LOAD_RADIUS; 
         z <= player_z + WORLD_PARTITION_LOAD_RADIUS; z++) {
      
      // Check if cell is within circular radius
      int dx = x - player_x;
      int dz = z - player_z;
      float distance = sqrtf(dx * dx + dz * dz);
      
      if (distance <= WORLD_PARTITION_LOAD_RADIUS) {
        if (!world_partition_is_loaded(x, z)) {
          world_partition_load_cell(x, z);
        }
      }
    }
  }
}

void world_partition_load_cell(int x, int z) {
  if (!g_world_partition.is_initialized) return;
  
  // Check if cell is already loaded
  if (world_partition_is_loaded(x, z)) {
    return;
  }
  
  // Check if we have space for more cells
  if (g_world_partition.loaded_cell_count >= WORLD_PARTITION_MAX_LOADED_CELLS) {
    LOG_WARN("Cannot load cell (%d, %d): maximum loaded cells reached", x, z);
    return;
  }
  
  // Find empty slot
  WorldCell *cell = &g_world_partition.cells[g_world_partition.loaded_cell_count];
  memset(cell, 0, sizeof(WorldCell));
  
  cell->x = x;
  cell->z = z;
  cell->loaded = true;
  cell->pending_load = true;
  cell->load_time = g_world_partition.current_time;
  cell->last_access_time = g_world_partition.current_time;
  
  // Simulate cell data loading
  cell->cell_data = malloc(1024 * 1024); // 1MB per cell
  if (cell->cell_data) {
    memset(cell->cell_data, 0, 1024 * 1024);
    
    // Generate some test data
    u32 *test_data = (u32*)cell->cell_data;
    for (u32 i = 0; i < (1024 * 1024) / 4; i++) {
      test_data[i] = (x << 16) | z | (i & 0xFFFF);
    }
  }
  
  // Simulate entity generation
  cell->entity_count = (rand() % 50) + 10;
  
  g_world_partition.loaded_cell_count++;
  
  LOG_DEBUG("Loading cell (%d, %d) with %u entities", x, z, cell->entity_count);
}

void world_partition_unload_cell(int x, int z) {
  if (!g_world_partition.is_initialized) return;
  
  // Find cell
  for (u32 i = 0; i < g_world_partition.loaded_cell_count; i++) {
    WorldCell *cell = &g_world_partition.cells[i];
    
    if (cell->loaded && cell->x == x && cell->z == z) {
      // Free cell data
      if (cell->cell_data) {
        free(cell->cell_data);
        cell->cell_data = NULL;
      }
      
      // Remove cell from array
      if (i < g_world_partition.loaded_cell_count - 1) {
        g_world_partition.cells[i] = g_world_partition.cells[g_world_partition.loaded_cell_count - 1];
      }
      
      g_world_partition.loaded_cell_count--;
      
      LOG_DEBUG("Unloaded cell (%d, %d)", x, z);
      return;
    }
  }
}

bool world_partition_is_loaded(int x, int z) {
  if (!g_world_partition.is_initialized) return false;
  
  for (u32 i = 0; i < g_world_partition.loaded_cell_count; i++) {
    WorldCell *cell = &g_world_partition.cells[i];
    
    if (cell->loaded && cell->x == x && cell->z == z) {
      return true;
    }
  }
  
  return false;
}

void world_partition_set_player_position(const Vec3 *position) {
  if (!g_world_partition.is_initialized || !position) return;
  
  // Convert world position to cell coordinates
  int cell_x = (int)(position->x / WORLD_PARTITION_CELL_SIZE);
  int cell_z = (int)(position->z / WORLD_PARTITION_CELL_SIZE);
  
  if (cell_x != g_world_partition.player_cell_x || 
      cell_z != g_world_partition.player_cell_z) {
    
    g_world_partition.player_cell_x = cell_x;
    g_world_partition.player_cell_z = cell_z;
    
    LOG_DEBUG("Player moved to cell (%d, %d)", cell_x, cell_z);
    
    // Trigger cell loading/unloading
    world_partition_scan();
  }
}

void world_partition_update_player_position(void) {
  // This would be called with actual player position
  // For now, we'll simulate player movement
  static int move_counter = 0;
  move_counter++;
  
  if (move_counter % 100 == 0) { // Move every 100 updates
    Vec3 player_pos = {
      (float)g_world_partition.player_cell_x * WORLD_PARTITION_CELL_SIZE,
      0.0f,
      (float)g_world_partition.player_cell_z * WORLD_PARTITION_CELL_SIZE
    };
    
    // Simulate random movement
    player_pos.x += (rand() % 3 - 1) * WORLD_PARTITION_CELL_SIZE;
    player_pos.z += (rand() % 3 - 1) * WORLD_PARTITION_CELL_SIZE;
    
    world_partition_set_player_position(&player_pos);
  }
}

void world_partition_process_load_queue(void) {
  if (!g_world_partition.is_initialized) return;
  
  for (u32 i = 0; i < g_world_partition.loaded_cell_count; i++) {
    WorldCell *cell = &g_world_partition.cells[i];
    
    if (cell->pending_load) {
      // Simulate load time
      if (g_world_partition.current_time - cell->load_time > 10) {
        cell->pending_load = false;
        LOG_DEBUG("Cell (%d, %d) load completed", cell->x, cell->z);
      }
    }
  }
}

void world_partition_process_unload_queue(void) {
  if (!g_world_partition.is_initialized) return;
  
  for (u32 i = 0; i < g_world_partition.loaded_cell_count; i++) {
    WorldCell *cell = &g_world_partition.cells[i];
    
    if (cell->pending_unload) {
      world_partition_unload_cell(cell->x, cell->z);
      i--; // Adjust index since we removed an element
    }
  }
}

void world_partition_get_loaded_cells(int *out_cells, u32 max_cells, u32 *out_count) {
  if (!g_world_partition.is_initialized || !out_cells || !out_count) return;
  
  u32 count = 0;
  
  for (u32 i = 0; i < g_world_partition.loaded_cell_count && count < max_cells; i++) {
    WorldCell *cell = &g_world_partition.cells[i];
    
    if (cell->loaded && !cell->pending_load) {
      out_cells[count * 2] = cell->x;
      out_cells[count * 2 + 1] = cell->z;
      count++;
    }
  }
  
  *out_count = count;
}

u32 world_partition_get_loaded_cell_count(void) {
  if (!g_world_partition.is_initialized) return 0;
  
  u32 count = 0;
  
  for (u32 i = 0; i < g_world_partition.loaded_cell_count; i++) {
    WorldCell *cell = &g_world_partition.cells[i];
    
    if (cell->loaded && !cell->pending_load) {
      count++;
    }
  }
  
  return count;
}

u32 world_partition_get_entity_count(int x, int z) {
  if (!g_world_partition.is_initialized) return 0;
  
  for (u32 i = 0; i < g_world_partition.loaded_cell_count; i++) {
    WorldCell *cell = &g_world_partition.cells[i];
    
    if (cell->loaded && cell->x == x && cell->z == z) {
      return cell->entity_count;
    }
  }
  
  return 0;
}

void world_partition_get_statistics(u32 *out_loaded_cells, u32 *out_total_entities,
                                   u32 *out_pending_loads, u32 *out_pending_unloads) {
  if (!g_world_partition.is_initialized) return;
  
  u32 loaded_cells = 0;
  u32 total_entities = 0;
  u32 pending_loads = 0;
  u32 pending_unloads = 0;
  
  for (u32 i = 0; i < g_world_partition.loaded_cell_count; i++) {
    WorldCell *cell = &g_world_partition.cells[i];
    
    if (cell->loaded) {
      if (!cell->pending_load) {
        loaded_cells++;
      } else {
        pending_loads++;
      }
      
      total_entities += cell->entity_count;
      
      if (cell->pending_unload) {
        pending_unloads++;
      }
    }
  }
  
  if (out_loaded_cells) *out_loaded_cells = loaded_cells;
  if (out_total_entities) *out_total_entities = total_entities;
  if (out_pending_loads) *out_pending_loads = pending_loads;
  if (out_pending_unloads) *out_pending_unloads = pending_unloads;
}

void world_partition_force_unload_distant_cells(void) {
  if (!g_world_partition.is_initialized) return;
  
  int player_x = g_world_partition.player_cell_x;
  int player_z = g_world_partition.player_cell_z;
  
  for (u32 i = 0; i < g_world_partition.loaded_cell_count; i++) {
    WorldCell *cell = &g_world_partition.cells[i];
    
    if (cell->loaded && !cell->pending_unload) {
      int dx = cell->x - player_x;
      int dz = cell->z - player_z;
      float distance = sqrtf(dx * dx + dz * dz);
      
      // Unload cells that are too far away
      if (distance > WORLD_PARTITION_LOAD_RADIUS + 1) {
        cell->pending_unload = true;
        LOG_DEBUG("Marked cell (%d, %d) for unload (distance: %.1f)", 
                 cell->x, cell->z, distance);
      }
    }
  }
}

void world_partition_update_cell_access_time(int x, int z) {
  if (!g_world_partition.is_initialized) return;
  
  for (u32 i = 0; i < g_world_partition.loaded_cell_count; i++) {
    WorldCell *cell = &g_world_partition.cells[i];
    
    if (cell->loaded && cell->x == x && cell->z == z) {
      cell->last_access_time = g_world_partition.current_time;
      return;
    }
  }
}

bool world_partition_is_cell_in_range(int x, int z, int center_x, int center_z, int radius) {
  int dx = x - center_x;
  int dz = z - center_z;
  float distance = sqrtf(dx * dx + dz * dz);
  return distance <= radius;
}

void world_partition_debug_print_status(void) {
  if (!g_world_partition.is_initialized) return;
  
  LOG_INFO("=== World Partition Status ===");
  LOG_INFO("Player cell: (%d, %d)", g_world_partition.player_cell_x, g_world_partition.player_cell_z);
  LOG_INFO("Loaded cells: %u / %u", world_partition_get_loaded_cell_count(), WORLD_PARTITION_MAX_LOADED_CELLS);
  
  u32 total_entities;
  world_partition_get_statistics(NULL, &total_entities, NULL, NULL);
  LOG_INFO("Total entities: %u", total_entities);
  
  LOG_INFO("Loaded cells:");
  for (u32 i = 0; i < g_world_partition.loaded_cell_count; i++) {
    WorldCell *cell = &g_world_partition.cells[i];
    if (cell->loaded) {
      LOG_INFO("  Cell (%d, %d): %u entities %s%s", 
               cell->x, cell->z, cell->entity_count,
               cell->pending_load ? " [loading]" : "",
               cell->pending_unload ? " [unloading]" : "");
    }
  }
  
  LOG_INFO("=============================");
}
