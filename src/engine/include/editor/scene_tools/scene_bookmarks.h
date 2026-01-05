#pragma once

#include <stdbool.h>

typedef struct {
    const char *name;
    float camera_position[3];
    float camera_rotation[4]; // quaternion
    void *thumbnail_texture;
} SceneBookmark;

typedef struct {
    SceneBookmark *bookmarks;
    int bookmark_count;
    int capacity;
    
    // Transition
    bool is_transitioning;
    float transition_time;
    int target_bookmark;
} BookmarkManager;

void bookmark_manager_init(BookmarkManager *mgr);
void bookmark_manager_destroy(BookmarkManager *mgr);

// Bookmark management
void bookmark_create(BookmarkManager *mgr, const char *name, const float *cam_pos, const float *cam_rot);
void bookmark_delete(BookmarkManager *mgr, int index);
void bookmark_goto(BookmarkManager *mgr, int index, bool animated);

// Thumbnail generation
void bookmark_generate_thumbnail(SceneBookmark *bookmark);

// Transition
void bookmark_update_transition(BookmarkManager *mgr, float delta_time);
