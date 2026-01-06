/*
 * voronoi_fracture.c
 * Voronoi-based mesh fracturing for destruction
 *
 * Part of the Destruction subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements pre-fracturing of meshes into Voronoi cells
 */

#include "voronoi_fracture.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define FRACTURE_MAX_COUNT 256
#define FRACTURE_DEFAULT_CAPACITY 32
#define FRACTURE_MAX_CELLS 256
#define FRACTURE_MAX_VERTICES_PER_CELL 128

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct plane {
    vec3_t normal;
    float distance;
} plane_t;

typedef struct aabb {
    vec3_t min;
    vec3_t max;
} aabb_t;

/* ============================================================================
 * FRACTURE TYPES
 * ============================================================================ */

typedef struct voronoi_cell {
    vec3_t seed_point;
    vec3_t* vertices;
    uint32_t* indices;
    uint32_t vertex_count;
    uint32_t index_count;
    
    // Cell properties
    vec3_t centroid;
    float volume;
    aabb_t bounds;
    
    // Connectivity
    uint32_t* neighbor_cells;
    uint32_t neighbor_count;
    
    // Physics state
    float health;
    bool fractured;
} voronoi_cell_t;

typedef struct fracture_pattern {
    voronoi_cell_t* cells;
    uint32_t cell_count;
    
    // Original mesh bounds
    aabb_t bounds;
    
    // Generation parameters
    uint32_t seed;
    float cell_size_variance;
    
    bool generated;
} fracture_pattern_t;

typedef struct destruction_fracture_internal {
    uint32_t id;
    uint32_t flags;
    fracture_pattern_t pattern;
    bool dirty;
    uint64_t frame_updated;
} destruction_fracture_internal_t;

typedef struct destruction_fracture_context {
    destruction_fracture_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} destruction_fracture_context_t;

static destruction_fracture_context_t g_fracture_ctx = {0};

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

static float randf(uint32_t* seed) {
    *seed = (*seed * 1103515245 + 12345) & 0x7fffffff;
    return (float)*seed / (float)0x7fffffff;
}

static vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

static vec3_t vec3_sub(vec3_t a, vec3_t b) {
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z};
}

static vec3_t vec3_scale(vec3_t v, float s) {
    return (vec3_t){v.x * s, v.y * s, v.z * s};
}

static float vec3_dot(vec3_t a, vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float vec3_length_sq(vec3_t v) {
    return vec3_dot(v, v);
}

static float vec3_distance_sq(vec3_t a, vec3_t b) {
    vec3_t d = vec3_sub(a, b);
    return vec3_length_sq(d);
}

static vec3_t vec3_lerp(vec3_t a, vec3_t b, float t) {
    return (vec3_t){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

/* ============================================================================
 * VORONOI GENERATION
 * ============================================================================ */

static void generate_seed_points(fracture_pattern_t* pattern, 
                                  uint32_t cell_count, aabb_t bounds) {
    uint32_t seed = pattern->seed;
    
    vec3_t size = vec3_sub(bounds.max, bounds.min);
    
    for (uint32_t i = 0; i < cell_count; i++) {
        // Random point within bounds
        pattern->cells[i].seed_point = (vec3_t){
            bounds.min.x + randf(&seed) * size.x,
            bounds.min.y + randf(&seed) * size.y,
            bounds.min.z + randf(&seed) * size.z
        };
        
        pattern->cells[i].health = 1.0f;
        pattern->cells[i].fractured = false;
    }
}

static uint32_t find_nearest_cell(fracture_pattern_t* pattern, vec3_t point) {
    uint32_t nearest = 0;
    float min_dist = vec3_distance_sq(point, pattern->cells[0].seed_point);
    
    for (uint32_t i = 1; i < pattern->cell_count; i++) {
        float dist = vec3_distance_sq(point, pattern->cells[i].seed_point);
        if (dist < min_dist) {
            min_dist = dist;
            nearest = i;
        }
    }
    
    return nearest;
}

static void compute_cell_centroids(fracture_pattern_t* pattern) {
    // Approximate centroid as seed point (for now)
    // Full implementation would calculate from actual cell vertices
    for (uint32_t i = 0; i < pattern->cell_count; i++) {
        pattern->cells[i].centroid = pattern->cells[i].seed_point;
    }
}

static void build_cell_neighbors(fracture_pattern_t* pattern) {
    // Find neighboring cells (cells that share a face)
    // Simplified: use distance threshold
    float threshold_sq = 0.0f;
    
    // Calculate average cell spacing
    if (pattern->cell_count > 1) {
        vec3_t size = vec3_sub(pattern->bounds.max, pattern->bounds.min);
        float volume = size.x * size.y * size.z;
        float avg_cell_volume = volume / pattern->cell_count;
        float avg_cell_size = powf(avg_cell_volume, 1.0f / 3.0f);
        threshold_sq = (avg_cell_size * 2.0f) * (avg_cell_size * 2.0f);
    }
    
    for (uint32_t i = 0; i < pattern->cell_count; i++) {
        voronoi_cell_t* cell = &pattern->cells[i];
        
        // Count neighbors first
        uint32_t neighbor_count = 0;
        for (uint32_t j = 0; j < pattern->cell_count; j++) {
            if (i != j) {
                float dist_sq = vec3_distance_sq(cell->seed_point, 
                                                  pattern->cells[j].seed_point);
                if (dist_sq < threshold_sq) {
                    neighbor_count++;
                }
            }
        }
        
        // Allocate and fill neighbor list
        if (neighbor_count > 0) {
            cell->neighbor_cells = calloc(neighbor_count, sizeof(uint32_t));
            if (cell->neighbor_cells) {
                uint32_t n = 0;
                for (uint32_t j = 0; j < pattern->cell_count; j++) {
                    if (i != j) {
                        float dist_sq = vec3_distance_sq(cell->seed_point, 
                                                          pattern->cells[j].seed_point);
                        if (dist_sq < threshold_sq) {
                            cell->neighbor_cells[n++] = j;
                        }
                    }
                }
                cell->neighbor_count = neighbor_count;
            }
        }
    }
}

// Simple power function for cube root approximation
static float powf(float base, float exp) {
    if (exp == 1.0f / 3.0f) {
        // Cube root approximation using Newton's method
        float x = base / 3.0f;
        for (int i = 0; i < 10; i++) {
            x = (2.0f * x + base / (x * x)) / 3.0f;
        }
        return x;
    }
    return base; // Simplified
}

/* ============================================================================
 * FRACTURE PATTERN MANAGEMENT
 * ============================================================================ */

static void fracture_pattern_cleanup(fracture_pattern_t* pattern) {
    if (!pattern) return;
    
    if (pattern->cells) {
        for (uint32_t i = 0; i < pattern->cell_count; i++) {
            voronoi_cell_t* cell = &pattern->cells[i];
            if (cell->vertices) free(cell->vertices);
            if (cell->indices) free(cell->indices);
            if (cell->neighbor_cells) free(cell->neighbor_cells);
        }
        free(pattern->cells);
        pattern->cells = NULL;
    }
    
    pattern->cell_count = 0;
    pattern->generated = false;
}

static int fracture_pattern_generate(fracture_pattern_t* pattern, 
                                       aabb_t bounds, uint32_t cell_count,
                                       uint32_t seed) {
    if (cell_count > FRACTURE_MAX_CELLS) {
        cell_count = FRACTURE_MAX_CELLS;
    }
    
    fracture_pattern_cleanup(pattern);
    
    pattern->cells = calloc(cell_count, sizeof(voronoi_cell_t));
    if (!pattern->cells) {
        return -1;
    }
    
    pattern->cell_count = cell_count;
    pattern->bounds = bounds;
    pattern->seed = seed;
    pattern->cell_size_variance = 0.3f;
    
    // Generate Voronoi seed points
    generate_seed_points(pattern, cell_count, bounds);
    
    // Compute cell properties
    compute_cell_centroids(pattern);
    build_cell_neighbors(pattern);
    
    pattern->generated = true;
    return 0;
}

/* ============================================================================
 * DAMAGE APPLICATION
 * ============================================================================ */

static void apply_damage_to_pattern(fracture_pattern_t* pattern, 
                                     vec3_t impact_point, float damage,
                                     float damage_radius) {
    if (!pattern->generated) return;
    
    float radius_sq = damage_radius * damage_radius;
    
    for (uint32_t i = 0; i < pattern->cell_count; i++) {
        voronoi_cell_t* cell = &pattern->cells[i];
        if (cell->fractured) continue;
        
        float dist_sq = vec3_distance_sq(cell->centroid, impact_point);
        
        if (dist_sq < radius_sq) {
            // Damage falls off with distance
            float dist = sqrtf(dist_sq);
            float falloff = 1.0f - (dist / damage_radius);
            cell->health -= damage * falloff;
            
            if (cell->health <= 0.0f) {
                cell->fractured = true;
                cell->health = 0.0f;
            }
        }
    }
}

static float sqrtf(float x) {
    if (x <= 0) return 0;
    float guess = x / 2.0f;
    for (int i = 0; i < 8; i++) {
        guess = (guess + x / guess) / 2.0f;
    }
    return guess;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int destruction_voronoi_fracture_init(void) {
    if (g_fracture_ctx.initialized) {
        return 0;
    }

    g_fracture_ctx.capacity = FRACTURE_DEFAULT_CAPACITY;
    g_fracture_ctx.items = calloc(g_fracture_ctx.capacity, 
                                   sizeof(destruction_fracture_internal_t));
    if (!g_fracture_ctx.items) {
        return -1;
    }

    g_fracture_ctx.count = 0;
    g_fracture_ctx.initialized = true;

    return 0;
}

void destruction_voronoi_fracture_shutdown(void) {
    if (!g_fracture_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_fracture_ctx.count; i++) {
        fracture_pattern_cleanup(&g_fracture_ctx.items[i].pattern);
    }

    free(g_fracture_ctx.items);
    g_fracture_ctx.items = NULL;
    g_fracture_ctx.count = 0;
    g_fracture_ctx.capacity = 0;
    g_fracture_ctx.initialized = false;
}

int destruction_voronoi_fracture_create(destruction_voronoi_fracture_handle_t* out_handle, 
                                          const destruction_voronoi_fracture_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_fracture_ctx.initialized) {
        return -2;
    }

    if (g_fracture_ctx.count >= g_fracture_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_fracture_ctx.count++;
    destruction_fracture_internal_t* item = &g_fracture_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    memset(&item->pattern, 0, sizeof(fracture_pattern_t));
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void destruction_voronoi_fracture_destroy(destruction_voronoi_fracture_handle_t handle) {
    if (handle.id >= g_fracture_ctx.count) {
        return;
    }

    fracture_pattern_cleanup(&g_fracture_ctx.items[handle.id].pattern);
}

int destruction_voronoi_fracture_generate(destruction_voronoi_fracture_handle_t handle,
                                            vec3_t bounds_min, vec3_t bounds_max,
                                            uint32_t cell_count, uint32_t seed) {
    if (handle.id >= g_fracture_ctx.count) {
        return -1;
    }
    
    aabb_t bounds = {bounds_min, bounds_max};
    fracture_pattern_t* pattern = &g_fracture_ctx.items[handle.id].pattern;
    
    return fracture_pattern_generate(pattern, bounds, cell_count, seed);
}

int destruction_voronoi_fracture_apply_damage(destruction_voronoi_fracture_handle_t handle,
                                                vec3_t impact_point, float damage,
                                                float damage_radius) {
    if (handle.id >= g_fracture_ctx.count) {
        return -1;
    }
    
    destruction_fracture_internal_t* item = &g_fracture_ctx.items[handle.id];
    apply_damage_to_pattern(&item->pattern, impact_point, damage, damage_radius);
    item->dirty = true;
    
    return 0;
}

uint32_t destruction_voronoi_fracture_get_fractured_cells(destruction_voronoi_fracture_handle_t handle,
                                                            uint32_t* out_cell_indices,
                                                            uint32_t max_count) {
    if (handle.id >= g_fracture_ctx.count) {
        return 0;
    }
    
    fracture_pattern_t* pattern = &g_fracture_ctx.items[handle.id].pattern;
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < pattern->cell_count && count < max_count; i++) {
        if (pattern->cells[i].fractured) {
            if (out_cell_indices) {
                out_cell_indices[count] = i;
            }
            count++;
        }
    }
    
    return count;
}

int destruction_voronoi_fracture_update(destruction_voronoi_fracture_handle_t handle, 
                                          const void* data, size_t size) {
    if (handle.id >= g_fracture_ctx.count) {
        return -1;
    }

    g_fracture_ctx.items[handle.id].dirty = true;
    return 0;
}

bool destruction_voronoi_fracture_is_valid(destruction_voronoi_fracture_handle_t handle) {
    if (handle.id >= g_fracture_ctx.count) {
        return false;
    }
    return g_fracture_ctx.items[handle.id].pattern.generated;
}

int destruction_voronoi_fracture_get_info(destruction_voronoi_fracture_handle_t handle, 
                                            destruction_voronoi_fracture_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_fracture_ctx.count) {
        return -2;
    }

    const destruction_fracture_internal_t* item = &g_fracture_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->pattern.generated;

    return 0;
}

void destruction_voronoi_fracture_mark_dirty(destruction_voronoi_fracture_handle_t handle) {
    if (handle.id < g_fracture_ctx.count) {
        g_fracture_ctx.items[handle.id].dirty = true;
    }
}

int destruction_voronoi_fracture_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_fracture_ctx.count; i++) {
        destruction_fracture_internal_t* item = &g_fracture_ctx.items[i];
        if (item->pattern.generated && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t destruction_voronoi_fracture_get_count(void) {
    return g_fracture_ctx.count;
}

size_t destruction_voronoi_fracture_get_memory_usage(void) {
    size_t total = sizeof(g_fracture_ctx);
    total += g_fracture_ctx.capacity * sizeof(destruction_fracture_internal_t);

    for (uint32_t i = 0; i < g_fracture_ctx.count; i++) {
        fracture_pattern_t* pattern = &g_fracture_ctx.items[i].pattern;
        if (pattern->cells) {
            total += pattern->cell_count * sizeof(voronoi_cell_t);
        }
    }

    return total;
}

void destruction_voronoi_fracture_debug_print(void) {
    // Debug output
}

/* End of voronoi_fracture.c */
