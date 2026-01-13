// src/engine/rendering/texture/virtual_streaming_impl.c
#include "include/core/logger.h"
#include "include/rendering/texture/virtual_texture.h"
#include <stdlib.h>
#include <string.h>

VirtualTextureSystem *vt_system_create(void) {
  VirtualTextureSystem *sys =
      (VirtualTextureSystem *)calloc(1, sizeof(VirtualTextureSystem));
  if (!sys)
    return NULL;

  sys->free_count = VT_PHYSICAL_PAGES_X * VT_PHYSICAL_PAGES_Y;
  for (uint32_t i = 0; i < sys->free_count; i++) {
    sys->free_slots[i] = i;
  }

  sys->page_last_used = (uint32_t *)calloc(sys->free_count, sizeof(uint32_t));

  LOG_INFO("Virtual Texture System initialized: %dx%d virtual resolution (%d "
           "MB cache)",
           VT_PAGE_TABLE_SIZE * VT_PAGE_SIZE, VT_PAGE_TABLE_SIZE * VT_PAGE_SIZE,
           (sys->free_count * VT_PAGE_SIZE * VT_PAGE_SIZE * 4) / (1024 * 1024));

  return sys;
}

void vt_system_destroy(VirtualTextureSystem *sys) {
  if (!sys)
    return;
  free(sys->page_last_used);
  free(sys);
}

void vt_request_page(VirtualTextureSystem *sys, float u, float v, uint8_t mip) {
  if (!sys)
    return;

  uint32_t px = (uint32_t)(u * VT_PAGE_TABLE_SIZE) % VT_PAGE_TABLE_SIZE;
  uint32_t py = (uint32_t)(v * VT_PAGE_TABLE_SIZE) % VT_PAGE_TABLE_SIZE;

  VirtualPage *page = &sys->table.pages[px][py];

  if (page->status == 2) {
    // Already resident, update timestamp
    uint32_t slot = page->physical_y * VT_PHYSICAL_PAGES_X + page->physical_x;
    sys->page_last_used[slot]++; // Simplified timestamp
    return;
  }

  if (page->status == 1)
    return; // Already loading

  // If no free slots, would need to evict (LRU)
  if (sys->free_count > 0) {
    uint32_t slot_idx = sys->free_slots[--sys->free_count];
    page->physical_x = slot_idx % VT_PHYSICAL_PAGES_X;
    page->physical_y = slot_idx / VT_PHYSICAL_PAGES_X;
    page->status = 1; // Mark as loading

    LOG_INFO("VT: Loading page (%d, %d) into physical slot %d", px, py,
             slot_idx);

    // In real system, trigger async IO load
    page->status = 2; // Immediate resident for simulation
  }
}

void vt_update(VirtualTextureSystem *sys) {
  if (!sys)
    return;
  // Perform periodically:
  // - Check finished IO requests
  // - GPU upload of new pages
  // - Update indirection texture (Page Table)
}

void *vt_get_page_table_handle(VirtualTextureSystem *sys) {
  return sys ? sys->gpu_page_table_texture : NULL;
}

void *vt_get_atlas_handle(VirtualTextureSystem *sys) {
  return sys ? sys->gpu_atlas_texture : NULL;
}
