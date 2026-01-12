#include "../editor_common.h"
#include "core/logger.h"
#include "core/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/**
 * =================================================================================================
 *                           TERRAIN SCULPTING TOOLS - AGENT_EDITOR_2
 * =================================================================================================
 *
 * PURPOSE: Professional terrain sculpting tools like Unity Terrain Engine or Unreal
 * Landscape Editor.
 *
 * PERFORMANCE TARGET: <16ms for 1024x1024 heightmap operations
 *
 * =================================================================================================
 */

//  PARTIAL IMPLEMENTATION: Terrain Sculpting Backend [Difficulty: 6]
// STATUS: Core sculpting algorithms implemented (raise, lower, smooth, flatten)
// with brush falloff curves and basic heightmap operations.
//  Undo/Redo system is NOW FULLY IMPLEMENTED with history capture and restoration
// 
//  MISSING CRITICAL FEATURES:
//    - No UI integration (no brush selector, property panels, etc.)
//    - No viewport rendering (cannot visualize terrain or brush cursor)
//    - No integration with main rendering engine
// 
// This is a LIBRARY of terrain manipulation functions, NOT a usable editor tool.
// To make this functional, you need to implement:
//    1. UI layer for brush controls
//    2. 3D viewport rendering for visual feedback
//    3. Integration with main rendering engine

// Forward declarations
void terrain_sculpting_clear_history(TerrainSculptingSystem* system);
void terrain_heightmap_destroy(TerrainHeightmap* heightmap);
TerrainHeightmap* terrain_heightmap_create(u32 width, u32 height); // Forward decl just in case
f32 terrain_heightmap_get_height(TerrainHeightmap* heightmap, u32 x, u32 z); // Used in pixel_to_world
void terrain_sculpting_push_history(TerrainSculptingSystem* system);

// Helper for clamping floats
static inline f32 fclampf(f32 v, f32 min_v, f32 max_v) {
    return fmaxf(min_v, fminf(v, max_v));
}

// =================================================================================================
//                                    CORE IMPLEMENTATION
// =================================================================================================

// Helper function to get falloff weight based on distance and curve
static f32 get_falloff_weight(f32 normalized_distance, FalloffType falloff) {
    switch (falloff) {
        case FALLOFF_LINEAR:
            return 1.0f - normalized_distance;
            
        case FALLOFF_SMOOTH:
            // Smoothstep falloff
            {
                f32 t = normalized_distance;
                return t * t * (3.0f - 2.0f * t);
            }
            
        case FALLOFF_SHARP:
            // Exponential falloff
            return powf(1.0f - normalized_distance, 2.0f);
            
        case FALLOFF_CONSTANT:
            return 1.0f;
            
        default:
            return 1.0f - normalized_distance;
    }
}

// System management
TerrainSculptingSystem* terrain_sculpting_create(TerrainHeightmap* heightmap) {
    TerrainSculptingSystem* system = malloc(sizeof(TerrainSculptingSystem));
    if (!system) return NULL;
    
    memset(system, 0, sizeof(TerrainSculptingSystem));
    
    // Initialize default brush
    system->current_brush.type = BRUSH_TYPE_CIRCLE;
    system->current_brush.operation = SCULPT_RAISE;
    system->current_brush.falloff = FALLOFF_SMOOTH;
    system->current_brush.radius = 10.0f;
    system->current_brush.strength = 0.5f;
    system->current_brush.hardness = 0.5f;
    system->current_brush.is_active = true;
    
    // Set heightmap
    system->heightmap = heightmap;
    
    // Initialize history
    system->history_capacity = 100;
    system->history = malloc(sizeof(SculptHistoryEntry) * system->history_capacity);
    system->history_size = 0;
    system->current_history_index = -1;
    
    LOG_INFO("Created terrain sculpting system");
    return system;
}

void terrain_sculpting_destroy(TerrainSculptingSystem* system) {
    if (!system) return;
    
    // Clear history
    terrain_sculpting_clear_history(system);
    free(system->history);
    
    free(system);
    LOG_INFO("Destroyed terrain sculpting system");
}

void terrain_sculpting_set_heightmap(TerrainSculptingSystem* system, TerrainHeightmap* heightmap) {
    if (system) {
        system->heightmap = heightmap;
        terrain_sculpting_clear_history(system);
    }
}

// Brush management
void terrain_sculpting_set_brush_type(TerrainSculptingSystem* system, BrushType type) {
    if (system) {
        system->current_brush.type = type;
    }
}

void terrain_sculpting_set_operation(TerrainSculptingSystem* system, SculptOperation operation) {
    if (system) {
        system->current_brush.operation = operation;
    }
}

void terrain_sculpting_set_falloff(TerrainSculptingSystem* system, FalloffType falloff) {
    if (system) {
        system->current_brush.falloff = falloff;
    }
}

void terrain_sculpting_set_brush_radius(TerrainSculptingSystem* system, f32 radius) {
    if (system) {
        system->current_brush.radius = fmaxf(1.0f, radius);
    }
}

void terrain_sculpting_set_brush_strength(TerrainSculptingSystem* system, f32 strength) {
    if (system) {
        system->current_brush.strength = fclampf(strength, 0.0f, 1.0f);
    }
}

void terrain_sculpting_set_brush_hardness(TerrainSculptingSystem* system, f32 hardness) {
    if (system) {
        system->current_brush.hardness = fclampf(hardness, 0.0f, 1.0f);
    }
}

// Heightmap utilities
TerrainHeightmap* terrain_heightmap_create(u32 width, u32 height) {
    TerrainHeightmap* heightmap = malloc(sizeof(TerrainHeightmap));
    if (!heightmap) return NULL;
    
    memset(heightmap, 0, sizeof(TerrainHeightmap));
    
    heightmap->width = width;
    heightmap->height = height;
    heightmap->scale_x = 1.0f;
    heightmap->scale_z = 1.0f;
    heightmap->height_scale = 1.0f;
    
    // Allocate height data
    size_t height_count = width * height;
    heightmap->heights = malloc(sizeof(f32) * height_count);
    heightmap->visibility_mask = malloc(sizeof(u8) * height_count);
    heightmap->layer_weights = malloc(sizeof(f32) * height_count * 8); // 8 layers max
    
    if (!heightmap->heights || !heightmap->visibility_mask || !heightmap->layer_weights) {
        terrain_heightmap_destroy(heightmap);
        return NULL;
    }
    
    // Initialize to flat terrain
    for (size_t i = 0; i < height_count; i++) {
        heightmap->heights[i] = 0.0f;
        heightmap->visibility_mask[i] = 255; // Fully visible
        for (u32 j = 0; j < 8; j++) {
            heightmap->layer_weights[i * 8 + j] = (j == 0) ? 1.0f : 0.0f; // First layer active
        }
    }
    
    LOG_INFO("Created terrain heightmap: %ux%u", width, height);
    return heightmap;
}

void terrain_heightmap_destroy(TerrainHeightmap* heightmap) {
    if (!heightmap) return;
    
    free(heightmap->heights);
    free(heightmap->visibility_mask);
    free(heightmap->layer_weights);
    free(heightmap);
    
    LOG_INFO("Destroyed terrain heightmap");
}

Vec3 terrain_heightmap_world_to_pixel(TerrainHeightmap* heightmap, Vec3 world_pos) {
    Vec3 pixel;
    pixel.x = world_pos.x / heightmap->scale_x;
    pixel.y = world_pos.y / heightmap->height_scale;
    pixel.z = world_pos.z / heightmap->scale_z;
    return pixel;
}

Vec3 terrain_heightmap_pixel_to_world(TerrainHeightmap* heightmap, u32 x, u32 z) {
    Vec3 world;
    world.x = x * heightmap->scale_x;
    world.y = terrain_heightmap_get_height(heightmap, x, z) * heightmap->height_scale;
    world.z = z * heightmap->scale_z;
    return world;
}

f32 terrain_heightmap_get_height(TerrainHeightmap* heightmap, u32 x, u32 z) {
    if (!heightmap || x >= heightmap->width || z >= heightmap->height) {
        return 0.0f;
    }
    
    return heightmap->heights[z * heightmap->width + x];
}

void terrain_heightmap_set_height(TerrainHeightmap* heightmap, u32 x, u32 z, f32 height) {
    if (!heightmap || x >= heightmap->width || z >= heightmap->height) {
        return;
    }
    
    heightmap->heights[z * heightmap->width + x] = height;
}

// Brush utilities
f32 terrain_brush_get_falloff_weight(TerrainBrush* brush, f32 distance) {
    if (!brush || distance > brush->radius) {
        return 0.0f;
    }
    
    f32 normalized_distance = distance / brush->radius;
    f32 falloff_weight = get_falloff_weight(normalized_distance, brush->falloff);
    
    // Apply hardness
    if (brush->hardness < 1.0f) {
        f32 hardness_factor = 1.0f - brush->hardness;
        falloff_weight = falloff_weight * (1.0f - hardness_factor) + 
                       hardness_factor * falloff_weight * falloff_weight;
    }
    
    return falloff_weight;
}

bool terrain_brush_is_in_range(TerrainBrush* brush, Vec3 brush_pos, Vec3 test_pos) {
    if (!brush) return false;
    
    f32 distance = vec3_distance(brush_pos, test_pos);
    return distance <= brush->radius;
}

// Sculpting operations
bool terrain_sculpting_apply_brush(TerrainSculptingSystem* system, Vec3 world_position) {
    if (!system || !system->heightmap || !system->current_brush.is_active) {
        return false;
    }
    
    u64 start_time = get_time_ns();
    
    TerrainBrush* brush = &system->current_brush;
    TerrainHeightmap* heightmap = system->heightmap;
    
    // Convert world position to pixel coordinates
    Vec3 pixel_pos = terrain_heightmap_world_to_pixel(heightmap, world_position);
    
    // Calculate affected pixel range
    i32 start_x = (i32)fmaxf(0, pixel_pos.x - brush->radius);
    i32 end_x = (i32)fminf(heightmap->width - 1, pixel_pos.x + brush->radius);
    i32 start_z = (i32)fmaxf(0, pixel_pos.z - brush->radius);
    i32 end_z = (i32)fminf(heightmap->height - 1, pixel_pos.z + brush->radius);
    
    u32 pixels_modified = 0;
    
    // Capture history before modification for undo/redo
    terrain_sculpting_push_history(system);
    
    // Apply brush operation
    for (i32 z = start_z; z <= end_z; z++) {
        for (i32 x = start_x; x <= end_x; x++) {
            Vec3 test_world = terrain_heightmap_pixel_to_world(heightmap, x, z);
            f32 distance = vec3_distance(world_position, test_world);
            
            if (distance <= brush->radius) {
                f32 weight = terrain_brush_get_falloff_weight(brush, distance);
                f32 current_height = terrain_heightmap_get_height(heightmap, x, z);
                f32 new_height = current_height;
                
                switch (brush->operation) {
                    case SCULPT_RAISE:
                        new_height = current_height + brush->strength * weight;
                        break;
                        
                    case SCULPT_LOWER:
                        new_height = current_height - brush->strength * weight;
                        break;
                        
                    case SCULPT_FLATTEN:
                        new_height = lerp(current_height, brush->target_height, brush->strength * weight);
                        break;
                        
                    case SCULPT_SMOOTH:
                        {
                            // Average with neighbors
                            f32 sum = 0.0f;
                            u32 count = 0;
                            for (i32 dz = -1; dz <= 1; dz++) {
                                for (i32 dx = -1; dx <= 1; dx++) {
                                    i32 nx = x + dx;
                                    i32 nz = z + dz;
                                    if (nx >= 0 && nx < (i32)heightmap->width && 
                                        nz >= 0 && nz < (i32)heightmap->height) {
                                        sum += terrain_heightmap_get_height(heightmap, nx, nz);
                                        count++;
                                    }
                                }
                            }
                            if (count > 0) {
                                f32 average = sum / count;
                                new_height = lerp(current_height, average, brush->strength * weight);
                            }
                        }
                        break;
                        
                    default:
                        break;
                }
                
                terrain_heightmap_set_height(heightmap, x, z, new_height);
                pixels_modified++;
            }
        }
    }
    
    // Update performance stats
    u64 sculpt_time = get_time_ns() - start_time;
    system->last_sculpt_time = sculpt_time;
    system->pixels_modified_last_frame = pixels_modified;
    
    // Update average time
    static u32 frame_count = 0;
    static f32 total_time = 0.0f;
    frame_count++;
    total_time += sculpt_time / 1000000.0f; // Convert to milliseconds
    system->average_sculpt_time = total_time / frame_count;
    
    return pixels_modified > 0;
}

// Specific sculpting operations
void terrain_sculpt_raise_height(TerrainSculptingSystem* system, Vec3 position, f32 strength) {
    if (system) {
        terrain_sculpting_set_operation(system, SCULPT_RAISE);
        terrain_sculpting_set_brush_strength(system, strength);
        terrain_sculpting_apply_brush(system, position);
    }
}

void terrain_sculpt_lower_height(TerrainSculptingSystem* system, Vec3 position, f32 strength) {
    if (system) {
        terrain_sculpting_set_operation(system, SCULPT_LOWER);
        terrain_sculpting_set_brush_strength(system, strength);
        terrain_sculpting_apply_brush(system, position);
    }
}

void terrain_sculpt_flatten_height(TerrainSculptingSystem* system, Vec3 position, f32 target_height) {
    if (system) {
        terrain_sculpting_set_operation(system, SCULPT_FLATTEN);
        system->current_brush.target_height = target_height;
        terrain_sculpting_apply_brush(system, position);
    }
}

void terrain_sculpt_smooth_height(TerrainSculptingSystem* system, Vec3 position, f32 radius) {
    if (system) {
        terrain_sculpting_set_operation(system, SCULPT_SMOOTH);
        terrain_sculpting_set_brush_radius(system, radius);
        terrain_sculpting_apply_brush(system, position);
    }
}

// Brush blending algorithms
f32 terrain_blend_linear(f32 current, f32 target, f32 weight) {
    return lerp(current, target, weight);
}

f32 terrain_blend_smooth(f32 current, f32 target, f32 weight) {
    f32 t = weight * weight * (3.0f - 2.0f * weight);
    return lerp(current, target, t);
}

f32 terrain_blend_additive(f32 current, f32 target, f32 weight) {
    return current + target * weight;
}

f32 terrain_blend_subtractive(f32 current, f32 target, f32 weight) {
    return current - target * weight;
}

// Undo/redo system
void terrain_sculpting_clear_history(TerrainSculptingSystem* system) {
    if (!system) return;
    
    for (u32 i = 0; i < system->history_size; i++) {
        free(system->history[i].height_data);
        free(system->history[i].modified_pixels);
    }
    
    system->history_size = 0;
    system->current_history_index = -1;
}

bool terrain_sculpting_can_undo(TerrainSculptingSystem* system) {
    return system && system->current_history_index >= 0;
}

bool terrain_sculpting_can_redo(TerrainSculptingSystem* system) {
    return system && system->current_history_index < (i32)system->history_size - 1;
}

//  IMPLEMENTED: Full undo/redo system
void terrain_sculpting_push_history(TerrainSculptingSystem* system) {
    if (!system || !system->heightmap) return;
    
    TerrainHeightmap* heightmap = system->heightmap;
    size_t data_size = heightmap->width * heightmap->height * sizeof(f32);
    
    // Clear any redo history beyond current point
    for (u32 i = system->current_history_index + 1; i < system->history_size; i++) {
        free(system->history[i].height_data);
        free(system->history[i].modified_pixels);
    }
    
    // If at capacity, remove oldest entry
    if (system->history_size >= system->history_capacity) {
        free(system->history[0].height_data);
        free(system->history[0].modified_pixels);
        
        // Shift all entries down
        for (u32 i = 0; i < system->history_capacity - 1; i++) {
            system->history[i] = system->history[i + 1];
        }
        system->history_size = system->history_capacity - 1;
        system->current_history_index = system->history_size - 1;
    }
    
    // Allocate new history entry
    u32 new_index = system->current_history_index + 1;
    SculptHistoryEntry* entry = &system->history[new_index];
    
    entry->height_data = malloc(data_size);
    if (!entry->height_data) {
        LOG_ERROR("Failed to allocate history entry");
        return;
    }
    
    // Copy current heightmap data
    memcpy(entry->height_data, heightmap->heights, data_size);
    entry->data_size = data_size;
    entry->pixel_count = heightmap->width * heightmap->height;
    entry->modified_pixels = NULL;  // Could optimize by tracking only modified pixels
    
    // Update indices
    system->current_history_index = new_index;
    system->history_size = new_index + 1;
    
    LOG_INFO("Pushed history entry %u (total: %u)", new_index, system->history_size);
}

bool terrain_sculpting_undo(TerrainSculptingSystem* system) {
    if (!terrain_sculpting_can_undo(system)) return false;
    
    TerrainHeightmap* heightmap = system->heightmap;
    if (!heightmap) return false;
    
    // Get history entry at current index
    SculptHistoryEntry* entry = &system->history[system->current_history_index];
    
    // Restore heightmap data
    if (entry->height_data && entry->data_size > 0) {
        memcpy(heightmap->heights, entry->height_data, entry->data_size);
        system->current_history_index--;
        
        LOG_INFO("Undo: restored to history entry %u", system->current_history_index);
        return true;
    }
    
    LOG_ERROR("Undo failed: invalid history entry");
    return false;
}

bool terrain_sculpting_redo(TerrainSculptingSystem* system) {
    if (!terrain_sculpting_can_redo(system)) return false;
    
    TerrainHeightmap* heightmap = system->heightmap;
    if (!heightmap) return false;
    
    // Move forward in history
    system->current_history_index++;
    SculptHistoryEntry* entry = &system->history[system->current_history_index];
    
    // Restore heightmap data
    if (entry->height_data && entry->data_size > 0) {
        memcpy(heightmap->heights, entry->height_data, entry->data_size);
        
        LOG_INFO("Redo: restored to history entry %u", system->current_history_index);
        return true;
    }
    
    LOG_ERROR("Redo failed: invalid history entry");
    system->current_history_index--;
    return false;
}

// Performance and statistics
void terrain_sculpting_get_stats(TerrainSculptingSystem* system, u32* history_size, 
                                f32* average_time, u32* pixels_last_frame) {
    if (!system) return;
    
    if (history_size) *history_size = system->history_size;
    if (average_time) *average_time = system->average_sculpt_time;
    if (pixels_last_frame) *pixels_last_frame = system->pixels_modified_last_frame;
}

// Debug utilities
void terrain_sculpting_print_brush_info(TerrainBrush* brush) {
    if (!brush) return;
    
    printf("=== Terrain Brush Info ===\n");
    printf("Type: %d\n", brush->type);
    printf("Operation: %d\n", brush->operation);
    printf("Falloff: %d\n", brush->falloff);
    printf("Radius: %.2f\n", brush->radius);
    printf("Strength: %.2f\n", brush->strength);
    printf("Hardness: %.2f\n", brush->hardness);
    printf("Active: %s\n", brush->is_active ? "Yes" : "No");
}

void terrain_sculpting_print_heightmap_info(TerrainHeightmap* heightmap) {
    if (!heightmap) return;
    
    printf("=== Terrain Heightmap Info ===\n");
    printf("Size: %ux%u\n", heightmap->width, heightmap->height);
    printf("Scale: X=%.2f, Z=%.2f, Height=%.2f\n", 
           heightmap->scale_x, heightmap->scale_z, heightmap->height_scale);
    
    // Calculate min/max heights
    f32 min_height = FLT_MAX;
    f32 max_height = -FLT_MAX;
    f32 avg_height = 0.0f;
    
    for (u32 i = 0; i < heightmap->width * heightmap->height; i++) {
        f32 h = heightmap->heights[i];
        min_height = fminf(min_height, h);
        max_height = fmaxf(max_height, h);
        avg_height += h;
    }
    
    avg_height /= (heightmap->width * heightmap->height);
    
    printf("Height Range: %.2f to %.2f (avg: %.2f)\n", min_height, max_height, avg_height);
}

// Terrain Import/Export
#include "terrain_io.c"
