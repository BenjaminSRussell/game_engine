#include "environment/landscape/terrain/landscape/vegetation/grass_renderer.h"

#include <stdio.h>
#include <string.h>

/* CPU grass instances only. No GPU buffer, GLFW, or Vulkan. */

enum { GRASS_MAX_RENDERERS = 8, GRASS_MAX_INSTANCES = 64 };

typedef struct grass_instance {
  Vec3 position;
} grass_instance_t;

typedef struct grass_renderer_slot {
  bool alive;
  bool dirty;
  landscape_grass_renderer_desc_t desc;
  grass_instance_t instances[GRASS_MAX_INSTANCES];
  uint32_t instance_count;
  uint32_t visible_instances;
  float wind_time;
} grass_renderer_slot_t;

static bool g_ready = false;
static grass_renderer_slot_t g_slots[GRASS_MAX_RENDERERS];

static int bad_nonnegative(float v) {
  if (v != v) {
    return 1;
  }
  if (v < 0.0f || v > 1.0e8f) {
    return 1;
  }
  return 0;
}

static uint32_t instance_budget(float density, uint32_t chunk_size) {
  double area;
  double wanted;

  if (!(density > 0.0f) || chunk_size == 0) {
    return 0;
  }
  area = (double)chunk_size * (double)chunk_size;
  wanted = area * (double)density;
  if (wanted < 1.0) {
    return 0;
  }
  if (wanted > (double)GRASS_MAX_INSTANCES) {
    return (uint32_t)GRASS_MAX_INSTANCES;
  }
  return (uint32_t)wanted;
}

static void place_instances(grass_renderer_slot_t *slot) {
  uint32_t count = instance_budget(slot->desc.density, slot->desc.chunk_size);
  uint32_t side = 1;
  uint32_t placed = 0;
  uint32_t x;
  uint32_t z;
  float span;

  slot->instance_count = 0;
  slot->visible_instances = 0;
  if (count == 0) {
    return;
  }
  while (side * side < count) {
    side++;
  }
  span = (float)slot->desc.chunk_size;
  for (z = 0; z < side && placed < count; z++) {
    for (x = 0; x < side && placed < count; x++) {
      grass_instance_t *blade = &slot->instances[placed];
      memset(&blade->position, 0, sizeof(blade->position));
      /* Cell centers of a side x side grid on the chunk, origin at the center. */
      blade->position.x = (span * ((float)x / (float)side)) - (span * 0.5f);
      blade->position.z = (span * ((float)z / (float)side)) - (span * 0.5f);
      placed++;
    }
  }
  slot->instance_count = placed;
  slot->visible_instances = placed;
}

static grass_renderer_slot_t *live_slot(landscape_grass_renderer_handle_t handle) {
  uint32_t index;

  if (!g_ready || handle.id == 0 || handle.id > (uint32_t)GRASS_MAX_RENDERERS) {
    return NULL;
  }
  index = handle.id - 1;
  if (!g_slots[index].alive) {
    return NULL;
  }
  return &g_slots[index];
}

int landscape_grass_renderer_init(void) {
  if (g_ready) {
    return 0;
  }
  memset(g_slots, 0, sizeof(g_slots));
  g_ready = true;
  return 0;
}

void landscape_grass_renderer_shutdown(void) {
  memset(g_slots, 0, sizeof(g_slots));
  g_ready = false;
}

int landscape_grass_renderer_create(landscape_grass_renderer_handle_t *out_handle,
                                    const landscape_grass_renderer_desc_t *desc) {
  int free_index = -1;
  int i;

  if (!out_handle) {
    return -1;
  }
  out_handle->id = 0;
  if (!g_ready || !desc) {
    return -1;
  }
  if (bad_nonnegative(desc->density) || bad_nonnegative(desc->cull_distance)) {
    return -1;
  }
  for (i = 0; i < GRASS_MAX_RENDERERS; i++) {
    if (!g_slots[i].alive) {
      free_index = i;
      break;
    }
  }
  if (free_index < 0) {
    return -1;
  }
  memset(&g_slots[free_index], 0, sizeof(g_slots[free_index]));
  g_slots[free_index].alive = true;
  g_slots[free_index].desc = *desc;
  place_instances(&g_slots[free_index]);
  out_handle->id = (uint32_t)free_index + 1;
  return 0;
}

void landscape_grass_renderer_destroy(landscape_grass_renderer_handle_t handle) {
  grass_renderer_slot_t *slot = live_slot(handle);
  if (!slot) {
    return;
  }
  memset(slot, 0, sizeof(*slot));
}

int landscape_grass_renderer_update(landscape_grass_renderer_handle_t handle,
                                    Vec3 camera_pos, float delta_time) {
  grass_renderer_slot_t *slot = live_slot(handle);
  uint32_t visible = 0;
  uint32_t i;
  float cull;
  float cull_sq;
  float stiffness;

  if (!slot || bad_nonnegative(delta_time)) {
    return -1;
  }
  stiffness = slot->desc.blade_config.wind_stiffness;
  if (stiffness < 0.0f || stiffness != stiffness) {
    stiffness = 0.0f;
  }
  slot->wind_time += delta_time * (1.0f + stiffness);
  cull = slot->desc.cull_distance;
  cull_sq = cull * cull;
  for (i = 0; i < slot->instance_count; i++) {
    float dx = slot->instances[i].position.x - camera_pos.x;
    float dy = slot->instances[i].position.y - camera_pos.y;
    float dz = slot->instances[i].position.z - camera_pos.z;
    float d2 = (dx * dx) + (dy * dy) + (dz * dz);
    if (d2 <= cull_sq) {
      visible++;
    }
  }
  slot->visible_instances = visible;
  return 0;
}

void landscape_grass_renderer_render(landscape_grass_renderer_handle_t handle,
                                     const void *render_context) {
  (void)render_context;
  (void)live_slot(handle);
}

bool landscape_grass_renderer_is_valid(landscape_grass_renderer_handle_t handle) {
  return live_slot(handle) != NULL;
}

int landscape_grass_renderer_get_info(landscape_grass_renderer_handle_t handle,
                                      landscape_grass_renderer_info_t *out_info) {
  grass_renderer_slot_t *slot = live_slot(handle);
  if (!slot || !out_info) {
    return -1;
  }
  out_info->id = handle.id;
  out_info->flags = slot->desc.flags;
  out_info->initialized = true;
  out_info->instance_count = slot->instance_count;
  out_info->visible_instances = slot->visible_instances;
  out_info->buffer_memory = (size_t)slot->instance_count * sizeof(grass_instance_t);
  return 0;
}

void landscape_grass_renderer_mark_dirty(landscape_grass_renderer_handle_t handle) {
  grass_renderer_slot_t *slot = live_slot(handle);
  if (slot) {
    slot->dirty = true;
  }
}

int landscape_grass_renderer_process_pending(void) {
  int processed = 0;
  int i;

  if (!g_ready) {
    return -1;
  }
  for (i = 0; i < GRASS_MAX_RENDERERS; i++) {
    if (g_slots[i].alive && g_slots[i].dirty) {
      place_instances(&g_slots[i]);
      g_slots[i].dirty = false;
      g_slots[i].wind_time = 0.0f;
      processed++;
    }
  }
  return processed;
}

uint32_t landscape_grass_renderer_get_count(void) {
  uint32_t count = 0;
  int i;

  if (!g_ready) {
    return 0;
  }
  for (i = 0; i < GRASS_MAX_RENDERERS; i++) {
    if (g_slots[i].alive) {
      count++;
    }
  }
  return count;
}

size_t landscape_grass_renderer_get_memory_usage(void) {
  size_t bytes = 0;
  int i;

  if (!g_ready) {
    return 0;
  }
  for (i = 0; i < GRASS_MAX_RENDERERS; i++) {
    if (g_slots[i].alive) {
      bytes += (size_t)g_slots[i].instance_count * sizeof(grass_instance_t);
    }
  }
  return bytes;
}

void landscape_grass_renderer_debug_print(void) {
  printf("grass renderers: %u memory: %zu\n", landscape_grass_renderer_get_count(),
         landscape_grass_renderer_get_memory_usage());
}
