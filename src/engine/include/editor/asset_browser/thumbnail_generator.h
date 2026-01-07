#ifndef EDITOR_ASSET_BROWSER_THUMBNAIL_GENERATOR_H
#define EDITOR_ASSET_BROWSER_THUMBNAIL_GENERATOR_H

#include "core/core.h"
#include "rendering/core/texture.h"
#include "include/rendering/material.h"
#include "core/sync/thread_pool.h"

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
//                        ASYNC THUMBNAIL GENERATOR - AGENT_EDITOR_2
// =================================================================================================

// Thumbnail types
typedef enum {
    THUMBNAIL_TYPE_TEXTURE,
    THUMBNAIL_TYPE_MODEL,
    THUMBNAIL_TYPE_MATERIAL,
    THUMBNAIL_TYPE_AUDIO,
    THUMBNAIL_TYPE_SCENE,
    THUMBNAIL_TYPE_PREFAB
} ThumbnailType;

// Thumbnail quality levels
typedef enum {
    THUMBNAIL_QUALITY_LOW,      // 64x64
    THUMBNAIL_QUALITY_MEDIUM,   // 128x128
    THUMBNAIL_QUALITY_HIGH,      // 256x256
    THUMBNAIL_QUALITY_ULTRA      // 512x512
} ThumbnailQuality;

// Thumbnail generation status
typedef enum {
    THUMBNAIL_STATUS_PENDING,
    THUMBNAIL_STATUS_PROCESSING,
    THUMBNAIL_STATUS_COMPLETED,
    THUMBNAIL_STATUS_FAILED,
    THUMBNAIL_STATUS_CACHED
} ThumbnailStatus;

// Thumbnail request
typedef struct {
    u32 request_id;
    char file_path[512];
    ThumbnailType type;
    ThumbnailQuality quality;
    
    // Output data
    u32 texture_id;
    u32 width;
    u32 height;
    u8* pixel_data;
    size_t pixel_data_size;
    
    // Status and timing
    ThumbnailStatus status;
    u64 request_time;
    u64 completion_time;
    f32 processing_time;
    
    // Error handling
    char error_message[256];
    bool has_error;
} ThumbnailRequest;

// Thumbnail cache entry
typedef struct {
    char file_path[512];
    u64 file_modified_time;
    ThumbnailType type;
    ThumbnailQuality quality;
    
    u32 texture_id;
    u32 width;
    u32 height;
    u8* pixel_data;
    size_t pixel_data_size;
    
    u64 cache_time;
    u32 access_count;
    u64 last_access_time;
    
    bool is_valid;
} ThumbnailCacheEntry;

// Thumbnail generator system
typedef struct {
    // Request queue
    ThumbnailRequest* requests;
    u32 request_count;
    u32 request_capacity;
    u32 next_request_id;
    
    // Processing queue
    ThumbnailRequest* processing_queue;
    u32 processing_count;
    u32 processing_capacity;
    
    // Cache system
    ThumbnailCacheEntry* cache;
    u32 cache_count;
    u32 cache_capacity;
    u64 max_cache_size;        // In bytes
    u64 current_cache_size;
    
    // Thread pool for async processing
    ThreadPool* thread_pool;
    u32 worker_thread_count;
    
    // Rendering resources
    u32 render_target;
    u32 render_framebuffer;
    u32 default_material;
    u32 default_camera;
    
    // Configuration
    ThumbnailQuality default_quality;
    u32 max_concurrent_requests;
    bool enable_caching;
    bool auto_cleanup;
    u64 cache_ttl;             // Time to live in seconds
    
    // Statistics
    u32 total_requests;
    u32 completed_requests;
    u32 failed_requests;
    u32 cache_hits;
    u32 cache_misses;
    f32 average_processing_time;
    
    // Callbacks
    void (*thumbnail_completed_callback)(ThumbnailRequest* request, void* user_data);
    void (*thumbnail_failed_callback)(ThumbnailRequest* request, void* user_data);
    void* callback_user_data;
    
    bool is_initialized;
} ThumbnailGenerator;

// =================================================================================================
//                                    CORE API FUNCTIONS
// =================================================================================================

// System management
ThumbnailGenerator* thumbnail_generator_create(u32 worker_threads);
void thumbnail_generator_destroy(ThumbnailGenerator* generator);
bool thumbnail_generator_initialize(ThumbnailGenerator* generator);

// Request management
u32 thumbnail_generator_request_thumbnail(ThumbnailGenerator* generator, 
                                         const char* file_path, 
                                         ThumbnailType type, 
                                         ThumbnailQuality quality);
ThumbnailRequest* thumbnail_generator_get_request(ThumbnailGenerator* generator, u32 request_id);
void thumbnail_generator_cancel_request(ThumbnailGenerator* generator, u32 request_id);
void thumbnail_generator_cancel_all_requests(ThumbnailGenerator* generator);

// Cache management
void thumbnail_generator_clear_cache(ThumbnailGenerator* generator);
void thumbnail_generator_cleanup_cache(ThumbnailGenerator* generator);
bool thumbnail_generator_is_cached(ThumbnailGenerator* generator, 
                                const char* file_path, 
                                ThumbnailType type, 
                                ThumbnailQuality quality);
ThumbnailCacheEntry* thumbnail_generator_get_cached_thumbnail(ThumbnailGenerator* generator,
                                                           const char* file_path,
                                                           ThumbnailType type,
                                                           ThumbnailQuality quality);

// Configuration
void thumbnail_generator_set_default_quality(ThumbnailGenerator* generator, ThumbnailQuality quality);
void thumbnail_generator_set_max_cache_size(ThumbnailGenerator* generator, u64 max_size_bytes);
void thumbnail_generator_set_max_concurrent_requests(ThumbnailGenerator* generator, u32 max_requests);
void thumbnail_generator_enable_caching(ThumbnailGenerator* generator, bool enable);
void thumbnail_generator_set_cache_ttl(ThumbnailGenerator* generator, u64 ttl_seconds);

// Callbacks
void thumbnail_generator_set_completed_callback(ThumbnailGenerator* generator,
                                               void (*callback)(ThumbnailRequest*, void*),
                                               void* user_data);
void thumbnail_generator_set_failed_callback(ThumbnailGenerator* generator,
                                            void (*callback)(ThumbnailRequest*, void*),
                                            void* user_data);

// =================================================================================================
//                             THUMBNAIL GENERATION
// =================================================================================================

// Type-specific generators
bool thumbnail_generate_texture_thumbnail(ThumbnailRequest* request);
bool thumbnail_generate_model_thumbnail(ThumbnailRequest* request);
bool thumbnail_generate_material_thumbnail(ThumbnailRequest* request);
bool thumbnail_generate_audio_thumbnail(ThumbnailRequest* request);
bool thumbnail_generate_scene_thumbnail(ThumbnailRequest* request);
bool thumbnail_generate_prefab_thumbnail(ThumbnailRequest* request);

// Utility functions
u32 thumbnail_get_quality_size(ThumbnailQuality quality);
const char* thumbnail_get_type_name(ThumbnailType type);
const char* thumbnail_get_status_name(ThumbnailStatus status);
bool thumbnail_validate_file_path(const char* file_path, ThumbnailType expected_type);

// =================================================================================================
//                              ASYNC PROCESSING
// =================================================================================================

// Thread worker function
void thumbnail_worker_function(void* user_data);

// Request processing
void thumbnail_process_request(ThumbnailGenerator* generator, ThumbnailRequest* request);
void thumbnail_complete_request(ThumbnailGenerator* generator, ThumbnailRequest* request);
void thumbnail_fail_request(ThumbnailGenerator* generator, ThumbnailRequest* request, const char* error);

// Queue management
void thumbnail_enqueue_request(ThumbnailGenerator* generator, ThumbnailRequest* request);
ThumbnailRequest* thumbnail_dequeue_request(ThumbnailGenerator* generator);
void thumbnail_update_processing_queue(ThumbnailGenerator* generator);

// =================================================================================================
//                                CACHE SYSTEM
// =================================================================================================

// Cache operations
bool thumbnail_cache_add(ThumbnailGenerator* generator, ThumbnailRequest* request);
ThumbnailCacheEntry* thumbnail_cache_find(ThumbnailGenerator* generator, 
                                        const char* file_path,
                                        ThumbnailType type,
                                        ThumbnailQuality quality);
void thumbnail_cache_remove(ThumbnailGenerator* generator, const char* file_path);
void thumbnail_cache_update_access(ThumbnailGenerator* generator, ThumbnailCacheEntry* entry);

// Cache cleanup
void thumbnail_cache_remove_expired(ThumbnailGenerator* generator);
void thumbnail_cache_remove_least_used(ThumbnailGenerator* generator, u32 count);
void thumbnail_cache_enforce_size_limit(ThumbnailGenerator* generator);

// =================================================================================================
//                              RENDERING HELPERS
// =================================================================================================

// Rendering setup
bool thumbnail_setup_render_context(ThumbnailGenerator* generator, u32 width, u32 height);
void thumbnail_cleanup_render_context(ThumbnailGenerator* generator);

// Camera setup
void thumbnail_setup_camera(ThumbnailGenerator* generator, const char* file_path, ThumbnailType type);

// Lighting setup
void thumbnail_setup_lighting(ThumbnailGenerator* generator, ThumbnailType type);

// =================================================================================================
//                                STATISTICS
// =================================================================================================

void thumbnail_generator_get_stats(ThumbnailGenerator* generator,
                                 u32* total_requests,
                                 u32* completed_requests,
                                 u32* failed_requests,
                                 f32* average_time,
                                 u32* cache_hits,
                                 u32* cache_misses);

void thumbnail_generator_print_stats(ThumbnailGenerator* generator);

// =================================================================================================
//                                DEBUG UTILITIES
// =================================================================================================

void thumbnail_print_request_info(ThumbnailRequest* request);
void thumbnail_print_cache_info(ThumbnailGenerator* generator);
void thumbnail_validate_cache_integrity(ThumbnailGenerator* generator);

#ifdef __cplusplus
}
#endif

#endif // EDITOR_ASSET_BROWSER_THUMBNAIL_GENERATOR_H
