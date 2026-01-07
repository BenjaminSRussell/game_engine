/**
 * UNIFIED ASSET IMPORTER
 * Handles loading of textures, meshes, and audio with caching
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  ASSET_TYPE_UNKNOWN,
  ASSET_TYPE_TEXTURE,
  ASSET_TYPE_MESH,
  ASSET_TYPE_AUDIO,
  ASSET_TYPE_SHADER,
  ASSET_TYPE_MATERIAL
} AssetType;

typedef struct {
  uint64_t asset_id;
  AssetType type;
  char path[256];
  void *data;
  size_t data_size;
  int ref_count;
  bool is_loaded;
} Asset;

#define MAX_ASSETS 1024
#define HASH_MAP_SIZE 2048

static Asset *g_asset_pool[MAX_ASSETS];
static int g_asset_count = 0;

// Simple hash map for path -> index
typedef struct {
  uint64_t hash;
  int asset_index;
} HashEntry;

static HashEntry g_asset_map[HASH_MAP_SIZE];

static uint64_t hash_string(const char *str) {
  uint64_t hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}

static int map_get(const char *path) {
  uint64_t h = hash_string(path);
  int idx = h % HASH_MAP_SIZE;
  while (g_asset_map[idx].asset_index != -1) {
    if (g_asset_map[idx].hash == h)
      return g_asset_map[idx].asset_index;
    idx = (idx + 1) % HASH_MAP_SIZE;
  }
  return -1;
}

static void map_insert(const char *path, int asset_index) {
  uint64_t h = hash_string(path);
  int idx = h % HASH_MAP_SIZE;
  while (g_asset_map[idx].asset_index != -1) {
    idx = (idx + 1) % HASH_MAP_SIZE;
  }
  g_asset_map[idx].hash = h;
  g_asset_map[idx].asset_index = asset_index;
}

// Format Loaders (Stubs for specific libraries like stb_image, cgltf)
static void *load_texture(const char *path, size_t *size) {
  // Stub: Would call stbi_load
  *size = 0;
  return (void *)1; // Fake pointer
}

#include <renderer/mesh.h>

static void *load_mesh(const char *path, size_t *size) {
  // Simple heuristic for procedural loading or file loading
  // "Serious" asset loading would integrate cgltf here.
  // For now, we support procedural generation to verify the pipeline.

  Mesh *mesh = NULL;

  if (strstr(path, "cube")) {
    mesh = mesh_create_cube(1.0f);
  } else if (strstr(path, "sphere")) {
    // mesh_create_sphere not implemented in mesh.c yet, fallback to cube
    mesh = mesh_create_cube(1.0f);
  } else {
    // Default fallback
    mesh = mesh_create_cube(1.0f);
  }

  if (mesh) {
    *size = sizeof(Mesh);
    // In a real scenario we might upload to GPU here or defer
    // For thread safety, usually defer upload.
  } else {
    *size = 0;
  }

  return (void *)mesh;
}

static void *load_audio(const char *path, size_t *size) {
  // Stub: Would call dr_wav or miniaudio
  *size = 0;
  return (void *)1;
}

void asset_system_init() {
  memset(g_asset_pool, 0, sizeof(g_asset_pool));
  for (int i = 0; i < HASH_MAP_SIZE; i++)
    g_asset_map[i].asset_index = -1;
  g_asset_count = 0;
}

Asset *asset_load(const char *path, AssetType type) {
  // Check cache
  int idx = map_get(path);
  if (idx != -1) {
    Asset *asset = g_asset_pool[idx];
    asset->ref_count++;
    return asset;
  }

  // Load new
  if (g_asset_count >= MAX_ASSETS)
    return NULL;

  Asset *asset = malloc(sizeof(Asset));
  asset->asset_id = hash_string(path); // Simple ID
  asset->type = type;
  strncpy(asset->path, path, 255);
  asset->ref_count = 1;
  asset->is_loaded = false;

  // Load data based on type
  switch (type) {
  case ASSET_TYPE_TEXTURE:
    asset->data = load_texture(path, &asset->data_size);
    break;
  case ASSET_TYPE_MESH:
    asset->data = load_mesh(path, &asset->data_size);
    break;
  case ASSET_TYPE_AUDIO:
    asset->data = load_audio(path, &asset->data_size);
    break;
  default:
    asset->data = NULL;
    break;
  }

  if (asset->data)
    asset->is_loaded = true;

  // Add to pool and map
  int pool_idx = g_asset_count++;
  g_asset_pool[pool_idx] = asset;
  map_insert(path, pool_idx);

  return asset;
}

#include <core/asset_importers.h>

// Stub implementation for audio loading
ImportedAudio *asset_importer_load_audio(const char *filepath) {
  // In a real implementation, this would use stbi_vorbis or dr_wav
  // For now, return a dummy buffer
  ImportedAudio *audio = (ImportedAudio *)malloc(sizeof(ImportedAudio));
  if (!audio)
    return NULL;

  audio->channels = 2;
  audio->sample_rate = 44100;
  audio->size_bytes = 44100 * 2 * 2; // 1 second of silence
  audio->data = calloc(1, audio->size_bytes);

  return audio;
}

void asset_release(Asset *asset) {
  if (!asset)
    return;
  asset->ref_count--;

  if (asset->ref_count <= 0) {
    // Unload logic usually deferred or ref-counted to 0
    // For this simple system, we keep it in cache until system shutdown
    // or implement explicit unload
  }
}

void asset_system_shutdown() {
  for (int i = 0; i < g_asset_count; i++) {
    // Free asset data
    // free(g_asset_pool[i]->data);
    free(g_asset_pool[i]);
  }
}
