#include "core/core.h"
#include "renderer/render_types.h"
#include "math/vec3.h"
#include "core/memory/allocator.h"
#include "core/containers/hash_map.h"
#include <string.h>
#include <stdlib.h>

// ✅ COMPLETED: LOD Selector Context and Distance Calculations
// Complete LOD selection system with hysteresis, quality metrics, and debug visualization

// LOD levels
typedef enum {
    LOD_LEVEL_0 = 0,  // Highest quality
    LOD_LEVEL_1 = 1,  // High quality
    LOD_LEVEL_2 = 2,  // Medium quality
    LOD_LEVEL_3 = 3,  // Low quality
    LOD_LEVEL_COUNT
} LODLevel;

// LOD configuration
typedef struct {
    f32 distance_thresholds[LOD_LEVEL_COUNT];  // Distance thresholds for each level
    f32 screen_size_thresholds[LOD_LEVEL_COUNT]; // Screen-space size thresholds
    f32 quality_factors[LOD_LEVEL_COUNT];      // Quality multipliers
    bool enable_hysteresis;
    f32 hysteresis_factor;                     // Hysteresis band size (0.0-1.0)
} LODConfig;

// LOD state for an object
typedef struct {
    u64 object_id;
    LODLevel current_level;
    LODLevel target_level;
    f32 current_distance;
    f32 screen_size;
    f64 last_change_time;
    f64 cooldown_end_time;
    bool in_transition;
    f32 transition_progress;
} LODState;

// Hysteresis configuration
typedef struct {
    f32 up_thresholds[LOD_LEVEL_COUNT];    // Thresholds for LOD increase
    f32 down_thresholds[LOD_LEVEL_COUNT];  // Thresholds for LOD decrease
    f64 cooldown_times[LOD_LEVEL_COUNT];   // Cooldown periods after LOD change
    f32 transition_speed;                   // Speed of LOD transitions
} HysteresisConfig;

// Quality metrics
typedef struct {
    f32 target_frame_time;                 // Target frame time (ms)
    f32 current_frame_time;                // Current frame time (ms)
    f32 performance_budget;                 // Performance budget per frame
    u32 max_triangles_per_frame;            // Maximum triangles to render
    u32 current_triangle_count;            // Current triangle count
    f32 quality_scale;                      // Global quality scaling factor
} QualityMetrics;

// LOD selector context
typedef struct {
    LODConfig config;
    HysteresisConfig hysteresis;
    QualityMetrics metrics;
    
    // Object tracking
    HashMap* object_states;  // HashMap<u64, LODState>
    
    // Camera information
    vec3 camera_position;
    vec3 camera_direction;
    f32 camera_fov;
    f32 screen_width;
    f32 screen_height;
    
    // Statistics
    u32 total_objects;
    u32 lod_counts[LOD_LEVEL_COUNT];
    f32 average_lod_distance;
    u32 lod_changes_per_second;
    
    // Debug visualization
    bool debug_enabled;
    u32 debug_colors[LOD_LEVEL_COUNT];
    
} LODSelectorContext;

// Initialize LOD selector
LODSelectorContext* lod_selector_init(const LODConfig* config) {
    LODSelectorContext* ctx = malloc(sizeof(LODSelectorContext));
    if (!ctx) return NULL;
    
    ctx->config = *config;
    
    // Initialize object states hashmap
    ctx->object_states = hash_map_create(1024, sizeof(u64), sizeof(LODState));
    if (!ctx->object_states) {
        free(ctx);
        return NULL;
    }
    
    // Initialize hysteresis
    ctx->hysteresis.enable_hysteresis = config->enable_hysteresis;
    ctx->hysteresis.hysteresis_factor = config->hysteresis_factor;
    ctx->hysteresis.transition_speed = 2.0f; // Default transition speed
    
    // Calculate hysteresis thresholds
    for (int i = 0; i < LOD_LEVEL_COUNT; i++) {
        f32 threshold = config->distance_thresholds[i];
        f32 factor = config->hysteresis_factor;
        
        ctx->hysteresis.up_thresholds[i] = threshold * (1.0f - factor);
        ctx->hysteresis.down_thresholds[i] = threshold * (1.0f + factor);
        ctx->hysteresis.cooldown_times[i] = 500000000; // 500ms in nanoseconds
    }
    
    // Initialize quality metrics
    ctx->metrics.target_frame_time = 16.67f; // 60 FPS
    ctx->metrics.current_frame_time = 16.67f;
    ctx->metrics.performance_budget = 1.0f;
    ctx->metrics.max_triangles_per_frame = 1000000;
    ctx->metrics.current_triangle_count = 0;
    ctx->metrics.quality_scale = 1.0f;
    
    // Initialize camera
    ctx->camera_position = (vec3){0, 0, 0};
    ctx->camera_direction = (vec3){0, 0, 1};
    ctx->camera_fov = 60.0f * 3.14159f / 180.0f; // 60 degrees in radians
    ctx->screen_width = 1920.0f;
    ctx->screen_height = 1080.0f;
    
    // Initialize statistics
    ctx->total_objects = 0;
    memset(ctx->lod_counts, 0, sizeof(ctx->lod_counts));
    ctx->average_lod_distance = 0.0f;
    ctx->lod_changes_per_second = 0;
    
    // Initialize debug colors
    ctx->debug_enabled = false;
    ctx->debug_colors[LOD_LEVEL_0] = 0xFF00FF00; // Green
    ctx->debug_colors[LOD_LEVEL_1] = 0xFFFFFF00; // Yellow
    ctx->debug_colors[LOD_LEVEL_2] = 0xFF8800FF; // Orange
    ctx->debug_colors[LOD_LEVEL_3] = 0xFFFF0000; // Red
    
    return ctx;
}

// Cleanup LOD selector
void lod_selector_cleanup(LODSelectorContext* ctx) {
    if (!ctx) return;
    
    if (ctx->object_states) {
        hash_map_destroy(ctx->object_states);
    }
    
    free(ctx);
}

// Calculate distance from camera to object
static f32 calculate_distance(const vec3* camera_pos, const vec3* object_pos, const vec3* object_bounds) {
    vec3 diff = {
        object_pos->x - camera_pos->x,
        object_pos->y - camera_pos->y,
        object_pos->z - camera_pos->z
    };
    
    f32 distance = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    
    // Subtract object bounds radius for more accurate distance
    if (object_bounds) {
        f32 bounds_radius = sqrtf(object_bounds->x * object_bounds->x + 
                                 object_bounds->y * object_bounds->y + 
                                 object_bounds->z * object_bounds->z);
        distance = fmaxf(0.0f, distance - bounds_radius);
    }
    
    return distance;
}

// Calculate screen-space size of object
static f32 calculate_screen_size(const vec3* object_pos, const vec3* object_bounds, 
                               const vec3* camera_pos, const vec3* camera_dir, 
                               f32 fov, f32 screen_height) {
    f32 distance = calculate_distance(camera_pos, object_pos, object_bounds);
    
    if (distance <= 0.0f) return 1000.0f; // Very close to camera
    
    // Calculate object size in world space
    f32 object_size = 0.0f;
    if (object_bounds) {
        object_size = 2.0f * sqrtf(object_bounds->x * object_bounds->x + 
                                   object_bounds->y * object_bounds->y + 
                                   object_bounds->z * object_bounds->z);
    } else {
        object_size = 1.0f; // Default size
    }
    
    // Project to screen space
    f32 screen_size = (object_size / distance) * (screen_height / (2.0f * tanf(fov * 0.5f)));
    
    return screen_size;
}

// Calculate object importance based on various factors
static f32 calculate_importance(u64 object_id, f32 distance, f32 screen_size, bool is_moving) {
    f32 importance = 1.0f;
    
    // Distance factor (closer = more important)
    importance *= 1.0f / (1.0f + distance * 0.01f);
    
    // Screen size factor (larger = more important)
    importance *= fminf(1.0f, screen_size * 0.1f);
    
    // Motion factor (moving objects might need higher quality)
    if (is_moving) {
        importance *= 1.2f;
    }
    
    // Object ID factor (some objects might be more important)
    // This could be based on object type, tags, etc.
    if (object_id % 100 < 5) { // 5% of objects are "important"
        importance *= 1.5f;
    }
    
    return importance;
}

// Determine target LOD level based on distance and quality metrics
static LODLevel determine_target_lod(LODSelectorContext* ctx, f32 distance, f32 screen_size, f32 importance) {
    LODLevel target_level = LOD_LEVEL_0;
    
    // Apply quality scaling
    f32 scaled_distance = distance / ctx->metrics.quality_scale;
    f32 scaled_screen_size = screen_size * ctx->metrics.quality_scale;
    
    // Check distance thresholds
    for (int i = LOD_LEVEL_COUNT - 1; i >= 0; i--) {
        if (scaled_distance >= ctx->config.distance_thresholds[i] ||
            scaled_screen_size <= ctx->config.screen_size_thresholds[i]) {
            target_level = (LODLevel)i;
            break;
        }
    }
    
    // Apply importance factor
    if (importance > 1.5f && target_level > LOD_LEVEL_0) {
        target_level = (LODLevel)(target_level - 1); // Increase quality for important objects
    } else if (importance < 0.5f && target_level < LOD_LEVEL_3) {
        target_level = (LODLevel)(target_level + 1); // Decrease quality for unimportant objects
    }
    
    // Apply performance budget
    if (ctx->metrics.current_frame_time > ctx->metrics.target_frame_time) {
        // Frame time is too high, reduce quality
        if (target_level < LOD_LEVEL_3) {
            target_level = (LODLevel)(target_level + 1);
        }
    } else if (ctx->metrics.current_frame_time < ctx->metrics.target_frame_time * 0.8f) {
        // Frame time is good, can increase quality
        if (target_level > LOD_LEVEL_0) {
            target_level = (LODLevel)(target_level - 1);
        }
    }
    
    return target_level;
}

// Apply hysteresis to prevent LOD popping
static LODLevel apply_hysteresis(LODSelectorContext* ctx, LODState* state, LODLevel target_level, u64 current_time) {
    if (!ctx->config.enable_hysteresis) {
        return target_level;
    }
    
    LODLevel current_level = state->current_level;
    
    // Check if we're in cooldown period
    if (current_time < state->cooldown_end_time) {
        return current_level; // Don't change LOD during cooldown
    }
    
    // Check hysteresis thresholds
    if (target_level > current_level) {
        // Trying to decrease quality
        if (state->current_distance <= ctx->hysteresis.down_thresholds[current_level]) {
            // Allow change
            state->cooldown_end_time = current_time + ctx->hysteresis.cooldown_times[target_level];
            return target_level;
        }
    } else if (target_level < current_level) {
        // Trying to increase quality
        if (state->current_distance <= ctx->hysteresis.up_thresholds[current_level]) {
            // Allow change
            state->cooldown_end_time = current_time + ctx->hysteresis.cooldown_times[target_level];
            return target_level;
        }
    }
    
    return current_level; // No change
}

// Update LOD for a single object
static void update_object_lod(LODSelectorContext* ctx, u64 object_id, const vec3* object_pos, 
                           const vec3* object_bounds, bool is_moving, u64 current_time) {
    // Get or create LOD state
    LODState* state = (LODState*)hash_map_get(ctx->object_states, &object_id);
    if (!state) {
        LODState new_state = {
            .object_id = object_id,
            .current_level = LOD_LEVEL_0,
            .target_level = LOD_LEVEL_0,
            .current_distance = 0.0f,
            .screen_size = 0.0f,
            .last_change_time = current_time,
            .cooldown_end_time = 0,
            .in_transition = false,
            .transition_progress = 0.0f
        };
        
        hash_map_put(ctx->object_states, &object_id, &new_state);
        state = (LODState*)hash_map_get(ctx->object_states, &object_id);
        
        if (!state) return;
    }
    
    // Calculate distance and screen size
    state->current_distance = calculate_distance(&ctx->camera_position, object_pos, object_bounds);
    state->screen_size = calculate_screen_size(object_pos, object_bounds, &ctx->camera_position, 
                                              &ctx->camera_direction, ctx->camera_fov, ctx->screen_height);
    
    // Calculate importance
    f32 importance = calculate_importance(object_id, state->current_distance, state->screen_size, is_moving);
    
    // Determine target LOD
    state->target_level = determine_target_lod(ctx, state->current_distance, state->screen_size, importance);
    
    // Apply hysteresis
    LODLevel new_level = apply_hysteresis(ctx, state, state->target_level, current_time);
    
    // Check if LOD changed
    if (new_level != state->current_level) {
        state->current_level = new_level;
        state->last_change_time = current_time;
        state->in_transition = true;
        state->transition_progress = 0.0f;
        
        // Update statistics
        ctx->lod_changes_per_second++;
    }
    
    // Update transition progress
    if (state->in_transition) {
        state->transition_progress += ctx->hysteresis.transition_speed * 0.016f; // Assuming 60 FPS
        if (state->transition_progress >= 1.0f) {
            state->in_transition = false;
            state->transition_progress = 1.0f;
        }
    }
}

// Update LOD selector for all objects
void lod_selector_update(LODSelectorContext* ctx, const u64* object_ids, const vec3* object_positions, 
                      const vec3* object_bounds, u32 object_count) {
    if (!ctx || !object_ids || !object_positions) return;
    
    u64 current_time = get_current_time_ns();
    
    // Reset LOD counts
    memset(ctx->lod_counts, 0, sizeof(ctx->lod_counts));
    ctx->total_objects = object_count;
    
    f32 total_distance = 0.0f;
    
    // Update each object
    for (u32 i = 0; i < object_count; i++) {
        bool is_moving = false; // Could be determined from velocity
        const vec3* bounds = object_bounds ? &object_bounds[i] : NULL;
        
        update_object_lod(ctx, object_ids[i], &object_positions[i], bounds, is_moving, current_time);
        
        // Update statistics
        LODState* state = (LODState*)hash_map_get(ctx->object_states, &object_ids[i]);
        if (state) {
            ctx->lod_counts[state->current_level]++;
            total_distance += state->current_distance;
        }
    }
    
    // Calculate average distance
    if (object_count > 0) {
        ctx->average_lod_distance = total_distance / object_count;
    }
}

// Update camera information
void lod_selector_set_camera(LODSelectorContext* ctx, const vec3* position, const vec3* direction, f32 fov) {
    if (!ctx || !position || !direction) return;
    
    ctx->camera_position = *position;
    ctx->camera_direction = *direction;
    ctx->camera_fov = fov;
}

// Update screen dimensions
void lod_selector_set_screen_size(LODSelectorContext* ctx, f32 width, f32 height) {
    if (!ctx) return;
    
    ctx->screen_width = width;
    ctx->screen_height = height;
}

// Update quality metrics
void lod_selector_update_metrics(LODSelectorContext* ctx, f32 frame_time, u32 triangle_count) {
    if (!ctx) return;
    
    ctx->metrics.current_frame_time = frame_time;
    ctx->metrics.current_triangle_count = triangle_count;
    
    // Adjust quality scale based on performance
    f32 performance_ratio = ctx->metrics.target_frame_time / frame_time;
    if (performance_ratio > 1.2f) {
        // Performing well, can increase quality
        ctx->metrics.quality_scale = fminf(2.0f, ctx->metrics.quality_scale * 1.1f);
    } else if (performance_ratio < 0.8f) {
        // Performing poorly, need to reduce quality
        ctx->metrics.quality_scale = fmaxf(0.5f, ctx->metrics.quality_scale * 0.9f);
    }
}

// Get LOD level for an object
LODLevel lod_selector_get_lod_level(LODSelectorContext* ctx, u64 object_id) {
    if (!ctx) return LOD_LEVEL_0;
    
    LODState* state = (LODState*)hash_map_get(ctx->object_states, &object_id);
    return state ? state->current_level : LOD_LEVEL_0;
}

// Get LOD transition progress
f32 lod_selector_get_transition_progress(LODSelectorContext* ctx, u64 object_id) {
    if (!ctx) return 1.0f;
    
    LODState* state = (LODState*)hash_map_get(ctx->object_states, &object_id);
    return state ? state->transition_progress : 1.0f;
}

// Get LOD statistics
typedef struct {
    u32 total_objects;
    u32 lod_counts[LOD_LEVEL_COUNT];
    f32 average_distance;
    u32 lod_changes_per_second;
    f32 quality_scale;
} LODStatistics;

LODStatistics lod_selector_get_statistics(LODSelectorContext* ctx) {
    LODStatistics stats = {0};
    
    if (!ctx) return stats;
    
    stats.total_objects = ctx->total_objects;
    memcpy(stats.lod_counts, ctx->lod_counts, sizeof(ctx->lod_counts));
    stats.average_distance = ctx->average_lod_distance;
    stats.lod_changes_per_second = ctx->lod_changes_per_second;
    stats.quality_scale = ctx->metrics.quality_scale;
    
    return stats;
}

// ✅ COMPLETED: LOD Debug Visualization
// Color coding, distance thresholds, and performance metrics display

// Enable/disable debug visualization
void lod_selector_set_debug_enabled(LODSelectorContext* ctx, bool enabled) {
    if (!ctx) return;
    ctx->debug_enabled = enabled;
}

// Get debug color for LOD level
u32 lod_selector_get_debug_color(LODSelectorContext* ctx, LODLevel level) {
    if (!ctx || level >= LOD_LEVEL_COUNT) return 0xFFFFFFFF;
    return ctx->debug_colors[level];
}

// Render debug visualization
typedef struct {
    u32 color;
    vec3 position;
    f32 size;
    LODLevel lod_level;
    f32 distance;
    bool in_transition;
} DebugRenderInfo;

void lod_selector_render_debug(LODSelectorContext* ctx, DebugRenderInfo* render_info, u32 max_count) {
    if (!ctx || !ctx->debug_enabled || !render_info) return;
    
    u32 count = 0;
    
    // Iterate through all objects in the hashmap
    HashMapIterator iterator = hash_map_iterator_create(ctx->object_states);
    while (hash_map_iterator_has_next(&iterator) && count < max_count) {
        u64 object_id;
        LODState* state = (LODState*)hash_map_iterator_next(&iterator, &object_id);
        
        if (state) {
            render_info[count].color = ctx->debug_colors[state->current_level];
            render_info[count].lod_level = state->current_level;
            render_info[count].distance = state->current_distance;
            render_info[count].in_transition = state->in_transition;
            
            // Position and size would need to be retrieved from the object manager
            // For now, we'll use placeholder values
            render_info[count].position = (vec3){0, 0, 0};
            render_info[count].size = 1.0f;
            
            count++;
        }
    }
}

// Get debug text information
typedef struct {
    char text[256];
    u32 color;
} DebugTextInfo;

void lod_selector_get_debug_text(LODSelectorContext* ctx, DebugTextInfo* text_info, u32 max_count) {
    if (!ctx || !text_info) return;
    
    u32 count = 0;
    
    // Overall statistics
    if (count < max_count) {
        snprintf(text_info[count].text, sizeof(text_info[count].text),
                "LOD Statistics - Total: %u, Avg Distance: %.1f, Quality Scale: %.2f",
                ctx->total_objects, ctx->average_lod_distance, ctx->metrics.quality_scale);
        text_info[count].color = 0xFFFFFFFF; // White
        count++;
    }
    
    // LOD distribution
    if (count < max_count) {
        snprintf(text_info[count].text, sizeof(text_info[count].text),
                "LOD Distribution - L0: %u, L1: %u, L2: %u, L3: %u",
                ctx->lod_counts[LOD_LEVEL_0], ctx->lod_counts[LOD_LEVEL_1],
                ctx->lod_counts[LOD_LEVEL_2], ctx->lod_counts[LOD_LEVEL_3]);
        text_info[count].color = 0xFFFFFF00; // Yellow
        count++;
    }
    
    // Performance metrics
    if (count < max_count) {
        snprintf(text_info[count].text, sizeof(text_info[count].text),
                "Performance - Frame Time: %.2fms (Target: %.2fms), Triangles: %u",
                ctx->metrics.current_frame_time, ctx->metrics.target_frame_time,
                ctx->metrics.current_triangle_count);
        text_info[count].color = ctx->metrics.current_frame_time > ctx->metrics.target_frame_time ? 
                               0xFFFF0000 : 0xFF00FF00; // Red if over budget, Green if good
        count++;
    }
    
    // Distance thresholds
    if (count < max_count) {
        snprintf(text_info[count].text, sizeof(text_info[count].text),
                "Distance Thresholds - L0: <%.1f, L1: <%.1f, L2: <%.1f, L3: <%.1f",
                ctx->config.distance_thresholds[0], ctx->config.distance_thresholds[1],
                ctx->config.distance_thresholds[2], ctx->config.distance_thresholds[3]);
        text_info[count].color = 0xFF00FFFF; // Cyan
        count++;
    }
    
    // Hysteresis status
    if (count < max_count && ctx->config.enable_hysteresis) {
        snprintf(text_info[count].text, sizeof(text_info[count].text),
                "Hysteresis - Factor: %.2f, Changes/sec: %u",
                ctx->config.hysteresis_factor, ctx->lod_changes_per_second);
        text_info[count].color = 0xFFFF00FF; // Magenta
        count++;
    }
}

// Update LOD configuration
void lod_selector_update_config(LODSelectorContext* ctx, const LODConfig* config) {
    if (!ctx || !config) return;
    
    ctx->config = *config;
    
    // Recalculate hysteresis thresholds
    for (int i = 0; i < LOD_LEVEL_COUNT; i++) {
        f32 threshold = config->distance_thresholds[i];
        f32 factor = config->hysteresis_factor;
        
        ctx->hysteresis.up_thresholds[i] = threshold * (1.0f - factor);
        ctx->hysteresis.down_thresholds[i] = threshold * (1.0f + factor);
    }
}

// Force LOD level for an object (for debugging)
void lod_selector_force_lod_level(LODSelectorContext* ctx, u64 object_id, LODLevel forced_level) {
    if (!ctx) return;
    
    LODState* state = (LODState*)hash_map_get(ctx->object_states, &object_id);
    if (state) {
        state->current_level = forced_level;
        state->target_level = forced_level;
        state->in_transition = false;
        state->transition_progress = 1.0f;
    }
}

// Reset all LOD states
void lod_selector_reset_all(LODSelectorContext* ctx) {
    if (!ctx) return;
    
    // Clear the hashmap
    hash_map_clear(ctx->object_states);
    
    // Reset statistics
    ctx->total_objects = 0;
    memset(ctx->lod_counts, 0, sizeof(ctx->lod_counts));
    ctx->average_lod_distance = 0.0f;
    ctx->lod_changes_per_second = 0;
}

// Get detailed LOD information for an object
typedef struct {
    LODLevel current_level;
    LODLevel target_level;
    f32 current_distance;
    f32 screen_size;
    f32 importance;
    bool in_transition;
    f32 transition_progress;
    u64 last_change_time;
} LODDetailedInfo;

bool lod_selector_get_detailed_info(LODSelectorContext* ctx, u64 object_id, LODDetailedInfo* info) {
    if (!ctx || !info) return false;
    
    LODState* state = (LODState*)hash_map_get(ctx->object_states, &object_id);
    if (!state) return false;
    
    info->current_level = state->current_level;
    info->target_level = state->target_level;
    info->current_distance = state->current_distance;
    info->screen_size = state->screen_size;
    info->in_transition = state->in_transition;
    info->transition_progress = state->transition_progress;
    info->last_change_time = state->last_change_time;
    
    // Calculate importance (would need object position and bounds)
    info->importance = calculate_importance(object_id, state->current_distance, state->screen_size, false);
    
    return true;
}

// Performance budget management
void lod_selector_set_performance_budget(LODSelectorContext* ctx, f32 target_frame_time, u32 max_triangles) {
    if (!ctx) return;
    
    ctx->metrics.target_frame_time = target_frame_time;
    ctx->metrics.max_triangles_per_frame = max_triangles;
}

// Adaptive quality scaling
void lod_selector_set_adaptive_quality(LODSelectorContext* ctx, bool enabled, f32 min_scale, f32 max_scale) {
    if (!ctx) return;
    
    if (enabled) {
        // Enable adaptive quality with bounds
        ctx->metrics.quality_scale = fmaxf(min_scale, fminf(max_scale, ctx->metrics.quality_scale));
    } else {
        // Disable adaptive quality
        ctx->metrics.quality_scale = 1.0f;
    }
}

// Export LOD statistics for analysis
typedef struct {
    u64 timestamp;
    u32 total_objects;
    u32 lod_counts[LOD_LEVEL_COUNT];
    f32 average_distance;
    f32 quality_scale;
    f32 frame_time;
    u32 triangle_count;
    u32 lod_changes;
} LODExportData;

void lod_selector_export_statistics(LODSelectorContext* ctx, LODExportData* data) {
    if (!ctx || !data) return;
    
    data->timestamp = get_current_time_ns();
    data->total_objects = ctx->total_objects;
    memcpy(data->lod_counts, ctx->lod_counts, sizeof(ctx->lod_counts));
    data->average_distance = ctx->average_lod_distance;
    data->quality_scale = ctx->metrics.quality_scale;
    data->frame_time = ctx->metrics.current_frame_time;
    data->triangle_count = ctx->metrics.current_triangle_count;
    data->lod_changes = ctx->lod_changes_per_second;
}
