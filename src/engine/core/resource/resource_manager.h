/**
 * =================================================================================================
 *                              RESOURCE MANAGER & ASSET LOADING
 *                              Agent: AGENT_ASSET_1
 * =================================================================================================
 */

#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum AssetType {
  ASSET_TYPE_UNKNOWN,
  ASSET_TYPE_TEXTURE,
  ASSET_TYPE_MESH,
  ASSET_TYPE_MATERIAL,
  ASSET_TYPE_SHADER,
  ASSET_TYPE_AUDIO,
  ASSET_TYPE_ANIMATION,
  ASSET_TYPE_FONT,
  ASSET_TYPE_PREFAB,
  ASSET_TYPE_SCENE,
  ASSET_TYPE_SCRIPT,
  ASSET_TYPE_DATA,
  ASSET_TYPE_LOCALIZATION,
  ASSET_TYPE_COUNT
} AssetType;

/* =================================================================================================
 *                                    ASSET LOADING
 * =================================================================================================
 */

uint32_t asset_texture_load(const char *path);
void asset_texture_load_async(const char *path, void (*callback)(uint32_t));
uint32_t asset_mesh_load(const char *path);
void asset_mesh_load_async(const char *path, void (*callback)(uint32_t));
uint32_t asset_material_load(const char *path);
uint32_t asset_shader_load(const char *path);
uint32_t asset_audio_load(const char *path);
void asset_audio_load_async(const char *path, void (*callback)(uint32_t));
uint32_t asset_animation_load(const char *path);
uint32_t asset_font_load(const char *path);
uint32_t asset_prefab_load(const char *path);
uint32_t asset_scene_load(const char *path);
uint32_t asset_script_load(const char *path);
uint32_t asset_data_load(const char *path);
uint32_t asset_localization_load(const char *path);

/* =================================================================================================
 *                                    ASSET UNLOADING
 * =================================================================================================
 */

void asset_texture_unload(uint32_t asset_id);
void asset_mesh_unload(uint32_t asset_id);
void asset_material_unload(uint32_t asset_id);
void asset_audio_unload(uint32_t asset_id);
void asset_animation_unload(uint32_t asset_id);
void asset_font_unload(uint32_t asset_id);
void asset_prefab_unload(uint32_t asset_id);
void asset_scene_unload(uint32_t asset_id);
void asset_unload_unused(void);
void asset_force_unload(uint32_t asset_id);

/* =================================================================================================
 *                                    REFERENCE COUNTING
 * =================================================================================================
 */

void ref_count_increment(uint32_t asset_id);
void ref_count_decrement(uint32_t asset_id);
uint32_t ref_count_get(uint32_t asset_id);
void ref_count_track_usage(uint32_t asset_id, const char *source);
void ref_count_find_leaks(void);
void ref_count_report(void);

/* =================================================================================================
 *                                    ASSET BUNDLES
 * =================================================================================================
 */

void *bundle_create(const char *name);
bool bundle_load(void *bundle, const char *path);
void bundle_load_async(void *bundle, const char *path,
                       void (*callback)(void *));
void bundle_unload(void *bundle);
void *bundle_get_asset(void *bundle, const char *asset_name);
void bundle_get_all_assets(void *bundle, void ***out_assets,
                           uint32_t *out_count);
void bundle_get_dependencies(void *bundle, char ***out_deps,
                             uint32_t *out_count);
bool bundle_verify_integrity(void *bundle);
bool bundle_compress(void *bundle);
bool bundle_decompress(void *bundle);
void bundle_encrypt(void *bundle, const char *key);
void bundle_decrypt(void *bundle, const char *key);
void bundle_patch(void *bundle, const char *patch_path);
void bundle_diff(void *bundle_a, void *bundle_b, const char *out_patch_path);

/* =================================================================================================
 *                                    STREAMING
 * =================================================================================================
 */

void streaming_init(void);
void streaming_shutdown(void);
void streaming_update(float dt);
void streaming_request(const char *path, bool high_priority);
void streaming_cancel(const char *path);
void streaming_prioritize(const char *path);
void streaming_set_budget(uint64_t bytes);
float streaming_get_progress(const char *path);
void streaming_prefetch(const char *path);
void streaming_flush(void);
void streaming_texture_mip(void *texture, int mip_level);
void streaming_mesh_lod(void *mesh, int lod_level);
void streaming_audio_chunk(void *audio, int chunk_index);

/* =================================================================================================
 *                                    HOT RELOAD
 * =================================================================================================
 */

void hot_reload_init(void);
void hot_reload_watch(const char *dir);
void hot_reload_unwatch(const char *dir);
void hot_reload_poll(void);
void hot_reload_trigger(const char *path);
void hot_reload_queue(const char *path);
void hot_reload_process(void);
void hot_reload_shader(const char *path);
void hot_reload_texture(const char *path);
void hot_reload_script(const char *path);
void hot_reload_config(const char *path);

/* =================================================================================================
 *                                    RESOURCE DATABASE
 * =================================================================================================
 */

void database_init(const char *db_path);
void database_shutdown(void);
void database_register(const char *path, AssetType type);
void database_unregister(const char *path);
uint32_t database_lookup(const char *path);
uint32_t database_lookup_by_path(const char *path);
uint32_t database_lookup_by_guid(const char *guid);
void database_search(const char *query, uint32_t *out_ids,
                     uint32_t max_results);
void database_filter_by_type(AssetType type, uint32_t *out_ids,
                             uint32_t max_results);
void *database_get_metadata(uint32_t id, size_t *size);
void database_set_metadata(uint32_t id, const char *key, const char *value);
void database_import(const char *src_path);
void database_export(uint32_t id, const char *dst_path);
void database_serialize(const char *path);
void database_deserialize(const char *path);

/* =================================================================================================
 *                                    RESOURCE MANAGER
 * =================================================================================================
 */

#include <stddef.h>

bool resource_manager_init(void);
void resource_manager_shutdown(void);
void resource_manager_update(void);
uint32_t resource_manager_load(const char *path);
void resource_manager_load_async(const char *path, void (*callback)(uint32_t));
void *resource_manager_get(uint32_t id);
void resource_manager_release(uint32_t id);
void resource_manager_preload(const char *path);
void resource_manager_gc(void);
void resource_manager_stats(uint32_t *count, size_t *memory_used, size_t *memory_budget);
void resource_manager_memory_budget(size_t budget);

#endif // RESOURCE_MANAGER_H
