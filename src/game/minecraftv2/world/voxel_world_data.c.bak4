// src/world/voxel_world_data.c
//
// Purpose: Implements VoxelWorldData, a concrete implementation of IWorldData
// for traditional 3D voxel worlds (Minecraft-style).
//
#include <chunk/chunk.h>
#include <world/world_data.h>

#include <float.h>
#include <game_common.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Voxel world data implementation
typedef struct {
  struct ChunkManager *chunk_manager;
  bool initialized;
} VoxelWorldDataImpl;

// Forward declarations
static bool voxel_world_init(IWorldData *self, WorldDataType type,
                             struct ChunkManager *chunk_manager);
static void voxel_world_cleanup(IWorldData *self);
static BlockID voxel_world_get_block(IWorldData *self, i32 x, i32 y, i32 z);
static void voxel_world_set_block(IWorldData *self, i32 x, i32 y, i32 z,
                                  BlockID block_id);
static bool voxel_world_is_solid(IWorldData *self, i32 x, i32 y, i32 z);
static bool voxel_world_is_transparent(IWorldData *self, i32 x, i32 y, i32 z);
static i16 voxel_world_get_height(IWorldData *self, i32 x, i32 z);
static void voxel_world_set_height(IWorldData *self, i32 x, i32 z, i16 height);
static HeightmapCell voxel_world_get_heightmap_cell(IWorldData *self, i32 x,
                                                    i32 z);
static void voxel_world_set_heightmap_cell(IWorldData *self, i32 x, i32 z,
                                           HeightmapCell cell);
static u8 voxel_world_get_light(IWorldData *self, i32 x, i32 y, i32 z);
static void voxel_world_set_light(IWorldData *self, i32 x, i32 y, i32 z,
                                  u8 level);
static u8 voxel_world_get_metadata(IWorldData *self, i32 x, i32 y, i32 z);
static void voxel_world_set_metadata(IWorldData *self, i32 x, i32 y, i32 z,
                                     u8 metadata);
static Chunk *voxel_world_get_chunk(IWorldData *self, ChunkPos pos);
static Chunk *voxel_world_get_or_create_chunk(IWorldData *self, ChunkPos pos);
static void voxel_world_mark_chunk_dirty(IWorldData *self, ChunkPos pos);
static bool voxel_world_raycast(IWorldData *self, Vec3 start, Vec3 direction,
                                f32 max_distance, Vec3 *hit_pos,
                                Vec3 *hit_normal, BlockID *hit_block);
static i32 voxel_world_get_top_block_y(IWorldData *self, i32 x, i32 z);
static Vec3 voxel_world_world_to_chunk_pos(IWorldData *self, Vec3 world_pos);
static Vec3 voxel_world_chunk_to_world_pos(IWorldData *self,
                                           ChunkPos chunk_pos);

// Create voxel world data
IWorldData *voxel_world_data_create(struct ChunkManager *chunk_manager) {
  IWorldData *world = (IWorldData *)calloc(1, sizeof(IWorldData));
  if (!world)
    return NULL;

  VoxelWorldDataImpl *impl =
      (VoxelWorldDataImpl *)calloc(1, sizeof(VoxelWorldDataImpl));
  if (!impl) {
    free(world);
    return NULL;
  }

  impl->chunk_manager = chunk_manager;

  // Initialize function pointers
  world->type = WORLD_DATA_TYPE_VOXEL;
  world->init = voxel_world_init;
  world->cleanup = voxel_world_cleanup;
  world->get_block = voxel_world_get_block;
  world->set_block = voxel_world_set_block;
  world->is_solid = voxel_world_is_solid;
  world->is_transparent = voxel_world_is_transparent;
  world->get_height = voxel_world_get_height;
  world->set_height = voxel_world_set_height;
  world->get_heightmap_cell = voxel_world_get_heightmap_cell;
  world->set_heightmap_cell = voxel_world_set_heightmap_cell;
  world->get_light = voxel_world_get_light;
  world->set_light = voxel_world_set_light;
  world->get_metadata = voxel_world_get_metadata;
  world->set_metadata = voxel_world_set_metadata;
  world->get_chunk = voxel_world_get_chunk;
  world->get_or_create_chunk = voxel_world_get_or_create_chunk;
  world->mark_chunk_dirty = voxel_world_mark_chunk_dirty;
  world->raycast = voxel_world_raycast;
  world->get_top_block_y = voxel_world_get_top_block_y;
  world->world_to_chunk_pos = voxel_world_world_to_chunk_pos;
  world->chunk_to_world_pos = voxel_world_chunk_to_world_pos;
  world->impl_data = impl;
  world->chunk_manager = chunk_manager;

  return world;
}

// Implementation functions
static bool voxel_world_init(IWorldData *self, WorldDataType type,
                             struct ChunkManager *chunk_manager) {
  VoxelWorldDataImpl *impl = (VoxelWorldDataImpl *)self->impl_data;
  if (!impl)
    return false;

  impl->chunk_manager = chunk_manager;
  impl->initialized = true;
  return true;
}

static void voxel_world_cleanup(IWorldData *self) {
  VoxelWorldDataImpl *impl = (VoxelWorldDataImpl *)self->impl_data;
  if (impl) {
    free(impl);
  }
  self->impl_data = NULL;
}

static BlockID voxel_world_get_block(IWorldData *self, i32 x, i32 y, i32 z) {
  VoxelWorldDataImpl *impl = (VoxelWorldDataImpl *)self->impl_data;
  if (!impl || !impl->chunk_manager)
    return 0;

  ChunkPos chunk_pos = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(impl->chunk_manager, chunk_pos);
  if (!chunk)
    return 0;

  i32 wx, wy, wz;
  chunk_to_world_pos(chunk_pos, &wx, &wy, &wz);
  i32 local_x = x - wx;
  i32 local_y = y - wy;
  i32 local_z = z - wz;

  return chunk_get_block(chunk, local_x, local_y, local_z);
}

static void voxel_world_set_block(IWorldData *self, i32 x, i32 y, i32 z,
                                  BlockID block_id) {
  VoxelWorldDataImpl *impl = (VoxelWorldDataImpl *)self->impl_data;
  if (!impl || !impl->chunk_manager)
    return;

  ChunkPos chunk_pos = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get_or_create(impl->chunk_manager, chunk_pos);
  if (!chunk)
    return;

  i32 wx, wy, wz;
  chunk_to_world_pos(chunk_pos, &wx, &wy, &wz);
  i32 local_x = x - wx;
  i32 local_y = y - wy;
  i32 local_z = z - wz;

  chunk_set_block(chunk, local_x, local_y, local_z, block_id);
  chunk_mark_mesh_dirty(chunk);
}

static bool voxel_world_is_solid(IWorldData *self, i32 x, i32 y, i32 z) {
  BlockID block = voxel_world_get_block(self, x, y, z);
  // TODO: Check block registry for solid property
  return block != 0; // Placeholder
}

static bool voxel_world_is_transparent(IWorldData *self, i32 x, i32 y, i32 z) {
  BlockID block = voxel_world_get_block(self, x, y, z);
  // TODO: Check block registry for transparent property
  return block == 0; // Placeholder
}

static i16 voxel_world_get_height(IWorldData *self, i32 x, i32 z) {
  // For voxel world, find topmost solid block
  return (i16)voxel_world_get_top_block_y(self, x, z);
}

static void voxel_world_set_height(IWorldData *self, i32 x, i32 z, i16 height) {
  (void)self;
  (void)x;
  (void)z;
  (void)height;
  // Voxel world doesn't directly set height - use set_block instead
}

static HeightmapCell voxel_world_get_heightmap_cell(IWorldData *self, i32 x,
                                                    i32 z) {
  HeightmapCell cell = {0};
  i32 top_y = voxel_world_get_top_block_y(self, x, z);
  cell.height = (i16)top_y;
  if (top_y >= 0) {
    cell.surface_block = voxel_world_get_block(self, x, top_y, z);
  }
  cell.light_level = voxel_world_get_light(self, x, top_y, z);
  return cell;
}

static void voxel_world_set_heightmap_cell(IWorldData *self, i32 x, i32 z,
                                           HeightmapCell cell) {
  // For voxel world, set blocks to match heightmap
  i32 current_top = voxel_world_get_top_block_y(self, x, z);
  i32 target_y = (i32)cell.height;

  if (target_y > current_top) {
    // Fill up to target height
    for (i32 y = current_top + 1; y <= target_y; y++) {
      voxel_world_set_block(self, x, y, z, cell.surface_block);
    }
  } else if (target_y < current_top) {
    // Remove blocks above target
    for (i32 y = target_y + 1; y <= current_top; y++) {
      voxel_world_set_block(self, x, y, z, 0);
    }
  }

  if (target_y >= 0) {
    voxel_world_set_light(self, x, target_y, z, cell.light_level);
  }
}

static u8 voxel_world_get_light(IWorldData *self, i32 x, i32 y, i32 z) {
  VoxelWorldDataImpl *impl = (VoxelWorldDataImpl *)self->impl_data;
  if (!impl || !impl->chunk_manager)
    return 0;

  ChunkPos chunk_pos = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(impl->chunk_manager, chunk_pos);
  if (!chunk)
    return 0;

  i32 wx, wy, wz;
  chunk_to_world_pos(chunk_pos, &wx, &wy, &wz);
  i32 local_x = x - wx;
  i32 local_y = y - wy;
  i32 local_z = z - wz;

  return chunk_get_light(chunk, local_x, local_y, local_z, LIGHT_TYPE_SKY);
}

static void voxel_world_set_light(IWorldData *self, i32 x, i32 y, i32 z,
                                  u8 level) {
  VoxelWorldDataImpl *impl = (VoxelWorldDataImpl *)self->impl_data;
  if (!impl || !impl->chunk_manager)
    return;

  ChunkPos chunk_pos = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get_or_create(impl->chunk_manager, chunk_pos);
  if (!chunk)
    return;

  i32 wx, wy, wz;
  chunk_to_world_pos(chunk_pos, &wx, &wy, &wz);
  i32 local_x = x - wx;
  i32 local_y = y - wy;
  i32 local_z = z - wz;

  chunk_set_light(chunk, local_x, local_y, local_z, level, LIGHT_TYPE_SKY);
}

static u8 voxel_world_get_metadata(IWorldData *self, i32 x, i32 y, i32 z) {
  VoxelWorldDataImpl *impl = (VoxelWorldDataImpl *)self->impl_data;
  if (!impl || !impl->chunk_manager)
    return 0;

  ChunkPos chunk_pos = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(impl->chunk_manager, chunk_pos);
  if (!chunk)
    return 0;

  i32 wx, wy, wz;
  chunk_to_world_pos(chunk_pos, &wx, &wy, &wz);
  i32 local_x = x - wx;
  i32 local_y = y - wy;
  i32 local_z = z - wz;

  return chunk_get_metadata(chunk, local_x, local_y, local_z);
}

static void voxel_world_set_metadata(IWorldData *self, i32 x, i32 y, i32 z,
                                     u8 metadata) {
  VoxelWorldDataImpl *impl = (VoxelWorldDataImpl *)self->impl_data;
  if (!impl || !impl->chunk_manager)
    return;

  ChunkPos chunk_pos = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get_or_create(impl->chunk_manager, chunk_pos);
  if (!chunk)
    return;

  i32 wx, wy, wz;
  chunk_to_world_pos(chunk_pos, &wx, &wy, &wz);
  i32 local_x = x - wx;
  i32 local_y = y - wy;
  i32 local_z = z - wz;

  chunk_set_metadata(chunk, local_x, local_y, local_z, metadata);
}

static Chunk *voxel_world_get_chunk(IWorldData *self, ChunkPos pos) {
  VoxelWorldDataImpl *impl = (VoxelWorldDataImpl *)self->impl_data;
  if (!impl || !impl->chunk_manager)
    return NULL;
  return chunk_manager_get(impl->chunk_manager, pos);
}

static Chunk *voxel_world_get_or_create_chunk(IWorldData *self, ChunkPos pos) {
  VoxelWorldDataImpl *impl = (VoxelWorldDataImpl *)self->impl_data;
  if (!impl || !impl->chunk_manager)
    return NULL;
  return chunk_manager_get_or_create(impl->chunk_manager, pos);
}

static void voxel_world_mark_chunk_dirty(IWorldData *self, ChunkPos pos) {
  Chunk *chunk = voxel_world_get_chunk(self, pos);
  if (chunk) {
    chunk_mark_mesh_dirty(chunk);
  }
}

static bool voxel_world_raycast(IWorldData *self, Vec3 start, Vec3 direction,
                                f32 max_distance, Vec3 *hit_pos,
                                Vec3 *hit_normal, BlockID *hit_block) {
  if (!self || max_distance <= 0.0f)
    return false;

  if (vec3_length_sq(direction) < (EPSILON * EPSILON)) {
    return false;
  }

  Vec3 dir_norm = vec3_normalize(direction);

  f32 inv_x = fabsf(dir_norm.x) < EPSILON ? FLT_MAX : 1.0f / dir_norm.x;
  f32 inv_y = fabsf(dir_norm.y) < EPSILON ? FLT_MAX : 1.0f / dir_norm.y;
  f32 inv_z = fabsf(dir_norm.z) < EPSILON ? FLT_MAX : 1.0f / dir_norm.z;

  i32 x = (i32)floorf(start.x);
  i32 y = (i32)floorf(start.y);
  i32 z = (i32)floorf(start.z);

  i32 step_x = (dir_norm.x > 0.0f) ? 1 : (dir_norm.x < 0.0f ? -1 : 0);
  i32 step_y = (dir_norm.y > 0.0f) ? 1 : (dir_norm.y < 0.0f ? -1 : 0);
  i32 step_z = (dir_norm.z > 0.0f) ? 1 : (dir_norm.z < 0.0f ? -1 : 0);

  f32 t_delta_x = fabsf(inv_x);
  f32 t_delta_y = fabsf(inv_y);
  f32 t_delta_z = fabsf(inv_z);

  f32 t_max_x = FLT_MAX;
  f32 t_max_y = FLT_MAX;
  f32 t_max_z = FLT_MAX;

  if (step_x > 0) {
    t_max_x = ((x + 1.0f) - start.x) * inv_x;
  } else if (step_x < 0) {
    t_max_x = (x - start.x) * inv_x;
  }

  if (step_y > 0) {
    t_max_y = ((y + 1.0f) - start.y) * inv_y;
  } else if (step_y < 0) {
    t_max_y = (y - start.y) * inv_y;
  }

  if (step_z > 0) {
    t_max_z = ((z + 1.0f) - start.z) * inv_z;
  } else if (step_z < 0) {
    t_max_z = (z - start.z) * inv_z;
  }

  f32 traveled = 0.0f;
  i32 last_axis = -1;

  while (traveled <= max_distance) {
    BlockID block = voxel_world_get_block(self, x, y, z);
    if (block != 0) {
      if (hit_block) {
        *hit_block = block;
      }
      if (hit_pos) {
        *hit_pos = vec3_add(start, vec3_mul(dir_norm, traveled));
      }
      if (hit_normal) {
        if (last_axis == 0) {
          *hit_normal = vec3((f32)-step_x, 0.0f, 0.0f);
        } else if (last_axis == 1) {
          *hit_normal = vec3(0.0f, (f32)-step_y, 0.0f);
        } else if (last_axis == 2) {
          *hit_normal = vec3(0.0f, 0.0f, (f32)-step_z);
        } else {
          *hit_normal = vec3(0.0f, 0.0f, 0.0f);
        }
      }
      return true;
    }

    if (t_max_x < t_max_y) {
      if (t_max_x < t_max_z) {
        traveled = t_max_x;
        t_max_x += t_delta_x;
        x += step_x;
        last_axis = 0;
      } else {
        traveled = t_max_z;
        t_max_z += t_delta_z;
        z += step_z;
        last_axis = 2;
      }
    } else {
      if (t_max_y < t_max_z) {
        traveled = t_max_y;
        t_max_y += t_delta_y;
        y += step_y;
        last_axis = 1;
      } else {
        traveled = t_max_z;
        t_max_z += t_delta_z;
        z += step_z;
        last_axis = 2;
      }
    }
  }

  return false;
}

static i32 voxel_world_get_top_block_y(IWorldData *self, i32 x, i32 z) {
  // Search from top down for first solid block
  for (i32 y = 255; y >= 0; y--) {
    if (voxel_world_is_solid(self, x, y, z)) {
      return y;
    }
  }
  return -1;
}

static Vec3 voxel_world_world_to_chunk_pos(IWorldData *self, Vec3 world_pos) {
  (void)self;
  ChunkPos cp =
      world_to_chunk_pos((i32)world_pos.x, (i32)world_pos.y, (i32)world_pos.z);
  Vec3 result = {(f32)cp.x, (f32)cp.y, (f32)cp.z};
  return result;
}

static Vec3 voxel_world_chunk_to_world_pos(IWorldData *self,
                                           ChunkPos chunk_pos) {
  (void)self;
  i32 wx, wy, wz;
  chunk_to_world_pos(chunk_pos, &wx, &wy, &wz);
  Vec3 result = {(f32)wx, (f32)wy, (f32)wz};
  return result;
}

// Factory function implementation
IWorldData *world_data_create(WorldDataType type,
                              struct ChunkManager *chunk_manager) {
  switch (type) {
  case WORLD_DATA_TYPE_VOXEL:
    return voxel_world_data_create(chunk_manager);
  case WORLD_DATA_TYPE_HEIGHTMAP: {
    IWorldData *world = voxel_world_data_create(chunk_manager);
    if (world) {
      world->type = WORLD_DATA_TYPE_HEIGHTMAP;
    }
    return world;
  }
  case WORLD_DATA_TYPE_HYBRID: {
    IWorldData *world = voxel_world_data_create(chunk_manager);
    if (world) {
      world->type = WORLD_DATA_TYPE_HYBRID;
    }
    return world;
  }
  default:
    return NULL;
  }
}

void world_data_destroy(IWorldData *world_data) {
  if (!world_data)
    return;

  if (world_data->cleanup) {
    world_data->cleanup(world_data);
  }
  free(world_data);
}
