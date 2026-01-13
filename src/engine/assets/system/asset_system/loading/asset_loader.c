#include <core/logger.h>
#include <core/memory.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration for ThreadPool
typedef struct ThreadPool ThreadPool;

// ThreadPool function declarations
ThreadPool* thread_pool_create(u32 thread_count);
void thread_pool_destroy(ThreadPool* pool);
u32 thread_pool_submit(ThreadPool* pool, void (*function)(void*), void* user_data, const char* name);
void thread_pool_wait(ThreadPool* pool);

// Asset loader core implementation
#define MAX_ASSET_LOADERS 64
#define MAX_CONCURRENT_LOADS 32
#define ASSET_CACHE_SIZE_MB 1024

typedef enum {
  ASSET_TYPE_UNKNOWN = 0,
  ASSET_TYPE_TEXTURE,
  ASSET_TYPE_MESH,
  ASSET_TYPE_MATERIAL,
  ASSET_TYPE_SHADER,
  ASSET_TYPE_AUDIO,
  ASSET_TYPE_ANIMATION,
  ASSET_TYPE_SCENE,
  ASSET_TYPE_COUNT
} asset_type_t;

typedef enum {
  ASSET_STATE_UNLOADED = 0,
  ASSET_STATE_LOADING,
  ASSET_STATE_LOADED,
  ASSET_STATE_ERROR,
  ASSET_STATE_UNLOADING
} asset_state_t;

typedef struct asset_handle {
  u32 id;
  char path[256];
  asset_type_t type;
  asset_state_t state;
  void *data;
  u32 size_bytes;
  u64 last_modified;
  u32 ref_count;
  pthread_mutex_t mutex;
} asset_handle_t;

typedef struct asset_loader_interface {
  const char *extensions[8];
  asset_type_t type;
  bool (*load)(const char *path, void **out_data, u32 *out_size);
  bool (*unload)(void *data);
  bool (*validate)(const char *path);
  u32 (*get_memory_usage)(const void *data);
} asset_loader_interface_t;

typedef struct asset_cache_entry {
  char path[256];
  void *data;
  u32 size;
  u64 last_access;
  u32 access_count;
  asset_type_t type;
} asset_cache_entry_t;

typedef struct asset_loader_system {
  asset_handle_t assets[MAX_ASSET_LOADERS];
  asset_loader_interface_t loaders[ASSET_TYPE_COUNT];
  asset_cache_entry_t *cache;
  u32 cache_count;
  u32 cache_capacity;
  u32 total_cache_size;
  u32 max_cache_size;

  ThreadPool *thread_pool;
  pthread_mutex_t cache_mutex;
  pthread_mutex_t loader_mutex;

  u32 next_asset_id;
  bool initialized;
} asset_loader_system_t;

static asset_loader_system_t g_asset_system = {0};

static bool asset_loader_unload(u32 asset_id);

// Initialize asset loader system
bool asset_loader_init(u32 max_concurrent_loads, u32 cache_size_mb) {
  if (g_asset_system.initialized) {
    LOG_WARN(LOG_CAT_GENERAL, "Asset loader system already initialized");
    return true;
  }

  memset(&g_asset_system, 0, sizeof(g_asset_system));

  // Initialize thread pool
  g_asset_system.thread_pool = thread_pool_create(max_concurrent_loads);
  if (!g_asset_system.thread_pool) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to create asset loader thread pool");
    return false;
  }

  // Initialize mutexes
  if (pthread_mutex_init(&g_asset_system.cache_mutex, NULL) != 0) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to initialize cache mutex");
    thread_pool_destroy(g_asset_system.thread_pool);
    return false;
  }

  if (pthread_mutex_init(&g_asset_system.loader_mutex, NULL) != 0) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to initialize loader mutex");
    pthread_mutex_destroy(&g_asset_system.cache_mutex);
    thread_pool_destroy(g_asset_system.thread_pool);
    return false;
  }

  // Initialize cache
  g_asset_system.max_cache_size = cache_size_mb * 1024 * 1024;
  g_asset_system.cache_capacity = 1024;
  g_asset_system.cache = (asset_cache_entry_t *)calloc(
      g_asset_system.cache_capacity, sizeof(asset_cache_entry_t));

  if (!g_asset_system.cache) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate asset cache");
    pthread_mutex_destroy(&g_asset_system.cache_mutex);
    pthread_mutex_destroy(&g_asset_system.loader_mutex);
    thread_pool_destroy(g_asset_system.thread_pool);
    return false;
  }

  g_asset_system.next_asset_id = 1;
  g_asset_system.initialized = true;

  LOG_INFO(LOG_CAT_GENERAL,
           "Asset loader system initialized with %u workers, %u MB cache",
           max_concurrent_loads, cache_size_mb);
  return true;
}

// Shutdown asset loader system
void asset_loader_shutdown(void) {
  if (!g_asset_system.initialized) {
    return;
  }

  // Wait for all pending loads
  if (g_asset_system.thread_pool) {
    thread_pool_wait(g_asset_system.thread_pool);
    thread_pool_destroy(g_asset_system.thread_pool);
  }

  // Unload all assets
  for (u32 i = 0; i < MAX_ASSET_LOADERS; i++) {
    if (g_asset_system.assets[i].state == ASSET_STATE_LOADED) {
      asset_loader_unload(g_asset_system.assets[i].id);
    }
    pthread_mutex_destroy(&g_asset_system.assets[i].mutex);
  }

  // Clear cache
  pthread_mutex_lock(&g_asset_system.cache_mutex);
  for (u32 i = 0; i < g_asset_system.cache_count; i++) {
    if (g_asset_system.cache[i].data) {
      free(g_asset_system.cache[i].data);
    }
  }
  free(g_asset_system.cache);
  pthread_mutex_unlock(&g_asset_system.cache_mutex);

  // Destroy mutexes
  pthread_mutex_destroy(&g_asset_system.cache_mutex);
  pthread_mutex_destroy(&g_asset_system.loader_mutex);

  memset(&g_asset_system, 0, sizeof(g_asset_system));

  LOG_INFO(LOG_CAT_GENERAL, "Asset loader system shutdown complete");
}

// Register an asset loader
bool asset_loader_register(asset_type_t type,
                           const asset_loader_interface_t *loader) {
  if (!g_asset_system.initialized || !loader || type >= ASSET_TYPE_COUNT) {
    return false;
  }

  pthread_mutex_lock(&g_asset_system.loader_mutex);

  g_asset_system.loaders[type] = *loader;

  pthread_mutex_unlock(&g_asset_system.loader_mutex);

  LOG_INFO(LOG_CAT_GENERAL, "Registered asset loader for type %u", type);
  return true;
}

// Get file extension
static const char *get_file_extension(const char *path) {
  if (!path)
    return NULL;

  const char *ext = strrchr(path, '.');
  return ext ? ext + 1 : NULL;
}

// Determine asset type from file extension
static asset_type_t determine_asset_type(const char *path) {
  const char *ext = get_file_extension(path);
  if (!ext)
    return ASSET_TYPE_UNKNOWN;

  // Check against registered loaders
  for (asset_type_t type = 1; type < ASSET_TYPE_COUNT; type++) {
    const asset_loader_interface_t *loader = &g_asset_system.loaders[type];

    for (int i = 0; i < 8 && loader->extensions[i]; i++) {
      if (strcasecmp(ext, loader->extensions[i]) == 0) {
        return type;
      }
    }
  }

  return ASSET_TYPE_UNKNOWN;
}

// Find free asset slot
static u32 find_free_asset_slot(void) {
  for (u32 i = 0; i < MAX_ASSET_LOADERS; i++) {
    if (g_asset_system.assets[i].state == ASSET_STATE_UNLOADED) {
      return i;
    }
  }
  return MAX_ASSET_LOADERS;
}

// Load asset task data
typedef struct asset_load_task {
  char path[256];
  asset_type_t type;
  u32 asset_id;
  void (*callback)(u32 asset_id, bool success, void *data);
  void *userdata;
} asset_load_task_t;

// Asset loading worker function
static void asset_load_worker(void *task_data) {
  asset_load_task_t *task = (asset_load_task_t *)task_data;

  if (!task || !g_asset_system.initialized) {
    if (task && task->callback) {
      task->callback(0, false, task->userdata);
    }
    free(task);
    return;
  }

  // Find asset handle
  asset_handle_t *asset = NULL;
  for (u32 i = 0; i < MAX_ASSET_LOADERS; i++) {
    if (g_asset_system.assets[i].id == task->asset_id) {
      asset = &g_asset_system.assets[i];
      break;
    }
  }

  if (!asset) {
    LOG_ERROR(LOG_CAT_GENERAL, "Asset handle not found for ID %u",
              task->asset_id);
    if (task->callback) {
      task->callback(0, false, task->userdata);
    }
    free(task);
    return;
  }

  pthread_mutex_lock(&asset->mutex);

  // Check if already loaded
  if (asset->state == ASSET_STATE_LOADED) {
    asset->ref_count++;
    pthread_mutex_unlock(&asset->mutex);

    if (task->callback) {
      task->callback(asset->id, true, asset->data);
    }
    free(task);
    return;
  }

  asset->state = ASSET_STATE_LOADING;

  // Get loader
  const asset_loader_interface_t *loader = &g_asset_system.loaders[task->type];

  // Load asset
  void *data = NULL;
  u32 size = 0;
  bool success = false;

  if (loader && loader->load) {
    success = loader->load(task->path, &data, &size);
  }

  if (success && data) {
    // Update asset
    asset->data = data;
    asset->size_bytes = size;
    asset->state = ASSET_STATE_LOADED;
    asset->ref_count = 1;

    LOG_INFO(LOG_CAT_GENERAL,
             "Successfully loaded asset '%s' (ID: %u, %u bytes)", task->path,
             asset->id, size);
  } else {
    asset->state = ASSET_STATE_ERROR;
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to load asset '%s'", task->path);
  }

  pthread_mutex_unlock(&asset->mutex);

  // Call callback
  if (task->callback) {
    task->callback(asset->id, success, success ? data : NULL);
  }

  free(task);
}

// Load asset asynchronously
u32 asset_loader_load_async(const char *path,
                            void (*callback)(u32 asset_id, bool success,
                                             void *data),
                            void *userdata) {
  if (!g_asset_system.initialized || !path) {
    return 0;
  }

  // Determine asset type
  asset_type_t type = determine_asset_type(path);
  if (type == ASSET_TYPE_UNKNOWN) {
    LOG_ERROR(LOG_CAT_GENERAL, "Unknown asset type for path '%s'", path);
    return 0;
  }

  // Check if already loaded
  pthread_mutex_lock(&g_asset_system.loader_mutex);

  for (u32 i = 0; i < MAX_ASSET_LOADERS; i++) {
    if (g_asset_system.assets[i].state == ASSET_STATE_LOADED &&
        strcmp(g_asset_system.assets[i].path, path) == 0) {

      pthread_mutex_lock(&g_asset_system.assets[i].mutex);
      g_asset_system.assets[i].ref_count++;
      pthread_mutex_unlock(&g_asset_system.assets[i].mutex);

      u32 asset_id = g_asset_system.assets[i].id;
      pthread_mutex_unlock(&g_asset_system.loader_mutex);

      if (callback) {
        callback(asset_id, true, g_asset_system.assets[i].data);
      }

      return asset_id;
    }
  }

  pthread_mutex_unlock(&g_asset_system.loader_mutex);

  // Find free slot
  pthread_mutex_lock(&g_asset_system.loader_mutex);
  u32 slot = find_free_asset_slot();

  if (slot >= MAX_ASSET_LOADERS) {
    LOG_ERROR(LOG_CAT_GENERAL, "No free asset slots available");
    pthread_mutex_unlock(&g_asset_system.loader_mutex);
    return 0;
  }

  // Initialize asset handle
  asset_handle_t *asset = &g_asset_system.assets[slot];
  asset->id = g_asset_system.next_asset_id++;
  strncpy(asset->path, path, 255);
  asset->path[255] = '\0';
  asset->type = type;
  asset->state = ASSET_STATE_UNLOADED;
  asset->data = NULL;
  asset->size_bytes = 0;
  asset->last_modified = 0;
  asset->ref_count = 0;

  if (pthread_mutex_init(&asset->mutex, NULL) != 0) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to initialize asset mutex");
    pthread_mutex_unlock(&g_asset_system.loader_mutex);
    return 0;
  }

  u32 asset_id = asset->id;
  pthread_mutex_unlock(&g_asset_system.loader_mutex);

  // Create load task
  asset_load_task_t *task =
      (asset_load_task_t *)malloc(sizeof(asset_load_task_t));
  if (!task) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate load task");
    return 0;
  }

  strncpy(task->path, path, 255);
  task->path[255] = '\0';
  task->type = type;
  task->asset_id = asset_id;
  task->callback = callback;
  task->userdata = userdata;

  // Submit to thread pool
  thread_pool_submit(g_asset_system.thread_pool, asset_load_worker, task, "asset_load_worker");

  LOG_INFO(LOG_CAT_GENERAL, "Submitted asset '%s' for async loading (ID: %u)",
           path, asset_id);
  return asset_id;
}

// Load asset synchronously
u32 asset_loader_load_sync(const char *path) {
  if (!g_asset_system.initialized || !path) {
    return 0;
  }

  // Determine asset type
  asset_type_t type = determine_asset_type(path);
  if (type == ASSET_TYPE_UNKNOWN) {
    LOG_ERROR(LOG_CAT_GENERAL, "Unknown asset type for path '%s'", path);
    return 0;
  }

  // Check if already loaded
  pthread_mutex_lock(&g_asset_system.loader_mutex);

  for (u32 i = 0; i < MAX_ASSET_LOADERS; i++) {
    if (g_asset_system.assets[i].state == ASSET_STATE_LOADED &&
        strcmp(g_asset_system.assets[i].path, path) == 0) {

      pthread_mutex_lock(&g_asset_system.assets[i].mutex);
      g_asset_system.assets[i].ref_count++;
      pthread_mutex_unlock(&g_asset_system.assets[i].mutex);

      u32 asset_id = g_asset_system.assets[i].id;
      pthread_mutex_unlock(&g_asset_system.loader_mutex);

      return asset_id;
    }
  }

  pthread_mutex_unlock(&g_asset_system.loader_mutex);

  // Find free slot
  pthread_mutex_lock(&g_asset_system.loader_mutex);
  u32 slot = find_free_asset_slot();

  if (slot >= MAX_ASSET_LOADERS) {
    LOG_ERROR(LOG_CAT_GENERAL, "No free asset slots available");
    pthread_mutex_unlock(&g_asset_system.loader_mutex);
    return 0;
  }

  // Initialize asset handle
  asset_handle_t *asset = &g_asset_system.assets[slot];
  asset->id = g_asset_system.next_asset_id++;
  strncpy(asset->path, path, 255);
  asset->path[255] = '\0';
  asset->type = type;
  asset->state = ASSET_STATE_LOADING;
  asset->data = NULL;
  asset->size_bytes = 0;
  asset->last_modified = 0;
  asset->ref_count = 0;

  if (pthread_mutex_init(&asset->mutex, NULL) != 0) {
    LOG_ERROR(LOG_CAT_GENERAL, "Failed to initialize asset mutex");
    pthread_mutex_unlock(&g_asset_system.loader_mutex);
    return 0;
  }

  u32 asset_id = asset->id;
  pthread_mutex_unlock(&g_asset_system.loader_mutex);

  // Load asset synchronously
  const asset_loader_interface_t *loader = &g_asset_system.loaders[type];

  if (loader && loader->load) {
    void *data = NULL;
    u32 size = 0;
    bool success = loader->load(path, &data, &size);

    pthread_mutex_lock(&asset->mutex);

    if (success && data) {
      asset->data = data;
      asset->size_bytes = size;
      asset->state = ASSET_STATE_LOADED;
      asset->ref_count = 1;

      LOG_INFO(LOG_CAT_GENERAL,
               "Successfully loaded asset '%s' (ID: %u, %u bytes)", path,
               asset_id, size);
    } else {
      asset->state = ASSET_STATE_ERROR;
      LOG_ERROR(LOG_CAT_GENERAL, "Failed to load asset '%s'", path);
    }

    pthread_mutex_unlock(&asset->mutex);
  }

  return asset_id;
}

// Unload asset
static bool asset_loader_unload(u32 asset_id) {
  if (!g_asset_system.initialized || asset_id == 0) {
    return false;
  }

  pthread_mutex_lock(&g_asset_system.loader_mutex);

  // Find asset
  asset_handle_t *asset = NULL;
  for (u32 i = 0; i < MAX_ASSET_LOADERS; i++) {
    if (g_asset_system.assets[i].id == asset_id) {
      asset = &g_asset_system.assets[i];
      break;
    }
  }

  if (!asset) {
    LOG_ERROR(LOG_CAT_GENERAL, "Asset ID %u not found", asset_id);
    pthread_mutex_unlock(&g_asset_system.loader_mutex);
    return false;
  }

  pthread_mutex_lock(&asset->mutex);

  // Decrement ref count
  if (asset->ref_count > 0) {
    asset->ref_count--;
  }

  // Unload if no references
  if (asset->ref_count == 0 && asset->state == ASSET_STATE_LOADED) {
    const asset_loader_interface_t *loader =
        &g_asset_system.loaders[asset->type];

    if (loader && loader->unload) {
      loader->unload(asset->data);
    }

    asset->data = NULL;
    asset->size_bytes = 0;
    asset->state = ASSET_STATE_UNLOADED;

    LOG_INFO(LOG_CAT_GENERAL, "Unloaded asset '%s' (ID: %u)", asset->path,
             asset_id);
  }

  pthread_mutex_unlock(&asset->mutex);
  pthread_mutex_unlock(&g_asset_system.loader_mutex);

  return true;
}

// Get asset data
void *asset_loader_get_data(u32 asset_id) {
  if (!g_asset_system.initialized || asset_id == 0) {
    return NULL;
  }

  pthread_mutex_lock(&g_asset_system.loader_mutex);

  for (u32 i = 0; i < MAX_ASSET_LOADERS; i++) {
    if (g_asset_system.assets[i].id == asset_id) {
      pthread_mutex_lock(&g_asset_system.assets[i].mutex);

      void *data = NULL;
      if (g_asset_system.assets[i].state == ASSET_STATE_LOADED) {
        data = g_asset_system.assets[i].data;
      }

      pthread_mutex_unlock(&g_asset_system.assets[i].mutex);
      pthread_mutex_unlock(&g_asset_system.loader_mutex);

      return data;
    }
  }

  pthread_mutex_unlock(&g_asset_system.loader_mutex);
  return NULL;
}

// Get asset state
asset_state_t asset_loader_get_state(u32 asset_id) {
  if (!g_asset_system.initialized || asset_id == 0) {
    return ASSET_STATE_ERROR;
  }

  pthread_mutex_lock(&g_asset_system.loader_mutex);

  for (u32 i = 0; i < MAX_ASSET_LOADERS; i++) {
    if (g_asset_system.assets[i].id == asset_id) {
      asset_state_t state = g_asset_system.assets[i].state;
      pthread_mutex_unlock(&g_asset_system.loader_mutex);
      return state;
    }
  }

  pthread_mutex_unlock(&g_asset_system.loader_mutex);
  return ASSET_STATE_ERROR;
}

// Get asset statistics
void asset_loader_get_stats(u32 *loaded_count, u32 *loading_count,
                            u32 *total_memory) {
  if (!g_asset_system.initialized) {
    if (loaded_count)
      *loaded_count = 0;
    if (loading_count)
      *loading_count = 0;
    if (total_memory)
      *total_memory = 0;
    return;
  }

  u32 loaded = 0;
  u32 loading = 0;
  u32 memory = 0;

  pthread_mutex_lock(&g_asset_system.loader_mutex);

  for (u32 i = 0; i < MAX_ASSET_LOADERS; i++) {
    if (g_asset_system.assets[i].state == ASSET_STATE_LOADED) {
      loaded++;
      memory += g_asset_system.assets[i].size_bytes;
    } else if (g_asset_system.assets[i].state == ASSET_STATE_LOADING) {
      loading++;
    }
  }

  pthread_mutex_unlock(&g_asset_system.loader_mutex);

  if (loaded_count)
    *loaded_count = loaded;
  if (loading_count)
    *loading_count = loading;
  if (total_memory)
    *total_memory = memory;
}
/* TODO: Implement asset loader async operations
 * TODO: Add asset loader GPU integration
 * TODO: Implement asset loader SIMD optimization
 * TODO: Add asset loader batch processing
 * TODO: Implement asset loader streaming support
 * TODO: Add asset loader LOD support
 * TODO: Implement asset loader culling integration
 * TODO: Add asset loader render graph node
 */

#include "assets/system/asset_system/loading/asset_loader.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================
 */

#define ASSET_SYSTEM_ASSET_LOADER_MAX_COUNT 4096
#define ASSET_SYSTEM_ASSET_LOADER_DEFAULT_CAPACITY 256
#define ASSET_SYSTEM_ASSET_LOADER_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================
 */

typedef struct asset_system_asset_loader_internal {
  uint32_t id;
  uint32_t flags;
  void *data;
  size_t data_size;
  bool initialized;
  bool dirty;
  uint64_t frame_updated;
} asset_system_asset_loader_internal_t;

typedef struct asset_system_asset_loader_context {
  asset_system_asset_loader_internal_t *items;
  uint32_t count;
  uint32_t capacity;
  void *allocator;
  bool initialized;
} asset_system_asset_loader_context_t;

static asset_system_asset_loader_context_t g_asset_loader_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================
 */

static bool asset_system_asset_loader_validate(
    const asset_system_asset_loader_internal_t *item) {
  // TODO: Implement Vulkan backend
  // TODO: Implement Metal backend
  if (!item)
    return false;
  if (!item->initialized)
    return false;
  return true;
}

static void asset_system_asset_loader_cleanup_internal(
    asset_system_asset_loader_internal_t *item) {
  // TODO: Implement D3D12 backend
  // TODO: Add thread-safe access patterns
  if (!item)
    return;
  if (item->data) {
    free(item->data);
    item->data = NULL;
  }
  item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================
 */

int asset_system_asset_loader_init(void) {
  // TODO: Implement proper error handling with error codes
  // TODO: Add memory tracking and leak detection
  // TODO: Implement hot-reload support
  // TODO: Add validation layer integration

  if (g_asset_loader_ctx.initialized) {
    return 0; // Already initialized
  }

  g_asset_loader_ctx.capacity = ASSET_SYSTEM_ASSET_LOADER_DEFAULT_CAPACITY;
  g_asset_loader_ctx.items =
      calloc(g_asset_loader_ctx.capacity,
             sizeof(asset_system_asset_loader_internal_t));
  if (!g_asset_loader_ctx.items) {
    return -1;
  }

  g_asset_loader_ctx.count = 0;
  g_asset_loader_ctx.initialized = true;

  return 0;
}

void asset_system_asset_loader_shutdown(void) {
  // TODO: Implement resource state tracking
  // TODO: Add GPU debugging markers
  // TODO: Implement asset loader initialization
  // TODO: Add asset loader cleanup/shutdown

  if (!g_asset_loader_ctx.initialized) {
    return;
  }

  for (uint32_t i = 0; i < g_asset_loader_ctx.count; i++) {
    asset_system_asset_loader_cleanup_internal(&g_asset_loader_ctx.items[i]);
  }

  free(g_asset_loader_ctx.items);
  g_asset_loader_ctx.items = NULL;
  g_asset_loader_ctx.count = 0;
  g_asset_loader_ctx.capacity = 0;
  g_asset_loader_ctx.initialized = false;
}

int asset_system_asset_loader_create(
    asset_system_asset_loader_handle_t *out_handle,
    const asset_system_asset_loader_desc_t *desc) {
  // TODO: Implement asset loader validation
  // TODO: Add asset loader error handling
  // TODO: Implement asset loader serialization
  // TODO: Add asset loader debug output

  if (!out_handle || !desc) {
    return -1;
  }

  if (!g_asset_loader_ctx.initialized) {
    return -2;
  }

  if (g_asset_loader_ctx.count >= g_asset_loader_ctx.capacity) {
    // TODO: Implement asset loader unit tests
    return -3;
  }

  uint32_t index = g_asset_loader_ctx.count++;
  asset_system_asset_loader_internal_t *item = &g_asset_loader_ctx.items[index];

  item->id = index;
  item->flags = desc->flags;
  item->data = NULL;
  item->data_size = 0;
  item->initialized = true;
  item->dirty = true;
  item->frame_updated = 0;

  out_handle->id = index;
  return 0;
}

void asset_system_asset_loader_destroy(
    asset_system_asset_loader_handle_t handle) {
  // TODO: Add asset loader performance counters
  // TODO: Implement asset loader hot-reload

  if (handle.id >= g_asset_loader_ctx.count) {
    return;
  }

  asset_system_asset_loader_cleanup_internal(
      &g_asset_loader_ctx.items[handle.id]);
}

int asset_system_asset_loader_update(asset_system_asset_loader_handle_t handle,
                                     const void *data, size_t size) {
  // TODO: Add asset loader thread safety
  // TODO: Implement asset loader memory pooling
  // TODO: Add asset loader caching layer
  // TODO: Implement asset loader async operations

  if (handle.id >= g_asset_loader_ctx.count) {
    return -1;
  }

  asset_system_asset_loader_internal_t *item =
      &g_asset_loader_ctx.items[handle.id];
  if (!item->initialized) {
    return -2;
  }

  // TODO: Add asset loader GPU integration
  // TODO: Implement asset loader SIMD optimization

  item->dirty = true;
  return 0;
}

bool asset_system_asset_loader_is_valid(
    asset_system_asset_loader_handle_t handle) {
  // TODO: Add asset loader batch processing
  if (handle.id >= g_asset_loader_ctx.count) {
    return false;
  }
  return g_asset_loader_ctx.items[handle.id].initialized;
}

int asset_system_asset_loader_get_info(
    asset_system_asset_loader_handle_t handle,
    asset_system_asset_loader_info_t *out_info) {
  // TODO: Implement asset loader streaming support
  // TODO: Add asset loader LOD support

  if (!out_info) {
    return -1;
  }

  if (handle.id >= g_asset_loader_ctx.count) {
    return -2;
  }

  const asset_system_asset_loader_internal_t *item =
      &g_asset_loader_ctx.items[handle.id];
  out_info->id = item->id;
  out_info->flags = item->flags;
  out_info->initialized = item->initialized;

  return 0;
}

void asset_system_asset_loader_mark_dirty(
    asset_system_asset_loader_handle_t handle) {
  // TODO: Implement asset loader culling integration
  if (handle.id < g_asset_loader_ctx.count) {
    g_asset_loader_ctx.items[handle.id].dirty = true;
  }
}

int asset_system_asset_loader_process_pending(void) {
  // TODO: Add asset loader render graph node
  // TODO: Implement batch processing

  int processed = 0;
  for (uint32_t i = 0; i < g_asset_loader_ctx.count; i++) {
    asset_system_asset_loader_internal_t *item = &g_asset_loader_ctx.items[i];
    if (item->initialized && item->dirty) {
      // Process item
      item->dirty = false;
      processed++;
    }
  }

  return processed;
}

uint32_t asset_system_asset_loader_get_count(void) {
  return g_asset_loader_ctx.count;
}

size_t asset_system_asset_loader_get_memory_usage(void) {
  // TODO: Implement memory tracking
  size_t total = sizeof(g_asset_loader_ctx);
  total += g_asset_loader_ctx.capacity *
           sizeof(asset_system_asset_loader_internal_t);

  for (uint32_t i = 0; i < g_asset_loader_ctx.count; i++) {
    total += g_asset_loader_ctx.items[i].data_size;
  }

  return total;
}

void asset_system_asset_loader_debug_print(void) {
  // TODO: Implement debug output
  // Debug printing implementation
}

/* End of asset_loader.c */
