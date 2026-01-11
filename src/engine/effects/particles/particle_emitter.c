#include "particle_emitter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PARTICLE_EMITTERS 512

static effects_particle_emitter_info_t g_emitters[MAX_PARTICLE_EMITTERS];
static uint32_t g_emitter_count = 0;
static bool g_initialized = false;

int effects_particle_emitter_init(void) {
  if (g_initialized)
    return 0;
  memset(g_emitters, 0, sizeof(g_emitters));
  g_emitter_count = 0;
  g_initialized = true;
  return 0;
}

void effects_particle_emitter_shutdown(void) { g_initialized = false; }

int effects_particle_emitter_create(
    effects_particle_emitter_handle_t *out_handle,
    const effects_particle_emitter_desc_t *desc) {
  if (!g_initialized || !out_handle || !desc)
    return -1;

  for (uint32_t i = 0; i < MAX_PARTICLE_EMITTERS; i++) {
    if (!g_emitters[i].initialized) {
      g_emitters[i].id = i;
      g_emitters[i].initialized = true;
      g_emitters[i].particle_count = 0;
      g_emitters[i].active_time = 0.0f;

      out_handle->id = i;
      g_emitter_count++;
      return 0;
    }
  }
  return -2;
}

void effects_particle_emitter_destroy(
    effects_particle_emitter_handle_t handle) {
  if (!g_initialized || handle.id >= MAX_PARTICLE_EMITTERS)
    return;
  if (g_emitters[handle.id].initialized) {
    g_emitters[handle.id].initialized = false;
    g_emitter_count--;
  }
}

bool effects_particle_emitter_is_valid(
    effects_particle_emitter_handle_t handle) {
  return g_initialized && handle.id < MAX_PARTICLE_EMITTERS &&
         g_emitters[handle.id].initialized;
}

int effects_particle_emitter_get_info(
    effects_particle_emitter_handle_t handle,
    effects_particle_emitter_info_t *out_info) {
  if (!effects_particle_emitter_is_valid(handle) || !out_info)
    return -1;
  *out_info = g_emitters[handle.id];
  return 0;
}

int effects_particle_emitter_update(effects_particle_emitter_handle_t handle,
                                    const void *data, size_t size) {
  if (!effects_particle_emitter_is_valid(handle))
    return -1;
  // Update logic for emitter parameters
  return 0;
}

void effects_particle_emitter_mark_dirty(
    effects_particle_emitter_handle_t handle) {}
int effects_particle_emitter_process_pending(void) { return 0; }

uint32_t effects_particle_emitter_get_count(void) { return g_emitter_count; }

size_t effects_particle_emitter_get_memory_usage(void) {
  return sizeof(g_emitters);
}

void effects_particle_emitter_debug_print(void) {
  printf("Active Particle Emitters: %u\n", g_emitter_count);
}
