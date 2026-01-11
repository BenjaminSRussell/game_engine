// environment/vegetation/foliage_system.c
// GPU foliage implementation
#include "include/environment/vegetation/foliage_system.h"
#include "include/core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

FoliageSystem *foliage_create(id<MTLDevice> device, f32 world_size,
                              f32 cell_size) {
  FoliageSystem *foliage = (FoliageSystem *)calloc(1, sizeof(FoliageSystem));
  foliage->cell_size = cell_size;

  // Calculate spatial grid
  u32 cells_per_side = (u32)(world_size / cell_size);
  foliage->cell_count = cells_per_side * cells_per_side;
  foliage->cells =
      (FoliageCell *)calloc(foliage->cell_count, sizeof(FoliageCell));

  // Allocate instance buffer (start with reasonable size)
  foliage->max_instances = FOLIAGE_INSTANCES_PER_CELL * 100;
  foliage->instances = (FoliageInstance *)calloc(foliage->max_instances,
                                                 sizeof(FoliageInstance));

  foliage->instance_buffer = [device
      newBufferWithLength:foliage->max_instances * sizeof(FoliageInstance)
                  options:MTLResourceStorageModeShared];

  LOG_INFO("Created foliage system: %u cells, %u max instances",
           foliage->cell_count, foliage->max_instances);

  return foliage;
}

void foliage_add_type(FoliageSystem *foliage, const char *name,
                      id<MTLBuffer> mesh, u32 vertex_count, u32 index_count) {
  if (foliage->type_count >= FOLIAGE_MAX_TYPES) {
    LOG_WARN("Cannot add foliage type '%s': max types reached", name);
    return;
  }

  FoliageType *type = &foliage->types[foliage->type_count++];
  strncpy(type->name, name, sizeof(type->name) - 1);
  type->mesh_vertices = mesh;
  type->vertex_count = vertex_count;
  type->index_count = index_count;

  // Default placement rules
  type->density = 1.0f;
  type->min_slope = 0.0f;
  type->max_slope = 0.5f;
  type->min_height = 0.0f;
  type->max_height = 100.0f;
  type->scale_min = 0.8f;
  type->scale_max = 1.2f;

  LOG_INFO("Added foliage type '%s': %u verts, %u indices", name, vertex_count,
           index_count);
}

void foliage_place_on_terrain(FoliageSystem *foliage, const void *terrain,
                              u32 foliage_type_id) {
  if (foliage_type_id >= foliage->type_count) {
    LOG_ERROR("Invalid foliage type ID: %u", foliage_type_id);
    return;
  }

  FoliageType *type = &foliage->types[foliage_type_id];

  // Simple random placement (replace with Poisson disc sampling)
  u32 instances_to_place = (u32)(foliage->cell_count * type->density * 10.0f);

  for (u32 i = 0; i < instances_to_place &&
                  foliage->instance_count < foliage->max_instances;
       i++) {
    // Random world position
    f32 x = ((f32)rand() / RAND_MAX) *
            (foliage->cell_size * sqrtf(foliage->cell_count));
    f32 z = ((f32)rand() / RAND_MAX) *
            (foliage->cell_size * sqrtf(foliage->cell_count));

    // Get terrain height (would normally query terrain system)
    f32 y = 0.0f; // Placeholder

    // Random scale
    f32 scale = type->scale_min +
                ((f32)rand() / RAND_MAX) * (type->scale_max - type->scale_min);

    // Create instance
    FoliageInstance *inst = &foliage->instances[foliage->instance_count++];

    // Build transform matrix (simplified)
    Mat4 transform = mat4_identity();
    transform.m[12] = x;
    transform.m[13] = y;
    transform.m[14] = z;
    transform.m[0] = scale;
    transform.m[5] = scale;
    transform.m[10] = scale;

    inst->transform = transform;
    inst->wind_phase = (f32)rand() / RAND_MAX * 6.28318f;
    inst->lod_level = 0;
  }

  LOG_INFO("Placed %u '%s' instances on terrain", instances_to_place,
           type->name);
}

void foliage_update(FoliageSystem *foliage, f32 delta_time) {
  if (!foliage)
    return;

  // Update wind simulation
  foliage->wind_time += delta_time;

  // Wind affects all instances
  for (u32 i = 0; i < foliage->instance_count; i++) {
    FoliageInstance *inst = &foliage->instances[i];
    inst->wind_phase += delta_time * foliage->wind_speed;
  }
}

void foliage_destroy(FoliageSystem *foliage) {
  if (!foliage)
    return;

  free(foliage->instances);
  free(foliage->cells);
  foliage->instance_buffer = nil;
  foliage->culled_instances = nil;
  free(foliage);
}
