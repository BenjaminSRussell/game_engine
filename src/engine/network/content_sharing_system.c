// Content Sharing System implementation
#include "network/content_sharing_system.h"
#include "engine/include/core/logger.h"
#include "engine/include/math/math_all.h"
#include <math/vec3.h>
#include <math/vec4.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(CONTENT_SHARING_HAVE_CURL)
#include <curl/curl.h>
#endif

// MARK: - Helper Functions

#include "engine/include/core/utils.h"

static void generate_content_id(char *id, size_t size) {
  const char charset[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (size_t i = 0; i < size - 1; i++) {
    id[i] = charset[rand() % (sizeof(charset) - 1)];
  }
  id[size - 1] = '\0';
}

#define CONTENT_SHA256_HEX_LEN 64

typedef struct ContentSha256 {
  u32 state[8];
  u64 bit_len;
  u8 buffer[64];
  u32 buffer_len;
} ContentSha256;

static u32 content_rotr(u32 x, u32 n) {
  return (x >> n) | (x << (32u - n));
}

static void content_sha256_transform(ContentSha256 *ctx, const u8 block[64]) {
  static const u32 k[64] = {
      0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu,
      0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u,
      0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u,
      0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
      0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u,
      0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
      0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
      0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
      0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u,
      0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u, 0x1e376c08u,
      0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu,
      0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
      0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};
  u32 w[64];
  for (u32 i = 0; i < 16; i++) {
    w[i] = ((u32)block[i * 4] << 24) | ((u32)block[i * 4 + 1] << 16) |
           ((u32)block[i * 4 + 2] << 8) | (u32)block[i * 4 + 3];
  }
  for (u32 i = 16; i < 64; i++) {
    u32 s0 = content_rotr(w[i - 15], 7) ^ content_rotr(w[i - 15], 18) ^
             (w[i - 15] >> 3);
    u32 s1 = content_rotr(w[i - 2], 17) ^ content_rotr(w[i - 2], 19) ^
             (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }

  u32 a = ctx->state[0];
  u32 b = ctx->state[1];
  u32 c = ctx->state[2];
  u32 d = ctx->state[3];
  u32 e = ctx->state[4];
  u32 f = ctx->state[5];
  u32 g = ctx->state[6];
  u32 h = ctx->state[7];
  for (u32 i = 0; i < 64; i++) {
    u32 s1 = content_rotr(e, 6) ^ content_rotr(e, 11) ^ content_rotr(e, 25);
    u32 ch = (e & f) ^ ((~e) & g);
    u32 t1 = h + s1 + ch + k[i] + w[i];
    u32 s0 = content_rotr(a, 2) ^ content_rotr(a, 13) ^ content_rotr(a, 22);
    u32 maj = (a & b) ^ (a & c) ^ (b & c);
    u32 t2 = s0 + maj;
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }

  ctx->state[0] += a;
  ctx->state[1] += b;
  ctx->state[2] += c;
  ctx->state[3] += d;
  ctx->state[4] += e;
  ctx->state[5] += f;
  ctx->state[6] += g;
  ctx->state[7] += h;
}

static void content_sha256_init(ContentSha256 *ctx) {
  ctx->state[0] = 0x6a09e667u;
  ctx->state[1] = 0xbb67ae85u;
  ctx->state[2] = 0x3c6ef372u;
  ctx->state[3] = 0xa54ff53au;
  ctx->state[4] = 0x510e527fu;
  ctx->state[5] = 0x9b05688cu;
  ctx->state[6] = 0x1f83d9abu;
  ctx->state[7] = 0x5be0cd19u;
  ctx->bit_len = 0;
  ctx->buffer_len = 0;
}

static void content_sha256_update(ContentSha256 *ctx, const u8 *data,
                                  size_t len) {
  for (size_t i = 0; i < len; i++) {
    ctx->buffer[ctx->buffer_len++] = data[i];
    if (ctx->buffer_len == 64) {
      content_sha256_transform(ctx, ctx->buffer);
      ctx->bit_len += 512;
      ctx->buffer_len = 0;
    }
  }
}

static void content_sha256_final(ContentSha256 *ctx, u8 digest[32]) {
  u32 i = ctx->buffer_len;

  if (ctx->buffer_len < 56) {
    ctx->buffer[i++] = 0x80;
    while (i < 56) {
      ctx->buffer[i++] = 0x00;
    }
  } else {
    ctx->buffer[i++] = 0x80;
    while (i < 64) {
      ctx->buffer[i++] = 0x00;
    }
    content_sha256_transform(ctx, ctx->buffer);
    memset(ctx->buffer, 0, 56);
  }

  ctx->bit_len += (u64)ctx->buffer_len * 8u;
  ctx->buffer[63] = (u8)ctx->bit_len;
  ctx->buffer[62] = (u8)(ctx->bit_len >> 8);
  ctx->buffer[61] = (u8)(ctx->bit_len >> 16);
  ctx->buffer[60] = (u8)(ctx->bit_len >> 24);
  ctx->buffer[59] = (u8)(ctx->bit_len >> 32);
  ctx->buffer[58] = (u8)(ctx->bit_len >> 40);
  ctx->buffer[57] = (u8)(ctx->bit_len >> 48);
  ctx->buffer[56] = (u8)(ctx->bit_len >> 56);
  content_sha256_transform(ctx, ctx->buffer);

  for (u32 j = 0; j < 8; j++) {
    digest[j * 4] = (u8)(ctx->state[j] >> 24);
    digest[j * 4 + 1] = (u8)(ctx->state[j] >> 16);
    digest[j * 4 + 2] = (u8)(ctx->state[j] >> 8);
    digest[j * 4 + 3] = (u8)ctx->state[j];
  }
}

static void calculate_file_checksum(const char *file_path, char *checksum,
                                    size_t size) {
  static const char hex_digits[] = "0123456789abcdef";

  if (!checksum || size == 0) {
    return;
  }
  checksum[0] = '\0';
  if (!file_path || size < CONTENT_SHA256_HEX_LEN + 1) {
    return;
  }

  FILE *file = fopen(file_path, "rb");
  if (!file) {
    memset(checksum, '0', CONTENT_SHA256_HEX_LEN);
    checksum[CONTENT_SHA256_HEX_LEN] = '\0';
    return;
  }

  ContentSha256 ctx;
  content_sha256_init(&ctx);

  unsigned char buffer[4096];
  size_t bytes_read;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    content_sha256_update(&ctx, buffer, bytes_read);
  }
  fclose(file);

  unsigned char digest[32];
  content_sha256_final(&ctx, digest);
  for (int i = 0; i < 32; i++) {
    checksum[i * 2] = hex_digits[digest[i] >> 4];
    checksum[i * 2 + 1] = hex_digits[digest[i] & 0x0f];
  }
  checksum[CONTENT_SHA256_HEX_LEN] = '\0';
}

#if defined(CONTENT_SHARING_HAVE_CURL)
static size_t write_callback(void *contents, size_t size, size_t nmemb,
                             void *userp) {
  size_t realsize = size * nmemb;
  char *response = (char *)userp;
  strncat(response, (char *)contents, realsize);
  return realsize;
}

static bool make_http_request(const char *url, const char *method,
                              const char *data, char *response,
                              size_t response_size) {
  CURL *curl = curl_easy_init();
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
  (void)response_size;

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    LOG_ERROR("HTTP request failed: %s", curl_easy_strerror(res));
    return false;
  }

  return true;
}
#else
static bool make_http_request(const char *url, const char *method,
                              const char *data, char *response,
                              size_t response_size) {
  (void)url;
  (void)method;
  (void)data;
  (void)response;
  (void)response_size;
  LOG_ERROR("HTTP is unavailable because libcurl is not linked");
  return false;
}
#endif

// MARK: - Content Sharing System Management

bool content_sharing_init(ContentSharingSystem *system,
                          const char *api_endpoint, u32 max_content,
                          u32 max_users) {
  if (!system || !api_endpoint || max_content == 0 || max_users == 0) {
    LOG_ERROR("Invalid parameters for content_sharing_init");
    return false;
  }

  memset(system, 0, sizeof(ContentSharingSystem));

  // Copy API endpoint
  strncpy(system->api_endpoint, api_endpoint, sizeof(system->api_endpoint) - 1);
  system->api_endpoint[sizeof(system->api_endpoint) - 1] = '\0';

  // Allocate content arrays
  system->local_content =
      (ContentPackage *)calloc(max_content, sizeof(ContentPackage));
  system->remote_content =
      (ContentMetadata *)calloc(max_content, sizeof(ContentMetadata));
  if (!system->local_content || !system->remote_content) {
    LOG_ERROR("Failed to allocate content arrays");
    return false;
  }
  system->local_content_capacity = max_content;
  system->remote_content_capacity = max_content;

  // Allocate user profiles array
  system->user_profiles = (UserProfile *)calloc(max_users, sizeof(UserProfile));
  if (!system->user_profiles) {
    free(system->local_content);
    free(system->remote_content);
    LOG_ERROR("Failed to allocate user profiles array");
    return false;
  }
  system->user_profile_capacity = max_users;

  // Allocate comments and ratings arrays
  system->comments = (Comment *)calloc(
      max_content * 10, sizeof(Comment)); // Assume 10 comments per content
  system->ratings = (Rating *)calloc(
      max_content * 5, sizeof(Rating)); // Assume 5 ratings per content
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

#if defined(CONTENT_SHARING_HAVE_CURL)
  curl_global_init(CURL_GLOBAL_DEFAULT);
#endif

  LOG_INFO("Content sharing system initialized (content: %u, users: %u)",
           max_content, max_users);
  return true;
}

void content_sharing_shutdown(ContentSharingSystem *system) {
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
        for (u32 j = 0; j < system->local_content[i].additional_file_count;
             j++) {
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

#if defined(CONTENT_SHARING_HAVE_CURL)
  curl_global_cleanup();
#endif

  memset(system, 0, sizeof(ContentSharingSystem));
  LOG_INFO("Content sharing system shutdown");
}

bool content_sharing_login(ContentSharingSystem *system, const char *username,
                           const char *password) {
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

  if (!make_http_request(url, "POST", request_data, response,
                         sizeof(response))) {
    LOG_ERROR("Login request failed");
    return false;
  }

  // TODO: Parse JSON response and extract user data
  // For now, simulate successful login

  strncpy(system->current_user.username, username,
          sizeof(system->current_user.username) - 1);
  strncpy(system->current_user.display_name, username,
          sizeof(system->current_user.display_name) - 1);
  system->current_user.joined_time_ms = get_current_time_ms();
  system->current_user.last_active_time_ms = get_current_time_ms();

  system->is_logged_in = true;

  LOG_INFO("User logged in: %s", username);
  return true;
}

void content_sharing_logout(ContentSharingSystem *system) {
  if (!system) {
    return;
  }

  system->is_logged_in = false;
  memset(&system->current_user, 0, sizeof(UserProfile));

  LOG_INFO("User logged out");
}

void content_sharing_update(ContentSharingSystem *system) {
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

ContentPackage *content_sharing_create_package(ContentSharingSystem *system,
                                               ContentType type,
                                               const char *title,
                                               const char *description,
                                               const char *file_path) {
  if (!system || !title || !file_path) {
    return NULL;
  }

  if (system->local_content_count >= system->local_content_capacity) {
    LOG_ERROR("Local content capacity exceeded");
    return NULL;
  }

  ContentPackage *package = &system->local_content[system->local_content_count];
  memset(package, 0, sizeof(ContentPackage));

  // Generate content ID
  generate_content_id(package->metadata.id, sizeof(package->metadata.id));

  // Set metadata
  strncpy(package->metadata.title, title, sizeof(package->metadata.title) - 1);
  package->metadata.title[sizeof(package->metadata.title) - 1] = '\0';

  if (description) {
    strncpy(package->metadata.description, description,
            sizeof(package->metadata.description) - 1);
    package->metadata.description[sizeof(package->metadata.description) - 1] =
        '\0';
  }

  strncpy(package->metadata.file_path, file_path,
          sizeof(package->metadata.file_path) - 1);
  package->metadata.file_path[sizeof(package->metadata.file_path) - 1] = '\0';

  package->metadata.type = type;
  package->metadata.status = SHARE_STATUS_PRIVATE;
  package->metadata.created_time_ms = get_current_time_ms();
  package->metadata.updated_time_ms = package->metadata.created_time_ms;

  // Set author to current user
  if (system->is_logged_in) {
    strncpy(package->metadata.author, system->current_user.username,
            sizeof(package->metadata.author) - 1);
    package->metadata.author[sizeof(package->metadata.author) - 1] = '\0';
  }

  // Calculate file checksum
  calculate_file_checksum(file_path, package->metadata.checksum,
                          sizeof(package->metadata.checksum));

  // Get file size
  FILE *file = fopen(file_path, "rb");
  if (file) {
    fseek(file, 0, SEEK_END);
    package->metadata.file_size = ftell(file);
    fclose(file);
  }

  system->local_content_count++;
  LOG_DEBUG("Created content package: %s", title);
  return package;
}

bool content_sharing_upload_content(ContentSharingSystem *system,
                                    ContentPackage *package) {
  if (!system || !package || !system->is_logged_in) {
    return false;
  }

  // Create upload request
  char request_data[8192];
  snprintf(
      request_data, sizeof(request_data),
      "{\"id\":\"%s\",\"title\":\"%s\",\"description\":\"%s\",\"type\":%u,"
      "\"author\":\"%s\","
      "\"file_size\":%llu,\"checksum\":\"%s\",\"tags\":[],\"category\":\"\"}",
      package->metadata.id, package->metadata.title,
      package->metadata.description, package->metadata.type,
      package->metadata.author, package->metadata.file_size,
      package->metadata.checksum);

  char response[4096];
  char url[512];
  snprintf(url, sizeof(url), "%s/api/content/upload", system->api_endpoint);

  if (!make_http_request(url, "POST", request_data, response,
                         sizeof(response))) {
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

bool content_sharing_download_content(ContentSharingSystem *system,
                                      const char *content_id,
                                      const char *download_path) {
  if (!system || !content_id || !download_path) {
    return false;
  }

  // Create download request
  char request_data[256];
  snprintf(request_data, sizeof(request_data), "{\"content_id\":\"%s\"}",
           content_id);

  char response[4096];
  char url[512];
  snprintf(url, sizeof(url), "%s/api/content/download", system->api_endpoint);

  if (!make_http_request(url, "POST", request_data, response,
                         sizeof(response))) {
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

SearchResults *content_sharing_search(ContentSharingSystem *system,
                                      const SearchFilters *filters) {
  if (!system || !filters) {
    return NULL;
  }

  // Create search request
  char request_data[1024];
  snprintf(request_data, sizeof(request_data),
           "{\"type_filter\":%u,\"status_filter\":%u,\"search_query\":\"%s\","
           "\"category_filter\":\"%s\",\"author_filter\":\"%s\",\"sort_by\":%u,"
           "\"page\":%u}",
           filters->type_filter, filters->status_filter, filters->search_query,
           filters->category_filter, filters->author_filter, filters->sort_by,
           filters->page);

  char response[8192];
  char url[512];
  snprintf(url, sizeof(url), "%s/api/content/search", system->api_endpoint);

  if (!make_http_request(url, "POST", request_data, response,
                         sizeof(response))) {
    LOG_ERROR("Search request failed");
    return NULL;
  }

  // TODO: Parse JSON response and create search results
  // For now, create mock results

  SearchResults *results = (SearchResults *)calloc(1, sizeof(SearchResults));
  if (!results) {
    return NULL;
  }

  results->results =
      (ContentMetadata *)calloc(10, sizeof(ContentMetadata)); // Mock 10 results
  results->result_count = 10;
  results->total_results = 100;
  results->page = filters->page;
  results->results_per_page = 10;

  // Generate mock results
  for (u32 i = 0; i < results->result_count; i++) {
    generate_content_id(results->results[i].id, sizeof(results->results[i].id));
    snprintf(results->results[i].description,
             sizeof(results->results[i].description),
             "Description for content %u", i + 1);
    results->results[i].type = filters->type_filter;
    results->results[i].status = SHARE_STATUS_PUBLIC;
    results->results[i].download_count = rand() % 1000;
    results->results[i].like_count = rand() % 100;
    results->results[i].created_time_ms =
        get_current_time_ms() - (rand() % 86400000); // Within last 24 hours
  }

  LOG_DEBUG("Search completed: %u results", results->result_count);

  if (system->on_search_completed) {
    system->on_search_completed(results);
  }

  return results;
}

// MARK: - Rating and Review System

bool content_sharing_rate_content(ContentSharingSystem *system,
                                  const char *content_id, u32 rating,
                                  const char *review_text) {
  if (!system || !content_id || rating < 1 || rating > 5 ||
      !system->is_logged_in) {
    return false;
  }

  if (system->rating_count >= system->rating_capacity) {
    LOG_ERROR("Rating capacity exceeded");
    return false;
  }

  Rating *new_rating = &system->ratings[system->rating_count++];
  memset(new_rating, 0, sizeof(Rating));

  // Generate rating ID
  generate_content_id(new_rating->rating_id, sizeof(new_rating->rating_id));

  // Set rating data
  strncpy(new_rating->content_id, content_id,
          sizeof(new_rating->content_id) - 1);
  strncpy(new_rating->user_id, system->current_user.user_id,
          sizeof(new_rating->user_id) - 1);
  new_rating->rating = rating;

  if (review_text) {
    strncpy(new_rating->review_text, review_text,
            sizeof(new_rating->review_text) - 1);
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

  if (!make_http_request(url, "POST", request_data, response,
                         sizeof(response))) {
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

bool content_sharing_post_comment(ContentSharingSystem *system,
                                  const char *content_id, const char *text,
                                  const char *parent_id) {
  if (!system || !content_id || !text || !system->is_logged_in) {
    return false;
  }

  if (system->comment_count >= system->comment_capacity) {
    LOG_ERROR("Comment capacity exceeded");
    return false;
  }

  Comment *new_comment = &system->comments[system->comment_count++];
  memset(new_comment, 0, sizeof(Comment));

  // Generate comment ID
  generate_content_id(new_comment->comment_id, sizeof(new_comment->comment_id));

  // Set comment data
  strncpy(new_comment->content_id, content_id,
          sizeof(new_comment->content_id) - 1);
  strncpy(new_comment->author_id, system->current_user.user_id,
          sizeof(new_comment->author_id) - 1);
  strncpy(new_comment->author_name, system->current_user.username,
          sizeof(new_comment->author_name) - 1);
  strncpy(new_comment->text, text, sizeof(new_comment->text) - 1);
  new_comment->text[sizeof(new_comment->text) - 1] = '\0';

  if (parent_id) {
    strncpy(new_comment->parent_comment_id, parent_id,
            sizeof(new_comment->parent_comment_id) - 1);
    new_comment->parent_comment_id[sizeof(new_comment->parent_comment_id) - 1] =
        '\0';
  }

  new_comment->created_time_ms = get_current_time_ms();

  // Create comment request
  char request_data[2048];
  snprintf(
      request_data, sizeof(request_data),
      "{\"content_id\":\"%s\",\"text\":\"%s\",\"parent_comment_id\":\"%s\"}",
      content_id, text, parent_id ? parent_id : "");

  char response[512];
  char url[512];
  snprintf(url, sizeof(url), "%s/api/content/comment", system->api_endpoint);

  if (!make_http_request(url, "POST", request_data, response,
                         sizeof(response))) {
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

void content_sharing_set_callbacks(ContentSharingSystem *system,
                                   void (*on_upload)(const char *, bool),
                                   void (*on_download)(const char *, bool),
                                   void (*on_search)(const SearchResults *),
                                   void (*on_comment)(const char *, bool),
                                   void (*on_rating)(const char *, bool)) {
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

void content_sharing_enable_auto_sync(ContentSharingSystem *system,
                                      bool enabled, u32 interval_ms) {
  if (!system) {
    return;
  }

  system->auto_sync = enabled;
  system->sync_interval_ms = interval_ms;

  LOG_INFO("Auto sync %s (interval: %u ms)", enabled ? "enabled" : "disabled",
           interval_ms);
}

bool content_sharing_sync_content(ContentSharingSystem *system) {
  if (!system || !system->is_online) {
    return false;
  }

  // TODO: Implement actual synchronization
  // For now, just log the sync attempt
  LOG_DEBUG("Syncing content with server");

  return true;
}

bool content_sharing_validate_package(const ContentPackage *package) {
  if (!package) {
    return false;
  }

  // Check required fields
  if (strlen(package->metadata.title) == 0 ||
      strlen(package->metadata.author) == 0) {
    return false;
  }

  // Check file exists
  FILE *file = fopen(package->metadata.file_path, "rb");
  if (!file) {
    return false;
  }
  fclose(file);

  // Validate checksum
  char calculated_checksum[65];
  calculate_file_checksum(package->metadata.file_path, calculated_checksum,
                          sizeof(calculated_checksum));

  if (strcmp(calculated_checksum, package->metadata.checksum) != 0) {
    LOG_WARN("File checksum mismatch for: %s", package->metadata.file_path);
    return false;
  }

  return true;
}

void content_sharing_get_content_statistics(ContentSharingSystem *system,
                                            u32 *total_content,
                                            u32 *public_content,
                                            u32 *private_content) {
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

  LOG_DEBUG("Content statistics: total=%u, public=%u, private=%u",
            *total_content, *public_content, *private_content);
}
