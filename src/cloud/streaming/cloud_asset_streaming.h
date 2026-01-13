#ifndef CLOUD_ASSET_STREAMING_H
#define CLOUD_ASSET_STREAMING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char url[256];
    char local_path[256];
    char hash[64];
    uint64_t size;
    uint32_t priority;
    bool is_required;
    bool is_cached;
} CloudAsset;

typedef struct {
    CloudAsset* assets;
    uint32_t asset_count;
    uint32_t max_concurrent_downloads;
    uint32_t active_downloads;
    uint64_t total_bandwidth;
    uint64_t bandwidth_limit;
    char cache_directory[256];
    bool compression_enabled;
    bool encryption_enabled;
} CloudStreamingManager;

typedef struct {
    uint32_t asset_id;
    float progress;
    uint64_t bytes_downloaded;
    uint64_t total_bytes;
    float download_speed;
    bool is_completed;
    bool has_error;
    char error_message[128];
} DownloadStatus;

typedef struct {
    char server_url[256];
    char api_key[128];
    uint32_t timeout_ms;
    uint32_t retry_count;
    bool use_https;
    bool verify_ssl;
} CloudConfig;

// Cloud streaming initialization
CloudStreamingManager* cloud_create_manager(const CloudConfig* config);
bool cloud_destroy_manager(CloudStreamingManager* manager);

// Asset management
bool cloud_add_asset(CloudStreamingManager* manager, const CloudAsset* asset);
bool cloud_remove_asset(CloudStreamingManager* manager, const char* asset_url);
CloudAsset* cloud_find_asset(const CloudStreamingManager* manager, const char* asset_url);
bool cloud_update_asset_priority(CloudStreamingManager* manager, const char* asset_url, uint32_t priority);

// Download management
bool cloud_start_download(CloudStreamingManager* manager, const char* asset_url);
bool cloud_pause_download(CloudStreamingManager* manager, const char* asset_url);
bool cloud_resume_download(CloudStreamingManager* manager, const char* asset_url);
bool cloud_cancel_download(CloudStreamingManager* manager, const char* asset_url);

// Batch operations
bool cloud_start_batch_download(CloudStreamingManager* manager, const char** asset_urls, uint32_t count);
bool cloud_pause_all_downloads(CloudStreamingManager* manager);
bool cloud_resume_all_downloads(CloudStreamingManager* manager);
bool cloud_cancel_all_downloads(CloudStreamingManager* manager);

// Status and monitoring
DownloadStatus cloud_get_download_status(const CloudStreamingManager* manager, const char* asset_url);
uint32_t cloud_get_all_download_statuses(const CloudStreamingManager* manager, DownloadStatus* statuses, uint32_t max_statuses);
bool cloud_is_download_complete(const CloudStreamingManager* manager, const char* asset_url);
float cloud_get_overall_progress(const CloudStreamingManager* manager);

// Cache management
bool cloud_clear_cache(CloudStreamingManager* manager);
bool cloud_clear_expired_cache(CloudStreamingManager* manager, uint32_t max_age_days);
uint64_t cloud_get_cache_size(const CloudStreamingManager* manager);
bool cloud_verify_cache_integrity(CloudStreamingManager* manager);

// Bandwidth management
bool cloud_set_bandwidth_limit(CloudStreamingManager* manager, uint64_t bytes_per_second);
uint64_t cloud_get_current_bandwidth_usage(const CloudStreamingManager* manager);
bool cloud_enable_adaptive_bandwidth(CloudStreamingManager* manager, bool enabled);

// Prefetching and prediction
typedef struct {
    char asset_url[256];
    float probability;
    uint32_t priority;
} AssetPrediction;

bool cloud_enable_predictive_streaming(CloudStreamingManager* manager, bool enabled);
bool cloud_update_predictions(CloudStreamingManager* manager, const AssetPrediction* predictions, uint32_t count);
bool cloud_prefetch_assets(CloudStreamingManager* manager, const char** asset_urls, uint32_t count);

// Compression and encryption
bool cloud_set_compression_algorithm(CloudStreamingManager* manager, const char* algorithm);
bool cloud_set_encryption_key(CloudStreamingManager* manager, const uint8_t* key, uint32_t key_size);
bool cloud_compress_asset(const char* input_path, const char* output_path);
bool cloud_decompress_asset(const char* input_path, const char* output_path);

// Error handling and recovery
typedef enum {
    CLOUD_SUCCESS,
    CLOUD_ERROR_NETWORK,
    CLOUD_ERROR_FILE_NOT_FOUND,
    CLOUD_ERROR_PERMISSION_DENIED,
    CLOUD_ERROR_DISK_FULL,
    CLOUD_ERROR_CORRUPTION,
    CLOUD_ERROR_TIMEOUT,
    CLOUD_ERROR_SERVER_ERROR
} CloudError;

CloudError cloud_get_last_error(const CloudStreamingManager* manager);
const char* cloud_get_error_string(CloudError error);
bool cloud_retry_failed_downloads(CloudStreamingManager* manager);

// Cloud storage integration
typedef struct {
    char provider[32]; // "aws", "azure", "gcp", etc.
    char bucket[128];
    char region[32];
    char access_key[128];
    char secret_key[128];
} CloudStorageConfig;

bool cloud_configure_storage(CloudStreamingManager* manager, const CloudStorageConfig* storage_config);
bool cloud_sync_with_storage(CloudStreamingManager* manager);
bool cloud_upload_to_storage(CloudStreamingManager* manager, const char* local_path, const char* remote_path);

// Statistics and analytics
typedef struct {
    uint32_t total_downloads;
    uint32_t successful_downloads;
    uint32_t failed_downloads;
    uint64_t total_bytes_downloaded;
    float average_download_speed;
    uint32_t cache_hits;
    uint32_t cache_misses;
    float cache_hit_rate;
} CloudStatistics;

CloudStatistics cloud_get_statistics(const CloudStreamingManager* manager);
bool cloud_reset_statistics(CloudStreamingManager* manager);
bool cloud_export_statistics(const CloudStreamingManager* manager, const char* filename);

// Utility functions
bool cloud_is_url_valid(const char* url);
bool cloud_calculate_file_hash(const char* file_path, char* hash_output);
uint64_t cloud_get_file_size(const char* file_path);
bool cloud_create_directory(const char* path);

#ifdef __cplusplus
}
#endif

#endif // CLOUD_ASSET_STREAMING_H
