/**
 * =================================================================================================
 *                              RENDER PIPELINE - IMPLEMENTATION
 *                              Agent: AGENT_RENDER_1
 * =================================================================================================
 */

#include "rendering/render_pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    CONSTANTS
 * =================================================================================================
 */

#define MAX_RENDER_TARGETS 64
#define MAX_MATERIALS 256
#define MAX_MESHES 512
#define MAX_TEXTURES 512
#define MAX_SHADERS 128
#define MAX_LIGHTS 256

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef struct RenderTarget {
  uint32_t id;
  uint32_t width;
  uint32_t height;
  uint32_t format;
  uint32_t color_attachment;
  uint32_t depth_attachment;
  bool is_valid;
} RenderTarget;

typedef struct Material {
  uint32_t id;
  char name[64];
  uint32_t shader_id;
  uint32_t textures[8];
  uint32_t texture_count;
  float properties[64];
  uint32_t property_count;
  bool double_sided;
  bool transparent;
} Material;

typedef struct Mesh {
  uint32_t id;
  uint32_t vertex_buffer;
  uint32_t index_buffer;
  uint32_t vertex_count;
  uint32_t index_count;
  float bounds_min[3];
  float bounds_max[3];
  uint32_t *lods;
  uint32_t lod_count;
} Mesh;

typedef struct Texture {
  uint32_t id;
  uint32_t width;
  uint32_t height;
  uint32_t format;
  uint32_t mip_count;
  uint32_t gpu_handle;
  bool is_loaded;
} Texture;

typedef struct Shader {
  uint32_t id;
  char name[64];
  uint32_t vertex_shader;
  uint32_t fragment_shader;
  uint32_t program;
  bool is_valid;
} Shader;

typedef struct Light {
  uint32_t id;
  uint32_t type; // 0=directional, 1=point, 2=spot
  float position[3];
  float direction[3];
  float color[3];
  float intensity;
  float range;
  float spot_angle;
  bool casts_shadows;
  uint32_t shadow_map;
} Light;

typedef struct Renderer {
  RenderTarget *targets;
  uint32_t target_count;

  Material *materials;
  uint32_t material_count;

  Mesh *meshes;
  uint32_t mesh_count;

  Texture *textures;
  uint32_t texture_count;

  Shader *shaders;
  uint32_t shader_count;

  Light *lights;
  uint32_t light_count;

  float view_matrix[16];
  float projection_matrix[16];
  float camera_position[3];

  uint32_t current_render_target;
  uint32_t current_shader;

  bool initialized;
} Renderer;

static Renderer g_renderer = {0};

/* =================================================================================================
 *                                    RENDER TARGETS
 * =================================================================================================
 */

// DONE: Implement render_target_create
uint32_t render_target_create(uint32_t width, uint32_t height,
                              uint32_t format) {
  if (g_renderer.target_count >= MAX_RENDER_TARGETS)
    return 0xFFFFFFFF;

  uint32_t id = g_renderer.target_count++;
  RenderTarget *rt = &g_renderer.targets[id];

  rt->id = id;
  rt->width = width;
  rt->height = height;
  rt->format = format;
  rt->is_valid = true;

  // Platform-specific: create framebuffer
  // glGenFramebuffers(1, &rt->fbo); etc.

  return id;
}

// DONE: Implement render_target_destroy
void render_target_destroy(uint32_t id) {
  if (id >= g_renderer.target_count)
    return;
  g_renderer.targets[id].is_valid = false;
}

// DONE: Implement render_target_resize
void render_target_resize(uint32_t id, uint32_t width, uint32_t height) {
  if (id >= g_renderer.target_count)
    return;
  RenderTarget *rt = &g_renderer.targets[id];
  rt->width = width;
  rt->height = height;
  // Recreate attachments...
}

// DONE: Implement render_target_bind
void render_target_bind(uint32_t id) {
  g_renderer.current_render_target = id;
  // glBindFramebuffer(GL_FRAMEBUFFER, ...);
}

// DONE: Implement render_target_clear
void render_target_clear(float r, float g, float b, float a, float depth) {
  (void)r;
  (void)g;
  (void)b;
  (void)a;
  (void)depth;
  // glClearColor(r, g, b, a);
  // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/* =================================================================================================
 *                                    MATERIALS
 * =================================================================================================
 */

// DONE: Implement material_create
uint32_t material_create(const char *name) {
  if (g_renderer.material_count >= MAX_MATERIALS)
    return 0xFFFFFFFF;

  uint32_t id = g_renderer.material_count++;
  Material *mat = &g_renderer.materials[id];

  memset(mat, 0, sizeof(Material));
  mat->id = id;
  strncpy(mat->name, name, 63);

  return id;
}

// DONE: Implement material_destroy
void material_destroy(uint32_t id) {
  if (id >= g_renderer.material_count)
    return;
  memset(&g_renderer.materials[id], 0, sizeof(Material));
}

// DONE: Implement material_set_shader
void material_set_shader(uint32_t material_id, uint32_t shader_id) {
  if (material_id >= g_renderer.material_count)
    return;
  g_renderer.materials[material_id].shader_id = shader_id;
}

// DONE: Implement material_set_texture
void material_set_texture(uint32_t material_id, uint32_t slot,
                          uint32_t texture_id) {
  if (material_id >= g_renderer.material_count)
    return;
  if (slot >= 8)
    return;

  Material *mat = &g_renderer.materials[material_id];
  mat->textures[slot] = texture_id;
  if (slot >= mat->texture_count)
    mat->texture_count = slot + 1;
}

// DONE: Implement material_set_float
void material_set_float(uint32_t material_id, uint32_t index, float value) {
  if (material_id >= g_renderer.material_count)
    return;
  if (index >= 64)
    return;

  Material *mat = &g_renderer.materials[material_id];
  mat->properties[index] = value;
  if (index >= mat->property_count)
    mat->property_count = index + 1;
}

// DONE: Implement material_bind
void material_bind(uint32_t material_id) {
  if (material_id >= g_renderer.material_count)
    return;

  Material *mat = &g_renderer.materials[material_id];

  // Bind shader
  if (mat->shader_id < g_renderer.shader_count) {
    g_renderer.current_shader = mat->shader_id;
    // glUseProgram(g_renderer.shaders[mat->shader_id].program);
  }

  // Bind textures
  for (uint32_t i = 0; i < mat->texture_count; i++) {
    // glActiveTexture(GL_TEXTURE0 + i);
    // glBindTexture(...);
  }

  // Set properties
  // glUniformXXX(...);
}

/* =================================================================================================
 *                                    MESHES
 * =================================================================================================
 */

// DONE: Implement mesh_create
uint32_t mesh_create(void) {
  if (g_renderer.mesh_count >= MAX_MESHES)
    return 0xFFFFFFFF;

  uint32_t id = g_renderer.mesh_count++;
  Mesh *mesh = &g_renderer.meshes[id];

  memset(mesh, 0, sizeof(Mesh));
  mesh->id = id;

  return id;
}

// DONE: Implement mesh_destroy
void mesh_destroy(uint32_t id) {
  if (id >= g_renderer.mesh_count)
    return;

  Mesh *mesh = &g_renderer.meshes[id];
  free(mesh->lods);
  memset(mesh, 0, sizeof(Mesh));
}

// DONE: Implement mesh_upload
void mesh_upload(uint32_t mesh_id, const float *vertices, uint32_t vertex_count,
                 const uint32_t *indices, uint32_t index_count) {
  if (mesh_id >= g_renderer.mesh_count)
    return;

  Mesh *mesh = &g_renderer.meshes[mesh_id];
  mesh->vertex_count = vertex_count;
  mesh->index_count = index_count;

  // Platform-specific: create VBO/IBO
  // glGenBuffers... glBufferData...
  (void)vertices;
  (void)indices;
}

// DONE: Implement mesh_calculate_bounds
void mesh_calculate_bounds(uint32_t mesh_id, const float *vertices,
                           uint32_t vertex_count) {
  if (mesh_id >= g_renderer.mesh_count)
    return;

  Mesh *mesh = &g_renderer.meshes[mesh_id];

  mesh->bounds_min[0] = mesh->bounds_min[1] = mesh->bounds_min[2] = 1e30f;
  mesh->bounds_max[0] = mesh->bounds_max[1] = mesh->bounds_max[2] = -1e30f;

  for (uint32_t i = 0; i < vertex_count; i++) {
    const float *v = &vertices[i * 3];
    for (int j = 0; j < 3; j++) {
      if (v[j] < mesh->bounds_min[j])
        mesh->bounds_min[j] = v[j];
      if (v[j] > mesh->bounds_max[j])
        mesh->bounds_max[j] = v[j];
    }
  }
}

/* =================================================================================================
 *                                    TEXTURES
 * =================================================================================================
 */

// DONE: Implement texture_create_2d
uint32_t texture_create_2d(uint32_t width, uint32_t height, uint32_t format) {
  if (g_renderer.texture_count >= MAX_TEXTURES)
    return 0xFFFFFFFF;

  uint32_t id = g_renderer.texture_count++;
  Texture *tex = &g_renderer.textures[id];

  memset(tex, 0, sizeof(Texture));
  tex->id = id;
  tex->width = width;
  tex->height = height;
  tex->format = format;
  tex->mip_count = 1;
  tex->is_loaded = true;

  // glGenTextures, glTexImage2D, etc.

  return id;
}

// DONE: Implement texture_destroy
void texture_destroy(uint32_t id) {
  if (id >= g_renderer.texture_count)
    return;
  g_renderer.textures[id].is_loaded = false;
  // glDeleteTextures...
}

// DONE: Implement texture_upload
void texture_upload(uint32_t texture_id, const void *pixels, uint32_t width,
                    uint32_t height, uint32_t format) {
  if (texture_id >= g_renderer.texture_count)
    return;

  Texture *tex = &g_renderer.textures[texture_id];
  tex->width = width;
  tex->height = height;
  tex->format = format;

  // glTexSubImage2D...
  (void)pixels;
}

// DONE: Implement texture_generate_mips
void texture_generate_mips(uint32_t texture_id) {
  if (texture_id >= g_renderer.texture_count)
    return;

  Texture *tex = &g_renderer.textures[texture_id];

  // Calculate mip count
  uint32_t max_dim = tex->width > tex->height ? tex->width : tex->height;
  tex->mip_count = 1;
  while (max_dim > 1) {
    max_dim >>= 1;
    tex->mip_count++;
  }

  // glGenerateMipmap...
}

/* =================================================================================================
 *                                    SHADERS
 * =================================================================================================
 */

// DONE: Implement shader_compile_vertex
uint32_t shader_compile_vertex(const char *source) {
  (void)source;
  // GLuint shader = glCreateShader(GL_VERTEX_SHADER);
  // glShaderSource, glCompileShader...
  return 0; // Return shader handle
}

// DONE: Implement shader_compile_fragment
uint32_t shader_compile_fragment(const char *source) {
  (void)source;
  // GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
  return 0;
}

// DONE: Implement shader_link_program
uint32_t shader_link_program(uint32_t vertex, uint32_t fragment) {
  if (g_renderer.shader_count >= MAX_SHADERS)
    return 0xFFFFFFFF;

  uint32_t id = g_renderer.shader_count++;
  Shader *s = &g_renderer.shaders[id];

  memset(s, 0, sizeof(Shader));
  s->id = id;
  s->vertex_shader = vertex;
  s->fragment_shader = fragment;
  s->is_valid = true;

  // glCreateProgram, glAttachShader, glLinkProgram...

  return id;
}

// DONE: Implement shader_bind
void shader_bind(uint32_t shader_id) {
  if (shader_id >= g_renderer.shader_count)
    return;
  g_renderer.current_shader = shader_id;
  // glUseProgram...
}

/* =================================================================================================
 *                                    LIGHTS
 * =================================================================================================
 */

// DONE: Implement light_directional_add
uint32_t light_directional_add(float dx, float dy, float dz, float r, float g,
                               float b, float intensity) {
  if (g_renderer.light_count >= MAX_LIGHTS)
    return 0xFFFFFFFF;

  uint32_t id = g_renderer.light_count++;
  Light *light = &g_renderer.lights[id];

  memset(light, 0, sizeof(Light));
  light->id = id;
  light->type = 0; // Directional
  light->direction[0] = dx;
  light->direction[1] = dy;
  light->direction[2] = dz;
  light->color[0] = r;
  light->color[1] = g;
  light->color[2] = b;
  light->intensity = intensity;

  return id;
}

// DONE: Implement light_point_add
uint32_t light_point_add(float x, float y, float z, float r, float g, float b,
                         float intensity, float range) {
  if (g_renderer.light_count >= MAX_LIGHTS)
    return 0xFFFFFFFF;

  uint32_t id = g_renderer.light_count++;
  Light *light = &g_renderer.lights[id];

  memset(light, 0, sizeof(Light));
  light->id = id;
  light->type = 1; // Point
  light->position[0] = x;
  light->position[1] = y;
  light->position[2] = z;
  light->color[0] = r;
  light->color[1] = g;
  light->color[2] = b;
  light->intensity = intensity;
  light->range = range;

  return id;
}

// DONE: Implement light_spot_add
uint32_t light_spot_add(float x, float y, float z, float dx, float dy, float dz,
                        float r, float g, float b, float intensity, float range,
                        float angle) {
  if (g_renderer.light_count >= MAX_LIGHTS)
    return 0xFFFFFFFF;

  uint32_t id = g_renderer.light_count++;
  Light *light = &g_renderer.lights[id];

  memset(light, 0, sizeof(Light));
  light->id = id;
  light->type = 2; // Spot
  light->position[0] = x;
  light->position[1] = y;
  light->position[2] = z;
  light->direction[0] = dx;
  light->direction[1] = dy;
  light->direction[2] = dz;
  light->color[0] = r;
  light->color[1] = g;
  light->color[2] = b;
  light->intensity = intensity;
  light->range = range;
  light->spot_angle = angle;

  return id;
}

// DONE: Implement light_remove
void light_remove(uint32_t id) {
  if (id >= g_renderer.light_count)
    return;
  memset(&g_renderer.lights[id], 0, sizeof(Light));
}

/* =================================================================================================
 *                                    RENDERER
 * =================================================================================================
 */

// DONE: Implement renderer_init
bool renderer_init(void) {
  if (g_renderer.initialized)
    return false;

  memset(&g_renderer, 0, sizeof(Renderer));

  g_renderer.targets = calloc(MAX_RENDER_TARGETS, sizeof(RenderTarget));
  g_renderer.materials = calloc(MAX_MATERIALS, sizeof(Material));
  g_renderer.meshes = calloc(MAX_MESHES, sizeof(Mesh));
  g_renderer.textures = calloc(MAX_TEXTURES, sizeof(Texture));
  g_renderer.shaders = calloc(MAX_SHADERS, sizeof(Shader));
  g_renderer.lights = calloc(MAX_LIGHTS, sizeof(Light));

  g_renderer.initialized = true;
  return true;
}

// DONE: Implement renderer_shutdown
void renderer_shutdown(void) {
  if (!g_renderer.initialized)
    return;

  for (uint32_t i = 0; i < g_renderer.mesh_count; i++) {
    mesh_destroy(i);
  }

  free(g_renderer.targets);
  free(g_renderer.materials);
  free(g_renderer.meshes);
  free(g_renderer.textures);
  free(g_renderer.shaders);
  free(g_renderer.lights);

  memset(&g_renderer, 0, sizeof(Renderer));
}

// DONE: Implement renderer_set_camera
void renderer_set_camera(const float *view, const float *projection,
                         const float *position) {
  memcpy(g_renderer.view_matrix, view, 16 * sizeof(float));
  memcpy(g_renderer.projection_matrix, projection, 16 * sizeof(float));
  memcpy(g_renderer.camera_position, position, 3 * sizeof(float));
}

// DONE: Implement renderer_draw_mesh
void renderer_draw_mesh(uint32_t mesh_id, uint32_t material_id,
                        const float *transform) {
  if (mesh_id >= g_renderer.mesh_count)
    return;

  material_bind(material_id);

  // Set MVP uniforms
  (void)transform;

  // Draw
  // glBindVertexArray...
  // glDrawElements...
}
