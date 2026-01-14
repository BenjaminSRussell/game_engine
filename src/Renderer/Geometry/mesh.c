#include "geometry/mesh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u32 g_next_mesh_id = 1;

mesh_t *mesh_create(const char *name) {
  mesh_t *mesh = (mesh_t *)calloc(1, sizeof(mesh_t));
  if (!mesh) {
    return NULL;
  }

  if (name) {
    strncpy(mesh->name, name, 63);
    mesh->name[63] = '\0';
  }

  mesh->id = g_next_mesh_id++;
  mesh->vertex_buffer_handle = 0;
  mesh->index_buffer_handle = 0;
  mesh->ref_count = 1;
  mesh->last_accessed_frame = 0;

  return mesh;
}

void mesh_destroy(mesh_t *mesh) {
  if (!mesh) {
    return;
  }

  if (mesh->ref_count > 0) {
    mesh->ref_count--;
    if (mesh->ref_count > 0) {
      return; // Still referenced elsewhere
    }
  }

  free(mesh->vertices);
  free(mesh->indices);
  free(mesh->submeshes);
  free(mesh);
}

void mesh_allocate_buffers(mesh_t *mesh, u32 vertex_count, u32 index_count) {
  if (!mesh) {
    return;
  }

  // Free existing buffers if any
  if (mesh->vertices) {
    free(mesh->vertices);
  }
  if (mesh->indices) {
    free(mesh->indices);
  }

  // Allocate new buffers
  mesh->vertices = (geometry_vertex_t *)calloc(
      vertex_count, sizeof(geometry_vertex_t));
  mesh->indices = (u32 *)calloc(index_count, sizeof(u32));

  if (mesh->vertices) {
    mesh->vertex_capacity = vertex_count;
    mesh->vertex_count = 0;
  } else {
    mesh->vertex_capacity = 0;
    mesh->vertex_count = 0;
  }

  if (mesh->indices) {
    mesh->index_capacity = index_count;
    mesh->index_count = 0;
  } else {
    mesh->index_capacity = 0;
    mesh->index_count = 0;
  }
}

void mesh_set_vertices(mesh_t *mesh, const geometry_vertex_t *vertices,
                       u32 count, u32 offset) {
  if (!mesh || !vertices || count == 0) {
    return;
  }

  if (offset + count > mesh->vertex_capacity) {
    printf("Warning: Vertex data exceeds capacity\n");
    return;
  }

  memcpy(&mesh->vertices[offset], vertices,
         count * sizeof(geometry_vertex_t));

  // Update vertex count if this extends beyond current count
  if (offset + count > mesh->vertex_count) {
    mesh->vertex_count = offset + count;
  }
}

void mesh_set_indices(mesh_t *mesh, const u32 *indices, u32 count, u32 offset) {
  if (!mesh || !indices || count == 0) {
    return;
  }

  if (offset + count > mesh->index_capacity) {
    printf("Warning: Index data exceeds capacity\n");
    return;
  }

  memcpy(&mesh->indices[offset], indices, count * sizeof(u32));

  // Update index count if this extends beyond current count
  if (offset + count > mesh->index_count) {
    mesh->index_count = offset + count;
  }
}

void mesh_add_submesh(mesh_t *mesh, submesh_t submesh) {
  if (!mesh) {
    return;
  }

  submesh_t *new_submeshes = (submesh_t *)realloc(
      mesh->submeshes, (mesh->submesh_count + 1) * sizeof(submesh_t));
  if (!new_submeshes) {
    printf("Warning: Failed to allocate memory for submesh\n");
    return;
  }

  mesh->submeshes = new_submeshes;
  mesh->submeshes[mesh->submesh_count] = submesh;
  mesh->submesh_count++;
}

void mesh_clear_submeshes(mesh_t *mesh) {
  if (!mesh) {
    return;
  }

  free(mesh->submeshes);
  mesh->submeshes = NULL;
  mesh->submesh_count = 0;
}

void mesh_set_material(mesh_t *mesh, u32 slot, u32 material_id) {
  if (!mesh || slot >= MESH_MAX_MATERIALS) {
    return;
  }

  mesh->material_ids[slot] = material_id;

  // Update material count if needed
  if (slot >= mesh->material_count) {
    mesh->material_count = slot + 1;
  }
}

void mesh_calculate_bounds(mesh_t *mesh) {
  if (!mesh || mesh->vertex_count == 0) {
    return;
  }

  // Initialize bounds to first vertex
  mesh->bounds.min = mesh->vertices[0].position;
  mesh->bounds.max = mesh->vertices[0].position;

  // Find AABB min/max
  for (u32 i = 1; i < mesh->vertex_count; i++) {
    Vec3 pos = mesh->vertices[i].position;

    if (pos.x < mesh->bounds.min.x)
      mesh->bounds.min.x = pos.x;
    if (pos.y < mesh->bounds.min.y)
      mesh->bounds.min.y = pos.y;
    if (pos.z < mesh->bounds.min.z)
      mesh->bounds.min.z = pos.z;

    if (pos.x > mesh->bounds.max.x)
      mesh->bounds.max.x = pos.x;
    if (pos.y > mesh->bounds.max.y)
      mesh->bounds.max.y = pos.y;
    if (pos.z > mesh->bounds.max.z)
      mesh->bounds.max.z = pos.z;
  }

  // Calculate sphere center (center of AABB)
  mesh->bounds.sphere_center.x =
      (mesh->bounds.min.x + mesh->bounds.max.x) * 0.5f;
  mesh->bounds.sphere_center.y =
      (mesh->bounds.min.y + mesh->bounds.max.y) * 0.5f;
  mesh->bounds.sphere_center.z =
      (mesh->bounds.min.z + mesh->bounds.max.z) * 0.5f;

  // Calculate sphere radius (distance to farthest corner)
  Vec3 to_corner = {mesh->bounds.max.x - mesh->bounds.sphere_center.x,
                    mesh->bounds.max.y - mesh->bounds.sphere_center.y,
                    mesh->bounds.max.z - mesh->bounds.sphere_center.z};

  mesh->bounds.sphere_radius =
      sqrtf(to_corner.x * to_corner.x + to_corner.y * to_corner.y +
            to_corner.z * to_corner.z);
}

void mesh_calculate_normals(mesh_t *mesh) {
  if (!mesh || mesh->vertex_count == 0 || mesh->index_count < 3) {
    return;
  }

  // Clear existing normals
  for (u32 i = 0; i < mesh->vertex_count; i++) {
    mesh->vertices[i].normal = (Vec3){0, 0, 0};
  }

  // Calculate face normals and accumulate
  for (u32 i = 0; i < mesh->index_count; i += 3) {
    if (i + 2 >= mesh->index_count)
      break;

    u32 i0 = mesh->indices[i];
    u32 i1 = mesh->indices[i + 1];
    u32 i2 = mesh->indices[i + 2];

    if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count ||
        i2 >= mesh->vertex_count) {
      continue;
    }

    Vec3 v0 = mesh->vertices[i0].position;
    Vec3 v1 = mesh->vertices[i1].position;
    Vec3 v2 = mesh->vertices[i2].position;

    // Calculate edge vectors
    Vec3 edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    Vec3 edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

    // Cross product gives face normal
    Vec3 normal = {edge1.y * edge2.z - edge1.z * edge2.y,
                   edge1.z * edge2.x - edge1.x * edge2.z,
                   edge1.x * edge2.y - edge1.y * edge2.x};

    // Accumulate normal for each vertex
    mesh->vertices[i0].normal.x += normal.x;
    mesh->vertices[i0].normal.y += normal.y;
    mesh->vertices[i0].normal.z += normal.z;

    mesh->vertices[i1].normal.x += normal.x;
    mesh->vertices[i1].normal.y += normal.y;
    mesh->vertices[i1].normal.z += normal.z;

    mesh->vertices[i2].normal.x += normal.x;
    mesh->vertices[i2].normal.y += normal.y;
    mesh->vertices[i2].normal.z += normal.z;
  }

  // Normalize all accumulated normals
  for (u32 i = 0; i < mesh->vertex_count; i++) {
    Vec3 normal = mesh->vertices[i].normal;
    f32 length =
        sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);

    if (length > EPSILON) {
      mesh->vertices[i].normal.x /= length;
      mesh->vertices[i].normal.y /= length;
      mesh->vertices[i].normal.z /= length;
    } else {
      mesh->vertices[i].normal = (Vec3){0, 1, 0}; // Default up
    }
  }
}

void mesh_calculate_tangents(mesh_t *mesh) {
  if (!mesh || mesh->vertex_count == 0 || mesh->index_count < 3) {
    return;
  }

  // Clear existing tangents
  for (u32 i = 0; i < mesh->vertex_count; i++) {
    mesh->vertices[i].tangent = (Vec4){0, 0, 0, 0};
  }

  // Calculate tangents for each triangle
  for (u32 i = 0; i < mesh->index_count; i += 3) {
    if (i + 2 >= mesh->index_count)
      break;

    u32 i0 = mesh->indices[i];
    u32 i1 = mesh->indices[i + 1];
    u32 i2 = mesh->indices[i + 2];

    if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count ||
        i2 >= mesh->vertex_count) {
      continue;
    }

    Vec3 v0 = mesh->vertices[i0].position;
    Vec3 v1 = mesh->vertices[i1].position;
    Vec3 v2 = mesh->vertices[i2].position;

    Vec2 uv0 = mesh->vertices[i0].texcoord;
    Vec2 uv1 = mesh->vertices[i1].texcoord;
    Vec2 uv2 = mesh->vertices[i2].texcoord;

    // Edge vectors
    Vec3 edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    Vec3 edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

    // UV edge vectors
    Vec2 uv_edge1 = {uv1.x - uv0.x, uv1.y - uv0.y};
    Vec2 uv_edge2 = {uv2.x - uv0.x, uv2.y - uv0.y};

    f32 r = 1.0f / (uv_edge1.x * uv_edge2.y - uv_edge1.y * uv_edge2.x);

    Vec3 tangent = {(edge1.x * uv_edge2.y - edge2.x * uv_edge1.y) * r,
                    (edge1.y * uv_edge2.y - edge2.y * uv_edge1.y) * r,
                    (edge1.z * uv_edge2.y - edge2.z * uv_edge1.y) * r};

    // Accumulate tangents
    mesh->vertices[i0].tangent.x += tangent.x;
    mesh->vertices[i0].tangent.y += tangent.y;
    mesh->vertices[i0].tangent.z += tangent.z;

    mesh->vertices[i1].tangent.x += tangent.x;
    mesh->vertices[i1].tangent.y += tangent.y;
    mesh->vertices[i1].tangent.z += tangent.z;

    mesh->vertices[i2].tangent.x += tangent.x;
    mesh->vertices[i2].tangent.y += tangent.y;
    mesh->vertices[i2].tangent.z += tangent.z;
  }

  // Orthogonalize tangents and calculate handedness
  for (u32 i = 0; i < mesh->vertex_count; i++) {
    Vec3 tangent = {mesh->vertices[i].tangent.x, mesh->vertices[i].tangent.y,
                    mesh->vertices[i].tangent.z};
    Vec3 normal = mesh->vertices[i].normal;

    // Gram-Schmidt orthogonalization
    f32 dot =
        tangent.x * normal.x + tangent.y * normal.y + tangent.z * normal.z;
    tangent.x -= dot * normal.x;
    tangent.y -= dot * normal.y;
    tangent.z -= dot * normal.z;

    // Normalize
    f32 length = sqrtf(tangent.x * tangent.x + tangent.y * tangent.y +
                       tangent.z * tangent.z);
    if (length > EPSILON) {
      tangent.x /= length;
      tangent.y /= length;
      tangent.z /= length;
    } else {
      tangent = (Vec3){1, 0, 0};
    }

    // Calculate handedness (w component)
    Vec3 bitangent = {normal.y * tangent.z - normal.z * tangent.y,
                      normal.z * tangent.x - normal.x * tangent.z,
                      normal.x * tangent.y - normal.y * tangent.x};

    // Cross product of normal and tangent should give bitangent direction
    Vec3 expected_bitangent = {tangent.y * normal.z - tangent.z * normal.y,
                               tangent.z * normal.x - tangent.x * normal.z,
                               tangent.x * normal.y - tangent.y * normal.x};

    f32 handedness = (expected_bitangent.x * bitangent.x +
                      expected_bitangent.y * bitangent.y +
                      expected_bitangent.z * bitangent.z) < 0.0f
                         ? -1.0f
                         : 1.0f;

    mesh->vertices[i].tangent =
        (Vec4){tangent.x, tangent.y, tangent.z, handedness};
  }
}

bool mesh_upload(mesh_t *mesh) {
  // TODO: Implement with backend GPU upload
  // This will integrate with the graphics backend
  (void)mesh;
  printf("Warning: mesh_upload not yet implemented with GPU backend\n");
  return false;
}

void mesh_unload(mesh_t *mesh) {
  // TODO: Implement with backend GPU unload
  // This will integrate with the graphics backend
  (void)mesh;
  printf("Warning: mesh_unload not yet implemented with GPU backend\n");
}

void mesh_print_stats(const mesh_t *mesh) {
  if (!mesh) {
    printf("Mesh: NULL\n");
    return;
  }

  printf("Mesh Stats:\n");
  printf("  Name: %s\n", mesh->name);
  printf("  ID: %u\n", mesh->id);
  printf("  Flags: %u\n", mesh->flags);
  printf("  Vertices: %u / %u\n", mesh->vertex_count, mesh->vertex_capacity);
  printf("  Indices: %u / %u\n", mesh->index_count, mesh->index_capacity);
  printf("  Submeshes: %u\n", mesh->submesh_count);
  printf("  LODs: %u\n", mesh->lod_count);
  printf("  Materials: %u\n", mesh->material_count);
  printf("  Ref Count: %u\n", mesh->ref_count);

  if (mesh->vertex_count > 0) {
    printf("  Bounds:\n");
    printf("    Min: (%.3f, %.3f, %.3f)\n", mesh->bounds.min.x,
           mesh->bounds.min.y, mesh->bounds.min.z);
    printf("    Max: (%.3f, %.3f, %.3f)\n", mesh->bounds.max.x,
           mesh->bounds.max.y, mesh->bounds.max.z);
    printf("    Sphere Center: (%.3f, %.3f, %.3f)\n",
           mesh->bounds.sphere_center.x, mesh->bounds.sphere_center.y,
           mesh->bounds.sphere_center.z);
    printf("    Sphere Radius: %.3f\n", mesh->bounds.sphere_radius);
  }
}

bool mesh_validate(const mesh_t *mesh) {
  if (!mesh) {
    printf("Validation Error: Mesh is NULL\n");
    return false;
  }

  if (mesh->vertex_count == 0 && mesh->index_count == 0) {
    printf("Validation Warning: Empty mesh '%s'\n", mesh->name);
    return true; // Empty mesh is valid
  }

  if (mesh->vertices && mesh->vertex_count > mesh->vertex_capacity) {
    printf("Validation Error: Vertex count exceeds capacity\n");
    return false;
  }

  if (mesh->indices && mesh->index_count > mesh->index_capacity) {
    printf("Validation Error: Index count exceeds capacity\n");
    return false;
  }

  // Check index bounds
  for (u32 i = 0; i < mesh->index_count; i++) {
    if (mesh->indices[i] >= mesh->vertex_count) {
      printf("Validation Error: Index %u out of bounds (vertex %u >= %u)\n", i,
             mesh->indices[i], mesh->vertex_count);
      return false;
    }
  }

  // Check submesh bounds
  for (u32 i = 0; i < mesh->submesh_count; i++) {
    submesh_t *submesh = &mesh->submeshes[i];

    if (submesh->index_start + submesh->index_count > mesh->index_count) {
      printf("Validation Error: Submesh %u index range out of bounds\n", i);
      return false;
    }

    if (submesh->material_index >= MESH_MAX_MATERIALS) {
      printf("Validation Error: Submesh %u material index out of bounds\n", i);
      return false;
    }
  }

  return true;
}
