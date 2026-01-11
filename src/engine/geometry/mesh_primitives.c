#include "geometry/mesh_primitives.h"
#include <stdlib.h>
#include <string.h>

// Helper to allocate mesh
static mesh_t *mesh_allocate(u32 vertex_count, u32 index_count) {
  mesh_t *mesh = (mesh_t *)calloc(1, sizeof(mesh_t));
  if (!mesh)
    return NULL;

  mesh->vertex_count = vertex_count;
  mesh->vertex_capacity = vertex_count;
  mesh->vertices = (vertex_t *)calloc(vertex_count, sizeof(vertex_t));

  mesh->index_count = index_count;
  mesh->index_capacity = index_count;
  mesh->indices = (u32 *)calloc(index_count, sizeof(u32));

  // Default submesh
  mesh->submesh_count = 1;
  mesh->submeshes = (submesh_t *)calloc(1, sizeof(submesh_t));
  mesh->submeshes[0].index_count = index_count;
  mesh->submeshes[0].vertex_count = vertex_count;

  return mesh;
}

mesh_t *mesh_create_cube(f32 size) {
  // 24 vertices (4 per face * 6 faces)
  // 36 indices (6 per face * 6 faces)
  mesh_t *mesh = mesh_allocate(24, 36);
  if (!mesh)
    return NULL;

  f32 h = size * 0.5f;

  // Positions
  Vec3 p[8] = {
      {-h, -h, h},  {h, -h, h},  {h, h, h},  {-h, h, h}, // Front
      {-h, -h, -h}, {h, -h, -h}, {h, h, -h}, {-h, h, -h} // Back
  };

  // Normals
  Vec3 n[6] = {
      {0, 0, 1},  // Front
      {0, 0, -1}, // Back
      {0, 1, 0},  // Top
      {0, -1, 0}, // Bottom
      {1, 0, 0},  // Right
      {-1, 0, 0}  // Left
  };

  // Faces (Normal Index, Vertex Indices...)
  // CCW Winding
  struct Face {
    int n_idx;
    int v_idx[4]; // BL, BR, TR, TL
  } faces[6] = {
      {0, {0, 1, 2, 3}}, // Front
      {1, {5, 4, 7, 6}}, // Back
      {2, {3, 2, 6, 7}}, // Top
      {3, {4, 5, 1, 0}}, // Bottom
      {4, {1, 5, 6, 2}}, // Right
      {5, {4, 0, 3, 7}}  // Left
  };

  u32 v_ptr = 0;
  u32 i_ptr = 0;

  for (int i = 0; i < 6; i++) {
    struct Face *f = &faces[i];
    Vec3 normal = n[f->n_idx];
    Vec4 tangent = {0, 0, 0, 1}; // Placeholder

    // Vertices
    mesh->vertices[v_ptr + 0].position = p[f->v_idx[0]];
    mesh->vertices[v_ptr + 0].normal = normal;
    mesh->vertices[v_ptr + 0].uv = (Vec2){0, 0};
    mesh->vertices[v_ptr + 0].tangent = tangent;

    mesh->vertices[v_ptr + 1].position = p[f->v_idx[1]];
    mesh->vertices[v_ptr + 1].normal = normal;
    mesh->vertices[v_ptr + 1].uv = (Vec2){1, 0};
    mesh->vertices[v_ptr + 1].tangent = tangent;

    mesh->vertices[v_ptr + 2].position = p[f->v_idx[2]];
    mesh->vertices[v_ptr + 2].normal = normal;
    mesh->vertices[v_ptr + 2].uv = (Vec2){1, 1};
    mesh->vertices[v_ptr + 2].tangent = tangent;

    mesh->vertices[v_ptr + 3].position = p[f->v_idx[3]];
    mesh->vertices[v_ptr + 3].normal = normal;
    mesh->vertices[v_ptr + 3].uv = (Vec2){0, 1};
    mesh->vertices[v_ptr + 3].tangent = tangent;

    // Indices (Two triangles)
    mesh->indices[i_ptr + 0] = v_ptr + 0;
    mesh->indices[i_ptr + 1] = v_ptr + 1;
    mesh->indices[i_ptr + 2] = v_ptr + 2;

    mesh->indices[i_ptr + 3] = v_ptr + 0;
    mesh->indices[i_ptr + 4] = v_ptr + 2;
    mesh->indices[i_ptr + 5] = v_ptr + 3;

    v_ptr += 4;
    i_ptr += 6;
  }

  strncpy(mesh->name, "Cube", 63);
  return mesh;
}

mesh_t *mesh_create_sphere(f32 radius, u32 segments) {
  // Stub
  (void)radius;
  (void)segments;
  return NULL;
}

mesh_t *mesh_create_cylinder(f32 radius, f32 height, u32 segments) {
  (void)radius;
  (void)height;
  (void)segments;
  return NULL;
}

mesh_t *mesh_create_plane(f32 width, f32 depth, u32 subdivisions) {
  (void)width;
  (void)depth;
  (void)subdivisions;
  return NULL;
}

mesh_t *mesh_create_cone(f32 radius, f32 height, u32 segments) {
  (void)radius;
  (void)height;
  (void)segments;
  return NULL;
}

mesh_t *mesh_create_torus(f32 major_radius, f32 minor_radius,
                          u32 major_segments, u32 minor_segments) {
  (void)major_radius;
  (void)minor_radius;
  (void)major_segments;
  (void)minor_segments;
  return NULL;
}
