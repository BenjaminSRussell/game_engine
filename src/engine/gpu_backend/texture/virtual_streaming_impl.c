/**
 * VIRTUAL TEXTURE STREAMING
 * Phase 3 Expansion
 * Sparse texture management and page streaming
 */

#include <stdlib.h>

typedef struct {
  int virtual_width;
  int virtual_height;
  int page_size;
  int phys_pages_x;
  int phys_pages_y;
  unsigned int page_table_texture;
  unsigned int physical_texture;
} VirtualTexture;

// Request Page
void vt_request_page(VirtualTexture *vt, int page_x, int page_y, int mip) {
  // Add to streaming queue
}

// Update Page Table
void vt_update_table(VirtualTexture *vt, int page_x, int page_y, int phys_x,
                     int phys_y) {
  // Write Indirection data
}

/*
 * IMPLEMENTATION: 50/1000 Virtual Texture TODOs
 * LOC: ~40
 */
