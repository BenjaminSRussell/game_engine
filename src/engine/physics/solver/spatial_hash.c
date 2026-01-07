#include "physics/spatial_hash.h"
#include "core/logger.h"
#include "core/memory.h"
#include "math/math.h"
#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

// Global spatial hash instance
static SpatialHash g_spatial_hash = {0};

// Prime numbers for hash function
static const uint32_t HASH_PRIMES[3] = {73856093, 19349663, 83492791};

// Forward declaration
bool spatial_hash_aabb_intersect(Vec3 min_a, Vec3 max_a, Vec3 min_b,
                                 Vec3 max_b);

bool spatial_hash_init(SpatialHash *hash, const SpatialHashConfig *config) {
  if (!hash || !config)
    return false;

  memset(hash, 0, sizeof(SpatialHash));
  hash->config = *config;

  // Ensure table size is power of two
  if (!spatial_hash_is_power_of_two(hash->config.table_size)) {
    hash->config.table_size =
        spatial_hash_next_power_of_two(hash->config.table_size);
  }

  // Allocate hash table
  hash->cells = (SpatialHashCell *)core_alloc(hash->config.table_size *
                                              sizeof(SpatialHashCell));
  if (!hash->cells)
    return false;
  memset(hash->cells, 0, hash->config.table_size * sizeof(SpatialHashCell));

  // Allocate cell indices (for collision detection)
  hash->cell_indices =
      (uint32_t *)core_alloc(hash->config.table_size * sizeof(uint32_t));
  if (!hash->cell_indices) {
    core_free(hash->cells);
    return false;
  }

  // Initialize hash table
  for (uint32_t i = 0; i < hash->config.table_size; i++) {
    hash->cells[i].object_indices = NULL;
    hash->cells[i].object_count = 0;
    hash->cells[i].capacity = 0;
    hash->cells[i].dirty = false;
    hash->cell_indices[i] = UINT32_MAX;
  }

  // Allocate objects
  hash->object_capacity = hash->config.max_objects;
  hash->objects = (SpatialHashObject *)core_alloc(hash->object_capacity *
                                                  sizeof(SpatialHashObject));
  if (!hash->objects) {
    core_free(hash->cells);
    core_free(hash->cell_indices);
    return false;
  }
  memset(hash->objects, 0, hash->object_capacity * sizeof(SpatialHashObject));

  // Allocate free indices
  hash->free_capacity = hash->object_capacity;
  hash->free_indices =
      (uint32_t *)core_alloc(hash->free_capacity * sizeof(uint32_t));
  if (!hash->free_indices) {
    core_free(hash->cells);
    core_free(hash->cell_indices);
    core_free(hash->objects);
    return false;
  }

  // Initialize free indices
  for (uint32_t i = 0; i < hash->free_capacity; i++) {
    hash->free_indices[i] = i;
  }
  hash->free_count = hash->free_capacity;

  // Initialize query buffer
  hash->query_buffer.capacity =
      hash->config.max_collisions_per_cell * 16; // Estimate total capacity
  hash->query_buffer.entity_ids =
      (EntityID *)core_alloc(hash->query_buffer.capacity * sizeof(EntityID));
  if (!hash->query_buffer.entity_ids) {
    core_free(hash->cells);
    core_free(hash->cell_indices);
    core_free(hash->objects);
    core_free(hash->free_indices);
    return false;
  }

  // Calculate memory usage
  hash->memory.hash_table_size =
      hash->config.table_size * sizeof(SpatialHashCell);
  hash->memory.objects_size = hash->object_capacity * sizeof(SpatialHashObject);
  hash->memory.total_size =
      hash->memory.hash_table_size + hash->memory.objects_size;

  LOG_INFO("Spatial hash initialized: %u cells, %u objects, cell size: %.2f",
           hash->config.table_size, hash->object_capacity,
           hash->config.cell_size);
  return true;
}

void spatial_hash_cleanup(SpatialHash *hash) {
  if (!hash)
    return;

  // Free cell object arrays
  for (uint32_t i = 0; i < hash->config.table_size; i++) {
    if (hash->cells[i].object_indices) {
      core_free(hash->cells[i].object_indices);
    }
  }

  // Free main allocations
  if (hash->cells)
    core_free(hash->cells);
  if (hash->cell_indices)
    core_free(hash->cell_indices);
  if (hash->objects)
    core_free(hash->objects);
  if (hash->free_indices)
    core_free(hash->free_indices);
  if (hash->query_buffer.entity_ids)
    core_free(hash->query_buffer.entity_ids);

  memset(hash, 0, sizeof(SpatialHash));
  LOG_INFO("Spatial hash cleaned up");
}

uint32_t spatial_hash_add_object(SpatialHash *hash, EntityID entity_id,
                                 Vec3 position, Vec3 bounds_min,
                                 Vec3 bounds_max, bool is_static,
                                 uint32_t collision_mask,
                                 uint32_t collision_group) {
  if (!hash || hash->object_count >= hash->object_capacity)
    return UINT32_MAX;

  // Get free object index
  if (hash->free_count == 0)
    return UINT32_MAX;
  uint32_t object_index = hash->free_indices[--hash->free_count];

  // Initialize object
  SpatialHashObject *object = &hash->objects[object_index];
  object->entity_id = entity_id;
  object->position = position;
  object->bounds_min = bounds_min;
  object->bounds_max = bounds_max;
  object->active = true;
  object->static_object = is_static;
  object->collision_mask = collision_mask;
  object->collision_group = collision_group;

  // Calculate hash and cell coordinates
  object->hash_value = spatial_hash_hash_position(hash, position);
  spatial_hash_position_to_cell_coords(hash, position, &object->cell_x,
                                       &object->cell_y, &object->cell_z);

  // Add object to appropriate cells
  spatial_hash_add_object_to_cells(hash, object_index);

  hash->object_count++;
  if (is_static) {
    hash->stats.static_objects++;
  } else {
    hash->stats.active_objects++;
  }

  return object_index;
}

bool spatial_hash_remove_object(SpatialHash *hash, EntityID entity_id) {
  if (!hash)
    return false;

  // Find object
  uint32_t object_index = UINT32_MAX;
  for (uint32_t i = 0; i < hash->object_count; i++) {
    if (hash->objects[i].entity_id == entity_id && hash->objects[i].active) {
      object_index = i;
      break;
    }
  }

  if (object_index == UINT32_MAX)
    return false;

  SpatialHashObject *object = &hash->objects[object_index];

  // Remove from cells
  spatial_hash_remove_object_from_cells(hash, object_index);

  // Deactivate object
  object->active = false;

  // Add to free indices
  if (hash->free_count < hash->free_capacity) {
    hash->free_indices[hash->free_count++] = object_index;
  }

  hash->object_count--;
  if (object->static_object) {
    hash->stats.static_objects--;
  } else {
    hash->stats.active_objects--;
  }

  return true;
}

bool spatial_hash_update_object(SpatialHash *hash, EntityID entity_id,
                                Vec3 new_position, Vec3 new_bounds_min,
                                Vec3 new_bounds_max) {
  if (!hash)
    return false;

  // Find object
  uint32_t object_index = UINT32_MAX;
  for (uint32_t i = 0; i < hash->object_count; i++) {
    if (hash->objects[i].entity_id == entity_id && hash->objects[i].active) {
      object_index = i;
      break;
    }
  }

  if (object_index == UINT32_MAX)
    return false;

  SpatialHashObject *object = &hash->objects[object_index];

  // Check if object moved to different cells
  uint32_t old_cell_x = object->cell_x;
  uint32_t old_cell_y = object->cell_y;
  uint32_t old_cell_z = object->cell_z;

  object->position = new_position;
  object->bounds_min = new_bounds_min;
  object->bounds_max = new_bounds_max;

  spatial_hash_position_to_cell_coords(hash, new_position, &object->cell_x,
                                       &object->cell_y, &object->cell_z);

  // Update cells if position changed significantly
  if (old_cell_x != object->cell_x || old_cell_y != object->cell_y ||
      old_cell_z != object->cell_z) {
    spatial_hash_remove_object_from_cells(hash, object_index);
    spatial_hash_add_object_to_cells(hash, object_index);
  }

  return true;
}

void spatial_hash_add_object_to_cells(SpatialHash *hash,
                                      uint32_t object_index) {
  if (!hash || object_index >= hash->object_capacity)
    return;

  SpatialHashObject *object = &hash->objects[object_index];
  if (!object->active)
    return;

  // Calculate bounds in cell coordinates
  Vec3 min_cell = {floorf((object->bounds_min.x - hash->config.world_min.x) /
                          hash->config.cell_size),
                   floorf((object->bounds_min.y - hash->config.world_min.y) /
                          hash->config.cell_size),
                   floorf((object->bounds_min.z - hash->config.world_min.z) /
                          hash->config.cell_size)};

  Vec3 max_cell = {floorf((object->bounds_max.x - hash->config.world_min.x) /
                          hash->config.cell_size),
                   floorf((object->bounds_max.y - hash->config.world_min.y) /
                          hash->config.cell_size),
                   floorf((object->bounds_max.z - hash->config.world_min.z) /
                          hash->config.cell_size)};

  // Add object to all cells it spans
  for (int x = (int)min_cell.x; x <= (int)max_cell.x; x++) {
    for (int y = (int)min_cell.y; y <= (int)max_cell.y; y++) {
      for (int z = (int)min_cell.z; z <= (int)max_cell.z; z++) {
        if (spatial_hash_is_valid_cell_coords(hash, x, y, z)) {
          uint32_t cell_index = spatial_hash_get_cell_index(hash, x, y, z);
          spatial_hash_add_to_cell(hash, cell_index, object_index);
        }
      }
    }
  }
}

void spatial_hash_remove_object_from_cells(SpatialHash *hash,
                                           uint32_t object_index) {
  if (!hash || object_index >= hash->object_capacity)
    return;

  SpatialHashObject *object = &hash->objects[object_index];
  if (!object->active)
    return;

  // Calculate bounds in cell coordinates
  Vec3 min_cell = {floorf((object->bounds_min.x - hash->config.world_min.x) /
                          hash->config.cell_size),
                   floorf((object->bounds_min.y - hash->config.world_min.y) /
                          hash->config.cell_size),
                   floorf((object->bounds_min.z - hash->config.world_min.z) /
                          hash->config.cell_size)};

  Vec3 max_cell = {floorf((object->bounds_max.x - hash->config.world_min.x) /
                          hash->config.cell_size),
                   floorf((object->bounds_max.y - hash->config.world_min.y) /
                          hash->config.cell_size),
                   floorf((object->bounds_max.z - hash->config.world_min.z) /
                          hash->config.cell_size)};

  // Remove object from all cells it spans
  for (int x = (int)min_cell.x; x <= (int)max_cell.x; x++) {
    for (int y = (int)min_cell.y; y <= (int)max_cell.y; y++) {
      for (int z = (int)min_cell.z; z <= (int)max_cell.z; z++) {
        if (spatial_hash_is_valid_cell_coords(hash, x, y, z)) {
          uint32_t cell_index = spatial_hash_get_cell_index(hash, x, y, z);
          spatial_hash_remove_from_cell(hash, cell_index, object_index);
        }
      }
    }
  }
}

void spatial_hash_add_to_cell(SpatialHash *hash, uint32_t cell_index,
                              uint32_t object_index) {
  if (!hash || cell_index >= hash->config.table_size ||
      object_index >= hash->object_capacity)
    return;

  SpatialHashCell *cell = &hash->cells[cell_index];

  // Check if object already in cell
  for (uint32_t i = 0; i < cell->object_count; i++) {
    if (cell->object_indices[i] == object_index) {
      return; // Already in cell
    }
  }

  // Expand cell capacity if needed
  if (cell->object_count >= cell->capacity) {
    uint32_t new_capacity = cell->capacity == 0 ? 4 : cell->capacity * 2;
    uint32_t *new_indices = (uint32_t *)core_realloc(
        cell->object_indices, new_capacity * sizeof(uint32_t));
    if (!new_indices)
      return;

    cell->object_indices = new_indices;
    cell->capacity = new_capacity;
  }

  // Add object to cell
  cell->object_indices[cell->object_count++] = object_index;
  cell->dirty = true;

  // Update statistics
  if (cell->object_count > hash->stats.max_objects_per_cell) {
    hash->stats.max_objects_per_cell = cell->object_count;
  }
}

void spatial_hash_remove_from_cell(SpatialHash *hash, uint32_t cell_index,
                                   uint32_t object_index) {
  if (!hash || cell_index >= hash->config.table_size)
    return;

  SpatialHashCell *cell = &hash->cells[cell_index];

  // Find and remove object
  for (uint32_t i = 0; i < cell->object_count; i++) {
    if (cell->object_indices[i] == object_index) {
      // Remove by shifting remaining elements
      for (uint32_t j = i; j < cell->object_count - 1; j++) {
        cell->object_indices[j] = cell->object_indices[j + 1];
      }
      cell->object_count--;
      cell->dirty = true;
      return;
    }
  }
}

SpatialHashQueryResult *
spatial_hash_query_aabb(SpatialHash *hash, Vec3 bounds_min, Vec3 bounds_max) {
  if (!hash)
    return NULL;

  // Reset query buffer
  hash->query_buffer.count = 0;

  // Calculate bounds in cell coordinates
  Vec3 min_cell = {floorf((bounds_min.x - hash->config.world_min.x) /
                          hash->config.cell_size),
                   floorf((bounds_min.y - hash->config.world_min.y) /
                          hash->config.cell_size),
                   floorf((bounds_min.z - hash->config.world_min.z) /
                          hash->config.cell_size)};

  Vec3 max_cell = {floorf((bounds_max.x - hash->config.world_min.x) /
                          hash->config.cell_size),
                   floorf((bounds_max.y - hash->config.world_min.y) /
                          hash->config.cell_size),
                   floorf((bounds_max.z - hash->config.world_min.z) /
                          hash->config.cell_size)};

  // Query all cells that overlap with bounds
  for (int x = (int)min_cell.x; x <= (int)max_cell.x; x++) {
    for (int y = (int)min_cell.y; y <= (int)max_cell.y; y++) {
      for (int z = (int)min_cell.z; z <= (int)max_cell.z; z++) {
        if (spatial_hash_is_valid_cell_coords(hash, x, y, z)) {
          uint32_t cell_index = spatial_hash_get_cell_index(hash, x, y, z);
          SpatialHashCell *cell = &hash->cells[cell_index];

          // Add all objects in this cell to result
          for (uint32_t i = 0;
               i < cell->object_count &&
               hash->query_buffer.count < hash->query_buffer.capacity;
               i++) {
            uint32_t object_index = cell->object_indices[i];
            if (object_index < hash->object_capacity &&
                hash->objects[object_index].active) {
              SpatialHashObject *object = &hash->objects[object_index];

              // Check if object actually overlaps with query bounds
              if (spatial_hash_aabb_intersect(object->bounds_min,
                                              object->bounds_max, bounds_min,
                                              bounds_max)) {
                hash->query_buffer.entity_ids[hash->query_buffer.count++] =
                    object->entity_id;
              }
            }
          }
        }
      }
    }
  }

  hash->stats.total_queries++;
  return &hash->query_buffer;
}

void spatial_hash_broad_phase_collision(SpatialHash *hash) {
  if (!hash)
    return;

  float start_time = get_time();

  hash->stats.broad_phase_tests = 0;
  hash->stats.narrow_phase_tests = 0;
  hash->stats.occupied_cells = 0;

  // Clear cell indices
  for (uint32_t i = 0; i < hash->config.table_size; i++) {
    hash->cell_indices[i] = UINT32_MAX;
  }

  // Collect all occupied cells
  uint32_t occupied_cell_count = 0;
  for (uint32_t i = 0; i < hash->config.table_size; i++) {
    if (hash->cells[i].object_count > 0) {
      hash->cell_indices[occupied_cell_count++] = i;
      hash->stats.occupied_cells++;
    }
  }

  // For each occupied cell, check collisions between objects
  for (uint32_t i = 0; i < occupied_cell_count; i++) {
    uint32_t cell_index = hash->cell_indices[i];
    SpatialHashCell *cell = &hash->cells[cell_index];

    // Check collisions within this cell
    for (uint32_t j = 0; j < cell->object_count; j++) {
      uint32_t obj_a_index = cell->object_indices[j];
      SpatialHashObject *obj_a = &hash->objects[obj_a_index];

      if (!obj_a->active)
        continue;

      for (uint32_t k = j + 1; k < cell->object_count; k++) {
        uint32_t obj_b_index = cell->object_indices[k];
        SpatialHashObject *obj_b = &hash->objects[obj_b_index];

        if (!obj_b->active)
          continue;

        // Check collision masks
        if (!spatial_hash_can_collide(obj_a, obj_b))
          continue;

        // Check AABB overlap
        if (spatial_hash_aabb_intersect(obj_a->bounds_min, obj_a->bounds_max,
                                        obj_b->bounds_min, obj_b->bounds_max)) {
          hash->stats.narrow_phase_tests++;
          // In a real implementation, this would add to a collision list
          // For now, we just count the potential collision
        }

        hash->stats.broad_phase_tests++;
      }
    }
  }

  hash->stats.hash_time = get_time() - start_time;

  // Calculate efficiency
  if (hash->stats.total_objects > 0) {
    hash->stats.average_load_factor =
        (float)hash->stats.occupied_cells / hash->config.table_size;
    hash->stats.hash_efficiency =
        1.0f - (float)hash->stats.broad_phase_tests /
                   (hash->stats.total_objects * hash->stats.total_objects);
  }
}

uint32_t spatial_hash_hash_coords(SpatialHash *hash, uint32_t x, uint32_t y,
                                  uint32_t z) {
  if (!hash)
    return 0;

  // Use prime number hash function
  return (x * HASH_PRIMES[0] ^ y * HASH_PRIMES[1] ^ z * HASH_PRIMES[2]) &
         (hash->config.table_size - 1);
}

void spatial_hash_position_to_cell_coords(SpatialHash *hash, Vec3 position,
                                          uint32_t *x, uint32_t *y,
                                          uint32_t *z) {
  if (!hash || !x || !y || !z)
    return;

  *x = (uint32_t)floorf((position.x - hash->config.world_min.x) /
                        hash->config.cell_size);
  *y = (uint32_t)floorf((position.y - hash->config.world_min.y) /
                        hash->config.cell_size);
  *z = (uint32_t)floorf((position.z - hash->config.world_min.z) /
                        hash->config.cell_size);
}

uint32_t spatial_hash_get_cell_index(SpatialHash *hash, uint32_t x, uint32_t y,
                                     uint32_t z) {
  if (!hash)
    return 0;

  return spatial_hash_hash_coords(hash, x, y, z);
}

bool spatial_hash_aabb_intersect(Vec3 min_a, Vec3 max_a, Vec3 min_b,
                                 Vec3 max_b) {
  return (min_a.x <= max_b.x && max_a.x >= min_b.x) &&
         (min_a.y <= max_b.y && max_a.y >= min_b.y) &&
         (min_a.z <= max_b.z && max_a.z >= min_b.z);
}

bool spatial_hash_can_collide(const SpatialHashObject *obj_a,
                              const SpatialHashObject *obj_b) {
  if (!obj_a || !obj_b)
    return false;

  // Check collision masks
  if ((obj_a->collision_mask & obj_b->collision_group) == 0)
    return false;
  if ((obj_b->collision_mask & obj_a->collision_group) == 0)
    return false;

  return true;
}

void spatial_hash_get_performance_stats(
    const SpatialHash *hash, uint32_t *total_objects, uint32_t *occupied_cells,
    float *load_factor, float *hash_efficiency, float *hash_time) {
  if (!hash)
    return;

  if (total_objects)
    *total_objects = hash->stats.total_objects;
  if (occupied_cells)
    *occupied_cells = hash->stats.occupied_cells;
  if (load_factor)
    *load_factor = hash->stats.average_load_factor;
  if (hash_efficiency)
    *hash_efficiency = hash->stats.hash_efficiency;
  if (hash_time)
    *hash_time = hash->stats.hash_time;
}

bool spatial_hash_is_power_of_two(uint32_t value) {
  return value != 0 && (value & (value - 1)) == 0;
}

uint32_t spatial_hash_next_power_of_two(uint32_t value) {
  if (value == 0)
    return 1;

  value--;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  value++;

  return value;
}

// Global accessor functions
SpatialHash *get_spatial_hash(void) { return &g_spatial_hash; }

bool init_spatial_hash(const SpatialHashConfig *config) {
  return spatial_hash_init(&g_spatial_hash, config);
}

void cleanup_spatial_hash(void) { spatial_hash_cleanup(&g_spatial_hash); }
