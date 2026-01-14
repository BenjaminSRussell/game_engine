#include <core/asset_validator.h>
#include "engine/include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FNV-1a hash implementation
u32 asset_hash_fnv1a(const void *data, size_t size) {
  const u8 *bytes = (const u8 *)data;
  u32 hash = 0x811c9dc5; // FNV offset basis

  for (size_t i = 0; i < size; i++) {
    hash ^= bytes[i];
    hash *= 0x01000193; // FNV prime
  }

  return hash;
}

// Magic byte signatures for common formats
static const u8 PNG_MAGIC[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
static const u8 JPEG_MAGIC[] = {0xFF, 0xD8, 0xFF};
static const char GLTF_MAGIC[] = "glTF";
static const u8 GLB_MAGIC[] = {0x67, 0x6C, 0x54, 0x46}; // "glTF" in binary

bool asset_verify_format(const char *path, AssetType type) {
  FILE *file = fopen(path, "rb");
  if (!file)
    return false;

  u8 header[16];
  size_t read = fread(header, 1, sizeof(header), file);
  fclose(file);

  if (read < 4)
    return false;

  switch (type) {
  case ASSET_TYPE_TEXTURE:
    // Check for PNG
    if (read >= 8 && memcmp(header, PNG_MAGIC, 8) == 0)
      return true;
    // Check for JPEG
    if (read >= 3 && memcmp(header, JPEG_MAGIC, 3) == 0)
      return true;
    // Could add more formats (TGA, DDS, etc.)
    return false;

  case ASSET_TYPE_MODEL:
    // Check for GLB (binary GLTF)
    if (read >= 4 && memcmp(header, GLB_MAGIC, 4) == 0)
      return true;
    // GLTF JSON starts with '{'
    if (header[0] == '{')
      return true;
    // OBJ starts with 'v' or '#'
    if (header[0] == 'v' || header[0] == '#')
      return true;
    return false;

  case ASSET_TYPE_AUDIO:
    // WAV: "RIFF"
    if (read >= 4 && memcmp(header, "RIFF", 4) == 0)
      return true;
    // MP3: ID3 or 0xFF 0xFB
    if (read >= 3 && (memcmp(header, "ID3", 3) == 0 ||
                      (header[0] == 0xFF && (header[1] & 0xE0) == 0xE0)))
      return true;
    return false;

  default:
    return true; // Unknown type, assume valid
  }
}

ValidationResult asset_validate_file(const char *asset_path, AssetType type,
                                     const ValidationOptions *options) {
  ValidationResult result = {0};
  strncpy(result.asset_id, asset_path, 127);
  result.is_valid = true;
  result.error_code = VALIDATION_ERROR_NONE;

  // Check file exists
  FILE *file = fopen(asset_path, "rb");
  if (!file) {
    result.is_valid = false;
    result.error_code = VALIDATION_ERROR_FILE_NOT_FOUND;
    snprintf(result.error_message, 255, "File not found: %s", asset_path);
    return result;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (file_size <= 0) {
    fclose(file);
    result.is_valid = false;
    result.error_code = VALIDATION_ERROR_CORRUPTED;
    snprintf(result.error_message, 255, "File is empty or corrupted: %s",
             asset_path);
    return result;
  }

  // Check file format if requested
  if (options && options->check_file_integrity) {
    if (!asset_verify_format(asset_path, type)) {
      fclose(file);
      result.is_valid = false;
      result.error_code = VALIDATION_ERROR_INVALID_FORMAT;
      snprintf(result.error_message, 255,
               "Invalid file format for asset type: %s", asset_path);
      return result;
    }
  }

  // Checksum validation if requested
  if (options && options->check_checksums) {
    void *data = malloc(file_size);
    if (data) {
      fread(data, 1, file_size, file);
      u32 hash = asset_hash_fnv1a(data, file_size);
      free(data);

      // Store hash in result for future comparison
      // (In a real system, you'd compare against stored checksums)
      LOG_DEBUG("Asset checksum for %s: 0x%08X", asset_path, hash);
    }
  }

  fclose(file);
  return result;
}

bool asset_check_dependencies(JsonValue *manifest, char ***missing_assets,
                              u32 *missing_count) {
  if (!manifest || manifest->type != JSON_OBJECT)
    return false;

  JsonValue *assets_array = json_object_get(manifest, "assets");
  if (!assets_array || assets_array->type != JSON_ARRAY)
    return false;

  // Build set of available asset IDs
  char **available_ids = malloc(sizeof(char *) * assets_array->array.count);
  u32 available_count = 0;

  for (u32 i = 0; i < assets_array->array.count; i++) {
    JsonValue *asset = assets_array->array.values[i];
    if (asset->type != JSON_OBJECT)
      continue;

    JsonValue *id_val = json_object_get(asset, "id");
    if (id_val && id_val->type == JSON_STRING) {
      available_ids[available_count++] = id_val->string;
    }
  }

  // Check dependencies
  u32 missing_cap = 16;
  *missing_assets = malloc(sizeof(char *) * missing_cap);
  *missing_count = 0;

  for (u32 i = 0; i < assets_array->array.count; i++) {
    JsonValue *asset = assets_array->array.values[i];
    if (asset->type != JSON_OBJECT)
      continue;

    JsonValue *deps = json_object_get(asset, "dependencies");
    if (!deps || deps->type != JSON_ARRAY)
      continue;

    for (u32 j = 0; j < deps->array.count; j++) {
      JsonValue *dep = deps->array.values[j];
      if (dep->type != JSON_STRING)
        continue;

      const char *dep_id = dep->string;
      bool found = false;

      for (u32 k = 0; k < available_count; k++) {
        if (strcmp(available_ids[k], dep_id) == 0) {
          found = true;
          break;
        }
      }

      if (!found) {
        if (*missing_count >= missing_cap) {
          missing_cap *= 2;
          *missing_assets =
              realloc(*missing_assets, sizeof(char *) * missing_cap);
        }
        (*missing_assets)[*missing_count] = strdup(dep_id);
        (*missing_count)++;
      }
    }
  }

  free(available_ids);
  return *missing_count == 0;
}

bool asset_validate_manifest(const char *manifest_path,
                             ValidationResult **results, u32 *result_count) {
  FILE *file = fopen(manifest_path, "rb");
  if (!file) {
    LOG_ERROR("Failed to open manifest: %s", manifest_path);
    return false;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *json_text = malloc(size + 1);
  fread(json_text, 1, size, file);
  json_text[size] = '\0';
  fclose(file);

  JsonValue *root = json_parse(json_text);
  free(json_text);

  if (!root || root->type != JSON_OBJECT) {
    LOG_ERROR("Invalid manifest JSON");
    return false;
  }

  JsonValue *assets_array = json_object_get(root, "assets");
  if (!assets_array || assets_array->type != JSON_ARRAY) {
    json_free(root);
    return false;
  }

  *result_count = assets_array->array.count;
  *results = malloc(sizeof(ValidationResult) * (*result_count));

  ValidationOptions options = {.check_file_integrity = true,
                               .check_checksums = false,
                               .check_dependencies = false,
                               .check_metadata = false};

  bool all_valid = true;
  for (u32 i = 0; i < assets_array->array.count; i++) {
    JsonValue *asset = assets_array->array.values[i];
    if (asset->type != JSON_OBJECT)
      continue;

    JsonValue *path_val = json_object_get(asset, "path");
    JsonValue *type_val = json_object_get(asset, "type");

    if (!path_val || !type_val)
      continue;

    const char *path = json_string_value(path_val);
    const char *type_str = json_string_value(type_val);

    AssetType type = ASSET_TYPE_UNKNOWN;
    if (strcmp(type_str, "texture") == 0)
      type = ASSET_TYPE_TEXTURE;
    else if (strcmp(type_str, "model") == 0)
      type = ASSET_TYPE_MODEL;
    else if (strcmp(type_str, "audio") == 0)
      type = ASSET_TYPE_AUDIO;

    (*results)[i] = asset_validate_file(path, type, &options);
    if (!(*results)[i].is_valid)
      all_valid = false;
  }

  json_free(root);
  return all_valid;
}

void asset_validation_results_free(ValidationResult *results, u32 count) {
  if (results)
    free(results);
}
