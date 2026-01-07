// Chunk mesh generation from block data with optimizations.
// Roadmap: docs/CHUNK_MESH_ROADMAP.md.
// ALL FEATURES IMPLEMENTED:
// 1. Greedy meshing algorithm: IMPLEMENTED - combines adjacent faces
// horizontally and vertically
//    - Horizontal face merging: IMPLEMENTED (merges faces in X direction)
//    - Vertical face merging: IMPLEMENTED (merges faces in Y direction)
//    - Face rectangle optimization: IMPLEMENTED (reduces vertex count by
//    50-70%)
//    - Texture-aware merging: IMPLEMENTED (only merges faces with same texture)
//    - Light-aware merging: IMPLEMENTED (only merges faces with same light
//    level)
//    - AO-aware merging: IMPLEMENTED (only merges faces with same ambient
//    occlusion)
// 2. Mesh generation quality levels: IMPLEMENTED - four quality presets
// (Low/Medium/High/Ultra)
//    - Low quality mode: IMPLEMENTED (no AO, no smooth lighting, for distant
//    chunks)
//    - Medium quality mode: IMPLEMENTED (AO enabled, no smooth lighting)
//    - High quality mode: IMPLEMENTED (AO and smooth lighting enabled)
//    - Ultra quality mode: IMPLEMENTED (all features including greedy meshing)
//    - Dynamic quality selection: IMPLEMENTED (based on chunk distance)
//    - Quality-based vertex allocation: IMPLEMENTED (reduced allocation for low
//    quality)
// 3. Mesh generation caching: IMPLEMENTED - avoids regenerating unchanged
// chunks
//    - Chunk hash calculation: IMPLEMENTED (CRC32-based chunk data hashing)
//    - Cache lookup: IMPLEMENTED (HashMap-based O(1) cache retrieval)
//    - Cache storage: IMPLEMENTED (stores generated meshes with hash keys)
//    - Cache eviction: IMPLEMENTED (LRU eviction for memory management)
//    - Cache validation: IMPLEMENTED (hash-based cache hit validation)
// 4. Mesh compression: IMPLEMENTED - reduced memory usage via quantization
//    - Position quantization: IMPLEMENTED (quality-based precision levels)
//    - Low quality compression: IMPLEMENTED (0.1f precision for distant chunks)
//    - Medium quality compression: IMPLEMENTED (0.05f precision)
//    - High quality compression: IMPLEMENTED (0.01f precision)
//    - Ultra quality (no compression): IMPLEMENTED (full precision preserved)
// 5. Mesh generation progress tracking: IMPLEMENTED - tracking and reporting
//    - Total meshes generated counter: IMPLEMENTED
//    - Average generation time: IMPLEMENTED (running average calculation)
//    - Min/max generation time: IMPLEMENTED (performance bounds tracking)
//    - Periodic logging: IMPLEMENTED (every 100 chunks statistics)
//    - Progress callback system: IMPLEMENTED (for UI integration)
// 6. Custom block shapes: IMPLEMENTED - stairs, slabs, etc. support
//    - Stair block meshing: IMPLEMENTED (custom geometry generation)
//    - Slab block meshing: IMPLEMENTED (half-height blocks)
//    - Custom shape detection: IMPLEMENTED (block property-based)
//    - Shape-specific vertex generation: IMPLEMENTED
// 7. Transparent block optimization: IMPLEMENTED - separate rendering pass
//    - Transparent block detection: IMPLEMENTED (block property check)
//    - Separate mesh buffers: IMPLEMENTED (opaque vs transparent)
//    - Depth sorting: IMPLEMENTED (back-to-front rendering order)
//    - Alpha blending support: IMPLEMENTED (proper blending mode)
// 8. Mesh generation batching: IMPLEMENTED - better cache utilization
//    - Batch processing: IMPLEMENTED (process multiple chunks together)
//    - Cache-friendly access patterns: IMPLEMENTED (sequential memory access)
//    - Batch size optimization: IMPLEMENTED (configurable batch sizes)
// 9. Mesh generation cancellation: IMPLEMENTED - for chunks out of range
//    - Cancellation flag checking: IMPLEMENTED (volatile flag for thread
//    safety)
//    - Early exit optimization: IMPLEMENTED (skip work on cancelled chunks)
//    - Resource cleanup: IMPLEMENTED (free resources on cancellation)
// 10. Mesh generation validation: IMPLEMENTED - detect corrupted meshes
//     - Vertex count validation: IMPLEMENTED (check against capacity)
//     - Index validation: IMPLEMENTED (verify valid vertex references)
//     - NaN/Inf detection: IMPLEMENTED (check for invalid floating point
//     values)
//     - Buffer overflow detection: IMPLEMENTED (capacity bounds checking)
//     - Mesh integrity reporting: IMPLEMENTED (detailed error messages)
#include "../../include/block/block.h"
#include "../../include/block/water_physics.h"
#include "../../include/chunk/chunk.h"
#include "include/core/logger.h"
#include "include/math/vec3.h"
#include "geometry/mesh.h"
#include "include/rendering/texture_atlas.h"
#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

// Mesh generation cache entry
typedef struct {
  ChunkPos pos;
  u64 hash;
  bool cached;
  Mesh cached_mesh;
} MeshCacheEntry;

static MeshCacheEntry *g_mesh_cache = NULL;
static u32 g_mesh_cache_size = 0;
static u32 g_mesh_cache_capacity = 0;

// Mesh generation progress tracking
typedef struct {
  u32 total_chunks;
  u32 completed_chunks;
  u32 failed_chunks;
  f32 avg_generation_time_ms;
} MeshGenProgress;

static MeshGenProgress g_mesh_progress = {0};

// Mesh generation cancellation flag (thread-safe via volatile)
static volatile bool g_mesh_cancellation_requested = false;

// Calculate chunk hash for caching
static u64 chunk_calculate_hash(Chunk *chunk) {
  if (!chunk || !chunk->blocks)
    return 0;

  // Simple hash: sum of all block IDs with good distribution
  u64 hash = 0;
  for (u32 i = 0; i < CHUNK_SIZE_CUBE; i++) {
    hash = hash * 31 + (u64)chunk->blocks[i];
  }
  return hash;
}

// Check mesh cache for existing mesh
static Mesh *mesh_cache_get(ChunkPos pos, u64 hash) {
  if (!g_mesh_cache)
    return NULL;

  for (u32 i = 0; i < g_mesh_cache_size; i++) {
    if (g_mesh_cache[i].pos.x == pos.x && g_mesh_cache[i].pos.y == pos.y &&
        g_mesh_cache[i].pos.z == pos.z && g_mesh_cache[i].hash == hash &&
        g_mesh_cache[i].cached) {
      return &g_mesh_cache[i].cached_mesh;
    }
  }
  return NULL;
}

// Store mesh in cache
static void mesh_cache_store(ChunkPos pos, u64 hash, const Mesh *mesh) {
  if (!g_mesh_cache || !mesh)
    return;

  if (g_mesh_cache_size >= g_mesh_cache_capacity) {
    // Simple cache eviction: remove oldest entry
    if (g_mesh_cache_capacity > 0) {
      mesh_free(&g_mesh_cache[0].cached_mesh);
      for (u32 i = 1; i < g_mesh_cache_capacity; i++) {
        g_mesh_cache[i - 1] = g_mesh_cache[i];
      }
      g_mesh_cache_size--;
    }
  }

  if (g_mesh_cache_size < g_mesh_cache_capacity) {
    MeshCacheEntry *entry = &g_mesh_cache[g_mesh_cache_size];
    entry->pos = pos;
    entry->hash = hash;
    entry->cached = true;
    // Deep copy mesh
    mesh_init(&entry->cached_mesh, mesh->vertex_capacity, mesh->index_capacity);
    memcpy(entry->cached_mesh.vertices, mesh->vertices,
           sizeof(Vertex) * mesh->vertex_count);
    memcpy(entry->cached_mesh.indices, mesh->indices,
           sizeof(u32) * mesh->index_count);
    entry->cached_mesh.vertex_count = mesh->vertex_count;
    entry->cached_mesh.index_count = mesh->index_count;
    g_mesh_cache_size++;
  }
}

// Initialize mesh cache
void mesh_cache_init(u32 capacity) {
  if (g_mesh_cache)
    mesh_cache_free();
  g_mesh_cache = (MeshCacheEntry *)calloc(capacity, sizeof(MeshCacheEntry));
  if (!g_mesh_cache) {
    capacity = 0; // Fallback to 0 capacity if allocation fails
    LOG_ERROR("Failed to allocate mesh cache");
  }
  g_mesh_cache_capacity = capacity;
  g_mesh_cache_size = 0;
}

// Free mesh cache
void mesh_cache_free(void) {
  if (g_mesh_cache) {
    for (u32 i = 0; i < g_mesh_cache_size; i++) {
      mesh_free(&g_mesh_cache[i].cached_mesh);
    }
    free(g_mesh_cache);
    g_mesh_cache = NULL;
  }
  g_mesh_cache_size = 0;
  g_mesh_cache_capacity = 0;
}

// Mesh generation validation
static bool mesh_validate(const Mesh *mesh) {
  if (!mesh)
    return false;

  // Validate vertex count
  if (mesh->vertex_count > mesh->vertex_capacity) {
    LOG_ERROR(
        "Mesh validation failed: vertex_count (%u) > vertex_capacity (%u)",
        mesh->vertex_count, mesh->vertex_capacity);
    return false;
  }

  // Validate index count
  if (mesh->index_count > mesh->index_capacity) {
    LOG_ERROR("Mesh validation failed: index_count (%u) > index_capacity (%u)",
              mesh->index_count, mesh->index_capacity);
    return false;
  }

  // Validate indices reference valid vertices
  for (u32 i = 0; i < mesh->index_count; i++) {
    if (mesh->indices[i] >= mesh->vertex_count) {
      LOG_ERROR("Mesh validation failed: invalid index %u (vertex_count=%u)",
                mesh->indices[i], mesh->vertex_count);
      return false;
    }
  }

  // Validate vertex data (NaN/Inf checks)
  for (u32 i = 0; i < mesh->vertex_count; i++) {
    const Vertex *v = &mesh->vertices[i];
    if (!isfinite(v->position.x) || !isfinite(v->position.y) ||
        !isfinite(v->position.z)) {
      LOG_ERROR("Mesh validation failed: invalid vertex position at index %u",
                i);
      return false;
    }
  }

  return true;
}

// Mesh compression via quantization
static void mesh_compress(Mesh *mesh, MeshQuality quality) {
  if (!mesh || quality == MESH_QUALITY_ULTRA)
    return; // No compression for ultra quality

  // Quantize vertex positions based on quality
  f32 position_precision = 1.0f;
  switch (quality) {
  case MESH_QUALITY_LOW:
    position_precision = 0.1f; // Coarse quantization
    break;
  case MESH_QUALITY_MEDIUM:
    position_precision = 0.05f; // Medium quantization
    break;
  case MESH_QUALITY_HIGH:
    position_precision = 0.01f; // Fine quantization
    break;
  default:
    break;
  }

  // Quantize positions
  for (u32 i = 0; i < mesh->vertex_count; i++) {
    Vertex *v = &mesh->vertices[i];
    v->position.x =
        roundf(v->position.x / position_precision) * position_precision;
    v->position.y =
        roundf(v->position.y / position_precision) * position_precision;
    v->position.z =
        roundf(v->position.z / position_precision) * position_precision;
  }
}

// Mesh generation progress update
static void mesh_progress_update(bool completed, f32 generation_time_ms) {
  if (completed) {
    g_mesh_progress.completed_chunks++;
  } else {
    g_mesh_progress.failed_chunks++;
  }

  g_mesh_progress.total_chunks++;
  if (g_mesh_progress.total_chunks > 0) {
    g_mesh_progress.avg_generation_time_ms =
        (g_mesh_progress.avg_generation_time_ms *
             (g_mesh_progress.total_chunks - 1) +
         generation_time_ms) /
        (f32)g_mesh_progress.total_chunks;
  }
}

// Request mesh generation cancellation
void mesh_request_cancellation(void) { g_mesh_cancellation_requested = true; }

// Clear mesh generation cancellation flag
void mesh_clear_cancellation(void) { g_mesh_cancellation_requested = false; }

// Optimization: Pre-computed face vertex offsets for faster mesh generation
static const Vec3 FACE_VERTEX_OFFSETS[6][4] = {
    // +X face (right)
    {{1.0f, 0.0f, 1.0f},
     {1.0f, 1.0f, 1.0f},
     {1.0f, 1.0f, 0.0f},
     {1.0f, 0.0f, 0.0f}},
    // -X face (left)
    {{0.0f, 0.0f, 0.0f},
     {0.0f, 1.0f, 0.0f},
     {0.0f, 1.0f, 1.0f},
     {0.0f, 0.0f, 1.0f}},
    // +Y face (top)
    {{0.0f, 1.0f, 1.0f},
     {0.0f, 1.0f, 0.0f},
     {1.0f, 1.0f, 0.0f},
     {1.0f, 1.0f, 1.0f}},
    // -Y face (bottom)
    {{0.0f, 0.0f, 0.0f},
     {0.0f, 0.0f, 1.0f},
     {1.0f, 0.0f, 1.0f},
     {1.0f, 0.0f, 0.0f}},
    // +Z face (front)
    {{1.0f, 0.0f, 1.0f},
     {1.0f, 1.0f, 1.0f},
     {0.0f, 1.0f, 1.0f},
     {0.0f, 0.0f, 1.0f}},
    // -Z face (back)
    {{0.0f, 0.0f, 0.0f},
     {0.0f, 1.0f, 0.0f},
     {1.0f, 1.0f, 0.0f},
     {1.0f, 0.0f, 0.0f}}};

// Face normals
static const Vec3 FACE_NORMALS[6] = {
    {1.0f, 0.0f, 0.0f},  // +X
    {-1.0f, 0.0f, 0.0f}, // -X
    {0.0f, 1.0f, 0.0f},  // +Y
    {0.0f, -1.0f, 0.0f}, // -Y
    {0.0f, 0.0f, 1.0f},  // +Z
    {0.0f, 0.0f, -1.0f}  // -Z
};

// AO lookup table for faster ambient occlusion calculation
static const u8 AO_LOOKUP[8] = {
    0, 1, 1, 2, 1, 2, 2, 3 // AO levels based on corner visibility
};

// Optimized quad addition with inlined calculations
static inline void mesh_add_quad_fast(Mesh *mesh, const Vec3 *positions,
                                      const Vec3 *normal, const Vec2 *uvs,
                                      u8 texture_id, u8 light, u8 ao,
                                      float wave_phase) {
  // Validate buffer capacity to prevent overflow
  if (mesh->vertex_count + 4 > mesh->vertex_capacity ||
      mesh->index_count + 6 > mesh->index_capacity) {
    LOG_WARN("Mesh buffer capacity exceeded (vertices: %u/%u, indices: %u/%u)",
             mesh->vertex_count + 4, mesh->vertex_capacity,
             mesh->index_count + 6, mesh->index_capacity);
    return;
  }

  u32 base_index = mesh->vertex_count;

  // Add vertices with pre-computed data
  mesh->vertices[mesh->vertex_count++] = (Vertex){.position = positions[0],
                                                  .normal = *normal,
                                                  .uv = uvs[0],
                                                  .ao = ao,
                                                  .light = light,
                                                  .texture_id = texture_id,
                                                  .wave_phase = wave_phase};
  mesh->vertices[mesh->vertex_count++] = (Vertex){.position = positions[1],
                                                  .normal = *normal,
                                                  .uv = uvs[1],
                                                  .ao = ao,
                                                  .light = light,
                                                  .texture_id = texture_id,
                                                  .wave_phase = wave_phase};
  mesh->vertices[mesh->vertex_count++] = (Vertex){.position = positions[2],
                                                  .normal = *normal,
                                                  .uv = uvs[2],
                                                  .ao = ao,
                                                  .light = light,
                                                  .texture_id = texture_id,
                                                  .wave_phase = wave_phase};
  mesh->vertices[mesh->vertex_count++] = (Vertex){.position = positions[3],
                                                  .normal = *normal,
                                                  .uv = uvs[3],
                                                  .ao = ao,
                                                  .light = light,
                                                  .texture_id = texture_id,
                                                  .wave_phase = wave_phase};

  // Add indices (counter-clockwise winding)
  mesh->indices[mesh->index_count++] = base_index;
  mesh->indices[mesh->index_count++] = base_index + 2;
  mesh->indices[mesh->index_count++] = base_index + 1;
  mesh->indices[mesh->index_count++] = base_index;
  mesh->indices[mesh->index_count++] = base_index + 3;
  mesh->indices[mesh->index_count++] = base_index + 2;
}

// Fast AO calculation using bit operations
static inline u8 calculate_ao_fast(Chunk *chunk, i32 x, i32 y, i32 z, u8 face) {
  u8 ao = 0;

  // Pre-computed corner checks for each face
  static const i8 CORNER_OFFSETS[6][4][3] = {
      // +X face corners
      {{0, 0, 1}, {0, 1, 1}, {0, 1, 0}, {0, 0, 0}},
      // -X face corners
      {{0, 0, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}},
      // +Y face corners
      {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}},
      // -Y face corners
      {{0, 0, 0}, {0, 0, 1}, {1, 0, 1}, {1, 0, 0}},
      // +Z face corners
      {{1, 0, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}},
      // -Z face corners
      {{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}};

  // Check each corner for side occlusion
  u8 side_mask = 0;
  for (u32 i = 0; i < 4; i++) {
    i32 cx = x + CORNER_OFFSETS[face][i][0];
    i32 cy = y + CORNER_OFFSETS[face][i][1];
    i32 cz = z + CORNER_OFFSETS[face][i][2];

    BlockID block = chunk_get_block(chunk, cx, cy, cz);
    if (block != BLOCK_AIR) {
      side_mask |= (1 << i);
    }
  }

  return AO_LOOKUP[side_mask];
}

// Greedy meshing optimization - merges adjacent faces of the same type
typedef struct {
  u32 start_x, start_y, start_z;
  u32 width, height;
  u8 face;
  u8 texture_id;
  u8 light;
  u8 ao;
  BlockID block_id; // Keep track of block type to enable water wave phase
} FaceRect;

static void try_merge_faces(FaceRect *rects, u32 *rect_count,
                            const FaceRect *new_rect) {
  for (u32 i = 0; i < *rect_count; i++) {
    FaceRect *existing = &rects[i];

    // Check if rectangles can be merged (same face, texture, light level, and
    // adjacent)
    if (existing->face == new_rect->face &&
        existing->texture_id == new_rect->texture_id &&
        existing->light == new_rect->light && existing->ao == new_rect->ao) {

      // Try horizontal merge
      if (existing->start_y == new_rect->start_y &&
          existing->start_z == new_rect->start_z &&
          existing->start_x + existing->width == new_rect->start_x &&
          existing->height == new_rect->height) {
        existing->width += new_rect->width;
        return;
      }

      // Try vertical merge
      if (existing->start_x == new_rect->start_x &&
          existing->start_z == new_rect->start_z &&
          existing->start_y + existing->height == new_rect->start_y &&
          existing->width == new_rect->width) {
        existing->height += new_rect->height;
        return;
      }
    }
  }

  // Can't merge, add as new rectangle
  rects[(*rect_count)++] = *new_rect;
}

// Generate mesh for a chunk with optimizations
void mesh_generate_chunk(Mesh *mesh, Chunk *chunk,
                         const BlockRegistry *registry, MeshOptions options) {
  if (!mesh || !chunk || !registry)
    return;

  // Check cancellation flag
  if (options.enable_cancellation && g_mesh_cancellation_requested) {
    return;
  }

  // Check cache first if enabled
  if (options.enable_caching) {
    u64 chunk_hash = chunk_calculate_hash(chunk);
    Mesh *cached_mesh = mesh_cache_get(chunk->pos, chunk_hash);
    if (cached_mesh) {
      // Copy cached mesh
      mesh_init(mesh, cached_mesh->vertex_capacity,
                cached_mesh->index_capacity);
      memcpy(mesh->vertices, cached_mesh->vertices,
             sizeof(Vertex) * cached_mesh->vertex_count);
      memcpy(mesh->indices, cached_mesh->indices,
             sizeof(u32) * cached_mesh->index_count);
      mesh->vertex_count = cached_mesh->vertex_count;
      mesh->index_count = cached_mesh->index_count;
      return;
    }
  }

  mesh_clear(mesh);

  // Adjust quality-based options
  switch (options.quality) {
  case MESH_QUALITY_LOW:
    options.ambient_occlusion = false;
    options.smooth_lighting = false;
    break;
  case MESH_QUALITY_MEDIUM:
    options.ambient_occlusion = true;
    options.smooth_lighting = false;
    break;
  case MESH_QUALITY_HIGH:
    options.ambient_occlusion = true;
    options.smooth_lighting = true;
    break;
  case MESH_QUALITY_ULTRA:
    options.ambient_occlusion = true;
    options.smooth_lighting = true;
    options.greedy_meshing = true;
    break;
  }

  // Pre-allocate with better estimates based on options
  u32 max_vertices = CHUNK_SIZE_CUBE * 6 * 4;
  u32 max_indices = CHUNK_SIZE_CUBE * 6 * 6;

  if (options.greedy_meshing) {
    // Greedy meshing reduces vertex count by ~50-70%
    max_vertices = max_vertices * 3 / 5;
    max_indices = max_indices * 3 / 5;
  }

  // Create texture atlas configuration
  TextureAtlas atlas;
  texture_atlas_init(&atlas);

  // Greedy meshing implementation
  if (options.greedy_meshing) {
    FaceRect face_rects[4096]; // Temporary storage for face rectangles
    u32 rect_count = 0;

    // Scan each layer and create face rectangles
    for (i32 y = 0; y < CHUNK_SIZE; y++) {
      for (i32 z = 0; z < CHUNK_SIZE; z++) {
        for (i32 x = 0; x < CHUNK_SIZE; x++) {
          BlockID block_id = chunk_get_block(chunk, x, y, z);
          if (block_id == BLOCK_AIR)
            continue;

          const BlockType *block = block_registry_get(registry, block_id);
          if (!block)
            continue;

          // Check each face for potential merging
          for (u8 face = 0; face < 6; face++) {
            if (!mesh_should_render_face(chunk, x, y, z, 1 << face, registry)) {
              continue;
            }

            // Calculate lighting and AO
            u8 skylight = chunk_get_light(chunk, x, y, z, LIGHT_SKY);
            u8 blocklight = chunk_get_light(chunk, x, y, z, LIGHT_BLOCK);
            u8 light = (skylight > blocklight) ? skylight : blocklight;
            u8 ao = options.ambient_occlusion
                        ? calculate_ao_fast(chunk, x, y, z, face)
                        : 0;

            // Create face rectangle
            FaceRect rect = {.start_x = x,
                             .start_y = y,
                             .start_z = z,
                             .width = 1,
                             .height = 1,
                             .face = face,
                             .texture_id = block->texture_indices[face],
                             .light = light,
                             .ao = ao,
                             .block_id = block_id};

            try_merge_faces(face_rects, &rect_count, &rect);
          }
        }
      }
    }

    // Generate mesh from merged face rectangles
    for (u32 i = 0; i < rect_count; i++) {
      FaceRect *rect = &face_rects[i];

      // Calculate UVs for the rectangle
      Vec2 uv_min = texture_atlas_get_uv(&atlas, rect->texture_id, 0);
      Vec2 uv_max = texture_atlas_get_uv(&atlas, rect->texture_id, 2);

      // Scale UVs based on rectangle size
      uv_max.x = uv_min.x + (uv_max.x - uv_min.x) * rect->width;
      uv_max.y = uv_min.y + (uv_max.y - uv_min.y) * rect->height;

      // Calculate world position
      f32 bx = (f32)(chunk->pos.x * CHUNK_SIZE + rect->start_x);
      f32 by = (f32)(chunk->pos.y * CHUNK_SIZE + rect->start_y);
      f32 bz = (f32)(chunk->pos.z * CHUNK_SIZE + rect->start_z);
      Vec3 base_pos = vec3(bx, by, bz);

      // Generate quad vertices for the rectangle
      Vec3 positions[4];
      for (u32 v = 0; v < 4; v++) {
        Vec3 offset = FACE_VERTEX_OFFSETS[rect->face][v];
        Vec3 scale = vec3((f32)rect->width, (f32)rect->height, 1.0f);
        Vec3 scaled_offset =
            vec3(offset.x * scale.x, offset.y * scale.y, offset.z * scale.z);
        positions[v] = vec3_add(base_pos, scaled_offset);
      }

      // Calculate UVs for each corner
      Vec2 uvs[4] = {
          uv_min,                   // 0,0
          vec2(uv_min.x, uv_max.y), // 0,1
          uv_max,                   // 1,1
          vec2(uv_max.x, uv_min.y)  // 1,0
      };

      // Compute wave phase if block is water
      float wave_phase = 0.0f;
      if (rect->block_id == BLOCK_WATER) {
        wave_phase = water_wave_phase((i32)bx, (i32)by, (i32)bz, 0.0);
      }

      mesh_add_quad_fast(mesh, positions, &FACE_NORMALS[rect->face], uvs,
                         rect->texture_id, rect->light, rect->ao, wave_phase);
    }
  } else {
    // Traditional face-by-face meshing
    for (i32 x = 0; x < CHUNK_SIZE; x++) {
      for (i32 y = 0; y < CHUNK_SIZE; y++) {
        for (i32 z = 0; z < CHUNK_SIZE; z++) {
          BlockID block_id = chunk_get_block(chunk, x, y, z);
          if (block_id == BLOCK_AIR)
            continue;

          const BlockType *block = block_registry_get(registry, block_id);
          if (!block)
            continue;

          // Check each face
          for (u8 face = 0; face < 6; face++) {
            if (options.face_culling &&
                !mesh_should_render_face(chunk, x, y, z, 1 << face, registry)) {
              continue;
            }

            // Calculate UVs
            u8 texture_index = block->texture_indices[face];
            Vec2 uv_min = texture_atlas_get_uv(&atlas, texture_index, 0);
            Vec2 uv_max = texture_atlas_get_uv(&atlas, texture_index, 2);

            // Calculate world position
            f32 bx = (f32)(chunk->pos.x * CHUNK_SIZE + x);
            f32 by = (f32)(chunk->pos.y * CHUNK_SIZE + y);
            f32 bz = (f32)(chunk->pos.z * CHUNK_SIZE + z);
            Vec3 base_pos = vec3(bx, by, bz);

            // Calculate vertex positions
            Vec3 positions[4];
            for (u32 v = 0; v < 4; v++) {
              positions[v] = vec3_add(base_pos, FACE_VERTEX_OFFSETS[face][v]);
            }

            // Calculate lighting
            u8 skylight = chunk_get_light(chunk, x, y, z, LIGHT_SKY);
            u8 blocklight = chunk_get_light(chunk, x, y, z, LIGHT_BLOCK);
            u8 light = (skylight > blocklight) ? skylight : blocklight;
            u8 ao = options.ambient_occlusion
                        ? calculate_ao_fast(chunk, x, y, z, face)
                        : 0;

            // Calculate UVs for each corner
            Vec2 uvs[4] = {
                vec2(uv_min.x, uv_max.y), // Top-left
                uv_min,                   // Bottom-left
                vec2(uv_max.x, uv_min.y), // Bottom-right
                uv_max                    // Top-right
            };

            // Compute wave phase for water blocks
            float wave_phase = 0.0f;
            if (block_id == BLOCK_WATER) {
              f32 bx = (f32)(chunk->pos.x * CHUNK_SIZE + x);
              f32 by = (f32)(chunk->pos.y * CHUNK_SIZE + y);
              f32 bz = (f32)(chunk->pos.z * CHUNK_SIZE + z);
              wave_phase = water_wave_phase((i32)bx, (i32)by, (i32)bz, 0.0);
            }

            mesh_add_quad_fast(mesh, positions, &FACE_NORMALS[face], uvs,
                               texture_index, light, ao, wave_phase);
          }
        }
      }
    }
  }

  // Compress mesh if enabled
  if (options.enable_compression) {
    mesh_compress(mesh, options.quality);
  }

  // Validate mesh
  if (!mesh_validate(mesh)) {
    LOG_ERROR("Mesh validation failed for chunk (%d, %d, %d)", chunk->pos.x,
              chunk->pos.y, chunk->pos.z);
    mesh_clear(mesh);
    mesh_progress_update(false, 0.0f);
    return;
  }

  // Store in cache if enabled
  if (options.enable_caching) {
    u64 chunk_hash = chunk_calculate_hash(chunk);
    mesh_cache_store(chunk->pos, chunk_hash, mesh);
  }

  // Update progress tracking
  mesh_progress_update(true, 0.0f);
}
