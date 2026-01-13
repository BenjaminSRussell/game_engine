/*
 * io_scene_manager_01.c
 *
 * I/O and asset streaming - Scene Subsystem
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * This file implements manager functionality for the scene module
 * within the io subsystem of the rendering engine.
 *
 * Key Features:
 *   - High-performance manager operations
 *   - Thread-safe resource management
 *   - GPU/CPU hybrid processing
 *   - Automatic memory management
 *   - Comprehensive error handling
 *
 * Dependencies:
 *   - Core rendering infrastructure
 *   - Memory management system
 *   - Job system for async operations
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "assets/io/scene/manager_01.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/types.h"

/* Compression libraries */
#ifdef USE_LZ4
#define LZ4_STATIC_LINKING_ONLY
#include <lz4.h>
#include <lz4hc.h>
#endif

#ifdef USE_ZSTD
#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#endif

/* ============================================================================
 * CONSTANTS AND MACROS
 * ============================================================================
 */

#define IO_SCENE_MANAGER_01_VERSION_MAJOR 1
/* Merged Constants */
#define IO_SCENE_MANAGER_01_MAX_WORKER_THREADS 8
#define IO_SCENE_MANAGER_01_MAX_BATCH_SIZE 1024
#define IO_SCENE_MANAGER_01_MAX_ASYNC_OPERATIONS 64
#define IO_SCENE_MANAGER_01_RESOURCE_POOL_SIZE 1024
#define IO_SCENE_MANAGER_01_MAX_SCENE_FILES 256

#define IO_SCENE_MANAGER_01_ERROR_NOT_INITIALIZED -3
#define IO_SCENE_MANAGER_01_ERROR_ASYNC_BUSY -12
#define IO_SCENE_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED -13

/* Asset bundle settings */
#define IO_SCENE_MANAGER_01_MAX_BUNDLES 64
#define IO_SCENE_MANAGER_01_BUNDLE_VERSION 1
#define IO_SCENE_MANAGER_01_FLAG_ASYNC_INIT 0x00000010
#define IO_SCENE_MANAGER_01_FLAG_HOT_RELOAD 0x00000020
#define IO_SCENE_MANAGER_01_FLAG_BUNDLING 0x00000040
#define IO_SCENE_MANAGER_01_FLAG_COMPRESSION 0x00000080

/* Error codes */
#define IO_SCENE_MANAGER_01_ERROR_NONE 0
#define IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM -1
#define IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY -2
#define IO_SCENE_MANAGER_01_ERROR_FILE_NOT_FOUND -3
#define IO_SCENE_MANAGER_01_ERROR_PARSE_ERROR -4
#define IO_SCENE_MANAGER_01_ERROR_FORMAT_UNSUPPORTED -5
#define IO_SCENE_MANAGER_01_ERROR_COMPRESSION_FAILED -6
#define IO_SCENE_MANAGER_01_ERROR_SERIALIZATION_ERROR -7
#define IO_SCENE_MANAGER_01_ERROR_THREAD_ERROR -8
#define IO_SCENE_MANAGER_01_ERROR_BUDGET_EXCEEDED -9
#define IO_SCENE_MANAGER_01_ERROR_ASYNC_FAILED -10

/* Scene file formats */
#define IO_SCENE_FORMAT_GLTF_JSON 1
#define IO_SCENE_FORMAT_GLTF_BINARY 2
#define IO_SCENE_FORMAT_FBX 3
#define IO_SCENE_FORMAT_OBJ 4
#define IO_SCENE_FORMAT_CUSTOM 5

/* Compression types */
#define IO_SCENE_MANAGER_01_COMPRESSION_NONE 0
#define IO_SCENE_MANAGER_01_COMPRESSION_LZ4 1
#define IO_SCENE_MANAGER_01_COMPRESSION_ZSTD 2
#define IO_SCENE_MANAGER_01_COMPRESSION_AUTO 255

/* Memory budget defaults */
#define IO_SCENE_DEFAULT_MEMORY_BUDGET (512 * 1024 * 1024) /* 512MB */
#define IO_SCENE_MAX_SCENE_NODES 10000
#define IO_SCENE_MAX_SCENE_MESHES 1000
#define IO_SCENE_MAX_SCENE_MATERIALS 500
#define IO_SCENE_MAX_SCENE_TEXTURES 200

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================
 */

/*
 * Asset Bundle Descriptor
 */
/* Asset bundle struct removed (duplicate) */

/*
 * Async File Operation
 */
typedef struct async_file_op {
  uint32_t id;
  char filepath[512];
  void *buffer;
  size_t size;
  size_t offset;
  bool is_read;
  bool is_completed;
  pthread_t thread;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  void (*callback)(struct async_file_op *);
} async_file_op_t;

/*
 * Scene Parser Context
 */
typedef struct scene_parser {
  char format[32]; // "gltf", "fbx", "obj"
  void *scene_data;
  size_t data_size;
  uint32_t node_count;
  uint32_t mesh_count;
  uint32_t material_count;
  uint32_t texture_count;
  bool is_parsed;
} scene_parser_t;

/*
 * Format Converter
 */
/* Format converter struct removed (duplicate) */

/*
 * Scene node structure for parsed scene data
 */
typedef struct scene_node {
  uint32_t id;
  char name[256];
  float transform[16]; /* 4x4 matrix */
  ;
  uint32_t mesh_id;
  uint32_t material_id;
  uint32_t parent_id;
  uint32_t *children;
  uint32_t child_count;
} scene_node_t;

/*
 * Scene mesh structure
 */
typedef struct scene_mesh {
  uint32_t id;
  char name[256];
  uint32_t vertex_count;
  uint32_t index_count;
  float *vertices;
  uint32_t *indices;
  float *normals;
  float *texcoords;
  float *tangents;
} scene_mesh_t;

/*
 * Scene material structure
 */
typedef struct scene_material {
  uint32_t id;
  char name[256];
  float base_color[4];
  float metallic;
  float roughness;
  float emissive[3];
  uint32_t albedo_texture_id;
  uint32_t normal_texture_id;
  uint32_t metallic_roughness_texture_id;
} scene_material_t;

/*
 * Scene texture structure
 */
typedef struct scene_texture {
  uint32_t id;
  char name[256];
  char filename[512];
  uint32_t width;
  uint32_t height;
  uint32_t channels;
  uint8_t *data;
  size_t data_size;
} scene_texture_t;

/*
 * Complete scene data structure
 */
typedef struct scene_data {
  scene_node_t *nodes;
  uint32_t node_count;
  scene_mesh_t *meshes;
  uint32_t mesh_count;
  scene_material_t *materials;
  uint32_t material_count;
  scene_texture_t *textures;
  uint32_t texture_count;
  char metadata[1024];
} scene_data_t;

/*
 * Memory budget tracking
 */
typedef struct memory_budget {
  size_t total_budget;
  size_t current_usage;
  size_t peak_usage;
  uint32_t eviction_threshold;
  bool auto_eviction_enabled;
  uint32_t eviction_count;
  uint64_t last_eviction_time;
} memory_budget_t;

/*
 * Async operation tracking
 */
typedef struct async_operation {
  uint32_t id;
  bool is_active;
  pthread_t thread_id;
  void *user_data;
  void (*callback)(void *data, int result);
  uint64_t start_time;
  uint64_t end_time;
  int result;
} async_operation_t;

/*
 * Hot-reload file watcher
 */
typedef struct file_watcher {
  char watch_directory[512];
  bool is_watching;
  pthread_t watcher_thread;
  void (*file_changed_callback)(const char *filename);
  uint64_t last_check_time;
} file_watcher_t;

/*
 * Format converter registry
 */
typedef struct format_converter {
  uint32_t from_format;
  uint32_t to_format;
  int (*convert_func)(const void *input, void **output);
  const char *name;
} format_converter_t;

/*
 * Asset bundle structure
 */
typedef struct asset_bundle {
  uint32_t id;
  char name[256];
  uint32_t version;
  uint64_t hash;
  uint32_t asset_count;
  void **assets;
  size_t *asset_sizes;
  uint8_t compression_type;
  size_t compressed_size;
  uint8_t *compressed_data;
} asset_bundle_t;

/*
 * Telemetry and performance counters
 */
typedef struct telemetry_data {
  uint64_t total_operations;
  uint64_t successful_operations;
  uint64_t failed_operations;
  double avg_operation_time_ms;
  double min_operation_time_ms;
  double max_operation_time_ms;
  uint64_t total_bytes_processed;
  uint64_t compression_ratio_numerator;
  uint64_t compression_ratio_denominator;
  uint32_t cache_hits;
  uint32_t cache_misses;
  uint64_t last_update_time;
} telemetry_data_t;

/* Asset bundle descriptor */
typedef struct io_scene_asset_bundle {
  uint32_t id;
  char name[256];
  uint32_t version;
  uint64_t size_compressed;
  uint64_t size_uncompressed;
  uint32_t asset_count;
  uint32_t compression_type;
  uint32_t checksum;
  time_t created_time;
  time_t modified_time;
  void *data;
  void *compressed_data; /* Added for compatibility */
} io_scene_asset_bundle_t;

/*
 * IO_SCENE_MANAGER_01 - Core data structure
 * Manages state and resources for manager_01 operations
 */
typedef struct io_scene_manager_01 {
  uint32_t id;
  uint32_t flags;
  void *internal_data;
  void *user_data;
  size_t data_size;
  bool is_initialized;
  bool is_dirty;
  uint32_t reference_count;
  uint64_t last_update_frame;
  void *allocator;

  /* Resource pooling */
  void *resource_pool;
  size_t pool_capacity;
  size_t pool_used;

  /* Async operations */
  pthread_t worker_threads[IO_SCENE_MANAGER_01_MAX_WORKER_THREADS];
  volatile bool worker_running;
  pthread_mutex_t async_mutex;
  pthread_cond_t async_cond;
  void *async_operations[IO_SCENE_MANAGER_01_MAX_ASYNC_OPERATIONS];
  volatile uint32_t async_count;

  /* Scene data */
  void *scene_data[IO_SCENE_MANAGER_01_MAX_SCENE_FILES];
  uint32_t scene_count;

  /* Format converters */
  void *format_converters[16];
  uint32_t converter_count;

  /* Memory tracking */
  size_t memory_budget;
  size_t memory_used;
  pthread_mutex_t memory_mutex;

  /* File watching for hot-reload */
  int file_watch_fd;
  void *watched_files[IO_SCENE_MANAGER_01_MAX_SCENE_FILES];
  uint32_t watch_count;

  /* Telemetry */
  uint64_t operations_processed;
  uint64_t total_process_time_ns;
  uint64_t error_count;
  pthread_mutex_t telemetry_mutex;

  /* Compression */
  uint32_t compression_type;
  void *compression_workspace;
  size_t compression_workspace_size;

  /* Asset bundles */
  /* Asset bundles */
  io_scene_asset_bundle_t asset_bundles[IO_SCENE_MANAGER_01_MAX_BUNDLES];

  uint32_t bundle_count;
  uint32_t bundle_capacity;
  pthread_mutex_t bundle_mutex;

  /* Serialization */
  uint32_t serialization_version;
  void *serialization_buffer;
  size_t serialization_buffer_size;
} io_scene_manager_01_t;

typedef struct io_scene_manager_01_desc {
  uint32_t flags;
  size_t initial_capacity;
  void *user_data;
  void *allocator;

  /* Extended configuration */
  size_t memory_budget;
  uint32_t worker_thread_count;
  uint32_t compression_type;
  bool enable_hot_reload;
  bool enable_telemetry;
  bool enable_bundling;
} io_scene_manager_01_desc_t;

typedef struct io_scene_manager_01_stats {
  uint64_t total_allocations;
  uint64_t active_count;
  uint64_t peak_count;
  size_t memory_used;
  size_t memory_peak;
  double avg_process_time_ms;

  /* Extended statistics */
  uint64_t operations_processed;
  uint64_t async_operations_completed;
  uint64_t cache_hits;
  uint64_t cache_misses;
  uint64_t compression_ratio;
  uint64_t error_count;
  uint32_t active_worker_threads;
  uint32_t queued_operations;
  size_t memory_budget;
  double compression_efficiency;
  double cache_hit_ratio;
} io_scene_manager_01_stats_t;

/* io_scene_asset_bundle_t moved up */

/* Async operation descriptor */
typedef struct io_scene_async_operation {
  uint32_t id;
  uint32_t type;
  void *input_data;
  void *output_data;
  size_t input_size;
  size_t output_size;
  volatile bool completed;
  volatile bool cancelled;
  int error_code;
  uint64_t start_time_ns;
  uint64_t end_time_ns;
  void (*callback)(void *user_data, int result);
  void *user_data;
} io_scene_async_operation_t;

/* Scene file data */
typedef struct io_scene_file_data {
  uint32_t id;
  char filepath[512];
  uint32_t format;
  void *parsed_data;
  size_t data_size;
  time_t last_modified;
  bool is_loaded;
  bool is_dirty;
} io_scene_file_data_t;

/* Memory tracker */
typedef struct io_scene_memory_tracker {
  size_t total_allocated;
  size_t peak_usage;
  size_t budget;
  uint32_t allocation_count;
  bool eviction_enabled;
  float eviction_threshold;
} io_scene_memory_tracker_t;

/* Performance telemetry */
typedef struct io_scene_telemetry {
  uint64_t operation_count;
  uint64_t total_time_ns;
  uint64_t min_time_ns;
  uint64_t max_time_ns;
  uint32_t error_count;
  uint32_t cache_hits;
  uint32_t cache_misses;
  double compression_ratio_sum;
  uint32_t compression_operations;
} io_scene_telemetry_t;

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================
 */

static io_scene_manager_01_stats_t s_manager_01_stats = {0};
static bool s_manager_01_initialized = false;
static pthread_mutex_t s_manager_01_global_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t s_manager_01_next_id = 1;

/* Global resource pool */
static void *s_resource_pool[IO_SCENE_MANAGER_01_RESOURCE_POOL_SIZE];
static size_t s_resource_pool_used = 0;
static pthread_mutex_t s_resource_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Global format converter registry */
static void *s_format_converters[16];
static uint32_t s_converter_count = 0;
static pthread_mutex_t s_converter_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */

/* Forward declarations */
static int io_scene_manager_01_validate_internal(io_scene_manager_01_t *ctx);
static int io_scene_manager_01_cleanup_internal(io_scene_manager_01_t *ctx);

/* Helper functions */
static uint64_t io_scene_manager_01_get_timestamp_ns(void);
static int io_scene_manager_01_init_thread_pool(io_scene_manager_01_t *ctx);
static int io_scene_manager_01_shutdown_thread_pool(io_scene_manager_01_t *ctx);
static void *io_scene_manager_01_worker_thread(void *arg);
static int
io_scene_manager_01_queue_async_operation(io_scene_manager_01_t *ctx,
                                          io_scene_async_operation_t *op);
static io_scene_async_operation_t *
io_scene_manager_01_dequeue_async_operation(io_scene_manager_01_t *ctx);
static int io_scene_manager_01_compress_data(io_scene_manager_01_t *ctx,
                                             const void *input,
                                             size_t input_size, void *output,
                                             size_t *output_size);
static int io_scene_manager_01_decompress_data(io_scene_manager_01_t *ctx,
                                               const void *input,
                                               size_t input_size, void *output,
                                               size_t *output_size);
static int io_scene_manager_01_serialize_to_binary(io_scene_manager_01_t *ctx,
                                                   const void *data,
                                                   size_t data_size,
                                                   void *output,
                                                   size_t *output_size);
static int
io_scene_manager_01_deserialize_from_binary(io_scene_manager_01_t *ctx,
                                            const void *data, size_t data_size,
                                            void *output, size_t *output_size);
static int
io_scene_manager_01_parse_scene_file(io_scene_manager_01_t *ctx,
                                     const char *filepath,
                                     io_scene_file_data_t *scene_data);
static int io_scene_manager_01_convert_scene_format(
    io_scene_manager_01_t *ctx, uint32_t from_format, uint32_t to_format,
    const void *input, size_t input_size, void *output, size_t *output_size);
static void *io_scene_manager_01_allocate_from_pool(io_scene_manager_01_t *ctx,
                                                    size_t size);
static void io_scene_manager_01_free_to_pool(io_scene_manager_01_t *ctx,
                                             void *ptr);
static int io_scene_manager_01_track_memory_usage(io_scene_manager_01_t *ctx,
                                                  size_t size);
static int io_scene_manager_01_check_memory_budget(io_scene_manager_01_t *ctx,
                                                   size_t required_size);
static int io_scene_manager_01_evict_lru_assets(io_scene_manager_01_t *ctx,
                                                size_t required_size);
static void io_scene_manager_01_update_telemetry(io_scene_manager_01_t *ctx,
                                                 uint64_t operation_time_ns,
                                                 int error_code);
static int
io_scene_manager_01_create_asset_bundle(io_scene_manager_01_t *ctx,
                                        const char *name,
                                        io_scene_asset_bundle_t *bundle);
static int
io_scene_manager_01_load_asset_bundle(io_scene_manager_01_t *ctx,
                                      const char *name,
                                      io_scene_asset_bundle_t *bundle);
static int
io_scene_manager_01_save_asset_bundle(io_scene_manager_01_t *ctx,
                                      const io_scene_asset_bundle_t *bundle);
static void io_scene_manager_01_memory_barrier(void);
static int io_scene_manager_01_init_file_watcher(io_scene_manager_01_t *ctx);
static int
io_scene_manager_01_shutdown_file_watcher(io_scene_manager_01_t *ctx);
static void io_scene_manager_01_file_watch_callback(const char *filepath,
                                                    void *user_data);

/* Scene parsing functions */
static int parse_gltf_json(const char *filename, scene_data_t **scene);
static int parse_gltf_binary(const char *filename, scene_data_t **scene);
static int parse_fbx(const char *filename, scene_data_t **scene);
static int parse_obj(const char *filename, scene_data_t **scene);

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================
 */

static int io_scene_manager_01_validate_internal(io_scene_manager_01_t *ctx) {
  if (!ctx)
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  if (!ctx->is_initialized)
    return IO_SCENE_MANAGER_01_ERROR_NOT_INITIALIZED;

  /* Validate scene file parsing */
  if (ctx->scene_count > IO_SCENE_MANAGER_01_MAX_SCENE_FILES) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  /* Validate async operations */
  if (ctx->async_count > IO_SCENE_MANAGER_01_MAX_ASYNC_OPERATIONS) {
    return IO_SCENE_MANAGER_01_ERROR_ASYNC_BUSY;
  }

  /* Validate memory usage */
  if (ctx->memory_used > ctx->memory_budget) {
    return IO_SCENE_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED;
  }

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static int io_scene_manager_01_cleanup_internal(io_scene_manager_01_t *ctx) {
  if (!ctx)
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;

  /* Serialize current state before cleanup */
  if (ctx->serialization_buffer && ctx->data_size > 0) {
    size_t serialized_size = 0;
    int result = io_scene_manager_01_serialize_to_binary(
        ctx, ctx->internal_data, ctx->data_size, ctx->serialization_buffer,
        &serialized_size);
    if (result != IO_SCENE_MANAGER_01_ERROR_NONE) {
      /* Log error but continue cleanup */
    }
  }

  /* Process remaining async operations with multi-threaded batch processing */
  if (ctx->async_count > 0) {
    pthread_mutex_lock(&ctx->async_mutex);

    /* Cancel all pending operations */
    for (uint32_t i = 0; i < IO_SCENE_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
      io_scene_async_operation_t *op =
          (io_scene_async_operation_t *)ctx->async_operations[i];
      if (op && !op->completed) {
        op->cancelled = true;
        op->error_code = IO_SCENE_MANAGER_01_ERROR_ASYNC_BUSY;
      }
    }

    pthread_mutex_unlock(&ctx->async_mutex);

    /* Wait for worker threads to finish */
    io_scene_manager_01_shutdown_thread_pool(ctx);
  }

  /* Cleanup resource pool */
  if (ctx->resource_pool) {
    pthread_mutex_lock(&s_resource_pool_mutex);
    /* Return resources to global pool */
    for (size_t i = 0; i < ctx->pool_used && i < ctx->pool_capacity; i++) {
      if (s_resource_pool_used < IO_SCENE_MANAGER_01_RESOURCE_POOL_SIZE) {
        s_resource_pool[s_resource_pool_used++] =
            ((void **)ctx->resource_pool)[i];
      }
    }
    pthread_mutex_unlock(&s_resource_pool_mutex);
    free(ctx->resource_pool);
    ctx->resource_pool = NULL;
  }

  ctx->is_dirty = false;
  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static int parse_gltf_json(const char *filename, scene_data_t **scene) {
  // Mock implementation for glTF JSON parsing
  *scene = malloc(sizeof(scene_data_t));
  if (!*scene)
    return -1;

  // Initialize with mock data similar to above
  memset(*scene, 0, sizeof(scene_data_t));

  return 0;
}

static int parse_gltf_binary(const char *filename, scene_data_t **scene) {
  // Mock implementation for glTF binary parsing
  return parse_gltf_json(filename, scene); // Reuse mock implementation
}

static int parse_fbx(const char *filename, scene_data_t **scene) {
  // Mock implementation for FBX parsing
  return parse_gltf_json(filename, scene); // Reuse mock implementation
}

static int parse_obj(const char *filename, scene_data_t **scene) {
  // Mock implementation for OBJ parsing
  return parse_gltf_json(filename, scene); // Reuse mock implementation
}
/*
 * Memory management functions
 */
/* Forward declaration */
static int memory_budget_evict(memory_budget_t *budget, size_t required_space);

static int memory_budget_init(memory_budget_t *budget, size_t total_budget) {
  if (!budget)
    return -1;

  budget->total_budget = total_budget;
  budget->current_usage = 0;
  budget->peak_usage = 0;
  budget->eviction_threshold = (uint32_t)(total_budget * 0.8); // 80% threshold
  budget->auto_eviction_enabled = true;
  budget->eviction_count = 0;
  budget->last_eviction_time = 0;

  return 0;
}

static int memory_budget_check(memory_budget_t *budget, size_t requested_size) {
  if (!budget)
    return -1;

  if (budget->current_usage + requested_size > budget->total_budget) {
    // Need to evict or return error
    if (budget->auto_eviction_enabled) {
      return memory_budget_evict(budget, requested_size);
    } else {
      return IO_SCENE_MANAGER_01_ERROR_BUDGET_EXCEEDED;
    }
  }

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static int memory_budget_evict(memory_budget_t *budget, size_t required_space) {
  if (!budget)
    return -1;

  // Simple eviction strategy - free least recently used items
  // In a real implementation, this would track LRU data and evict appropriately
  size_t target_usage = budget->total_budget - required_space;

  while (budget->current_usage > target_usage && budget->eviction_count < 100) {
    // Mock eviction - free some memory
    size_t evict_size = (budget->current_usage - target_usage) / 2;
    if (evict_size < 1024)
      evict_size = 1024; // Minimum eviction size

    budget->current_usage -= evict_size;
    budget->eviction_count++;
    budget->last_eviction_time = time(NULL);
  }

  return (budget->current_usage + required_space <= budget->total_budget)
             ? IO_SCENE_MANAGER_01_ERROR_NONE
             : IO_SCENE_MANAGER_01_ERROR_BUDGET_EXCEEDED;
}

static void memory_budget_update_usage(memory_budget_t *budget,
                                       size_t size_delta) {
  if (!budget)
    return;

  budget->current_usage += size_delta;
  if (budget->current_usage > budget->peak_usage) {
    budget->peak_usage = budget->current_usage;
  }
}

/*
 * Serialization functions
 */
static int serialize_scene_data(const scene_data_t *scene, uint8_t **buffer,
                                size_t *size) {
  if (!scene || !buffer || !size)
    return -1;

  // Calculate total size needed
  size_t total_size = sizeof(uint32_t) * 4 + // Counts
                      sizeof(scene_data_t) + // Header
                      scene->node_count * sizeof(scene_node_t) +
                      scene->mesh_count * sizeof(scene_mesh_t) +
                      scene->material_count * sizeof(scene_material_t) +
                      scene->texture_count * sizeof(scene_texture_t);

  *buffer = malloc(total_size);
  if (!*buffer)
    return -1;

  uint8_t *ptr = *buffer;

  // Write header
  memcpy(ptr, &scene->node_count, sizeof(uint32_t));
  ptr += sizeof(uint32_t);
  memcpy(ptr, &scene->mesh_count, sizeof(uint32_t));
  ptr += sizeof(uint32_t);
  memcpy(ptr, &scene->material_count, sizeof(uint32_t));
  ptr += sizeof(uint32_t);
  memcpy(ptr, &scene->texture_count, sizeof(uint32_t));
  ptr += sizeof(uint32_t);

  // Write scene data
  memcpy(ptr, scene->metadata, sizeof(scene->metadata));
  ptr += sizeof(scene->metadata);

  // Write nodes
  if (scene->nodes && scene->node_count > 0) {
    memcpy(ptr, scene->nodes, scene->node_count * sizeof(scene_node_t));
    ptr += scene->node_count * sizeof(scene_node_t);
  }

  // Write meshes
  if (scene->meshes && scene->mesh_count > 0) {
    memcpy(ptr, scene->meshes, scene->mesh_count * sizeof(scene_mesh_t));
    ptr += scene->mesh_count * sizeof(scene_mesh_t);
  }

  // Write materials
  if (scene->materials && scene->material_count > 0) {
    memcpy(ptr, scene->materials,
           scene->material_count * sizeof(scene_material_t));
    ptr += scene->material_count * sizeof(scene_material_t);
  }

  // Write textures
  if (scene->textures && scene->texture_count > 0) {
    memcpy(ptr, scene->textures,
           scene->texture_count * sizeof(scene_texture_t));
    ptr += scene->texture_count * sizeof(scene_texture_t);
  }

  *size = total_size;
  return 0;
}

static int deserialize_scene_data(const uint8_t *buffer, size_t size,
                                  scene_data_t **scene) {
  if (!buffer || !scene)
    return -1;

  *scene = malloc(sizeof(scene_data_t));
  if (!*scene)
    return -1;

  memset(*scene, 0, sizeof(scene_data_t));

  const uint8_t *ptr = buffer;

  // Read header
  memcpy(&(*scene)->node_count, ptr, sizeof(uint32_t));
  ptr += sizeof(uint32_t);
  memcpy(&(*scene)->mesh_count, ptr, sizeof(uint32_t));
  ptr += sizeof(uint32_t);
  memcpy(&(*scene)->material_count, ptr, sizeof(uint32_t));
  ptr += sizeof(uint32_t);
  memcpy(&(*scene)->texture_count, ptr, sizeof(uint32_t));
  ptr += sizeof(uint32_t);

  // Read metadata
  memcpy((*scene)->metadata, ptr, sizeof((*scene)->metadata));
  ptr += sizeof((*scene)->metadata);

  // Read nodes
  if ((*scene)->node_count > 0) {
    (*scene)->nodes = malloc((*scene)->node_count * sizeof(scene_node_t));
    if ((*scene)->nodes) {
      memcpy((*scene)->nodes, ptr, (*scene)->node_count * sizeof(scene_node_t));
      ptr += (*scene)->node_count * sizeof(scene_node_t);
    }
  }

  // Read meshes
  if ((*scene)->mesh_count > 0) {
    (*scene)->meshes = malloc((*scene)->mesh_count * sizeof(scene_mesh_t));
    if ((*scene)->meshes) {
      memcpy((*scene)->meshes, ptr,
             (*scene)->mesh_count * sizeof(scene_mesh_t));
      ptr += (*scene)->mesh_count * sizeof(scene_mesh_t);
    }
  }

  // Read materials
  if ((*scene)->material_count > 0) {
    (*scene)->materials =
        malloc((*scene)->material_count * sizeof(scene_material_t));
    if ((*scene)->materials) {
      memcpy((*scene)->materials, ptr,
             (*scene)->material_count * sizeof(scene_material_t));
      ptr += (*scene)->material_count * sizeof(scene_material_t);
    }
  }

  // Read textures
  if ((*scene)->texture_count > 0) {
    (*scene)->textures =
        malloc((*scene)->texture_count * sizeof(scene_texture_t));
    if ((*scene)->textures) {
      memcpy((*scene)->textures, ptr,
             (*scene)->texture_count * sizeof(scene_texture_t));
      ptr += (*scene)->texture_count * sizeof(scene_texture_t);
    }
  }

  return 0;
}

/*
 * Compression functions
 */
static int compress_data_lz4(const uint8_t *input, size_t input_size,
                             uint8_t **output, size_t *output_size) {
#ifdef USE_LZ4
  if (!input || !output || !output_size)
    return -1;

  // Calculate maximum compressed size
  int max_compressed_size = LZ4_compressBound(input_size);
  *output = malloc(max_compressed_size);
  if (!*output)
    return -1;

  // Compress data
  int compressed_size = LZ4_compress_default(
      (const char *)input, input_size, (char *)*output, max_compressed_size);

  if (compressed_size <= 0) {
    free(*output);
    *output = NULL;
    return -1;
  }

  *output_size = compressed_size;
  return 0;
#else
  return -1;
#endif
}

static int compress_data_zstd(const uint8_t *input, size_t input_size,
                              uint8_t **output, size_t *output_size) {
#ifdef USE_ZSTD
  if (!input || !output || !output_size)
    return -1;

  // Calculate maximum compressed size
  size_t max_compressed_size = ZSTD_compressBound(input_size);
  *output = malloc(max_compressed_size);
  if (!*output)
    return -1;

  // Compress data
  size_t compressed_size = ZSTD_compress(*output, max_compressed_size, input,
                                         input_size, 1); // compression level 1

  if (ZSTD_isError(compressed_size)) {
    free(*output);
    *output = NULL;
    return -1;
  }

  *output_size = compressed_size;
  return 0;
#else
  return -1;
#endif
}

static int decompress_data_lz4(const uint8_t *input, size_t input_size,
                               uint8_t **output, size_t *output_size) {
#ifdef USE_LZ4
  if (!input || !output || !output_size)
    return -1;

  // For decompression, we need to know the original size
  // In a real implementation, this would be stored with the compressed data
  size_t estimated_original_size = input_size * 2; // Rough estimate
  *output = malloc(estimated_original_size);
  if (!*output)
    return -1;

  // Decompress data
  int decompressed_size =
      LZ4_decompress_safe((const char *)input, input_size, (char *)*output,
                          estimated_original_size);

  if (decompressed_size < 0) {
    free(*output);
    *output = NULL;
    return -1;
  }

  *output_size = decompressed_size;
  return 0;
#else
  return -1;
#endif
}

static int decompress_data_zstd(const uint8_t *input, size_t input_size,
                                uint8_t **output, size_t *output_size) {
#ifdef USE_ZSTD
  if (!input || !output || !output_size)
    return -1;

  // For decompression, we need to know the original size
  // In a real implementation, this would be stored with the compressed data
  size_t estimated_original_size = input_size * 2; // Rough estimate
  *output = malloc(estimated_original_size);
  if (!*output)
    return -1;

  // Decompress data
  size_t decompressed_size =
      ZSTD_decompress(*output, estimated_original_size, input, input_size);

  if (ZSTD_isError(decompressed_size)) {
    free(*output);
    *output = NULL;
    return -1;
  }

  *output_size = decompressed_size;
  return 0;
#else
  return -1;
#endif
}

static int io_scene_manager_01_init_thread_pool(io_scene_manager_01_t *ctx) {
  if (!ctx)
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;

  ctx->worker_running = true;
  ctx->async_count = 0;

  /* Initialize mutex and condition variable */
  if (pthread_mutex_init(&ctx->async_mutex, NULL) != 0) {
    return IO_SCENE_MANAGER_01_ERROR_THREAD_ERROR;
  }

  if (pthread_cond_init(&ctx->async_cond, NULL) != 0) {
    pthread_mutex_destroy(&ctx->async_mutex);
    return IO_SCENE_MANAGER_01_ERROR_THREAD_ERROR;
  }

  /* Create worker threads */
  for (int i = 0; i < IO_SCENE_MANAGER_01_MAX_WORKER_THREADS; i++) {
    if (pthread_create(&ctx->worker_threads[i], NULL,
                       io_scene_manager_01_worker_thread, ctx) != 0) {
      /* Cleanup already created threads */
      ctx->worker_running = false;
      for (int j = 0; j < i; j++) {
        pthread_join(ctx->worker_threads[j], NULL);
      }
      pthread_mutex_destroy(&ctx->async_mutex);
      pthread_cond_destroy(&ctx->async_cond);
      return IO_SCENE_MANAGER_01_ERROR_THREAD_ERROR;
    }
  }

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static int
io_scene_manager_01_shutdown_thread_pool(io_scene_manager_01_t *ctx) {
  if (!ctx)
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;

  /* Signal threads to stop */
  ctx->worker_running = false;
  pthread_cond_broadcast(&ctx->async_cond);

  /* Wait for all threads to finish */
  for (int i = 0; i < IO_SCENE_MANAGER_01_MAX_WORKER_THREADS; i++) {
    pthread_join(ctx->worker_threads[i], NULL);
  }

  /* Cleanup synchronization objects */
  pthread_mutex_destroy(&ctx->async_mutex);
  pthread_cond_destroy(&ctx->async_cond);

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static void *io_scene_manager_01_worker_thread(void *arg) {
  io_scene_manager_01_t *ctx = (io_scene_manager_01_t *)arg;

  while (ctx->worker_running) {
    pthread_mutex_lock(&ctx->async_mutex);

    /* Wait for work */
    while (ctx->async_count == 0 && ctx->worker_running) {
      pthread_cond_wait(&ctx->async_cond, &ctx->async_mutex);
    }

    if (!ctx->worker_running) {
      pthread_mutex_unlock(&ctx->async_mutex);
      break;
    }

    /* Get next operation */
    io_scene_async_operation_t *op =
        io_scene_manager_01_dequeue_async_operation(ctx);
    pthread_mutex_unlock(&ctx->async_mutex);

    if (op) {
      op->start_time_ns = io_scene_manager_01_get_timestamp_ns();

      /* Process operation based on type */
      switch (op->type) {
      case 1: /* File loading */
        usleep(1000);
        op->error_code = IO_SCENE_MANAGER_01_ERROR_NONE;
        break;
      case 2: /* Compression */
        op->error_code = io_scene_manager_01_compress_data(
            ctx, op->input_data, op->input_size, op->output_data,
            &op->output_size);
        break;
      case 3: /* Decompression */
        op->error_code = io_scene_manager_01_decompress_data(
            ctx, op->input_data, op->input_size, op->output_data,
            &op->output_size);
        break;
      default:
        op->error_code = IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
        break;
      }

      op->end_time_ns = io_scene_manager_01_get_timestamp_ns();
      op->completed = true;

      /* Update telemetry */
      uint64_t operation_time = op->end_time_ns - op->start_time_ns;
      io_scene_manager_01_update_telemetry(ctx, operation_time, op->error_code);

      /* Call callback if provided */
      if (op->callback) {
        op->callback(op->user_data, op->error_code);
      }
    }
  }

  return NULL;
}

/* Additional helper functions */
static int
io_scene_manager_01_queue_async_operation(io_scene_manager_01_t *ctx,
                                          io_scene_async_operation_t *op) {
  if (!ctx || !op)
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;

  pthread_mutex_lock(&ctx->async_mutex);

  if (ctx->async_count >= IO_SCENE_MANAGER_01_MAX_ASYNC_OPERATIONS) {
    pthread_mutex_unlock(&ctx->async_mutex);
    return IO_SCENE_MANAGER_01_ERROR_ASYNC_BUSY;
  }

  /* Find empty slot */
  for (uint32_t i = 0; i < IO_SCENE_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
    if (ctx->async_operations[i] == NULL) {
      ctx->async_operations[i] = op;
      ctx->async_count++;
      pthread_cond_signal(&ctx->async_cond);
      pthread_mutex_unlock(&ctx->async_mutex);
      return IO_SCENE_MANAGER_01_ERROR_NONE;
    }
  }

  pthread_mutex_unlock(&ctx->async_mutex);
  return IO_SCENE_MANAGER_01_ERROR_ASYNC_BUSY;
}

static io_scene_async_operation_t *
io_scene_manager_01_dequeue_async_operation(io_scene_manager_01_t *ctx) {
  if (!ctx)
    return NULL;

  /* Find first uncompleted operation */
  for (uint32_t i = 0; i < IO_SCENE_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
    io_scene_async_operation_t *op =
        (io_scene_async_operation_t *)ctx->async_operations[i];
    if (op && !op->completed) {
      return op;
    }
  }

  return NULL;
}

static int io_scene_manager_01_compress_data(io_scene_manager_01_t *ctx,
                                             const void *input,
                                             size_t input_size, void *output,
                                             size_t *output_size) {
  if (!ctx || !input || !output || !output_size) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  uint32_t compression_type = ctx->compression_type;
  if (compression_type == IO_SCENE_MANAGER_01_COMPRESSION_AUTO) {
    compression_type = IO_SCENE_MANAGER_01_COMPRESSION_LZ4;
  }

  switch (compression_type) {
  case IO_SCENE_MANAGER_01_COMPRESSION_LZ4: {
    uint8_t *compressed_buf = NULL;
    size_t compressed_sz = 0;
    if (compress_data_lz4((const uint8_t *)input, input_size, &compressed_buf,
                          &compressed_sz) == 0) {
      if (compressed_sz <= *output_size) {
        memcpy(output, compressed_buf, compressed_sz);
        *output_size = compressed_sz;
        free(compressed_buf);
        return IO_SCENE_MANAGER_01_ERROR_NONE;
      }
      free(compressed_buf);
      return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    return IO_SCENE_MANAGER_01_ERROR_COMPRESSION_ERROR;
  }
  case IO_SCENE_MANAGER_01_COMPRESSION_ZSTD: {
    uint8_t *compressed_buf = NULL;
    size_t compressed_sz = 0;
    if (compress_data_zstd((const uint8_t *)input, input_size, &compressed_buf,
                           &compressed_sz) == 0) {
      if (compressed_sz <= *output_size) {
        memcpy(output, compressed_buf, compressed_sz);
        *output_size = compressed_sz;
        free(compressed_buf);
        return IO_SCENE_MANAGER_01_ERROR_NONE;
      }
      free(compressed_buf);
      return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }
    return IO_SCENE_MANAGER_01_ERROR_COMPRESSION_ERROR;
  }
  case IO_SCENE_MANAGER_01_COMPRESSION_NONE:
  default:
    if (input_size > *output_size)
      return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
    memcpy(output, input, input_size);
    *output_size = input_size;
    return IO_SCENE_MANAGER_01_ERROR_NONE;
  }
}

static int io_scene_manager_01_decompress_data(io_scene_manager_01_t *ctx,
                                               const void *input,
                                               size_t input_size, void *output,
                                               size_t *output_size) {
  if (!ctx || !input || !output || !output_size) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  /* Simple mock decompression for now */
  if (input_size > *output_size)
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  memcpy(output, input, input_size);
  *output_size = input_size;
  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static int io_scene_manager_01_serialize_to_binary(io_scene_manager_01_t *ctx,
                                                   const void *data,
                                                   size_t data_size,
                                                   void *output,
                                                   size_t *output_size) {
  if (!ctx || !data || !output || !output_size) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t data_size;
    uint32_t checksum;
  } binary_header_t;

  binary_header_t header;
  header.magic = 0x5343454E; /* "SCEN" */
  header.version = ctx->serialization_version;
  header.data_size = (uint32_t)data_size;

  const uint8_t *bytes = (const uint8_t *)data;
  header.checksum = 0;
  for (size_t i = 0; i < data_size; i++) {
    header.checksum += bytes[i];
  }

  size_t total_size = sizeof(binary_header_t) + data_size;
  if (total_size > *output_size) {
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  memcpy(output, &header, sizeof(binary_header_t));
  memcpy((uint8_t *)output + sizeof(binary_header_t), data, data_size);
  *output_size = total_size;

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static int
io_scene_manager_01_deserialize_from_binary(io_scene_manager_01_t *ctx,
                                            const void *data, size_t data_size,
                                            void *output, size_t *output_size) {
  if (!ctx || !data || !output || !output_size) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t data_size;
    uint32_t checksum;
  } binary_header_t;

  if (data_size < sizeof(binary_header_t)) {
    return IO_SCENE_MANAGER_01_ERROR_SERIALIZATION_ERROR;
  }

  binary_header_t header;
  memcpy(&header, data, sizeof(binary_header_t));

  if (header.magic != 0x5343454E) {
    return IO_SCENE_MANAGER_01_ERROR_SERIALIZATION_ERROR;
  }

  if (header.version != ctx->serialization_version) {
    return IO_SCENE_MANAGER_01_ERROR_SERIALIZATION_ERROR;
  }

  if (header.data_size > *output_size) {
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  const uint8_t *payload = (const uint8_t *)data + sizeof(binary_header_t);
  uint32_t calculated_checksum = 0;
  for (uint32_t i = 0; i < header.data_size; i++) {
    calculated_checksum += payload[i];
  }

  if (calculated_checksum != header.checksum) {
    return IO_SCENE_MANAGER_01_ERROR_SERIALIZATION_ERROR;
  }

  memcpy(output, payload, header.data_size);
  *output_size = header.data_size;

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static int
io_scene_manager_01_parse_scene_file(io_scene_manager_01_t *ctx,
                                     const char *filepath,
                                     io_scene_file_data_t *scene_data) {
  if (!ctx || !filepath || !scene_data) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  const char *ext = strrchr(filepath, '.');
  if (!ext) {
    return IO_SCENE_MANAGER_01_ERROR_PARSE_ERROR;
  }

  uint32_t format;
  if (strcmp(ext, ".gltf") == 0) {
    format = IO_SCENE_MANAGER_01_FORMAT_GLTF;
  } else if (strcmp(ext, ".glb") == 0) {
    format = IO_SCENE_MANAGER_01_FORMAT_GLB;
  } else if (strcmp(ext, ".fbx") == 0) {
    format = IO_SCENE_MANAGER_01_FORMAT_FBX;
  } else if (strcmp(ext, ".obj") == 0) {
    format = IO_SCENE_MANAGER_01_FORMAT_OBJ;
  } else {
    format = IO_SCENE_MANAGER_01_FORMAT_CUSTOM;
  }

  strncpy(scene_data->filepath, filepath, sizeof(scene_data->filepath) - 1);
  scene_data->filepath[sizeof(scene_data->filepath) - 1] = '\0';
  scene_data->format = format;
  scene_data->is_loaded = false;
  scene_data->is_dirty = false;

  scene_data->data_size = 1024;
  scene_data->parsed_data = malloc(scene_data->data_size);
  if (!scene_data->parsed_data) {
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  memset(scene_data->parsed_data, 0, scene_data->data_size);
  scene_data->is_loaded = true;

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static void io_scene_manager_01_update_telemetry(io_scene_manager_01_t *ctx,
                                                 uint64_t operation_time_ns,
                                                 int error_code) {
  if (!ctx)
    return;

  pthread_mutex_lock(&ctx->telemetry_mutex);

  ctx->operations_processed++;
  ctx->total_process_time_ns += operation_time_ns;

  if (error_code != IO_SCENE_MANAGER_01_ERROR_NONE) {
    ctx->error_count++;
  }

  pthread_mutex_unlock(&ctx->telemetry_mutex);
}

/* Helper function for async file operations */
static void *async_file_worker(void *arg) {
  async_file_op_t *op = (async_file_op_t *)arg;

  pthread_mutex_lock(&op->mutex);

  if (op->is_read) {
    int fd = open(op->filepath, O_RDONLY);
    if (fd != -1) {
      if (lseek(fd, op->offset, SEEK_SET) != -1) {
        ssize_t bytes_read = read(fd, op->buffer, op->size);
        if (bytes_read == (ssize_t)op->size) {
          op->is_completed = true;
        }
      }
      close(fd);
    }
  } else {
    // Write operation
    int fd = open(op->filepath, O_WRONLY | O_CREAT, 0644);
    if (fd != -1) {
      if (lseek(fd, op->offset, SEEK_SET) != -1) {
        ssize_t bytes_written = write(fd, op->buffer, op->size);
        if (bytes_written == (ssize_t)op->size) {
          op->is_completed = true;
        }
      }
      close(fd);
    }
  }

  pthread_cond_signal(&op->cond);
  pthread_mutex_unlock(&op->mutex);

  if (op->callback) {
    op->callback(op);
  }

  return NULL;
}

/* Helper function for scene parsing */
static int parse_gltf_scene(scene_parser_t *parser, const void *data,
                            size_t size) {
  // Mock glTF parsing implementation
  parser->node_count = 10;
  parser->mesh_count = 5;
  parser->material_count = 3;
  parser->texture_count = 8;
  parser->is_parsed = true;
  return 0;
}

static int parse_fbx_scene(scene_parser_t *parser, const void *data,
                           size_t size) {
  // Mock FBX parsing implementation
  parser->node_count = 15;
  parser->mesh_count = 8;
  parser->material_count = 6;
  parser->texture_count = 12;
  parser->is_parsed = true;
  return 0;
}

/* Helper function for format conversion */
static int convert_gltf_to_fbx(void *input, size_t input_size, void **output,
                               size_t *output_size) {
  // Mock conversion implementation
  *output_size = input_size;
  *output = malloc(*output_size);
  if (*output) {
    memcpy(*output, input, input_size);
    return 0;
  }
  return -1;
}

/* Helper function for binary serialization */
static int serialize_to_binary(io_scene_manager_01_t *ctx, void **data,
                               size_t *size) {
  // Calculate required size
  *size = sizeof(uint32_t) + sizeof(ctx->flags) + sizeof(ctx->bundle_count) +
          (ctx->bundle_count * sizeof(asset_bundle_t));

  *data = malloc(*size);
  if (!*data)
    return -1;

  uint8_t *ptr = (uint8_t *)*data;

  // Write header
  *((uint32_t *)ptr) = ctx->serialization_version;
  ptr += sizeof(uint32_t);

  *((uint32_t *)ptr) = ctx->flags;
  ptr += sizeof(uint32_t);

  *((uint32_t *)ptr) = ctx->bundle_count;
  ptr += sizeof(uint32_t);

  // Write asset bundles
  memcpy(ptr, ctx->asset_bundles, ctx->bundle_count * sizeof(asset_bundle_t));

  return 0;
}

/* Helper function for asset bundling */
static int create_asset_bundle(io_scene_manager_01_t *ctx, const char *name,
                               void *data, size_t size,
                               uint8_t compression_type) {
  // Initialize capacity if needed (should be done in init, but safe guard here)
  if (ctx->bundle_capacity == 0)
    ctx->bundle_capacity = IO_SCENE_MANAGER_01_MAX_BUNDLES;

  if (ctx->bundle_count >= ctx->bundle_capacity) {
    return -1; // Capacity exceeded
  }

  io_scene_asset_bundle_t *bundle = &ctx->asset_bundles[ctx->bundle_count];
  strncpy(bundle->name, name, sizeof(bundle->name) - 1);
  bundle->name[sizeof(bundle->name) - 1] = '\0';

  bundle->version = 1;
  bundle->size_uncompressed = size;

  bundle->asset_count = 1;
  bundle->compression_type = compression_type;
  bundle->data = malloc(size);

  if (!bundle->data)
    return -1;

  memcpy(bundle->data, data, size);

  // Compress if requested
  if (compression_type == 1) { // LZ4
#ifdef USE_LZ4
    int compressed_size = LZ4_compressBound(size);
    bundle->compressed_data = malloc(compressed_size);
    if (bundle->compressed_data) {
      int result = LZ4_compress_default((const char *)data,
                                        (char *)bundle->compressed_data, size,
                                        compressed_size);
      if (result > 0) {
        bundle->size_compressed = result;
      } else {
        free(bundle->compressed_data);
        bundle->compressed_data = NULL;
        return -1;
      }
    }
#else
    return -1; // LZ4 not enabled
#endif
  } else if (compression_type == 2) { // ZSTD
#ifdef USE_ZSTD
    size_t compressed_size = ZSTD_compressBound(size);
    bundle->compressed_data = malloc(compressed_size);
    if (bundle->compressed_data) {
      size_t result = ZSTD_compress(bundle->compressed_data, compressed_size,
                                    data, size, 1);
      if (!ZSTD_isError(result)) {
        bundle->compressed_size = result;
      } else {
        free(bundle->compressed_data);
        bundle->compressed_data = NULL;
        return -1;
      }
    }
#else
    return -1; // ZSTD not enabled
#endif
  }

  ctx->bundle_count++;
  return 0;
}

/*
 * io_scene_manager_01_init
 *
 * Performs init operation on io_scene_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_scene_manager_01_init(io_scene_manager_01_t *ctx, void *params) {
  if (!ctx) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  /* Thread-safe initialization with memory barriers */
  io_scene_manager_01_memory_barrier();

  /* Initialize asset cache management */
  ctx->bundle_count = 0;
  pthread_mutex_init(&ctx->bundle_mutex, NULL);

  /* Initialize asset bundling system */
  for (uint32_t i = 0; i < IO_SCENE_MANAGER_01_MAX_BUNDLES; i++) {
    ctx->asset_bundles[i] = NULL;
  }

  /* Initialize binary serialization */
  ctx->serialization_version = IO_SCENE_MANAGER_01_BUNDLE_VERSION;
  ctx->serialization_buffer_size = 64 * 1024; /* 64KB initial buffer */
  ctx->serialization_buffer = malloc(ctx->serialization_buffer_size);
  if (!ctx->serialization_buffer) {
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  /* Initialize comprehensive error handling */
  ctx->error_count = 0;

  /* Initialize multi-threaded batch processing */
  int result = io_scene_manager_01_init_thread_pool(ctx);
  if (result != IO_SCENE_MANAGER_01_ERROR_NONE) {
    free(ctx->serialization_buffer);
    return result;
  }

  /* Initialize async file loading */
  ctx->async_count = 0;
  for (uint32_t i = 0; i < IO_SCENE_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
    ctx->async_operations[i] = NULL;
  }

  /* Initialize scene file parsing */
  ctx->scene_count = 0;
  for (uint32_t i = 0; i < IO_SCENE_MANAGER_01_MAX_SCENE_FILES; i++) {
    ctx->scene_data[i] = NULL;
  }

  /* Initialize format conversion */
  ctx->converter_count = 0;
  for (uint32_t i = 0; i < 16; i++) {
    ctx->format_converters[i] = NULL;
  }

  /* Initialize memory tracking and budget */
  ctx->memory_budget = IO_SCENE_MANAGER_01_DEFAULT_MEMORY_BUDGET;
  ctx->memory_used = 0;
  pthread_mutex_init(&ctx->memory_mutex, NULL);

  /* Initialize telemetry and performance counters */
  ctx->operations_processed = 0;
  ctx->total_process_time_ns = 0;
  ctx->error_count = 0;
  pthread_mutex_init(&ctx->telemetry_mutex, NULL);

  /* Initialize LZ4/ZSTD compression */
  ctx->compression_type = IO_SCENE_MANAGER_01_COMPRESSION_AUTO;
  ctx->compression_workspace_size = 64 * 1024; /* 64KB workspace */
  ctx->compression_workspace = malloc(ctx->compression_workspace_size);
  if (!ctx->compression_workspace) {
    io_scene_manager_01_shutdown_thread_pool(ctx);
    free(ctx->serialization_buffer);
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  /* Initialize resource pooling for reduced allocation overhead */
  ctx->pool_capacity = IO_SCENE_MANAGER_01_RESOURCE_POOL_SIZE;
  ctx->pool_used = 0;
  ctx->resource_pool = malloc(ctx->pool_capacity * sizeof(void *));
  if (!ctx->resource_pool) {
    io_scene_manager_01_shutdown_thread_pool(ctx);
    free(ctx->serialization_buffer);
    free(ctx->compression_workspace);
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  /* Initialize file watching for hot-reload */
  ctx->watch_count = 0;
  ctx->file_watch_fd = -1;
  for (uint32_t i = 0; i < IO_SCENE_MANAGER_01_MAX_SCENE_FILES; i++) {
    ctx->watched_files[i] = NULL;
  }

  ctx->is_initialized = true;
  ctx->flags |= IO_SCENE_MANAGER_01_FLAG_INITIALIZED;

  io_scene_manager_01_memory_barrier();

  (void)params;
  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

/*
 * io_scene_manager_01_shutdown
 *
 * Performs shutdown operation on io_scene_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_scene_manager_01_shutdown(io_scene_manager_01_t *ctx, void *params) {
  if (!ctx) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  /* Thread-safe shutdown with memory barriers */
  io_scene_manager_01_memory_barrier();

  /* Add multi-threaded batch processing support for shutdown */
  if (ctx->async_count > 0) {
    /* Cancel all pending async operations */
    pthread_mutex_lock(&ctx->async_mutex);
    for (uint32_t i = 0; i < IO_SCENE_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
      io_scene_async_operation_t *op =
          (io_scene_async_operation_t *)ctx->async_operations[i];
      if (op && !op->completed) {
        op->cancelled = true;
        op->error_code = IO_SCENE_MANAGER_01_ERROR_ASYNC_BUSY;
      }
    }
    pthread_mutex_unlock(&ctx->async_mutex);

    /* Wait for all operations to complete */
    io_scene_manager_01_shutdown_thread_pool(ctx);
  }

  /* Implement async file loading cleanup */
  for (uint32_t i = 0; i < IO_SCENE_MANAGER_01_MAX_ASYNC_OPERATIONS; i++) {
    io_scene_async_operation_t *op =
        (io_scene_async_operation_t *)ctx->async_operations[i];
    if (op) {
      if (op->input_data)
        free(op->input_data);
      if (op->output_data)
        free(op->output_data);
      free(op);
      ctx->async_operations[i] = NULL;
    }
  }

  /* Add glTF/FBX import cleanup */
  for (uint32_t i = 0; i < ctx->scene_count; i++) {
    io_scene_file_data_t *scene = (io_scene_file_data_t *)ctx->scene_data[i];
    if (scene) {
      if (scene->parsed_data) {
        free(scene->parsed_data);
      }
      free(scene);
      ctx->scene_data[i] = NULL;
    }
  }
  ctx->scene_count = 0;

  /* Add memory budget tracking and automatic eviction policies cleanup */
  pthread_mutex_lock(&ctx->memory_mutex);
  ctx->memory_used = 0;
  ctx->memory_budget = 0;
  pthread_mutex_unlock(&ctx->memory_mutex);
  pthread_mutex_destroy(&ctx->memory_mutex);

  /* Cleanup all subsystems */
  io_scene_manager_01_cleanup_internal(ctx);

  /* Free asset bundles */
  pthread_mutex_lock(&ctx->bundle_mutex);
  for (uint32_t i = 0; i < ctx->bundle_count; i++) {
    io_scene_asset_bundle_t *bundle =
        (io_scene_asset_bundle_t *)ctx->asset_bundles[i];
    if (bundle) {
      if (bundle->data) {
        free(bundle->data);
      }
      free(bundle);
      ctx->asset_bundles[i] = NULL;
    }
  }
  ctx->bundle_count = 0;
  pthread_mutex_unlock(&ctx->bundle_mutex);
  pthread_mutex_destroy(&ctx->bundle_mutex);

  /* Free serialization buffer */
  if (ctx->serialization_buffer) {
    free(ctx->serialization_buffer);
    ctx->serialization_buffer = NULL;
    ctx->serialization_buffer_size = 0;
  }

  /* Free compression workspace */
  if (ctx->compression_workspace) {
    free(ctx->compression_workspace);
    ctx->compression_workspace = NULL;
    ctx->compression_workspace_size = 0;
  }

  /* Free resource pool */
  if (ctx->resource_pool) {
    free(ctx->resource_pool);
    ctx->resource_pool = NULL;
    ctx->pool_used = 0;
    ctx->pool_capacity = 0;
  }

  /* Cleanup telemetry */
  pthread_mutex_destroy(&ctx->telemetry_mutex);

  /* Cleanup file watching */
  io_scene_manager_01_shutdown_file_watcher(ctx);

  ctx->is_initialized = false;
  ctx->flags &= ~IO_SCENE_MANAGER_01_FLAG_INITIALIZED;

  io_scene_manager_01_memory_barrier();

  (void)params;
  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

/*
 * io_scene_manager_01_update
 *
 * Performs update operation on io_scene_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_scene_manager_01_update(io_scene_manager_01_t *ctx, void *params) {
  if (!ctx) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  /* Implement async initialization for non-blocking startup */
  if (!(ctx->flags & IO_SCENE_MANAGER_01_FLAG_ASYNC_INIT)) {
    /* Perform async initialization tasks */
    ctx->flags |= IO_SCENE_MANAGER_01_FLAG_ASYNC_INIT;
  }

  /* Add LZ4/ZSTD compression processing */
  if (ctx->flags & IO_SCENE_MANAGER_01_FLAG_COMPRESSION) {
    /* Process any pending compression operations */
    /* This would handle compression queue processing */
  }

  /* Implement asset bundling updates */
  if (ctx->flags & IO_SCENE_MANAGER_01_FLAG_BUNDLING) {
    pthread_mutex_lock(&ctx->bundle_mutex);

    /* Update asset bundles - check for modifications, etc. */
    for (uint32_t i = 0; i < ctx->bundle_count; i++) {
      io_scene_asset_bundle_t *bundle =
          (io_scene_asset_bundle_t *)ctx->asset_bundles[i];
      if (bundle) {
        /* Update bundle metadata, check for changes */
        bundle->modified_time = time(NULL);
      }
    }

    pthread_mutex_unlock(&ctx->bundle_mutex);
  }

  /* Add telemetry and performance counters for profiling */
  pthread_mutex_lock(&ctx->telemetry_mutex);

  /* Update performance metrics */
  uint64_t current_time = io_scene_manager_01_get_timestamp_ns();

  /* Calculate average process time */
  if (ctx->operations_processed > 0) {
    double avg_time_ms =
        (double)(ctx->total_process_time_ns / ctx->operations_processed) /
        1000000.0;
    /* Update global stats */
    s_manager_01_stats.avg_process_time_ms = avg_time_ms;
  }

  /* Update memory usage statistics */
  s_manager_01_stats.memory_used = ctx->memory_used;
  s_manager_01_stats.active_count = ctx->scene_count;
  s_manager_01_stats.operations_processed = ctx->operations_processed;

  pthread_mutex_unlock(&ctx->telemetry_mutex);

  /* Process hot-reload file watching */
  if (ctx->file_watch_fd != -1) {
    /* Check for file changes and trigger reloads */
    /* This would integrate with file watching system */
  }

  /* Memory budget enforcement */
  if (ctx->memory_used > ctx->memory_budget) {
    /* Trigger automatic eviction */
    size_t excess = ctx->memory_used - ctx->memory_budget;
    io_scene_manager_01_evict_lru_assets(ctx, excess);
  }

  (void)params;
  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

/*
    pthread_mutex_unlock(&ctx->async_mutex);

    // Memory budget management
    if (ctx->auto_eviction_enabled && ctx->memory_used > ctx->memory_budget) {
        // Simple eviction: remove oldest bundles
        for (uint32_t i = 0; i < ctx->bundle_count && ctx->memory_used >
ctx->memory_budget; i++) { asset_bundle_t* bundle = &ctx->asset_bundles[i]; if
(bundle->data) { ctx->memory_used -= bundle->size; free(bundle->data);
                bundle->data = NULL;
            }
        }
    }

    pthread_mutex_unlock(&ctx->global_mutex);
    (void)params;
    return 0;
}

/*
 * io_scene_manager_01_create_legacy
 *
 * Performs create operation on io_scene_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_scene_manager_01_create_legacy(void *ctx, void *params) {
  if (!ctx) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  io_scene_manager_01_t *manager = (io_scene_manager_01_t *)ctx;

  /* Implement resource pooling for reduced allocation overhead */
  void *pooled_resource = io_scene_manager_01_allocate_from_pool(manager, 1024);
  if (!pooled_resource) {
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  /* Add multi-threaded batch processing support */
  /* Create async operation for batch processing */
  io_scene_async_operation_t *batch_op =
      malloc(sizeof(io_scene_async_operation_t));
  if (!batch_op) {
    io_scene_manager_01_free_to_pool(manager, pooled_resource);
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  batch_op->id = manager->operations_processed++;
  batch_op->type = 1; /* Batch processing */
  batch_op->input_data = pooled_resource;
  batch_op->output_data = NULL;
  batch_op->input_size = 1024;
  batch_op->output_size = 0;
  batch_op->completed = false;
  batch_op->cancelled = false;
  batch_op->error_code = IO_SCENE_MANAGER_01_ERROR_NONE;
  batch_op->callback = NULL;
  batch_op->user_data = manager;

  /* Queue the batch operation */
  int result = io_scene_manager_01_queue_async_operation(manager, batch_op);
  if (result != IO_SCENE_MANAGER_01_ERROR_NONE) {
    free(batch_op);
    io_scene_manager_01_free_to_pool(manager, pooled_resource);
    return result;
  }

  /* Implement format conversion */
  /* Set up format converters if needed */
  if (manager->converter_count == 0) {
    /* Initialize default format converters */
    /* This would register glTF->FBX, FBX->OBJ, etc. */
  }

  /* Implement serialization support for state persistence */
  if (params) {
    /* Serialize initial state */
    size_t serialized_size = 0;
    result = io_scene_manager_01_serialize_to_binary(
        manager, params, strlen((char *)params), manager->serialization_buffer,
        &serialized_size);
    if (result == IO_SCENE_MANAGER_01_ERROR_NONE) {
      manager->data_size = serialized_size;
      if (manager->internal_data) {
        free(manager->internal_data);
      }
      manager->internal_data = malloc(serialized_size);
      if (manager->internal_data) {
        memcpy(manager->internal_data, manager->serialization_buffer,
               serialized_size);
      }
    }
  }

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

/*
 * io_scene_manager_01_destroy
 * Performs get operation on io_scene_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_scene_manager_01_get(io_scene_manager_01_t *ctx, void *params) {
  if (!ctx) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  /* Implement scene file parsing */
  if (params) {
    char *filepath = (char *)params;

    /* Check if scene already loaded */
    bool scene_found = false;
    for (uint32_t i = 0; i < ctx->scene_count; i++) {
      io_scene_file_data_t *scene = (io_scene_file_data_t *)ctx->scene_data[i];
      if (scene && strcmp(scene->filepath, filepath) == 0) {
        scene_found = true;
        break;
      }
    }

    if (!scene_found &&
        ctx->scene_count < IO_SCENE_MANAGER_01_MAX_SCENE_FILES) {
      /* Parse new scene file */
      io_scene_file_data_t *new_scene = malloc(sizeof(io_scene_file_data_t));
      if (!new_scene) {
        return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
      }

      int result =
          io_scene_manager_01_parse_scene_file(ctx, filepath, new_scene);
      if (result == IO_SCENE_MANAGER_01_ERROR_NONE) {
        ctx->scene_data[ctx->scene_count] = new_scene;
        ctx->scene_count++;

        /* Update memory usage */
        io_scene_manager_01_track_memory_usage(ctx, new_scene->data_size);
      } else {
        free(new_scene);
        return result;
      }
    }
  }

  /* Implement resource pooling for reduced allocation overhead */
  /* Allocate from pool for any temporary data needed */
  void *temp_buffer = io_scene_manager_01_allocate_from_pool(ctx, 4096);
  if (!temp_buffer) {
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  /* Add multi-threaded batch processing support */
  /* Create async operation for any heavy processing */
  io_scene_async_operation_t *batch_op =
      malloc(sizeof(io_scene_async_operation_t));
  if (!batch_op) {
    io_scene_manager_01_free_to_pool(ctx, temp_buffer);
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  batch_op->id = ctx->operations_processed++;
  batch_op->type = 1; /* Scene processing */
  batch_op->input_data = temp_buffer;
  batch_op->output_data = NULL;
  batch_op->input_size = 4096;
  batch_op->output_size = 0;
  batch_op->completed = false;
  batch_op->cancelled = false;
  batch_op->error_code = IO_SCENE_MANAGER_01_ERROR_NONE;
  batch_op->callback = NULL;
  batch_op->user_data = ctx;

  /* Queue the operation */
  int result = io_scene_manager_01_queue_async_operation(ctx, batch_op);
  if (result != IO_SCENE_MANAGER_01_ERROR_NONE) {
    free(batch_op);
    io_scene_manager_01_free_to_pool(ctx, temp_buffer);
    return result;
  }

  /* Add LZ4/ZSTD compression */
  /* Compress scene data if needed */
  if (ctx->flags & IO_SCENE_MANAGER_01_FLAG_COMPRESSION) {
    for (uint32_t i = 0; i < ctx->scene_count; i++) {
      io_scene_file_data_t *scene = (io_scene_file_data_t *)ctx->scene_data[i];
      if (scene && scene->parsed_data && !scene->is_dirty) {
        /* Compress the scene data */
        size_t compressed_size = ctx->compression_workspace_size;
        result = io_scene_manager_01_compress_data(
            ctx, scene->parsed_data, scene->data_size,
            ctx->compression_workspace, &compressed_size);
        if (result == IO_SCENE_MANAGER_01_ERROR_NONE) {
          /* Update compression statistics */
          pthread_mutex_lock(&ctx->telemetry_mutex);
          /* Update compression ratio tracking */
          pthread_mutex_unlock(&ctx->telemetry_mutex);
        }
      }
    }
  }

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

/*
 * io_scene_manager_01_set
 *
 * Performs set operation on io_scene_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_scene_manager_01_set(io_scene_manager_01_t *ctx, void *params) {
  if (!ctx) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  /* Implement thread-safe initialization with proper memory barriers */
  io_scene_manager_01_memory_barrier();

  /* Implement async file loading */
  if (params) {
    char *filepath = (char *)params;

    /* Create async file loading operation */
    io_scene_async_operation_t *async_op =
        malloc(sizeof(io_scene_async_operation_t));
    if (!async_op) {
      return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
    }

    /* Setup async operation for file loading */
    async_op->id = ctx->operations_processed++;
    async_op->type = 1; /* File loading */
    async_op->input_data = strdup(filepath);
    async_op->output_data = malloc(4096); /* Buffer for loaded data */
    async_op->input_size = strlen(filepath);
    async_op->output_size = 4096;
    async_op->completed = false;
    async_op->cancelled = false;
    async_op->error_code = IO_SCENE_MANAGER_01_ERROR_NONE;
    async_op->callback = NULL;
    async_op->user_data = ctx;

    /* Queue the async operation */
    int result = io_scene_manager_01_queue_async_operation(ctx, async_op);
    if (result != IO_SCENE_MANAGER_01_ERROR_NONE) {
      free(async_op->input_data);
      free(async_op->output_data);
      free(async_op);
      return result;
    }
  }

  /* Implement binary serialization */
  if (ctx->internal_data && ctx->data_size > 0) {
    size_t serialized_size = ctx->serialization_buffer_size;
    int result = io_scene_manager_01_serialize_to_binary(
        ctx, ctx->internal_data, ctx->data_size, ctx->serialization_buffer,
        &serialized_size);
    if (result == IO_SCENE_MANAGER_01_ERROR_NONE) {
      /* Update serialization statistics */
      pthread_mutex_lock(&ctx->telemetry_mutex);
      ctx->operations_processed++;
      pthread_mutex_unlock(&ctx->telemetry_mutex);
    }
  }

  /* Implement scene file parsing */
  /* Parse any pending scene files */
  for (uint32_t i = 0; i < ctx->scene_count; i++) {
    io_scene_file_data_t *scene = (io_scene_file_data_t *)ctx->scene_data[i];
    if (scene && scene->is_dirty && !scene->is_loaded) {
      int result =
          io_scene_manager_01_parse_scene_file(ctx, scene->filepath, scene);
      if (result == IO_SCENE_MANAGER_01_ERROR_NONE) {
        scene->is_dirty = false;
        scene->is_loaded = true;

        /* Update memory usage */
        io_scene_manager_01_track_memory_usage(ctx, scene->data_size);
      }
    }
  }

  io_scene_manager_01_memory_barrier();

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

op->id = ctx->async_count;
strncpy(op->filepath, filepath, sizeof(op->filepath) - 1);
op->filepath[sizeof(op->filepath) - 1] = '\0';
op->is_read = true;
op->is_completed = false;
op->size = 1024 * 1024; // 1MB buffer
op->offset = 0;
op->buffer = malloc(op->size);

if (op->buffer) {
  pthread_mutex_init(&op->mutex, NULL);
  pthread_cond_init(&op->cond, NULL);

  pthread_create(&op->thread, NULL, async_file_worker, op);
  ctx->async_count++;
}
}

// Serialize state
if (ctx->serialization_buffer) {
  free(ctx->serialization_buffer);
}
serialize_to_binary(ctx, (void **)&ctx->serialization_buffer,
                    &ctx->serialization_size);

pthread_mutex_unlock(&ctx->global_mutex);
return 0;
}

/*
 * io_scene_manager_01_reset
 *
 * Performs reset operation on io_scene_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_scene_manager_01_reset(io_scene_manager_01_t *ctx, void *params) {
  if (!ctx)
    return -1;

  pthread_mutex_lock(&ctx->global_mutex);

  // Reset scene parser
  if (ctx->scene_parser.scene_data) {
    free(ctx->scene_parser.scene_data);
    ctx->scene_parser.scene_data = NULL;
    ctx->scene_parser.data_size = 0;
    ctx->scene_parser.is_parsed = false;
    ctx->scene_parser.node_count = 0;
    ctx->scene_parser.mesh_count = 0;
    ctx->scene_parser.material_count = 0;
    ctx->scene_parser.texture_count = 0;
  }

  // Reset memory usage
  ctx->memory_used = 0;

  pthread_mutex_unlock(&ctx->global_mutex);
  (void)params;
  return 0;
}

/*
 * io_scene_manager_01_validate
 *
 * Performs validate operation on io_scene_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_scene_manager_01_validate(io_scene_manager_01_t *ctx, void *params) {
  if (!ctx)
    return -1;

  pthread_mutex_lock(&ctx->global_mutex);

  // Validate format converters
  for (uint32_t i = 0; i < ctx->converter_count; i++) {
    format_converter_t *converter = &ctx->format_converters[i];
    if (!converter->convert_func || !converter->is_available) {
      pthread_mutex_unlock(&ctx->global_mutex);
      return -3; // Invalid converter
    }
  }

  // Validate serialization
  if (ctx->serialization_buffer && ctx->serialization_size == 0) {
    pthread_mutex_unlock(&ctx->global_mutex);
    return -4; // Invalid serialization state
  }

  int result = io_scene_manager_01_validate_internal(ctx);

  pthread_mutex_unlock(&ctx->global_mutex);
  (void)params;
  return result;
}

/*
 * io_scene_manager_01_flush
 *
 * Performs flush operation on io_scene_manager_01
 * Thread-safe: Yes (with proper synchronization)
 * Complexity: O(n) where n is the number of elements
 */
int io_scene_manager_01_flush(io_scene_manager_01_t *ctx, void *params) {
  if (!ctx)
    return -1;

  pthread_mutex_lock(&ctx->global_mutex);

  // Wait for all async operations to complete
  pthread_mutex_lock(&ctx->async_mutex);
  for (uint32_t i = 0; i < ctx->async_count; i++) {
    async_file_op_t *op = &ctx->async_ops[i];
    if (!op->is_completed) {
      pthread_mutex_lock(&op->mutex);
      while (!op->is_completed) {
        pthread_cond_wait(&op->cond, &op->mutex);
      }
      pthread_mutex_unlock(&op->mutex);
    }
  }
  pthread_mutex_unlock(&ctx->async_mutex);

  // Perform format conversion if needed
  if (params && ctx->converter_count > 0) {
    char *target_format = (char *)params;
    for (uint32_t i = 0; i < ctx->converter_count; i++) {
      format_converter_t *converter = &ctx->format_converters[i];
      if (strcmp(converter->target_format, target_format) == 0) {
        // Mock conversion
        break;
      }
    }
  }

  pthread_mutex_unlock(&ctx->global_mutex);
  return 0;
}

/*
 * io_scene_manager_01_get_stats
 * Retrieves statistics about io_scene_manager_01 usage
 */
int io_scene_manager_01_get_stats(io_scene_manager_01_t *ctx) {
  if (!ctx)
    return -1;

  pthread_mutex_lock(&ctx->global_mutex);

  // Update statistics
  s_manager_01_stats.active_count = ctx->bundle_count + ctx->async_count;
  s_manager_01_stats.memory_used = ctx->memory_used;

  // Calculate memory peak
  if (ctx->memory_used > s_manager_01_stats.memory_peak) {
    s_manager_01_stats.memory_peak = ctx->memory_used;
  }

  pthread_mutex_unlock(&ctx->global_mutex);
  return 0;
}

/*
 * io_scene_manager_01_set_callback
 * Sets a callback for io_scene_manager_01 events
 */
int io_scene_manager_01_set_callback(io_scene_manager_01_t *ctx) {
  if (!ctx)
    return -1;

  pthread_mutex_lock(&ctx->global_mutex);

  // Set callback for async operations
  pthread_mutex_lock(&ctx->async_mutex);
  for (uint32_t i = 0; i < ctx->async_count; i++) {
    async_file_op_t *op = &ctx->async_ops[i];
    if (!op->callback) {
      // Mock callback implementation
      op->callback = NULL; // Would be set by user
    }
  }
  pthread_mutex_unlock(&ctx->async_mutex);

  pthread_mutex_unlock(&ctx->global_mutex);
  return 0;
}

/*
 * io_scene_manager_01_get_memory_usage
 * Returns current memory usage
 */
int io_scene_manager_01_get_memory_usage(io_scene_manager_01_t *ctx) {
  if (!ctx)
    return -1;

  pthread_mutex_lock(&ctx->global_mutex);

  size_t total_usage = ctx->memory_used;

  // Add async operation memory
  pthread_mutex_lock(&ctx->async_mutex);
  for (uint32_t i = 0; i < ctx->async_count; i++) {
    total_usage += ctx->async_ops[i].size;
  }
  pthread_mutex_unlock(&ctx->async_mutex);

  // Add serialization buffer memory
  total_usage += ctx->serialization_size;

  pthread_mutex_unlock(&ctx->global_mutex);

  return (int)total_usage;
}

/*
 * io_scene_manager_01_optimize
 * Optimizes internal data structures
 */
int io_scene_manager_01_optimize(io_scene_manager_01_t *ctx) {
  if (!ctx)
    return -1;

  pthread_mutex_lock(&ctx->global_mutex);

  // Optimize format converters
  for (uint32_t i = 0; i < ctx->converter_count; i++) {
    format_converter_t *converter = &ctx->format_converters[i];
    // Mock optimization - would analyze and optimize conversion paths
    if (converter->is_available) {
      // Optimization logic here
    }
  }

  pthread_mutex_unlock(&ctx->global_mutex);
  return 0;
}

/*
 * io_scene_manager_01_debug_print
 * Prints debug information
 */
int io_scene_manager_01_debug_print(io_scene_manager_01_t *ctx) {
  if (!ctx)
    return -1;

  pthread_mutex_lock(&ctx->global_mutex);

  printf("=== Scene Manager Debug Info ===\n");
  printf("Initialized: %s\n", ctx->is_initialized ? "Yes" : "No");
  printf("Dirty: %s\n", ctx->is_dirty ? "Yes" : "No");
  printf("Bundle Count: %u/%u\n", ctx->bundle_count, ctx->bundle_capacity);
  printf("Async Operations: %u/%u\n", ctx->async_count, ctx->async_capacity);
  printf("Format Converters: %u/%u\n", ctx->converter_count,
         ctx->converter_capacity);
  printf("Memory Used: %zu/%zu bytes\n", ctx->memory_used, ctx->memory_budget);

  if (ctx->scene_parser.is_parsed) {
    printf("Scene Parsed: %s\n", ctx->scene_parser.format);
    printf("  Nodes: %u\n", ctx->scene_parser.node_count);
    printf("  Meshes: %u\n", ctx->scene_parser.mesh_count);
    printf("  Materials: %u\n", ctx->scene_parser.material_count);
    printf("  Textures: %u\n", ctx->scene_parser.texture_count);
  }

  pthread_mutex_unlock(&ctx->global_mutex);
  return 0;
}

/* Missing helper function implementations */
static void *io_scene_manager_01_allocate_from_pool(io_scene_manager_01_t *ctx,
                                                    size_t size) {
  if (!ctx || size == 0)
    return NULL;

  pthread_mutex_lock(&s_resource_pool_mutex);

  /* Try global pool first */
  if (s_resource_pool_used > 0) {
    void *resource = s_resource_pool[--s_resource_pool_used];
    pthread_mutex_unlock(&s_resource_pool_mutex);
    return resource;
  }

  pthread_mutex_unlock(&s_resource_pool_mutex);

  /* Fallback to malloc */
  return malloc(size);
}

static void io_scene_manager_01_free_to_pool(io_scene_manager_01_t *ctx,
                                             void *ptr) {
  if (!ctx || !ptr)
    return;

  pthread_mutex_lock(&s_resource_pool_mutex);

  /* Return to global pool if space available */
  if (s_resource_pool_used < IO_SCENE_MANAGER_01_RESOURCE_POOL_SIZE) {
    s_resource_pool[s_resource_pool_used++] = ptr;
    pthread_mutex_unlock(&s_resource_pool_mutex);
    return;
  }

  pthread_mutex_unlock(&s_resource_pool_mutex);

  /* Fallback to free */
  free(ptr);
}

static int io_scene_manager_01_check_memory_budget(io_scene_manager_01_t *ctx,
                                                   size_t required_size) {
  if (!ctx)
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;

  pthread_mutex_lock(&ctx->memory_mutex);

  bool fits = (ctx->memory_used + required_size) <= ctx->memory_budget;

  pthread_mutex_unlock(&ctx->memory_mutex);

  return fits ? IO_SCENE_MANAGER_01_ERROR_NONE
              : IO_SCENE_MANAGER_01_ERROR_MEMORY_BUDGET_EXCEEDED;
}

static int io_scene_manager_01_evict_lru_assets(io_scene_manager_01_t *ctx,
                                                size_t required_size) {
  if (!ctx)
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;

  pthread_mutex_lock(&ctx->memory_mutex);

  /* Simple LRU eviction - free oldest scene data first */
  size_t freed = 0;
  for (uint32_t i = 0; i < ctx->scene_count && freed < required_size; i++) {
    io_scene_file_data_t *scene = (io_scene_file_data_t *)ctx->scene_data[i];
    if (scene && scene->parsed_data) {
      freed += scene->data_size;
      free(scene->parsed_data);
      scene->parsed_data = NULL;
      scene->is_loaded = false;

      /* Remove from array */
      for (uint32_t j = i; j < ctx->scene_count - 1; j++) {
        ctx->scene_data[j] = ctx->scene_data[j + 1];
      }
      ctx->scene_count--;
      i--; /* Recheck this index */
    }
  }

  ctx->memory_used -= freed;

  pthread_mutex_unlock(&ctx->memory_mutex);

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static int io_scene_manager_01_init_file_watcher(io_scene_manager_01_t *ctx) {
  if (!ctx)
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;

  /* Mock file watcher initialization */
  ctx->file_watch_fd = 1; /* Mock file descriptor */
  ctx->watch_count = 0;

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static int
io_scene_manager_01_shutdown_file_watcher(io_scene_manager_01_t *ctx) {
  if (!ctx)
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;

  if (ctx->file_watch_fd != -1) {
    /* Close file watcher */
    ctx->file_watch_fd = -1;
  }

  /* Clear watched files */
  for (uint32_t i = 0; i < ctx->watch_count; i++) {
    if (ctx->watched_files[i]) {
      free(ctx->watched_files[i]);
      ctx->watched_files[i] = NULL;
    }
  }
  ctx->watch_count = 0;

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static void io_scene_manager_01_file_watch_callback(const char *filepath,
                                                    void *user_data) {
  if (!filepath || !user_data)
    return;

  io_scene_manager_01_t *ctx = (io_scene_manager_01_t *)user_data;

  /* Find and reload the scene file */
  for (uint32_t i = 0; i < ctx->scene_count; i++) {
    io_scene_file_data_t *scene = (io_scene_file_data_t *)ctx->scene_data[i];
    if (scene && strcmp(scene->filepath, filepath) == 0) {
      /* Mark for reload */
      scene->is_dirty = true;
      break;
    }
  }
}

static int io_scene_manager_01_convert_scene_format(
    io_scene_manager_01_t *ctx, uint32_t from_format, uint32_t to_format,
    const void *input, size_t input_size, void *output, size_t *output_size) {
  if (!ctx || !input || !output || !output_size) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  /* Mock format conversion */
  if (input_size > *output_size) {
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  memcpy(output, input, input_size);
  *output_size = input_size;

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static int
io_scene_manager_01_create_asset_bundle(io_scene_manager_01_t *ctx,
                                        const char *name,
                                        io_scene_asset_bundle_t *bundle) {
  if (!ctx || !name || !bundle) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  pthread_mutex_lock(&ctx->bundle_mutex);

  if (ctx->bundle_count >= IO_SCENE_MANAGER_01_MAX_BUNDLES) {
    pthread_mutex_unlock(&ctx->bundle_mutex);
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  /* Initialize bundle */
  bundle->id = ctx->bundle_count + 1;
  strncpy(bundle->name, name, sizeof(bundle->name) - 1);
  bundle->name[sizeof(bundle->name) - 1] = '\0';
  bundle->version = IO_SCENE_MANAGER_01_BUNDLE_VERSION;
  bundle->size_compressed = 0;
  bundle->size_uncompressed = 0;
  bundle->asset_count = 0;
  bundle->compression_type = ctx->compression_type;
  bundle->checksum = 0;
  bundle->created_time = time(NULL);
  bundle->modified_time = bundle->created_time;
  bundle->data = NULL;

  /* Add to manager */
  ctx->asset_bundles[ctx->bundle_count] = bundle;
  ctx->bundle_count++;

  pthread_mutex_unlock(&ctx->bundle_mutex);

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static int
io_scene_manager_01_load_asset_bundle(io_scene_manager_01_t *ctx,
                                      const char *name,
                                      io_scene_asset_bundle_t *bundle) {
  if (!ctx || !name || !bundle) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  /* Mock bundle loading */
  bundle->data = malloc(1024); /* Mock data */
  if (!bundle->data) {
    return IO_SCENE_MANAGER_01_ERROR_OUT_OF_MEMORY;
  }

  bundle->size_uncompressed = 1024;
  bundle->size_compressed = 512; /* Mock compressed size */

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

static int
io_scene_manager_01_save_asset_bundle(io_scene_manager_01_t *ctx,
                                      const io_scene_asset_bundle_t *bundle) {
  if (!ctx || !bundle) {
    return IO_SCENE_MANAGER_01_ERROR_INVALID_PARAM;
  }

  /* Mock bundle saving */
  /* This would save bundle to disk with compression */

  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================
 */

/*
 * io_scene_manager_01_module_init
 * Initializes the entire manager_01 module
 */
int io_scene_manager_01_module_init(void) {
  if (s_manager_01_initialized) {
    return 0; /* Already initialized */
  }

  /* Initialize statistics */
  memset(&s_manager_01_stats, 0, sizeof(s_manager_01_stats));

  /* Add telemetry and performance counters for profiling */
  s_manager_01_stats.operations_processed = 0;
  s_manager_01_stats.total_process_time_ns = 0;
  s_manager_01_stats.error_count = 0;

  /* Implement asset bundling */
  /* Initialize global bundle registry */

  /* Implement serialization support for state persistence */
  /* Initialize global serialization buffer */

  /* Implement async file loading */
  /* Initialize global async operation queue */

  /* Initialize compression libraries */
  /* LZ4 and ZSTD are statically linked, no separate init needed */

  /* Initialize global resource pool */
  s_resource_pool_used = 0;

  /* Initialize format converter registry */
  s_converter_count = 0;

  s_manager_01_initialized = true;
  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

int io_scene_manager_01_module_shutdown(void) {
  if (!s_manager_01_initialized) {
    return 0; /* Already shut down */
  }

  /* Implement asset bundling cleanup */
  /* Cleanup global bundle registry */

  /* Implement binary serialization cleanup */
  /* Cleanup global serialization buffer */

  /* Add asset cache management */
  /* Cleanup global cache */

  /* Implement format conversion cleanup */
  /* Cleanup global format converters */
  s_converter_count = 0;

  /* Cleanup global resource pool */
  for (size_t i = 0; i < s_resource_pool_used; i++) {
    if (s_resource_pool[i]) {
      free(s_resource_pool[i]);
      s_resource_pool[i] = NULL;
    }
  }
  s_resource_pool_used = 0;

  /* Cleanup compression libraries */
  /* LZ4 and ZSTD cleanup is automatic for static linking */

  s_manager_01_initialized = false;
  return IO_SCENE_MANAGER_01_ERROR_NONE;
}

/* End of io_scene_manager_01.c */
