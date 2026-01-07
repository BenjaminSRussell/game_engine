#include "editor/scene_tools/level_streaming.h"
#include "core/memory.h"
#include "core/logger.h"
#include <string.h>
#include <include/math/math.h>

// Placeholder for actual resource loading calls
static void internal_load_level_resource(const char* path) {
    LOG_INFO("Streaming In Level: %s", path);
}

static void internal_unload_level_resource(const char* path) {
    LOG_INFO("Streaming Out Level: %s", path);
}

void level_streaming_init(LevelStreamingContext* ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(LevelStreamingContext));
    ctx->capacity = 32;
    ctx->levels = (LevelSegment*)calloc(ctx->capacity, sizeof(LevelSegment));
    ctx->enabled = true;
    ctx->update_interval = 0.5f; // Update twice per second
    LOG_INFO("Level Streaming initialized");
}

void level_streaming_shutdown(LevelStreamingContext* ctx) {
    if (!ctx) return;
    if (ctx->levels) {
        free(ctx->levels);
    }
    memset(ctx, 0, sizeof(LevelStreamingContext));
}

void level_streaming_update(LevelStreamingContext* ctx, Vec3 camera_position, float delta_time) {
    if (!ctx || !ctx->enabled) return;

    ctx->time_since_update += delta_time;
    if (ctx->time_since_update < ctx->update_interval) return;
    ctx->time_since_update = 0.0f;

    for (u32 i = 0; i < ctx->level_count; i++) {
        LevelSegment* level = &ctx->levels[i];
        if (level->always_loaded) continue;

        float distance = vec3_distance(camera_position, level->position);

        // Streaming Logic
        if (level->state == LEVEL_STATE_UNLOADED) {
            if (distance < level->stream_in_distance) {
                level->state = LEVEL_STATE_LOADING;
                internal_load_level_resource(level->path);
                level->state = LEVEL_STATE_LOADED; // Immediate for now
            }
        } else if (level->state == LEVEL_STATE_LOADED || level->state == LEVEL_STATE_VISIBLE) {
            if (distance > level->stream_out_distance) {
                level->state = LEVEL_STATE_UNLOADING;
                internal_unload_level_resource(level->path);
                level->state = LEVEL_STATE_UNLOADED;
            }
        }
    }
}

void level_streaming_add_level(LevelStreamingContext* ctx, const char* name, const char* path, Vec3 pos) {
    if (!ctx || ctx->level_count >= ctx->capacity) return;

    LevelSegment* level = &ctx->levels[ctx->level_count++];
    strncpy(level->name, name, sizeof(level->name) - 1);
    strncpy(level->path, path, sizeof(level->path) - 1);
    level->position = pos;
    level->state = LEVEL_STATE_UNLOADED;
    level->stream_in_distance = 100.0f; // Default
    level->stream_out_distance = 150.0f; // Default hysteresis
}

void level_streaming_remove_level(LevelStreamingContext* ctx, const char* name) {
    // Basic remove (swap and pop) logic could go here
    // For now, stubbed
}

void level_streaming_load_level(LevelStreamingContext* ctx, const char* level_name) {
    for (u32 i = 0; i < ctx->level_count; i++) {
        if (strcmp(ctx->levels[i].name, level_name) == 0) {
            internal_load_level_resource(ctx->levels[i].path);
            ctx->levels[i].state = LEVEL_STATE_LOADED;
            return;
        }
    }
}

void level_streaming_unload_level(LevelStreamingContext* ctx, const char* level_name) {
    for (u32 i = 0; i < ctx->level_count; i++) {
        if (strcmp(ctx->levels[i].name, level_name) == 0) {
            internal_unload_level_resource(ctx->levels[i].path);
            ctx->levels[i].state = LEVEL_STATE_UNLOADED;
            return;
        }
    }
}

void level_streaming_set_distances(LevelStreamingContext* ctx, const char* level_name, float load_dist, float unload_dist) {
     for (u32 i = 0; i < ctx->level_count; i++) {
        if (strcmp(ctx->levels[i].name, level_name) == 0) {
            ctx->levels[i].stream_in_distance = load_dist;
            ctx->levels[i].stream_out_distance = unload_dist;
            return;
        }
    }
}
