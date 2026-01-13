/*
 * texture_streamer.h
 * Texture streaming system
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_TEXTURE_STREAMER_H
#define TEXTURE_TEXTURE_STREAMER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MAX_STREAMING_REQUESTS 1024
#define DEFAULT_STREAMING_BUDGET (512 * 1024 * 1024) // 512MB

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_entry {
    uint32_t handle;           // Resource handle
    uint32_t current_mip;      // Currently resident mip level
    uint32_t target_mip;       // Desired mip level based on priority
    uint32_t max_mips;         // Total available mips
    float screen_coverage;     // Last calculated screen coverage
    float priority;            // Final priority score
    bool is_streaming;         // True if currently loading/unloading
} texture_entry_t;

typedef struct stream_request {
    uint32_t texture_index;
    uint32_t target_mip;
    float priority;
} stream_request_t;

typedef struct priority_queue {
    stream_request_t* requests;
    uint32_t count;
    uint32_t capacity;
} priority_queue_t;

typedef struct texture_streamer {
    texture_entry_t* textures;
    uint32_t texture_count;
    uint32_t texture_capacity;
    
    priority_queue_t request_queue;
    
    uint32_t memory_budget;
    uint32_t memory_used;
    
    bool initialized;
} texture_streamer_t;

typedef struct texture_texture_streamer_handle {
    uint32_t id;
} texture_texture_streamer_handle_t;

typedef struct texture_texture_streamer_desc {
    uint32_t memory_budget;
    uint32_t max_textures;
} texture_texture_streamer_desc_t;

typedef struct texture_texture_streamer_info {
    uint32_t texture_count;
    uint32_t memory_budget;
    uint32_t memory_used;
    uint32_t pending_requests;
} texture_texture_streamer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Global instance access (if using singleton pattern) */
texture_streamer_t* texture_streamer_get_global(void);

/* Initialization & Cleanup */
int texture_streamer_init(texture_streamer_t* streamer, const texture_texture_streamer_desc_t* desc);
void texture_streamer_shutdown(texture_streamer_t* streamer);

/* Texture Registration */
int texture_streamer_register_texture(texture_streamer_t* streamer, uint32_t handle, uint32_t max_mips);
void texture_streamer_unregister_texture(texture_streamer_t* streamer, uint32_t handle);

/* Streaming Logic */
void texture_streamer_update(texture_streamer_t* streamer, void* camera);
void texture_streamer_queue_request(texture_streamer_t* streamer, uint32_t texture_index, uint32_t target_mip, float priority);
void texture_streamer_process_requests(texture_streamer_t* streamer);

/* Utilities */
float texture_streamer_calculate_screen_coverage(uint32_t handle, void* camera);
uint32_t texture_streamer_calculate_target_mip(float screen_coverage, uint32_t max_mips);
int texture_streamer_load_mip(uint32_t handle, uint32_t mip);
int texture_streamer_unload_mip(uint32_t handle, uint32_t mip);

/* Statistics */
int texture_streamer_get_info(const texture_streamer_t* streamer, texture_texture_streamer_info_t* out_info);
void texture_streamer_debug_print(const texture_streamer_t* streamer);

/* Core API compatibility (original stub functions) */
int texture_texture_streamer_init(void);
void texture_texture_streamer_shutdown(void);
int texture_texture_streamer_create(texture_texture_streamer_handle_t* out_handle, const texture_texture_streamer_desc_t* desc);
void texture_texture_streamer_destroy(texture_texture_streamer_handle_t handle);
int texture_texture_streamer_process_pending(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_TEXTURE_STREAMER_H */

