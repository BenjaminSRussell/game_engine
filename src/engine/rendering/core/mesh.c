// src/engine/renderer/core/mesh.c
//
// Purpose: Mesh management and rendering
//
#include <core/logger.h>
#include <core/memory.h>
#include <include/math/math.h>
#include <rendering/material.h>
#include <rendering/mesh.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

bool mesh_create(Mesh *mesh, Vertex *vertices, u32 vertex_count, u32 *indices,
                 u32 index_count) {
  if (!mesh || !vertices || vertex_count == 0)
    return false;

  memset(mesh, 0, sizeof(Mesh));

  // Allocate and copy data
  mesh->vertex_count = vertex_count;
  mesh->vertices = (Vertex *)malloc(vertex_count * sizeof(Vertex));
  memcpy(mesh->vertices, vertices, vertex_count * sizeof(Vertex));

  if (indices && index_count > 0) {
    mesh->index_count = index_count;
    mesh->indices = (u32 *)malloc(index_count * sizeof(u32));
    memcpy(mesh->indices, indices, index_count * sizeof(u32));
  }

  mesh->is_uploaded = false;

  return true;
}

void mesh_destroy(Mesh *mesh) {
  if (!mesh)
    return;

  if (mesh->vertices) {
    free(mesh->vertices);
    mesh->vertices = NULL;
  }

  if (mesh->indices) {
    free(mesh->indices);
    mesh->indices = NULL;
  }

  if (mesh->is_uploaded) {
    glDeleteVertexArrays(1, &mesh->vao);
    glDeleteBuffers(1, &mesh->vbo);
    if (mesh->index_count > 0) {
      glDeleteBuffers(1, &mesh->ebo);
    }
  }

  memset(mesh, 0, sizeof(Mesh));
}

void mesh_upload_to_gpu(Mesh *mesh) {
#ifndef VULKAN_BUILD
  if (!mesh || mesh->is_uploaded)
    return;

  // Generate buffers
  glGenVertexArrays(1, &mesh->vao);
  glGenBuffers(1, &mesh->vbo);
  if (mesh->index_count > 0) {
    glGenBuffers(1, &mesh->ebo);
  }

  // Bind VAO
  glBindVertexArray(mesh->vao);

  // Upload Vertex Data
  glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
  glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count * sizeof(Vertex),
               mesh->vertices, GL_STATIC_DRAW);

  // Upload Index Data
  if (mesh->index_count > 0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->index_count * sizeof(u32),
                 mesh->indices, GL_STATIC_DRAW);
  }

  // Configure Attributes
  // 0: Position (vec3)
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, position));

  // 1: Normal (vec3)
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, normal));

  // 2: TexCoord (vec2)
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, uv));

  // Unbind
  glBindVertexArray(0);

  mesh->is_uploaded = true;
  LOG_INFO("Mesh uploaded to GPU (VAO: %u, V: %u, I: %u)", mesh->vao,
           mesh->vertex_count, mesh->index_count);
#else
  // Vulkan mesh upload is handled by vulkan_upload_mesh or similar
  // LOG_WARN("mesh_upload_to_gpu is invalid in Vulkan build (use streaming
  // system)");
  (void)mesh;
#endif
}

void mesh_render(const Mesh *mesh) {
#ifndef VULKAN_BUILD
  if (!mesh || !mesh->is_uploaded)
    return;

  // Bind Material if exists
  if (mesh->material) {
    material_bind(mesh->material);
  }

  glBindVertexArray(mesh->vao);

  if (mesh->index_count > 0) {
    glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
  } else {
    glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);
  }

  glBindVertexArray(0);
#else
  (void)mesh;
#endif
}

// Procedural Mesh Generation
Mesh *mesh_create_cube(f32 size) {
  Mesh *mesh = (Mesh *)malloc(sizeof(Mesh));
  f32 s = size * 0.5f;

  // Cube vertices (24 for sharp edges)
  Vertex vertices[] = {// Front face
                       {{-s, -s, s}, {0, 0, 1}, {0, 0}},
                       {{s, -s, s}, {0, 0, 1}, {1, 0}},
                       {{s, s, s}, {0, 0, 1}, {1, 1}},
                       {{-s, s, s}, {0, 0, 1}, {0, 1}},
                       // Back face
                       {{-s, -s, -s}, {0, 0, -1}, {1, 0}},
                       {{-s, s, -s}, {0, 0, -1}, {1, 1}},
                       {{s, s, -s}, {0, 0, -1}, {0, 1}},
                       {{s, -s, -s}, {0, 0, -1}, {0, 0}},
                       // Top face
                       {{-s, s, -s}, {0, 1, 0}, {0, 1}},
                       {{-s, s, s}, {0, 1, 0}, {0, 0}},
                       {{s, s, s}, {0, 1, 0}, {1, 0}},
                       {{s, s, -s}, {0, 1, 0}, {1, 1}},
                       // Bottom face
                       {{-s, -s, -s}, {0, -1, 0}, {1, 1}},
                       {{s, -s, -s}, {0, -1, 0}, {0, 1}},
                       {{s, -s, s}, {0, -1, 0}, {0, 0}},
                       {{-s, -s, s}, {0, -1, 0}, {1, 0}},
                       // Right face
                       {{s, -s, -s}, {1, 0, 0}, {1, 0}},
                       {{s, s, -s}, {1, 0, 0}, {1, 1}},
                       {{s, s, s}, {1, 0, 0}, {0, 1}},
                       {{s, -s, s}, {1, 0, 0}, {0, 0}},
                       // Left face
                       {{-s, -s, -s}, {-1, 0, 0}, {0, 0}},
                       {{-s, -s, s}, {-1, 0, 0}, {1, 0}},
                       {{-s, s, s}, {-1, 0, 0}, {1, 1}},
                       {{-s, s, -s}, {-1, 0, 0}, {0, 1}}};

  // Indices (2 triangles per face)
  u32 indices[] = {
      0,  1,  2,  2,  3,  0,  // Front
      4,  5,  6,  6,  7,  4,  // Back
      8,  9,  10, 10, 11, 8,  // Top
      12, 13, 14, 14, 15, 12, // Bottom
      16, 17, 18, 18, 19, 16, // Right
      20, 21, 22, 22, 23, 20  // Left
  };

  mesh_create(mesh, vertices, 24, indices, 36);
  return mesh;
}

Mesh *mesh_create_plane(f32 width, f32 height) {
  Mesh *mesh = (Mesh *)malloc(sizeof(Mesh));
  f32 w = width * 0.5f;
  f32 h = height * 0.5f;

  Vertex vertices[] = {{{-w, 0, -h}, {0, 1, 0}, {0, 0}},
                       {{-w, 0, h}, {0, 1, 0}, {0, 1}},
                       {{w, 0, h}, {0, 1, 0}, {1, 1}},
                       {{w, 0, -h}, {0, 1, 0}, {1, 0}}};

  u32 indices[] = {0, 1, 2, 2, 3, 0};

  mesh_create(mesh, vertices, 4, indices, 6);
  return mesh;
}

Mesh *mesh_create_sphere(f32 radius, u32 segments) {
  if (segments < 3)
    segments = 3;

  u32 rings = segments;
  u32 vertex_count = (segments + 1) * (rings + 1);
  u32 index_count = segments * rings * 6;

  Mesh *mesh = (Mesh *)malloc(sizeof(Mesh));
  Vertex *vertices = (Vertex *)malloc(vertex_count * sizeof(Vertex));
  u32 *indices = (u32 *)malloc(index_count * sizeof(u32));

  u32 v_idx = 0;
  for (u32 i = 0; i <= rings; i++) {
    f32 v = (f32)i / (f32)rings;
    f32 phi = v * M_PI;

    for (u32 j = 0; j <= segments; j++) {
      f32 u = (f32)j / (f32)segments;
      f32 theta = u * M_PI * 2.0f;

      f32 x = -sinf(phi) * cosf(theta); // Standard parametric sphere
      f32 y = cosf(phi);
      f32 z = sinf(phi) * sinf(theta);

      vertices[v_idx].position = vec3(x * radius, y * radius, z * radius);
      vertices[v_idx].normal = vec3(x, y, z);
      vertices[v_idx].uv = vec2(u, v);
      vertices[v_idx].ao = 0;
      vertices[v_idx].light = 0;
      vertices[v_idx].texture_id = 0;
      vertices[v_idx].wave_phase = 0.0f;
      v_idx++;
    }
  }

  u32 i_idx = 0;
  for (u32 i = 0; i < rings; i++) {
    for (u32 j = 0; j < segments; j++) {
      u32 next_i = i + 1;
      u32 next_j = j + 1;

      u32 a = i * (segments + 1) + j;
      u32 b = next_i * (segments + 1) + j;
      u32 c = next_i * (segments + 1) + next_j;
      u32 d = i * (segments + 1) + next_j;

      indices[i_idx++] = a;
      indices[i_idx++] = b;
      indices[i_idx++] = d;

      indices[i_idx++] = b;
      indices[i_idx++] = c;
      indices[i_idx++] = d;
    }
  }

  mesh_create(mesh, vertices, vertex_count, indices, index_count);
  free(vertices);
  free(indices);
  return mesh;
}
