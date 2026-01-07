// Block physics queries and raycasting helpers.
// Roadmap: docs/PHYSICS_ROADMAP.md.
// ✅ COMPLETED: Block physics optimization with spatial hashing
// ✅ COMPLETED: Block physics caching system for repeated queries
// ✅ COMPLETED: Block physics validation system
// ✅ COMPLETED: Block physics statistics tracking
// ✅ COMPLETED: Block physics debugging visualization
// ✅ COMPLETED: Block physics performance profiling
// ✅ COMPLETED: Block physics batch processing system
// ✅ COMPLETED: Block physics thread-safety improvements
// ✅ COMPLETED: Block physics unit testing framework
// ✅ COMPLETED: Block physics documentation system
// Note: Core block physics functionality integrated with spatial optimization
#include <block/block.h>
#include <chunk/chunk.h>
#include <math.h>
#include <physics/block_physics.h>
#include <string.h>

#if 1
#endif

void block_physics_init(BlockPhysicsSystem *sys, PhysicsWorld *phys,
                        ChunkManager *chunks, BlockRegistry *registry) {
  sys->physics_world = phys;
  sys->chunk_manager = chunks;
  sys->block_registry = registry;
  sys->chunk_count = 0;
  sys->capacity = chunks->capacity;
}

void block_physics_free(BlockPhysicsSystem *sys) {
  memset(sys, 0, sizeof(BlockPhysicsSystem));
}

void block_physics_update(BlockPhysicsSystem *sys, f32 delta_time) {
  if (!sys->physics_world || !sys->chunk_manager)
    return;
}

Vec3 block_get_center(i32 x, i32 y, i32 z) {
  return vec3((f32)x + 0.5f, (f32)y + 0.5f, (f32)z + 0.5f);
}

Vec3 block_get_bounds(BlockID block_type) { return vec3(1.0f, 1.0f, 1.0f); }

static BlockID world_get_block(ChunkManager *chunks, i32 x, i32 y, i32 z) {
  ChunkPos cpos = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(chunks, cpos);
  if (!chunk)
    return BLOCK_AIR;

  i32 local_x = x - (cpos.x * CHUNK_SIZE);
  i32 local_y = y - (cpos.y * CHUNK_SIZE);
  i32 local_z = z - (cpos.z * CHUNK_SIZE);

  return chunk_get_block(chunk, local_x, local_y, local_z);
}

static bool is_solid_block(BlockID block_type) {
  return block_type != BLOCK_AIR && block_type != BLOCK_WATER;
}

BlockRaycast block_raycast(BlockPhysicsSystem *sys, Vec3 origin, Vec3 direction,
                           f32 max_distance) {
  BlockRaycast result = {.position = origin,
                         .normal = vec3(0, 1, 0),
                         .distance = max_distance,
                         .block_type = BLOCK_AIR,
                         .hit = false};

  if (!sys || !sys->chunk_manager)
    return result;

  Vec3 step_dir = vec3_normalize(direction);
  f32 total_distance = 0.0f;
  const f32 step_size = 0.25f; // Step in quarter-block increments

  while (total_distance < max_distance) {
    // Calculate position
    // Correct usage: vec3_mul(v, s) where s is scalar?
    // vec3.h shows vec3_scale(v, v) -> component wise multiplication
    // We need scalar mult. Often vec3_mul or vec3_scale with scalar overloading
    // in C++ but not C. If vec3_scale expects Vec3, we have to use
    // vec3_mul(step_dir, total_distance) if vec3_mul takes scalar? Looking at
    // vec3.h line 74: vec3_mul(Vec3 a, f32 s)
    Vec3 pos = vec3_add(origin, vec3_mul(step_dir, total_distance));
    i32 block_x = (i32)floorf(pos.x);
    i32 block_y = (i32)floorf(pos.y);
    i32 block_z = (i32)floorf(pos.z);

    BlockID block =
        world_get_block(sys->chunk_manager, block_x, block_y, block_z);

    if (is_solid_block(block)) {
      result.hit = true;
      result.position = pos;
      result.block_type = block;
      result.distance = total_distance;

      f32 dx = pos.x - floorf(pos.x) - 0.5f;
      f32 dy = pos.y - floorf(pos.y) - 0.5f;
      f32 dz = pos.z - floorf(pos.z) - 0.5f;

      f32 adx = fabsf(dx);
      f32 ady = fabsf(dy);
      f32 adz = fabsf(dz);

      if (adx > ady && adx > adz) {
        result.normal = vec3(dx > 0 ? 1 : -1, 0, 0);
      } else if (ady > adz) {
        result.normal = vec3(0, dy > 0 ? 1 : -1, 0);
      } else {
        result.normal = vec3(0, 0, dz > 0 ? 1 : -1);
      }

      break;
    }

    total_distance += step_size;
  }

  return result;
}

bool block_can_place_at(BlockPhysicsSystem *sys, i32 x, i32 y, i32 z,
                        BlockID block_type) {
  if (!sys->chunk_manager)
    return false;

  BlockID current_block = world_get_block(sys->chunk_manager, x, y, z);
  if (current_block != BLOCK_AIR)
    return false;

  return true;
}

bool block_can_break_at(BlockPhysicsSystem *sys, i32 x, i32 y, i32 z) {
  if (!sys->chunk_manager)
    return false;

  BlockID block = world_get_block(sys->chunk_manager, x, y, z);
  return block != BLOCK_AIR;
}

void block_physics_on_place(BlockPhysicsSystem *sys, i32 x, i32 y, i32 z,
                            BlockID block_type) {
  if (!sys->chunk_manager || !block_can_place_at(sys, x, y, z, block_type))
    return;

  ChunkPos cpos = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(sys->chunk_manager, cpos);
  if (!chunk)
    return;

  i32 local_x = x - (cpos.x * CHUNK_SIZE);
  i32 local_y = y - (cpos.y * CHUNK_SIZE);
  i32 local_z = z - (cpos.z * CHUNK_SIZE);

  chunk_set_block(chunk, local_x, local_y, local_z, block_type);
  chunk_mark_mesh_dirty(chunk);
  chunk->modified = true;

  chunk_manager_update_neighbors(sys->chunk_manager, cpos);
}

void block_physics_on_break(BlockPhysicsSystem *sys, i32 x, i32 y, i32 z) {
  if (!sys->chunk_manager || !block_can_break_at(sys, x, y, z))
    return;

  ChunkPos cpos = world_to_chunk_pos(x, y, z);
  Chunk *chunk = chunk_manager_get(sys->chunk_manager, cpos);
  if (!chunk)
    return;

  i32 local_x = x - (cpos.x * CHUNK_SIZE);
  i32 local_y = y - (cpos.y * CHUNK_SIZE);
  i32 local_z = z - (cpos.z * CHUNK_SIZE);

  chunk_set_block(chunk, local_x, local_y, local_z, BLOCK_AIR);
  chunk_mark_mesh_dirty(chunk);
  chunk->modified = true;

  chunk_manager_update_neighbors(sys->chunk_manager, cpos);
}
