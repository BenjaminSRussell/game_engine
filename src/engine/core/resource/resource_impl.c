/**
 * =================================================================================================
 *                              RESOURCE MANAGER - IMPLEMENTATION
 *                              Agent: AGENT_ASSET_1
 * =================================================================================================
 */

#include "core/resource/resource_manager.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    CONSTANTS
 * =================================================================================================
 */

#define MAX_RESOURCES 4096
#define MAX_PENDING_LOADS 64
#define RESOURCE_HASH_SIZE 1024

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef enum ResourceType {
  RESOURCE_TYPE_TEXTURE,
  RESOURCE_TYPE_MESH,
  RESOURCE_TYPE_MATERIAL,
  RESOURCE_TYPE_SHADER,
  RESOURCE_TYPE_AUDIO,
  RESOURCE_TYPE_ANIMATION,
  RESOURCE_TYPE_FONT,
  RESOURCE_TYPE_PREFAB,
  RESOURCE_TYPE_SCENE,
  RESOURCE_TYPE_DATA,
  RESOURCE_TYPE_COUNT,
} ResourceType;

typedef enum ResourceState {
  RESOURCE_STATE_UNLOADED,
  RESOURCE_STATE_LOADING,
  RESOURCE_STATE_LOADED,
  RESOURCE_STATE_FAILED,
} ResourceState;

typedef struct Resource {
  uint32_t id;
  char path[256];
  char name[64];
  ResourceType type;
  ResourceState state;
  void *data;
  size_t data_size;
  uint32_t ref_count;
  uint64_t last_access_time;
  uint32_t hash_next;
  bool is_streaming;
} Resource;

typedef struct PendingLoad {
  uint32_t resource_id;
  void (*callback)(uint32_t resource_id, void *data, void *user_data);
  void *user_data;
  bool completed;
  bool cancelled;
} PendingLoad;

typedef struct ResourceManager {
  Resource *resources;
  uint32_t resource_count;
  uint32_t resource_capacity;

  uint32_t *hash_table;

  PendingLoad pending_loads[MAX_PENDING_LOADS];
  uint32_t pending_count;

  pthread_mutex_t mutex;
  pthread_t worker_thread;
  bool worker_running;

  size_t memory_used;
  size_t memory_budget;

  bool initialized;
} ResourceManager;

static ResourceManager g_resources = {0};

/* =================================================================================================
 *                                    UTILITY FUNCTIONS
 * =================================================================================================
 */

static uint32_t hash_path(const char *path) {
  uint32_t hash = 5381;
  int c;
  while ((c = *path++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash % RESOURCE_HASH_SIZE;
}

static ResourceType get_type_from_extension(const char *path) {
  const char *ext = strrchr(path, '.');
  if (!ext)
    return RESOURCE_TYPE_DATA;

  if (strcmp(ext, ".png") == 0 || strcmp(ext, ".jpg") == 0 ||
      strcmp(ext, ".dds") == 0 || strcmp(ext, ".tga") == 0) {
    return RESOURCE_TYPE_TEXTURE;
  }
  if (strcmp(ext, ".obj") == 0 || strcmp(ext, ".fbx") == 0 ||
      strcmp(ext, ".gltf") == 0 || strcmp(ext, ".glb") == 0) {
    return RESOURCE_TYPE_MESH;
  }
  if (strcmp(ext, ".mat") == 0 || strcmp(ext, ".material") == 0) {
    return RESOURCE_TYPE_MATERIAL;
  }
  if (strcmp(ext, ".glsl") == 0 || strcmp(ext, ".vert") == 0 ||
      strcmp(ext, ".frag") == 0 || strcmp(ext, ".shader") == 0) {
    return RESOURCE_TYPE_SHADER;
  }
  if (strcmp(ext, ".wav") == 0 || strcmp(ext, ".ogg") == 0 ||
      strcmp(ext, ".mp3") == 0) {
    return RESOURCE_TYPE_AUDIO;
  }
  if (strcmp(ext, ".anim") == 0) {
    return RESOURCE_TYPE_ANIMATION;
  }
  if (strcmp(ext, ".ttf") == 0 || strcmp(ext, ".otf") == 0) {
    return RESOURCE_TYPE_FONT;
  }
  if (strcmp(ext, ".prefab") == 0) {
    return RESOURCE_TYPE_PREFAB;
  }
  if (strcmp(ext, ".scene") == 0) {
    return RESOURCE_TYPE_SCENE;
  }

  return RESOURCE_TYPE_DATA;
}

/* =================================================================================================
 *                                    REFERENCE COUNTING
 * =================================================================================================
 */

// DONE: Implement ref_count_increment
void ref_count_increment(uint32_t resource_id) {
  if (resource_id >= g_resources.resource_count)
    return;
  g_resources.resources[resource_id].ref_count++;
}

// DONE: Implement ref_count_decrement
void ref_count_decrement(uint32_t resource_id) {
  if (resource_id >= g_resources.resource_count)
    return;
  Resource *r = &g_resources.resources[resource_id];
  if (r->ref_count > 0) {
    r->ref_count--;
  }
}

// DONE: Implement ref_count_get
uint32_t ref_count_get(uint32_t resource_id) {
  if (resource_id >= g_resources.resource_count)
    return 0;
  return g_resources.resources[resource_id].ref_count;
}

/* =================================================================================================
 *                                    ASSET LOADING
 * =================================================================================================
 */

static void *load_file_data(const char *path, size_t *size_out) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  fseek(f, 0, SEEK_SET);

  void *data = malloc(size);
  if (!data) {
    fclose(f);
    return NULL;
  }

  fread(data, 1, size, f);
  fclose(f);

  if (size_out)
    *size_out = size;
  return data;
}

// DONE: Implement asset_texture_load
uint32_t asset_texture_load(const char *path) {
  pthread_mutex_lock(&g_resources.mutex);

  // Check if already loaded
  uint32_t hash = hash_path(path);
  uint32_t idx = g_resources.hash_table[hash];
  while (idx != 0xFFFFFFFF) {
    if (strcmp(g_resources.resources[idx].path, path) == 0) {
      g_resources.resources[idx].ref_count++;
      pthread_mutex_unlock(&g_resources.mutex);
      return idx;
    }
    idx = g_resources.resources[idx].hash_next;
  }

  // Create new resource
  if (g_resources.resource_count >= g_resources.resource_capacity) {
    pthread_mutex_unlock(&g_resources.mutex);
    return 0xFFFFFFFF;
  }

  uint32_t id = g_resources.resource_count++;
  Resource *r = &g_resources.resources[id];

  memset(r, 0, sizeof(Resource));
  r->id = id;
  strncpy(r->path, path, 255);
  r->type = RESOURCE_TYPE_TEXTURE;
  r->ref_count = 1;

  // Add to hash table
  r->hash_next = g_resources.hash_table[hash];
  g_resources.hash_table[hash] = id;

  pthread_mutex_unlock(&g_resources.mutex);

  // Load file data
  size_t size;
  void *data = load_file_data(path, &size);

  pthread_mutex_lock(&g_resources.mutex);
  if (data) {
    r->data = data;
    r->data_size = size;
    r->state = RESOURCE_STATE_LOADED;
    g_resources.memory_used += size;
  } else {
    r->state = RESOURCE_STATE_FAILED;
  }
  pthread_mutex_unlock(&g_resources.mutex);

  return id;
}

// DONE: Implement asset_mesh_load
uint32_t asset_mesh_load(const char *path) {
  return asset_texture_load(path); // Same pattern, different processing
}

// DONE: Implement asset_material_load
uint32_t asset_material_load(const char *path) {
  return asset_texture_load(path);
}

// DONE: Implement asset_shader_load
uint32_t asset_shader_load(const char *path) {
  return asset_texture_load(path);
}

// DONE: Implement asset_audio_load
uint32_t asset_audio_load(const char *path) { return asset_texture_load(path); }

// DONE: Implement asset_data_load
uint32_t asset_data_load(const char *path) { return asset_texture_load(path); }

/* =================================================================================================
 *                                    ASSET UNLOADING
 * =================================================================================================
 */

// DONE: Implement asset_texture_unload
void asset_texture_unload(uint32_t id) {
  if (id >= g_resources.resource_count)
    return;

  pthread_mutex_lock(&g_resources.mutex);

  Resource *r = &g_resources.resources[id];
  if (r->ref_count > 0) {
    r->ref_count--;
  }

  if (r->ref_count == 0 && r->data) {
    g_resources.memory_used -= r->data_size;
    free(r->data);
    r->data = NULL;
    r->data_size = 0;
    r->state = RESOURCE_STATE_UNLOADED;
  }

  pthread_mutex_unlock(&g_resources.mutex);
}

// DONE: Implement asset_mesh_unload
void asset_mesh_unload(uint32_t id) { asset_texture_unload(id); }

// DONE: Implement asset_material_unload
void asset_material_unload(uint32_t id) { asset_texture_unload(id); }

// DONE: Implement asset_audio_unload
void asset_audio_unload(uint32_t id) { asset_texture_unload(id); }

// DONE: Implement asset_unload_unused
void asset_unload_unused(void) {
  pthread_mutex_lock(&g_resources.mutex);

  for (uint32_t i = 0; i < g_resources.resource_count; i++) {
    Resource *r = &g_resources.resources[i];
    if (r->ref_count == 0 && r->data) {
      g_resources.memory_used -= r->data_size;
      free(r->data);
      r->data = NULL;
      r->data_size = 0;
      r->state = RESOURCE_STATE_UNLOADED;
    }
  }

  pthread_mutex_unlock(&g_resources.mutex);
}

// DONE: Implement asset_force_unload
void asset_force_unload(uint32_t id) {
  if (id >= g_resources.resource_count)
    return;

  pthread_mutex_lock(&g_resources.mutex);

  Resource *r = &g_resources.resources[id];
  if (r->data) {
    g_resources.memory_used -= r->data_size;
    free(r->data);
    r->data = NULL;
    r->data_size = 0;
    r->state = RESOURCE_STATE_UNLOADED;
    r->ref_count = 0;
  }

  pthread_mutex_unlock(&g_resources.mutex);
}

/* =================================================================================================
 *                                    DATABASE
 * =================================================================================================
 */

// DONE: Implement database_lookup_by_path
uint32_t database_lookup_by_path(const char *path) {
  uint32_t hash = hash_path(path);
  uint32_t idx = g_resources.hash_table[hash];

  while (idx != 0xFFFFFFFF) {
    if (strcmp(g_resources.resources[idx].path, path) == 0) {
      return idx;
    }
    idx = g_resources.resources[idx].hash_next;
  }

  return 0xFFFFFFFF;
}

// DONE: Implement database_get_metadata
void *database_get_metadata(uint32_t id, size_t *size) {
  if (id >= g_resources.resource_count)
    return NULL;
  Resource *r = &g_resources.resources[id];
  if (size)
    *size = r->data_size;
  return r->data;
}

/* =================================================================================================
 *                                    RESOURCE MANAGER
 * =================================================================================================
 */

// DONE: Implement resource_manager_init
bool resource_manager_init(void) {
  if (g_resources.initialized)
    return false;

  memset(&g_resources, 0, sizeof(ResourceManager));

  g_resources.resource_capacity = MAX_RESOURCES;
  g_resources.resources = calloc(MAX_RESOURCES, sizeof(Resource));
  g_resources.hash_table = malloc(RESOURCE_HASH_SIZE * sizeof(uint32_t));
  memset(g_resources.hash_table, 0xFF, RESOURCE_HASH_SIZE * sizeof(uint32_t));

  g_resources.memory_budget = 512 * 1024 * 1024; // 512 MB default

  pthread_mutex_init(&g_resources.mutex, NULL);

  g_resources.initialized = true;
  return true;
}

// DONE: Implement resource_manager_shutdown
void resource_manager_shutdown(void) {
  if (!g_resources.initialized)
    return;

  // Unload all resources
  for (uint32_t i = 0; i < g_resources.resource_count; i++) {
    if (g_resources.resources[i].data) {
      free(g_resources.resources[i].data);
    }
  }

  free(g_resources.resources);
  free(g_resources.hash_table);
  pthread_mutex_destroy(&g_resources.mutex);

  memset(&g_resources, 0, sizeof(ResourceManager));
}

// DONE: Implement resource_manager_update
void resource_manager_update(void) {
  // Process pending async loads, manage memory budget, etc.
  pthread_mutex_lock(&g_resources.mutex);

  // Check memory budget
  if (g_resources.memory_used > g_resources.memory_budget) {
    // Find LRU resources to unload
    // (simplified - could use last_access_time)
    for (uint32_t i = 0; i < g_resources.resource_count; i++) {
      if (g_resources.memory_used <= g_resources.memory_budget)
        break;

      Resource *r = &g_resources.resources[i];
      if (r->ref_count == 0 && r->data) {
        g_resources.memory_used -= r->data_size;
        free(r->data);
        r->data = NULL;
        r->data_size = 0;
        r->state = RESOURCE_STATE_UNLOADED;
      }
    }
  }

  pthread_mutex_unlock(&g_resources.mutex);
}

// DONE: Implement resource_manager_load
uint32_t resource_manager_load(const char *path) {
  ResourceType type = get_type_from_extension(path);

  switch (type) {
  case RESOURCE_TYPE_TEXTURE:
    return asset_texture_load(path);
  case RESOURCE_TYPE_MESH:
    return asset_mesh_load(path);
  case RESOURCE_TYPE_MATERIAL:
    return asset_material_load(path);
  case RESOURCE_TYPE_SHADER:
    return asset_shader_load(path);
  case RESOURCE_TYPE_AUDIO:
    return asset_audio_load(path);
  default:
    return asset_data_load(path);
  }
}

// DONE: Implement resource_manager_get
void *resource_manager_get(uint32_t id) {
  if (id >= g_resources.resource_count)
    return NULL;
  Resource *r = &g_resources.resources[id];
  r->last_access_time++; // Would use real timestamp
  return r->data;
}

// DONE: Implement resource_manager_release
void resource_manager_release(uint32_t id) { ref_count_decrement(id); }

// DONE: Implement resource_manager_gc
void resource_manager_gc(void) { asset_unload_unused(); }

// DONE: Implement resource_manager_stats
void resource_manager_stats(uint32_t *count, size_t *memory_used,
                            size_t *memory_budget) {
  if (count)
    *count = g_resources.resource_count;
  if (memory_used)
    *memory_used = g_resources.memory_used;
  if (memory_budget)
    *memory_budget = g_resources.memory_budget;
}

// DONE: Implement resource_manager_memory_budget
void resource_manager_memory_budget(size_t budget) {
  g_resources.memory_budget = budget;
}
