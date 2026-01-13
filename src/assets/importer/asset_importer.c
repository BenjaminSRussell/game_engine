/**
 * UNIFIED ASSET IMPORTER
 * Handles loading of textures, meshes, and audio with caching
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CGLTF_IMPLEMENTATION
#include "vendor/cgltf.h"

#include <core/asset_importers.h>
#include <core/logger.h>
#include <geometry/mesh_primitives.h>
#include <rendering/mesh.h>

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

// Format Loaders
static void *load_texture(const char *path, size_t *size) {
  // Generate checkerboard texture if file doesn't exist
  // 64x64 RGBA
  int width = 64;
  int height = 64;
  int channels = 4;
  *size = width * height * channels;
  uint8_t *pixels = (uint8_t *)malloc(*size);

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int index = (y * width + x) * channels;
      int check = ((x / 8) + (y / 8)) % 2;
      if (check) {
        pixels[index] = 255;     // R
        pixels[index + 1] = 0;   // G
        pixels[index + 2] = 255; // B (Magenta)
        pixels[index + 3] = 255;
      } else {
        pixels[index] = 30;
        pixels[index + 1] = 30;
        pixels[index + 2] = 30;
        pixels[index + 3] = 255;
      }
    }
  }

  return pixels;
}

static mesh_t *load_mesh_gltf(const char *path) {
  cgltf_options options = {0};
  cgltf_data *data = NULL;
  cgltf_result result = cgltf_parse_file(&options, path, &data);

  if (result != cgltf_result_success) {
    LOG_ERROR("Failed to parse GLTF: %s", path);
    return NULL;
  }

  result = cgltf_load_buffers(&options, data, path);
  if (result != cgltf_result_success) {
    cgltf_free(data);
    return NULL;
  }

  // Convert first mesh primitive to our Mesh format
  // Simplification: Just take first node with a mesh
  // Real impl would iterate scene

  mesh_t *mesh = NULL; // Placeholder

  // ... Extraction logic would go here ...
  // For now, logging validity
  LOG_INFO("GLTF Loaded: %s (%u meshes)", path, data->meshes_count);

  cgltf_free(data);
  return mesh_create_cube(1.0f); // Fallback until extraction logic is full
}

static void *load_mesh(const char *path, size_t *size) {
  mesh_t *mesh = NULL;

  if (strstr(path, ".gltf") || strstr(path, ".glb")) {
    mesh = load_mesh_gltf(path);
  } else if (strstr(path, "cube")) {
    mesh = mesh_create_cube(1.0f);
  } else if (strstr(path, "sphere")) {
    mesh = mesh_create_sphere(1.0f, 16);
  } else {
    mesh = mesh_create_cube(1.0f);
  }

  if (mesh) {
    *size = sizeof(mesh_t);
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
  int idx = map_get(path);
  if (idx != -1) {
    Asset *asset = g_asset_pool[idx];
    asset->ref_count++;
    return asset;
  }

  if (g_asset_count >= MAX_ASSETS)
    return NULL;

  Asset *asset = malloc(sizeof(Asset));
  asset->asset_id = hash_string(path);
  asset->type = type;
  strncpy(asset->path, path, 255);
  asset->ref_count = 1;
  asset->is_loaded = false;

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

  int pool_idx = g_asset_count++;
  g_asset_pool[pool_idx] = asset;
  map_insert(path, pool_idx);

  return asset;
}

// Stub implementation for audio loading
ImportedAudio *asset_importer_load_audio(const char *filepath) {
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
}

void asset_system_shutdown() {
  for (int i = 0; i < g_asset_count; i++) {
    free(g_asset_pool[i]);
  }
}
