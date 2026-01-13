/*
 * mipmap_generation.c
 * Mipmap generation implementation
 */

#include "assets/textures/sampling/mipmap_generation.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static uint32_t average_colors(uint32_t c1, uint32_t c2, uint32_t c3, uint32_t c4) {
    uint32_t r = ((c1 >> 0) & 0xFF) + ((c2 >> 0) & 0xFF) + ((c3 >> 0) & 0xFF) + ((c4 >> 0) & 0xFF);
    uint32_t g = ((c1 >> 8) & 0xFF) + ((c2 >> 8) & 0xFF) + ((c3 >> 8) & 0xFF) + ((c4 >> 8) & 0xFF);
    uint32_t b = ((c1 >> 16) & 0xFF) + ((c2 >> 16) & 0xFF) + ((c3 >> 16) & 0xFF) + ((c4 >> 16) & 0xFF);
    uint32_t a = ((c1 >> 24) & 0xFF) + ((c2 >> 24) & 0xFF) + ((c3 >> 24) & 0xFF) + ((c4 >> 24) & 0xFF);
    
    return ((r / 4) << 0) | ((g / 4) << 8) | ((b / 4) << 16) | ((a / 4) << 24);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int mipmap_downsample_box(const uint32_t* src, uint32_t src_w, uint32_t src_h, uint32_t* dst) {
    if (!src || !dst) return -1;
    
    uint32_t dst_w = src_w / 2;
    uint32_t dst_h = src_h / 2;
    if (dst_w == 0) dst_w = 1;
    if (dst_h == 0) dst_h = 1;
    
    for (uint32_t y = 0; y < dst_h; y++) {
        for (uint32_t x = 0; x < dst_w; x++) {
            uint32_t c1 = src[(y * 2) * src_w + (x * 2)];
            uint32_t c2 = src[(y * 2) * src_w + (x * 2 + 1)];
            uint32_t c3 = src[(y * 2 + 1) * src_w + (x * 2)];
            uint32_t c4 = src[(y * 2 + 1) * src_w + (x * 2 + 1)];
            
            dst[y * dst_w + x] = average_colors(c1, c2, c3, c4);
        }
    }
    
    return 0;
}

int mipmap_generate_chain(const void* src_data, uint32_t width, uint32_t height, 
                         void** out_mips, uint32_t* out_mip_count) {
    if (!src_data || !out_mips || !out_mip_count) return -1;
    
    uint32_t mips = 1;
    uint32_t w = width, h = height;
    while (w > 1 || h > 1) {
        w /= 2; h /= 2;
        if (w == 0) w = 1;
        if (h == 0) h = 1;
        mips++;
    }
    
    // Simplified: we'll return an array of pointers
    uint32_t** chain = malloc(mips * sizeof(uint32_t*));
    if (!chain) return -2;
    
    chain[0] = malloc(width * height * sizeof(uint32_t));
    memcpy(chain[0], src_data, width * height * sizeof(uint32_t));
    
    w = width; h = height;
    for (uint32_t i = 1; i < mips; i++) {
        uint32_t prev_w = w, prev_h = h;
        w /= 2; h /= 2;
        if (w == 0) w = 1;
        if (h == 0) h = 1;
        
        chain[i] = malloc(w * h * sizeof(uint32_t));
        mipmap_downsample_box(chain[i-1], prev_w, prev_h, chain[i]);
    }
    
    *out_mips = chain;
    *out_mip_count = mips;
    return 0;
}

void mipmap_free_chain(void* mips, uint32_t mip_count) {
    if (!mips) return;
    uint32_t** chain = (uint32_t**)mips;
    for (uint32_t i = 0; i < mip_count; i++) {
        free(chain[i]);
    }
    free(chain);
}

/* Original stub compatibility */
int texture_mipmap_generation_init(void) {
    return 0;
}

void texture_mipmap_generation_shutdown(void) {
}

