// Content Sharing System implementation
#include "network/content_sharing_system.h"
#include <core/logger.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <math/math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <curl/curl.h>
#include <openssl/sha.h>

// MARK: - Helper Functions

static u64 get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static void generate_content_id(char* id, size_t size) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < size - 1; i++) {
        id[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    id[size - 1] = '\0';
}

static void calculate_file_checksum(const char* file_path, char* checksum, size_t size) {
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        strcpy(checksum, "0000000000000000000000000000000000000000000000000000000000000000");
        return;
    }
    
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        SHA256_Update(&sha256, buffer, bytes_read);
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);
    
    fclose(file);
    
    // Convert to hex string
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(checksum + (i * 2), "%02x", hash[i]);
    }
    checksum[64] = '\0';
}

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    char* response = (char*)userp;
    strncat(response, (char*)contents, realsize);
    return realsize;
}

static bool make_http_request(const char* url, const char* method, const char* data, char* response, size_t response_size) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize CURL");
        return false;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
    
    if (data) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    }
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    
    response[0] = '\0';
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        LOG_ERROR("HTTP request failed: %s", curl_easy_strerror(res));
        return false;
    }
    
    return true;
}

// MARK: - Content Sharing System Management

bool content_sharing_init(ContentSharingSystem* system, const char* api_endpoint, u32 max_content, u32 max_users) {
    if (!system || !api_endpoint || max_content == 0 || max_users == 0) {
        LOG_ERROR("Invalid parameters for content_sharing_init");
        return false;
    }
    
    memset(system, 0, sizeof(ContentSharingSystem));
    
    // Copy API endpoint
    strncpy(system->api_endpoint, api_endpoint, sizeof(system->api_endpoint) - 1);
    system->api_endpoint[sizeof(system->api_endpoint) - 1] = '\0';
    
    // Allocate content arrays
    system->local_content = (ContentPackage*)calloc(max_content, sizeof(ContentPackage));
    system->remote_content = (ContentMetadata*)calloc(max_content, sizeof(ContentMetadata));
    if (!system->local_content || !system->remote_content) {
        LOG_ERROR("Failed to allocate content arrays");
        return false;
    }
    system->local_content_capacity = max_content;
    system->remote_content_capacity = max_content;
    
    // Allocate user profiles array
    system->user_profiles = (UserProfile*)calloc(max_users, sizeof(UserProfile));
    if (!system->user_profiles) {
        free(system->local_content);
        free(system->remote_content);
        LOG_ERROR("Failed to allocate user profiles array");
        return false;
    }
    system->user_profile_capacity = max_users;
    
    // Allocate comments and ratings arrays
    system->comments = (Comment*)calloc(max_content * 10, sizeof(Comment)); // Assume 10 comments per content
    system->ratings = (Rating*)calloc(max_content * 5, sizeof(Rating)); // Assume 5 ratings per content
    if (!system->comments || !system->ratings) {
        free(system->local_content);
        free(system->remote_content);
        free(system->user_profiles);
        LOG_ERROR("Failed to allocate comments/ratings arrays");
        return false;
    }
    system->comment_capacity = max_content * 10;
    system->rating_capacity = max_content * 5;
    
    // Set default configuration
    system->sync_interval_ms = 300000; // 5 minutes
    system->is_online = true;
    system->auto_sync = true;
    
    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    LOG_INFO("Content sharing system initialized (content: %u, users: %u)", max_content, max_users);
    return true;
}

void content_sharing_shutdown(ContentSharingSystem* system) {
    if (!system) {
        return;
    }
    
    // Free content packages
    if (system->local_content) {
        for (u32 i = 0; i < system->local_content_count; i++) {
            if (system->local_content[i].package_data) {
                free(system->local_content[i].package_data);
            }
            if (system->local_content[i].additional_files) {
                for (u32 j = 0; j < system->local_content[i].additional_file_count; j++) {
                    free(system->local_content[i].additional_files[j]);
                }
                free(system->local_content[i].additional_files);
            }
        }
        free(system->local_content);
    }
    
    // Free arrays
    free(system->remote_content);
    free(system->user_profiles);
    free(system->comments);
    free(system->ratings);
    
    // Cleanup CURL
    curl_global_cleanup();
    
    memset(system, 0, sizeof(ContentSharingSystem));
    LOG_INFO("Content sharing system shutdown");
}

bool content_sharing_login(ContentSharingSystem* system, const char* username, const char* password) {
    if (!system || !username || !password) {
        return false;
    }
    
    // Create login request
    char request_data[512];
    snprintf(request_data, sizeof(request_data), 
             "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
    
    char response[4096];
    char url[512];
    snprintf(url, sizeof(url), "%s/api/auth/login", system->api_endpoint);
    
    if (!make_http_request(url, "POST", request_data, response, sizeof(response))) {
        LOG_ERROR("Login request failed");
        return false;
    }
    
    // TODO: Parse JSON response and extract user data
    // For now, simulate successful login
    
    strncpy(system->current_user.username, username, sizeof(system->current_user.username) - 1);
    strncpy(system->current_user.display_name, username, sizeof(system->current_user.display_name) - 1);
    system->current_user.joined_time_ms = get_current_time_ms();
    system->current_user.last_active_time_ms = get_current_time_ms();
    
    system->is_logged_in = true;
    
    LOG_INFO("User logged in: %s", username);
    return true;
}

void content_sharing_logout(ContentSharingSystem* system) {
    if (!system) {
        return;
    }
    
    system->is_logged_in = false;
    memset(&system->current_user, 0, sizeof(UserProfile));
    
    LOG_INFO("User logged out");
}

void content_sharing_update(ContentSharingSystem* system) {
    if (!system) {
        return;
    }
    
    u64 current_time = get_current_time_ms();
    
    // Auto-sync if enabled
    if (system->auto_sync && system->is_online) {
        if (current_time - system->last_sync_time_ms >= system->sync_interval_ms) {
            content_sharing_sync_content(system);
            system->last_sync_time_ms = current_time;
        }
    }
    
    // Update user last active time
    if (system->is_logged_in) {
        system->current_user.last_active_time_ms = current_time;
    }
}

// MARK: - Content Management

ContentPackage* content_sharing_create_package(ContentSharingSystem* system, ContentType type, const char* title, 
                                              const char* description, const char* file_path) {
    if (!system || !title || !file_path) {
        return NULL;
    }
    
    if (system->local_content_count >= system->local_content_capacity) {
        LOG_ERROR("Local content capacity exceeded");
        return NULL;
    }
    
    ContentPackage* package = &system->local_content[system->local_content_count];
    memset(package, 0, sizeof(ContentPackage));
    
    // Generate content ID
    generate_content_id(package->metadata.id, sizeof(package->metadata.id));
    
    // Set metadata
    strncpy(package->metadata.title, title, sizeof(package->metadata.title) - 1);
    package->metadata.title[sizeof(package->metadata.title) - 1] = '\0';
    
    if (description) {
        strncpy(package->metadata.description, description, sizeof(package->metadata.description) - 1);
        package->metadata.description[sizeof(package->metadata.description) - 1] = '\0';
    }
    
    strncpy(package->metadata.file_path, file_path, sizeof(package->metadata.file_path) - 1);
    package->metadata.file_path[sizeof(package->metadata.file_path) - 1] = '\0';
    
    package->metadata.type = type;
    package->metadata.status = SHARE_STATUS_PRIVATE;
    package->metadata.created_time_ms = get_current_time_ms();
    package->metadata.updated_time_ms = package->metadata.created_time_ms;
    
    // Set author to current user
    if (system->is_logged_in) {
        strncpy(package->metadata.author, system->current_user.username, sizeof(package->metadata.author) - 1);
        package->metadata.author[sizeof(package->metadata.author) - 1] = '\0';
    }
    
    // Calculate file checksum
    calculate_file_checksum(file_path, package->metadata.checksum, sizeof(package->metadata.checksum));
    
    // Get file size
    FILE* file = fopen(file_path, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        package->metadata.file_size = ftell(file);
        fclose(file);
    }
    
    system->local_content_count++;
    LOG_DEBUG("Created content package: %s", title);
    return package;
}

bool content_sharing_upload_content(ContentSharingSystem* system, const ContentPackage* package) {
    if (!system || !package || !system->is_logged_in) {
        return false;
    }
    
    // Create upload request
    char request_data[8192];
    snprintf(request_data, sizeof(request_data),
             "{\"id\":\"%s\",\"title\":\"%s\",\"description\":\"%s\",\"type\":%u,\"author\":\"%s\","
             "\"file_size\":%llu,\"checksum\":\"%s\",\"tags\":[],\"category\":\"\"}",
             package->metadata.id, package->metadata.title, package->metadata.description,
             package->metadata.type, package->metadata.author, package->metadata.file_size,
             package->metadata.checksum);
    
    char response[4096];
    char url[512];
    snprintf(url, sizeof(url), "%s/api/content/upload", system->api_endpoint);
    
    if (!make_http_request(url, "POST", request_data, response, sizeof(response))) {
        LOG_ERROR("Upload request failed");
        if (system->on_content_uploaded) {
            system->on_content_uploaded(package->metadata.id, false);
        }
        return false;
    }
    
    // TODO: Upload actual file data
    // For now, simulate successful upload
    
    package->metadata.status = SHARE_STATUS_PUBLIC;
    package->metadata.published_time_ms = get_current_time_ms();
    
    LOG_INFO("Content uploaded: %s", package->metadata.title);
    
    if (system->on_content_uploaded) {
        system->on_content_uploaded(package->metadata.id, true);
    }
    
    return true;
}

bool content_sharing_download_content(ContentSharingSystem* system, const char* content_id, const char* download_path) {
    if (!system || !content_id || !download_path) {
        return false;
    }
    
    // Create download request
    char request_data[256];
    snprintf(request_data, sizeof(request_data), "{\"content_id\":\"%s\"}", content_id);
    
    char response[4096];
    char url[512];
    snprintf(url, sizeof(url), "%s/api/content/download", system->api_endpoint);
    
    if (!make_http_request(url, "POST", request_data, response, sizeof(response))) {
        LOG_ERROR("Download request failed");
        if (system->on_content_downloaded) {
            system->on_content_downloaded(content_id, false);
        }
        return false;
    }
    
    // TODO: Download actual file data
    // For now, simulate successful download
    
    LOG_INFO("Content downloaded: %s", content_id);
    
    if (system->on_content_downloaded) {
        system->on_content_downloaded(content_id, true);
    }
    
    return true;
}

SearchResults* content_sharing_search(ContentSharingSystem* system, const SearchFilters* filters) {
    if (!system || !filters) {
        return NULL;
    }
    
    // Create search request
    char request_data[1024];
    snprintf(request_data, sizeof(request_data),
             "{\"type_filter\":%u,\"status_filter\":%u,\"search_query\":\"%s\","
             "\"category_filter\":\"%s\",\"author_filter\":\"%s\",\"sort_by\":%u,\"page\":%u}",
             filters->type_filter, filters->status_filter, filters->search_query,
             filters->category_filter, filters->author_filter, filters->sort_by, filters->page);
    
    char response[8192];
    char url[512];
    snprintf(url, sizeof(url), "%s/api/content/search", system->api_endpoint);
    
    if (!make_http_request(url, "POST", request_data, response, sizeof(response))) {
        LOG_ERROR("Search request failed");
        return NULL;
    }
    
    // TODO: Parse JSON response and create search results
    // For now, create mock results
    
    SearchResults* results = (SearchResults*)calloc(1, sizeof(SearchResults));
    if (!results) {
        return NULL;
    }
    
    results->results = (ContentMetadata*)calloc(10, sizeof(ContentMetadata)); // Mock 10 results
    results->result_count = 10;
    results->total_results = 100;
    results->page = filters->page;
    results->results_per_page = 10;
    
    // Generate mock results
    for (u32 i = 0; i < results->result_count; i++) {
        generate_content_id(results->results[i].id, sizeof(results->results[i].id));
        snprintf(results->results[i].title, sizeof(results->results[i].title), "Content %u", i + 1);
        snprintf(results->results[i].description, sizeof(results->results[i].description), 
                "Description for content %u", i + 1);
        results->results[i].type = filters->type_filter;
        results->results[i].status = SHARE_STATUS_PUBLIC;
        results->results[i].download_count = rand() % 1000;
        results->results[i].like_count = rand() % 100;
        results->results[i].created_time_ms = get_current_time_ms() - (rand() % 86400000); // Within last 24 hours
    }
    
    LOG_DEBUG("Search completed: %u results", results->result_count);
    
    if (system->on_search_completed) {
        system->on_search_completed(results);
    }
    
    return results;
}

// MARK: - Rating and Review System

bool content_sharing_rate_content(ContentSharingSystem* system, const char* content_id, u32 rating, const char* review_text) {
    if (!system || !content_id || rating < 1 || rating > 5 || !system->is_logged_in) {
        return false;
    }
    
    if (system->rating_count >= system->rating_capacity) {
        LOG_ERROR("Rating capacity exceeded");
        return false;
    }
    
    Rating* new_rating = &system->ratings[system->rating_count++];
    memset(new_rating, 0, sizeof(Rating));
    
    // Generate rating ID
    generate_content_id(new_rating->rating_id, sizeof(new_rating->rating_id));
    
    // Set rating data
    strncpy(new_rating->content_id, content_id, sizeof(new_rating->content_id) - 1);
    strncpy(new_rating->user_id, system->current_user.user_id, sizeof(new_rating->user_id) - 1);
    new_rating->rating = rating;
    
    if (review_text) {
        strncpy(new_rating->review_text, review_text, sizeof(new_rating->review_text) - 1);
        new_rating->review_text[sizeof(new_rating->review_text) - 1] = '\0';
    }
    
    new_rating->created_time_ms = get_current_time_ms();
    new_rating->is_verified_purchase = true; // Assume verified
    
    // Create rating request
    char request_data[1024];
    snprintf(request_data, sizeof(request_data),
             "{\"content_id\":\"%s\",\"rating\":%u,\"review_text\":\"%s\"}",
             content_id, rating, review_text ? review_text : "");
    
    char response[512];
    char url[512];
    snprintf(url, sizeof(url), "%s/api/content/rate", system->api_endpoint);
    
    if (!make_http_request(url, "POST", request_data, response, sizeof(response))) {
        LOG_ERROR("Rating request failed");
        if (system->on_rating_submitted) {
            system->on_rating_submitted(new_rating->rating_id, false);
        }
        return false;
    }
    
    LOG_INFO("Content rated: %s (%u stars)", content_id, rating);
    
    if (system->on_rating_submitted) {
        system->on_rating_submitted(new_rating->rating_id, true);
    }
    
    return true;
}

// MARK: - Comment System

bool content_sharing_post_comment(ContentSharingSystem* system, const char* content_id, const char* text, const char* parent_id) {
    if (!system || !content_id || !text || !system->is_logged_in) {
        return false;
    }
    
    if (system->comment_count >= system->comment_capacity) {
        LOG_ERROR("Comment capacity exceeded");
        return false;
    }
    
    Comment* new_comment = &system->comments[system->comment_count++];
    memset(new_comment, 0, sizeof(Comment));
    
    // Generate comment ID
    generate_content_id(new_comment->comment_id, sizeof(new_comment->comment_id));
    
    // Set comment data
    strncpy(new_comment->content_id, content_id, sizeof(new_comment->content_id) - 1);
    strncpy(new_comment->author_id, system->current_user.user_id, sizeof(new_comment->author_id) - 1);
    strncpy(new_comment->author_name, system->current_user.username, sizeof(new_comment->author_name) - 1);
    strncpy(new_comment->text, text, sizeof(new_comment->text) - 1);
    new_comment->text[sizeof(new_comment->text) - 1] = '\0';
    
    if (parent_id) {
        strncpy(new_comment->parent_comment_id, parent_id, sizeof(new_comment->parent_comment_id) - 1);
        new_comment->parent_comment_id[sizeof(new_comment->parent_comment_id) - 1] = '\0';
    }
    
    new_comment->created_time_ms = get_current_time_ms();
    
    // Create comment request
    char request_data[2048];
    snprintf(request_data, sizeof(request_data),
             "{\"content_id\":\"%s\",\"text\":\"%s\",\"parent_comment_id\":\"%s\"}",
             content_id, text, parent_id ? parent_id : "");
    
    char response[512];
    char url[512];
    snprintf(url, sizeof(url), "%s/api/content/comment", system->api_endpoint);
    
    if (!make_http_request(url, "POST", request_data, response, sizeof(response))) {
        LOG_ERROR("Comment request failed");
        if (system->on_comment_posted) {
            system->on_comment_posted(new_comment->comment_id, false);
        }
        return false;
    }
    
    LOG_INFO("Comment posted: %s", new_comment->comment_id);
    
    if (system->on_comment_posted) {
        system->on_comment_posted(new_comment->comment_id, true);
    }
    
    return true;
}

// MARK: - Utility Functions

void content_sharing_set_callbacks(ContentSharingSystem* system, 
                                   void (*on_upload)(const char*, bool),
                                   void (*on_download)(const char*, bool),
                                   void (*on_search)(const SearchResults*),
                                   void (*on_comment)(const char*, bool),
                                   void (*on_rating)(const char*, bool)) {
    if (!system) {
        return;
    }
    
    system->on_content_uploaded = on_upload;
    system->on_content_downloaded = on_download;
    system->on_search_completed = on_search;
    system->on_comment_posted = on_comment;
    system->on_rating_submitted = on_rating;
    
    LOG_INFO("Content sharing callbacks set");
}

void content_sharing_enable_auto_sync(ContentSharingSystem* system, bool enabled, u32 interval_ms) {
    if (!system) {
        return;
    }
    
    system->auto_sync = enabled;
    system->sync_interval_ms = interval_ms;
    
    LOG_INFO("Auto sync %s (interval: %u ms)", enabled ? "enabled" : "disabled", interval_ms);
}

bool content_sharing_sync_content(ContentSharingSystem* system) {
    if (!system || !system->is_online) {
        return false;
    }
    
    // TODO: Implement actual synchronization
    // For now, just log the sync attempt
    LOG_DEBUG("Syncing content with server");
    
    return true;
}

bool content_sharing_validate_package(const ContentPackage* package) {
    if (!package) {
        return false;
    }
    
    // Check required fields
    if (strlen(package->metadata.title) == 0 || strlen(package->metadata.author) == 0) {
        return false;
    }
    
    // Check file exists
    FILE* file = fopen(package->metadata.file_path, "rb");
    if (!file) {
        return false;
    }
    fclose(file);
    
    // Validate checksum
    char calculated_checksum[65];
    calculate_file_checksum(package->metadata.file_path, calculated_checksum, sizeof(calculated_checksum));
    
    if (strcmp(calculated_checksum, package->metadata.checksum) != 0) {
        LOG_WARN("File checksum mismatch for: %s", package->metadata.file_path);
        return false;
    }
    
    return true;
}

void content_sharing_get_content_statistics(ContentSharingSystem* system, u32* total_content, u32* public_content, u32* private_content) {
    if (!system || !total_content || !public_content || !private_content) {
        return;
    }
    
    *total_content = system->local_content_count + system->remote_content_count;
    *public_content = 0;
    *private_content = 0;
    
    // Count local content by status
    for (u32 i = 0; i < system->local_content_count; i++) {
        if (system->local_content[i].metadata.status == SHARE_STATUS_PUBLIC) {
            (*public_content)++;
        } else {
            (*private_content)++;
        }
    }
    
    // Count remote content by status
    for (u32 i = 0; i < system->remote_content_count; i++) {
        if (system->remote_content[i].status == SHARE_STATUS_PUBLIC) {
            (*public_content)++;
        } else {
            (*private_content)++;
        }
    }
    
    LOG_DEBUG("Content statistics: total=%u, public=%u, private=%u", *total_content, *public_content, *private_content);
}
