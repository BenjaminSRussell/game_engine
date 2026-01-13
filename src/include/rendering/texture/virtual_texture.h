#ifndef VIRTUAL_TEXTURE_H
#define VIRTUAL_TEXTURE_H

#include "include/common.h"
#include <stdint.h>

#define VT_PAGE_SIZE 128
#define VT_PAGE_TABLE_SIZE 128 // 128x128 pages = 16384x16384 virtual resolution
#define VT_PHYSICAL_PAGES_X 16
#define VT_PHYSICAL_PAGES_Y 16 // 256 physical pages = 2048x2048 atlas

typedef struct {
  uint8_t physical_x;
  uint8_t physical_y;
  uint8_t mip_level;
  uint8_t status; // 0: Unloaded, 1: Loading, 2: Resident
} VirtualPage;

typedef struct {
  VirtualPage pages[VT_PAGE_TABLE_SIZE][VT_PAGE_TABLE_SIZE];
  uint32_t virtual_width;
  uint32_t virtual_height;
} PageTable;

typedef struct {
  void *gpu_atlas_texture;
  void *gpu_page_table_texture;

  PageTable table;

  uint32_t free_slots[VT_PHYSICAL_PAGES_X * VT_PHYSICAL_PAGES_Y];
  uint32_t free_count;

  // LRU or Priority based eviction
  uint32_t *page_last_used;
} VirtualTextureSystem;

#ifdef __cplusplus
extern "C" {
#endif

VirtualTextureSystem *vt_system_create(void);
void vt_system_destroy(VirtualTextureSystem *sys);

// Request a page to be resident for a given UV and mip
void vt_request_page(VirtualTextureSystem *sys, float u, float v, uint8_t mip);

// Update/Streaming
void vt_update(VirtualTextureSystem *sys);

// GPU Bindings
void *vt_get_page_table_handle(VirtualTextureSystem *sys);
void *vt_get_atlas_handle(VirtualTextureSystem *sys);

#ifdef __cplusplus
}
#endif

#endif
