#ifndef ASSET_VALIDATOR_H
#define ASSET_VALIDATOR_H

#include "../common.h"
#include "asset_manager.h"
#include "json.h"

typedef struct {
  char asset_id[128];
  char error_message[256];
  bool is_valid;
  u32 error_code;
} ValidationResult;

typedef enum {
  VALIDATION_ERROR_NONE = 0,
  VALIDATION_ERROR_FILE_NOT_FOUND,
  VALIDATION_ERROR_CORRUPTED,
  VALIDATION_ERROR_INVALID_FORMAT,
  VALIDATION_ERROR_MISSING_DEPENDENCY,
  VALIDATION_ERROR_METADATA_MISMATCH,
  VALIDATION_ERROR_CIRCULAR_DEPENDENCY
} ValidationErrorCode;

typedef struct {
  bool check_dependencies;
  bool check_metadata;
  bool check_file_integrity;
  bool check_checksums;
} ValidationOptions;

// Hash function for checksums (FNV-1a)
u32 asset_hash_fnv1a(const void *data, size_t size);

// Validate single asset file
ValidationResult asset_validate_file(const char *asset_path, AssetType type,
                                     const ValidationOptions *options);

// Validate entire manifest
bool asset_validate_manifest(const char *manifest_path,
                             ValidationResult **results, u32 *result_count);

// Check for missing dependencies in manifest
bool asset_check_dependencies(JsonValue *manifest, char ***missing_assets,
                              u32 *missing_count);

// Verify file format magic bytes
bool asset_verify_format(const char *path, AssetType type);

// Free validation results
void asset_validation_results_free(ValidationResult *results, u32 count);

#endif // ASSET_VALIDATOR_H
