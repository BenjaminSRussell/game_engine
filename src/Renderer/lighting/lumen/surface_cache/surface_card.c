#include "surface_card.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SURFACE_CARDS 1024

static lumen_surface_card_t *g_surface_cards = NULL;
static uint32_t g_card_count = 0;
static bool g_initialized = false;

int lumen_surface_card_init(void) {
  if (g_initialized)
    return 0;

  g_surface_cards = (lumen_surface_card_t *)calloc(
      MAX_SURFACE_CARDS, sizeof(lumen_surface_card_t));
  if (!g_surface_cards)
    return -1;

  g_card_count = 0;
  g_initialized = true;
  return 0;
}

void lumen_surface_card_shutdown(void) {
  if (!g_initialized)
    return;
  free(g_surface_cards);
  g_surface_cards = NULL;
  g_card_count = 0;
  g_initialized = false;
}

int lumen_surface_card_create(lumen_surface_card_handle_t *out_handle,
                              const lumen_surface_card_desc_t *desc) {
  if (!g_initialized || !out_handle || !desc)
    return -1;

  // Simple linear search for inactive slot
  for (uint32_t i = 0; i < MAX_SURFACE_CARDS; i++) {
    if (!g_surface_cards[i].active) {
      lumen_surface_card_t *card = &g_surface_cards[i];
      card->position = desc->position;
      card->normal = desc->normal;
      card->extent = desc->extent;
      card->mesh_id = desc->mesh_id;
      card->active = true;

      out_handle->id = i;
      g_card_count++;
      return 0;
    }
  }

  return -2; // Full
}

void lumen_surface_card_destroy(lumen_surface_card_handle_t handle) {
  if (!g_initialized || handle.id >= MAX_SURFACE_CARDS)
    return;
  if (g_surface_cards[handle.id].active) {
    g_surface_cards[handle.id].active = false;
    g_card_count--;
  }
}

int lumen_surface_card_generate_for_mesh(
    uint32_t mesh_id, lumen_surface_card_handle_t *out_handles,
    uint32_t *out_count) {
  if (!g_initialized || !out_handles || !out_count)
    return -1;

  // Simplified: Generate 6 cards representing the AABB of the mesh
  // In a real implementation, this would involve mesh analysis
  *out_count = 6;
  for (int i = 0; i < 6; i++) {
    lumen_surface_card_desc_t desc = {0};
    desc.mesh_id = mesh_id;
    desc.position = (vec3_t){0, 0, 0}; // Placeholder
    desc.extent = (vec3_t){1, 1, 1};   // Placeholder

    // Face normals: +X, -X, +Y, -Y, +Z, -Z
    float normals[6][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                           {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
    desc.normal = (vec3_t){normals[i][0], normals[i][1], normals[i][2]};

    lumen_surface_card_create(&out_handles[i], &desc);
  }

  return 0;
}

bool lumen_surface_card_is_valid(lumen_surface_card_handle_t handle) {
  return g_initialized && handle.id < MAX_SURFACE_CARDS &&
         g_surface_cards[handle.id].active;
}

int lumen_surface_card_get_info(lumen_surface_card_handle_t handle,
                                lumen_surface_card_info_t *out_info) {
  if (!lumen_surface_card_is_valid(handle) || !out_info)
    return -1;

  lumen_surface_card_t *card = &g_surface_cards[handle.id];
  out_info->id = handle.id;
  out_info->position = card->position;
  out_info->active = card->active;
  out_info->initialized = true;
  return 0;
}

uint32_t lumen_surface_card_get_count(void) { return g_card_count; }

size_t lumen_surface_card_get_memory_usage(void) {
  return MAX_SURFACE_CARDS * sizeof(lumen_surface_card_t);
}

// Stubs for remaining operations
int lumen_surface_card_update(lumen_surface_card_handle_t handle,
                              const void *data, size_t size) {
  return 0;
}
void lumen_surface_card_mark_dirty(lumen_surface_card_handle_t handle) {}
int lumen_surface_card_process_pending(void) { return 0; }
void lumen_surface_card_debug_print(void) {
  printf("Lumen Surface Cards: %u active\n", g_card_count);
}
