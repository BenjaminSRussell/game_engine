/*
 * cloth_shading.c
 * Cloth shading implementation
 *
 * Part of the Cloth System subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements physically based shading for cloth using Sheen and Subsurface
 * Scattering
 */

#include "character/cloth/rendering_cloth/cloth_shading.h"
#include <include/math/math_all.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================
 */

#define CLOTH_SHADING_MAX_COUNT 32
#define CLOTH_SHADING_DEFAULT_CAPACITY 8
#define PI 3.14159265359f

/* ============================================================================
 * MATH TYPES
 * ============================================================================
 */

#include "math/vec3.h"

/* ============================================================================
 * CLOTH SHADING TYPES
 * ============================================================================
 */

typedef struct cloth_material {
  vec3_t albedo;
  float roughness;
  float metalness;

  // Cloth specific
  vec3_t sheen_color;
  float sheen_roughness;

  vec3_t subsurface_color;
  float subsurface_radius;
  float subsurface_power;

  // Texture maps (IDs)
  uint32_t albedo_map;
  uint32_t normal_map;
  uint32_t roughness_map;
  uint32_t sheen_map;
} cloth_material_t;

typedef struct cloth_system_cloth_shading_internal {
  uint32_t id;
  uint32_t flags;
  cloth_material_t material;
  bool initialized;
  bool dirty;
  uint64_t frame_updated;
} cloth_system_cloth_shading_internal_t;

typedef struct cloth_system_cloth_shading_context {
  cloth_system_cloth_shading_internal_t *items;
  uint32_t count;
  uint32_t capacity;
  bool initialized;
} cloth_system_cloth_shading_context_t;

static cloth_system_cloth_shading_context_t g_cloth_shading_ctx = {0};

/* ============================================================================
 * SHADING HELPERS (CPU Reference / Precomputation)
 * ============================================================================
 */

/* Charlie Sheen BRDF for cloth */
static float d_charlie(float roughness, float ndoth) {
  float inv_alpha = 1.0f / roughness;
  float cos2h = ndoth * ndoth;
  float sin2h = 1.0f - cos2h;
  if (sin2h <= 0.0f)
    return 0.0f;
  return (2.0f + inv_alpha) * powf(sin2h, inv_alpha * 0.5f) / (2.0f * PI);
}

static float v_ashikhmin(float ndotl, float ndotv) {
  return 1.0f / (4.0f * (ndotl + ndotv - ndotl * ndotv));
}

// These functions would be implemented in GLSL/HLSL,
// here we just manage the parameters.

/* ============================================================================
 * PUBLIC API
 * ============================================================================
 */

int cloth_system_cloth_shading_init(void) {
  if (g_cloth_shading_ctx.initialized) {
    return 0;
  }

  g_cloth_shading_ctx.capacity = CLOTH_SHADING_DEFAULT_CAPACITY;
  g_cloth_shading_ctx.items =
      calloc(g_cloth_shading_ctx.capacity,
             sizeof(cloth_system_cloth_shading_internal_t));
  if (!g_cloth_shading_ctx.items) {
    return -1;
  }

  g_cloth_shading_ctx.count = 0;
  g_cloth_shading_ctx.initialized = true;

  return 0;
}

void cloth_system_cloth_shading_shutdown(void) {
  if (!g_cloth_shading_ctx.initialized) {
    return;
  }

  free(g_cloth_shading_ctx.items);
  g_cloth_shading_ctx.items = NULL;
  g_cloth_shading_ctx.count = 0;
  g_cloth_shading_ctx.capacity = 0;
  g_cloth_shading_ctx.initialized = false;
}

int cloth_system_cloth_shading_create(
    cloth_system_cloth_shading_handle_t *out_handle,
    const cloth_system_cloth_shading_desc_t *desc) {
  if (!out_handle || !desc) {
    return -1;
  }

  if (!g_cloth_shading_ctx.initialized) {
    return -2;
  }

  if (g_cloth_shading_ctx.count >= g_cloth_shading_ctx.capacity) {
    return -3;
  }

  uint32_t index = g_cloth_shading_ctx.count++;
  cloth_system_cloth_shading_internal_t *item =
      &g_cloth_shading_ctx.items[index];

  item->id = index;
  item->flags = desc->flags;

  // Default velvet-like cloth
  item->material.albedo = (vec3_t){0.5f, 0.1f, 0.1f};
  item->material.roughness = 0.8f;
  item->material.metalness = 0.0f;
  item->material.sheen_color = (vec3_t){1.0f, 0.8f, 0.8f};
  item->material.sheen_roughness = 0.5f;
  item->material.subsurface_color = (vec3_t){0.6f, 0.2f, 0.2f};
  item->material.subsurface_radius = 0.01f;
  item->material.subsurface_power = 1.0f;

  item->initialized = true;
  item->dirty = true;
  item->frame_updated = 0;

  out_handle->id = index;
  return 0;
}

void cloth_system_cloth_shading_destroy(
    cloth_system_cloth_shading_handle_t handle) {
  if (handle.id >= g_cloth_shading_ctx.count) {
    return;
  }

  g_cloth_shading_ctx.items[handle.id].initialized = false;
}

int cloth_system_cloth_shading_set_material(
    cloth_system_cloth_shading_handle_t handle, vec3_t albedo, float roughness,
    vec3_t sheen) {
  if (handle.id >= g_cloth_shading_ctx.count) {
    return -1;
  }

  cloth_system_cloth_shading_internal_t *item =
      &g_cloth_shading_ctx.items[handle.id];
  item->material.albedo = albedo;
  item->material.roughness = roughness;
  item->material.sheen_color = sheen;
  item->dirty = true;

  return 0;
}

int cloth_system_cloth_shading_update(
    cloth_system_cloth_shading_handle_t handle, const void *data, size_t size) {
  if (handle.id >= g_cloth_shading_ctx.count) {
    return -1;
  }

  g_cloth_shading_ctx.items[handle.id].dirty = true;
  return 0;
}

bool cloth_system_cloth_shading_is_valid(
    cloth_system_cloth_shading_handle_t handle) {
  if (handle.id >= g_cloth_shading_ctx.count) {
    return false;
  }
  return g_cloth_shading_ctx.items[handle.id].initialized;
}

int cloth_system_cloth_shading_get_info(
    cloth_system_cloth_shading_handle_t handle,
    cloth_system_cloth_shading_info_t *out_info) {
  if (!out_info) {
    return -1;
  }

  if (handle.id >= g_cloth_shading_ctx.count) {
    return -2;
  }

  const cloth_system_cloth_shading_internal_t *item =
      &g_cloth_shading_ctx.items[handle.id];
  out_info->id = item->id;
  out_info->flags = item->flags;
  out_info->initialized = item->initialized;

  return 0;
}

void cloth_system_cloth_shading_mark_dirty(
    cloth_system_cloth_shading_handle_t handle) {
  if (handle.id < g_cloth_shading_ctx.count) {
    g_cloth_shading_ctx.items[handle.id].dirty = true;
  }
}

int cloth_system_cloth_shading_process_pending(void) {
  int processed = 0;
  for (uint32_t i = 0; i < g_cloth_shading_ctx.count; i++) {
    cloth_system_cloth_shading_internal_t *item = &g_cloth_shading_ctx.items[i];
    if (item->initialized && item->dirty) {
      // Update GPU buffers in real implementation
      item->dirty = false;
      processed++;
    }
  }

  return processed;
}

uint32_t cloth_system_cloth_shading_get_count(void) {
  return g_cloth_shading_ctx.count;
}

size_t cloth_system_cloth_shading_get_memory_usage(void) {
  size_t total = sizeof(g_cloth_shading_ctx);
  total += g_cloth_shading_ctx.capacity *
           sizeof(cloth_system_cloth_shading_internal_t);
  return total;
}

void cloth_system_cloth_shading_debug_print(void) {
  // Debug output
}

/* End of cloth_shading.c */
