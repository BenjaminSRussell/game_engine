/*
 * cloth_tearing.h
 * Cloth tearing
 *
 * Part of the Cloth System subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CLOTH_SYSTEM_CLOTH_TEARING_H
#define CLOTH_SYSTEM_CLOTH_TEARING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cloth_system_cloth_tearing_handle {
    uint32_t id;
} cloth_system_cloth_tearing_handle_t;

typedef struct cloth_system_cloth_tearing_desc {
    uint32_t flags;
    void* user_data;
} cloth_system_cloth_tearing_desc_t;

typedef struct cloth_system_cloth_tearing_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} cloth_system_cloth_tearing_info_t;

/* Backend types */
typedef enum cloth_system_cloth_tearing_backend {
    CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_CPU = 0,
    CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_VULKAN,
    CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_METAL,
    CLOTH_SYSTEM_CLOTH_TEARING_BACKEND_D3D12
} cloth_system_cloth_tearing_backend_t;

typedef enum cloth_system_cloth_tearing_error {
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NONE = 0,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_INVALID_PARAM = -1,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_NOT_INITIALIZED = -2,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_OUT_OF_MEMORY = -3,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_BACKEND_FAILED = -4,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_VALIDATION_FAILED = -5,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_SERIALIZATION_FAILED = -6,
    CLOTH_SYSTEM_CLOTH_TEARING_ERROR_THREAD_SAFETY_VIOLATION = -7
} cloth_system_cloth_tearing_error_t;

typedef struct cloth_system_cloth_tearing_backend_ctx {
    uint32_t version;
    uint64_t last_frame;
#ifdef _WIN32
    ID3D12Device* d3d12_device;
    ID3D12CommandQueue* d3d12_queue;
#endif
    void* vulkan_instance;
    void* metal_device;
} cloth_system_cloth_tearing_backend_ctx_t;

typedef struct cloth_system_cloth_tearing_memory_stats {
    size_t total_allocated;
    size_t peak_usage;
    uint32_t allocation_count;
    uint32_t leak_count;
} cloth_system_cloth_tearing_memory_stats_t;

typedef struct cloth_system_cloth_tearing_performance_counters {
    uint64_t frames_processed;
    uint64_t tears_detected;
    uint64_t tears_processed;
    double avg_processing_time;
    uint64_t last_update_time;
} cloth_system_cloth_tearing_performance_counters_t;

typedef struct cloth_system_cloth_tearing_hot_reload {
    bool enabled;
    int file_watch_fd;
    pthread_t watch_thread;
    bool watch_thread_running;
    char watch_directory[256];
} cloth_system_cloth_tearing_hot_reload_t;

typedef struct cloth_system_cloth_tearing_cache_entry {
    void* data;
    size_t size;
    uint32_t checksum;
    uint64_t timestamp;
    bool valid;
} cloth_system_cloth_tearing_cache_entry_t;

typedef struct cloth_system_cloth_tearing_async_operation {
    pthread_t thread;
    void* input_data;
    void* output_data;
    size_t input_size;
    size_t output_size;
    bool active;
    void (*callback)(void* output_data, void* user_data);
    void* user_data;
} cloth_system_cloth_tearing_async_operation_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int cloth_system_cloth_tearing_init(void);
void cloth_system_cloth_tearing_shutdown(void);

/* Lifecycle */
int cloth_system_cloth_tearing_create(cloth_system_cloth_tearing_handle_t* out_handle, const cloth_system_cloth_tearing_desc_t* desc);
void cloth_system_cloth_tearing_destroy(cloth_system_cloth_tearing_handle_t handle);

/* Operations */
int cloth_system_cloth_tearing_update(cloth_system_cloth_tearing_handle_t handle, const void* data, size_t size);
bool cloth_system_cloth_tearing_is_valid(cloth_system_cloth_tearing_handle_t handle);
int cloth_system_cloth_tearing_get_info(cloth_system_cloth_tearing_handle_t handle, cloth_system_cloth_tearing_info_t* out_info);
void cloth_system_cloth_tearing_mark_dirty(cloth_system_cloth_tearing_handle_t handle);
int cloth_system_cloth_tearing_process_pending(void);

/* Statistics */
uint32_t cloth_system_cloth_tearing_get_count(void);
size_t cloth_system_cloth_tearing_get_memory_usage(void);
void cloth_system_cloth_tearing_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CLOTH_SYSTEM_CLOTH_TEARING_H */
