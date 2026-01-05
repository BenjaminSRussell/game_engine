/*
 * nanite_streamer.h
 * Async streaming of cluster pages for Nanite
 */

#ifndef NANITE_STREAMER_H
#define NANITE_STREAMER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct cluster_page {
    uint32_t id;
    uint32_t cluster_count;
    void* raw_data;
} cluster_page_t;

// Streamer API
void nanite_streamer_init(void);
void nanite_streamer_shutdown(void);

// Request a page to be loaded
void nanite_streamer_request_page(uint32_t page_id);

// Check if a page is ready
bool nanite_streamer_is_page_ready(uint32_t page_id);

// Get the page data (if ready)
cluster_page_t* nanite_streamer_get_page(uint32_t page_id);

// Background update
void nanite_streamer_update(void);

#endif // NANITE_STREAMER_H
