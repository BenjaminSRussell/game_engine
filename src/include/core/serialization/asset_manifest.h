#pragma once

#include "include/core/types.h"
#include "include/core/serialization/json_parser.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Asset manifest version
#define ASSET_MANIFEST_VERSION 1

// Asset integrity check types
typedef enum {
    INTEGRITY_CHECK_NONE = 0,
    INTEGRITY_CHECK_CRC32,
    INTEGRITY_CHECK_MD5,
    INTEGRITY_CHECK_SHA256
} IntegrityCheckType;

// Manifest entry status
typedef enum {
    MANIFEST_ENTRY_OK = 0,
    MANIFEST_ENTRY_MISSING,
    MANIFEST_ENTRY_CORRUPTED,
    MANIFEST_ENTRY_INVALID_HASH,
    MANIFEST_ENTRY_WRONG_SIZE,
    MANIFEST_ENTRY_UNKNOWN
} ManifestEntryStatus;

// Asset dependency reference
typedef struct {
    char *asset_id;
    char *version;
    bool optional;
} AssetDependency;

// Asset file entry in manifest
typedef struct {
    char *asset_id;
    char *path;
    char *file_hash;
    IntegrityCheckType hash_type;
    u64 file_size;
    u32 compression_type;
    u32 compression_level;
    u32 dependency_count;
    AssetDependency *dependencies;
    JsonValue *metadata;
} AssetManifestEntry;

// Asset manifest structure
typedef struct {
    u32 version;
    char *manifest_id;
    char *created_timestamp;
    char *updated_timestamp;
    u32 entry_count;
    AssetManifestEntry *entries;
    u32 entry_capacity;
    u32 total_size;
    char *checksum;
} AssetManifest;

// Integrity verification result
typedef struct {
    bool all_valid;
    u32 checked_count;
    u32 valid_count;
    u32 corrupted_count;
    u32 missing_count;
    char **corrupted_files;
    char **missing_files;
    u32 max_files;
} IntegrityCheckResult;

// ============================================================================
// Manifest Creation and Management
// ============================================================================

/**
 * Create a new empty asset manifest
 *
 * Args:
 *   manifest_id: Unique identifier for this manifest
 *
 * Returns:
 *   New AssetManifest (must be freed with asset_manifest_free)
 */
AssetManifest *asset_manifest_create(const char *manifest_id);

/**
 * Load manifest from JSON
 *
 * Args:
 *   json: JsonValue containing manifest data
 *
 * Returns:
 *   Loaded AssetManifest or NULL on error
 */
AssetManifest *asset_manifest_from_json(JsonValue *json);

/**
 * Load manifest from file
 *
 * Args:
 *   file_path: Path to manifest file
 *
 * Returns:
 *   Loaded AssetManifest or NULL on error
 */
AssetManifest *asset_manifest_load_file(const char *file_path);

/**
 * Save manifest to file
 *
 * Args:
 *   manifest: AssetManifest to save
 *   file_path: Output file path
 *
 * Returns:
 *   true if saved successfully
 */
bool asset_manifest_save_file(AssetManifest *manifest, const char *file_path);

/**
 * Convert manifest to JSON representation
 *
 * Args:
 *   manifest: AssetManifest to serialize
 *
 * Returns:
 *   JsonValue representing the manifest
 */
JsonValue *asset_manifest_to_json(AssetManifest *manifest);

/**
 * Free an asset manifest and all its entries
 *
 * Args:
 *   manifest: AssetManifest to free (NULL-safe)
 */
void asset_manifest_free(AssetManifest *manifest);

/**
 * Deep copy a manifest
 *
 * Args:
 *   manifest: Manifest to copy
 *
 * Returns:
 *   New manifest with identical data
 */
AssetManifest *asset_manifest_copy(AssetManifest *manifest);

// ============================================================================
// Entry Management
// ============================================================================

/**
 * Add asset entry to manifest
 *
 * Args:
 *   manifest: AssetManifest to modify
 *   asset_id: Unique asset identifier
 *   path: File path
 *   file_hash: Hash of file contents
 *   hash_type: Type of hash used
 *   file_size: Size in bytes
 *
 * Returns:
 *   true if entry added successfully
 */
bool asset_manifest_add_entry(AssetManifest *manifest, const char *asset_id, const char *path, const char *file_hash, IntegrityCheckType hash_type, u64 file_size);

/**
 * Find entry by asset ID
 *
 * Args:
 *   manifest: AssetManifest to search
 *   asset_id: Asset ID to find
 *
 * Returns:
 *   Pointer to AssetManifestEntry or NULL if not found
 */
AssetManifestEntry *asset_manifest_find_entry(AssetManifest *manifest, const char *asset_id);

/**
 * Find entry by file path
 *
 * Args:
 *   manifest: AssetManifest to search
 *   path: File path to find
 *
 * Returns:
 *   Pointer to AssetManifestEntry or NULL if not found
 */
AssetManifestEntry *asset_manifest_find_entry_by_path(AssetManifest *manifest, const char *path);

/**
 * Remove entry from manifest
 *
 * Args:
 *   manifest: AssetManifest to modify
 *   asset_id: Asset ID to remove
 *
 * Returns:
 *   true if entry was found and removed
 */
bool asset_manifest_remove_entry(AssetManifest *manifest, const char *asset_id);

/**
 * Update entry hash and size
 *
 * Args:
 *   entry: AssetManifestEntry to update
 *   file_hash: New file hash
 *   file_size: New file size
 *
 * Returns:
 *   true if updated successfully
 */
bool asset_manifest_update_entry(AssetManifestEntry *entry, const char *file_hash, u64 file_size);

// ============================================================================
// Dependencies
// ============================================================================

/**
 * Add dependency to manifest entry
 *
 * Args:
 *   entry: AssetManifestEntry to modify
 *   dependency_id: ID of dependent asset
 *   version: Version requirement (optional)
 *   optional: Whether dependency is optional
 *
 * Returns:
 *   true if dependency added
 */
bool asset_manifest_add_dependency(AssetManifestEntry *entry, const char *dependency_id, const char *version, bool optional);

/**
 * Get all dependencies for an entry
 *
 * Args:
 *   entry: AssetManifestEntry
 *   dependencies_out: Output array
 *   max_dependencies: Maximum to return
 *
 * Returns:
 *   Number of dependencies returned
 */
u32 asset_manifest_get_dependencies(AssetManifestEntry *entry, AssetDependency **dependencies_out, u32 max_dependencies);

/**
 * Get dependency chain for asset (all transitive dependencies)
 *
 * Args:
 *   manifest: AssetManifest
 *   asset_id: Starting asset ID
 *   chain_out: Output array for dependency chain
 *   max_chain_length: Maximum chain length
 *
 * Returns:
 *   Number of assets in dependency chain
 */
u32 asset_manifest_get_dependency_chain(AssetManifest *manifest, const char *asset_id, AssetManifestEntry **chain_out, u32 max_chain_length);

/**
 * Check for circular dependencies
 *
 * Args:
 *   manifest: AssetManifest
 *
 * Returns:
 *   true if circular dependencies detected
 */
bool asset_manifest_has_circular_dependencies(AssetManifest *manifest);

// ============================================================================
// Integrity Verification
// ============================================================================

/**
 * Verify integrity of a single file
 *
 * Args:
 *   file_path: Path to file to verify
 *   expected_hash: Expected hash value
 *   hash_type: Type of hash
 *   expected_size: Expected file size
 *
 * Returns:
 *   MANIFEST_ENTRY_OK if valid, error status otherwise
 */
ManifestEntryStatus asset_manifest_verify_file(const char *file_path, const char *expected_hash, IntegrityCheckType hash_type, u64 expected_size);

/**
 * Verify all entries in manifest
 *
 * Args:
 *   manifest: AssetManifest to verify
 *   base_path: Base directory path where assets are located
 *   result: Output for verification results
 *
 * Returns:
 *   MANIFEST_ENTRY_OK if all valid, error status otherwise
 */
ManifestEntryStatus asset_manifest_verify_all(AssetManifest *manifest, const char *base_path, IntegrityCheckResult *result);

/**
 * Verify single entry
 *
 * Args:
 *   entry: AssetManifestEntry to verify
 *   base_path: Base directory path
 *
 * Returns:
 *   Status of the entry
 */
ManifestEntryStatus asset_manifest_verify_entry(AssetManifestEntry *entry, const char *base_path);

/**
 * Generate hash for file
 *
 * Args:
 *   file_path: Path to file
 *   hash_type: Type of hash to generate
 *   hash_out: Output buffer for hash string
 *   hash_out_size: Output buffer size
 *
 * Returns:
 *   Number of bytes written to hash_out
 */
u32 asset_manifest_generate_hash(const char *file_path, IntegrityCheckType hash_type, char *hash_out, u32 hash_out_size);

// ============================================================================
// Checksum and Manifest Integrity
// ============================================================================

/**
 * Calculate checksum of entire manifest
 *
 * Args:
 *   manifest: AssetManifest
 *   checksum_out: Output buffer for checksum
 *   checksum_out_size: Output buffer size
 *
 * Returns:
 *   Number of bytes written
 */
u32 asset_manifest_calculate_checksum(AssetManifest *manifest, char *checksum_out, u32 checksum_out_size);

/**
 * Verify manifest integrity
 *
 * Args:
 *   manifest: AssetManifest to verify
 *
 * Returns:
 *   true if manifest checksum is valid
 */
bool asset_manifest_verify_integrity(AssetManifest *manifest);

/**
 * Update manifest checksum
 *
 * Args:
 *   manifest: AssetManifest to update
 *
 * Returns:
 *   true if checksum calculated and updated
 */
bool asset_manifest_update_checksum(AssetManifest *manifest);

// ============================================================================
// Query and Iteration
// ============================================================================

/**
 * Get total uncompressed size of all assets
 *
 * Args:
 *   manifest: AssetManifest
 *
 * Returns:
 *   Total size in bytes
 */
u64 asset_manifest_get_total_size(AssetManifest *manifest);

/**
 * Count entries by type
 *
 * Args:
 *   manifest: AssetManifest
 *   compression_type: Filter by compression type (0 = all)
 *
 * Returns:
 *   Number of matching entries
 */
u32 asset_manifest_count_entries(AssetManifest *manifest, u32 compression_type);

/**
 * Iterate over all entries with callback
 *
 * Args:
 *   manifest: AssetManifest
 *   callback: Function to call for each entry
 *   user_data: Passed to callback
 *
 * Callback signature: void(*)(AssetManifestEntry *entry, void *user_data)
 */
typedef void (*AssetManifestCallback)(AssetManifestEntry *entry, void *user_data);
void asset_manifest_foreach_entry(AssetManifest *manifest, AssetManifestCallback callback, void *user_data);

/**
 * Get entries matching filter
 *
 * Args:
 *   manifest: AssetManifest
 *   pattern: Glob pattern to match asset IDs
 *   results_out: Output array
 *   max_results: Maximum results
 *
 * Returns:
 *   Number of matching entries
 */
u32 asset_manifest_find_by_pattern(AssetManifest *manifest, const char *pattern, AssetManifestEntry **results_out, u32 max_results);

// ============================================================================
// Error Handling
// ============================================================================

/**
 * Get human-readable error message
 *
 * Args:
 *   status: ManifestEntryStatus
 *
 * Returns:
 *   Static string describing the status
 */
const char *asset_manifest_status_string(ManifestEntryStatus status);

/**
 * Create integrity check result structure
 *
 * Returns:
 *   New IntegrityCheckResult
 */
IntegrityCheckResult *asset_manifest_result_create(void);

/**
 * Free integrity check result
 *
 * Args:
 *   result: IntegrityCheckResult to free (NULL-safe)
 */
void asset_manifest_result_free(IntegrityCheckResult *result);

/**
 * Format integrity results as readable string
 *
 * Args:
 *   result: IntegrityCheckResult
 *   out: Output buffer
 *   out_size: Output buffer size
 *
 * Returns:
 *   Number of bytes written
 */
u32 asset_manifest_format_results(IntegrityCheckResult *result, char *out, u32 out_size);

/**
 * Run self-test for asset manifest functionality
 *
 * Tests:
 *   - Entry creation and addition
 *   - Entry search (by ID and by path)
 *   - Dependency management
 *   - Circular dependency detection
 *   - Copy and JSON serialization
 *   - Removal operations
 *
 * Returns:
 *   true if all tests pass
 */
bool asset_manifest_self_test(void);

#ifdef __cplusplus
}
#endif

