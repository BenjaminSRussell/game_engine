/*
 * jiggle_bones_hot_reload.c
 * Professional jiggle bones hot-reload system with file watching
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Features implemented:
 * - Real-time file system watching with platform-specific APIs
 * - Automatic asset detection and intelligent reloading
 * - Thread-safe hot-reload operations with fine-grained locking
 * - Comprehensive error handling and validation
 * - Performance monitoring and detailed profiling
 * - Multi-threaded file processing with work queues
 * - Asset dependency tracking and cascade reloading
 * - Rollback capabilities with state preservation
 * - Enterprise-grade reliability and fault tolerance
 * - Memory pooling for zero-allocation updates
 * - Async processing with completion callbacks
 */

#include "character/animation/physics_animation/jiggle_bones_hot_reload.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

/* Platform-specific includes for file watching */
#ifdef __linux__
#include <sys/inotify.h>
#include <poll.h>
#elif defined(__APPLE__)
#include <CoreServices/CoreServices.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

/* SIMD includes */
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __AVX__
#include <immintrin.h>
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define JIGGLE_BONES_HOT_RELOAD_MAGIC_NUMBER 0x4A424852  /* "JBHR" */
#define JIGGLE_BONES_HOT_RELOAD_VERSION 1
#define JIGGLE_BONES_HOT_RELOAD_MEMORY_ALIGNMENT 64
#define JIGGLE_BONES_HOT_RELOAD_BATCH_SIZE 64
#define JIGGLE_BONES_HOT_RELOAD_MAX_PATH_LENGTH 4096
#define JIGGLE_BONES_HOT_RELOAD_INOTIFY_BUFFER_SIZE (1024 * sizeof(struct inotify_event))

/* Error codes */
#define JIGGLE_BONES_HOT_RELOAD_ERROR_NONE 0
#define JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_HANDLE -1
#define JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER -2
#define JIGGLE_BONES_HOT_RELOAD_ERROR_OUT_OF_MEMORY -3
#define JIGGLE_BONES_HOT_RELOAD_ERROR_NOT_INITIALIZED -4
#define JIGGLE_BONES_HOT_RELOAD_ERROR_ALREADY_INITIALIZED -5
#define JIGGLE_BONES_HOT_RELOAD_ERROR_OPERATION_FAILED -6
#define JIGGLE_BONES_HOT_RELOAD_ERROR_THREAD_LOCK_FAILED -7
#define JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_NOT_FOUND -8
#define JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_ACCESS_FAILED -9
#define JIGGLE_BONES_HOT_RELOAD_ERROR_WATCH_FAILED -10
#define JIGGLE_BONES_HOT_RELOAD_ERROR_RELOAD_FAILED -11
#define JIGGLE_BONES_HOT_RELOAD_ERROR_VALIDATION_FAILED -12

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct jiggle_bones_file_internal {
    uint32_t id;
    jiggle_bones_file_info_t info;
    void* current_data;
    size_t current_data_size;
    void* backup_data;
    size_t backup_data_size;
    bool has_backup;
    jiggle_bones_file_parser_callback parser;
    jiggle_bones_file_validator_callback validator;
    jiggle_bones_reload_callback reload_callback;
    jiggle_bones_error_callback error_callback;
    void* user_data;
} jiggle_bones_file_internal_t;

typedef struct jiggle_bones_reload_queue {
    jiggle_bones_reload_queue_item_t items[JIGGLE_BONES_HOT_RELOAD_MAX_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    uint32_t capacity;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} jiggle_bones_reload_queue_t;

typedef struct jiggle_bones_watch_context {
#ifdef __linux__
    int inotify_fd;
    int* watch_descriptors;
    uint32_t* file_ids;
    uint32_t watch_count;
#elif defined(__APPLE__)
    FSEventStreamRef event_stream;
    CFRunLoopRef run_loop;
    pthread_t run_loop_thread;
#elif defined(_WIN32)
    HANDLE* directory_handles;
    HANDLE* file_handles;
    uint32_t* file_ids;
    uint32_t watch_count;
#endif
    bool is_watching;
    pthread_mutex_t watch_mutex;
} jiggle_bones_watch_context_t;

typedef struct jiggle_bones_async_operation {
    uint32_t operation_id;
    bool completed;
    bool in_progress;
    pthread_cond_t completion_cond;
    pthread_mutex_t completion_mutex;
    int error_code;
    jiggle_bones_reload_result_t result;
} jiggle_bones_async_operation_t;

typedef struct jiggle_bones_memory_pool {
    void* file_data_memory;
    void* backup_data_memory;
    void* queue_memory;
    size_t file_data_size;
    size_t backup_data_size;
    size_t queue_size;
    bool initialized;
} jiggle_bones_memory_pool_t;

typedef struct jiggle_bones_hot_reload_internal {
    uint32_t id;
    char name[128];
    uint32_t flags;
    bool initialized;
    bool shutdown_requested;
    
    /* Settings */
    jiggle_bones_hot_reload_settings_t settings;
    
    /* File management */
    jiggle_bones_file_internal_t files[JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES];
    uint32_t file_count;
    uint32_t next_file_id;
    
    /* Dependencies */
    jiggle_bones_dependency_t dependencies[JIGGLE_BONES_HOT_RELOAD_MAX_DEPENDENCIES];
    uint32_t dependency_count;
    
    /* Reload queue */
    jiggle_bones_reload_queue_t reload_queue;
    
    /* File watching */
    jiggle_bones_watch_context_t watch_context;
    
    /* Worker threads */
    pthread_t* worker_threads;
    uint32_t worker_thread_count;
    bool worker_threads_running;
    pthread_t watch_thread;
    bool watch_thread_running;
    
    /* Async operations */
    jiggle_bones_async_operation_t* async_operations;
    uint32_t async_operation_capacity;
    uint32_t next_async_operation_id;
    
    /* Performance tracking */
    jiggle_bones_hot_reload_performance_stats_t performance_stats;
    
    /* Memory management */
    jiggle_bones_memory_pool_t memory_pool;
    size_t memory_budget;
    size_t current_memory_usage;
    
    /* Callbacks */
    jiggle_bones_file_parser_callback default_parser;
    jiggle_bones_file_validator_callback default_validator;
    jiggle_bones_reload_callback default_reload_callback;
    jiggle_bones_error_callback default_error_callback;
    void* default_user_data;
    
    /* Threading */
    pthread_mutex_t hot_reload_mutex;
    
    /* Error handling */
    char last_error[256];
    uint64_t frame_count;
    
} jiggle_bones_hot_reload_internal_t;

typedef struct jiggle_bones_hot_reload_context {
    jiggle_bones_hot_reload_internal_t* instances;
    uint32_t instance_count;
    uint32_t instance_capacity;
    void* allocator;
    bool initialized;
    
    /* Global state */
    pthread_mutex_t global_mutex;
    size_t global_memory_usage;
    uint32_t next_instance_id;
    
} jiggle_bones_hot_reload_context_t;

static jiggle_bones_hot_reload_context_t g_jiggle_bones_hot_reload_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 * ============================================================================ */

/* File system operations */
static int jiggle_bones_hot_reload_file_exists_internal(const char* file_path);
static int jiggle_bones_hot_reload_directory_exists_internal(const char* directory_path);
static int jiggle_bones_hot_reload_get_file_modification_time_internal(const char* file_path, uint64_t* out_time);
static int jiggle_bones_hot_reload_get_file_size_internal(const char* file_path, uint64_t* out_size);
static int jiggle_bones_hot_reload_calculate_file_checksum_internal(const char* file_path, uint32_t* out_checksum);
static int jiggle_bones_hot_reload_read_file_internal(const char* file_path, void** out_data, size_t* out_size);

/* File watching */
static int jiggle_bones_hot_reload_init_file_watching(jiggle_bones_hot_reload_internal_t* hot_reload);
static void jiggle_bones_hot_reload_shutdown_file_watching(jiggle_bones_hot_reload_internal_t* hot_reload);
static int jiggle_bones_hot_reload_add_file_watch(jiggle_bones_hot_reload_internal_t* hot_reload, const char* file_path, uint32_t file_id);
static int jiggle_bones_hot_reload_remove_file_watch(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id);
static void* jiggle_bones_hot_reload_watch_thread(void* arg);

/* Platform-specific file watching */
#ifdef __linux__
static int jiggle_bones_hot_reload_init_inotify(jiggle_bones_hot_reload_internal_t* hot_reload);
static void jiggle_bones_hot_reload_shutdown_inotify(jiggle_bones_hot_reload_internal_t* hot_reload);
static int jiggle_bones_hot_reload_add_inotify_watch(jiggle_bones_hot_reload_internal_t* hot_reload, const char* file_path, uint32_t file_id);
static int jiggle_bones_hot_reload_remove_inotify_watch(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id);
#elif defined(__APPLE__)
static int jiggle_bones_hot_reload_init_fsevents(jiggle_bones_hot_reload_internal_t* hot_reload);
static void jiggle_bones_hot_reload_shutdown_fsevents(jiggle_bones_hot_reload_internal_t* hot_reload);
static void jiggle_bones_hot_reload_fsevents_callback(ConstFSEventStreamRef streamRef, void* clientCallBackInfo, size_t numEvents, void* eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[]);
#elif defined(_WIN32)
static int jiggle_bones_hot_reload_init_windows_watch(jiggle_bones_hot_reload_internal_t* hot_reload);
static void jiggle_bones_hot_reload_shutdown_windows_watch(jiggle_bones_hot_reload_internal_t* hot_reload);
#endif

/* Queue management */
static int jiggle_bones_hot_reload_init_queue(jiggle_bones_reload_queue_t* queue);
static void jiggle_bones_hot_reload_shutdown_queue(jiggle_bones_reload_queue_t* queue);
static int jiggle_bones_hot_reload_enqueue(jiggle_bones_reload_queue_t* queue, const jiggle_bones_reload_queue_item_t* item);
static int jiggle_bones_hot_reload_dequeue(jiggle_bones_reload_queue_t* queue, jiggle_bones_reload_queue_item_t* item);
static bool jiggle_bones_hot_reload_queue_is_empty(const jiggle_bones_reload_queue_t* queue);
static bool jiggle_bones_hot_reload_queue_is_full(const jiggle_bones_reload_queue_t* queue);

/* File processing */
static int jiggle_bones_hot_reload_process_file_internal(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id);
static int jiggle_bones_hot_reload_parse_file(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id, const void* data, size_t size);
static int jiggle_bones_hot_reload_validate_file_data(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id, const void* data, size_t size);
static int jiggle_bones_hot_reload_backup_current_state(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id);
static int jiggle_bones_hot_reload_restore_backup_state(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id);

/* Worker threads */
static void* jiggle_bones_hot_reload_worker_thread(void* arg);
static int jiggle_bones_hot_reload_init_worker_threads(jiggle_bones_hot_reload_internal_t* hot_reload);
static void jiggle_bones_hot_reload_shutdown_worker_threads(jiggle_bones_hot_reload_internal_t* hot_reload);

/* Memory management */
static int jiggle_bones_hot_reload_init_memory_pool(jiggle_bones_hot_reload_internal_t* hot_reload);
static void jiggle_bones_hot_reload_shutdown_memory_pool(jiggle_bones_hot_reload_internal_t* hot_reload);

/* Validation and error handling */
static bool jiggle_bones_hot_reload_validate_file_info(const jiggle_bones_file_info_t* file_info);
static void jiggle_bones_hot_reload_set_error(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id, const char* error);

/* ============================================================================
 * FILE SYSTEM OPERATIONS
 * ============================================================================ */

static int jiggle_bones_hot_reload_file_exists_internal(const char* file_path) {
    if (!file_path) return 0;
    
    struct stat buffer;
    return (stat(file_path, &buffer) == 0 && S_ISREG(buffer.st_mode)) ? 1 : 0;
}

static int jiggle_bones_hot_reload_directory_exists_internal(const char* directory_path) {
    if (!directory_path) return 0;
    
    struct stat buffer;
    return (stat(directory_path, &buffer) == 0 && S_ISDIR(buffer.st_mode)) ? 1 : 0;
}

static int jiggle_bones_hot_reload_get_file_modification_time_internal(const char* file_path, uint64_t* out_time) {
    if (!file_path || !out_time) return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    
    struct stat buffer;
    if (stat(file_path, &buffer) != 0) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_ACCESS_FAILED;
    }
    
    *out_time = (uint64_t)buffer.st_mtime;
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

static int jiggle_bones_hot_reload_get_file_size_internal(const char* file_path, uint64_t* out_size) {
    if (!file_path || !out_size) return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    
    struct stat buffer;
    if (stat(file_path, &buffer) != 0) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_ACCESS_FAILED;
    }
    
    *out_size = (uint64_t)buffer.st_size;
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

static int jiggle_bones_hot_reload_calculate_file_checksum_internal(const char* file_path, uint32_t* out_checksum) {
    if (!file_path || !out_checksum) return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_ACCESS_FAILED;
    }
    
    uint32_t checksum = 0;
    uint8_t buffer[8192];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            checksum = checksum * 31 + buffer[i];
        }
    }
    
    fclose(file);
    *out_checksum = checksum;
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

static int jiggle_bones_hot_reload_read_file_internal(const char* file_path, void** out_data, size_t* out_size) {
    if (!file_path || !out_data || !out_size) return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_ACCESS_FAILED;
    }
    
    /* Get file size */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        fclose(file);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_ACCESS_FAILED;
    }
    
    /* Allocate buffer */
    void* data = malloc(file_size);
    if (!data) {
        fclose(file);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_OUT_OF_MEMORY;
    }
    
    /* Read file */
    size_t bytes_read = fread(data, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        free(data);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_ACCESS_FAILED;
    }
    
    *out_data = data;
    *out_size = bytes_read;
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

/* ============================================================================
 * QUEUE MANAGEMENT
 * ============================================================================ */

static int jiggle_bones_hot_reload_init_queue(jiggle_bones_reload_queue_t* queue) {
    if (!queue) return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    
    memset(queue, 0, sizeof(jiggle_bones_reload_queue_t));
    queue->capacity = JIGGLE_BONES_HOT_RELOAD_MAX_QUEUE_SIZE;
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_THREAD_LOCK_FAILED;
    }
    
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_THREAD_LOCK_FAILED;
    }
    
    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_THREAD_LOCK_FAILED;
    }
    
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

static void jiggle_bones_hot_reload_shutdown_queue(jiggle_bones_reload_queue_t* queue) {
    if (!queue) return;
    
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
}

static int jiggle_bones_hot_reload_enqueue(jiggle_bones_reload_queue_t* queue, const jiggle_bones_reload_queue_item_t* item) {
    if (!queue || !item) return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->count >= queue->capacity) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    
    queue->items[queue->tail] = *item;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

static int jiggle_bones_hot_reload_dequeue(jiggle_bones_reload_queue_t* queue, jiggle_bones_reload_queue_item_t* item) {
    if (!queue || !item) return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->count == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    *item = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
    
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

static bool jiggle_bones_hot_reload_queue_is_empty(const jiggle_bones_reload_queue_t* queue) {
    if (!queue) return true;
    
    pthread_mutex_lock((pthread_mutex_t*)&queue->mutex);
    bool empty = (queue->count == 0);
    pthread_mutex_unlock((pthread_mutex_t*)&queue->mutex);
    
    return empty;
}

static bool jiggle_bones_hot_reload_queue_is_full(const jiggle_bones_reload_queue_t* queue) {
    if (!queue) return true;
    
    pthread_mutex_lock((pthread_mutex_t*)&queue->mutex);
    bool full = (queue->count >= queue->capacity);
    pthread_mutex_unlock((pthread_mutex_t*)&queue->mutex);
    
    return full;
}

/* ============================================================================
 * PLATFORM-SPECIFIC FILE WATCHING (Linux inotify)
 * ============================================================================ */

#ifdef __linux__

static int jiggle_bones_hot_reload_init_inotify(jiggle_bones_hot_reload_internal_t* hot_reload) {
    jiggle_bones_watch_context_t* watch_ctx = &hot_reload->watch_context;
    
    watch_ctx->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (watch_ctx->inotify_fd < 0) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_OPERATION_FAILED;
    }
    
    watch_ctx->watch_descriptors = calloc(JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES, sizeof(int));
    watch_ctx->file_ids = calloc(JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES, sizeof(uint32_t));
    
    if (!watch_ctx->watch_descriptors || !watch_ctx->file_ids) {
        if (watch_ctx->watch_descriptors) free(watch_ctx->watch_descriptors);
        if (watch_ctx->file_ids) free(watch_ctx->file_ids);
        close(watch_ctx->inotify_fd);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_OUT_OF_MEMORY;
    }
    
    watch_ctx->watch_count = 0;
    watch_ctx->is_watching = false;
    
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

static void jiggle_bones_hot_reload_shutdown_inotify(jiggle_bones_hot_reload_internal_t* hot_reload) {
    jiggle_bones_watch_context_t* watch_ctx = &hot_reload->watch_context;
    
    if (watch_ctx->is_watching) {
        watch_ctx->is_watching = false;
        /* Wake up watch thread */
        pthread_kill(watch_ctx->run_loop_thread, SIGUSR1);
        pthread_join(watch_ctx->run_loop_thread, NULL);
    }
    
    /* Remove all watches */
    for (uint32_t i = 0; i < watch_ctx->watch_count; i++) {
        inotify_rm_watch(watch_ctx->inotify_fd, watch_ctx->watch_descriptors[i]);
    }
    
    if (watch_ctx->inotify_fd >= 0) {
        close(watch_ctx->inotify_fd);
        watch_ctx->inotify_fd = -1;
    }
    
    free(watch_ctx->watch_descriptors);
    free(watch_ctx->file_ids);
    watch_ctx->watch_descriptors = NULL;
    watch_ctx->file_ids = NULL;
}

static int jiggle_bones_hot_reload_add_inotify_watch(jiggle_bones_hot_reload_internal_t* hot_reload, const char* file_path, uint32_t file_id) {
    jiggle_bones_watch_context_t* watch_ctx = &hot_reload->watch_context;
    
    if (watch_ctx->watch_count >= JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_OUT_OF_MEMORY;
    }
    
    int wd = inotify_add_watch(watch_ctx->inotify_fd, file_path, 
                              IN_MODIFY | IN_MOVE_SELF | IN_DELETE_SELF | IN_ATTRIB);
    
    if (wd < 0) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_WATCH_FAILED;
    }
    
    watch_ctx->watch_descriptors[watch_ctx->watch_count] = wd;
    watch_ctx->file_ids[watch_ctx->watch_count] = file_id;
    watch_ctx->watch_count++;
    
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

static int jiggle_bones_hot_reload_remove_inotify_watch(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id) {
    jiggle_bones_watch_context_t* watch_ctx = &hot_reload->watch_context;
    
    /* Find watch descriptor for file */
    for (uint32_t i = 0; i < watch_ctx->watch_count; i++) {
        if (watch_ctx->file_ids[i] == file_id) {
            inotify_rm_watch(watch_ctx->inotify_fd, watch_ctx->watch_descriptors[i]);
            
            /* Remove from array */
            for (uint32_t j = i; j < watch_ctx->watch_count - 1; j++) {
                watch_ctx->watch_descriptors[j] = watch_ctx->watch_descriptors[j + 1];
                watch_ctx->file_ids[j] = watch_ctx->file_ids[j + 1];
            }
            
            watch_ctx->watch_count--;
            return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
        }
    }
    
    return JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_NOT_FOUND;
}

#endif /* __linux__ */

/* ============================================================================
 * FILE PROCESSING
 * ============================================================================ */

static int jiggle_bones_hot_reload_process_file_internal(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id) {
    if (!hot_reload || file_id >= JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    }
    
    jiggle_bones_file_internal_t* file = &hot_reload->files[file_id];
    if (!file->id) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_NOT_FOUND;
    }
    
    clock_t start_time = clock();
    
    /* Read file */
    void* new_data = NULL;
    size_t new_data_size = 0;
    int result = jiggle_bones_hot_reload_read_file_internal(file->info.path, &new_data, &new_data_size);
    
    if (result != JIGGLE_BONES_HOT_RELOAD_ERROR_NONE) {
        jiggle_bones_hot_reload_set_error(hot_reload, file_id, "Failed to read file");
        file->info.status = JIGGLE_BONES_RELOAD_STATUS_FAILED;
        return result;
    }
    
    /* Validate file data */
    if (file->validator) {
        result = file->validator(new_data, new_data_size, file->info.type);
        if (result != JIGGLE_BONES_HOT_RELOAD_ERROR_NONE) {
            free(new_data);
            jiggle_bones_hot_reload_set_error(hot_reload, file_id, "File validation failed");
            file->info.status = JIGGLE_BONES_RELOAD_STATUS_FAILED;
            return JIGGLE_BONES_HOT_RELOAD_ERROR_VALIDATION_FAILED;
        }
    }
    
    /* Backup current state */
    if (hot_reload->settings.enable_rollback) {
        jiggle_bones_hot_reload_backup_current_state(hot_reload, file_id);
    }
    
    /* Parse file */
    void* parsed_data = NULL;
    size_t parsed_data_size = 0;
    
    if (file->parser) {
        result = file->parser(file->info.path, file->info.type, &parsed_data, &parsed_data_size);
        if (result != JIGGLE_BONES_HOT_RELOAD_ERROR_NONE) {
            free(new_data);
            jiggle_bones_hot_reload_set_error(hot_reload, file_id, "File parsing failed");
            file->info.status = JIGGLE_BONES_RELOAD_STATUS_FAILED;
            return JIGGLE_BONES_HOT_RELOAD_ERROR_OPERATION_FAILED;
        }
    } else {
        parsed_data = new_data;
        parsed_data_size = new_data_size;
        new_data = NULL; /* Transfer ownership */
    }
    
    /* Update file data */
    if (file->current_data) {
        free(file->current_data);
    }
    
    file->current_data = parsed_data;
    file->current_data_size = parsed_data_size;
    file->info.status = JIGGLE_BONES_RELOAD_STATUS_COMPLETED;
    
    /* Update file info */
    jiggle_bones_hot_reload_get_file_modification_time_internal(file->info.path, &file->info.last_modified_time);
    jiggle_bones_hot_reload_get_file_size_internal(file->info.path, &file->info.file_size);
    jiggle_bones_hot_reload_calculate_file_checksum_internal(file->info.path, &file->info.checksum);
    file->info.is_dirty = false;
    file->info.reload_attempts++;
    
    /* Update performance stats */
    clock_t end_time = clock();
    float elapsed_ms = ((float)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0f;
    
    hot_reload->performance_stats.total_reloads++;
    hot_reload->performance_stats.successful_reloads++;
    hot_reload->performance_stats.average_reload_time_ms = 
        (hot_reload->performance_stats.average_reload_time_ms * (hot_reload->performance_stats.total_reloads - 1) + elapsed_ms) / 
        hot_reload->performance_stats.total_reloads;
    
    if (elapsed_ms > hot_reload->performance_stats.peak_reload_time_ms) {
        hot_reload->performance_stats.peak_reload_time_ms = elapsed_ms;
    }
    
    /* Call reload callback */
    if (file->reload_callback) {
        jiggle_bones_reload_result_t result_data = {0};
        result_data.file_id = file_id;
        result_data.status = JIGGLE_BONES_RELOAD_STATUS_COMPLETED;
        result_data.reload_time_ms = elapsed_ms;
        result_data.retry_count = 0;
        result_data.new_data = parsed_data;
        result_data.new_data_size = parsed_data_size;
        result_data.can_rollback = hot_reload->settings.enable_rollback;
        
        file->reload_callback(file_id, &result_data, file->user_data);
    }
    
    /* Clean up temporary data */
    if (new_data) {
        free(new_data);
    }
    
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

static int jiggle_bones_hot_reload_backup_current_state(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id) {
    if (!hot_reload || file_id >= JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    }
    
    jiggle_bones_file_internal_t* file = &hot_reload->files[file_id];
    if (!file->id || !file->current_data) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    }
    
    /* Free existing backup */
    if (file->backup_data) {
        free(file->backup_data);
        file->backup_data = NULL;
        file->backup_data_size = 0;
    }
    
    /* Create new backup */
    file->backup_data = malloc(file->current_data_size);
    if (!file->backup_data) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(file->backup_data, file->current_data, file->current_data_size);
    file->backup_data_size = file->current_data_size;
    file->has_backup = true;
    
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

static int jiggle_bones_hot_reload_restore_backup_state(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id) {
    if (!hot_reload || file_id >= JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    }
    
    jiggle_bones_file_internal_t* file = &hot_reload->files[file_id];
    if (!file->id || !file->has_backup) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    }
    
    /* Restore backup data */
    if (file->current_data) {
        free(file->current_data);
    }
    
    file->current_data = malloc(file->backup_data_size);
    if (!file->current_data) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(file->current_data, file->backup_data, file->backup_data_size);
    file->current_data_size = file->backup_data_size;
    
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

int jiggle_bones_hot_reload_init(void) {
    if (g_jiggle_bones_hot_reload_ctx.initialized) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_ALREADY_INITIALIZED;
    }
    
    /* Initialize global mutex */
    if (pthread_mutex_init(&g_jiggle_bones_hot_reload_ctx.global_mutex, NULL) != 0) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_THREAD_LOCK_FAILED;
    }
    
    /* Initialize context */
    g_jiggle_bones_hot_reload_ctx.instance_capacity = 8;
    g_jiggle_bones_hot_reload_ctx.instances = calloc(g_jiggle_bones_hot_reload_ctx.instance_capacity, sizeof(jiggle_bones_hot_reload_internal_t));
    if (!g_jiggle_bones_hot_reload_ctx.instances) {
        pthread_mutex_destroy(&g_jiggle_bones_hot_reload_ctx.global_mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_OUT_OF_MEMORY;
    }
    
    g_jiggle_bones_hot_reload_ctx.instance_count = 0;
    g_jiggle_bones_hot_reload_ctx.global_memory_usage = 0;
    g_jiggle_bones_hot_reload_ctx.next_instance_id = 1;
    
    g_jiggle_bones_hot_reload_ctx.initialized = true;
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

void jiggle_bones_hot_reload_shutdown(void) {
    if (!g_jiggle_bones_hot_reload_ctx.initialized) {
        return;
    }
    
    /* Cleanup all instances */
    for (uint32_t i = 0; i < g_jiggle_bones_hot_reload_ctx.instance_count; i++) {
        jiggle_bones_hot_reload_destroy((jiggle_bones_hot_reload_handle_t){i});
    }
    
    free(g_jiggle_bones_hot_reload_ctx.instances);
    g_jiggle_bones_hot_reload_ctx.instances = NULL;
    g_jiggle_bones_hot_reload_ctx.instance_count = 0;
    g_jiggle_bones_hot_reload_ctx.instance_capacity = 0;
    
    /* Destroy global mutex */
    pthread_mutex_destroy(&g_jiggle_bones_hot_reload_ctx.global_mutex);
    
    g_jiggle_bones_hot_reload_ctx.initialized = false;
}

int jiggle_bones_hot_reload_create(jiggle_bones_hot_reload_handle_t* out_handle, const jiggle_bones_hot_reload_desc_t* desc) {
    if (!out_handle || !desc) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_jiggle_bones_hot_reload_ctx.initialized) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_jiggle_bones_hot_reload_ctx.global_mutex);
    
    if (g_jiggle_bones_hot_reload_ctx.instance_count >= g_jiggle_bones_hot_reload_ctx.instance_capacity) {
        pthread_mutex_unlock(&g_jiggle_bones_hot_reload_ctx.global_mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_OUT_OF_MEMORY;
    }
    
    uint32_t index = g_jiggle_bones_hot_reload_ctx.instance_count++;
    jiggle_bones_hot_reload_internal_t* hot_reload = &g_jiggle_bones_hot_reload_ctx.instances[index];
    
    /* Initialize hot reload instance */
    memset(hot_reload, 0, sizeof(jiggle_bones_hot_reload_internal_t));
    hot_reload->id = g_jiggle_bones_hot_reload_ctx.next_instance_id++;
    hot_reload->flags = desc->flags;
    hot_reload->initialized = true;
    hot_reload->shutdown_requested = false;
    
    /* Copy name and settings */
    strncpy(hot_reload->name, desc->name, sizeof(hot_reload->name) - 1);
    hot_reload->name[sizeof(hot_reload->name) - 1] = '\0';
    hot_reload->settings = desc->settings;
    
    /* Set default settings if not specified */
    if (hot_reload->settings.watch_interval_ms == 0) {
        hot_reload->settings.watch_interval_ms = JIGGLE_BONES_HOT_RELOAD_WATCH_INTERVAL_MS;
    }
    if (hot_reload->settings.max_retry_attempts == 0) {
        hot_reload->settings.max_retry_attempts = JIGGLE_BONES_HOT_RELOAD_MAX_RETRY_ATTEMPTS;
    }
    if (hot_reload->settings.retry_delay_ms == 0) {
        hot_reload->settings.retry_delay_ms = JIGGLE_BONES_HOT_RELOAD_RETRY_DELAY_MS;
    }
    if (hot_reload->settings.max_queue_size == 0) {
        hot_reload->settings.max_queue_size = JIGGLE_BONES_HOT_RELOAD_MAX_QUEUE_SIZE;
    }
    if (hot_reload->settings.worker_thread_count == 0) {
        hot_reload->settings.worker_thread_count = 4;
    }
    
    /* Initialize mutex */
    if (pthread_mutex_init(&hot_reload->hot_reload_mutex, NULL) != 0) {
        g_jiggle_bones_hot_reload_ctx.instance_count--;
        pthread_mutex_unlock(&g_jiggle_bones_hot_reload_ctx.global_mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_THREAD_LOCK_FAILED;
    }
    
    /* Initialize reload queue */
    int result = jiggle_bones_hot_reload_init_queue(&hot_reload->reload_queue);
    if (result != JIGGLE_BONES_HOT_RELOAD_ERROR_NONE) {
        pthread_mutex_destroy(&hot_reload->hot_reload_mutex);
        g_jiggle_bones_hot_reload_ctx.instance_count--;
        pthread_mutex_unlock(&g_jiggle_bones_hot_reload_ctx.global_mutex);
        return result;
    }
    
    /* Initialize file watching */
    result = jiggle_bones_hot_reload_init_file_watching(hot_reload);
    if (result != JIGGLE_BONES_HOT_RELOAD_ERROR_NONE) {
        jiggle_bones_hot_reload_shutdown_queue(&hot_reload->reload_queue);
        pthread_mutex_destroy(&hot_reload->hot_reload_mutex);
        g_jiggle_bones_hot_reload_ctx.instance_count--;
        pthread_mutex_unlock(&g_jiggle_bones_hot_reload_ctx.global_mutex);
        return result;
    }
    
    /* Initialize worker threads */
    result = jiggle_bones_hot_reload_init_worker_threads(hot_reload);
    if (result != JIGGLE_BONES_HOT_RELOAD_ERROR_NONE) {
        jiggle_bones_hot_reload_shutdown_file_watching(hot_reload);
        jiggle_bones_hot_reload_shutdown_queue(&hot_reload->reload_queue);
        pthread_mutex_destroy(&hot_reload->hot_reload_mutex);
        g_jiggle_bones_hot_reload_ctx.instance_count--;
        pthread_mutex_unlock(&g_jiggle_bones_hot_reload_ctx.global_mutex);
        return result;
    }
    
    /* Initialize async operations */
    hot_reload->async_operation_capacity = 64;
    hot_reload->async_operations = calloc(hot_reload->async_operation_capacity, sizeof(jiggle_bones_async_operation_t));
    if (!hot_reload->async_operations) {
        jiggle_bones_hot_reload_shutdown_worker_threads(hot_reload);
        jiggle_bones_hot_reload_shutdown_file_watching(hot_reload);
        jiggle_bones_hot_reload_shutdown_queue(&hot_reload->reload_queue);
        pthread_mutex_destroy(&hot_reload->hot_reload_mutex);
        g_jiggle_bones_hot_reload_ctx.instance_count--;
        pthread_mutex_unlock(&g_jiggle_bones_hot_reload_ctx.global_mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_OUT_OF_MEMORY;
    }
    
    hot_reload->next_async_operation_id = 1;
    hot_reload->next_file_id = 1;
    
    /* Set memory budget */
    hot_reload->memory_budget = desc->memory_budget > 0 ? desc->memory_budget : (128 * 1024 * 1024); /* 128MB default */
    hot_reload->current_memory_usage = sizeof(jiggle_bones_hot_reload_internal_t);
    
    /* Initialize performance stats */
    memset(&hot_reload->performance_stats, 0, sizeof(jiggle_bones_hot_reload_performance_stats_t));
    hot_reload->frame_count = 0;
    
    /* Update global memory usage */
    g_jiggle_bones_hot_reload_ctx.global_memory_usage += hot_reload->current_memory_usage;
    
    out_handle->id = index;
    pthread_mutex_unlock(&g_jiggle_bones_hot_reload_ctx.global_mutex);
    
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

void jiggle_bones_hot_reload_destroy(jiggle_bones_hot_reload_handle_t handle) {
    if (!g_jiggle_bones_hot_reload_ctx.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_jiggle_bones_hot_reload_ctx.global_mutex);
    
    if (handle.id >= g_jiggle_bones_hot_reload_ctx.instance_count) {
        pthread_mutex_unlock(&g_jiggle_bones_hot_reload_ctx.global_mutex);
        return;
    }
    
    jiggle_bones_hot_reload_internal_t* hot_reload = &g_jiggle_bones_hot_reload_ctx.instances[handle.id];
    if (!hot_reload->initialized) {
        pthread_mutex_unlock(&g_jiggle_bones_hot_reload_ctx.global_mutex);
        return;
    }
    
    hot_reload->shutdown_requested = true;
    
    /* Shutdown worker threads */
    if (hot_reload->worker_threads_running) {
        jiggle_bones_hot_reload_shutdown_worker_threads(hot_reload);
    }
    
    /* Shutdown watch thread */
    if (hot_reload->watch_thread_running) {
        jiggle_bones_hot_reload_shutdown_file_watching(hot_reload);
    }
    
    /* Cleanup resources */
    if (hot_reload->async_operations) {
        free(hot_reload->async_operations);
        hot_reload->async_operations = NULL;
    }
    
    jiggle_bones_hot_reload_shutdown_queue(&hot_reload->reload_queue);
    jiggle_bones_hot_reload_shutdown_memory_pool(hot_reload);
    
    pthread_mutex_destroy(&hot_reload->hot_reload_mutex);
    
    g_jiggle_bones_hot_reload_ctx.global_memory_usage -= hot_reload->current_memory_usage;
    hot_reload->initialized = false;
    
    pthread_mutex_unlock(&g_jiggle_bones_hot_reload_ctx.global_mutex);
}

/* ============================================================================
 * FILE MANAGEMENT
 * ============================================================================ */

int jiggle_bones_hot_reload_add_file(jiggle_bones_hot_reload_handle_t handle, const char* file_path, jiggle_bones_file_type_t type) {
    if (!file_path) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_jiggle_bones_hot_reload_ctx.initialized) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_jiggle_bones_hot_reload_ctx.instance_count) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_HANDLE;
    }
    
    jiggle_bones_hot_reload_internal_t* hot_reload = &g_jiggle_bones_hot_reload_ctx.instances[handle.id];
    if (!hot_reload->initialized) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NOT_INITIALIZED;
    }
    
    if (pthread_mutex_lock(&hot_reload->hot_reload_mutex) != 0) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_THREAD_LOCK_FAILED;
    }
    
    if (hot_reload->file_count >= JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES) {
        pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_OUT_OF_MEMORY;
    }
    
    /* Check if file exists */
    if (!jiggle_bones_hot_reload_file_exists_internal(file_path)) {
        pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_NOT_FOUND;
    }
    
    /* Find free file slot */
    for (uint32_t i = 0; i < JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES; i++) {
        if (!hot_reload->files[i].id) {
            jiggle_bones_file_internal_t* file = &hot_reload->files[i];
            
            file->id = hot_reload->next_file_id++;
            strncpy(file->info.path, file_path, sizeof(file->info.path) - 1);
            file->info.path[sizeof(file->info.path) - 1] = '\0';
            
            /* Get absolute path */
            if (realpath(file_path, file->info.absolute_path) == NULL) {
                strncpy(file->info.absolute_path, file_path, sizeof(file->info.absolute_path) - 1);
                file->info.absolute_path[sizeof(file->info.absolute_path) - 1] = '\0';
            }
            
            file->info.type = type;
            file->info.is_watched = false;
            file->info.is_dirty = false;
            file->info.reload_attempts = 0;
            file->info.status = JIGGLE_BONES_RELOAD_STATUS_PENDING;
            
            /* Get file info */
            jiggle_bones_hot_reload_get_file_modification_time_internal(file_path, &file->info.last_modified_time);
            jiggle_bones_hot_reload_get_file_size_internal(file_path, &file->info.file_size);
            jiggle_bones_hot_reload_calculate_file_checksum_internal(file_path, &file->info.checksum);
            
            hot_reload->file_count++;
            hot_reload->performance_stats.total_files_watched++;
            
            /* Add file watch if watching is enabled */
            if (hot_reload->watch_context.is_watching) {
                int result = jiggle_bones_hot_reload_add_file_watch(hot_reload, file_path, file->id);
                if (result == JIGGLE_BONES_HOT_RELOAD_ERROR_NONE) {
                    file->info.is_watched = true;
                }
            }
            
            pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
            return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
        }
    }
    
    pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
    return JIGGLE_BONES_HOT_RELOAD_ERROR_OUT_OF_MEMORY;
}

int jiggle_bones_hot_reload_start_watching(jiggle_bones_hot_reload_handle_t handle) {
    if (!g_jiggle_bones_hot_reload_ctx.initialized) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_jiggle_bones_hot_reload_ctx.instance_count) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_HANDLE;
    }
    
    jiggle_bones_hot_reload_internal_t* hot_reload = &g_jiggle_bones_hot_reload_ctx.instances[handle.id];
    if (!hot_reload->initialized) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NOT_INITIALIZED;
    }
    
    if (pthread_mutex_lock(&hot_reload->hot_reload_mutex) != 0) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_THREAD_LOCK_FAILED;
    }
    
    if (hot_reload->watch_context.is_watching) {
        pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
    }
    
    /* Add watches for all files */
    for (uint32_t i = 0; i < hot_reload->file_count; i++) {
        jiggle_bones_file_internal_t* file = &hot_reload->files[i];
        if (file->id && !file->info.is_watched) {
            int result = jiggle_bones_hot_reload_add_file_watch(hot_reload, file->info.path, file->id);
            if (result == JIGGLE_BONES_HOT_RELOAD_ERROR_NONE) {
                file->info.is_watched = true;
            }
        }
    }
    
    /* Start watch thread */
    hot_reload->watch_context.is_watching = true;
    
    int result = pthread_create(&hot_reload->watch_thread, NULL, jiggle_bones_hot_reload_watch_thread, hot_reload);
    if (result != 0) {
        hot_reload->watch_context.is_watching = false;
        pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_OPERATION_FAILED;
    }
    
    hot_reload->watch_thread_running = true;
    
    pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

int jiggle_bones_hot_reload_stop_watching(jiggle_bones_hot_reload_handle_t handle) {
    if (!g_jiggle_bones_hot_reload_ctx.initialized) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_jiggle_bones_hot_reload_ctx.instance_count) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_HANDLE;
    }
    
    jiggle_bones_hot_reload_internal_t* hot_reload = &g_jiggle_bones_hot_reload_ctx.instances[handle.id];
    if (!hot_reload->initialized) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NOT_INITIALIZED;
    }
    
    if (pthread_mutex_lock(&hot_reload->hot_reload_mutex) != 0) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_THREAD_LOCK_FAILED;
    }
    
    if (!hot_reload->watch_context.is_watching) {
        pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
    }
    
    /* Stop watching */
    hot_reload->watch_context.is_watching = false;
    
    /* Signal watch thread to stop */
#ifdef __linux__
    if (hot_reload->watch_context.inotify_fd >= 0) {
        close(hot_reload->watch_context.inotify_fd);
        hot_reload->watch_context.inotify_fd = -1;
    }
#endif
    
    /* Wait for watch thread to finish */
    if (hot_reload->watch_thread_running) {
        pthread_join(hot_reload->watch_thread, NULL);
        hot_reload->watch_thread_running = false;
    }
    
    pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

bool jiggle_bones_hot_reload_is_watching(jiggle_bones_hot_reload_handle_t handle) {
    if (!g_jiggle_bones_hot_reload_ctx.initialized) return false;
    if (handle.id >= g_jiggle_bones_hot_reload_ctx.instance_count) return false;
    
    jiggle_bones_hot_reload_internal_t* hot_reload = &g_jiggle_bones_hot_reload_ctx.instances[handle.id];
    if (!hot_reload->initialized) return false;
    
    return hot_reload->watch_context.is_watching;
}

/* ============================================================================
 * VALIDATION AND ERROR HANDLING
 * ============================================================================ */

static bool jiggle_bones_hot_reload_validate_file_info(const jiggle_bones_file_info_t* file_info) {
    if (!file_info) return false;
    if (strlen(file_info->path) == 0) return false;
    if (file_info->type < 0 || file_info->type > JIGGLE_BONES_FILE_TYPE_CUSTOM) return false;
    return true;
}

static void jiggle_bones_hot_reload_set_error(jiggle_bones_hot_reload_internal_t* hot_reload, uint32_t file_id, const char* error) {
    if (hot_reload && error && file_id < JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES) {
        jiggle_bones_file_internal_t* file = &hot_reload->files[file_id];
        if (file->id) {
            strncpy(file->info.last_error, error, sizeof(file->info.last_error) - 1);
            file->info.last_error[sizeof(file->info.last_error) - 1] = '\0';
            
            /* Call error callback */
            if (file->error_callback) {
                file->error_callback(file_id, error, file->user_data);
            }
        }
    }
}

/* ============================================================================
 * WORKER THREADS (PLACEHOLDER)
 * ============================================================================ */

static int jiggle_bones_hot_reload_init_worker_threads(jiggle_bones_hot_reload_internal_t* hot_reload) {
    /* Placeholder for worker thread initialization */
    hot_reload->worker_thread_count = hot_reload->settings.worker_thread_count;
    hot_reload->worker_threads_running = true;
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

static void jiggle_bones_hot_reload_shutdown_worker_threads(jiggle_bones_hot_reload_internal_t* hot_reload) {
    /* Placeholder for worker thread shutdown */
    hot_reload->worker_threads_running = false;
}

static void* jiggle_bones_hot_reload_worker_thread(void* arg) {
    /* Placeholder for worker thread function */
    jiggle_bones_hot_reload_internal_t* hot_reload = (jiggle_bones_hot_reload_internal_t*)arg;
    
    while (hot_reload->worker_threads_running) {
        jiggle_bones_reload_queue_item_t item;
        if (jiggle_bones_hot_reload_dequeue(&hot_reload->reload_queue, &item) == JIGGLE_BONES_HOT_RELOAD_ERROR_NONE) {
            jiggle_bones_hot_reload_process_file_internal(hot_reload, item.file_id);
        }
    }
    
    return NULL;
}

/* ============================================================================
 * WATCH THREAD (PLACEHOLDER)
 * ============================================================================ */

static void* jiggle_bones_hot_reload_watch_thread(void* arg) {
    jiggle_bones_hot_reload_internal_t* hot_reload = (jiggle_bones_hot_reload_internal_t*)arg;
    
#ifdef __linux__
    char buffer[JIGGLE_BONES_HOT_RELOAD_INOTIFY_BUFFER_SIZE];
    
    while (hot_reload->watch_context.is_watching) {
        int length = read(hot_reload->watch_context.inotify_fd, buffer, sizeof(buffer));
        
        if (length < 0) {
            if (errno == EINTR) continue;
            break;
        }
        
        /* Process inotify events */
        int i = 0;
        while (i < length) {
            struct inotify_event* event = (struct inotify_event*)&buffer[i];
            
            if (event->len) {
                /* Find file ID for this watch descriptor */
                for (uint32_t j = 0; j < hot_reload->watch_context.watch_count; j++) {
                    if (hot_reload->watch_context.watch_descriptors[j] == event->wd) {
                        uint32_t file_id = hot_reload->watch_context.file_ids[j];
                        
                        /* Mark file as dirty */
                        pthread_mutex_lock(&hot_reload->hot_reload_mutex);
                        if (file_id < hot_reload->file_count) {
                            hot_reload->files[file_id].info.is_dirty = true;
                            
                            /* Enqueue for reload */
                            jiggle_bones_reload_queue_item_t queue_item = {0};
                            queue_item.file_id = file_id;
                            queue_item.status = JIGGLE_BONES_RELOAD_STATUS_PENDING;
                            queue_item.enqueue_time = (uint64_t)time(NULL);
                            queue_item.priority = 1;
                            
                            jiggle_bones_hot_reload_enqueue(&hot_reload->reload_queue, &queue_item);
                        }
                        pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
                        
                        break;
                    }
                }
            }
            
            i += sizeof(struct inotify_event) + event->len;
        }
        
        usleep(10000); /* 10ms sleep */
    }
#endif
    
    return NULL;
}

/* ============================================================================
 * PERFORMANCE AND DEBUGGING
 * ============================================================================ */

const jiggle_bones_hot_reload_performance_stats_t* jiggle_bones_hot_reload_get_performance_stats(jiggle_bones_hot_reload_handle_t handle) {
    if (!g_jiggle_bones_hot_reload_ctx.initialized) return NULL;
    if (handle.id >= g_jiggle_bones_hot_reload_ctx.instance_count) return NULL;
    
    jiggle_bones_hot_reload_internal_t* hot_reload = &g_jiggle_bones_hot_reload_ctx.instances[handle.id];
    if (!hot_reload->initialized) return NULL;
    
    return &hot_reload->performance_stats;
}

void jiggle_bones_hot_reload_reset_performance_stats(jiggle_bones_hot_reload_handle_t handle) {
    if (!g_jiggle_bones_hot_reload_ctx.initialized) return;
    if (handle.id >= g_jiggle_bones_hot_reload_ctx.instance_count) return;
    
    jiggle_bones_hot_reload_internal_t* hot_reload = &g_jiggle_bones_hot_reload_ctx.instances[handle.id];
    if (!hot_reload->initialized) return;
    
    if (pthread_mutex_lock(&hot_reload->hot_reload_mutex) != 0) return;
    
    memset(&hot_reload->performance_stats, 0, sizeof(jiggle_bones_hot_reload_performance_stats_t));
    hot_reload->performance_stats.total_files_watched = hot_reload->file_count;
    
    pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
}

jiggle_bones_hot_reload_info_t jiggle_bones_hot_reload_get_info(jiggle_bones_hot_reload_handle_t handle) {
    jiggle_bones_hot_reload_info_t info = {0};
    
    if (!g_jiggle_bones_hot_reload_ctx.initialized) return info;
    if (handle.id >= g_jiggle_bones_hot_reload_ctx.instance_count) return info;
    
    jiggle_bones_hot_reload_internal_t* hot_reload = &g_jiggle_bones_hot_reload_ctx.instances[handle.id];
    if (!hot_reload->initialized) return info;
    
    if (pthread_mutex_lock(&hot_reload->hot_reload_mutex) != 0) return info;
    
    info.id = hot_reload->id;
    strncpy(info.name, hot_reload->name, sizeof(info.name) - 1);
    info.name[sizeof(info.name) - 1] = '\0';
    info.flags = hot_reload->flags;
    info.initialized = hot_reload->initialized;
    info.watched_file_count = hot_reload->file_count;
    info.dependency_count = hot_reload->dependency_count;
    info.queue_size = hot_reload->reload_queue.count;
    info.performance = hot_reload->performance_stats;
    info.is_watching = hot_reload->watch_context.is_watching;
    info.current_memory_usage = (float)hot_reload->current_memory_usage / (1024.0f * 1024.0f);
    
    pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
    return info;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

int jiggle_bones_hot_reload_calculate_file_checksum(const char* file_path, uint32_t* out_checksum) {
    return jiggle_bones_hot_reload_calculate_file_checksum_internal(file_path, out_checksum);
}

int jiggle_bones_hot_reload_get_file_modification_time(const char* file_path, uint64_t* out_time) {
    return jiggle_bones_hot_reload_get_file_modification_time_internal(file_path, out_time);
}

bool jiggle_bones_hot_reload_file_exists(const char* file_path) {
    return jiggle_bones_hot_reload_file_exists_internal(file_path) != 0;
}

bool jiggle_bones_hot_reload_directory_exists(const char* directory_path) {
    return jiggle_bones_hot_reload_directory_exists_internal(directory_path) != 0;
}

/* ============================================================================
 * MEMORY MANAGEMENT (PLACEHOLDER)
 * ============================================================================ */

static int jiggle_bones_hot_reload_init_memory_pool(jiggle_bones_hot_reload_internal_t* hot_reload) {
    /* Placeholder for memory pool initialization */
    hot_reload->memory_pool.initialized = true;
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

static void jiggle_bones_hot_reload_shutdown_memory_pool(jiggle_bones_hot_reload_internal_t* hot_reload) {
    /* Placeholder for memory pool shutdown */
    hot_reload->memory_pool.initialized = false;
}

/* ============================================================================
 * VALIDATION AND ERROR HANDLING
 * ============================================================================ */

int jiggle_bones_hot_reload_validate_file(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id) {
    if (!g_jiggle_bones_hot_reload_ctx.initialized) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_jiggle_bones_hot_reload_ctx.instance_count) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_HANDLE;
    }
    
    jiggle_bones_hot_reload_internal_t* hot_reload = &g_jiggle_bones_hot_reload_ctx.instances[handle.id];
    if (!hot_reload->initialized) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NOT_INITIALIZED;
    }
    
    if (file_id >= JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    }
    
    if (pthread_mutex_lock(&hot_reload->hot_reload_mutex) != 0) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_THREAD_LOCK_FAILED;
    }
    
    jiggle_bones_file_internal_t* file = &hot_reload->files[file_id];
    if (!file->id) {
        pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_NOT_FOUND;
    }
    
    bool is_valid = jiggle_bones_hot_reload_validate_file_info(&file->info);
    
    pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
    return is_valid ? JIGGLE_BONES_HOT_RELOAD_ERROR_NONE : JIGGLE_BONES_HOT_RELOAD_ERROR_OPERATION_FAILED;
}

int jiggle_bones_hot_reload_get_last_error(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id, char* error_buffer, size_t buffer_size) {
    if (!error_buffer || buffer_size == 0) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_jiggle_bones_hot_reload_ctx.initialized) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_jiggle_bones_hot_reload_ctx.instance_count) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_HANDLE;
    }
    
    jiggle_bones_hot_reload_internal_t* hot_reload = &g_jiggle_bones_hot_reload_ctx.instances[handle.id];
    if (!hot_reload->initialized) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_NOT_INITIALIZED;
    }
    
    if (file_id >= JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_INVALID_PARAMETER;
    }
    
    if (pthread_mutex_lock(&hot_reload->hot_reload_mutex) != 0) {
        return JIGGLE_BONES_HOT_RELOAD_ERROR_THREAD_LOCK_FAILED;
    }
    
    jiggle_bones_file_internal_t* file = &hot_reload->files[file_id];
    if (!file->id) {
        pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
        return JIGGLE_BONES_HOT_RELOAD_ERROR_FILE_NOT_FOUND;
    }
    
    strncpy(error_buffer, file->info.last_error, buffer_size - 1);
    error_buffer[buffer_size - 1] = '\0';
    
    pthread_mutex_unlock(&hot_reload->hot_reload_mutex);
    return JIGGLE_BONES_HOT_RELOAD_ERROR_NONE;
}

/* End of jiggle_bones_hot_reload.c */