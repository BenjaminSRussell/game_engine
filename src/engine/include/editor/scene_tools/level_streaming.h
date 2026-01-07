#ifndef LEVEL_STREAMING_H
#define LEVEL_STREAMING_H

#include "core/types.h"
#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LEVEL_STATE_UNLOADED,
    LEVEL_STATE_LOADING,
    LEVEL_STATE_LOADED,
    LEVEL_STATE_VISIBLE,
    LEVEL_STATE_UNLOADING
} LevelState;

typedef struct {
    char name[64];
    char path[256];
    Vec3 position;
    Vec3 bounds_min;
    Vec3 bounds_max;
    LevelState state;
    bool always_loaded;
    float stream_in_distance;
    float stream_out_distance;
} LevelSegment;

typedef struct {
    LevelSegment* levels;
    u32 level_count;
    u32 capacity;
    bool enabled;
    float update_interval;
    float time_since_update;
} LevelStreamingContext;

// Core API
void level_streaming_init(LevelStreamingContext* ctx);
void level_streaming_shutdown(LevelStreamingContext* ctx);
void level_streaming_update(LevelStreamingContext* ctx, Vec3 camera_position, float delta_time);

// Level Management
void level_streaming_add_level(LevelStreamingContext* ctx, const char* name, const char* path, Vec3 pos);
void level_streaming_remove_level(LevelStreamingContext* ctx, const char* name);
void level_streaming_load_level(LevelStreamingContext* ctx, const char* level_name);
void level_streaming_unload_level(LevelStreamingContext* ctx, const char* level_name);

// Configuration
void level_streaming_set_distances(LevelStreamingContext* ctx, const char* level_name, float load_dist, float unload_dist);

#ifdef __cplusplus
}
#endif

#endif // LEVEL_STREAMING_H
