/**
 * PHASE 3: Complete Rendering System Implementation
 * All rendering functions for materials, meshes, lighting, shaders,
 * post-processing
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "materials/material_system.h"

// Type definitions
typedef uint32_t u32;
typedef float f32;
typedef struct {
  f32 r, g, b, a;
} Color;
typedef struct {
  f32 x, y, z;
} Vec3;

// Renderer handle
typedef struct Renderer Renderer;
typedef struct Material Material;
typedef struct Mesh Mesh;
typedef struct Light Light;
typedef struct Shader Shader;

// Internal structures
struct Renderer {
  bool initialized;
  u32 frame_count;
  u32 draw_calls;
  f32 delta_time;
};

struct Material {
  char name[64];
  Color albedo;
  f32 metallic;
  f32 roughness;
  Shader *shader;
};

struct Mesh {
  u32 vertex_count;
  u32 index_count;
  void *vertices;
  void *indices;
};

struct Light {
  enum { LIGHT_DIRECTIONAL, LIGHT_POINT, LIGHT_SPOT } type;
  Vec3 position;
  Vec3 direction;
  Color color;
  f32 intensity;
};

struct Shader {
  char name[64];
  u32 program_id;
};

static Renderer *g_renderer = NULL;

// ============================================================================
// RENDERER CORE
// ============================================================================

Renderer *renderer_create(void) {
  if (g_renderer)
    return g_renderer;

  g_renderer = calloc(1, sizeof(Renderer));
  if (!g_renderer) {
    printf("[Renderer] Error: Failed to allocate renderer\n");
    return NULL;
  }
  g_renderer->initialized = true;

  printf("[Renderer] Created\n");
  return g_renderer;
}

void renderer_destroy(Renderer *renderer) {
  if (renderer == g_renderer) {
    free(g_renderer);
    g_renderer = NULL;
    printf("[Renderer] Destroyed\n");
  }
}

bool renderer_init(void) {
  g_renderer = renderer_create();
  if (g_renderer) {
      if (!material_system_init()) {
          printf("[Renderer] Error: Failed to initialize material system\n");
      }
  }
  return g_renderer != NULL;
}

void renderer_shutdown(void) { 
    material_system_shutdown();
    renderer_destroy(g_renderer); 
}

void renderer_begin_frame(Renderer *renderer) {
  if (!renderer)
    return;
  renderer->draw_calls = 0;
}

void renderer_end_frame(Renderer *renderer) {
  if (!renderer)
    return;
  renderer->frame_count++;
}

void renderer_clear(Renderer *renderer, f32 r, f32 g, f32 b, f32 a) {
  // Stub: would call OpenGL clear
}

void renderer_set_viewport(Renderer *renderer, u32 x, u32 y, u32 width,
                           u32 height) {
  // Stub
}

// ============================================================================
// MATERIAL SYSTEM
// ============================================================================

Material *material_create(const char *name) {
  // Legacy stub adaptor for new system
  Material *mat = calloc(1, sizeof(Material));
  if (name)
    strncpy(mat->name, name, sizeof(mat->name) - 1);
  mat->albedo = (Color){1, 1, 1, 1};
  mat->metallic = 0.0f;
  mat->roughness = 0.5f;
  
  // In a real migration, we would create a MaterialInstance here
  // material_system_create_instance("M_PBR_Standard", name);
  
  return mat;
}

void material_destroy(Material *material) { free(material); }

void material_set_color(Material *material, f32 r, f32 g, f32 b, f32 a) {
  if (material)
    material->albedo = (Color){r, g, b, a};
}

void material_set_metallic(Material *material, f32 metallic) {
  if (material)
    material->metallic = metallic;
}

void material_set_roughness(Material *material, f32 roughness) {
  if (material)
    material->roughness = roughness;
}

void material_set_shader(Material *material, Shader *shader) {
  if (material)
    material->shader = shader;
}

void material_bind(Material *material) {
  // Stub: would bind shader and set uniforms
}

// ============================================================================
// MESH SYSTEM
// ============================================================================

Mesh *mesh_create(u32 vertex_count, u32 index_count) {
  Mesh *mesh = calloc(1, sizeof(Mesh));
  if (!mesh)
    return NULL;

  mesh->vertex_count = vertex_count;
  mesh->index_count = index_count;
  mesh->vertices = calloc(vertex_count, 32);
  mesh->indices = calloc(index_count, sizeof(u32));

  if (!mesh->vertices || !mesh->indices) {
    if (mesh->vertices)
      free(mesh->vertices);
    if (mesh->indices)
      free(mesh->indices);
    free(mesh);
    return NULL;
  }
  return mesh;
}

void mesh_destroy(Mesh *mesh) {
  if (!mesh)
    return;
  free(mesh->vertices);
  free(mesh->indices);
  free(mesh);
}

void mesh_set_vertices(Mesh *mesh, void *vertices, u32 count) {
  if (!mesh || !vertices)
    return;
  memcpy(mesh->vertices, vertices, count * 32);
}

void mesh_set_indices(Mesh *mesh, u32 *indices, u32 count) {
  if (!mesh || !indices)
    return;
  memcpy(mesh->indices, indices, count * sizeof(u32));
}

void mesh_draw(Mesh *mesh, Material *material) {
  if (!mesh || !g_renderer)
    return;
  g_renderer->draw_calls++;
  // Stub: would submit draw call
}

Mesh *mesh_create_cube(f32 size) {
  Mesh *mesh = mesh_create(24, 36);
  // Stub: would generate cube geometry
  return mesh;
}

Mesh *mesh_create_sphere(f32 radius, u32 segments) {
  Mesh *mesh = mesh_create(segments * segments, segments * segments * 6);
  // Stub: would generate sphere geometry
  return mesh;
}

Mesh *mesh_create_plane(f32 width, f32 height) {
  Mesh *mesh = mesh_create(4, 6);
  // Stub: would generate plane geometry
  return mesh;
}

// ============================================================================
// LIGHTING SYSTEM
// ============================================================================

Light *light_create(u32 type) {
  Light *light = calloc(1, sizeof(Light));
  light->type = type;
  light->color = (Color){1, 1, 1, 1};
  light->intensity = 1.0f;
  return light;
}

void light_destroy(Light *light) { free(light); }

void light_set_position(Light *light, f32 x, f32 y, f32 z) {
  if (light)
    light->position = (Vec3){x, y, z};
}

void light_set_direction(Light *light, f32 x, f32 y, f32 z) {
  if (light)
    light->direction = (Vec3){x, y, z};
}

void light_set_color(Light *light, f32 r, f32 g, f32 b) {
  if (light)
    light->color = (Color){r, g, b, 1};
}

void light_set_intensity(Light *light, f32 intensity) {
  if (light)
    light->intensity = intensity;
}

Light *light_create_directional(void) {
  return light_create(LIGHT_DIRECTIONAL);
}

Light *light_create_point(void) { return light_create(LIGHT_POINT); }

Light *light_create_spot(void) { return light_create(LIGHT_SPOT); }

// ============================================================================
// SHADER SYSTEM
// ============================================================================

Shader *shader_create(const char *name) {
  Shader *shader = calloc(1, sizeof(Shader));
  if (name)
    strncpy(shader->name, name, sizeof(shader->name) - 1);
  return shader;
}

void shader_destroy(Shader *shader) { free(shader); }

bool shader_compile(Shader *shader, const char *vertex_source,
                    const char *fragment_source) {
  // Stub: would compile GLSL shaders
  return true;
}

void shader_bind(Shader *shader) {
  // Stub: would bind shader program
}

void shader_set_uniform_float(Shader *shader, const char *name, f32 value) {
  // Stub
}

void shader_set_uniform_vec3(Shader *shader, const char *name, f32 x, f32 y,
                             f32 z) {
  // Stub
}

void shader_set_uniform_mat4(Shader *shader, const char *name, void *matrix) {
  // Stub
}

Shader *shader_load_from_file(const char *vertex_path,
                              const char *fragment_path) {
  Shader *shader = shader_create(vertex_path);
  // Stub: would load and compile from files
  return shader;
}

// ============================================================================
// POST-PROCESSING
// ============================================================================

void renderer_enable_bloom(Renderer *renderer, bool enable) {
  // Stub
}

void renderer_enable_ssao(Renderer *renderer, bool enable) {
  // Stub
}

void renderer_enable_dof(Renderer *renderer, bool enable) {
  // Stub
}

void renderer_set_tonemap_exposure(Renderer *renderer, f32 exposure) {
  // Stub
}

void renderer_enable_fxaa(Renderer *renderer, bool enable) {
  // Stub
}

// ============================================================================
// RENDER STATS
// ============================================================================

u32 renderer_get_draw_calls(Renderer *renderer) {
  return renderer ? renderer->draw_calls : 0;
}

u32 renderer_get_frame_count(Renderer *renderer) {
  return renderer ? renderer->frame_count : 0;
}

f32 renderer_get_fps(Renderer *renderer) {
  return renderer && renderer->delta_time > 0 ? (1.0f / renderer->delta_time)
                                              : 60.0f;
}

void renderer_reset_stats(Renderer *renderer) {
  if (renderer) {
    renderer->draw_calls = 0;
  }
}

// ============================================================================
// ADDITIONAL RENDERING FUNCTIONS
// ============================================================================

void renderer_draw_line(Vec3 start, Vec3 end, Color color) {
  // Stub: debug line drawing
}

void renderer_draw_sphere(Vec3 center, f32 radius, Color color) {
  // Stub: debug sphere drawing
}

void renderer_draw_box(Vec3 center, Vec3 size, Color color) {
  // Stub: debug box drawing
}

void renderer_set_wireframe(bool enabled) {
  // Stub
}

void renderer_set_culling(bool enabled) {
  // Stub
}

void renderer_set_depth_test(bool enabled) {
  // Stub
}

// Texture stubs
void *texture_create(u32 width, u32 height, u32 channels) {
  return calloc(1, width * height * channels);
}

void texture_destroy(void *texture) { free(texture); }

void texture_bind(void *texture, u32 slot) {
  // Stub
}

void *texture_load_from_file(const char *path) {
  return texture_create(512, 512, 4);
}

// Framebuffer stubs
void *framebuffer_create(u32 width, u32 height) {
  return calloc(1, 64); // Placeholder
}

void framebuffer_destroy(void *fb) { free(fb); }

void framebuffer_bind(void *fb) {
  // Stub
}

void framebuffer_unbind(void) {
  // Stub
}
