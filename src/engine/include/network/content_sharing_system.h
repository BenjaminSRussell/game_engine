#ifndef CONTENT_SHARING_SYSTEM_H
#define CONTENT_SHARING_SYSTEM_H

#include <core/types.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <stdbool.h>
#include <stddef.h>

// Content types
typedef enum {
  CONTENT_TYPE_BLOCK,
  CONTENT_TYPE_ITEM,
  CONTENT_TYPE_MOB,
  CONTENT_TYPE_WORLD,
  CONTENT_TYPE_TEXTURE,
  CONTENT_TYPE_MODEL,
  CONTENT_TYPE_SCRIPT,
  CONTENT_TYPE_SHADER,
  CONTENT_TYPE_SOUND,
  CONTENT_TYPE_MUSIC
} ContentType;

// Content sharing status
typedef enum {
  SHARE_STATUS_PRIVATE,
  SHARE_STATUS_PUBLIC,
  SHARE_STATUS_UNLISTED,
  SHARE_STATUS_MODERATED,
  SHARE_STATUS_REJECTED
} ShareStatus;

// Content metadata
typedef struct {
  char id[64];
  char title[128];
  char description[512];
  char author[64];
  char version[16];
  ContentType type;
  ShareStatus status;

  // File information
  char file_path[256];
  u64 file_size;
  char checksum[64]; // SHA-256 hash

  // Statistics
  u32 download_count;
  u32 view_count;
  u32 like_count;
  u32 dislike_count;
  u32 comment_count;

  // Timestamps
  u64 created_time_ms;
  u64 updated_time_ms;
  u64 published_time_ms;

  // Tags and categories
  char tags[8][32];
  u32 tag_count;
  char category[64];

  // Preview information
  char preview_image_path[256];
  char preview_video_path[256];

  // Compatibility
  char min_engine_version[16];
  char max_engine_version[16];

  // Dependencies
  char dependencies[16][64];
  u32 dependency_count;
} ContentMetadata;

// Content package
typedef struct {
  ContentMetadata metadata;
  char *package_data;
  u64 package_size;
  bool is_compressed;

  // Additional files in package
  char **additional_files;
  u32 additional_file_count;
} ContentPackage;

// User profile
typedef struct {
  char user_id[64];
  char username[64];
  char display_name[64];
  char email[128];
  char bio[512];
  char avatar_url[256];

  // User statistics
  u32 content_count;
  u32 total_downloads;
  u32 total_likes;
  u32 followers_count;
  u32 following_count;

  // User settings
  bool is_verified;
  bool is_premium;
  bool allow_comments;
  bool allow_ratings;

  // Timestamps
  u64 joined_time_ms;
  u64 last_active_time_ms;
} UserProfile;

// Comment system
typedef struct {
  char comment_id[64];
  char content_id[64];
  char author_id[64];
  char author_name[64];
  char text[1024];

  // Comment hierarchy
  char parent_comment_id[64]; // Empty for top-level comments
  u32 reply_count;

  // Statistics
  u32 like_count;
  u32 dislike_count;

  // Status
  bool is_deleted;
  bool is_edited;
  bool is_pinned;

  // Timestamps
  u64 created_time_ms;
  u64 updated_time_ms;
} Comment;

// Rating system
typedef struct {
  char rating_id[64];
  char content_id[64];
  char user_id[64];

  // Rating values (1-5 stars)
  u32 rating;
  char review_text[512];

  // Status
  bool is_verified_purchase;
  bool is_helpful;

  // Timestamps
  u64 created_time_ms;
  u64 updated_time_ms;
} Rating;

// Search filters
typedef struct {
  ContentType type_filter;
  ShareStatus status_filter;
  char search_query[128];
  char category_filter[64];
  char author_filter[64];
  char tags_filter[8][32];
  u32 tag_filter_count;

  // Sorting options
  enum {
    SORT_BY_NEWEST,
    SORT_BY_OLDEST,
    SORT_BY_MOST_DOWNLOADED,
    SORT_BY_HIGHEST_RATED,
    SORT_BY_MOST_LIKED,
    SORT_BY_RELEVANCE
  } sort_by;

  // Date range
  u64 date_from_ms;
  u64 date_to_ms;

  // Rating filter
  u32 min_rating;
  u32 max_rating;

  // Pagination
  u32 page;
  u32 results_per_page;
} SearchFilters;

// Search results
typedef struct {
  ContentMetadata *results;
  u32 result_count;
  u32 total_results;
  u32 page;
  u32 results_per_page;
} SearchResults;

// Content sharing system
typedef struct {
  // Local content cache
  ContentPackage *local_content;
  u32 local_content_count;
  u32 local_content_capacity;

  // Remote content cache
  ContentMetadata *remote_content;
  u32 remote_content_count;
  u32 remote_content_capacity;

  // User profiles cache
  UserProfile *user_profiles;
  u32 user_profile_count;
  u32 user_profile_capacity;

  // Comments cache
  Comment *comments;
  u32 comment_count;
  u32 comment_capacity;

  // Ratings cache
  Rating *ratings;
  u32 rating_count;
  u32 rating_capacity;

  // System configuration
  char api_endpoint[256];
  char api_key[128];
  bool is_online;
  bool auto_sync;
  u32 sync_interval_ms;
  u64 last_sync_time_ms;

  // Current user
  UserProfile current_user;
  bool is_logged_in;

  // Callbacks
  void (*on_content_uploaded)(const char *content_id, bool success);
  void (*on_content_downloaded)(const char *content_id, bool success);
  void (*on_search_completed)(const SearchResults *results);
  void (*on_comment_posted)(const char *comment_id, bool success);
  void (*on_rating_submitted)(const char *rating_id, bool success);

  void *user_data;
} ContentSharingSystem;

// MARK: - Content Sharing System Management

bool content_sharing_init(ContentSharingSystem *system,
                          const char *api_endpoint, u32 max_content,
                          u32 max_users);
void content_sharing_shutdown(ContentSharingSystem *system);

bool content_sharing_login(ContentSharingSystem *system, const char *username,
                           const char *password);
void content_sharing_logout(ContentSharingSystem *system);
void content_sharing_update(ContentSharingSystem *system);

// MARK: - Content Management

bool content_sharing_upload_content(ContentSharingSystem *system,
                                    ContentPackage *package);
bool content_sharing_download_content(ContentSharingSystem *system,
                                      const char *content_id,
                                      const char *download_path);
bool content_sharing_delete_content(ContentSharingSystem *system,
                                    const char *content_id);
bool content_sharing_update_content(ContentSharingSystem *system,
                                    const ContentPackage *package);

ContentPackage *content_sharing_create_package(ContentSharingSystem *system,
                                               ContentType type,
                                               const char *title,
                                               const char *description,
                                               const char *file_path);
bool content_sharing_add_file_to_package(ContentPackage *package,
                                         const char *file_path);
bool content_sharing_set_package_metadata(ContentPackage *package,
                                          const ContentMetadata *metadata);

// MARK: - Search and Discovery

SearchResults *content_sharing_search(ContentSharingSystem *system,
                                      const SearchFilters *filters);
SearchResults *content_sharing_get_trending(ContentSharingSystem *system,
                                            ContentType type, u32 limit);
SearchResults *content_sharing_get_featured(ContentSharingSystem *system,
                                            ContentType type, u32 limit);
SearchResults *content_sharing_get_user_content(ContentSharingSystem *system,
                                                const char *user_id, u32 limit);

ContentMetadata *
content_sharing_get_content_details(ContentSharingSystem *system,
                                    const char *content_id);
UserProfile *content_sharing_get_user_profile(ContentSharingSystem *system,
                                              const char *user_id);

// MARK: - Rating and Review System

bool content_sharing_rate_content(ContentSharingSystem *system,
                                  const char *content_id, u32 rating,
                                  const char *review_text);
bool content_sharing_like_content(ContentSharingSystem *system,
                                  const char *content_id, bool like);
Rating *content_sharing_get_content_ratings(ContentSharingSystem *system,
                                            const char *content_id, u32 *count);

// MARK: - Comment System

bool content_sharing_post_comment(ContentSharingSystem *system,
                                  const char *content_id, const char *text,
                                  const char *parent_id);
bool content_sharing_edit_comment(ContentSharingSystem *system,
                                  const char *comment_id, const char *text);
bool content_sharing_delete_comment(ContentSharingSystem *system,
                                    const char *comment_id);
bool content_sharing_like_comment(ContentSharingSystem *system,
                                  const char *comment_id, bool like);

Comment *content_sharing_get_content_comments(ContentSharingSystem *system,
                                              const char *content_id,
                                              u32 *count);
Comment *content_sharing_get_comment_replies(ContentSharingSystem *system,
                                             const char *comment_id,
                                             u32 *count);

// MARK: - User Management

bool content_sharing_update_user_profile(ContentSharingSystem *system,
                                         const UserProfile *profile);
bool content_sharing_follow_user(ContentSharingSystem *system,
                                 const char *user_id, bool follow);
UserProfile *content_sharing_get_user_followers(ContentSharingSystem *system,
                                                const char *user_id,
                                                u32 *count);
UserProfile *content_sharing_get_user_following(ContentSharingSystem *system,
                                                const char *user_id,
                                                u32 *count);

// MARK: - Moderation System

bool content_sharing_report_content(ContentSharingSystem *system,
                                    const char *content_id, const char *reason);
bool content_sharing_report_comment(ContentSharingSystem *system,
                                    const char *comment_id, const char *reason);
bool content_sharing_moderate_content(ContentSharingSystem *system,
                                      const char *content_id,
                                      ShareStatus status);
bool content_sharing_moderate_comment(ContentSharingSystem *system,
                                      const char *comment_id,
                                      bool delete_comment);

// MARK: - Utility Functions

void content_sharing_set_callbacks(ContentSharingSystem *system,
                                   void (*on_upload)(const char *, bool),
                                   void (*on_download)(const char *, bool),
                                   void (*on_search)(const SearchResults *),
                                   void (*on_comment)(const char *, bool),
                                   void (*on_rating)(const char *, bool));

void content_sharing_enable_auto_sync(ContentSharingSystem *system,
                                      bool enabled, u32 interval_ms);
bool content_sharing_sync_content(ContentSharingSystem *system);
bool content_sharing_export_content_list(ContentSharingSystem *system,
                                         const char *filename);
bool content_sharing_import_content_list(ContentSharingSystem *system,
                                         const char *filename);

// MARK: - Content Validation

bool content_sharing_validate_package(const ContentPackage *package);
bool content_sharing_validate_metadata(const ContentMetadata *metadata);
bool content_sharing_check_compatibility(const ContentMetadata *metadata,
                                         const char *engine_version);

// MARK: - Statistics and Analytics

void content_sharing_get_content_statistics(ContentSharingSystem *system,
                                            u32 *total_content,
                                            u32 *public_content,
                                            u32 *private_content);
void content_sharing_get_user_statistics(ContentSharingSystem *system,
                                         u32 *total_users, u32 *active_users,
                                         u32 *premium_users);
void content_sharing_get_download_statistics(ContentSharingSystem *system,
                                             u64 *total_downloads,
                                             u32 *today_downloads);

#endif // CONTENT_SHARING_SYSTEM_H
