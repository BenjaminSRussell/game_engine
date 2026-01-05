#include "editor/terrain_editor/brush_system.h"
#include "core/memory.h"
#include "core/logger.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define MAX_UNDO_STATES 50

// Simple noise function (Perlin-like)
static float noise2d(float x, float y) {
    int n = (int)x + (int)y * 57;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

void brush_system_init(BrushSystem* system, TerrainData* terrain) {
    if (!system) return;
    memset(system, 0, sizeof(BrushSystem));
    
    system->active_terrain = terrain;
    
    // Default brush config
    system->config.operation = BRUSH_OP_RAISE;
    system->config.shape = BRUSH_SHAPE_CIRCLE;
    system->config.falloff = BRUSH_FALLOFF_SMOOTH;
    system->config.radius = 10.0f;
    system->config.strength = 1.0f;
    system->config.opacity = 1.0f;
    
    // Initialize undo system
    system->undo_system.undo_capacity = MAX_UNDO_STATES;
    system->undo_system.undo_stack = (TerrainUndoState*)calloc(MAX_UNDO_STATES, sizeof(TerrainUndoState));
    
    LOG_INFO("Brush system initialized");
}

void brush_system_shutdown(BrushSystem* system) {
    if (!system) return;
    
    // Free undo states
    for (u32 i = 0; i < system->undo_system.undo_count; i++) {
        if (system->undo_system.undo_stack[i].height_backup) {
            free(system->undo_system.undo_stack[i].height_backup);
        }
    }
    free(system->undo_system.undo_stack);
    
    memset(system, 0, sizeof(BrushSystem));
}

void brush_set_operation(BrushSystem* system, BrushOperation op) {
    if (system) system->config.operation = op;
}

void brush_set_radius(BrushSystem* system, float radius) {
    if (system) system->config.radius = fmaxf(0.1f, radius);
}

void brush_set_strength(BrushSystem* system, float strength) {
    if (system) system->config.strength = fmaxf(0.0f, fminf(10.0f, strength));
}

void brush_set_falloff(BrushSystem* system, BrushFalloff falloff) {
    if (system) system->config.falloff = falloff;
}

float brush_get_falloff(BrushSystem* system, float distance_from_center) {
    if (!system) return 0.0f;
    
    float radius = system->config.radius;
    if (distance_from_center >= radius) return 0.0f;
    
    float t = 1.0f - (distance_from_center / radius);
    
    switch (system->config.falloff) {
        case BRUSH_FALLOFF_LINEAR:
            return t;
        
        case BRUSH_FALLOFF_SMOOTH:
            return t * t * (3.0f - 2.0f * t); // Smoothstep
        
        case BRUSH_FALLOFF_SPHERICAL:
            return sqrtf(1.0f - (1.0f - t) * (1.0f - t));
        
        case BRUSH_FALLOFF_TIP:
            return t * t * t; // Cubic falloff
        
        default:
            return t;
    }
}

void brush_get_affected_region(BrushSystem* system, Vec2 center, u32* x_min, u32* x_max, u32* y_min, u32* y_max) {
    if (!system || !system->active_terrain) return;
    
    TerrainData* terrain = system->active_terrain;
    float radius = system->config.radius;
    
    // Convert world space to grid space
    int center_x = (int)(center.x / terrain->unit_size);
    int center_y = (int)(center.y / terrain->unit_size);
    int radius_cells = (int)(radius / terrain->unit_size) + 1;
    
    *x_min = (u32)fmaxf(0, center_x - radius_cells);
    *x_max = (u32)fminf(terrain->width - 1, center_x + radius_cells);
    *y_min = (u32)fmaxf(0, center_y - radius_cells);
    *y_max = (u32)fminf(terrain->height - 1, center_y + radius_cells);
}

static void save_undo_state(BrushSystem* system, u32 x_min, u32 x_max, u32 y_min, u32 y_max) {
    if (!system || !system->active_terrain) return;
    
    TerrainUndoSystem* undo = &system->undo_system;
    
    // Truncate redo history
    if (undo->undo_index < undo->undo_count) {
        for (u32 i = undo->undo_index; i < undo->undo_count; i++) {
            if (undo->undo_stack[i].height_backup) {
                free(undo->undo_stack[i].height_backup);
            }
        }
        undo->undo_count = undo->undo_index;
    }
    
    // Remove oldest if at capacity
    if (undo->undo_count >= undo->undo_capacity) {
        if (undo->undo_stack[0].height_backup) {
            free(undo->undo_stack[0].height_backup);
        }
        memmove(&undo->undo_stack[0], &undo->undo_stack[1], 
                (undo->undo_capacity - 1) * sizeof(TerrainUndoState));
        undo->undo_count--;
        undo->undo_index--;
    }
    
    // Create new undo state
    TerrainUndoState* state = &undo->undo_stack[undo->undo_count];
    state->x_min = x_min;
    state->x_max = x_max;
    state->y_min = y_min;
    state->y_max = y_max;
    state->timestamp = (u64)time(NULL);
    
    // Backup affected region
    u32 width = x_max - x_min + 1;
    u32 height = y_max - y_min + 1;
    state->height_backup = (f32*)malloc(width * height * sizeof(f32));
    
    TerrainData* terrain = system->active_terrain;
    for (u32 y = y_min; y <= y_max; y++) {
        for (u32 x = x_min; x <= x_max; x++) {
            u32 src_idx = y * terrain->width + x;
            u32 dst_idx = (y - y_min) * width + (x - x_min);
            state->height_backup[dst_idx] = terrain->height_data[src_idx];
        }
    }
    
    undo->undo_count++;
    undo->undo_index = undo->undo_count;
}

void brush_begin_stroke(BrushSystem* system, Vec2 world_pos) {
    if (!system) return;
    
    system->is_stroking = true;
    system->last_stroke_pos = world_pos;
    
    // Save undo state for entire stroke
    u32 x_min, x_max, y_min, y_max;
    brush_get_affected_region(system, world_pos, &x_min, &x_max, &y_min, &y_max);
    save_undo_state(system, x_min, x_max, y_min, y_max);
}

void brush_update_stroke(BrushSystem* system, Vec2 world_pos) {
    if (!system || !system->is_stroking) return;
    
    float delta_time = 0.016f; // Assume 60fps brush update
    
    switch (system->config.operation) {
        case BRUSH_OP_RAISE:
            brush_apply_raise(system, world_pos, delta_time);
            break;
        case BRUSH_OP_LOWER:
            brush_apply_lower(system, world_pos, delta_time);
            break;
        case BRUSH_OP_FLATTEN:
            brush_apply_flatten(system, world_pos, system->config.target_height);
            break;
        case BRUSH_OP_SMOOTH:
            brush_apply_smooth(system, world_pos, 1);
            break;
        case BRUSH_OP_NOISE:
            brush_apply_noise(system, world_pos);
            break;
        case BRUSH_OP_EROSION:
            brush_apply_erosion(system, world_pos);
            break;
        default:
            break;
    }
    
    system->last_stroke_pos = world_pos;
}

void brush_end_stroke(BrushSystem* system) {
    if (!system) return;
    system->is_stroking = false;
}

void brush_apply_raise(BrushSystem* system, Vec2 center, float delta_time) {
    if (!system || !system->active_terrain) return;
    
    TerrainData* terrain = system->active_terrain;
    u32 x_min, x_max, y_min, y_max;
    brush_get_affected_region(system, center, &x_min, &x_max, &y_min, &y_max);
    
    for (u32 y = y_min; y <= y_max; y++) {
        for (u32 x = x_min; x <= x_max; x++) {
            Vec2 grid_pos = {x * terrain->unit_size, y * terrain->unit_size};
            float dx = grid_pos.x - center.x;
            float dy = grid_pos.y - center.y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            float falloff = brush_get_falloff(system, dist);
            if (falloff > 0.0f) {
                u32 idx = y * terrain->width + x;
                terrain->height_data[idx] += system->config.strength * falloff * delta_time * 10.0f;
            }
        }
    }
}

void brush_apply_lower(BrushSystem* system, Vec2 center, float delta_time) {
    if (!system || !system->active_terrain) return;
    
    TerrainData* terrain = system->active_terrain;
    u32 x_min, x_max, y_min, y_max;
    brush_get_affected_region(system, center, &x_min, &x_max, &y_min, &y_max);
    
    for (u32 y = y_min; y <= y_max; y++) {
        for (u32 x = x_min; x <= x_max; x++) {
            Vec2 grid_pos = {x * terrain->unit_size, y * terrain->unit_size};
            float dx = grid_pos.x - center.x;
            float dy = grid_pos.y - center.y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            float falloff = brush_get_falloff(system, dist);
            if (falloff > 0.0f) {
                u32 idx = y * terrain->width + x;
                terrain->height_data[idx] -= system->config.strength * falloff * delta_time * 10.0f;
            }
        }
    }
}

void brush_apply_flatten(BrushSystem* system, Vec2 center, float target_height) {
    if (!system || !system->active_terrain) return;
    
    TerrainData* terrain = system->active_terrain;
    u32 x_min, x_max, y_min, y_max;
    brush_get_affected_region(system, center, &x_min, &x_max, &y_min, &y_max);
    
    for (u32 y = y_min; y <= y_max; y++) {
        for (u32 x = x_min; x <= x_max; x++) {
            Vec2 grid_pos = {x * terrain->unit_size, y * terrain->unit_size};
            float dx = grid_pos.x - center.x;
            float dy = grid_pos.y - center.y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            float falloff = brush_get_falloff(system, dist);
            if (falloff > 0.0f) {
                u32 idx = y * terrain->width + x;
                float current = terrain->height_data[idx];
                terrain->height_data[idx] = current + (target_height - current) * falloff * system->config.strength * 0.1f;
            }
        }
    }
}

void brush_apply_smooth(BrushSystem* system, Vec2 center, u32 iterations) {
    if (!system || !system->active_terrain) return;
    
    TerrainData* terrain = system->active_terrain;
    u32 x_min, x_max, y_min, y_max;
    brush_get_affected_region(system, center, &x_min, &x_max, &y_min, &y_max);
    
    for (u32 iter = 0; iter < iterations; iter++) {
        for (u32 y = y_min; y <= y_max; y++) {
            for (u32 x = x_min; x <= x_max; x++) {
                Vec2 grid_pos = {x * terrain->unit_size, y * terrain->unit_size};
                float dx = grid_pos.x - center.x;
                float dy = grid_pos.y - center.y;
                float dist = sqrtf(dx * dx + dy * dy);
                
                float falloff = brush_get_falloff(system, dist);
                if (falloff > 0.0f) {
                    // Average with neighbors
                    float sum = 0.0f;
                    u32 count = 0;
                    
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            int nx = x + dx;
                            int ny = y + dy;
                            if (nx >= 0 && nx < (int)terrain->width && ny >= 0 && ny < (int)terrain->height) {
                                sum += terrain->height_data[ny * terrain->width + nx];
                                count++;
                            }
                        }
                    }
                    
                    u32 idx = y * terrain->width + x;
                    float avg = sum / count;
                    terrain->height_data[idx] = terrain->height_data[idx] + (avg - terrain->height_data[idx]) * falloff * system->config.strength * 0.5f;
                }
            }
        }
    }
}

void brush_apply_noise(BrushSystem* system, Vec2 center) {
    if (!system || !system->active_terrain) return;
    
    TerrainData* terrain = system->active_terrain;
    u32 x_min, x_max, y_min, y_max;
    brush_get_affected_region(system, center, &x_min, &x_max, &y_min, &y_max);
    
    float scale = system->config.noise_scale > 0.0f ? system->config.noise_scale : 0.1f;
    
    for (u32 y = y_min; y <= y_max; y++) {
        for (u32 x = x_min; x <= x_max; x++) {
            Vec2 grid_pos = {x * terrain->unit_size, y * terrain->unit_size};
            float dx = grid_pos.x - center.x;
            float dy = grid_pos.y - center.y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            float falloff = brush_get_falloff(system, dist);
            if (falloff > 0.0f) {
                u32 idx = y * terrain->width + x;
                float noise_val = noise2d(x * scale, y * scale);
                terrain->height_data[idx] += noise_val * system->config.strength * falloff;
            }
        }
    }
}

void brush_apply_erosion(BrushSystem* system, Vec2 center) {
    if (!system || !system->active_terrain) return;
    
    TerrainData* terrain = system->active_terrain;
    u32 x_min, x_max, y_min, y_max;
    brush_get_affected_region(system, center, &x_min, &x_max, &y_min, &y_max);
    
    // Simplified hydraulic erosion: move height from high to low neighbors
    for (u32 y = y_min; y <= y_max; y++) {
        for (u32 x = x_min; x <= x_max; x++) {
            Vec2 grid_pos = {x * terrain->unit_size, y * terrain->unit_size};
            float dx = grid_pos.x - center.x;
            float dy = grid_pos.y - center.y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            float falloff = brush_get_falloff(system, dist);
            if (falloff > 0.0f) {
                u32 idx = y * terrain->width + x;
                float current_height = terrain->height_data[idx];
                
                // Find lowest neighbor
                float min_neighbor_height = current_height;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < (int)terrain->width && ny >= 0 && ny < (int)terrain->height) {
                            float neighbor_height = terrain->height_data[ny * terrain->width + nx];
                            if (neighbor_height < min_neighbor_height) {
                                min_neighbor_height = neighbor_height;
                            }
                        }
                    }
                }
                
                // Erode towards lowest neighbor
                float erosion_amount = (current_height - min_neighbor_height) * system->config.erosion_strength * falloff * 0.1f;
                terrain->height_data[idx] -= erosion_amount;
            }
        }
    }
}

void brush_undo(BrushSystem* system) {
    if (!system || !brush_can_undo(system)) return;
    
    TerrainUndoSystem* undo = &system->undo_system;
    undo->undo_index--;
    
    TerrainUndoState* state = &undo->undo_stack[undo->undo_index];
    TerrainData* terrain = system->active_terrain;
    
    // Restore backup
    u32 width = state->x_max - state->x_min + 1;
    for (u32 y = state->y_min; y <= state->y_max; y++) {
        for (u32 x = state->x_min; x <= state->x_max; x++) {
            u32 dst_idx = y * terrain->width + x;
            u32 src_idx = (y - state->y_min) * width + (x - state->x_min);
            terrain->height_data[dst_idx] = state->height_backup[src_idx];
        }
    }
    
    LOG_INFO("Undo terrain edit");
}

void brush_redo(BrushSystem* system) {
    if (!system || !brush_can_redo(system)) return;
    
    // Simplified redo: would need to save "after" state
    // For now, just increment index
    system->undo_system.undo_index++;
    LOG_INFO("Redo terrain edit");
}

bool brush_can_undo(BrushSystem* system) {
    return system && system->undo_system.undo_index > 0;
}

bool brush_can_redo(BrushSystem* system) {
    return system && system->undo_system.undo_index < system->undo_system.undo_count;
}
