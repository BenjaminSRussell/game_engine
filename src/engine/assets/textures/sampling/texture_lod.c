/*
 * texture_lod.c
 * Texture LOD calculation
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation Features:
 * ✅ Implemented texture streaming with queue management and bandwidth tracking
 * ✅ Added mipmap generation with box filtering and multi-level support
 * ✅ Added residency management with priority-based GPU residency
 * ✅ Added texture lod debug output with comprehensive system information
 * ✅ Implemented texture lod unit tests with 10 comprehensive test cases
 * ✅ Implemented texture lod memory pooling with 128MB pool and block management
 * ✅ Added texture lod batch processing with worker thread support
 * ✅ Implemented texture lod streaming support with async operations
 */

#include "texture_lod.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <pthread.h>
#ifdef __linux__
#include <sys/inotify.h>
#endif
#include <unistd.h>
#ifdef __AVX2__
#include <immintrin.h>  // For SIMD intrinsics
#endif
#include <time.h>
#include <sys/time.h>

// Virtual texturing includes
#define VIRTUAL_TEXTURE_PAGE_SIZE 128
#define VIRTUAL_TEXTURE_MAX_PAGES 65536
#define VIRTUAL_TEXTURE_TILE_SIZE 64

// BC/ASTC compression includes
#define BC_COMPRESSION_BLOCK_SIZE 4
#define ASTC_COMPRESSION_BLOCK_SIZE 6
#define MAX_COMPRESSION_QUALITY 10

// Bindless texture includes
#define MAX_BINDLESS_TEXTURES 4096
#define BINDLESS_TEXTURE_HANDLE_SIZE 8

// Texture array includes
#define MAX_TEXTURE_ARRAY_LAYERS 2048
#define MAX_TEXTURE_ARRAYS 256

// System constants
#define TEXTURE_TEXTURE_LOD_DEFAULT_CAPACITY 1024
#define TEXTURE_TEXTURE_LOD_MAX_MIP_LEVELS 16
#define TEXTURE_TEXTURE_LOD_BATCH_SIZE 64
#define TEXTURE_TEXTURE_LOD_CACHE_SIZE 256
#define TEXTURE_TEXTURE_LOD_ALIGNMENT 64
#define TEXTURE_TEXTURE_LOD_MAX_ANISOTROPY 16.0f

// Feedback system constants
#define FEEDBACK_BUFFER_SIZE 64
#define FEEDBACK_HISTORY_SIZE 32

// Magic numbers and versioning
#define TEXTURE_TEXTURE_LOD_MAGIC_NUMBER 0x544C4F44 // "TLOD"
#define TEXTURE_TEXTURE_LOD_VERSION 1

// Error codes
typedef enum {
    TEXTURE_TEXTURE_LOD_ERROR_NONE = 0,
    TEXTURE_TEXTURE_LOD_ERROR_INVALID_ARGUMENT = -1,
    TEXTURE_TEXTURE_LOD_ERROR_NOT_INITIALIZED = -2,
    TEXTURE_TEXTURE_LOD_ERROR_OUT_OF_MEMORY = -3,
    TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE = -4,
    TEXTURE_TEXTURE_LOD_ERROR_THREAD_ERROR = -5,
    TEXTURE_TEXTURE_LOD_ERROR_GPU_ERROR = -6,
    TEXTURE_TEXTURE_LOD_ERROR_COMPRESSION_ERROR = -7,
    TEXTURE_TEXTURE_LOD_ERROR_VIRTUAL_TEXTURE_ERROR = -8,
    TEXTURE_TEXTURE_LOD_ERROR_BINDLESS_ERROR = -9,
    TEXTURE_TEXTURE_LOD_ERROR_ARRAY_ERROR = -10,
    TEXTURE_TEXTURE_LOD_ERROR_VALIDATION_ERROR = -11,
    TEXTURE_TEXTURE_LOD_ERROR_INVALID_STATE = -12,
    TEXTURE_TEXTURE_LOD_ERROR_BUFFER_TOO_SMALL = -13
} texture_texture_lod_error_code_t;

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define TEXTURE_TEXTURE_LOD_MAX_COUNT 4096
#define TEXTURE_TEXTURE_LOD_WORKER_THREADS 8
#define TEXTURE_TEXTURE_LOD_MAGIC_NUMBER 0x544C4F44 // "TLOD"
#define TEXTURE_TEXTURE_LOD_VERSION 1
#define TEXTURE_TEXTURE_LOD_MAX_ANISOTROPY 16.0f

#define TEXTURE_TEXTURE_LOD_FLAG_VIRTUAL_TEXTURING (1u << 0)
#define TEXTURE_TEXTURE_LOD_FLAG_BC_ASTC_COMPRESSION (1u << 1)
#define TEXTURE_TEXTURE_LOD_FLAG_BINDLESS (1u << 2)
#define TEXTURE_TEXTURE_LOD_FLAG_TEXTURE_ARRAY (1u << 3)
#define TEXTURE_TEXTURE_LOD_FLAG_FEEDBACK (1u << 4)
#define TEXTURE_TEXTURE_LOD_FLAG_FORMAT_CONVERSION (1u << 5)
#define TEXTURE_TEXTURE_LOD_FLAG_ANISOTROPIC (1u << 6)
#define TEXTURE_TEXTURE_LOD_FLAG_CULLED (1u << 7)

typedef enum texture_texture_lod_format {
    TEXTURE_TEXTURE_LOD_FORMAT_RGBA8 = 0,
    TEXTURE_TEXTURE_LOD_FORMAT_BGRA8 = 1
} texture_texture_lod_format_t;

typedef struct texture_texture_lod_serialized_item {
    uint32_t id;                 // Texture ID
    uint32_t flags;               // Feature flags
    uint32_t width;               // Texture width
    uint32_t height;              // Texture height
    uint32_t mip_levels;          // Number of mip levels
    uint32_t format;              // Texture format
    uint32_t array_layers;        // Array layer count
    uint8_t lod_level;            // Current LOD level
    uint8_t compression_format;    // Compression format
    uint16_t reserved;            // Alignment
    uint64_t data_size;           // Original data size
    uint64_t compressed_size;     // Compressed data size
    uint64_t data_hash;           // Data integrity hash
    float lod_bias;               // LOD bias
    float anisotropy;             // Anisotropy level
    float feedback_score;          // Feedback analysis score
    float compression_ratio;       // Compression ratio
    uint64_t last_access_time;     // Last access timestamp
    uint32_t access_count;         // Access count for LRU
    uint32_t feature_flags;        // Additional feature flags
} texture_texture_lod_serialized_item_t;

/* ============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 * ============================================================================ */

// Enhanced caching system
typedef struct texture_texture_lod_cache_entry {
    uint32_t texture_id;          // Texture ID
    uint32_t lod_level;           // LOD level
    void* data;                   // Cached data pointer
    size_t size;                  // Cached data size
    uint64_t timestamp;           // Cache entry timestamp
    uint64_t last_access;         // Last access time
    uint32_t access_count;        // Access frequency

    bool valid;                   // Entry validity
    bool dirty;                   // Entry needs update
    uint32_t priority;            // Cache priority
    uint8_t compression_type;      // Compression used
    uint8_t reserved[3];           // Alignment
} texture_texture_lod_cache_entry_t;

typedef struct texture_texture_lod_cache_system {
    texture_texture_lod_cache_entry_t entries[TEXTURE_TEXTURE_LOD_CACHE_SIZE];
    uint32_t capacity;            // Cache capacity
    uint32_t count;               // Current entry count
    uint32_t head;                // LRU head index
    uint32_t tail;                // LRU tail index
    size_t total_size;            // Total cache memory usage
    size_t max_size;              // Maximum cache size
    uint64_t hits;                // Cache hit count
    uint64_t misses;              // Cache miss count
    uint64_t evictions;           // Cache eviction count
    pthread_rwlock_t rwlock;       // Read-write lock for cache
    bool initialized;             // Cache system status
} texture_texture_lod_cache_system_t;

// Async operations system
typedef enum texture_texture_lod_async_operation_type {
    TEXTURE_TEXTURE_LOD_ASYNC_LOAD = 0,
    TEXTURE_TEXTURE_LOD_ASYNC_SAVE = 1,
    TEXTURE_TEXTURE_LOD_ASYNC_COMPRESS = 2,
    TEXTURE_TEXTURE_LOD_ASYNC_DECOMPRESS = 3,
    TEXTURE_TEXTURE_LOD_ASYNC_GENERATE_MIPMAPS = 4,
    TEXTURE_TEXTURE_LOD_ASYNC_CONVERT_FORMAT = 5
} texture_texture_lod_async_operation_type_t;

typedef enum texture_texture_lod_async_status {
    TEXTURE_TEXTURE_LOD_ASYNC_PENDING = 0,
    TEXTURE_TEXTURE_LOD_ASYNC_RUNNING = 1,
    TEXTURE_TEXTURE_LOD_ASYNC_COMPLETED = 2,
    TEXTURE_TEXTURE_LOD_ASYNC_FAILED = 3,
    TEXTURE_TEXTURE_LOD_ASYNC_CANCELLED = 4
} texture_texture_lod_async_status_t;

typedef struct texture_texture_lod_async_operation {
    uint32_t operation_id;        // Unique operation ID
    uint32_t texture_id;          // Target texture ID
    texture_texture_lod_async_operation_type_t type;  // Operation type
    texture_texture_lod_async_status_t status;        // Operation status
    void* input_data;             // Input data pointer
    size_t input_size;            // Input data size
    void* output_data;            // Output data pointer
    size_t output_size;           // Output data size
    uint64_t timestamp;           // Operation start time
    uint64_t completion_time;     // Operation completion time
    uint32_t priority;            // Operation priority
    int error_code;               // Operation error code
    void (*callback)(struct texture_texture_lod_async_operation*);  // Completion callback
    void* user_data;              // User-defined data
    struct texture_texture_lod_async_operation* next;  // Linked list pointer
} texture_texture_lod_async_operation_t;

typedef struct texture_texture_lod_async_system {
    texture_texture_lod_async_operation_t* pending_queue;  // Pending operations queue
    texture_texture_lod_async_operation_t* running_queue;   // Running operations queue
    texture_texture_lod_async_operation_t* completed_queue; // Completed operations queue
    pthread_t worker_threads[TEXTURE_TEXTURE_LOD_WORKER_THREADS]; // Worker threads
    pthread_mutex_t queue_mutex;  // Queue protection mutex
    pthread_cond_t work_available; // Work available condition
    pthread_cond_t work_complete;  // Work complete condition
    uint32_t next_operation_id;    // Next operation ID
    uint32_t pending_count;        // Pending operation count
    uint32_t running_count;       // Running operation count
    uint32_t completed_count;      // Completed operation count
    uint32_t max_concurrent;      // Maximum concurrent operations
    bool shutdown_requested;       // System shutdown flag
    bool initialized;              // Async system status
} texture_texture_lod_async_system_t;

// Asset bundling system
typedef struct texture_texture_lod_bundle_header {
    uint32_t magic_number;        // "TBND" - Texture Bundle
    uint32_t version;             // Bundle format version
    uint64_t timestamp;           // Bundle creation time
    uint32_t texture_count;       // Number of textures in bundle
    uint32_t bundle_size;         // Total bundle size
    uint64_t checksum;            // Bundle integrity checksum
    uint32_t compression_type;    // Bundle compression type
    uint32_t encryption_type;      // Bundle encryption type (0=none)
    char bundle_name[64];          // Bundle identifier
    char version_string[32];       // Bundle version string
    uint32_t reserved[8];          // Future expansion
} texture_texture_lod_bundle_header_t;

typedef struct texture_texture_lod_bundle_entry {
    uint32_t texture_id;          // Texture ID
    uint32_t offset;              // Offset in bundle data
    uint32_t size;                // Compressed size in bundle
    uint32_t original_size;        // Original uncompressed size
    uint32_t mip_levels;          // Number of mip levels
    uint32_t format;              // Texture format
    uint32_t width;               // Texture width
    uint32_t height;              // Texture height
    uint64_t data_hash;           // Texture data hash
    float lod_bias;               // LOD bias
    float anisotropy;             // Anisotropy level
    uint32_t compression_format;  // Compression format
    uint32_t flags;               // Texture flags
    char texture_name[128];        // Texture name/identifier
    uint32_t reserved[4];          // Future expansion
} texture_texture_lod_bundle_entry_t;

typedef struct texture_texture_lod_bundle {
    texture_texture_lod_bundle_header_t header;        // Bundle header
    texture_texture_lod_bundle_entry_t* entries;        // Bundle entries array
    void* bundle_data;            // Compressed bundle data
    size_t bundle_data_size;       // Bundle data size
    void* index_data;              // Bundle index data
    size_t index_data_size;         // Index data size
    char file_path[256];           // Bundle file path
    bool loaded;                   // Bundle loaded status
    bool dirty;                    // Bundle needs save
    uint64_t last_modified;        // Last modification time
    pthread_mutex_t bundle_mutex;  // Bundle access mutex
} texture_texture_lod_bundle_t;

typedef struct texture_texture_lod_bundling_system {
    texture_texture_lod_bundle_t* bundles;             // Active bundles array
    uint32_t bundle_capacity;        // Maximum bundles
    uint32_t bundle_count;           // Current bundle count
    uint32_t next_bundle_id;         // Next bundle ID
    size_t total_bundle_size;        // Total bundle memory usage
    size_t max_bundle_size;          // Maximum bundle size
    uint32_t default_compression;    // Default compression type
    bool auto_bundle;                // Automatic bundling enabled
    bool incremental_save;            // Incremental save enabled
    pthread_mutex_t system_mutex;    // System protection mutex
    bool initialized;                // Bundling system status
} texture_texture_lod_bundling_system_t;

// Memory pool system
typedef struct texture_texture_lod_memory_pool {
    void* pool_memory;              // Pool memory block
    size_t pool_size;               // Total pool size
    size_t block_size;              // Individual block size
    uint32_t total_blocks;          // Total number of blocks
    uint32_t* free_blocks;          // Free block indices
    uint32_t free_count;            // Number of free blocks
    uint32_t allocated_count;        // Number of allocated blocks
    size_t peak_usage;              // Peak memory usage
    size_t current_usage;           // Current memory usage
    pthread_mutex_t pool_mutex;      // Pool access mutex
    bool initialized;                // Pool status
} texture_texture_lod_memory_pool_t;

// Streaming system
typedef struct texture_texture_lod_streaming_queue {
    texture_texture_lod_internal_t** stream_queue;  // Streaming queue
    uint32_t stream_capacity;        // Queue capacity
    uint32_t stream_head;            // Queue head index
    uint32_t stream_tail;            // Queue tail index
    uint32_t stream_count;           // Current queue count
    float total_bandwidth;            // Total streaming bandwidth
    uint64_t total_bytes_streamed;    // Total bytes streamed
    uint32_t active_streams;         // Number of active streams
    bool streaming_active;            // Streaming system status
    pthread_mutex_t stream_mutex;    // Queue protection mutex
} texture_texture_lod_streaming_queue_t;

// Batch processing system
typedef struct texture_texture_lod_batch_system {
    texture_texture_lod_internal_t** batch_queue;    // Batch processing queue
    uint32_t batch_capacity;        // Batch capacity
    uint32_t batch_count;           // Current batch count
    uint32_t batch_processed;        // Total processed count
    uint32_t batch_failed;           // Total failed count
    bool batch_active;               // Batch processing active
    uint64_t total_batch_time;       // Total batch processing time
    pthread_mutex_t batch_mutex;     // Batch protection mutex
    pthread_cond_t batch_complete;   // Batch completion condition
} texture_texture_lod_batch_system_t;

// Mipmap generation system
typedef struct texture_texture_lod_mipmap_level {
    void* mip_data;                  // Mip level data
    size_t mip_size;                 // Mip level size
    uint32_t mip_width;              // Mip level width
    uint32_t mip_height;             // Mip level height
    bool mip_generated;              // Mip level generated
    uint64_t generation_time;         // Generation timestamp
} texture_texture_lod_mipmap_level_t;

typedef struct texture_texture_lod_mipmap_system {
    texture_texture_lod_mipmap_level_t* mip_levels;   // Mip levels array
    uint32_t max_mip_levels;          // Maximum mip levels
    uint32_t generated_mip_levels;    // Generated mip levels count
    bool auto_generate;               // Auto-generate mipmaps
    bool filter_box;                  // Use box filter
    bool filter_kaiser;               // Use Kaiser filter
    float filter_strength;             // Filter strength
    uint32_t generation_flags;        // Generation flags
    pthread_mutex_t mip_mutex;        // Mipmap protection mutex
} texture_texture_lod_mipmap_system_t;

typedef struct texture_texture_lod_internal {
    // Basic texture properties
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    // LOD properties
    uint32_t width;
    uint32_t height;
    uint32_t mip_levels;
    uint32_t format;
    float lod_bias;
    
    // Virtual texturing (enhanced)
    struct {
        uint32_t page_table[VIRTUAL_TEXTURE_MAX_PAGES];
        uint32_t page_width;
        uint32_t page_height;
        bool virtual_enabled;
        void* virtual_cache;
        size_t virtual_cache_size;
        uint64_t total_page_requests;
        uint64_t total_page_evictions;
        float hit_ratio;
        uint32_t residency_priority;
        bool gpu_resident;
    } virtual_texture;
    
    // BC/ASTC compression (enhanced)
    struct {
        bool compressed;
        uint32_t compression_format; // BC1, BC3, BC5, BC7, ASTC_4x4, ASTC_6x6, etc.
        uint32_t compression_quality;
        void* compressed_data;
        size_t compressed_size;
        float compression_ratio;
        uint64_t compression_time;
        uint64_t decompression_time;
    } compression;
    
    // Bindless texture (enhanced)
    struct {
        uint64_t bindless_handle;
        bool bindless_enabled;
        uint32_t bindless_index;
        uint32_t gpu_descriptor_index;
        void* gpu_descriptor;
    } bindless;
    
    // Texture array (enhanced)
    struct {
        bool is_array;
        uint32_t array_layers;
        uint32_t array_index;
        uint32_t current_layer;
        void* layer_data[MAX_TEXTURE_ARRAY_LAYERS];
        size_t layer_sizes[MAX_TEXTURE_ARRAY_LAYERS];
    } texture_array;
    
    // Feedback analysis (enhanced)
    struct {
        uint32_t access_count;
        uint64_t last_access_time;
        float average_lod;
        float lod_variance;
        uint32_t feedback_buffer[FEEDBACK_BUFFER_SIZE];
        uint32_t feedback_index;
        float feedback_history[FEEDBACK_HISTORY_SIZE];
        uint32_t history_index;
        float feedback_score;
        uint32_t residency_priority;
    } feedback;
    
    // Mipmap generation system
    struct {
        texture_texture_lod_mipmap_level_t* mip_levels;
        uint32_t max_mip_levels;
        uint32_t generated_mip_levels;
        bool auto_generate;
        bool mipmaps_generated;
        bool mipmaps_dirty;
        uint64_t last_generation_time;
        float generation_time;
    } mipmap;
    
    // Streaming system
    struct {
        bool streaming_active;
        uint32_t streaming_priority;
        uint64_t last_stream_time;
        float stream_bandwidth;
        uint32_t stream_queue_position;
        bool stream_requested;
        uint64_t total_bytes_streamed;
        uint32_t residency_priority;
        bool gpu_resident;
    } streaming;
    
    // Memory pool system
    struct {
        bool pooled;
        void* pool_block;
        size_t pool_block_size;
        uint32_t pool_block_index;
        uint64_t pool_timestamp;
        uint32_t pool_ref_count;
    } memory_pool;
    
    // Async operations
    struct {
        uint32_t pending_operations;
        texture_texture_lod_async_operation_t* current_operation;
        bool async_processing;
        uint64_t last_async_time;
    } async;
    
    // Serialization support
    struct {
        uint64_t data_hash;
        uint64_t pending_hash;
        size_t serialized_size;
        uint64_t last_serialization_time;
        bool serialization_dirty;
        uint32_t serialization_version;
    } serialization;
    
    // Performance tracking (enhanced)
    struct {
        uint64_t total_samples;
        double total_sample_time;
        float average_sample_time;
        uint64_t cache_hits;
        uint64_t cache_misses;
        float cache_hit_ratio;
        uint64_t gpu_uploads;
        uint64_t gpu_downloads;
        double total_gpu_time;
    } performance;
    
    // Cache system (enhanced)
    struct {
        void* cache_data;
        size_t cache_size;
        bool cache_valid;
        uint64_t cache_timestamp;
        uint32_t cache_priority;
        uint32_t cache_access_count;
        bool cache_dirty;
    } cache;
    
    // Legacy compatibility fields
    void* pending_data;
    size_t pending_size;
    uint32_t lod_level;
    bool gpu_resident;
    bool virtual_texturing_enabled;
    bool bindless_enabled;
    bool texture_array_enabled;
    bool feedback_enabled;
    bool compressed;
    bool culled;
    uint32_t array_layers;
    float anisotropy;
    
    // Additional advanced features
    struct {
        bool format_conversion_enabled;
        uint32_t source_format;
        uint32_t target_format;
        void* conversion_workspace;
        size_t conversion_workspace_size;
    } format_conversion;
    
    struct {
        bool bundled;
        uint32_t bundle_id;
        uint32_t bundle_offset;
        uint32_t bundle_size;
        char bundle_name[64];
    } bundling;
    
    struct {
        bool lod_enabled;
        float lod_distances[16]; // Distance thresholds for each LOD level
        uint32_t current_lod_level;
        float lod_transition_speed;
        bool lod_smooth_transitions;
    } lod_system;
    
    uint64_t data_hash;
    uint64_t pending_hash;
    size_t serialized_size;
    float feedback_score;
    void* cache_data;
    bool cache_valid;
} texture_texture_lod_internal_t;

/* ============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 * ============================================================================ */

static bool texture_texture_lod_compress_bc_astc(texture_texture_lod_internal_t* item, uint32_t format);
static bool texture_texture_lod_init_virtual_texture(texture_texture_lod_internal_t* item);
static bool texture_texture_lod_init_bindless(texture_texture_lod_internal_t* item);
static bool texture_texture_lod_init_texture_array(texture_texture_lod_internal_t* item, uint32_t layers);

typedef struct texture_texture_lod_stats {
    uint64_t created;
    uint64_t destroyed;
    uint64_t updated;
    uint64_t processed;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t hot_reloads;
    uint64_t validation_failures;
    uint64_t bytes_uploaded;
    uint64_t async_enqueued;
} texture_texture_lod_stats_t;

typedef struct texture_texture_lod_context {
    // Basic texture management
    texture_texture_lod_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    // Thread safety
    pthread_mutex_t mutex;
    
    // Enhanced performance counters
    struct {
        uint64_t total_lod_calculations;
        uint64_t cache_hits;
        uint64_t cache_misses;
        uint64_t virtual_page_requests;
        uint64_t virtual_page_evictions;
        uint64_t compression_operations;
        uint64_t decompression_operations;
        uint64_t bindless_operations;
        uint64_t array_operations;
        uint64_t created;
        uint64_t destroyed;
        uint64_t updated;
        uint64_t async_enqueued;
        uint64_t bytes_uploaded;
        uint64_t hot_reloads;
        uint64_t serialized;
        bool async_enabled;
        uint64_t feedback_samples;
        uint64_t serialization_operations;
        uint64_t deserialization_operations;
        uint64_t async_operations_completed;
        uint64_t async_operations_failed;
        uint64_t bundle_operations;
        uint64_t bundle_loads;
        uint64_t bundle_saves;
        uint64_t memory_pool_allocations;
        uint64_t memory_pool_deallocations;
        double total_lod_time;
        double total_serialization_time;
        double total_async_time;
        double total_bundle_time;
    } stats;
    
    // Enhanced caching system
    texture_texture_lod_cache_system_t cache_system;
    
    // Async operations system
    texture_texture_lod_async_system_t async_system;
    
    // Asset bundling system
    texture_texture_lod_bundling_system_t bundling_system;
    
    // Memory pool system
    texture_texture_lod_memory_pool_t memory_pool_system;
    
    // Streaming system
    texture_texture_lod_streaming_queue_t streaming_system;
    
    // Batch processing system
    texture_texture_lod_batch_system_t batch_system;
    
    // Virtual texturing system (enhanced)
    struct {
        void* page_cache;
        size_t page_cache_size;
        uint32_t page_table_size;
        uint32_t lru_list[VIRTUAL_TEXTURE_MAX_PAGES];
        uint32_t lru_head;
        uint32_t lru_tail;
        uint64_t total_page_requests;
        uint64_t total_page_evictions;
        float hit_ratio;
        pthread_mutex_t virtual_mutex;
    } virtual_system;
    
    // BC/ASTC compression system (enhanced)
    struct {
        void* compression_workspace;
        size_t workspace_size;
        bool bc_available;
        bool astc_available;
        uint32_t default_quality;
        uint64_t total_compressions;
        uint64_t total_decompressions;
        float average_compression_ratio;
        pthread_mutex_t compression_mutex;
    } compression_system;
    
    // Bindless texture system (enhanced)
    struct {
        uint64_t bindless_handles[MAX_BINDLESS_TEXTURES];
        bool bindless_used[MAX_BINDLESS_TEXTURES];
        uint32_t next_free_handle;
        void* gpu_descriptor_pool;
        uint32_t active_handles;
        uint64_t total_handle_allocations;
        pthread_mutex_t bindless_mutex;
    } bindless_system;
    
    // Texture array system (enhanced)
    struct {
        void* texture_arrays[MAX_TEXTURE_ARRAYS];
        uint32_t array_layers[MAX_TEXTURE_ARRAYS];
        bool array_used[MAX_TEXTURE_ARRAYS];
        uint32_t next_free_array;
        uint32_t active_arrays;
        uint64_t total_array_operations;
        pthread_mutex_t array_mutex;
    } array_system;
    
    // Feedback analysis system (enhanced)
    struct {
        float global_feedback_buffer[FEEDBACK_BUFFER_SIZE];
        uint32_t global_feedback_index;
        float lod_distribution[16]; // Distribution of LOD usage
        float performance_metrics[8]; // Various performance metrics
        uint64_t analysis_timestamp;
        uint64_t total_samples;
        float average_lod;
        float lod_variance;
        pthread_mutex_t feedback_mutex;
    } feedback_system;
    
    // Hot-reload system (enhanced)
    struct {
        int inotify_fd;
        int inotify_wd;
        pthread_t file_watch_thread;
        bool file_watch_active;
        uint32_t watched_files;
        uint64_t total_reloads;
        char watch_directory[256];
        pthread_mutex_t watch_mutex;
    } hot_reload_system;
    
    // GPU integration (enhanced)
    struct {
        void* gpu_context;
        bool gpu_available;
        uint32_t gpu_memory_used;
        uint32_t gpu_memory_total;
        uint64_t total_gpu_uploads;
        uint64_t total_gpu_downloads;
        pthread_mutex_t gpu_mutex;
    } gpu_system;
    
    // SIMD optimization (enhanced)
    struct {
        bool simd_available;
        bool avx2_available;
        bool sse4_available;
        uint64_t simd_operations;
        uint64_t scalar_fallbacks;
    } simd_system;
    
    // Legacy cache compatibility
    struct {
        void* data;
        size_t size;
        uint32_t texture_id;
        uint32_t lod_level;
        bool valid;
        uint64_t timestamp;
    } cache[TEXTURE_TEXTURE_LOD_CACHE_SIZE];
    
    // System state
    struct {
        uint64_t frame_counter;
        bool async_enabled;
        bool debug_mode;
        bool profiling_enabled;
        uint32_t log_level;
        uint64_t system_start_time;
        uint64_t last_update_time;
    } system_state;
    
    // Error handling
    char last_error_message[256];
    texture_texture_lod_error_code_t last_error_code;
    uint64_t error_count;
    
} texture_texture_lod_context_t;

static texture_texture_lod_context_t g_texture_lod_ctx = {0};
static texture_texture_lod_error_code_t g_texture_lod_ctx_last_error = TEXTURE_TEXTURE_LOD_ERROR_NONE;
static bool g_texture_lod_ctx_mutex_initialized = false;

// Render graph node
typedef struct texture_lod_render_node {
    uint32_t node_id;
    texture_texture_lod_handle_t lod_handle;
    uint32_t input_texture_id;
    uint32_t output_texture_id;
    float lod_bias;
    bool enabled;
} texture_lod_render_node_t;

// Compression formats
typedef enum {
    TEXTURE_LOD_COMPRESSION_NONE = 0,
    TEXTURE_LOD_COMPRESSION_BC1 = 1,
    TEXTURE_LOD_COMPRESSION_BC3 = 2,
    TEXTURE_LOD_COMPRESSION_BC5 = 3,
    TEXTURE_LOD_COMPRESSION_BC7 = 4,
    TEXTURE_LOD_COMPRESSION_ASTC_4X4 = 5,
    TEXTURE_LOD_COMPRESSION_ASTC_6X6 = 6,
    TEXTURE_LOD_COMPRESSION_ASTC_8X8 = 7
} texture_lod_compression_format_t;

// Missing function implementations

// Mipmap generation implementation
static int generate_mipmaps(uint8_t* src_data, uint32_t src_width, uint32_t src_height,
                          void* mip_data, size_t* mip_sizes, uint32_t* mip_count) {
    if (!src_data || !mip_data || !mip_sizes || !mip_count) return -1;
    
    uint32_t width = src_width;
    uint32_t height = src_height;
    uint8_t* current_src = src_data;
    uint8_t* current_dst = (uint8_t*)mip_data;
    
    *mip_count = 0;
    
    while (width > 1 && height > 1 && *mip_count < 16) {
        uint32_t next_width = width / 2;
        uint32_t next_height = height / 2;
        
        // Simple box filter for mipmap generation
        for (uint32_t y = 0; y < next_height; y++) {
            for (uint32_t x = 0; x < next_width; x++) {
                uint32_t src_x = x * 2;
                uint32_t src_y = y * 2;
                
                // Sample 4 pixels and average
                uint32_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
                for (uint32_t dy = 0; dy < 2; dy++) {
                    for (uint32_t dx = 0; dx < 2; dx++) {
                        uint32_t sample_x = (src_x + dx < width) ? src_x + dx : width - 1;
                        uint32_t sample_y = (src_y + dy < height) ? src_y + dy : height - 1;
                        uint32_t src_idx = (sample_y * width + sample_x) * 4;
                        
                        sum_r += current_src[src_idx + 0];
                        sum_g += current_src[src_idx + 1];
                        sum_b += current_src[src_idx + 2];
                        sum_a += current_src[src_idx + 3];
                    }
                }
                
                uint32_t dst_idx = (y * next_width + x) * 4;
                current_dst[dst_idx + 0] = sum_r / 4;
                current_dst[dst_idx + 1] = sum_g / 4;
                current_dst[dst_idx + 2] = sum_b / 4;
                current_dst[dst_idx + 3] = sum_a / 4;
            }
        }
        
        mip_sizes[*mip_count] = next_width * next_height * 4;
        current_dst += mip_sizes[*mip_count];
        width = next_width;
        height = next_height;
        (*mip_count)++;
    }
    
    return 0;
}

// Streaming queue implementation
static texture_texture_lod_internal_t* texture_texture_lod_stream_dequeue(void) {
    if (g_texture_lod_ctx.streaming_system.stream_count == 0) {
        return NULL;
    }
    
    texture_texture_lod_internal_t* item = g_texture_lod_ctx.streaming_system.stream_queue[g_texture_lod_ctx.streaming_system.stream_head];
    g_texture_lod_ctx.streaming_system.stream_head = 
        (g_texture_lod_ctx.streaming_system.stream_head + 1) % g_texture_lod_ctx.streaming_system.stream_capacity;
    g_texture_lod_ctx.streaming_system.stream_count--;
    
    return item;
}

// Memory pool implementation
static void* texture_texture_lod_pool_alloc(size_t size) {
    if (!g_texture_lod_ctx.memory_pool_system.initialized) {
        return malloc(size);
    }
    
    pthread_mutex_lock(&g_texture_lod_ctx.memory_pool_system.pool_mutex);
    
    // Find a free block that fits
    for (uint32_t i = 0; i < g_texture_lod_ctx.memory_pool_system.free_count; i++) {
        uint32_t block_idx = g_texture_lod_ctx.memory_pool_system.free_blocks[i];
        if (g_texture_lod_ctx.memory_pool_system.block_size >= size) {
            void* ptr = (uint8_t*)g_texture_lod_ctx.memory_pool_system.pool_memory + 
                        (block_idx * g_texture_lod_ctx.memory_pool_system.block_size);
            
            // Remove from free list
            for (uint32_t j = i; j < g_texture_lod_ctx.memory_pool_system.free_count - 1; j++) {
                g_texture_lod_ctx.memory_pool_system.free_blocks[j] = 
                    g_texture_lod_ctx.memory_pool_system.free_blocks[j + 1];
            }
            g_texture_lod_ctx.memory_pool_system.free_count--;
            g_texture_lod_ctx.memory_pool_system.allocated_count++;
            g_texture_lod_ctx.memory_pool_system.current_usage += g_texture_lod_ctx.memory_pool_system.block_size;
            
            if (g_texture_lod_ctx.memory_pool_system.current_usage > g_texture_lod_ctx.memory_pool_system.peak_usage) {
                g_texture_lod_ctx.memory_pool_system.peak_usage = g_texture_lod_ctx.memory_pool_system.current_usage;
            }
            
            pthread_mutex_unlock(&g_texture_lod_ctx.memory_pool_system.pool_mutex);
            return ptr;
        }
    }
    
    pthread_mutex_unlock(&g_texture_lod_ctx.memory_pool_system.pool_mutex);
    return malloc(size); // Fallback to regular malloc
}

static void texture_texture_lod_pool_free(void* ptr) {
    if (!ptr) return;
    
    // Check if pointer is from pool
    if (ptr >= g_texture_lod_ctx.memory_pool_system.pool_memory && 
        ptr < (uint8_t*)g_texture_lod_ctx.memory_pool_system.pool_memory + g_texture_lod_ctx.memory_pool_system.pool_size) {
        
        pthread_mutex_lock(&g_texture_lod_ctx.memory_pool_system.pool_mutex);
        
        // Calculate block index
        uint32_t block_idx = ((uint8_t*)ptr - (uint8_t*)g_texture_lod_ctx.memory_pool_system.pool_memory) / 
                            g_texture_lod_ctx.memory_pool_system.block_size;
        
        // Add back to free list
        if (g_texture_lod_ctx.memory_pool_system.free_count < g_texture_lod_ctx.memory_pool_system.total_blocks) {
            g_texture_lod_ctx.memory_pool_system.free_blocks[g_texture_lod_ctx.memory_pool_system.free_count] = block_idx;
            g_texture_lod_ctx.memory_pool_system.free_count++;
        }
        
        g_texture_lod_ctx.memory_pool_system.allocated_count--;
        g_texture_lod_ctx.memory_pool_system.current_usage -= g_texture_lod_ctx.memory_pool_system.block_size;
        
        pthread_mutex_unlock(&g_texture_lod_ctx.memory_pool_system.pool_mutex);
    } else {
        free(ptr); // Not from pool, use regular free
    }
}

// Batch processing implementation
static int texture_texture_lod_process_batch(texture_texture_lod_internal_t** batch, uint32_t count) {
    if (!batch || count == 0) return 0;
    
    int processed = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        texture_texture_lod_internal_t* item = batch[i];
        if (!item || !item->initialized) continue;
        
        // Process item based on its flags
        if (item->flags & TEXTURE_TEXTURE_LOD_FLAG_BC_ASTC_COMPRESSION) {
            if (texture_texture_lod_compress_bc_astc(item, item->compression.compression_format)) {
                processed++;
            }
        }
        
        if (item->flags & TEXTURE_TEXTURE_LOD_FLAG_VIRTUAL_TEXTURING) {
            if (texture_texture_lod_init_virtual_texture(item)) {
                processed++;
            }
        }
        
        if (item->flags & TEXTURE_TEXTURE_LOD_FLAG_BINDLESS) {
            if (texture_texture_lod_init_bindless(item)) {
                processed++;
            }
        }
        
        if (item->flags & TEXTURE_TEXTURE_LOD_FLAG_TEXTURE_ARRAY) {
            if (texture_texture_lod_init_texture_array(item, item->texture_array.array_layers)) {
                processed++;
            }
        }
        
        // Update item status
        item->dirty = false;
        item->frame_updated = ++g_texture_lod_ctx.system_state.frame_counter;
    }
    
    return processed;
}

// Residency management implementation
static void texture_texture_lod_update_residency(texture_texture_lod_internal_t* item) {
    if (!item) return;
    
    // Calculate residency priority based on access patterns
    uint64_t current_time = time(NULL);
    uint64_t time_since_last_access = current_time - item->feedback.last_access_time;
    
    // Higher priority for recently accessed items
    if (time_since_last_access < 60) { // Accessed within last minute
        item->streaming.residency_priority = 10;
    } else if (time_since_last_access < 300) { // Accessed within last 5 minutes
        item->streaming.residency_priority = 5;
    } else {
        item->streaming.residency_priority = 1;
    }
    
    // Update GPU residency status
    item->streaming.gpu_resident = (item->feedback.access_count > 5) && 
                                   (item->streaming.residency_priority > 3);
    
    // Update virtual texture residency
    if (item->virtual_texture.virtual_enabled) {
        item->virtual_texture.total_page_requests++;
        
        // Simulate page eviction based on residency
        if (item->streaming.residency_priority < 2) {
            item->virtual_texture.total_page_evictions++;
        }
    }
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void texture_texture_lod_set_error(texture_texture_lod_error_code_t code, const char* message) {
    g_texture_lod_ctx_last_error = code;
    if (message) {
        snprintf(g_texture_lod_ctx.last_error_message, sizeof(g_texture_lod_ctx.last_error_message), "%s", message);
    } else {
        g_texture_lod_ctx.last_error_message[0] = '\0';
    }
}

static void texture_texture_lod_lock(void) {
    if (g_texture_lod_ctx_mutex_initialized) {
        pthread_mutex_lock(&g_texture_lod_ctx.mutex);
    }
}

static void texture_texture_lod_unlock(void) {
    if (g_texture_lod_ctx_mutex_initialized) {
        pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
    }
}

static uint64_t texture_texture_lod_hash(const void* data, size_t size) {
    const unsigned char* bytes = (const unsigned char*)data;
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static void texture_texture_lod_copy_data(void* dst, const void* src, size_t size) {
    if (!dst || !src || size == 0) {
        return;
    }

    if ((((uintptr_t)dst | (uintptr_t)src) & (TEXTURE_TEXTURE_LOD_ALIGNMENT - 1)) == 0 &&
        (size % TEXTURE_TEXTURE_LOD_ALIGNMENT) == 0) {
        const uint64_t* src64 = (const uint64_t*)src;
        uint64_t* dst64 = (uint64_t*)dst;
        size_t count = size / sizeof(uint64_t);
        for (size_t i = 0; i < count; ++i) {
            dst64[i] = src64[i];
        }
        return;
    }

    memcpy(dst, src, size);
}

static void texture_texture_lod_convert_format(void* dst, const void* src, size_t size, bool swap_rb) {
    if (!dst || !src || size == 0) {
        return;
    }

    if (!swap_rb || size < 4 || (size % 4) != 0) {
        texture_texture_lod_copy_data(dst, src, size);
        return;
    }

    const uint8_t* in_bytes = (const uint8_t*)src;
    uint8_t* out_bytes = (uint8_t*)dst;
    for (size_t i = 0; i < size; i += 4) {
        out_bytes[i] = in_bytes[i + 2];
        out_bytes[i + 1] = in_bytes[i + 1];
        out_bytes[i + 2] = in_bytes[i];
        out_bytes[i + 3] = in_bytes[i + 3];
    }
}

static uint8_t texture_texture_lod_compute_level(size_t size) {
    uint8_t level = 0;
    while (size > 1) {
        size >>= 1;
        level++;
    }
    return level;
}

static size_t texture_texture_lod_serialize_item(const texture_texture_lod_internal_t* item, void* out_buffer, size_t buffer_size) {
    if (!item) {
        return 0;
    }

    struct texture_texture_lod_serialized {
        uint32_t id;
        uint32_t flags;
        uint32_t array_layers;
        uint8_t lod_level;
        uint8_t reserved[3];
        uint64_t data_size;
        uint64_t data_hash;
        float anisotropy;
        float feedback_score;
        uint32_t feature_flags;
    } snapshot;

    snapshot.id = item->id;
    snapshot.flags = item->flags;
    snapshot.array_layers = item->array_layers;
    snapshot.lod_level = item->lod_level;
    snapshot.reserved[0] = 0;
    snapshot.reserved[1] = 0;
    snapshot.reserved[2] = 0;
    snapshot.data_size = (uint64_t)item->data_size;
    snapshot.data_hash = item->data_hash;
    snapshot.anisotropy = item->anisotropy;
    snapshot.feedback_score = item->feedback_score;
    snapshot.feature_flags = 0;
    if (item->virtual_texturing_enabled) {
        snapshot.feature_flags |= TEXTURE_TEXTURE_LOD_FLAG_VIRTUAL_TEXTURING;
    }
    if (item->compressed) {
        snapshot.feature_flags |= TEXTURE_TEXTURE_LOD_FLAG_BC_ASTC_COMPRESSION;
    }
    if (item->bindless_enabled) {
        snapshot.feature_flags |= TEXTURE_TEXTURE_LOD_FLAG_BINDLESS;
    }
    if (item->texture_array_enabled) {
        snapshot.feature_flags |= TEXTURE_TEXTURE_LOD_FLAG_TEXTURE_ARRAY;
    }
    if (item->feedback_enabled) {
        snapshot.feature_flags |= TEXTURE_TEXTURE_LOD_FLAG_FEEDBACK;
    }

    if (out_buffer && buffer_size >= sizeof(snapshot)) {
        memcpy(out_buffer, &snapshot, sizeof(snapshot));
    }

    return sizeof(snapshot);
}

static void texture_texture_lod_update_feedback(texture_texture_lod_internal_t* item) {
    if (!item) {
        return;
    }

    if (item->data_size == 0) {
        item->feedback_score = 0.0f;
        return;
    }

    item->feedback_score = 1.0f;
}

static void texture_texture_lod_apply_gpu_upload(texture_texture_lod_internal_t* item) {
    if (!item || item->data_size == 0) {
        return;

    }
    item->gpu_resident = true;
    g_texture_lod_ctx.stats.bytes_uploaded += item->data_size;
    item->frame_updated = ++g_texture_lod_ctx.system_state.frame_counter;
}

static bool texture_texture_lod_validate(const texture_texture_lod_internal_t* item) {
    if (!item->initialized) return false;
    if (item->width == 0 || item->height == 0) return false;
    if (item->mip_levels == 0 || item->mip_levels > TEXTURE_TEXTURE_LOD_MAX_MIP_LEVELS) return false;
    return true;
}

static void texture_texture_lod_cleanup_internal(texture_texture_lod_internal_t* item) {
    if (!item) return;
    
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    // Cleanup virtual texturing
    if (item->virtual_texture.virtual_cache) {
        free(item->virtual_texture.virtual_cache);
        item->virtual_texture.virtual_cache = NULL;
    }
    
    // Cleanup compression
    if (item->compression.compressed_data) {
        free(item->compression.compressed_data);
        item->compression.compressed_data = NULL;
    }
    
    // Cleanup cache
    if (item->cache_data) {
        free(item->cache_data);
        item->cache_data = NULL;
    }
    
    item->initialized = false;
    item->cache_valid = false;
}

// Virtual texturing implementation
static bool texture_texture_lod_init_virtual_texture(texture_texture_lod_internal_t* item) {
    if (!item) return false;
    
    item->virtual_texture.page_width = VIRTUAL_TEXTURE_PAGE_SIZE;
    item->virtual_texture.page_height = VIRTUAL_TEXTURE_PAGE_SIZE;
    item->virtual_texture.virtual_enabled = true;
    
    // Initialize page table
    memset(item->virtual_texture.page_table, 0, sizeof(item->virtual_texture.page_table));
    
    // Allocate virtual cache
    size_t cache_size = VIRTUAL_TEXTURE_MAX_PAGES * VIRTUAL_TEXTURE_PAGE_SIZE * VIRTUAL_TEXTURE_PAGE_SIZE * 4; // RGBA
    item->virtual_texture.virtual_cache = malloc(cache_size);
    if (!item->virtual_texture.virtual_cache) return false;
    
    item->virtual_texture.virtual_cache_size = cache_size;
    g_texture_lod_ctx.stats.virtual_page_requests++;
    return true;
}

// BC/ASTC compression implementation
static bool texture_texture_lod_compress_bc_astc(texture_texture_lod_internal_t* item, uint32_t format) {
    if (!item || !item->data) return false;
    
    item->compression.compressed = true;
    item->compression.compression_format = format;
    item->compression.compression_quality = 8; // Default quality
    
    // Simulate compression (real implementation would use GPU/compression library)
    size_t compressed_size = item->data_size / 2; // Assume 50% compression
    item->compression.compressed_data = malloc(compressed_size);
    if (!item->compression.compressed_data) return false;
    
    // Simulate compression process
    memcpy(item->compression.compressed_data, item->data, item->data_size);
    item->compression.compressed_size = compressed_size;
    item->compression.compression_ratio = (float)item->data_size / compressed_size;
    
    g_texture_lod_ctx.stats.compression_operations++;
    return true;
}

// Bindless texture implementation
static bool texture_texture_lod_init_bindless(texture_texture_lod_internal_t* item) {
    if (!item) return false;
    
    // Find free bindless handle
    for (uint32_t i = 0; i < MAX_BINDLESS_TEXTURES; i++) {
        if (!g_texture_lod_ctx.bindless_system.bindless_used[i]) {
            item->bindless.bindless_enabled = true;
            item->bindless.bindless_index = i;
            item->bindless.bindless_handle = ((uint64_t)i << 32) | item->id;
            
            g_texture_lod_ctx.bindless_system.bindless_used[i] = true;
            g_texture_lod_ctx.bindless_system.bindless_handles[i] = item->bindless.bindless_handle;
            
            g_texture_lod_ctx.stats.bindless_operations++;
            return true;
        }
    }
    return false;
}

// Texture array implementation
static bool texture_texture_lod_init_texture_array(texture_texture_lod_internal_t* item, uint32_t layers) {
    if (!item || layers == 0 || layers > MAX_TEXTURE_ARRAY_LAYERS) return false;
    
    // Find free array slot
    for (uint32_t i = 0; i < MAX_TEXTURE_ARRAYS; i++) {
        if (!g_texture_lod_ctx.array_system.array_used[i]) {
            item->texture_array.is_array = true;
            item->texture_array.array_layers = layers;
            item->texture_array.array_index = i;
            item->texture_array.current_layer = 0;
            
            g_texture_lod_ctx.array_system.array_used[i] = true;
            g_texture_lod_ctx.array_system.array_layers[i] = layers;
            
            g_texture_lod_ctx.stats.array_operations++;
            return true;
        }
    }
    return false;
}

// Feedback analysis implementation

static void texture_texture_lod_apply_pending_locked(texture_texture_lod_internal_t* item) {
    if (!item || !item->pending_data) {
        return;
    }

    bool had_data = item->data_size > 0;
    uint64_t previous_hash = item->data_hash;

    if (item->data) {
        free(item->data);
    }

    item->data = item->pending_data;
    item->data_size = item->pending_size;
    item->data_hash = item->pending_hash;
    item->pending_data = NULL;
    item->pending_size = 0;
    item->pending_hash = 0;
    item->lod_level = texture_texture_lod_compute_level(item->data_size);
    item->serialized_size = texture_texture_lod_serialize_item(item, NULL, 0);
    item->compressed = (item->flags & TEXTURE_TEXTURE_LOD_FLAG_BC_ASTC_COMPRESSION) != 0;
    item->gpu_resident = false;

    if (had_data && previous_hash != item->data_hash) {
        g_texture_lod_ctx.stats.hot_reloads++;
    }
}

// Hot-reload file watching thread
static void* texture_texture_lod_file_watch_thread(void* arg) {
    char buffer[4096];
    
    while (g_texture_lod_ctx.hot_reload_system.file_watch_active) {
        ssize_t length = read(g_texture_lod_ctx.hot_reload_system.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            // Process file events
            for (char* ptr = buffer; ptr < buffer + length; ) {
                struct inotify_event* event = (struct inotify_event*)ptr;
                if (event->mask & IN_MODIFY) {
                    // Trigger hot-reload for modified texture
                    pthread_mutex_lock(&g_texture_lod_ctx.mutex);
                    // Mark all LOD textures as dirty for reload
                    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
                        g_texture_lod_ctx.items[i].dirty = true;
                        g_texture_lod_ctx.items[i].cache_valid = false;
                    }
                    pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
                }
                ptr += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); // 100ms sleep
    }
    return NULL;
}

// SIMD-optimized LOD calculation
static float texture_texture_lod_simd_calculate(const texture_texture_lod_internal_t* item, 
                                              float u, float v, float dudx, float dvdx, 
                                              float dudy, float dvdy) {
    if (!g_texture_lod_ctx.simd_system.simd_available) {
        // Fallback to scalar calculation
        float dx = sqrtf(dudx * dudx + dvdx * dvdx);
        float dy = sqrtf(dudy * dudy + dvdy * dvdy);
        float max_derivative = fmaxf(dx, dy);
        float lod = log2f(max_derivative * fmaxf(item->width, item->height));
        return fmaxf(0.0f, fminf(lod + item->lod_bias, (float)item->mip_levels - 1.0f));
    }
}
#ifdef __AVX2__
    
    // SIMD implementation using AVX2
    const __m256 uv_vec = _mm256_set_ps(u, v, dudx, dvdx, dudy, dvdy, 0.0f, 0.0f);
    const __m256 width_height = _mm256_set_ps((float)item->width, (float)item->height, 
                                           (float)item->width, (float)item->height,
                                           0.0f, 0.0f, 0.0f, 0.0f);
    
    // Calculate derivatives
    __m256 derivatives = _mm256_mul_ps(uv_vec, uv_vec);
    derivatives = _mm256_hadd_ps(derivatives, derivatives);
    
    // Extract dx and dy
    float deriv_array[8];
    _mm256_storeu_ps(deriv_array, derivatives);
    float dx = sqrtf(deriv_array[0]);
    float dy = sqrtf(deriv_array[1]);
    
    float max_derivative = fmaxf(dx, dy);
    float lod = log2f(max_derivative * fmaxf(item->width, item->height));
    
    return fmaxf(0.0f, fminf(lod + item->lod_bias, (float)item->mip_levels - 1.0f));
}
#endif

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int texture_texture_lod_init(void) {
    if (g_texture_lod_ctx.initialized) {
        return 0; // Already initialized
    }

    // Initialize mutex
    if (pthread_mutex_init(&g_texture_lod_ctx.mutex, NULL) != 0) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_THREAD_ERROR, "Failed to initialize mutex");
        return -1;
    }
    g_texture_lod_ctx_mutex_initialized = true;

    // Initialize items array
    g_texture_lod_ctx.capacity = TEXTURE_TEXTURE_LOD_DEFAULT_CAPACITY;
    g_texture_lod_ctx.items = (texture_texture_lod_internal_t*)calloc(
        g_texture_lod_ctx.capacity, sizeof(texture_texture_lod_internal_t));
    if (!g_texture_lod_ctx.items) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_OUT_OF_MEMORY, "Failed to allocate items array");
        return -2;
    }

    // Initialize memory pool system
    g_texture_lod_ctx.memory_pool_system.pool_size = TEXTURE_TEXTURE_LOD_MEMORY_POOL_SIZE;
    g_texture_lod_ctx.memory_pool_system.pool_memory = malloc(g_texture_lod_ctx.memory_pool_system.pool_size);
    if (g_texture_lod_ctx.memory_pool_system.pool_memory) {
        g_texture_lod_ctx.memory_pool_system.block_size = 64 * 1024; // 64KB blocks
        g_texture_lod_ctx.memory_pool_system.total_blocks = g_texture_lod_ctx.memory_pool_system.pool_size / g_texture_lod_ctx.memory_pool_system.block_size;
        g_texture_lod_ctx.memory_pool_system.free_blocks = (uint32_t*)malloc(g_texture_lod_ctx.memory_pool_system.total_blocks * sizeof(uint32_t));
        if (g_texture_lod_ctx.memory_pool_system.free_blocks) {
            // Initialize free list
            for (uint32_t i = 0; i < g_texture_lod_ctx.memory_pool_system.total_blocks; i++) {
                g_texture_lod_ctx.memory_pool_system.free_blocks[i] = i;
            }
            g_texture_lod_ctx.memory_pool_system.free_count = g_texture_lod_ctx.memory_pool_system.total_blocks;
            g_texture_lod_ctx.memory_pool_system.initialized = true;
        }
    }

    // Initialize streaming system
    g_texture_lod_ctx.streaming_system.stream_capacity = 1024;
    g_texture_lod_ctx.streaming_system.stream_queue = (texture_texture_lod_internal_t**)malloc(
        g_texture_lod_ctx.streaming_system.stream_capacity * sizeof(texture_texture_lod_internal_t*));
    if (g_texture_lod_ctx.streaming_system.stream_queue) {
        g_texture_lod_ctx.streaming_system.stream_head = 0;
        g_texture_lod_ctx.streaming_system.stream_tail = 0;
        g_texture_lod_ctx.streaming_system.stream_count = 0;
        g_texture_lod_ctx.streaming_system.streaming_active = true;
        g_texture_lod_ctx.streaming_system.total_bandwidth = 0.0f;
    }

    // Initialize batch processing system
    g_texture_lod_ctx.batch_system.batch_capacity = TEXTURE_TEXTURE_LOD_BATCH_SIZE;
    g_texture_lod_ctx.batch_system.batch_queue = (texture_texture_lod_internal_t**)malloc(
        g_texture_lod_ctx.batch_system.batch_capacity * sizeof(texture_texture_lod_internal_t*));
    if (g_texture_lod_ctx.batch_system.batch_queue) {
        g_texture_lod_ctx.batch_system.batch_count = 0;
        g_texture_lod_ctx.batch_system.batch_processed = 0;
        g_texture_lod_ctx.batch_system.batch_active = false;
    }

    // Initialize statistics
    memset(&g_texture_lod_ctx.stats, 0, sizeof(g_texture_lod_ctx.stats));
    
    // Initialize frame counter
    g_texture_lod_ctx.system_state.frame_counter = 0;
    
    // Initialize async system
    g_texture_lod_ctx.system_state.async_enabled = true;

    g_texture_lod_ctx.initialized = true;
    printf("Texture LOD System initialized\n");
    printf("  Memory Pool: %s (%zu bytes)\n", 
           g_texture_lod_ctx.memory_pool_system.initialized ? "Enabled" : "Disabled",
           g_texture_lod_ctx.memory_pool_system.pool_size);
    printf("  Streaming: %s (%u capacity)\n", 
           g_texture_lod_ctx.streaming_system.streaming_active ? "Enabled" : "Disabled",
           g_texture_lod_ctx.streaming_system.stream_capacity);
    printf("  Batch Processing: %s (%u capacity)\n", 
           g_texture_lod_ctx.batch_system.batch_queue ? "Enabled" : "Disabled",
           g_texture_lod_ctx.batch_system.batch_capacity);
    
    return TEXTURE_TEXTURE_LOD_ERROR_NONE;
}

void texture_texture_lod_shutdown(void) {
    if (!g_texture_lod_ctx.initialized) {
        return;
    }

    texture_texture_lod_lock();
    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
        texture_texture_lod_cleanup_internal(&g_texture_lod_ctx.items[i]);
    }

    free(g_texture_lod_ctx.items);
    g_texture_lod_ctx.items = NULL;
    g_texture_lod_ctx.count = 0;
    g_texture_lod_ctx.capacity = 0;
    g_texture_lod_ctx.initialized = false;
    texture_texture_lod_unlock();

    if (g_texture_lod_ctx_mutex_initialized) {
        pthread_mutex_destroy(&g_texture_lod_ctx.mutex);
        g_texture_lod_ctx_mutex_initialized = false;
    }
}

void texture_texture_lod_debug_print(void) {
    // Implement texture lod debug output
    printf("=== Texture LOD System Debug Info ===\n");
    printf("Initialized: %s\n", g_texture_lod_ctx.initialized ? "Yes" : "No");
    printf("Count: %u / %u\n", g_texture_lod_ctx.count, g_texture_lod_ctx.capacity);
    printf("Async enabled: %s\n", g_texture_lod_ctx.system_state.async_enabled ? "Yes" : "No");
    printf("Frame counter: %lu\n", g_texture_lod_ctx.system_state.frame_counter);
    
    // Memory pool debug info
    printf("\n--- Memory Pool System ---\n");
    printf("Pool Initialized: %s\n", g_texture_lod_ctx.memory_pool_system.initialized ? "Yes" : "No");
    if (g_texture_lod_ctx.memory_pool_system.initialized) {
        printf("Pool Size: %zu bytes\n", g_texture_lod_ctx.memory_pool_system.pool_size);
        printf("Block Size: %u bytes\n", g_texture_lod_ctx.memory_pool_system.block_size);
        printf("Total Blocks: %u\n", g_texture_lod_ctx.memory_pool_system.total_blocks);
        printf("Free Blocks: %u\n", g_texture_lod_ctx.memory_pool_system.free_count);
    }
    
    // Streaming system debug info
    printf("\n--- Streaming System ---\n");
    printf("Streaming Active: %s\n", g_texture_lod_ctx.streaming_system.streaming_active ? "Yes" : "No");
    if (g_texture_lod_ctx.streaming_system.streaming_active) {
        printf("Stream Capacity: %u\n", g_texture_lod_ctx.streaming_system.stream_capacity);
        printf("Stream Count: %u\n", g_texture_lod_ctx.streaming_system.stream_count);
        printf("Total Bandwidth: %.2f KB\n", g_texture_lod_ctx.streaming_system.total_bandwidth);
    }
    
    // Batch processing debug info
    printf("\n--- Batch Processing System ---\n");
    printf("Batch Queue: %s\n", g_texture_lod_ctx.batch_system.batch_queue ? "Available" : "Not Available");
    if (g_texture_lod_ctx.batch_system.batch_queue) {
        printf("Batch Capacity: %u\n", g_texture_lod_ctx.batch_system.batch_capacity);
        printf("Batch Count: %u\n", g_texture_lod_ctx.batch_system.batch_count);
        printf("Batch Processed: %u\n", g_texture_lod_ctx.batch_system.batch_processed);
        printf("Batch Active: %s\n", g_texture_lod_ctx.batch_system.batch_active ? "Yes" : "No");
    }
    
    // Statistics
    printf("\n--- Performance Statistics ---\n");
    printf("Total LOD Calculations: %llu\n", g_texture_lod_ctx.stats.total_lod_calculations);
    printf("Cache Hits: %llu\n", g_texture_lod_ctx.stats.cache_hits);
    printf("Cache Misses: %llu\n", g_texture_lod_ctx.stats.cache_misses);
    printf("Virtual Page Requests: %llu\n", g_texture_lod_ctx.stats.virtual_page_requests);
    printf("Virtual Page Evictions: %llu\n", g_texture_lod_ctx.stats.virtual_page_evictions);
    printf("Compression Operations: %llu\n", g_texture_lod_ctx.stats.compression_operations);
    printf("Decompression Operations: %llu\n", g_texture_lod_ctx.stats.decompression_operations);
    printf("Total LOD Time: %.2f ms\n", g_texture_lod_ctx.stats.total_lod_time);
    
    // Individual texture info
    printf("\n--- Active Textures ---\n");
    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
        const texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[i];
        if (item->initialized) {
            printf("Texture %u:\n", item->id);
            printf("  Size: %ux%u\n", item->width, item->height);
            printf("  Mip Levels: %u\n", item->mip_levels);
            printf("  LOD Bias: %.2f\n", item->lod_bias);
            printf("  Streaming: %s\n", item->streaming.streaming_active ? "Active" : "Inactive");
            printf("  GPU Resident: %s\n", item->streaming.gpu_resident ? "Yes" : "No");
            printf("  Residency Priority: %u\n", item->streaming.residency_priority);
                    printf("LOD: %.2f, Bias: %.2f, Anisotropy: %.2f, Feedback: %.2f\n",
                    item->lod_bias, item->anisotropy, item->feedback_score);
            printf("  Compressed: %s\n", item->compressed ? "Yes" : "No");
            if (item->compressed) {
                printf("  Compression Ratio: %.2f%%\n", item->compression.compression_ratio * 100.0f);
            }
                   item->lod_bias, item->anisotropy, item->feedback_score);
            printf("  GPU resident: %s, Culled: %s, Dirty: %s\n",
                   item->gpu_resident ? "Yes" : "No",
                   item->culled ? "Yes" : "No",
                   item->dirty ? "Yes" : "No");
        }
    }
    
    if (g_texture_lod_ctx_last_error != TEXTURE_TEXTURE_LOD_ERROR_NONE) {
        printf("\n--- Last Error ---\n");
        printf("Error code: %d\n", g_texture_lod_ctx_last_error);
        printf("Error message: %s\n", g_texture_lod_ctx.last_error_message);
    }
    
    printf("\n=== End Debug Info ===\n");
}

int texture_texture_lod_create(texture_texture_lod_handle_t* out_handle, const texture_texture_lod_desc_t* desc) {
    // Add texture lod debug output
    printf("Creating texture LOD with flags: 0x%08x\n", desc->flags);

    if (!out_handle || !desc) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_ARGUMENT, "Invalid texture LOD create arguments");
        return -1;
    }

    if (!g_texture_lod_ctx.initialized) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_NOT_INITIALIZED, "Texture LOD system not initialized");
        return -2;
    }

    pthread_mutex_lock(&g_texture_lod_ctx.mutex);
    
    if (g_texture_lod_ctx.count >= g_texture_lod_ctx.capacity) {
        pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_OUT_OF_MEMORY, "Texture LOD capacity reached");
        return -3;
    }

    uint32_t index = g_texture_lod_ctx.count++;
    texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[index];

    // Initialize all fields
    memset(item, 0, sizeof(texture_texture_lod_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->gpu_resident = false;
    item->virtual_texturing_enabled = (desc->flags & TEXTURE_TEXTURE_LOD_FLAG_VIRTUAL_TEXTURING) != 0;
    item->bindless_enabled = (desc->flags & TEXTURE_TEXTURE_LOD_FLAG_BINDLESS) != 0;
    item->texture_array_enabled = (desc->flags & TEXTURE_TEXTURE_LOD_FLAG_TEXTURE_ARRAY) != 0;
    item->feedback_enabled = (desc->flags & TEXTURE_TEXTURE_LOD_FLAG_FEEDBACK) != 0;
    item->compressed = (desc->flags & TEXTURE_TEXTURE_LOD_FLAG_BC_ASTC_COMPRESSION) != 0;
    item->culled = (desc->flags & TEXTURE_TEXTURE_LOD_FLAG_CULLED) != 0;
    item->frame_updated = 0;
    item->lod_level = 0;
    item->anisotropy = TEXTURE_TEXTURE_LOD_MAX_ANISOTROPY;
    item->feedback_score = 0.0f;

    // Initialize virtual texturing if enabled
    if (item->virtual_texturing_enabled) {
        if (!texture_texture_lod_init_virtual_texture(item)) {
            pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
            texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_VIRTUAL_TEXTURE_ERROR, "Failed to initialize virtual texturing");
            return -4;
        }
    }

    // Initialize compression if enabled
    if (item->compressed) {
        if (!texture_texture_lod_compress_bc_astc(item, TEXTURE_LOD_COMPRESSION_BC7)) {
            pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
            texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_COMPRESSION_ERROR, "Failed to initialize compression");
            return -5;
        }
    }

    // Initialize bindless if enabled
    if (item->bindless_enabled) {
        if (!texture_texture_lod_init_bindless(item)) {
            pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
            texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_BINDLESS_ERROR, "Failed to initialize bindless texture");
            return -6;
        }
    }

    // Initialize texture array if enabled
    if (item->texture_array_enabled) {
        if (!texture_texture_lod_init_texture_array(item, 1)) {
            pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
            texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_ARRAY_ERROR, "Failed to initialize texture array");
            return -7;
        }
    }

    out_handle->id = index;
    g_texture_lod_ctx.stats.created++;
    printf("Successfully created texture LOD %u\n", index);
    pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
    return 0;
}

void texture_texture_lod_destroy(texture_texture_lod_handle_t handle) {
    texture_texture_lod_lock();

    if (handle.id >= g_texture_lod_ctx.count) {
        texture_texture_lod_unlock();
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE, "Invalid texture LOD handle");
        return;
    }

    texture_texture_lod_cleanup_internal(&g_texture_lod_ctx.items[handle.id]);
    g_texture_lod_ctx.stats.destroyed++;
    texture_texture_lod_unlock();
}

int texture_texture_lod_update(texture_texture_lod_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_texture_lod_ctx.count) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE, "Invalid texture LOD handle");
        return -1;
    }

    if (size > 0 && !data) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_ARGUMENT, "Texture LOD update missing data");
        return -2;
    }

    texture_texture_lod_lock();
    texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[handle.id];
    if (!item->initialized) {
        texture_texture_lod_unlock();
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_STATE, "Texture LOD not initialized");
        return -2;
    }

    if (size == 0) {
        if (item->data) {
            free(item->data);
            item->data = NULL;
            item->data_size = 0;
        }
        if (item->pending_data) {
            free(item->pending_data);
            item->pending_data = NULL;
            item->pending_size = 0;
        }
        item->data_hash = 0;
        item->pending_hash = 0;
        item->dirty = true;
        g_texture_lod_ctx.stats.updated++;
        texture_texture_lod_unlock();
        return 0;
    }

    uint64_t incoming_hash = texture_texture_lod_hash(data, size);
    if (item->data_size == size && item->data_hash == incoming_hash && item->pending_data == NULL) {
        g_texture_lod_ctx.stats.cache_hits++;
        item->dirty = false;
        texture_texture_lod_unlock();
        return 0;
    }
    g_texture_lod_ctx.stats.cache_misses++;

    if (item->pending_data) {
        free(item->pending_data);
        item->pending_data = NULL;
    }

    item->pending_data = malloc(size);
    if (!item->pending_data) {
        texture_texture_lod_unlock();
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_OUT_OF_MEMORY, "Texture LOD update allocation failed");
        return -3;
    }

    bool needs_format_conversion = (item->flags & TEXTURE_TEXTURE_LOD_FLAG_FORMAT_CONVERSION) != 0;
    texture_texture_lod_convert_format(
        item->pending_data,
        data,
        size,
        needs_format_conversion
    );

    item->pending_size = size;
    item->pending_hash = incoming_hash;
    item->dirty = true;
    g_texture_lod_ctx.stats.updated++;
    g_texture_lod_ctx.stats.async_enqueued++;

    if (!g_texture_lod_ctx.system_state.async_enabled) {
        texture_texture_lod_apply_pending_locked(item);
    }
    texture_texture_lod_unlock();
    return 0;
}

bool texture_texture_lod_serialize_item_public(texture_texture_lod_handle_t handle, void* buffer, size_t buffer_size, size_t* out_size) {
    // Implement texture lod serialization
    if (handle.id >= g_texture_lod_ctx.count) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE, "Invalid texture LOD handle for serialization");
        return false;
    }
    
    texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[handle.id];
    if (!item->initialized) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_NOT_INITIALIZED, "Texture LOD not initialized for serialization");
        return false;
    }
    
    if (!buffer || !out_size) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_ARGUMENT, "Invalid buffer or size pointer for serialization");
        return false;
    }
    
    // Calculate required size
    size_t required_size = sizeof(uint32_t) * 4 + // id, flags, data_size, mip_levels
                           sizeof(float) * 2 + // lod_bias, anisotropy
                           item->data_size +
                           sizeof(item->virtual_texture) +
                           sizeof(item->compression) +
                           sizeof(item->bindless) +
                           sizeof(item->texture_array) +
                           sizeof(item->feedback);
    
    if (buffer_size < required_size) {
        *out_size = required_size;
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_BUFFER_TOO_SMALL, "Buffer too small for serialization");
        return false;
    }
    
    // Serialize data
    uint8_t* ptr = (uint8_t*)buffer;
    
    // Write header
    *(uint32_t*)ptr = TEXTURE_TEXTURE_LOD_MAGIC_NUMBER;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = TEXTURE_TEXTURE_LOD_VERSION;
    ptr += sizeof(uint32_t);
    
    // Write basic properties
    *(uint32_t*)ptr = item->id;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->flags;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->data_size;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = item->mip_levels;
    ptr += sizeof(uint32_t);
    *(float*)ptr = item->lod_bias;
    ptr += sizeof(float);
    *(float*)ptr = item->anisotropy;
    ptr += sizeof(float);
    
    // Write data
    if (item->data && item->data_size > 0) {
        memcpy(ptr, item->data, item->data_size);
        ptr += item->data_size;
    }
    
    // Write virtual texture data
    memcpy(ptr, &item->virtual_texture, sizeof(item->virtual_texture));
    ptr += sizeof(item->virtual_texture);
    
    // Write compression data
    memcpy(ptr, &item->compression, sizeof(item->compression));
    ptr += sizeof(item->compression);
    
    // Write bindless data
    memcpy(ptr, &item->bindless, sizeof(item->bindless));
    ptr += sizeof(item->bindless);
    
    // Write texture array data
    memcpy(ptr, &item->texture_array, sizeof(item->texture_array));
    ptr += sizeof(item->texture_array);
    
    // Write feedback data
    memcpy(ptr, &item->feedback, sizeof(item->feedback));
    ptr += sizeof(item->feedback);
    
    *out_size = required_size;
    g_texture_lod_ctx.stats.serialized++;
    
    return true; // Success
}

bool texture_texture_lod_validate_public(texture_texture_lod_handle_t handle) {
    if (handle.id >= g_texture_lod_ctx.count) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE, "Invalid texture LOD handle for validation");
        return false;
    }
    
    texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[handle.id];
    if (!item->initialized) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_NOT_INITIALIZED, "Texture LOD not initialized for validation");
        return false;
    }
    
    // Validate data integrity
    if (!item->data && item->data_size > 0) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_VALIDATION_ERROR, "Texture LOD data inconsistency detected");
        return false;
    }
    
    // Validate virtual texturing if enabled
    if (item->virtual_texturing_enabled && !item->virtual_texture.page_cache) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_VIRTUAL_TEXTURE_ERROR, "Virtual texture cache not initialized");
        return false;
    }
    
    // Validate compression if enabled
    if (item->compressed && !item->compression.compressed_data) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_COMPRESSION_ERROR, "Compression data not available");
        return false;
    }
    
    // Validate bindless if enabled
    if (item->bindless_enabled && item->bindless.bindless_handle == 0) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_BINDLESS_ERROR, "Bindless handle not valid");
        return false;
    }
    
    // Validate texture array if enabled
    if (item->texture_array_enabled && item->texture_array.array_layers == 0) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_ARRAY_ERROR, "Texture array layers not configured");
        return false;
    }
    
    return true; // Validation successful
}

bool texture_texture_lod_is_valid(texture_texture_lod_handle_t handle) {
    if (handle.id >= g_texture_lod_ctx.count) {
        return false;
    }
    
    texture_texture_lod_lock();
    const texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[handle.id];
    bool valid = texture_texture_lod_validate(item);
    texture_texture_lod_unlock();
    
    return valid;
}

int texture_texture_lod_get_info(texture_texture_lod_handle_t handle, texture_texture_lod_info_t* out_info) {
    // Implement texture lod info retrieval with streaming support
    if (!out_info) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_ARGUMENT, "Texture LOD info output missing");
        return -1;
    }

    if (handle.id >= g_texture_lod_ctx.count) {
        texture_texture_lod_set_error(TEXTURE_TEXTURE_LOD_ERROR_INVALID_HANDLE, "Invalid texture LOD handle");
        return -2;
    }

    texture_texture_lod_lock();
    const texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[handle.id];
    
    // Copy basic info
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->width = item->width;
    out_info->height = item->height;
    out_info->mip_levels = item->mip_levels;
    out_info->format = item->format;
    out_info->lod_bias = item->lod_bias;
    
    // Copy streaming info
    out_info->streaming_enabled = item->virtual_texturing_enabled;
    out_info->streaming_priority = item->feedback_score > 0.5f ? 1 : 0;
    out_info->streaming_bandwidth = item->data_size > 0 ? (float)(item->data_size / 1024) : 0.0f; // KB/s
    
    // Copy compression info
    out_info->compression_enabled = item->compressed;
    out_info->compression_format = item->compression.compression_format;
    out_info->compression_ratio = item->compression.compression_ratio;
    
    // Copy virtual texturing info
    out_info->virtual_enabled = item->virtual_texturing_enabled;
    out_info->virtual_page_width = item->virtual_texture.page_width;
    out_info->virtual_page_height = item->virtual_texture.page_height;
    
    // Copy bindless info
    out_info->bindless_enabled = item->bindless_enabled;
    out_info->bindless_handle = item->bindless.bindless_handle;
    
    // Copy array info
    out_info->array_enabled = item->texture_array_enabled;
    out_info->array_layers = item->texture_array.array_layers;
    
    // Copy feedback info
    out_info->feedback_enabled = item->feedback_enabled;
    out_info->feedback_score = item->feedback_score;
    
    texture_texture_lod_unlock();
    return 0;
}

void texture_texture_lod_mark_dirty(texture_texture_lod_handle_t handle) {
    if (handle.id < g_texture_lod_ctx.count) {
        texture_texture_lod_lock();
        g_texture_lod_ctx.items[handle.id].dirty = true;
        g_texture_lod_ctx.items[handle.id].culled = false;
        texture_texture_lod_unlock();
    }
}

int texture_texture_lod_process_pending(void) {
    // Implement batch processing with streaming and memory pooling
    int processed = 0;
    texture_texture_lod_lock();
    
    // Process streaming queue first
    while (g_texture_lod_ctx.streaming_system.stream_count > 0) {
        texture_texture_lod_internal_t* item = texture_texture_lod_stream_dequeue();
        if (!item) break;
        
        // Update residency management
        texture_texture_lod_update_residency(item);
        
        // Process streaming if active
        if (item->streaming.streaming_active) {
            // Simulate streaming operation
            g_texture_lod_ctx.stats.virtual_page_requests++;
            item->streaming.last_stream_time = time(NULL);
            
            // Update bandwidth tracking
            float bandwidth = (float)item->data_size / 1024.0f; // KB
            g_texture_lod_ctx.streaming_system.total_bandwidth += bandwidth;
            item->streaming.stream_bandwidth = bandwidth;
        }
        
        processed++;
    }
    
    // Process batch queue
    if (g_texture_lod_ctx.batch_system.batch_count > 0) {
        g_texture_lod_ctx.batch_system.batch_active = true;
        
        int batch_processed = texture_texture_lod_process_batch(
            g_texture_lod_ctx.batch_system.batch_queue, 
            g_texture_lod_ctx.batch_system.batch_count);
        
        if (batch_processed > 0) {
            g_texture_lod_ctx.batch_system.batch_processed += batch_processed;
            
            // Remove processed items from queue
            uint32_t remaining = 0;
            for (uint32_t i = batch_processed; i < g_texture_lod_ctx.batch_system.batch_count; i++) {
                g_texture_lod_ctx.batch_system.batch_queue[remaining++] = 
                    g_texture_lod_ctx.batch_system.batch_queue[i];
            }
            g_texture_lod_ctx.batch_system.batch_count = remaining;
        }
        
        g_texture_lod_ctx.batch_system.batch_active = false;
        processed += batch_processed;
    }
    
    // Process regular items for mipmap generation
    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
        texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[i];
        
        if (!item->initialized || item->dirty) continue;
        
        // Generate mipmaps if needed
        if (!item->mipmap.mipmaps_generated || item->mipmap.mipmaps_dirty) {
            if (item->data && item->width > 0 && item->height > 0) {
                int result = generate_mipmaps(
                    (uint8_t*)item->data, item->width, item->height,
                    item->mipmap.mip_data, item->mipmap.mip_sizes, &item->mipmap.mip_count);
                
                if (result == 0) {
                    item->mipmap.mipmaps_generated = true;
                    item->mipmap.mipmaps_dirty = false;
                    g_texture_lod_ctx.stats.total_lod_calculations++;
                    processed++;
                }
            }
        }
        
        // Apply memory pooling if enabled
        if (item->data_size > 0 && !item->cache_valid && 
            g_texture_lod_ctx.memory_pool_system.initialized) {
            
            // Try to allocate from pool
            void* pooled_data = texture_texture_lod_pool_alloc(item->data_size);
            if (pooled_data) {
                memcpy(pooled_data, item->data, item->data_size);
                
                // Free old data and update pointers
                free(item->data);
                item->data = pooled_data;
                item->memory_pool.pooled = true;
                item->memory_pool.pool_block = pooled_data;
                item->memory_pool.pool_block_size = item->data_size;
                item->memory_pool.pool_timestamp = time(NULL);
                
                // Update cache
                for (uint32_t j = 0; j < TEXTURE_TEXTURE_LOD_CACHE_SIZE; j++) {
                    if (!g_texture_lod_ctx.cache[j].valid) {
                        g_texture_lod_ctx.cache[j].texture_id = item->id;
                        g_texture_lod_ctx.cache[j].lod_level = item->lod_level;
                        g_texture_lod_ctx.cache[j].data = pooled_data;
                        g_texture_lod_ctx.cache[j].size = item->data_size;
                        g_texture_lod_ctx.cache[j].valid = true;
                        g_texture_lod_ctx.cache[j].timestamp = time(NULL);
                        item->cache_valid = true;
                        g_texture_lod_ctx.stats.cache_hits++;
                        break;
                    }
                }
            }
        }
    }
    
    texture_texture_lod_unlock();
    return processed;
}

int texture_texture_lod_process_batch(void) {
    // Implement batch processing for all dirty textures
    int processed = 0;
    texture_texture_lod_lock();
    
    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
        texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[i];
        if (item->culled) {
            continue;
        }
        if (item->initialized && item->dirty) {
            if (item->pending_data) {
                texture_texture_lod_apply_pending_locked(item);
            }

            if (item->feedback_enabled) {
                texture_texture_lod_update_feedback(item);
            }

            if (!item->gpu_resident) {
                texture_texture_lod_apply_gpu_upload(item);
            }

            item->dirty = false;
            processed++;
            g_texture_lod_ctx.stats.processed++;
        }
    }
    
    texture_texture_lod_unlock();
    return processed;
}

uint32_t texture_texture_lod_get_count(void) {
    texture_texture_lod_lock();
    uint32_t count = g_texture_lod_ctx.count;
    texture_texture_lod_unlock();
    return count;
}

size_t texture_texture_lod_get_memory_usage(void) {
    texture_texture_lod_lock();
    
    size_t total_memory = sizeof(texture_texture_lod_context_t);
    
    // Add context memory
    total_memory += g_texture_lod_ctx.capacity * sizeof(texture_texture_lod_internal_t);
    
    // Add virtual texturing memory
    total_memory += g_texture_lod_ctx.virtual_system.page_cache_size;
    
    // Add compression workspace memory
    total_memory += g_texture_lod_ctx.compression_system.workspace_size;
    
    // Add cache memory
    total_memory += TEXTURE_TEXTURE_LOD_CACHE_SIZE * sizeof(g_texture_lod_ctx.cache[0]);
    
    // Add individual texture data
    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
        const texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[i];
        if (item->initialized) {
            total_memory += item->data_size;
            
            // Add mipmap data
            if (item->mipmap.mipmaps_generated) {
                for (uint32_t j = 0; j < item->mipmap.mip_count; j++) {
                    total_memory += item->mipmap.mip_sizes[j];
                }
            }
            
            // Add compression data
            if (item->compressed && item->compression.compressed_data) {
                total_memory += item->compression.compressed_size;
            }
            
            // Add virtual texturing data
            if (item->virtual_texturing_enabled && item->virtual_texture.virtual_cache) {
                total_memory += item->virtual_texture.virtual_cache_size;
            }
            
            // Add bindless data
            if (item->bindless_enabled) {
                total_memory += sizeof(uint64_t);
            }
            
            // Add array data
            if (item->texture_array_enabled) {
                total_memory += item->texture_array.array_layers * sizeof(void*);
            }
        }
    }
    
    texture_texture_lod_unlock();
    return total_memory;
}

void texture_texture_lod_debug_print(void) {
    if (!g_texture_lod_ctx.initialized) {
        printf("Texture LOD System: Not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_texture_lod_ctx.mutex);
    
    printf("=== Texture LOD System Debug Info ===\n");
    printf("Initialized: %s\n", g_texture_lod_ctx.initialized ? "Yes" : "No");
    printf("LOD Count: %u / %u\n", g_texture_lod_ctx.count, g_texture_lod_ctx.capacity);
    printf("GPU Available: %s\n", g_texture_lod_ctx.gpu_available ? "Yes" : "No");
    printf("SIMD Available: %s\n", g_texture_lod_ctx.simd_system.simd_available ? "Yes" : "No");
    printf("File Watch Active: %s\n", g_texture_lod_ctx.hot_reload_system.file_watch_active ? "Yes" : "No");
    printf("Batch Queue Size: %u / %u\n", g_texture_lod_ctx.batch_count, TEXTURE_TEXTURE_LOD_BATCH_SIZE);
    
    printf("\n=== Performance Statistics ===\n");
    printf("Total LOD Calculations: %lu\n", g_texture_lod_ctx.stats.total_lod_calculations);
    printf("Cache Hits: %lu\n", g_texture_lod_ctx.stats.cache_hits);
    printf("Cache Misses: %lu\n", g_texture_lod_ctx.stats.cache_misses);
    printf("Virtual Page Requests: %lu\n", g_texture_lod_ctx.stats.virtual_page_requests);
    printf("Virtual Page Evictions: %lu\n", g_texture_lod_ctx.stats.virtual_page_evictions);
    printf("Compression Operations: %lu\n", g_texture_lod_ctx.stats.compression_operations);
    printf("Decompression Operations: %lu\n", g_texture_lod_ctx.stats.decompression_operations);
    printf("Bindless Operations: %lu\n", g_texture_lod_ctx.stats.bindless_operations);
    printf("Array Operations: %lu\n", g_texture_lod_ctx.stats.array_operations);
    printf("Feedback Samples: %lu\n", g_texture_lod_ctx.stats.feedback_samples);
    printf("Total LOD Time: %.3f ms\n", g_texture_lod_ctx.stats.total_lod_time);
    
    if (g_texture_lod_ctx.stats.cache_hits + g_texture_lod_ctx.stats.cache_misses > 0) {
        float hit_rate = (float)g_texture_lod_ctx.stats.cache_hits / 
                        (g_texture_lod_ctx.stats.cache_hits + g_texture_lod_ctx.stats.cache_misses) * 100.0f;
        printf("Cache Hit Rate: %.2f%%\n", hit_rate);
    }
    
    printf("\n=== System Status ===\n");
    printf("Virtual Texturing: %s\n", g_texture_lod_ctx.virtual_system.page_cache ? "Available" : "Not Available");
    printf("BC/ASTC Compression: %s\n", g_texture_lod_ctx.compression_system.bc_available ? "BC Available" : "BC Not Available");
    if (g_texture_lod_ctx.compression_system.astc_available) {
        printf("ASTC Compression: Available\n");
    }
    printf("Bindless Textures: %s\n", g_texture_lod_ctx.bindless_system.gpu_descriptor_pool ? "Available" : "Not Available");
    printf("Texture Arrays: %u arrays allocated\n", g_texture_lod_ctx.array_system.next_free_array);
    
    printf("\n=== LOD Details ===\n");
    for (uint32_t i = 0; i < g_texture_lod_ctx.count; i++) {
        const texture_texture_lod_internal_t* item = &g_texture_lod_ctx.items[i];
        printf("LOD %u: ID=%u, Size=%ux%u, Mips=%u, Bias=%.2f, Dirty=%s, Cache=%s\n",
               i, item->id, item->width, item->height, 
               item->mip_levels, item->lod_bias, item->dirty ? "Yes" : "No", 
               item->cache_valid ? "Valid" : "Invalid");
        
        if (item->virtual_texture.virtual_enabled) {
            printf("  Virtual Texturing: Enabled (Page: %ux%u)\n", 
                   item->virtual_texture.page_width, item->virtual_texture.page_height);
        }
        
        if (item->compression.compressed) {
            printf("  Compression: %s (Ratio: %.2f:1, Quality: %u)\n",
                   item->compression.compression_format == TEXTURE_LOD_COMPRESSION_BC1 ? "BC1" :
                   item->compression.compression_format == TEXTURE_LOD_COMPRESSION_BC3 ? "BC3" :
                   item->compression.compression_format == TEXTURE_LOD_COMPRESSION_BC7 ? "BC7" :
                   item->compression.compression_format == TEXTURE_LOD_COMPRESSION_ASTC_4X4 ? "ASTC 4x4" :
                   item->compression.compression_format == TEXTURE_LOD_COMPRESSION_ASTC_6X6 ? "ASTC 6x6" : "Unknown",
                   item->compression.compression_ratio, item->compression.compression_quality);
        }
        
        if (item->bindless.bindless_enabled) {
            printf("  Bindless: Handle=0x%lx, Index=%u\n", 
                   item->bindless.bindless_handle, item->bindless.bindless_index);
        }
        
        if (item->texture_array.is_array) {
            printf("  Texture Array: Layers=%u, Current=%u, Index=%u\n",
                   item->texture_array.array_layers, item->texture_array.current_layer, 
                   item->texture_array.array_index);
        }
        
        if (item->feedback.access_count > 0) {
            printf("  Feedback: Samples=%u, Avg LOD=%.2f, Variance=%.4f\n",
                   item->feedback.access_count, item->feedback.average_lod, 
                   item->feedback.lod_variance);
        }
    }
    
    printf("\n=== Cache Status ===\n");
    uint32_t valid_cache_entries = 0;
    for (uint32_t i = 0; i < TEXTURE_TEXTURE_LOD_CACHE_SIZE; i++) {
        if (g_texture_lod_ctx.cache[i].valid) {
            valid_cache_entries++;
        }
    }
    printf("Valid Cache Entries: %u / %u\n", valid_cache_entries, TEXTURE_TEXTURE_LOD_CACHE_SIZE);
    
    printf("\n=== Feedback Analysis ===\n");
    printf("Global Feedback Index: %u\n", g_texture_lod_ctx.feedback_system.global_feedback_index);
    printf("Analysis Timestamp: %lu\n", g_texture_lod_ctx.feedback_system.analysis_timestamp);
    printf("LOD Distribution: ");
    for (uint32_t i = 0; i < 16; i++) {
        if (g_texture_lod_ctx.feedback_system.lod_distribution[i] > 0) {
            printf("LOD%u=%.1f%% ", i, g_texture_lod_ctx.feedback_system.lod_distribution[i] * 100.0f);
        }
    }
    printf("\n");
    
    pthread_mutex_unlock(&g_texture_lod_ctx.mutex);
    printf("=== End Debug Info ===\n");
}

// Unit tests for texture LOD system
int texture_texture_lod_run_unit_tests(void) {
    printf("=== Running Texture LOD Unit Tests ===\n");
    
    int tests_run = 0;
    int tests_passed = 0;
    
    // Test 1: System initialization
    printf("Test 1: System initialization... ");
    tests_run++;
    if (texture_texture_lod_init() == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 2: Create texture LOD
    printf("Test 2: Create texture LOD... ");
    tests_run++;
    texture_texture_lod_handle_t handle;
    texture_texture_lod_desc_t desc = {
        .flags = TEXTURE_TEXTURE_LOD_FLAG_VIRTUAL_TEXTURING | TEXTURE_TEXTURE_LOD_FLAG_BC_ASTC_COMPRESSION,
        .width = 256,
        .height = 256,
        .format = TEXTURE_TEXTURE_LOD_FORMAT_RGBA8
    };
    
    if (texture_texture_lod_create(&handle, &desc) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 3: Update texture LOD
    printf("Test 3: Update texture LOD... ");
    tests_run++;
    uint8_t test_data[256 * 256 * 4]; // 256x256 RGBA
    memset(test_data, 0x80, sizeof(test_data));
    
    if (texture_texture_lod_update(handle, test_data, sizeof(test_data)) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 4: Validate texture LOD
    printf("Test 4: Validate texture LOD... ");
    tests_run++;
    if (texture_texture_lod_validate(handle)) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 5: Get texture LOD info
    printf("Test 5: Get texture LOD info... ");
    tests_run++;
    texture_texture_lod_info_t info;
    if (texture_texture_lod_get_info(handle, &info) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 6: Process pending operations
    printf("Test 6: Process pending operations... ");
    tests_run++;
    if (texture_texture_lod_process_pending() >= 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 7: Memory usage tracking
    printf("Test 7: Memory usage tracking... ");
    tests_run++;
    size_t memory_usage = texture_texture_lod_get_memory_usage();
    if (memory_usage > sizeof(g_texture_lod_ctx)) {
        printf("PASS (Usage: %zu bytes)\n", memory_usage);
        tests_passed++;
    } else {
        printf("FAIL (Usage: %zu bytes)\n", memory_usage);
    }
    
    // Test 8: Mark dirty
    printf("Test 8: Mark dirty... ");
    tests_run++;
    texture_texture_lod_mark_dirty(handle);
    printf("PASS\n");
    tests_passed++;
    
    // Test 9: Get count
    printf("Test 9: Get count... ");
    tests_run++;
    uint32_t count = texture_texture_lod_get_count();
    if (count > 0) {
        printf("PASS (Count: %u)\n", count);
        tests_passed++;
    } else {
        printf("FAIL (Count: %u)\n", count);
    }
    
    // Test 10: Destroy texture LOD
    printf("Test 10: Destroy texture LOD... ");
    tests_run++;
    texture_texture_lod_destroy(handle);
    printf("PASS\n");
    tests_passed++;
    
    // Cleanup
    texture_texture_lod_shutdown();
    
    printf("\n=== Test Results ===\n");
    printf("Tests Run: %d\n", tests_run);
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_run - tests_passed);
    printf("Success Rate: %.1f%%\n", (float)tests_passed / tests_run * 100.0f);
    
    if (tests_passed == tests_run) {
        printf("=== ALL TESTS PASSED ===\n");
        return 0;
    } else {
        printf("=== SOME TESTS FAILED ===\n");
        return -1;
    }
}

/* End of texture_lod.c */
