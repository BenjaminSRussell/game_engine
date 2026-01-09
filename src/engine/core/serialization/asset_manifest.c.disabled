#include "core/serialization/asset_manifest.h"
#include "core/serialization/json_parser.h"
#include "core/json.h"
#include "core/types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define CRC32_POLY 0xEDB88320

static u32 crc32_compute(const u8 *data, u32 size) {
    u32 crc = 0xFFFFFFFF;
    for (u32 i = 0; i < size; i++) {
        u8 byte = data[i];
        crc ^= byte;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (CRC32_POLY & (-(i32)(crc & 1)));
        }
    }
    return crc ^ 0xFFFFFFFF;
}

AssetManifest *asset_manifest_create(const char *manifest_id) {
    AssetManifest *manifest = (AssetManifest *)calloc(1, sizeof(AssetManifest));
    if (!manifest) return NULL;

    manifest->version = ASSET_MANIFEST_VERSION;
    manifest->entry_capacity = 128;
    manifest->entries = (AssetManifestEntry *)calloc(manifest->entry_capacity, sizeof(AssetManifestEntry));
    manifest->manifest_id = strdup(manifest_id ? manifest_id : "default");

    if (!manifest->entries) {
        free(manifest->manifest_id);
        free(manifest);
        return NULL;
    }

    return manifest;
}

void asset_manifest_free(AssetManifest *manifest) {
    if (!manifest) return;

    for (u32 i = 0; i < manifest->entry_count; i++) {
        AssetManifestEntry *entry = &manifest->entries[i];
        free(entry->asset_id);
        free(entry->path);
        free(entry->file_hash);
        for (u32 j = 0; j < entry->dependency_count; j++) {
            free(entry->dependencies[j].asset_id);
            free(entry->dependencies[j].version);
        }
        free(entry->dependencies);
        if (entry->metadata) {
            json_free(entry->metadata);
        }
    }

    free(manifest->entries);
    free(manifest->manifest_id);
    free(manifest->created_timestamp);
    free(manifest->updated_timestamp);
    free(manifest->checksum);
    free(manifest);
}

bool asset_manifest_add_entry(AssetManifest *manifest, const char *asset_id,
                             const char *path, const char *file_hash,
                             IntegrityCheckType hash_type, u64 file_size) {
    if (!manifest || !asset_id || !path || !file_hash) {
        return false;
    }

    if (manifest->entry_count >= manifest->entry_capacity) {
        u32 new_capacity = manifest->entry_capacity * 2;
        AssetManifestEntry *new_entries = (AssetManifestEntry *)realloc(
            manifest->entries, sizeof(AssetManifestEntry) * new_capacity);
        if (!new_entries) return false;
        manifest->entries = new_entries;
        manifest->entry_capacity = new_capacity;
    }

    AssetManifestEntry *entry = &manifest->entries[manifest->entry_count++];
    memset(entry, 0, sizeof(AssetManifestEntry));

    entry->asset_id = strdup(asset_id);
    entry->path = strdup(path);
    entry->file_hash = strdup(file_hash);
    entry->hash_type = hash_type;
    entry->file_size = file_size;

    manifest->total_size += file_size;

    return entry->asset_id && entry->path && entry->file_hash;
}

AssetManifestEntry *asset_manifest_find_entry(AssetManifest *manifest, const char *asset_id) {
    if (!manifest || !asset_id) return NULL;

    for (u32 i = 0; i < manifest->entry_count; i++) {
        if (strcmp(manifest->entries[i].asset_id, asset_id) == 0) {
            return &manifest->entries[i];
        }
    }
    return NULL;
}

AssetManifestEntry *asset_manifest_find_entry_by_path(AssetManifest *manifest, const char *path) {
    if (!manifest || !path) return NULL;

    for (u32 i = 0; i < manifest->entry_count; i++) {
        if (strcmp(manifest->entries[i].path, path) == 0) {
            return &manifest->entries[i];
        }
    }
    return NULL;
}

bool asset_manifest_remove_entry(AssetManifest *manifest, const char *asset_id) {
    if (!manifest || !asset_id) return false;

    for (u32 i = 0; i < manifest->entry_count; i++) {
        if (strcmp(manifest->entries[i].asset_id, asset_id) == 0) {
            AssetManifestEntry *entry = &manifest->entries[i];
            manifest->total_size -= entry->file_size;

            free(entry->asset_id);
            free(entry->path);
            free(entry->file_hash);
            free(entry->dependencies);
            if (entry->metadata) json_free(entry->metadata);

            if (i < manifest->entry_count - 1) {
                memmove(entry, &manifest->entries[i + 1],
                       sizeof(AssetManifestEntry) * (manifest->entry_count - i - 1));
            }
            manifest->entry_count--;
            return true;
        }
    }
    return false;
}

bool asset_manifest_update_entry(AssetManifestEntry *entry, const char *file_hash, u64 file_size) {
    if (!entry || !file_hash) return false;

    free(entry->file_hash);
    entry->file_hash = strdup(file_hash);
    entry->file_size = file_size;

    return true;
}

bool asset_manifest_add_dependency(AssetManifestEntry *entry, const char *dependency_id,
                                  const char *version, bool optional) {
    if (!entry || !dependency_id) return false;

    u32 new_count = entry->dependency_count + 1;
    AssetDependency *new_deps = (AssetDependency *)realloc(
        entry->dependencies, sizeof(AssetDependency) * new_count);
    if (!new_deps) return false;

    entry->dependencies = new_deps;
    AssetDependency *dep = &entry->dependencies[entry->dependency_count++];

    dep->asset_id = strdup(dependency_id);
    dep->version = version ? strdup(version) : NULL;
    dep->optional = optional;

    return true;
}

u32 asset_manifest_get_dependencies(AssetManifestEntry *entry, AssetDependency **dependencies_out,
                                    u32 max_dependencies) {
    if (!entry || !dependencies_out) return 0;

    u32 count = entry->dependency_count < max_dependencies ? entry->dependency_count : max_dependencies;
    for (u32 i = 0; i < count; i++) {
        dependencies_out[i] = &entry->dependencies[i];
    }
    return count;
}

ManifestEntryStatus asset_manifest_verify_file(const char *file_path, const char *expected_hash,
                                              IntegrityCheckType hash_type, u64 expected_size) {
    if (!file_path || !expected_hash) {
        return MANIFEST_ENTRY_INVALID_HASH;
    }

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        return MANIFEST_ENTRY_MISSING;
    }

    fseek(file, 0, SEEK_END);
    u64 file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size != expected_size) {
        fclose(file);
        return MANIFEST_ENTRY_WRONG_SIZE;
    }

    if (hash_type == INTEGRITY_CHECK_CRC32) {
        u8 buffer[4096];
        u32 crc = 0xFFFFFFFF;
        size_t read;
        while ((read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            crc = crc32_compute(buffer, (u32)read);
        }
        fclose(file);

        char hash_str[16];
        snprintf(hash_str, sizeof(hash_str), "%08x", crc ^ 0xFFFFFFFF);
        if (strcmp(hash_str, expected_hash) != 0) {
            return MANIFEST_ENTRY_CORRUPTED;
        }
    } else {
        fclose(file);
    }

    return MANIFEST_ENTRY_OK;
}

ManifestEntryStatus asset_manifest_verify_entry(AssetManifestEntry *entry, const char *base_path) {
    if (!entry || !base_path) return MANIFEST_ENTRY_INVALID_HASH;

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, entry->path);

    return asset_manifest_verify_file(full_path, entry->file_hash, entry->hash_type, entry->file_size);
}

const char *asset_manifest_status_string(ManifestEntryStatus status) {
    switch (status) {
        case MANIFEST_ENTRY_OK: return "OK";
        case MANIFEST_ENTRY_MISSING: return "Missing";
        case MANIFEST_ENTRY_CORRUPTED: return "Corrupted";
        case MANIFEST_ENTRY_INVALID_HASH: return "Invalid Hash";
        case MANIFEST_ENTRY_WRONG_SIZE: return "Wrong Size";
        default: return "Unknown";
    }
}

u64 asset_manifest_get_total_size(AssetManifest *manifest) {
    return manifest ? manifest->total_size : 0;
}

u32 asset_manifest_count_entries(AssetManifest *manifest, u32 compression_type) {
    if (!manifest) return 0;

    if (compression_type == 0) {
        return manifest->entry_count;
    }

    u32 count = 0;
    for (u32 i = 0; i < manifest->entry_count; i++) {
        if (manifest->entries[i].compression_type == compression_type) {
            count++;
        }
    }
    return count;
}

void asset_manifest_foreach_entry(AssetManifest *manifest, AssetManifestCallback callback,
                                 void *user_data) {
    if (!manifest || !callback) return;

    for (u32 i = 0; i < manifest->entry_count; i++) {
        callback(&manifest->entries[i], user_data);
    }
}

bool asset_manifest_has_circular_dependencies(AssetManifest *manifest) {
    if (!manifest) return false;

    for (u32 i = 0; i < manifest->entry_count; i++) {
        AssetManifestEntry *entry = &manifest->entries[i];
        for (u32 j = 0; j < entry->dependency_count; j++) {
            AssetManifestEntry *dep = asset_manifest_find_entry(manifest, entry->dependencies[j].asset_id);
            if (dep) {
                for (u32 k = 0; k < dep->dependency_count; k++) {
                    if (strcmp(dep->dependencies[k].asset_id, entry->asset_id) == 0) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

AssetManifest *asset_manifest_from_json(JsonValue *json) {
    if (!json || json->type != JSON_OBJECT) return NULL;

    AssetManifest *manifest = (AssetManifest *)calloc(1, sizeof(AssetManifest));
    if (!manifest) return NULL;

    JsonValue *id_val = json_object_get(json, "manifest_id");
    manifest->manifest_id = id_val ? strdup(id_val->string) : strdup("default");

    JsonValue *entries_val = json_object_get(json, "entries");
    if (entries_val && entries_val->type == JSON_ARRAY) {
        manifest->entry_capacity = entries_val->array.count * 2;
        manifest->entries = (AssetManifestEntry *)calloc(manifest->entry_capacity,
                                                        sizeof(AssetManifestEntry));

        for (u32 i = 0; i < entries_val->array.count; i++) {
            JsonValue *entry_json = entries_val->array.values[i];
            if (entry_json && entry_json->type == JSON_OBJECT) {
                JsonValue *id = json_object_get(entry_json, "asset_id");
                JsonValue *path = json_object_get(entry_json, "path");
                JsonValue *hash = json_object_get(entry_json, "file_hash");
                JsonValue *size = json_object_get(entry_json, "file_size");

                if (id && path && hash && size) {
                    asset_manifest_add_entry(manifest, id->string, path->string,
                                           hash->string, INTEGRITY_CHECK_CRC32,
                                           (u64)size->number);
                }
            }
        }
    }

    return manifest;
}

JsonValue *asset_manifest_to_json(AssetManifest *manifest) {
    if (!manifest) return NULL;

    JsonValue *root = json_create_object();
    if (!root) return NULL;

    json_object_set_string(root, "manifest_id", manifest->manifest_id);
    json_object_set_number(root, "version", manifest->version);

    JsonValue *entries_arr = json_create_array();
    for (u32 i = 0; i < manifest->entry_count; i++) {
        AssetManifestEntry *entry = &manifest->entries[i];
        JsonValue *entry_obj = json_create_object();

        json_object_set_string(entry_obj, "asset_id", entry->asset_id);
        json_object_set_string(entry_obj, "path", entry->path);
        json_object_set_string(entry_obj, "file_hash", entry->file_hash);
        json_object_set_number(entry_obj, "file_size", (double)entry->file_size);

        json_array_push(entries_arr, entry_obj);
    }
    json_object_set_value(root, "entries", entries_arr);

    return root;
}

bool asset_manifest_save_file(AssetManifest *manifest, const char *file_path) {
    if (!manifest || !file_path) return false;

    JsonValue *json = asset_manifest_to_json(manifest);
    if (!json) return false;

    char *json_str = json_serialize(json, NULL);
    if (!json_str) {
        json_free(json);
        return false;
    }

    FILE *file = fopen(file_path, "wb");
    if (!file) {
        free(json_str);
        json_free(json);
        return false;
    }

    fputs(json_str, file);
    fclose(file);
    free(json_str);
    json_free(json);

    return true;
}

AssetManifest *asset_manifest_load_file(const char *file_path) {
    if (!file_path) return NULL;

    FILE *file = fopen(file_path, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *json_str = (char *)malloc(size + 1);
    if (!json_str) {
        fclose(file);
        return NULL;
    }

    fread(json_str, 1, size, file);
    json_str[size] = '\0';
    fclose(file);

    JsonValue *json = json_parse(json_str);
    free(json_str);

    if (!json) return NULL;

    AssetManifest *manifest = asset_manifest_from_json(json);
    json_free(json);

    return manifest;
}

AssetManifest *asset_manifest_copy(AssetManifest *manifest) {
    if (!manifest) return NULL;

    AssetManifest *copy = asset_manifest_create(manifest->manifest_id);
    if (!copy) return NULL;

    for (u32 i = 0; i < manifest->entry_count; i++) {
        AssetManifestEntry *src = &manifest->entries[i];
        asset_manifest_add_entry(copy, src->asset_id, src->path,
                                src->file_hash, src->hash_type, src->file_size);
    }

    return copy;
}

bool asset_manifest_self_test(void) {
    AssetManifest *manifest = asset_manifest_create("test_manifest");
    if (!manifest) {
        return false;
    }

    if (!asset_manifest_add_entry(manifest, "asset_001", "textures/block.png",
                                  "abc12345", INTEGRITY_CHECK_CRC32, 1024)) {
        asset_manifest_free(manifest);
        return false;
    }

    if (!asset_manifest_add_entry(manifest, "asset_002", "models/cube.obj",
                                  "def67890", INTEGRITY_CHECK_CRC32, 2048)) {
        asset_manifest_free(manifest);
        return false;
    }

    AssetManifestEntry *found = asset_manifest_find_entry(manifest, "asset_001");
    if (!found || strcmp(found->asset_id, "asset_001") != 0) {
        asset_manifest_free(manifest);
        return false;
    }

    found = asset_manifest_find_entry_by_path(manifest, "models/cube.obj");
    if (!found || strcmp(found->path, "models/cube.obj") != 0) {
        asset_manifest_free(manifest);
        return false;
    }

    if (!asset_manifest_add_dependency(found, "asset_001", "1.0", false)) {
        asset_manifest_free(manifest);
        return false;
    }

    if (asset_manifest_has_circular_dependencies(manifest)) {
        asset_manifest_free(manifest);
        return false;
    }

    AssetDependency *deps[10];
    u32 dep_count = asset_manifest_get_dependencies(found, deps, 10);
    if (dep_count != 1) {
        asset_manifest_free(manifest);
        return false;
    }

    u64 total_size = asset_manifest_get_total_size(manifest);
    if (total_size != 3072) {
        asset_manifest_free(manifest);
        return false;
    }

    u32 entry_count = asset_manifest_count_entries(manifest, 0);
    if (entry_count != 2) {
        asset_manifest_free(manifest);
        return false;
    }

    AssetManifest *copy = asset_manifest_copy(manifest);
    if (!copy || copy->entry_count != manifest->entry_count) {
        asset_manifest_free(manifest);
        asset_manifest_free(copy);
        return false;
    }

    JsonValue *json = asset_manifest_to_json(manifest);
    if (!json) {
        asset_manifest_free(manifest);
        asset_manifest_free(copy);
        return false;
    }

    AssetManifest *from_json = asset_manifest_from_json(json);
    if (!from_json || from_json->entry_count != manifest->entry_count) {
        json_free(json);
        asset_manifest_free(manifest);
        asset_manifest_free(copy);
        asset_manifest_free(from_json);
        return false;
    }

    asset_manifest_remove_entry(manifest, "asset_001");
    if (manifest->entry_count != 1) {
        json_free(json);
        asset_manifest_free(manifest);
        asset_manifest_free(copy);
        asset_manifest_free(from_json);
        return false;
    }

    json_free(json);
    asset_manifest_free(manifest);
    asset_manifest_free(copy);
    asset_manifest_free(from_json);

    return true;
}
