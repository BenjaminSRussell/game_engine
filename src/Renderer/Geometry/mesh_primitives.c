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
    mesh->vertices[v_ptr + 0].texcoord = (Vec2){0, 0};
    mesh->vertices[v_ptr + 0].tangent = tangent;

    mesh->vertices[v_ptr + 1].position = p[f->v_idx[1]];
    mesh->vertices[v_ptr + 1].normal = normal;
    mesh->vertices[v_ptr + 1].texcoord = (Vec2){1, 0};
    mesh->vertices[v_ptr + 1].tangent = tangent;

    mesh->vertices[v_ptr + 2].position = p[f->v_idx[2]];
    mesh->vertices[v_ptr + 2].normal = normal;
    mesh->vertices[v_ptr + 2].texcoord = (Vec2){1, 1};
    mesh->vertices[v_ptr + 2].tangent = tangent;

    mesh->vertices[v_ptr + 3].position = p[f->v_idx[3]];
    mesh->vertices[v_ptr + 3].normal = normal;
    mesh->vertices[v_ptr + 3].texcoord = (Vec2){0, 1};
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
  if (segments < 3 || radius <= 0.0f) {
    return NULL;
  }

  u32 rings = segments;
  u32 sectors = segments * 2;
  
  // Calculate vertex and index counts
  u32 vertex_count = (rings + 1) * (sectors + 1);
  u32 index_count = rings * sectors * 6;
  
  mesh_t *mesh = mesh_allocate(vertex_count, index_count);
  if (!mesh) {
    return NULL;
  }
  
  f32 const R = 1.0f / (f32)(rings);
  f32 const S = 1.0f / (f32)(sectors);
  u32 v_idx = 0;
  u32 i_idx = 0;
  
  // Generate vertices
  for (u32 r = 0; r <= rings; ++r) {
    for (u32 s = 0; s <= sectors; ++s) {
      f32 const y = sinf(-PI * 0.5f + PI * r * R);
      f32 const x = cosf(2 * PI * s * S) * sinf(PI * r * R);
      f32 const z = sinf(2 * PI * s * S) * sinf(PI * r * R);
      
      mesh->vertices[v_idx].position = (Vec3){x * radius, y * radius, z * radius};
      mesh->vertices[v_idx].normal = (Vec3){x, y, z};
      mesh->vertices[v_idx].texcoord = (Vec2){s * S, r * R};
      
      // Calculate tangent (for sphere, tangent is perpendicular to normal)
      Vec3 tangent = {-z, 0, x};
      f32 tangent_length = sqrtf(tangent.x * tangent.x + tangent.z * tangent.z);
      if (tangent_length > EPSILON) {
        tangent.x /= tangent_length;
        tangent.z /= tangent_length;
      } else {
        tangent = (Vec3){1, 0, 0};
      }
      mesh->vertices[v_idx].tangent = (Vec4){tangent.x, tangent.y, tangent.z, 1.0f};
      
      v_idx++;
    }
  }
  
  // Generate indices
  for (u32 r = 0; r < rings; ++r) {
    for (u32 s = 0; s < sectors; ++s) {
      u32 const cur_row = r * (sectors + 1);
      u32 const next_row = (r + 1) * (sectors + 1);
      u32 const cur = cur_row + s;
      u32 const next = cur_row + s + 1;
      u32 const cur_next_row = next_row + s;
      u32 const next_next_row = next_row + s + 1;
      
      // First triangle
      mesh->indices[i_idx++] = cur;
      mesh->indices[i_idx++] = cur_next_row;
      mesh->indices[i_idx++] = next;
      
      // Second triangle
      mesh->indices[i_idx++] = next;
      mesh->indices[i_idx++] = cur_next_row;
      mesh->indices[i_idx++] = next_next_row;
    }
  }
  
  strncpy(mesh->name, "Sphere", 63);
  return mesh;
}

mesh_t *mesh_create_cylinder(f32 radius, f32 height, u32 segments) {
  if (segments < 3 || radius <= 0.0f || height <= 0.0f) {
    return NULL;
  }
  
  u32 vertex_count = (segments * 2) + 2; // Side vertices + top/bottom center
  u32 index_count = (segments * 6) + (segments * 6); // Side + top + bottom
  
  mesh_t *mesh = mesh_allocate(vertex_count, index_count);
  if (!mesh) {
    return NULL;
  }
  
  f32 half_height = height * 0.5f;
  u32 v_idx = 0;
  u32 i_idx = 0;
  
  // Generate side vertices
  for (u32 i = 0; i < segments; ++i) {
    f32 angle = 2.0f * PI * (f32)i / (f32)segments;
    f32 x = cosf(angle) * radius;
    f32 z = sinf(angle) * radius;
    
    // Bottom vertex
    mesh->vertices[v_idx].position = (Vec3){x, -half_height, z};
    mesh->vertices[v_idx].normal = (Vec3){x / radius, 0.0f, z / radius};
    mesh->vertices[v_idx].texcoord = (Vec2){(f32)i / (f32)segments, 0.0f};
    mesh->vertices[v_idx].tangent = (Vec4){-z / radius, 0.0f, x / radius, 1.0f};
    v_idx++;
    
    // Top vertex
    mesh->vertices[v_idx].position = (Vec3){x, half_height, z};
    mesh->vertices[v_idx].normal = (Vec3){x / radius, 0.0f, z / radius};
    mesh->vertices[v_idx].texcoord = (Vec2){(f32)i / (f32)segments, 1.0f};
    mesh->vertices[v_idx].tangent = (Vec4){-z / radius, 0.0f, x / radius, 1.0f};
    v_idx++;
  }
  
  // Bottom center vertex
  mesh->vertices[v_idx].position = (Vec3){0, -half_height, 0};
  mesh->vertices[v_idx].normal = (Vec3){0, -1.0f, 0};
  mesh->vertices[v_idx].texcoord = (Vec2){0.5f, 0.5f};
  mesh->vertices[v_idx].tangent = (Vec4){1, 0, 0, 1.0f};
  u32 bottom_center_idx = v_idx++;
  
  // Top center vertex
  mesh->vertices[v_idx].position = (Vec3){0, half_height, 0};
  mesh->vertices[v_idx].normal = (Vec3){0, 1.0f, 0};
  mesh->vertices[v_idx].texcoord = (Vec2){0.5f, 0.5f};
  mesh->vertices[v_idx].tangent = (Vec4){1, 0, 0, 1.0f};
  u32 top_center_idx = v_idx++;
  
  // Generate side faces
  for (u32 i = 0; i < segments; ++i) {
    u32 next = (i + 1) % segments;
    u32 bottom_idx = i * 2;
    u32 top_idx = bottom_idx + 1;
    u32 next_bottom_idx = next * 2;
    u32 next_top_idx = next_bottom_idx + 1;
    
    // First triangle
    mesh->indices[i_idx++] = bottom_idx;
    mesh->indices[i_idx++] = next_bottom_idx;
    mesh->indices[i_idx++] = top_idx;
    
    // Second triangle
    mesh->indices[i_idx++] = top_idx;
    mesh->indices[i_idx++] = next_bottom_idx;
    mesh->indices[i_idx++] = next_top_idx;
  }
  
  // Generate top cap
  for (u32 i = 0; i < segments; ++i) {
    u32 next = (i + 1) % segments;
    u32 top_idx = i * 2 + 1;
    u32 next_top_idx = next * 2 + 1;
    
    mesh->indices[i_idx++] = top_center_idx;
    mesh->indices[i_idx++] = next_top_idx;
    mesh->indices[i_idx++] = top_idx;
  }
  
  // Generate bottom cap
  for (u32 i = 0; i < segments; ++i) {
    u32 next = (i + 1) % segments;
    u32 bottom_idx = i * 2;
    u32 next_bottom_idx = next * 2;
    
    mesh->indices[i_idx++] = bottom_center_idx;
    mesh->indices[i_idx++] = bottom_idx;
    mesh->indices[i_idx++] = next_bottom_idx;
  }
  
  strncpy(mesh->name, "Cylinder", 63);
  return mesh;
}

mesh_t *mesh_create_plane(f32 width, f32 depth, u32 subdivisions) {
  if (width <= 0.0f || depth <= 0.0f || subdivisions == 0) {
    return NULL;
  }
  
  u32 vertices_per_side = subdivisions + 1;
  u32 vertex_count = vertices_per_side * vertices_per_side;
  u32 quad_count = subdivisions * subdivisions;
  u32 index_count = quad_count * 6;
  
  mesh_t *mesh = mesh_allocate(vertex_count, index_count);
  if (!mesh) {
    return NULL;
  }
  
  f32 half_width = width * 0.5f;
  f32 half_depth = depth * 0.5f;
  u32 v_idx = 0;
  u32 i_idx = 0;
  
  // Generate vertices
  for (u32 z = 0; z < vertices_per_side; ++z) {
    for (u32 x = 0; x < vertices_per_side; ++x) {
      f32 px = (f32)x / (f32)subdivisions * width - half_width;
      f32 pz = (f32)z / (f32)subdivisions * depth - half_depth;
      
      mesh->vertices[v_idx].position = (Vec3){px, 0.0f, pz};
      mesh->vertices[v_idx].normal = (Vec3){0, 1.0f, 0};
      mesh->vertices[v_idx].texcoord = (Vec2){(f32)x / (f32)subdivisions, (f32)z / (f32)subdivisions};
      mesh->vertices[v_idx].tangent = (Vec4){1, 0, 0, 1.0f};
      v_idx++;
    }
  }
  
  // Generate indices
  for (u32 z = 0; z < subdivisions; ++z) {
    for (u32 x = 0; x < subdivisions; ++x) {
      u32 top_left = z * vertices_per_side + x;
      u32 top_right = top_left + 1;
      u32 bottom_left = (z + 1) * vertices_per_side + x;
      u32 bottom_right = bottom_left + 1;
      
      // First triangle
      mesh->indices[i_idx++] = top_left;
      mesh->indices[i_idx++] = bottom_left;
      mesh->indices[i_idx++] = top_right;
      
      // Second triangle
      mesh->indices[i_idx++] = top_right;
      mesh->indices[i_idx++] = bottom_left;
      mesh->indices[i_idx++] = bottom_right;
    }
  }
  
  strncpy(mesh->name, "Plane", 63);
  return mesh;
}

mesh_t *mesh_create_cone(f32 radius, f32 height, u32 segments) {
  if (segments < 3 || radius <= 0.0f || height <= 0.0f) {
    return NULL;
  }
  
  u32 vertex_count = segments + 2; // Base vertices + tip + base center
  u32 index_count = segments * 6; // Side triangles + base triangles
  
  mesh_t *mesh = mesh_allocate(vertex_count, index_count);
  if (!mesh) {
    return NULL;
  }
  
  u32 v_idx = 0;
  u32 i_idx = 0;
  
  // Generate base vertices
  for (u32 i = 0; i < segments; ++i) {
    f32 angle = 2.0f * PI * (f32)i / (f32)segments;
    f32 x = cosf(angle) * radius;
    f32 z = sinf(angle) * radius;
    
    mesh->vertices[v_idx].position = (Vec3){x, 0.0f, z};
    mesh->vertices[v_idx].normal = (Vec3){x, radius, z}; // Will be normalized later
    mesh->vertices[v_idx].texcoord = (Vec2){(f32)i / (f32)segments, 0.0f};
    mesh->vertices[v_idx].tangent = (Vec4){-z, 0.0f, x, 1.0f};
    v_idx++;
  }
  
  // Tip vertex
  mesh->vertices[v_idx].position = (Vec3){0, height, 0};
  mesh->vertices[v_idx].normal = (Vec3){0, 1.0f, 0};
  mesh->vertices[v_idx].texcoord = (Vec2){0.5f, 1.0f};
  mesh->vertices[v_idx].tangent = (Vec4){1, 0, 0, 1.0f};
  u32 tip_idx = v_idx++;
  
  // Base center vertex
  mesh->vertices[v_idx].position = (Vec3){0, 0.0f, 0};
  mesh->vertices[v_idx].normal = (Vec3){0, -1.0f, 0};
  mesh->vertices[v_idx].texcoord = (Vec2){0.5f, 0.5f};
  mesh->vertices[v_idx].tangent = (Vec4){1, 0, 0, 1.0f};
  u32 base_center_idx = v_idx++;
  
  // Normalize side normals
  for (u32 i = 0; i < segments; ++i) {
    Vec3 normal = mesh->vertices[i].normal;
    f32 length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (length > EPSILON) {
      mesh->vertices[i].normal.x /= length;
      mesh->vertices[i].normal.y /= length;
      mesh->vertices[i].normal.z /= length;
    }
  }
  
  // Generate side faces
  for (u32 i = 0; i < segments; ++i) {
    u32 next = (i + 1) % segments;
    
    mesh->indices[i_idx++] = i;
    mesh->indices[i_idx++] = next;
    mesh->indices[i_idx++] = tip_idx;
  }
  
  // Generate base
  for (u32 i = 0; i < segments; ++i) {
    u32 next = (i + 1) % segments;
    
    mesh->indices[i_idx++] = base_center_idx;
    mesh->indices[i_idx++] = next;
    mesh->indices[i_idx++] = i;
  }
  
  strncpy(mesh->name, "Cone", 63);
  return mesh;
}

mesh_t *mesh_create_torus(f32 major_radius, f32 minor_radius,
                          u32 major_segments, u32 minor_segments) {
  if (major_segments < 3 || minor_segments < 3 || 
      major_radius <= 0.0f || minor_radius <= 0.0f) {
    return NULL;
  }
  
  u32 vertex_count = (major_segments + 1) * (minor_segments + 1);
  u32 index_count = major_segments * minor_segments * 6;
  
  mesh_t *mesh = mesh_allocate(vertex_count, index_count);
  if (!mesh) {
    return NULL;
  }
  
  u32 v_idx = 0;
  u32 i_idx = 0;
  
  // Generate vertices
  for (u32 i = 0; i <= major_segments; ++i) {
    f32 u = 2.0f * PI * (f32)i / (f32)major_segments;
    
    for (u32 j = 0; j <= minor_segments; ++j) {
      f32 v = 2.0f * PI * (f32)j / (f32)minor_segments;
      
      f32 cos_u = cosf(u);
      f32 sin_u = sinf(u);
      f32 cos_v = cosf(v);
      f32 sin_v = sinf(v);
      
      f32 x = (major_radius + minor_radius * cos_v) * cos_u;
      f32 y = minor_radius * sin_v;
      f32 z = (major_radius + minor_radius * cos_v) * sin_u;
      
      mesh->vertices[v_idx].position = (Vec3){x, y, z};
      
      // Normal is normalized position from center of tube
      Vec3 center_to_point = {x - major_radius * cos_u, y, z - major_radius * sin_u};
      f32 length = sqrtf(center_to_point.x * center_to_point.x + 
                        center_to_point.y * center_to_point.y + 
                        center_to_point.z * center_to_point.z);
      if (length > EPSILON) {
        mesh->vertices[v_idx].normal.x = center_to_point.x / length;
        mesh->vertices[v_idx].normal.y = center_to_point.y / length;
        mesh->vertices[v_idx].normal.z = center_to_point.z / length;
      } else {
        mesh->vertices[v_idx].normal = (Vec3){1, 0, 0};
      }
      
      mesh->vertices[v_idx].texcoord = (Vec2){(f32)i / (f32)major_segments, (f32)j / (f32)minor_segments};
      
      // Calculate tangent (along major circle)
      Vec3 tangent = {-sin_u, 0, cos_u};
      length = sqrtf(tangent.x * tangent.x + tangent.z * tangent.z);
      if (length > EPSILON) {
        mesh->vertices[v_idx].tangent.x = tangent.x / length;
        mesh->vertices[v_idx].tangent.y = tangent.y / length;
        mesh->vertices[v_idx].tangent.z = tangent.z / length;
        mesh->vertices[v_idx].tangent.w = 1.0f;
      } else {
        mesh->vertices[v_idx].tangent = (Vec4){1, 0, 0, 1.0f};
      }
      
      v_idx++;
    }
  }
  
  // Generate indices
  for (u32 i = 0; i < major_segments; ++i) {
    for (u32 j = 0; j < minor_segments; ++j) {
      u32 cur_row = i * (minor_segments + 1);
      u32 next_row = (i + 1) * (minor_segments + 1);
      u32 cur = cur_row + j;
      u32 next = cur_row + j + 1;
      u32 cur_next_row = next_row + j;
      u32 next_next_row = next_row + j + 1;
      
      // First triangle
      mesh->indices[i_idx++] = cur;
      mesh->indices[i_idx++] = cur_next_row;
      mesh->indices[i_idx++] = next;
      
      // Second triangle
      mesh->indices[i_idx++] = next;
      mesh->indices[i_idx++] = cur_next_row;
      mesh->indices[i_idx++] = next_next_row;
    }
  }
  
  strncpy(mesh->name, "Torus", 63);
  return mesh;
}
