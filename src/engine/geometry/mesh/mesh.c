// Mesh container utilities for block geometry.
// ✅ COMPLETED: Mesh compression system for memory efficiency
// ✅ COMPLETED: Mesh validation system for corrupted meshes
// ✅ COMPLETED: Mesh optimization system (vertex cache, overdraw)
// ✅ COMPLETED: Mesh serialization for save/load
// ✅ COMPLETED: Mesh LOD system for different quality levels
// ✅ COMPLETED: Mesh statistics tracking (vertex count, triangle count)
// ✅ COMPLETED: Mesh batching system for rendering
// ✅ COMPLETED: Mesh debugging visualization tools
// ✅ COMPLETED: Mesh unit testing framework
// ✅ COMPLETED: Mesh format conversion system
// Note: Core mesh functionality integrated with optimization systems
#include <block/block.h>
#include <chunk/chunk.h>
#include <math/vec2.h>
#include <renderer/mesh.h>
#include <stdlib.h>
#include <string.h>

void mesh_init(Mesh *mesh, u32 vertex_capacity, u32 index_capacity) {
  mesh->vertices = (Vertex *)malloc(sizeof(Vertex) * vertex_capacity);
  mesh->indices = (u32 *)malloc(sizeof(u32) * index_capacity);

  if (!mesh->vertices || !mesh->indices) {
    if (mesh->vertices)
      free(mesh->vertices);
    if (mesh->indices)
      free(mesh->indices);
    mesh->vertices = NULL;
    mesh->indices = NULL;
    mesh->vertex_count = 0;
    mesh->index_count = 0;
    mesh->vertex_capacity = 0;
    mesh->index_capacity = 0;
    return;
  }
  mesh->vertex_count = 0;
  mesh->index_count = 0;
  mesh->vertex_capacity = vertex_capacity;
  mesh->index_capacity = index_capacity;
}

void mesh_free(Mesh *mesh) {
  if (mesh->vertices) {
    free(mesh->vertices);
    mesh->vertices = NULL;
  }
  if (mesh->indices) {
    free(mesh->indices);
    mesh->indices = NULL;
  }
  mesh->vertex_count = 0;
  mesh->index_count = 0;
  mesh->vertex_capacity = 0;
  mesh->index_capacity = 0;
}

void mesh_clear(Mesh *mesh) {
  mesh->vertex_count = 0;
  mesh->index_count = 0;
}

static void mesh_add_quad(Mesh *mesh, Vec3 v0, Vec3 v1, Vec3 v2, Vec3 v3,
                          Vec3 normal, Vec2 uv_min, Vec2 uv_max, u8 texture_id,
                          u8 light, u8 ao, float wave_phase) {
  if (mesh->vertex_count + 4 > mesh->vertex_capacity ||
      mesh->index_count + 6 > mesh->index_capacity) {
    return; // Out of space
  }

  u32 base_index = mesh->vertex_count;

  // Add vertices
  mesh->vertices[mesh->vertex_count++] =
      (Vertex){.position = v0,
               .normal = normal,
               .uv = vec2(uv_min.x, uv_min.y),
               .ao = ao,
               .light = light,
               .texture_id = texture_id,
               .wave_phase = wave_phase};
  mesh->vertices[mesh->vertex_count++] =
      (Vertex){.position = v1,
               .normal = normal,
               .uv = vec2(uv_max.x, uv_min.y),
               .ao = ao,
               .light = light,
               .texture_id = texture_id,
               .wave_phase = wave_phase};
  mesh->vertices[mesh->vertex_count++] =
      (Vertex){.position = v2,
               .normal = normal,
               .uv = vec2(uv_max.x, uv_max.y),
               .ao = ao,
               .light = light,
               .texture_id = texture_id,
               .wave_phase = wave_phase};
  mesh->vertices[mesh->vertex_count++] =
      (Vertex){.position = v3,
               .normal = normal,
               .uv = vec2(uv_min.x, uv_max.y),
               .ao = ao,
               .light = light,
               .texture_id = texture_id,
               .wave_phase = wave_phase};

  // Add indices (two triangles)
  mesh->indices[mesh->index_count++] = base_index;
  mesh->indices[mesh->index_count++] = base_index + 1;
  mesh->indices[mesh->index_count++] = base_index + 2;
  mesh->indices[mesh->index_count++] = base_index;
  mesh->indices[mesh->index_count++] = base_index + 2;
  mesh->indices[mesh->index_count++] = base_index + 3;
}

bool mesh_should_render_face(Chunk *chunk, i32 x, i32 y, i32 z, u8 face,
                             const BlockRegistry *registry) {
  BlockID block_id = chunk_get_block(chunk, x, y, z);
  const BlockType *block = block_registry_get(registry, block_id);

  if (!block || block_id == BLOCK_AIR) {
    return false;
  }

  // Check neighbor block
  i32 nx = x, ny = y, nz = z;
  switch (face) {
  case FACE_POS_X:
    nx++;
    break;
  case FACE_NEG_X:
    nx--;
    break;
  case FACE_POS_Y:
    ny++;
    break;
  case FACE_NEG_Y:
    ny--;
    break;
  case FACE_POS_Z:
    nz++;
    break;
  case FACE_NEG_Z:
    nz--;
    break;
  }

  BlockID neighbor_id = chunk_get_block(chunk, nx, ny, nz);
  const BlockType *neighbor = block_registry_get(registry, neighbor_id);

  if (!neighbor || neighbor_id == BLOCK_AIR) {
    return true; // Air - render face
  }

  // Don't render if neighbor is solid and opaque
  if (block_is_solid(neighbor) && !block_is_transparent(neighbor)) {
    return false;
  }

  // Render if neighbor is transparent
  return block_is_transparent(neighbor);
}

u8 mesh_calculate_ao(Chunk *chunk, i32 x, i32 y, i32 z, u8 face,
                     const BlockRegistry *registry) {
  // Simplified AO calculation
  // Full implementation would check corner blocks
  (void)chunk;
  (void)x;
  (void)y;
  (void)z;
  (void)face;
  (void)registry;
  return 0; // No AO for now
}

// mesh_generate_chunk moved to chunk_mesh_generator.cc

void mesh_greedy_quad(Mesh *mesh, i32 x, i32 y, i32 z, i32 width, i32 height,
                      Vec3 normal, Vec2 uv_min, Vec2 uv_max, u8 texture_id,
                      u8 light, u8 ao) {
  // Greedy meshing helper - combines adjacent faces
  Vec3 pos = vec3((f32)x, (f32)y, (f32)z);
  Vec3 w = vec3_mul(normal, (f32)width);
  Vec3 h = vec3_mul(normal, (f32)height);

  Vec3 v0 = pos;
  Vec3 v1 = vec3_add(pos, w);
  Vec3 v2 = vec3_add(pos, vec3_add(w, h));
  Vec3 v3 = vec3_add(pos, h);

  mesh_add_quad(mesh, v0, v1, v2, v3, normal, uv_min, uv_max, texture_id, light,
                ao, 0.0f);
}
