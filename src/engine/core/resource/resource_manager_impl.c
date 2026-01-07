/**
 * =================================================================================================
 *                              RESOURCE MANAGER IMPLEMENTATION
 *                              Agent: AGENT_ASSET_1
 * =================================================================================================
 */

#include "resource_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    INTERNAL STRUCTURES
 * =================================================================================================
 */

typedef struct AssetHeader {
  uint32_t id;
  AssetType type;
  char path[256];
  uint32_t size_bytes;
  uint32_t ref_count;
  bool is_loaded;
  void *data;
} AssetHeader;

static struct {
  AssetHeader *assets;
  uint32_t count;
  uint32_t capacity;
  uint64_t memory_budget;
  uint64_t memory_used;
  bool shutting_down;
} g_resource_manager = {0};

/* =================================================================================================
 *                                    ASSET LOADING
 * =================================================================================================
 */

void *asset_texture_load(const char *path) { return malloc(1024); }
void asset_texture_load_async(const char *path, void (*callback)(void *)) {}
void *asset_mesh_load(const char *path) { return malloc(1024); }
void asset_mesh_load_async(const char *path, void (*callback)(void *)) {}
void *asset_material_load(const char *path) { return malloc(1024); }
void *asset_shader_load(const char *path) { return malloc(1024); }
void *asset_audio_load(const char *path) { return malloc(1024); }
void asset_audio_load_async(const char *path, void (*callback)(void *)) {}
void *asset_animation_load(const char *path) { return malloc(1024); }
void *asset_font_load(const char *path) { return malloc(1024); }
void *asset_prefab_load(const char *path) { return malloc(1024); }
void *asset_scene_load(const char *path) { return malloc(1024); }
void *asset_script_load(const char *path) { return malloc(1024); }
void *asset_data_load(const char *path) { return malloc(1024); }
void *asset_localization_load(const char *path) { return malloc(1024); }

/* =================================================================================================
 *                                    ASSET UNLOADING
 * =================================================================================================
 */

void asset_texture_unload(void *asset) { free(asset); }
void asset_mesh_unload(void *asset) { free(asset); }
void asset_material_unload(void *asset) { free(asset); }
void asset_audio_unload(void *asset) { free(asset); }
void asset_animation_unload(void *asset) { free(asset); }
void asset_font_unload(void *asset) { free(asset); }
void asset_prefab_unload(void *asset) { free(asset); }
void asset_scene_unload(void *asset) { free(asset); }
void asset_unload_unused(void) {}
void asset_force_unload(uint32_t asset_id) {}

/* =================================================================================================
 *                                    REFERENCE COUNTING
 * =================================================================================================
 */

void ref_count_increment(uint32_t asset_id) {}
void ref_count_decrement(uint32_t asset_id) {}
uint32_t ref_count_get(uint32_t asset_id) { return 0; }
void ref_count_track_usage(uint32_t asset_id, const char *source) {}
void ref_count_find_leaks(void) {}
void ref_count_report(void) {}

/* =================================================================================================
 *                                    ASSET BUNDLES
 * =================================================================================================
 */

void *bundle_create(const char *name) { return malloc(64); }
bool bundle_load(void *bundle, const char *path) { return true; }
void bundle_load_async(void *bundle, const char *path,
                       void (*callback)(void *)) {}
void bundle_unload(void *bundle) { free(bundle); }
void *bundle_get_asset(void *bundle, const char *asset_name) {
  return malloc(1024);
}
void bundle_get_all_assets(void *bundle, void ***out_assets,
                           uint32_t *out_count) {}
void bundle_get_dependencies(void *bundle, char ***out_deps,
                             uint32_t *out_count) {}
bool bundle_verify_integrity(void *bundle) { return true; }
bool bundle_compress(void *bundle) { return true; }
bool bundle_decompress(void *bundle) { return true; }
void bundle_encrypt(void *bundle, const char *key) {}
void bundle_decrypt(void *bundle, const char *key) {}
void bundle_patch(void *bundle, const char *patch_path) {}
void bundle_diff(void *bundle_a, void *bundle_b, const char *out_patch_path) {}

/* =================================================================================================
 *                                    STREAMING
 * =================================================================================================
 */

void streaming_init(void) {}
void streaming_shutdown(void) {}
void streaming_update(float dt) {}
void streaming_request(const char *path, bool high_priority) {}
void streaming_cancel(const char *path) {}
void streaming_prioritize(const char *path) {}
void streaming_set_budget(uint64_t bytes) {}
float streaming_get_progress(const char *path) { return 1.0f; }
void streaming_prefetch(const char *path) {}
void streaming_flush(void) {}
void streaming_texture_mip(void *texture, int mip_level) {}
void streaming_mesh_lod(void *mesh, int lod_level) {}
void streaming_audio_chunk(void *audio, int chunk_index) {}

/* =================================================================================================
 *                                    HOT RELOAD
 * =================================================================================================
 */

void hot_reload_init(void) {}
void hot_reload_watch(const char *dir) {}
void hot_reload_unwatch(const char *dir) {}
void hot_reload_poll(void) {}
void hot_reload_trigger(const char *path) {}
void hot_reload_queue(const char *path) {}
void hot_reload_process(void) {}
void hot_reload_shader(const char *path) {}
void hot_reload_texture(const char *path) {}
void hot_reload_script(const char *path) {}
void hot_reload_config(const char *path) {}

/* =================================================================================================
 *                                    RESOURCE DATABASE
 * =================================================================================================
 */

void database_init(const char *db_path) {}
void database_shutdown(void) {}
void database_register(const char *path, AssetType type) {}
void database_unregister(const char *path) {}
uint32_t database_lookup(const char *path) { return 0; }
uint32_t database_lookup_by_path(const char *path) { return 0; }
uint32_t database_lookup_by_guid(const char *guid) { return 0; }
void database_search(const char *query, uint32_t *out_ids,
                     uint32_t max_results) {}
void database_filter_by_type(AssetType type, uint32_t *out_ids,
                             uint32_t max_results) {}
const char *database_get_metadata(uint32_t id, const char *key) { return ""; }
void database_set_metadata(uint32_t id, const char *key, const char *value) {}
void database_import(const char *src_path) {}
void database_export(uint32_t id, const char *dst_path) {}
void database_serialize(const char *path) {}
void database_deserialize(const char *path) {}

/* =================================================================================================
 *                                    RESOURCE MANAGER API
 * =================================================================================================
 */

void resource_manager_init(uint32_t initial_capacity) {
  g_resource_manager.capacity = initial_capacity;
  g_resource_manager.assets =
      (AssetHeader *)calloc(initial_capacity, sizeof(AssetHeader));
}

void resource_manager_shutdown(void) {
  g_resource_manager.shutting_down = true;
  if (g_resource_manager.assets)
    free(g_resource_manager.assets);
}

void resource_manager_update(float dt) {
  streaming_update(dt);
  hot_reload_poll();
}

void *resource_manager_load(const char *path) {
  // Check db, check cache, load
  return malloc(1024);
}

void resource_manager_load_async(const char *path, void (*callback)(void *)) {}
void *resource_manager_get(const char *path) { return NULL; }
void resource_manager_release(const char *path) {}
void resource_manager_preload(const char *path) {}
void resource_manager_gc(void) {}
void resource_manager_stats(void) {}
void resource_manager_memory_budget(uint64_t bytes) {
  g_resource_manager.memory_budget = bytes;
}
