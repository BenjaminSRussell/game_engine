#include "cloud_asset_streaming.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <curl/curl.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, struct MemoryStruct *userp) {
    size_t realsize = size * nmemb;
    char *ptr = realloc(userp->memory, userp->size + realsize + 1);
    if(!ptr) {
        return 0;
    }
    
    userp->memory = ptr;
    memcpy(&(userp->memory[userp->size]), contents, realsize);
    userp->size += realsize;
    userp->memory[userp->size] = 0;
    
    return realsize;
}

static CloudStreamingManager* g_manager = NULL;
static CloudError g_last_error = CLOUD_SUCCESS;

CloudStreamingManager* cloud_create_manager(const CloudConfig* config) {
    if (!config) return NULL;
    
    CloudStreamingManager* manager = malloc(sizeof(CloudStreamingManager));
    if (!manager) return NULL;
    
    manager->assets = NULL;
    manager->asset_count = 0;
    manager->max_concurrent_downloads = 4;
    manager->active_downloads = 0;
    manager->total_bandwidth = 0;
    manager->bandwidth_limit = config->timeout_ms > 0 ? 1024 * 1024 : 0; // 1MB/s default
    strcpy(manager->cache_directory, "./cloud_cache/");
    manager->compression_enabled = true;
    manager->encryption_enabled = false;
    
    // Create cache directory
    cloud_create_directory(manager->cache_directory);
    
    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    g_manager = manager;
    return manager;
}

bool cloud_destroy_manager(CloudStreamingManager* manager) {
    if (!manager) return false;
    
    // Cancel all active downloads
    cloud_cancel_all_downloads(manager);
    
    // Free assets
    if (manager->assets) {
        for (uint32_t i = 0; i < manager->asset_count; i++) {
            free(manager->assets[i].local_path);
        }
        free(manager->assets);
    }
    
    curl_global_cleanup();
    free(manager);
    g_manager = NULL;
    return true;
}

bool cloud_add_asset(CloudStreamingManager* manager, const CloudAsset* asset) {
    if (!manager || !asset) return false;
    
    CloudAsset* new_assets = realloc(manager->assets, (manager->asset_count + 1) * sizeof(CloudAsset));
    if (!new_assets) return false;
    
    manager->assets = new_assets;
    manager->assets[manager->asset_count] = *asset;
    
    // Allocate and copy local path
    manager->assets[manager->asset_count].local_path = malloc(256);
    if (!manager->assets[manager->asset_count].local_path) return false;
    
    sprintf(manager->assets[manager->asset_count].local_path, "%s/%s", 
            manager->cache_directory, strrchr(asset->url, '/') + 1);
    
    manager->asset_count++;
    return true;
}

bool cloud_start_download(CloudStreamingManager* manager, const char* asset_url) {
    if (!manager || !asset_url || manager->active_downloads >= manager->max_concurrent_downloads) {
        g_last_error = CLOUD_ERROR_NETWORK;
        return false;
    }
    
    CloudAsset* asset = cloud_find_asset(manager, asset_url);
    if (!asset) {
        g_last_error = CLOUD_ERROR_FILE_NOT_FOUND;
        return false;
    }
    
    // Check if already cached
    struct stat st;
    if (stat(asset->local_path, &st) == 0) {
        asset->is_cached = true;
        return true;
    }
    
    // Perform download
    CURL *curl;
    CURLcode res;
    FILE *fp;
    struct MemoryStruct chunk;
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    curl = curl_easy_init();
    if (!curl) {
        free(chunk.memory);
        g_last_error = CLOUD_ERROR_NETWORK;
        return false;
    }
    
    fp = fopen(asset->local_path, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        free(chunk.memory);
        g_last_error = CLOUD_ERROR_PERMISSION_DENIED;
        return false;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, asset_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CloudAssetStreamer/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fclose(fp);
        remove(asset->local_path);
        curl_easy_cleanup(curl);
        free(chunk.memory);
        g_last_error = CLOUD_ERROR_NETWORK;
        return false;
    }
    
    // Write downloaded data to file
    fwrite(chunk.memory, 1, chunk.size, fp);
    fclose(fp);
    
    curl_easy_cleanup(curl);
    free(chunk.memory);
    
    asset->is_cached = true;
    manager->active_downloads--;
    
    g_last_error = CLOUD_SUCCESS;
    return true;
}

CloudAsset* cloud_find_asset(const CloudStreamingManager* manager, const char* asset_url) {
    if (!manager || !asset_url) return NULL;
    
    for (uint32_t i = 0; i < manager->asset_count; i++) {
        if (strcmp(manager->assets[i].url, asset_url) == 0) {
            return (CloudAsset*)&manager->assets[i];
        }
    }
    
    return NULL;
}

DownloadStatus cloud_get_download_status(const CloudStreamingManager* manager, const char* asset_url) {
    DownloadStatus status = {0};
    
    if (!manager || !asset_url) {
        status.has_error = true;
        strcpy(status.error_message, "Invalid parameters");
        return status;
    }
    
    CloudAsset* asset = cloud_find_asset(manager, asset_url);
    if (!asset) {
        status.has_error = true;
        strcpy(status.error_message, "Asset not found");
        return status;
    }
    
    status.asset_id = 0; // Would be actual ID in real implementation
    status.is_completed = asset->is_cached;
    status.has_error = false;
    status.bytes_downloaded = asset->is_cached ? asset->size : 0;
    status.total_bytes = asset->size;
    status.progress = asset->is_cached ? 1.0f : 0.0f;
    status.download_speed = 0.0f; // Would calculate in real implementation
    
    return status;
}

bool cloud_start_batch_download(CloudStreamingManager* manager, const char** asset_urls, uint32_t count) {
    if (!manager || !asset_urls || count == 0) return false;
    
    bool all_success = true;
    
    for (uint32_t i = 0; i < count; i++) {
        if (!cloud_start_download(manager, asset_urls[i])) {
            all_success = false;
        }
    }
    
    return all_success;
}

bool cloud_cancel_all_downloads(CloudStreamingManager* manager) {
    if (!manager) return false;
    
    manager->active_downloads = 0;
    return true;
}

float cloud_get_overall_progress(const CloudStreamingManager* manager) {
    if (!manager || manager->asset_count == 0) return 0.0f;
    
    uint32_t completed = 0;
    for (uint32_t i = 0; i < manager->asset_count; i++) {
        if (manager->assets[i].is_cached) {
            completed++;
        }
    }
    
    return (float)completed / (float)manager->asset_count;
}

bool cloud_clear_cache(CloudStreamingManager* manager) {
    if (!manager) return false;
    
    for (uint32_t i = 0; i < manager->asset_count; i++) {
        remove(manager->assets[i].local_path);
        manager->assets[i].is_cached = false;
    }
    
    return true;
}

uint64_t cloud_get_cache_size(const CloudStreamingManager* manager) {
    if (!manager) return 0;
    
    uint64_t total_size = 0;
    struct stat st;
    
    for (uint32_t i = 0; i < manager->asset_count; i++) {
        if (stat(manager->assets[i].local_path, &st) == 0) {
            total_size += st.st_size;
        }
    }
    
    return total_size;
}

bool cloud_set_bandwidth_limit(CloudStreamingManager* manager, uint64_t bytes_per_second) {
    if (!manager) return false;
    
    manager->bandwidth_limit = bytes_per_second;
    return true;
}

CloudError cloud_get_last_error(const CloudStreamingManager* manager) {
    return g_last_error;
}

const char* cloud_get_error_string(CloudError error) {
    switch (error) {
        case CLOUD_SUCCESS: return "Success";
        case CLOUD_ERROR_NETWORK: return "Network error";
        case CLOUD_ERROR_FILE_NOT_FOUND: return "File not found";
        case CLOUD_ERROR_PERMISSION_DENIED: return "Permission denied";
        case CLOUD_ERROR_DISK_FULL: return "Disk full";
        case CLOUD_ERROR_CORRUPTION: return "File corruption";
        case CLOUD_ERROR_TIMEOUT: return "Timeout";
        case CLOUD_ERROR_SERVER_ERROR: return "Server error";
        default: return "Unknown error";
    }
}

CloudStatistics cloud_get_statistics(const CloudStreamingManager* manager) {
    CloudStatistics stats = {0};
    
    if (!manager) return stats;
    
    stats.total_downloads = manager->asset_count;
    stats.cache_hits = 0;
    stats.cache_misses = 0;
    
    for (uint32_t i = 0; i < manager->asset_count; i++) {
        if (manager->assets[i].is_cached) {
            stats.cache_hits++;
            stats.successful_downloads++;
        } else {
            stats.cache_misses++;
        }
    }
    
    stats.cache_hit_rate = (float)stats.cache_hits / (float)stats.total_downloads;
    
    return stats;
}

bool cloud_create_directory(const char* path) {
    #ifdef _WIN32
        return mkdir(path) == 0;
    #else
        return mkdir(path, 0755) == 0;
    #endif
}

bool cloud_is_url_valid(const char* url) {
    if (!url) return false;
    
    return (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0);
}

uint64_t cloud_get_file_size(const char* file_path) {
    struct stat st;
    if (stat(file_path, &st) == 0) {
        return st.st_size;
    }
    return 0;
}
