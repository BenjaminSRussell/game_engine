/*
 * page_table.c
 * Virtual texture page table management implementation
 */

#include "page_table.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int page_table_init(page_table_t* pt, uint32_t width, uint32_t height) {
    if (!pt) return -1;
    
    pt->width = width;
    pt->height = height;
    pt->entries = calloc(width * height, sizeof(page_entry_t));
    if (!pt->entries) return -2;
    
    // Initialize with "invalid" or default mips
    for (uint32_t i = 0; i < width * height; i++) {
        pt->entries[i].mip_level = 0xFF; // Invalid/Not resident
    }
    
    pt->gpu_buffer_handle = 0; // Placeholder
    
    return 0;
}

void page_table_shutdown(page_table_t* pt) {
    if (!pt) return;
    if (pt->entries) free(pt->entries);
    memset(pt, 0, sizeof(page_table_t));
}

void page_table_map(page_table_t* pt, uint32_t virtual_x, uint32_t virtual_y, uint32_t physical_x, uint32_t physical_y, uint32_t mip) {
    if (!pt || virtual_x >= pt->width || virtual_y >= pt->height) return;
    
    uint32_t index = virtual_y * pt->width + virtual_x;
    page_entry_t* entry = &pt->entries[index];
    
    entry->physical_x = (uint16_t)physical_x;
    entry->physical_y = (uint16_t)physical_y;
    entry->mip_level = (uint8_t)mip;
    entry->flags = 1; // Active/Resident
}

void page_table_unmap(page_table_t* pt, uint32_t virtual_x, uint32_t virtual_y) {
    if (!pt || virtual_x >= pt->width || virtual_y >= pt->height) return;
    
    uint32_t index = virtual_y * pt->width + virtual_x;
    pt->entries[index].mip_level = 0xFF;
    pt->entries[index].flags = 0;
}

void page_table_update_gpu(page_table_t* pt) {
    if (!pt || pt->gpu_buffer_handle == 0) return;
    
    // Placeholder for GPU buffer upload (e.g., Vulkan buffer update)
    // upload_to_gpu(pt->gpu_buffer_handle, pt->entries, pt->width * pt->height * sizeof(page_entry_t));
}

/* Original stub compatibility */
int texture_page_table_init(void) {
    return 0;
}

void texture_page_table_shutdown(void) {
}

